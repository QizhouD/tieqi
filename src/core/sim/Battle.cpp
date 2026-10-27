#include "core/sim/Battle.h"

#include "core/math/Geometry.h"
#include "core/rules/Combat.h"
#include "core/rules/Terrain.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <numbers>
#include <stdexcept>
#include <utility>

namespace tq {
namespace {

constexpr std::uint64_t kFnvOffset = 14695981039346656037ULL;
constexpr std::uint64_t kFnvPrime = 1099511628211ULL;

float length(Vec2 value) { return std::sqrt(dot(value, value)); }

float directionOf(Vec2 from, Vec2 to) { return std::atan2(to.y - from.y, to.x - from.x); }

bool alive(const Unit &unit) { return unit.state != UnitState::Gone; }

bool fighting(const Unit &unit) { return unit.state == UnitState::Steady || unit.state == UnitState::Wavering; }

void mix(std::uint64_t &hash, const void *data, std::size_t size) {
    const auto *bytes = static_cast<const unsigned char *>(data);
    for (std::size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= kFnvPrime;
    }
}

template <class T> void mixPod(std::uint64_t &hash, const T &value) { mix(hash, &value, sizeof(T)); }

void mixBool(std::uint64_t &hash, bool value) {
    const std::uint8_t byte = value ? 1 : 0;
    mixPod(hash, byte);
}

void mixOptional(std::uint64_t &hash, const std::optional<float> &value) {
    mixBool(hash, value.has_value());
    mixPod(hash, value.value_or(0.f));
}

float alignOf(bool cavalry, float diff) {
    if (cavalry) {
        if (diff < kCavalryAlignNear) {
            return 1.f;
        }
        if (diff < kCavalryAlignFar) {
            return kCavalryAlignMid;
        }
        return kCavalryAlignSlow;
    }
    if (diff < kInfantryAlignNear) {
        return 1.f;
    }
    if (diff < kInfantryAlignFar) {
        return kInfantryAlignMid;
    }
    return kInfantryAlignSlow;
}

Obb unitBox(const Unit &unit, float inflate) { return obb(unit.pos, unit.facing, unit.depth, unit.frontage, inflate); }

bool isMoving(const Unit &unit) {
    return fighting(unit) && (unit.order.type == Order::Type::Move || unit.order.type == Order::Type::Attack);
}

float capOf(const Unit &unit, const UnitType &type) {
    return type.morale * (kCapFloor + kCapMen * std::min(1.f, unit.men / type.men));
}

} // namespace

Battle::Battle(const GameDefs &defs, const ScenarioDef &scenario, std::uint64_t seed, BattleOptions opts)
    : defs_(defs), scenario_(scenario), options_(opts), rng_(seed) {
    for (const Placement &place : scenario_.deployment) {
        const UnitType &type = defs_.types.at(place.type);
        Unit unit;
        unit.id = static_cast<UnitId>(units_.size());
        unit.type = place.type;
        unit.side = type.side;
        unit.labelKey = place.labelKey;
        unit.pos = place.pos;
        unit.prevPos = place.pos;
        unit.facing = place.facing;
        unit.prevFacing = place.facing;
        unit.frontage = type.frontage;
        unit.depth = type.depth;
        unit.men = type.men;
        unit.maxMen = type.men;
        unit.morale = type.morale;
        unit.order.type = Order::Type::Hold;
        unit.order.face = place.facing;
        unit.stillSec = kInitialStillSec;
        unit.reloadSec = type.range > 0.f ? kReloadRollSec * rng_.uniform() : 0.f;
        unit.vanguard = place.vanguard;
        if (type.side == Side::Ming) {
            totalMing_ += type.men;
        } else {
            totalJp_ += type.men;
        }
        units_.push_back(std::move(unit));
    }
}

void Battle::submit(Command cmd) {
    cmd.tick = tick_ + 1;
    commands_.push_back(std::move(cmd));
}

void Battle::step() {
    const std::uint32_t executing = tick_ + 1;
    for (const Command &cmd : commands_) {
        if (cmd.tick == executing) {
            apply(cmd);
        }
    }
    std::erase_if(commands_, [&](const Command &cmd) { return cmd.tick == executing; });
    tick_ = executing;

    if (options_.aiEnabled && tick_ % 10 == 0) {
        // C7 补 AI。
    }
    for (Unit &unit : units_) {
        if (alive(unit)) {
            moveUnit(unit);
        }
    }
    contactsStep();
    if (tick_ % 5 == 0) {
        combatTick();
    }
}

std::vector<Event> Battle::drainEvents() {
    std::vector<Event> taken;
    taken.swap(events_);
    return taken;
}

const Unit &Battle::unit(UnitId id) const {
    if (id >= units_.size()) {
        throw std::out_of_range("unit");
    }
    return units_[id];
}

const UnitType &Battle::typeOf(const Unit &unit) const { return defs_.types.at(unit.type); }

Terrain Battle::terrainAt(Vec2 point) const { return tq::terrainAt(scenario_.map, point); }

std::uint64_t Battle::stateHash() const {
    std::uint64_t hash = kFnvOffset;
    for (const Unit &unit : units_) {
        mixPod(hash, unit.id);
        mixPod(hash, unit.type);
        mixPod(hash, unit.side);
        const std::uint32_t labelBytes = static_cast<std::uint32_t>(unit.labelKey.size());
        mixPod(hash, labelBytes);
        mix(hash, unit.labelKey.data(), unit.labelKey.size());
        mixPod(hash, unit.fragNo);
        mixPod(hash, unit.pos);
        mixPod(hash, unit.prevPos);
        mixPod(hash, unit.facing);
        mixPod(hash, unit.prevFacing);
        mixPod(hash, unit.frontage);
        mixPod(hash, unit.depth);
        mixPod(hash, unit.men);
        mixPod(hash, unit.maxMen);
        mixPod(hash, unit.morale);
        mixPod(hash, unit.state);
        mixPod(hash, unit.gone);
        mixPod(hash, unit.order.type);
        mixPod(hash, unit.order.point);
        mixOptional(hash, unit.order.face);
        mixPod(hash, unit.order.target);
        const std::uint32_t contactCount = static_cast<std::uint32_t>(unit.contacts.size());
        mixPod(hash, contactCount);
        for (UnitId id : unit.contacts) {
            mixPod(hash, id);
        }
        mixPod(hash, unit.engagedSec);
        mixPod(hash, unit.recoverSec);
        mixPod(hash, unit.reloadSec);
        mixPod(hash, unit.stillSec);
        mixPod(hash, unit.speed);
        mixPod(hash, unit.runDist);
        mixBool(hash, unit.charging);
        mixBool(hash, unit.volleyDone);
        mixBool(hash, unit.breaking);
        mixPod(hash, unit.momentum);
        mixPod(hash, unit.momentumSec);
        mixPod(hash, unit.brokenSec);
        mixPod(hash, unit.routSec);
        mixPod(hash, unit.knock);
        mixOptional(hash, unit.scatterAngle);
        mixBool(hash, unit.everBroken);
        mixBool(hash, unit.isFragment);
        mixBool(hash, unit.rejoin);
        mixBool(hash, unit.vanguard);
        mixBool(hash, unit.deploying);
        mixPod(hash, unit.origin);
    }
    return hash;
}

void Battle::apply(const Command &cmd) {
    std::vector<Unit *> selected;
    for (UnitId id : cmd.units) {
        if (id >= units_.size()) {
            continue;
        }
        Unit &unit = units_[id];
        if (unit.side == Side::Ming && fighting(unit)) {
            selected.push_back(&unit);
        }
    }
    if (selected.empty()) {
        return;
    }
    if (cmd.type == Command::Type::Hold) {
        for (Unit *unit : selected) {
            unit->order = {};
            unit->order.type = Order::Type::Hold;
            unit->order.face = unit->facing;
        }
        return;
    }
    if (cmd.type == Command::Type::Attack) {
        if (cmd.target >= units_.size() || !alive(units_[cmd.target]) || units_[cmd.target].side == Side::Ming) {
            return;
        }
        for (Unit *unit : selected) {
            unit->order = {};
            unit->order.type = Order::Type::Attack;
            unit->order.target = cmd.target;
        }
        return;
    }
    if (cmd.type == Command::Type::Split) {
        std::vector<UnitId> ids;
        ids.reserve(selected.size());
        for (const Unit *unit : selected) {
            ids.push_back(unit->id);
        }
        for (UnitId id : ids) {
            if (canSplit(*this, id)) {
                splitUnit(id);
            }
        }
        return;
    }
    if (cmd.type == Command::Type::Join) {
        std::vector<UnitId> ids;
        ids.reserve(selected.size());
        for (const Unit *unit : selected) {
            ids.push_back(unit->id);
        }
        const std::vector<std::vector<UnitId>> groups = joinGroups(*this, ids);
        for (const std::vector<UnitId> &group : groups) {
            UnitId anchor = group.front();
            for (UnitId id : group) {
                if (units_[id].men > units_[anchor].men || (units_[id].men == units_[anchor].men && id < anchor)) {
                    anchor = id;
                }
            }
            const float face = units_[anchor].facing;
            const Vec2 point = units_[anchor].pos;
            for (UnitId id : group) {
                Unit &unit = units_[id];
                unit.rejoin = true;
                unit.order = {};
                unit.order.face = face;
                if (id == anchor) {
                    unit.order.type = Order::Type::Hold;
                } else {
                    unit.order.type = Order::Type::Move;
                    unit.order.point = point;
                }
            }
        }
        return;
    }
    Vec2 point = cmd.point;
    if (cmd.type == Command::Type::Retreat) {
        point.x = scenario_.exit.rect.x + scenario_.exit.rect.w * 0.5f;
        point.y = exitOpen_ ? scenario_.exit.retreatYOpen : scenario_.exit.retreatYClosed;
    } else if (cmd.type != Command::Type::Move) {
        return;
    }
    Vec2 center{};
    for (const Unit *unit : selected) {
        center = center + unit->pos;
    }
    center = center * (1.f / static_cast<float>(selected.size()));
    const float face = selected.size() == 1 ? directionOf(selected[0]->pos, point) : directionOf(center, point);
    const float maxX = scenario_.map.width - kOrderEdge;
    const float maxY = scenario_.map.height - kOrderEdge;
    for (Unit *unit : selected) {
        Vec2 offset{};
        if (selected.size() > 1) {
            offset.x = std::clamp(unit->pos.x - center.x, -kFormationLimit, kFormationLimit);
            offset.y = std::clamp(unit->pos.y - center.y, -kFormationLimit, kFormationLimit);
        }
        unit->order = {};
        unit->order.type = Order::Type::Move;
        unit->order.point.x = std::clamp(point.x + offset.x, kOrderEdge, maxX);
        unit->order.point.y = std::clamp(point.y + offset.y, kOrderEdge, maxY);
        unit->order.face = face;
    }
}

void Battle::moveUnit(Unit &unit) {
    const UnitType &type = typeOf(unit);
    const bool cavalry = type.cls == UnitClass::Cavalry;
    unit.prevPos = unit.pos;
    unit.prevFacing = unit.facing;
    unit.recoverSec = std::max(0.f, unit.recoverSec - kTickSeconds);
    unit.reloadSec = std::max(0.f, unit.reloadSec - kTickSeconds);
    if (unit.momentum > 0) {
        unit.momentumSec += kTickSeconds;
        if (unit.momentumSec > kMomentumPeriodSec) {
            unit.momentum -= 1;
            unit.momentumSec = 0.f;
        }
    }
    if (unit.knock.x != 0.f || unit.knock.y != 0.f) {
        unit.pos = unit.pos + unit.knock * kTickSeconds;
        const float decay = static_cast<float>(
            std::pow(static_cast<double>(kKnockDecay), static_cast<double>(kTickSeconds / kKnockDecaySec)));
        unit.knock = unit.knock * decay;
        if (std::fabs(unit.knock.x) + std::fabs(unit.knock.y) < kKnockStop) {
            unit.knock = {};
        }
        clampPos(unit);
    }
    if (unit.state == UnitState::Broken) {
        unit.brokenSec -= kTickSeconds;
        if (unit.brokenSec <= 0.f) {
            unit.state = UnitState::Routing;
            unit.routSec = 0.f;
        }
        return;
    }
    if (unit.state == UnitState::Routing) {
        routMove(unit);
        return;
    }
    if (!unit.contacts.empty() && !unit.breaking) {
        if (unit.order.type == Order::Type::Move && cavalry) {
            unit.breaking = true;
            kill(unit, kBreakawayMen * type.men);
            unit.morale -= kBreakawayMorale;
            emit(Event::Type::Breakaway, unit.id, unit.pos);
            if (!alive(unit)) {
                return;
            }
        } else {
            if (const Unit *contact = primaryContact(unit)) {
                unit.facing = turnToward(unit.facing, directionOf(unit.pos, contact->pos),
                                         type.turn * kContactTurn * kTickSeconds);
            }
            unit.charging = false;
            unit.speed = 0.f;
            unit.stillSec = 0.f;
            return;
        }
    }

    Vec2 goal = unit.pos;
    bool hasGoal = false;
    bool charge = false;
    float distance = 0.f;
    if (unit.order.type == Order::Type::Attack) {
        if (unit.order.target >= units_.size() || !alive(units_[unit.order.target])) {
            unit.order.type = Order::Type::Hold;
            unit.order.face = unit.facing;
            unit.order.target = kNoUnit;
            unit.charging = false;
            return;
        }
        goal = units_[unit.order.target].pos;
        hasGoal = true;
        distance = length(goal - unit.pos);
        const float reach = cavalry ? kCavalryChargeMeters : kInfantryChargeMeters;
        if (type.run > 0.f) {
            charge = unit.charging ? distance < reach * kChargeKeep
                                   : (unit.recoverSec <= 0.f && distance > kChargeMinMeters && distance < reach);
        }
    } else if (unit.order.type == Order::Type::Move) {
        goal = unit.order.point;
        distance = length(goal - unit.pos);
        if (distance < kArriveMeters) {
            unit.order.type = Order::Type::Hold;
            unit.speed = 0.f;
            return;
        }
        hasGoal = true;
    }
    if (!hasGoal) {
        unit.charging = false;
        unit.speed = 0.f;
        unit.stillSec += kTickSeconds;
        if (const Unit *enemy = nearestEnemy(unit, kHoldEnemyMeters)) {
            unit.facing =
                turnToward(unit.facing, directionOf(unit.pos, enemy->pos), type.turn * kHoldEnemyTurn * kTickSeconds);
        } else if (unit.order.face) {
            unit.facing = turnToward(unit.facing, *unit.order.face, type.turn * kTickSeconds);
        }
        return;
    }

    unit.stillSec = 0.f;
    const float want = directionOf(unit.pos, goal);
    unit.facing = turnToward(unit.facing, want, type.turn * (charge ? kChargeTurn : 1.f) * kTickSeconds);
    const float diff = std::fabs(angDiff(want, unit.facing));
    if (charge) {
        if (!unit.charging) {
            unit.charging = true;
            unit.runDist = 0.f;
            unit.volleyDone = false;
            unit.speed = std::max(unit.speed, type.walk);
        }
        const float accel = cavalry ? kCavalryAccel : kInfantryChargeAccel;
        unit.speed = std::min(type.run, unit.speed + accel * kTickSeconds);
    } else {
        unit.charging = false;
        const float accel = cavalry ? kCavalryAccel : kInfantryWalkAccel;
        unit.speed += std::clamp(type.walk - unit.speed, -kSpeedBrake * kTickSeconds, accel * kTickSeconds);
    }
    const float travel = unit.speed * alignOf(cavalry, diff) * terrainSpeed(type.cls, terrainAt(unit.pos));
    unit.pos = unit.pos + Vec2{std::cos(unit.facing), std::sin(unit.facing)} * (travel * kTickSeconds);
    if (unit.charging) {
        unit.runDist += travel * kTickSeconds;
        if (type.threeEyedGun && !unit.volleyDone && distance < kThreeEyedMeters && unit.order.target < units_.size()) {
            unit.volleyDone = true;
            mingVolley(unit, units_[unit.order.target]);
        }
    }
    clampPos(unit);
}

void Battle::routMove(Unit &unit) {
    const UnitType &type = typeOf(unit);
    const bool cavalry = type.cls == UnitClass::Cavalry;
    unit.routSec += kTickSeconds;
    unit.charging = false;
    Vec2 dir{0.f, unit.side == Side::Ming ? -1.f : 1.f};
    if (const Unit *enemy = nearestEnemy(unit, -1.f)) {
        const Vec2 away = unit.pos - enemy->pos;
        const float distance = length(away);
        if (distance > 0.f) {
            const float weight = distance < kRoutFearMeters ? kRoutFearNear : kRoutFearFar;
            dir = dir + away * (weight / distance);
        }
    }
    if (unit.scatterAngle && unit.routSec < kScatterSec) {
        const float weight = kScatterWeight * (1.f - unit.routSec / kScatterSec);
        dir = dir + Vec2{std::cos(*unit.scatterAngle), std::sin(*unit.scatterAngle)} * weight;
    }
    unit.facing = turnToward(unit.facing, std::atan2(dir.y, dir.x), type.turn * kRoutTurn * kTickSeconds);
    const float travel = (cavalry ? kRoutCavalry : kRoutInfantry) * terrainSpeed(type.cls, terrainAt(unit.pos));
    unit.pos = unit.pos + Vec2{std::cos(unit.facing), std::sin(unit.facing)} * (travel * kTickSeconds);
    unit.pos.x = std::clamp(unit.pos.x, kInteriorMeters, scenario_.map.width - kInteriorMeters);
    if (unit.pos.y < -kFleeMeters || unit.pos.y > scenario_.map.height + kFleeMeters) {
        fled(unit);
    }
}

void Battle::clampPos(Unit &unit) const {
    const float outside = unit.deploying ? -kSpawnOutsideMeters : kInteriorMeters;
    const float maxX =
        unit.deploying ? scenario_.map.width + kSpawnOutsideMeters : scenario_.map.width - kInteriorMeters;
    const float maxY =
        unit.deploying ? scenario_.map.height + kSpawnOutsideMeters : scenario_.map.height - kInteriorMeters;
    unit.pos.x = std::clamp(unit.pos.x, outside, maxX);
    unit.pos.y = std::clamp(unit.pos.y, unit.deploying ? -kSpawnOutsideMeters : kInteriorMeters, maxY);
}

void Battle::contactsStep() {
    std::vector<UnitId> live;
    for (Unit &unit : units_) {
        if (!alive(unit)) {
            continue;
        }
        unit.contacts.clear();
        live.push_back(unit.id);
    }
    for (std::size_t i = 0; i < live.size(); ++i) {
        for (std::size_t j = i + 1; j < live.size(); ++j) {
            Unit &first = units_[live[i]];
            Unit &second = units_[live[j]];
            if (!alive(first) || !alive(second)) {
                continue;
            }
            const Vec2 delta = second.pos - first.pos;
            if (dot(delta, delta) > kContactRange * kContactRange) {
                continue;
            }
            if (first.side == second.side) {
                if (first.state == UnitState::Routing || second.state == UnitState::Routing) {
                    continue;
                }
                if (const std::optional<SatHit> hit = sat(unitBox(first, 0.f), unitBox(second, 0.f))) {
                    const Vec2 push = hit->axis * (hit->overlap * kSeparateEven);
                    first.pos = first.pos - push;
                    second.pos = second.pos + push;
                }
                continue;
            }
            if (!sat(unitBox(first, kContactInflate), unitBox(second, kContactInflate))) {
                continue;
            }
            if (units_[live[i]].charging && fastEnough(units_[live[i]], typeOf(units_[live[i]]))) {
                impact(units_[live[i]], units_[live[j]]);
            }
            if (alive(units_[live[i]]) && alive(units_[live[j]]) && units_[live[j]].charging &&
                fastEnough(units_[live[j]], typeOf(units_[live[j]]))) {
                impact(units_[live[j]], units_[live[i]]);
            }
            if (!alive(units_[live[i]]) || !alive(units_[live[j]])) {
                continue;
            }
            Unit &left = units_[live[i]];
            Unit &right = units_[live[j]];
            left.contacts.push_back(right.id);
            right.contacts.push_back(left.id);
            if (const std::optional<SatHit> hit = sat(unitBox(left, 0.f), unitBox(right, 0.f))) {
                float weight = kSeparateEven;
                if (left.state == UnitState::Routing) {
                    weight = kSeparateRout;
                } else if (right.state == UnitState::Routing) {
                    weight = kSeparateRoutOther;
                } else if (isMoving(left) && !isMoving(right)) {
                    weight = kSeparateMover;
                } else if (isMoving(right) && !isMoving(left)) {
                    weight = kSeparateHolder;
                }
                left.pos = left.pos - hit->axis * (hit->overlap * weight);
                right.pos = right.pos + hit->axis * (hit->overlap * (1.f - weight));
            }
        }
    }
    for (Unit &unit : units_) {
        if (alive(unit)) {
            std::sort(unit.contacts.begin(), unit.contacts.end());
        }
    }
    for (UnitId id : live) {
        Unit &unit = units_[id];
        if (!alive(unit)) {
            continue;
        }
        if (unit.breaking && unit.contacts.empty()) {
            unit.breaking = false;
        }
        if (unit.deploying && unit.side == Side::Ming && unit.pos.y > kDeployDoneY) {
            unit.deploying = false;
        }
        clampPos(unit);
        const Aabb &rect = scenario_.exit.rect;
        if (exitOpen_ && unit.side == Side::Ming && !unit.deploying && fighting(unit) && unit.pos.x > rect.x &&
            unit.pos.x < rect.x + rect.w && unit.pos.y < rect.y + rect.h) {
            withdraw(unit);
        }
    }
}

void Battle::impact(Unit &attacker, Unit &target) {
    const UnitType &attackerType = typeOf(attacker);
    const UnitType &targetType = typeOf(target);
    const ChargeDetail detail =
        chargeCalc(attacker, attackerType, target, targetType, terrainAt(attacker.pos), terrainAt(target.pos), false);
    const float ratio = detail.ratio;
    stats_.impacts += 1;
    const bool wasFighting = fighting(target);
    if (wasFighting) {
        stats_.bestImpact = std::max(stats_.bestImpact, ratio);
    }
    attacker.charging = false;
    attacker.speed = 0.f;
    attacker.runDist = 0.f;
    attacker.recoverSec = attackerType.cls == UnitClass::Cavalry ? kRecoverCavalry : kRecoverInfantry;
    if (wasFighting) {
        target.morale -= detail.pikeFront ? kImpactPikeMorale : kImpactMorale + kImpactMoraleScale * ratio;
    }
    kill(target, kImpactKill * ratio * targetType.armor * targetType.men);
    if (detail.pikeFront) {
        kill(attacker, kImpactKillPike * attackerType.men);
        attacker.morale -= kImpactPikeMoraleHit;
        attacker.recoverSec = kImpactPikeRecover;
    } else {
        kill(attacker, kImpactKillSelf * attackerType.men / std::sqrt(std::max(ratio, kImpactRatioFloor)));
    }
    if (!detail.pikeFront && alive(target)) {
        const float push = std::min(kImpactPushMax, kImpactPushBase + kImpactPushScale * ratio);
        const float angle = std::atan2(target.pos.y - attacker.pos.y, target.pos.x - attacker.pos.x);
        target.pos = target.pos + Vec2{std::cos(angle), std::sin(angle)} * push;
        clampPos(target);
    }
    const bool breaks =
        ratio >= kImpactBreak && !detail.pikeFront && wasFighting && alive(target) && !targetType.general;
    Event event;
    event.type = Event::Type::Impact;
    event.tick = tick_;
    event.a = attacker.id;
    event.b = target.id;
    event.pos = (attacker.pos + target.pos) * 0.5f;
    event.value = ratio;
    event.side = detail.side;
    if (detail.pikeFront) {
        event.flags = static_cast<std::uint8_t>(event.flags | kPikeFront);
    }
    if (breaks) {
        event.flags = static_cast<std::uint8_t>(event.flags | kBreaks);
    }
    events_.push_back(event);
    if (breaks) {
        breakUnit(target, attacker.id);
    }
}

void Battle::mingVolley(Unit &attacker, Unit &target) {
    if (!alive(target)) {
        return;
    }
    kill(target, kThreeEyedMen * typeOf(target).men);
    if (fighting(target)) {
        target.morale -= kThreeEyedMorale;
    }
    Event event;
    event.type = Event::Type::ThreeEyedVolley;
    event.tick = tick_;
    event.a = attacker.id;
    event.b = target.id;
    event.pos = attacker.pos;
    events_.push_back(event);
}

void Battle::breakUnit(Unit &unit, UnitId from) {
    if (!fighting(unit)) {
        return;
    }
    const UnitType &type = typeOf(unit);
    const int parts = (!type.general && unit.men >= kBreakSplitMen) ? (unit.men >= kBreakTripleMen ? 3 : 2) : 1;
    unit.state = UnitState::Broken;
    unit.brokenSec = kBrokenSec;
    unit.order = {};
    unit.order.type = Order::Type::Hold;
    unit.charging = false;
    unit.momentum = 0;
    unit.breaking = false;
    if (!unit.everBroken) {
        if (unit.side == Side::Ming) {
            stats_.brokenMing += 1;
        } else {
            stats_.brokenJp += 1;
        }
        unit.everBroken = true;
    }
    unit.morale = std::min(unit.morale, kBrokenMorale);
    Event event;
    event.type = Event::Type::Broken;
    event.tick = tick_;
    event.a = unit.id;
    event.pos = unit.pos;
    event.value = static_cast<float>(parts);
    events_.push_back(event);
    for (Unit &other : units_) {
        if (other.id == unit.id || !alive(other) || !fighting(other)) {
            continue;
        }
        const float distance = length(other.pos - unit.pos);
        if (other.side == unit.side && distance < kBreakAllyRange) {
            const float cover = nearGeneral(other) ? kBreakAllyGeneral : 1.f;
            other.morale -= (kBreakAllyBase + kBreakAllyScale * (1.f - distance / kBreakAllyRange)) * cover;
        } else if (other.side != unit.side && distance < kBreakEnemyRange) {
            const float cap = capOf(other, typeOf(other)) + kBreakEnemyCap;
            other.morale = std::min(cap, other.morale + kBreakEnemyGain);
        }
    }
    for (UnitId id : unit.contacts) {
        if (id >= units_.size() || !fighting(units_[id]) || !typeOf(units_[id]).momentum) {
            continue;
        }
        Unit &other = units_[id];
        const UnitType &otherType = typeOf(other);
        int gained = otherType.momentumBoost ? 2 : 1;
        if (gained == 1) {
            if (const Unit *general = nearGeneral(other)) {
                if (typeOf(*general).momentumBoost) {
                    gained = 2;
                }
            }
        }
        other.momentum = std::min(static_cast<int>(kMomentumCap), other.momentum + gained);
        other.momentumSec = 0.f;
        Event gainedEvent;
        gainedEvent.type = Event::Type::MomentumGained;
        gainedEvent.tick = tick_;
        gainedEvent.a = other.id;
        gainedEvent.pos = other.pos;
        gainedEvent.value = static_cast<float>(gained);
        events_.push_back(gainedEvent);
    }
    if (from == kNoUnit) {
        if (const Unit *contact = primaryContact(unit)) {
            from = contact->id;
        }
    }
    if (from == kNoUnit) {
        if (const Unit *enemy = nearestEnemy(unit, kShatterSeek)) {
            from = enemy->id;
        }
    }
    const UnitId self = unit.id;
    shatter(self, parts, from);
}

void Battle::kill(Unit &unit, float amount) {
    if (!alive(unit)) {
        return;
    }
    const float loss = std::min(std::max(0.f, amount), unit.men);
    unit.men -= loss;
    if (unit.side == Side::Ming) {
        stats_.lostMing += loss;
    } else {
        stats_.lostJp += loss;
    }
    const UnitType &type = typeOf(unit);
    if (type.general) {
        if (unit.men < kGeneralDownMen * unit.maxMen) {
            generalDown(unit);
        }
    } else if (unit.men < std::max(kDisperseMenRatio * unit.maxMen, kDisperseMenMin)) {
        disperse(unit);
    }
}

void Battle::generalDown(Unit &unit) {
    unit.state = UnitState::Gone;
    unit.gone = GoneReason::GeneralKilled;
    emit(Event::Type::GeneralDown, unit.id, unit.pos);
    for (Unit &other : units_) {
        if (!fighting(other) || other.id == unit.id) {
            continue;
        }
        if (length(other.pos - unit.pos) >= kGeneralAuraKill) {
            continue;
        }
        if (other.side == unit.side) {
            other.morale -= kGeneralAllyMorale;
        } else {
            other.morale = std::min(kMoraleMax, other.morale + kGeneralEnemyMorale);
        }
    }
}

void Battle::disperse(Unit &unit) {
    unit.state = UnitState::Gone;
    unit.gone = GoneReason::Dispersed;
    emit(Event::Type::Dispersed, unit.id, unit.pos);
}

void Battle::fled(Unit &unit) {
    unit.state = UnitState::Gone;
    unit.gone = GoneReason::Fled;
    emit(Event::Type::Fled, unit.id, unit.pos);
}

void Battle::withdraw(Unit &unit) {
    if (!alive(unit)) {
        return;
    }
    stats_.exitedMen += unit.men;
    unit.state = UnitState::Gone;
    unit.gone = GoneReason::Withdrawn;
    emit(Event::Type::Withdrawn, unit.id, unit.pos);
}

const Unit *Battle::nearestEnemy(const Unit &self, float limit) const {
    const Unit *best = nullptr;
    float bestDistance = limit;
    for (const Unit &other : units_) {
        if (other.id == self.id || other.side == self.side || !fighting(other)) {
            continue;
        }
        const float distance = length(other.pos - self.pos);
        const bool inRange = limit < 0.f || distance < limit;
        if (inRange &&
            (best == nullptr || distance < bestDistance || (distance == bestDistance && other.id < best->id))) {
            best = &other;
            bestDistance = distance;
        }
    }
    return best;
}

const Unit *Battle::primaryContact(const Unit &self) const {
    const Unit *best = nullptr;
    float bestPower = -1.f;
    for (UnitId id : self.contacts) {
        if (id >= units_.size()) {
            continue;
        }
        const Unit &other = units_[id];
        const float power = fighting(other) ? tq::meleePower(other, typeOf(other), terrainAt(other.pos)) : kUnfitMelee;
        if (best == nullptr || power > bestPower || (power == bestPower && other.id < best->id)) {
            best = &other;
            bestPower = power;
        }
    }
    return best;
}

void Battle::emit(Event::Type type, UnitId id, Vec2 pos) {
    Event event;
    event.type = type;
    event.tick = tick_;
    event.a = id;
    event.pos = pos;
    events_.push_back(event);
}

const Unit *Battle::nearGeneral(const Unit &unit) const {
    for (const Unit &other : units_) {
        if (other.id == unit.id || other.side != unit.side || !fighting(other) || !typeOf(other).general) {
            continue;
        }
        if (length(other.pos - unit.pos) < typeOf(other).aura) {
            return &other;
        }
    }
    return nullptr;
}

void Battle::tryFire(Unit &shooter) {
    const UnitType &shooterType = typeOf(shooter);
    UnitId best = kNoUnit;
    float bestScore = -1.f;
    for (const Unit &other : units_) {
        if (other.side == shooter.side || !alive(other)) {
            continue;
        }
        const float distance = length(other.pos - shooter.pos);
        if (distance > shooterType.range) {
            continue;
        }
        if (!other.charging && distance > shooterType.range * kShotClose) {
            continue;
        }
        const float aim = std::atan2(other.pos.y - shooter.pos.y, other.pos.x - shooter.pos.x);
        if (std::fabs(angDiff(aim, shooter.facing)) > kShotArc) {
            continue;
        }
        const float score = (other.charging ? kShotChargeScore : 1.f) / (distance + kShotRangePad);
        if (score > bestScore) {
            bestScore = score;
            best = other.id;
        }
    }
    if (best == kNoUnit) {
        return;
    }
    Unit &target = units_[best];
    const UnitType &targetType = typeOf(target);
    const float distance = length(target.pos - shooter.pos);
    const float falloff = 1.f - kShotFalloff * distance / shooterType.range;
    const bool charging = target.charging;
    const float strength =
        std::sqrt(shooter.men / shooterType.men) * (shooter.state == UnitState::Wavering ? kShotWaver : 1.f);
    const float charge = charging ? kShotCharge : 1.f;
    kill(target, targetType.men * kShotMen * falloff * strength * targetType.armor * charge);
    if (fighting(target)) {
        const float cover = nearGeneral(target) ? kMeleeNearGeneral : 1.f;
        target.morale -= (kShotMorale + kShotMoraleScale * falloff) * strength * charge * cover;
    }
    if (charging && alive(target)) {
        target.speed *= kShotSlow;
    }
    shooter.reloadSec = shooterType.reload + kShotReloadJitter * rng_.uniform();
    Event event;
    event.type = Event::Type::TeppoVolley;
    event.tick = tick_;
    event.a = shooter.id;
    event.b = target.id;
    event.pos = shooter.pos;
    events_.push_back(event);
}

void Battle::combatTick() {
    const UnitId born = static_cast<UnitId>(units_.size());
    std::vector<UnitId> live;
    for (const Unit &unit : units_) {
        if (alive(unit)) {
            live.push_back(unit.id);
        }
    }
    for (UnitId id : live) {
        Unit &unit = units_[id];
        if (!alive(unit)) {
            continue;
        }
        if (!unit.contacts.empty()) {
            unit.engagedSec += kEngagedGain;
        } else {
            unit.engagedSec = std::max(0.f, unit.engagedSec - kEngagedLoss);
        }
    }

    struct Harm {
        float casualties = 0.f;
        float morale = 0.f;
        bool used = false;
    };
    std::vector<Harm> harm(units_.size());
    for (UnitId id : live) {
        Unit &attacker = units_[id];
        if (!alive(attacker) || attacker.contacts.empty() || !fighting(attacker) || attacker.breaking) {
            continue;
        }
        const float share = 1.f / static_cast<float>(attacker.contacts.size());
        const UnitType &attackerType = typeOf(attacker);
        const float basePower = meleePower(attacker, attackerType, terrainAt(attacker.pos));
        for (UnitId targetId : attacker.contacts) {
            if (targetId >= units_.size() || !alive(units_[targetId])) {
                continue;
            }
            Unit &target = units_[targetId];
            const UnitType &targetType = typeOf(target);
            const bool routed = !fighting(target);
            const float attackPower = basePower * pairMul(attacker, attackerType, target, targetType);
            const ContactSide side = sideOf(target.pos, target.facing, target.depth, target.frontage, attacker.pos);
            float sideMul = 1.f;
            float moraleSide = 1.f;
            if (routed) {
                sideMul = kMeleePursuit;
            } else if (side == ContactSide::Flank) {
                sideMul = kMeleeFlank;
                moraleSide = kMeleeMoraleFlank;
            } else if (side == ContactSide::Rear) {
                sideMul = kMeleeRear;
                moraleSide = kMeleeMoraleRear;
            }
            const float defendPower = routed ? kMeleeRoutPower
                                             : meleePower(target, targetType, terrainAt(target.pos)) *
                                                   pairMul(target, targetType, attacker, attackerType);
            const float ratio = std::clamp(attackPower * sideMul / std::max(defendPower, kMeleePowerFloor),
                                           kMeleeRatioMin, kMeleeRatioMax);
            const float roll = kMeleeRandomBase + kMeleeRandomSpan * rng_.uniform();
            const float cavalryRout = routed && attackerType.cls == UnitClass::Cavalry ? kMeleeCavalryRout : 1.f;
            harm[targetId].used = true;
            harm[targetId].casualties += targetType.men * kMeleeCasualties * attackPower * sideMul * roll * share *
                                         targetType.armor * cavalryRout;
            if (!routed) {
                harm[targetId].morale += (kMeleeMoraleBase + target.engagedSec / kMeleeMoraleEngaged) *
                                         std::sqrt(ratio) * moraleSide * share * roll;
            }
        }
    }
    for (UnitId id = 0; id < harm.size(); ++id) {
        if (!harm[id].used || !alive(units_[id])) {
            continue;
        }
        Unit &target = units_[id];
        const float before = target.men;
        kill(target, harm[id].casualties);
        if (!alive(target) || !fighting(target) || target.maxMen <= 0.f) {
            continue;
        }
        const float lost = (before - target.men) / target.maxMen;
        const float cover = nearGeneral(target) ? kMeleeNearGeneral : 1.f;
        target.morale -= (harm[id].morale + kMeleeLossMorale * lost) * cover;
    }

    for (UnitId id : live) {
        Unit &shooter = units_[id];
        if (alive(shooter) && typeOf(shooter).range > 0.f && fighting(shooter) && shooter.contacts.empty() &&
            shooter.reloadSec <= 0.f && shooter.stillSec >= kChargeUnformedSec) {
            tryFire(shooter);
        }
    }

    for (UnitId id : live) {
        if (!alive(units_[id])) {
            continue;
        }
        if (fighting(units_[id])) {
            Unit &unit = units_[id];
            const float cap = capOf(unit, typeOf(unit));
            if (unit.contacts.empty() && unit.morale < cap) {
                const float extra = nearGeneral(unit) ? kMoraleGeneral : 0.f;
                unit.morale = std::min(cap, unit.morale + kMoraleRecover + extra);
            }
            unit.morale = std::clamp(unit.morale, 0.f, kMoraleMax);
            if (unit.morale < kMoraleBreak) {
                breakUnit(unit);
            } else {
                unit.state = unit.morale < kMoraleWaver ? UnitState::Wavering : UnitState::Steady;
            }
        } else if (units_[id].state == UnitState::Routing) {
            Unit &unit = units_[id];
            kill(unit, kRoutAttrition * unit.maxMen);
            if (!alive(unit)) {
                continue;
            }
            if (nearestEnemy(unit, kRallyRange) == nullptr && unit.routSec > kRallySec) {
                unit.morale = std::min(unit.morale + kRallyGain, capOf(unit, typeOf(unit)));
                if (unit.morale >= kRallyMorale && unit.men > kRallyMen * unit.maxMen) {
                    unit.state = UnitState::Wavering;
                    unit.order = {};
                    unit.order.type = Order::Type::Hold;
                    unit.order.face =
                        unit.side == Side::Ming ? std::numbers::pi_v<float> / 2.f : -std::numbers::pi_v<float> / 2.f;
                    emit(Event::Type::Rallied, unit.id, unit.pos);
                }
            }
        }
    }
    mergeStep(born);
}

Unit Battle::makeUnit(std::uint16_t typeIndex, Vec2 pos, float facing, bool countTotal, bool rollReload) {
    const UnitType &type = defs_.types.at(typeIndex);
    Unit unit;
    unit.type = typeIndex;
    unit.side = type.side;
    unit.pos = pos;
    unit.prevPos = pos;
    unit.facing = facing;
    unit.prevFacing = facing;
    unit.frontage = type.frontage;
    unit.depth = type.depth;
    unit.men = type.men;
    unit.maxMen = type.men;
    unit.morale = type.morale;
    unit.order.type = Order::Type::Hold;
    unit.order.face = facing;
    unit.stillSec = kInitialStillSec;
    if (rollReload && type.range > 0.f) {
        unit.reloadSec = kReloadRollSec * rng_.uniform();
    }
    if (countTotal) {
        if (type.side == Side::Ming) {
            totalMing_ += type.men;
        } else {
            totalJp_ += type.men;
        }
    }
    return unit;
}

std::uint16_t Battle::nextFrag(UnitId origin) {
    if (fragCount_.size() <= origin) {
        fragCount_.resize(static_cast<std::size_t>(origin) + 1);
    }
    fragCount_[origin] = static_cast<std::uint16_t>(fragCount_[origin] + 1);
    return fragCount_[origin];
}

void Battle::shatter(UnitId id, int parts, UnitId from) {
    const Vec2 selfPos = units_[id].pos;
    float base = units_[id].side == Side::Ming ? -std::numbers::pi_v<float> / 2.f : std::numbers::pi_v<float> / 2.f;
    if (from != kNoUnit && from < units_.size()) {
        const Vec2 fromPos = units_[from].pos;
        base = std::atan2(selfPos.y - fromPos.y, selfPos.x - fromPos.x);
    }
    units_[id].facing = base;
    if (parts <= 1) {
        units_[id].knock = Vec2{std::cos(base), std::sin(base)} * kShatterKnock;
        units_[id].scatterAngle = std::nullopt;
        return;
    }

    const float share = units_[id].men / static_cast<float>(parts);
    const float morale = units_[id].morale;
    const UnitId origin = units_[id].origin == kNoUnit ? id : units_[id].origin;
    const bool keepVan = units_[id].vanguard;
    const std::uint16_t typeIndex = units_[id].type;
    const std::string label = units_[id].labelKey;
    std::vector<UnitId> partIds;
    partIds.push_back(id);
    for (int extra = 1; extra < parts; ++extra) {
        Unit created = makeUnit(typeIndex, selfPos, base, false, true);
        created.id = static_cast<UnitId>(units_.size());
        created.labelKey = label;
        partIds.push_back(created.id);
        units_.push_back(std::move(created));
    }
    for (int index = 0; index < parts; ++index) {
        Unit &part = units_[partIds[static_cast<std::size_t>(index)]];
        const float angle = base + (static_cast<float>(index) / static_cast<float>(parts - 1) - 0.5f) * kShatterSpread +
                            (rng_.uniform() - 0.5f) * kShatterJitter;
        const float speed = kShatterSpeed + kShatterSpeedSpan * rng_.uniform();
        part.fragNo = nextFrag(origin);
        part.men = share;
        part.maxMen = share;
        part.morale = morale;
        part.state = UnitState::Broken;
        part.brokenSec = kBrokenSec;
        part.order = {};
        part.order.type = Order::Type::Hold;
        part.charging = false;
        part.speed = 0.f;
        part.momentum = 0;
        part.everBroken = true;
        part.isFragment = true;
        part.origin = origin;
        part.knock = Vec2{std::cos(angle), std::sin(angle)} * speed;
        part.scatterAngle = angle;
        part.facing = angle;
        part.routSec = 0.f;
        part.breaking = false;
        part.recoverSec = 0.f;
        part.vanguard = index == 0 ? keepVan : false;
        sizeFrag(part, typeOf(part));
    }
    Event event;
    event.type = Event::Type::Shattered;
    event.tick = tick_;
    event.a = id;
    event.pos = units_[id].pos;
    event.value = static_cast<float>(parts);
    events_.push_back(event);
}

void Battle::splitUnit(UnitId id) {
    const float facing = units_[id].facing;
    const Vec2 pos = units_[id].pos;
    const float halfMen = units_[id].men * 0.5f;
    const float halfMax = units_[id].maxMen * 0.5f;
    const UnitId origin = units_[id].origin == kNoUnit ? id : units_[id].origin;
    const std::string label = units_[id].labelKey;
    Unit created = makeUnit(units_[id].type, pos, facing, false, false);
    created.id = static_cast<UnitId>(units_.size());
    created.labelKey = label;
    created.morale = units_[id].morale;
    created.state = units_[id].state;
    created.recoverSec = units_[id].recoverSec;
    created.reloadSec = units_[id].reloadSec;
    created.stillSec = units_[id].stillSec;
    created.momentum = units_[id].momentum;
    created.everBroken = units_[id].everBroken;
    created.vanguard = false;
    const UnitId other = created.id;
    units_.push_back(std::move(created));

    units_[id].fragNo = nextFrag(origin);
    units_[other].fragNo = nextFrag(origin);
    for (UnitId part : {id, other}) {
        Unit &unit = units_[part];
        unit.men = halfMen;
        unit.maxMen = halfMax;
        unit.origin = origin;
        unit.isFragment = false;
        unit.rejoin = false;
        sizeFrag(unit, typeOf(unit));
    }
    const Vec2 right{-std::sin(facing), std::cos(facing)};
    const float off = units_[id].frontage * 0.5f + kSplitGap;
    units_[id].pos = pos - right * off;
    units_[other].pos = pos + right * off;
    clampPos(units_[id]);
    clampPos(units_[other]);
    units_[id].prevPos = units_[id].pos;
    units_[other].prevPos = units_[other].pos;
    for (UnitId part : {id, other}) {
        Unit &unit = units_[part];
        unit.order = {};
        unit.order.type = Order::Type::Hold;
        unit.order.face = facing;
    }
    Event event;
    event.type = Event::Type::Split;
    event.tick = tick_;
    event.a = id;
    event.b = other;
    event.pos = units_[id].pos;
    events_.push_back(event);
}

void Battle::mergeStep(UnitId limit) {
    std::vector<UnitId> candidates;
    const UnitId count = std::min(limit, static_cast<UnitId>(units_.size()));
    for (UnitId id = 0; id < count; ++id) {
        const Unit &unit = units_[id];
        const bool knocked = unit.knock.x != 0.f || unit.knock.y != 0.f;
        if (!alive(unit) || !(unit.isFragment || unit.rejoin) || !fighting(unit) || !unit.contacts.empty() || knocked ||
            unit.origin == kNoUnit) {
            continue;
        }
        candidates.push_back(id);
    }
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        for (std::size_t j = i + 1; j < candidates.size(); ++j) {
            Unit &left = units_[candidates[i]];
            Unit &right = units_[candidates[j]];
            if (!alive(left) || !alive(right) || left.origin != right.origin) {
                continue;
            }
            if (length(right.pos - left.pos) > kMergeDistance) {
                continue;
            }
            const float total = left.men + right.men;
            if (total > 0.f) {
                left.morale = (left.morale * left.men + right.morale * right.men) / total;
            }
            left.men = total;
            left.maxMen += right.maxMen;
            right.state = UnitState::Gone;
            right.gone = GoneReason::Merged;
            left.rejoin = left.rejoin || right.rejoin;
            left.fragNo = 0;
            int remaining = 0;
            for (const Unit &other : units_) {
                if (alive(other) && other.origin == left.origin) {
                    remaining += 1;
                }
            }
            if (remaining == 1) {
                left.isFragment = false;
                left.rejoin = false;
            }
            sizeFrag(left, typeOf(left));
            Event event;
            event.type = Event::Type::Merged;
            event.tick = tick_;
            event.a = left.id;
            event.b = right.id;
            event.pos = left.pos;
            event.value = left.men;
            events_.push_back(event);
        }
    }
}

