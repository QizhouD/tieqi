#include "app/Formation.h"

#include "app/Pieces.h"
#include "core/math/Geometry.h"
#include "core/rules/Constants.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <numbers>

namespace tq {
namespace {

constexpr float kPi = std::numbers::pi_v<float>;
// How fast a soldier turns, radians per battlefield second.
constexpr float kTurnRate = 4.f;
// Gait: metres per step of the move cycle.
constexpr float kStrideCavalry = 1.6f;
constexpr float kStrideInfantry = 0.9f;
// A charge reaches its full bulge and lead after this many metres.
constexpr float kChargeFullRun = 120.f;
constexpr float kStrikeRate = 1.5f;
// Buckets of the per-unit spatial hash; a power of two.
constexpr int kHashBuckets = 256;

Vec2 rotated(Vec2 origin, float facing, Vec2 local) {
    const float c = std::cos(facing);
    const float s = std::sin(facing);
    return {origin.x + local.x * c - local.y * s, origin.y + local.x * s + local.y * c};
}

Vec2 toLocal(Vec2 world, Vec2 origin, float facing) {
    const Vec2 d = world - origin;
    const float c = std::cos(facing);
    const float s = std::sin(facing);
    return {d.x * c + d.y * s, -d.x * s + d.y * c};
}

float length(Vec2 v) { return std::hypot(v.x, v.y); }

Vec2 normalized(Vec2 v, Vec2 fallback) {
    const float l = length(v);
    return l > 1e-5f ? Vec2{v.x / l, v.y / l} : fallback;
}

float smooth(float t) {
    t = std::clamp(t, 0.f, 1.f);
    return t * t * (3.f - 2.f * t);
}

bool routed(UnitState state) { return state == UnitState::Broken || state == UnitState::Routing; }

// Noise that changes smoothly every `period` battlefield seconds.
float drift(std::uint32_t seed, int salt, float seconds, float period) {
    const float t = seconds / period;
    const float base = std::floor(t);
    const int step = static_cast<int>(base);
    const float a = soldierNoise(seed, step, salt);
    const float b = soldierNoise(seed, step + 1, salt);
    return a + (b - a) * smooth(t - base);
}

// Critically damped spring toward a fixed target over dt, solved exactly so any stiffness is stable.
void spring(Vec2 &pos, Vec2 &vel, Vec2 target, float stiffness, float dt) {
    const float w = std::max(stiffness, 1e-3f);
    const float e = std::exp(-w * dt);
    const Vec2 x = pos - target;
    const Vec2 j = vel + x * w;
    pos = target + (x + j * dt) * e;
    vel = (vel - j * (w * dt)) * e;
}

void mixBits(std::uint64_t &hash, float value) {
    const std::uint32_t bits = std::bit_cast<std::uint32_t>(value);
    for (int byte = 0; byte < 4; ++byte) {
        hash ^= (bits >> (byte * 8)) & 0xffu;
        hash *= 1099511628211ull;
    }
}

// A tiny spatial hash for at most a few hundred points: cells of `cell` metres folded into kHashBuckets buckets.
class Grid {
  public:
    void build(const std::vector<Vec2> &points, float cell) {
        cell_ = std::max(cell, 1e-3f);
        head_.fill(-1);
        next_.assign(points.size(), -1);
        for (std::size_t i = 0; i < points.size(); ++i) {
            const int bucket = bucketOf(cellX(points[i].x), cellY(points[i].y));
            next_[i] = head_[static_cast<std::size_t>(bucket)];
            head_[static_cast<std::size_t>(bucket)] = static_cast<int>(i);
        }
    }

    // Calls visit(j) once for every point j < i in the 3 x 3 cells around point i.
    template <class Visit> void neighbours(const std::vector<Vec2> &points, std::size_t i, Visit visit) const {
        const int cx = cellX(points[i].x);
        const int cy = cellY(points[i].y);
        std::array<int, 9> seen{};
        int count = 0;
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                const int bucket = bucketOf(cx + dx, cy + dy);
                if (std::find(seen.begin(), seen.begin() + count, bucket) != seen.begin() + count) {
                    continue;
                }
                seen[static_cast<std::size_t>(count++)] = bucket;
                for (int j = head_[static_cast<std::size_t>(bucket)]; j >= 0; j = next_[static_cast<std::size_t>(j)]) {
                    if (static_cast<std::size_t>(j) < i) {
                        visit(static_cast<std::size_t>(j));
                    }
                }
            }
        }
    }

