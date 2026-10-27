#include "core/sim/Battle.h"

#include "core/math/Geometry.h"
#include "core/rules/Combat.h"
#include "core/rules/Terrain.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
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

float strength(const Unit &unit, const UnitType &type) { return std::sqrt(unit.men / type.men); }

float meleePower(const Unit &unit, const UnitType &type) { return type.melee * strength(unit, type); }

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
        // C4 补战斗结算。
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
            if (first.charging && fastEnough(first, typeOf(first))) {
                impact(first, second);
            }
            if (alive(first) && alive(second) && second.charging && fastEnough(second, typeOf(second))) {
                impact(second, first);
            }
            if (!alive(first) || !alive(second)) {
                continue;
            }
            first.contacts.push_back(second.id);
            second.contacts.push_back(first.id);
            if (const std::optional<SatHit> hit = sat(unitBox(first, 0.f), unitBox(second, 0.f))) {
                float weight = kSeparateEven;
                if (first.state == UnitState::Routing) {
                    weight = kSeparateRout;
                } else if (second.state == UnitState::Routing) {
                    weight = kSeparateRoutOther;
                } else if (isMoving(first) && !isMoving(second)) {
                    weight = kSeparateMover;
                } else if (isMoving(second) && !isMoving(first)) {
                    weight = kSeparateHolder;
                }
                first.pos = first.pos - hit->axis * (hit->overlap * weight);
                second.pos = second.pos + hit->axis * (hit->overlap * (1.f - weight));
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
        breakUnit(target);
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

void Battle::breakUnit(Unit &unit) {
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
        const float power = fighting(other) ? meleePower(other, typeOf(other)) : kUnfitMelee;
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

} // namespace tq