bool canSplit(const Battle &battle, UnitId id) {
    if (id >= battle.units().size()) {
        return false;
    }
    const Unit &unit = battle.unit(id);
    const bool fighting = unit.state == UnitState::Steady || unit.state == UnitState::Wavering;
    return fighting && unit.contacts.empty() && !battle.typeOf(unit).general && !unit.deploying &&
           unit.men >= kSplitMinMen;
}

std::vector<std::vector<UnitId>> joinGroups(const Battle &battle, std::span<const UnitId> selection) {
    std::map<UnitId, std::vector<UnitId>> grouped;
    for (UnitId id : selection) {
        if (id >= battle.units().size()) {
            continue;
        }
        const Unit &unit = battle.unit(id);
        const bool fighting = unit.state == UnitState::Steady || unit.state == UnitState::Wavering;
        if (!fighting || unit.origin == kNoUnit || unit.isFragment) {
            continue;
        }
        std::vector<UnitId> &group = grouped[unit.origin];
        if (std::find(group.begin(), group.end(), id) == group.end()) {
            group.push_back(id);
        }
    }
    std::vector<std::vector<UnitId>> result;
    for (auto &[origin, group] : grouped) {
        (void)origin;
        if (group.size() < 2) {
            continue;
        }
        std::sort(group.begin(), group.end());
        result.push_back(std::move(group));
    }
    return result;
}

} // namespace tq