  private:
    int cellX(float x) const { return static_cast<int>(std::floor(x / cell_)); }
    int cellY(float y) const { return static_cast<int>(std::floor(y / cell_)); }
    static int bucketOf(int x, int y) {
        const auto key = static_cast<std::uint32_t>(x) * 73856093u ^ static_cast<std::uint32_t>(y) * 19349663u;
        return static_cast<int>(key & static_cast<std::uint32_t>(kHashBuckets - 1));
    }

    float cell_ = 1.f;
    std::array<int, kHashBuckets> head_{};
    std::vector<int> next_;
};

// Pushes points closer than spacing apart; each pair moves by a quarter of the overlap. Order is fixed, so the
// result is deterministic.
void relax(std::vector<Vec2> &points, const std::vector<int> &group, float spacing, float enemySpacing) {
    Grid grid;
    grid.build(points, std::max(spacing, enemySpacing));
    for (std::size_t i = 0; i < points.size(); ++i) {
        grid.neighbours(points, i, [&](std::size_t j) {
            const float want = group[i] == group[j] ? spacing : enemySpacing;
            Vec2 d = points[i] - points[j];
            float l = length(d);
            if (l >= want) {
                return;
            }
            if (l < 1e-4f) {
                const float a = static_cast<float>(i * 7 + j * 13) * 0.61803f * 2.f * kPi;
                d = {std::cos(a), std::sin(a)};
                l = 1.f;
            } else {
                d = d * (1.f / l);
            }
            const float push = (want - std::min(l, want)) * 0.25f;
            points[i] = points[i] + d * push;
            points[j] = points[j] - d * push;
        });
    }
}

} // namespace

float unitLooseness(const Battle &battle, const Unit &unit, const FormationTuning &tuning) {
    float loose = 0.f;
    switch (unit.state) {
    case UnitState::Steady:
        break;
    case UnitState::Wavering:
        loose = 0.45f;
        break;
    case UnitState::Broken:
        loose = 0.6f;
        break;
    case UnitState::Routing:
        loose = 1.f;
        break;
    case UnitState::Gone:
        return 0.f;
    }
    if (!unit.contacts.empty()) {
        loose = std::max(loose, 0.3f);
    }
    if (unit.charging) {
        loose = std::max(loose, 0.2f);
    }
    if (battle.terrainAt(unit.pos) == Terrain::Paddy) {
        loose += tuning.paddyLoose;
    }
    return std::clamp(loose, 0.f, 1.f);
}

void FormationLayer::reset() {
    units_.clear();
    bodies_.clear();
}

SoldierKind FormationLayer::kindOf(const UnitType &type, const Trooper &trooper, int files) {
    if (type.general && trooper.slot == files / 2) {
        return SoldierKind::General;
    }
    return soldierKind(type);
}

FormationLayer::Trooper FormationLayer::fresh(Body &body, std::uint32_t unitId, Vec2 pos, float facing) {
    Trooper trooper;
    trooper.pos = pos;
    trooper.prev = pos;
    trooper.facing = facing;
    trooper.prevFacing = facing;
    trooper.seed = unitId * 4096u + body.nextSeed++;
    return trooper;
}

void FormationLayer::fall(Side side, SoldierKind kind, const Trooper &trooper) {
    FallenSoldier body;
    body.pos = trooper.pos;
    body.facing = trooper.facing;
    body.side = side;
    body.kind = kind;
    bodies_.push_back(body);
    const auto cap = static_cast<std::size_t>(std::max(0.f, tuning_.bodyCap));
    if (bodies_.size() > cap) {
        bodies_.erase(bodies_.begin(), bodies_.begin() + static_cast<std::ptrdiff_t>(bodies_.size() - cap));
    }
}

float FormationLayer::bodyAlpha(const FallenSoldier &body) const {
    const float fade = tuning_.bodyFadeSec > 0.f ? std::clamp(body.age / tuning_.bodyFadeSec, 0.f, 1.f) : 1.f;
    return 1.f - (1.f - tuning_.bodyMinAlpha) * fade;
}

void FormationLayer::apply(const Battle &battle, const Event &event, std::size_t knownUnits) {
    const std::size_t count = battle.units().size();
    const auto valid = [&](UnitId id) { return id != kNoUnit && id < count; };
    switch (event.type) {
    case Event::Type::Impact: {
        if (!valid(event.a) || !valid(event.b)) {
            break;
        }
        const Unit &attacker = battle.unit(event.a);
        const Unit &defender = battle.unit(event.b);
        Body &hit = units_[event.b];
        hit.threat = attacker.pos - defender.pos;
        hit.hasThreat = true;
        const float sideMul = event.side == ContactSide::Rear    ? tuning_.impactRear
                              : event.side == ContactSide::Flank ? tuning_.impactFlank
                                                                 : 1.f;
        const bool pike = (event.flags & kPikeFront) != 0;
        const float strength = std::clamp(event.value / 1.9f, 0.4f, 1.f) * (pike ? 0.3f : 1.f);
        const float depth =
            std::min(defender.depth * tuning_.impactDepthFraction * strength * sideMul, defender.depth * 0.6f);
        // The defender is dented where the attacker struck.
        const Vec2 toward = normalized(toLocal(attacker.pos, defender.pos, defender.facing), {1.f, 0.f});
        const float extent = std::abs(toward.x) * defender.depth * 0.5f + std::abs(toward.y) * defender.frontage * 0.5f;
        hit.dents.push_back({toward * extent, -toward, depth, 0.f, false});
        // The attacker's front rank drives in.
        Body &striker = units_[event.a];
        const Vec2 at = toLocal(defender.pos, attacker.pos, attacker.facing);
        const float across = std::clamp(at.y, -attacker.frontage * 0.5f, attacker.frontage * 0.5f);
        striker.dents.push_back({{attacker.depth * 0.5f, across}, {1.f, 0.f}, depth, 0.f, true});
        break;
    }
    case Event::Type::TeppoVolley:
    case Event::Type::ThreeEyedVolley:
        if (valid(event.a) && valid(event.b)) {
            units_[event.b].threat = battle.unit(event.a).pos - battle.unit(event.b).pos;
            units_[event.b].hasThreat = true;
        }
        break;
    case Event::Type::Shattered:
        if (valid(event.a)) {
            shatter(battle, event.a, knownUnits);
        }
        break;
    case Event::Type::Split:
        if (valid(event.a) && valid(event.b)) {
            split(battle, event.a, event.b);
        }
        break;
    case Event::Type::Merged:
        if (valid(event.a) && valid(event.b) && event.a != event.b) {
            Body &into = units_[event.a];
            Body &from = units_[event.b];
            for (Trooper &trooper : from.troopers) {
                into.troopers.push_back(trooper);
            }
            from.troopers.clear();
            into.rallyAge = 0.f;
        }
        break;
    case Event::Type::Rallied:
        if (valid(event.a)) {
            units_[event.a].rallyAge = 0.f;
        }
        break;
    default:
        break;
    }
}

void FormationLayer::shatter(const Battle &battle, UnitId source, std::size_t knownUnits) {
    const Unit &unit = battle.unit(source);
    const UnitId origin = unit.origin != kNoUnit ? unit.origin : unit.id;
    std::vector<UnitId> parts{source};
    for (std::size_t id = knownUnits; id < battle.units().size(); ++id) {
        const Unit &part = battle.unit(static_cast<UnitId>(id));
        if (part.origin == origin && part.id != source) {
            parts.push_back(part.id);
        }
    }
    if (parts.size() < 2) {
        return;
    }
    // Parts fan out across the scatter direction; deal the soldiers to them by their side of the fan.
    const float base = unit.facing;
    const auto offsetOf = [&](UnitId id) {
        const Unit &part = battle.unit(id);
        return part.scatterAngle ? angDiff(*part.scatterAngle, base) : 0.f;
    };
    std::stable_sort(parts.begin(), parts.end(), [&](UnitId a, UnitId b) { return offsetOf(a) < offsetOf(b); });
    std::vector<Trooper> pool = std::move(units_[source].troopers);
    units_[source].troopers.clear();
    const Vec2 lateral{-std::sin(base), std::cos(base)};
    std::stable_sort(pool.begin(), pool.end(), [&](const Trooper &a, const Trooper &b) {
        return dot(a.pos - unit.pos, lateral) < dot(b.pos - unit.pos, lateral);
    });
    const UnitType &type = battle.typeOf(unit);
    std::size_t next = 0;
    for (std::size_t index = 0; index < parts.size(); ++index) {
        Body &body = units_[parts[index]];
        body.started = true;
        const bool last = index + 1 == parts.size();
        const std::size_t want =
            last ? pool.size() - next : static_cast<std::size_t>(soldierCount(type, battle.unit(parts[index]).men));
        for (std::size_t k = 0; k < want && next < pool.size(); ++k) {
            body.troopers.push_back(pool[next++]);
        }
    }
}

void FormationLayer::split(const Battle &battle, UnitId left, UnitId right) {
    const Unit &unit = battle.unit(left);
    Body &from = units_[left];
    Body &to = units_[right];
    to.started = true;
    std::vector<Trooper> keep;
    for (Trooper &trooper : from.troopers) {
        if (toLocal(trooper.pos, unit.pos, unit.facing).y > 0.f) {
            to.troopers.push_back(trooper);
        } else {
            keep.push_back(trooper);
        }
    }
    from.troopers = std::move(keep);
}

void FormationLayer::settleCount(const Battle &battle, const Unit &unit, Body &body) {
    const UnitType &type = battle.typeOf(unit);
    const SoldierGrid grid = soldierGrid(type, unit);
    if (unit.state == UnitState::Gone) {
        // A fallen general leaves a body; everyone else has left the field or joined another unit.
        if (unit.gone == GoneReason::GeneralKilled) {
            for (const Trooper &trooper : body.troopers) {
                if (kindOf(type, trooper, grid.files) == SoldierKind::General) {
                    fall(unit.side, SoldierKind::General, trooper);
                }
            }
        }
        body.troopers.clear();
        return;
    }
    const auto needed = static_cast<std::size_t>(soldierCount(type, unit.men));
    if (!body.started) {
        body.started = true;
        for (std::size_t i = 0; i < needed; ++i) {
            Trooper trooper = fresh(body, unit.id, unit.pos, unit.facing);
            trooper.slot = static_cast<int>(i);
            const Aim aim = target(battle, unit, body, trooper, 1.f);
            trooper.pos = trooper.prev = aim.pos;
            trooper.facing = trooper.prevFacing = aim.facing;
            body.troopers.push_back(trooper);
        }
        return;
    }
    if (body.troopers.size() > needed) {
        const std::size_t excess = body.troopers.size() - needed;
        // Routing men who slip away are not casualties; anyone else falls on the side facing the threat.
        const bool desertion = unit.state == UnitState::Routing && unit.contacts.empty();
        Vec2 threat = body.hasThreat ? body.threat : Vec2{std::cos(unit.facing), std::sin(unit.facing)};
        if (!unit.contacts.empty() && unit.contacts.front() < battle.units().size()) {
            threat = battle.unit(unit.contacts.front()).pos - unit.pos;
        }
        threat = normalized(threat, {std::cos(unit.facing), std::sin(unit.facing)});
        std::vector<std::size_t> order(body.troopers.size());
        for (std::size_t i = 0; i < order.size(); ++i) {
            order[i] = i;
        }
        const auto score = [&](std::size_t i) {
            const Trooper &trooper = body.troopers[i];
            return desertion ? soldierNoise(trooper.seed, 0, 31) : dot(trooper.pos - unit.pos, threat);
        };
        std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) { return score(a) > score(b); });
        std::vector<bool> drop(body.troopers.size(), false);
        for (std::size_t k = 0; k < excess; ++k) {
            drop[order[k]] = true;
        }
        std::vector<Trooper> kept;
        kept.reserve(needed);
        for (std::size_t i = 0; i < body.troopers.size(); ++i) {
            if (!drop[i]) {
                kept.push_back(body.troopers[i]);
            } else if (!desertion) {
                fall(unit.side, kindOf(type, body.troopers[i], grid.files), body.troopers[i]);
            }
        }
        body.troopers = std::move(kept);
    }
    while (body.troopers.size() < needed) {
        // New men (a merge that brought fewer soldiers than troops, or a spawn) run in from the unit centre.
        body.troopers.push_back(fresh(body, unit.id, unit.pos, unit.facing));
    }
    // Close the ranks: soldiers keep their order, so the rear steps forward into the gaps.
    std::stable_sort(body.troopers.begin(), body.troopers.end(),
                     [](const Trooper &a, const Trooper &b) { return a.slot < b.slot; });
    for (std::size_t i = 0; i < body.troopers.size(); ++i) {
        body.troopers[i].slot = static_cast<int>(i);
    }
}

FormationLayer::Aim FormationLayer::target(const Battle &battle, const Unit &unit, const Body &body,
                                           const Trooper &trooper, float fraction) const {
    const UnitType &type = battle.typeOf(unit);
    const Pose pose = interpolatedPose(unit, fraction);
    const SoldierGrid grid = soldierGrid(type, unit);
    const bool broken = routed(unit.state);
    // Broken and routing units keep the grid turned toward the enemy, so the ranks do not swap ends as they turn.
    const float frame = broken ? pose.facing + kPi : pose.facing;
    const float halfDepth = unit.depth * 0.5f;
    const float halfFront = unit.frontage * 0.5f;
    const float seconds = battle.battleSeconds();
    const std::uint32_t seed = trooper.seed;
    const float loose = unitLooseness(battle, unit, tuning_);

    Aim aim;
    aim.facing = frame;
    aim.stiffness = tuning_.stiffness;
    Vec2 local = soldierSlot(grid, trooper.slot) * (1.f + loose * tuning_.looseSpread);
    const float wobble = 0.6f + 0.4f * std::sin(seconds * 0.7f + soldierNoise(seed, 0, 3) * 2.f * kPi);
    local.x += (drift(seed, 1, seconds, 3.f) - 0.5f) * 2.f * loose * tuning_.looseJitter * wobble;
    local.y += (drift(seed, 2, seconds, 3.f) - 0.5f) * 2.f * loose * tuning_.looseJitter * wobble;

    if (unit.charging) {
        const float progress = std::clamp(unit.runDist / kChargeFullRun, 0.f, 1.f);
        const float open = std::min(tuning_.chargeOpenMax, unit.runDist * tuning_.chargeOpenPerMeter);
        local = local * (1.f + open);
        const float across = halfFront > 0.f ? std::clamp(local.y / halfFront, -1.f, 1.f) : 0.f;
        local.x += tuning_.chargeBulge * (1.f - across * across) * progress;
        local.x += tuning_.chargeLead * (soldierNoise(seed, 0, 4) - 0.5f) * 2.f * progress;
        aim.stiffness = tuning_.chargeStiffness;
    }

    for (const Dent &dent : body.dents) {
        const float decay = std::exp(-dent.age / std::max(tuning_.impactRelaxSec, 0.1f));
        if (dent.thrust) {
            if (local.x > halfDepth - grid.rankSpacing * 1.5f) {
                const float w = std::max(0.f, 1.f - std::abs(local.y - dent.local.y) / tuning_.impactRadius);
                local.x += dent.depth * w * decay;
            }
        } else {
            const float w = std::max(0.f, 1.f - length(local - dent.local) / tuning_.impactRadius);
            local = local + dent.dir * (dent.depth * w * decay);
        }
    }

    if (!unit.contacts.empty() && !broken && unit.contacts.front() < battle.units().size()) {
        const Unit &enemy = battle.unit(unit.contacts.front());
        const Vec2 there = toLocal(enemy.pos, pose.pos, frame);
        const Vec2 dir = normalized(there, {1.f, 0.f});
        const Vec2 perp{-dir.y, dir.x};
        const float extent = std::abs(dir.x) * halfDepth + std::abs(dir.y) * halfFront;
        const float along = dot(local, dir);
        if (along > extent - grid.rankSpacing * 1.6f) {
            aim.fighting = true;
            local = local + dir * (tuning_.meleeBand * (drift(seed, 5, seconds, 2.f) - 0.35f));
            const float lateral = dot(local, perp);
            const float centre = dot(there, perp);
            const float reach = std::max(enemy.frontage, enemy.depth) * 0.5f;
            const float excess = std::abs(lateral - centre) - reach;
            if (excess > 0.f) {
                const float inward = lateral < centre ? 1.f : -1.f;
                local = local + dir * (excess * tuning_.meleeCurl * 0.6f) + perp * (inward * excess * tuning_.meleeCurl * 0.3f);
            }
            aim.facing = frame + std::atan2(dir.y, dir.x);
        }
    }

    const bool straggler = soldierNoise(seed, 0, 6) < tuning_.stragglerShare;
    if (unit.breaking && straggler) {
        aim.stiffness *= tuning_.stragglerLag;
    }

    if (unit.state == UnitState::Broken) {
        // The rear ranks turn and leave first; the front still faces the enemy.
        const float progress = std::clamp(1.f - unit.brokenSec / kBrokenSec, 0.f, 1.f);
        const int rank = trooper.slot / std::max(1, grid.files);
        const float rear = grid.ranks > 1 ? static_cast<float>(rank) / static_cast<float>(grid.ranks - 1) : 1.f;
        const float start = (1.f - rear) * 0.7f;
        const float leave = std::clamp((progress - start) / 0.3f, 0.f, 1.f);
        local.x -= leave * unit.depth * 0.9f;
        aim.facing = leave > 0.5f ? pose.facing : frame;
    } else if (unit.state == UnitState::Routing) {
        // A scattered stream: the body spreads, stragglers trail toward the pursuers.
        const float spread = std::clamp(unit.routSec / 24.f, 0.f, 1.f);
        local = local * (1.f + tuning_.routSpread * spread);
        const float lag = soldierNoise(seed, 0, 7);
        local.x += tuning_.routStream * unit.depth * lag * lag * (0.4f + spread);
        aim.facing = pose.facing + (soldierNoise(seed, 0, 8) - 0.5f) * 1.f;
        aim.stiffness = tuning_.routStiffness * (straggler ? tuning_.stragglerLag : 1.f);
    }

    if (body.rallyAge >= 0.f && !broken) {
        // Gather on the banner, then re-form the ranks.
        const float angle = soldierNoise(seed, 0, 9) * 2.f * kPi;
        const float radius = std::sqrt(soldierNoise(seed, 0, 10)) * tuning_.gatherRadius;
        const Vec2 gather{-unit.depth * 0.3f + std::cos(angle) * radius, std::sin(angle) * radius};
        if (body.rallyAge < tuning_.gatherSec) {
            local = gather;
        } else {
            const float t = smooth((body.rallyAge - tuning_.gatherSec) / std::max(tuning_.reformSec, 0.1f));
            local = gather + (local - gather) * t;
        }
        aim.stiffness = tuning_.rallyStiffness;
    }

    // Outside a melee, a shattering, or a rout, soldiers stay close to the rules box.
    if (unit.contacts.empty() && !broken && body.rallyAge < 0.f) {
        const float margin = unit.depth * tuning_.clampOutside;
        local.x = std::clamp(local.x, -halfDepth - margin, halfDepth + margin);
        local.y = std::clamp(local.y, -halfFront - margin, halfFront + margin);
    }
    aim.pos = rotated(pose.pos, frame, local);
    return aim;
}

void FormationLayer::substep(const Battle &battle, float fraction, float dt) {
    constexpr float kEaseMetersPerSecond = 6.f;
    for (const Unit &unit : battle.units()) {
        Body &body = units_[unit.id];
        if (unit.state == UnitState::Gone || body.troopers.empty()) {
            continue;
        }
        const bool cavalry = battle.typeOf(unit).cls == UnitClass::Cavalry;
        const float maxStep = (cavalry ? tuning_.maxSpeedCavalry : tuning_.maxSpeedInfantry) * dt;
        for (Trooper &trooper : body.troopers) {
            const Aim aim = target(battle, unit, body, trooper, fraction);
            const Vec2 before = trooper.pos;
            spring(trooper.pos, trooper.vel, aim.pos, aim.stiffness, dt);
            const Vec2 moved = trooper.pos - before;
            const float distance = length(moved);
            if (distance > maxStep) {
                trooper.pos = before + moved * (maxStep / distance);
                trooper.vel = (trooper.pos - before) * (1.f / dt);
            }
            trooper.stride += length(trooper.pos - before);
            trooper.facing += std::clamp(angDiff(aim.facing, trooper.facing), -kTurnRate * dt, kTurnRate * dt);
            trooper.fighting = aim.fighting;
        }
        const float spacing = battle.typeOf(unit).cls == UnitClass::Cavalry ? tuning_.spacingCavalry
                                                                             : tuning_.spacingInfantry;
        separate(body, spacing);
    }
    // Between two units in contact the lines may interleave, so enemies keep only part of the spacing.
    for (const Unit &unit : battle.units()) {
        if (unit.state == UnitState::Gone) {
            continue;
        }
        for (UnitId other : unit.contacts) {
            if (other <= unit.id || other >= units_.size()) {
                continue;
            }
            separatePair(units_[unit.id], units_[other], tuning_.spacingInfantry * 0.6f);
        }
    }
    for (const Unit &unit : battle.units()) {
        if (unit.state == UnitState::Gone) {
            continue;
        }
        Body &body = units_[unit.id];
        // Outside a melee, a shattering, or a rout, soldiers never stray more than the margin from the rules box.
        // Right after such a state they are eased back in; once the unit is settled the bound is hard.
        if (unit.contacts.empty() && !routed(unit.state)) {
            const Pose pose = interpolatedPose(unit, fraction);
            const float margin = unit.depth * tuning_.clampOutside;
            const float step = body.calmSec >= kSettleSec ? 1e9f : kEaseMetersPerSecond * dt;
            for (Trooper &trooper : body.troopers) {
                const Vec2 local = toLocal(trooper.pos, pose.pos, pose.facing);
                Vec2 bound = local;
                bound.x = std::clamp(bound.x, -unit.depth * 0.5f - margin, unit.depth * 0.5f + margin);
                bound.y = std::clamp(bound.y, -unit.frontage * 0.5f - margin, unit.frontage * 0.5f + margin);
                const Vec2 fix = bound - local;
                const float size = length(fix);
                const Vec2 moved = size > step ? local + fix * (step / size) : bound;
                trooper.pos = rotated(pose.pos, pose.facing, moved);
            }
        }
    }
}

void FormationLayer::separate(Body &body, float spacing) {
    std::vector<Vec2> points;
    points.reserve(body.troopers.size());
    for (const Trooper &trooper : body.troopers) {
        points.push_back(trooper.pos);
    }
    const std::vector<int> group(points.size(), 0);
    relax(points, group, spacing, spacing);
    for (std::size_t i = 0; i < points.size(); ++i) {
        body.troopers[i].pos = points[i];
    }
}

void FormationLayer::separatePair(Body &left, Body &right, float spacing) {
    if (left.troopers.empty() || right.troopers.empty()) {
        return;
    }
    std::vector<Vec2> points;
    std::vector<int> group;
    for (const Trooper &trooper : left.troopers) {
        points.push_back(trooper.pos);
        group.push_back(0);
    }
    for (const Trooper &trooper : right.troopers) {
        points.push_back(trooper.pos);
        group.push_back(1);
    }
    // Friends are already spaced; only enemies are pushed here.
    relax(points, group, 0.f, spacing);
    for (std::size_t i = 0; i < left.troopers.size(); ++i) {
        left.troopers[i].pos = points[i];
    }
    for (std::size_t i = 0; i < right.troopers.size(); ++i) {
        right.troopers[i].pos = points[left.troopers.size() + i];
    }
}

void FormationLayer::tick(const Battle &battle, std::span<const Event> events) {
    const std::size_t known = units_.size();
    if (units_.size() < battle.units().size()) {
        units_.resize(battle.units().size());
    }
    for (Body &body : units_) {
        for (Trooper &trooper : body.troopers) {
            trooper.prev = trooper.pos;
            trooper.prevFacing = trooper.facing;
        }
    }
    for (const Event &event : events) {
        apply(battle, event, known);
    }
    for (const Unit &unit : battle.units()) {
        Body &body = units_[unit.id];
        settleCount(battle, unit, body);
        const bool calm = unit.contacts.empty() && !routed(unit.state) && body.rallyAge < 0.f;
        body.calmSec = calm ? body.calmSec + kTickSeconds : 0.f;
    }
    const float dt = kTickSeconds / static_cast<float>(kFormationSubsteps);
    for (int step = 0; step < kFormationSubsteps; ++step) {
        substep(battle, static_cast<float>(step + 1) / static_cast<float>(kFormationSubsteps), dt);
    }
    for (Body &body : units_) {
        for (Dent &dent : body.dents) {
            dent.age += kTickSeconds;
        }
        const float keep = std::max(tuning_.impactRelaxSec, 0.1f) * 4.f;
        std::erase_if(body.dents, [keep](const Dent &dent) { return dent.age > keep; });
        if (body.rallyAge >= 0.f) {
            body.calmSec = 0.f;
            body.rallyAge += kTickSeconds;
            if (body.rallyAge > tuning_.gatherSec + tuning_.reformSec) {
                body.rallyAge = -1.f;
            }
        }
    }
    for (FallenSoldier &body : bodies_) {
        body.age += kTickSeconds;
    }
}

void FormationLayer::layout(const Battle &battle, float alpha, float animTime, std::vector<Soldier> &out) const {
    alpha = std::clamp(alpha, 0.f, 1.f);
    const float seconds = battle.battleSeconds();
    for (const Unit &unit : battle.units()) {
        if (unit.id >= units_.size() || unit.state == UnitState::Gone) {
            continue;
        }
        const Body &body = units_[unit.id];
        const UnitType &type = battle.typeOf(unit);
        const SoldierGrid grid = soldierGrid(type, unit);
        const bool cavalry = type.cls == UnitClass::Cavalry;
        const float strideLength = cavalry ? kStrideCavalry : kStrideInfantry;
        for (const Trooper &trooper : body.troopers) {
            Soldier soldier;
            soldier.unit = unit.id;
            soldier.pos = trooper.prev + (trooper.pos - trooper.prev) * alpha;
            soldier.facing = trooper.prevFacing + angDiff(trooper.facing, trooper.prevFacing) * alpha;
            soldier.side = unit.side;
            soldier.kind = kindOf(type, trooper, grid.files);
            soldier.direction = soldierDirection(soldier.facing);
            const float speed = length(trooper.vel);
            if (trooper.fighting) {
                const int strike =
                    static_cast<int>(std::floor(seconds * kStrikeRate + soldierNoise(trooper.seed, 0, 11) * 2.f));
                soldier.frame = kFrameStrike + ((strike % 2) + 2) % 2;
            } else if (speed > 0.5f) {
                const int step = static_cast<int>(std::floor(trooper.stride / strideLength));
                soldier.frame = kFrameMove + ((step % kMoveFrames) + kMoveFrames) % kMoveFrames;
            } else {
                soldier.frame = soldierNoise(trooper.seed, 0, 12) < 0.15f ? kFrameAside : kFrameIdle;
            }
            soldier.shade = unit.state == UnitState::Routing ? 0.8f : 1.f;
            if (unit.charging && cavalry && soldierNoise(trooper.seed, 0, 13) < 0.5f) {
                const float phase = soldierNoise(trooper.seed, 0, 14);
                const float puff = animTime * 1.5f + phase - std::floor(animTime * 1.5f + phase);
                soldier.dust = (1.f - puff) * 0.55f;
                const Vec2 back{-std::cos(soldier.facing), -std::sin(soldier.facing)};
                soldier.dustPos = soldier.pos + back * (2.f + puff * 5.f);
            }
            out.push_back(soldier);
        }
    }
}

UnitId FormationLayer::pick(const Battle &battle, Vec2 world, float radius, float alpha) const {
    UnitId best = kNoUnit;
    float bestDistance = radius;
    for (const Unit &unit : battle.units()) {
        if (unit.id >= units_.size() || unit.state == UnitState::Gone) {
            continue;
        }
        for (const Trooper &trooper : units_[unit.id].troopers) {
            const Vec2 at = trooper.prev + (trooper.pos - trooper.prev) * alpha;
            const float distance = length(at - world);
            if (distance < bestDistance) {
                bestDistance = distance;
                best = unit.id;
            }
        }
    }
    return best;
}

std::uint64_t FormationLayer::hash() const {
    std::uint64_t hash = 1469598103934665603ull;
    for (const Body &body : units_) {
        mixBits(hash, static_cast<float>(body.troopers.size()));
        for (const Trooper &trooper : body.troopers) {
            mixBits(hash, trooper.pos.x);
            mixBits(hash, trooper.pos.y);
            mixBits(hash, trooper.vel.x);
            mixBits(hash, trooper.vel.y);
            mixBits(hash, trooper.facing);
        }
    }
    for (const FallenSoldier &body : bodies_) {
        mixBits(hash, body.pos.x);
        mixBits(hash, body.pos.y);
        mixBits(hash, body.age);
    }
    return hash;
}

std::size_t FormationLayer::livingSoldiers() const {
    std::size_t count = 0;
    for (const Body &body : units_) {
        count += body.troopers.size();
    }
    return count;
}

} // namespace tq
