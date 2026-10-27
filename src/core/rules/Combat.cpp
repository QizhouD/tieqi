#include "core/rules/Combat.h"

#include "core/math/Geometry.h"

#include <algorithm>
#include <cmath>

namespace tq {
namespace {

bool fighting(const Unit &unit) { return unit.state == UnitState::Steady || unit.state == UnitState::Wavering; }

float strengthOf(const Unit &unit, const UnitType &type) { return std::sqrt(unit.men / type.men); }

} // namespace

ChargeDetail chargeCalc(const Unit &attacker, const UnitType &attackerType, const Unit &target,
                        const UnitType &targetType, Terrain attackerTerrain, Terrain targetTerrain, bool predict) {
    ChargeDetail detail;
    detail.side = sideOf(target.pos, target.facing, target.depth, target.frontage, attacker.pos);
    detail.base = attackerType.impact;
    if (predict || attackerType.run <= 0.f) {
        detail.speed = 1.f;
    } else {
        detail.speed = std::clamp(attacker.speed / attackerType.run, kChargeSpeedFloor, 1.f);
    }
    if (detail.side == ContactSide::Flank) {
        detail.sideMul = kChargeFlank;
    } else if (detail.side == ContactSide::Rear) {
        detail.sideMul = kChargeRear;
    }
    if (target.state == UnitState::Wavering) {
        detail.stateMul = kChargeWavering;
    } else if (target.state == UnitState::Broken || target.state == UnitState::Routing) {
        detail.stateMul = kChargeBroken;
    }
    if (targetType.range > 0.f && fighting(target)) {
        if (target.reloadSec > kChargeReloadSec) {
            detail.gunMul = kChargeReloadMul;
        } else if (target.stillSec < kChargeUnformedSec) {
            detail.gunMul = kChargeUnformedMul;
        }
    }
    detail.paddy = attackerTerrain == Terrain::Paddy || targetTerrain == Terrain::Paddy;
    detail.uphill = targetTerrain == Terrain::Hill && attackerTerrain != Terrain::Hill;
    const float terrain = (detail.paddy ? kChargePaddy : 1.f) * (detail.uphill ? kChargeUphill : 1.f);
    detail.pikeFront = targetType.pike && detail.side == ContactSide::Front && fighting(target);
    detail.pikeMul = detail.pikeFront ? kChargePike : 1.f;
    detail.momentum = 1.f + kChargeMomentumStep * static_cast<float>(attacker.momentum);
    detail.ownWaver = attacker.state == UnitState::Wavering ? kChargeOwnWaver : 1.f;
    detail.strength = strengthOf(attacker, attackerType);
    detail.defense = targetType.defense;
    const float targetStrength = strengthOf(target, targetType);
    const float steady = target.state == UnitState::Steady ? 1.f : kChargeResistSteady;
    const float power = detail.base * detail.speed * detail.sideMul * detail.stateMul * detail.gunMul * terrain *
                        detail.pikeMul * detail.momentum * detail.ownWaver * detail.strength;
    const float resist = std::max(detail.defense * targetStrength * steady, kChargeResistFloor);
    detail.ratio = power / resist;
    return detail;
}

float meleePower(const Unit &unit, const UnitType &type, Terrain terrain) {
    float power = type.melee * std::sqrt(unit.men / type.men);
    if (unit.state == UnitState::Wavering) {
        power *= kMeleeWaver;
    }
    power *= 1.f + kMeleeMomentum * static_cast<float>(unit.momentum);
    if (type.cls == UnitClass::Cavalry && unit.recoverSec > 0.f) {
        power *= kMeleeRecover;
    }
    if (type.cls == UnitClass::Cavalry && terrain == Terrain::Paddy) {
        power *= kMeleePaddy;
    }
    if (type.cls == UnitClass::Cavalry && unit.engagedSec > kMeleeGrindSec) {
        power *= kMeleeGrind;
    }
    return power;
}

float pairMul(const Unit &left, const UnitType &leftType, const Unit &right, const UnitType &rightType) {
    float factor = 1.f;
    if (leftType.pike && rightType.cls == UnitClass::Cavalry &&
        sideOf(left.pos, left.facing, left.depth, left.frontage, right.pos) == ContactSide::Front) {
        factor *= kPikeVsCavalry;
    }
    if (leftType.cls == UnitClass::Cavalry && rightType.pike && fighting(right) &&
        sideOf(right.pos, right.facing, right.depth, right.frontage, left.pos) == ContactSide::Front) {
        factor *= kCavalryVsPike;
    }
    return factor;
}

MeleeDetail meleeCalc(const Unit &attacker, const UnitType &attackerType, const Unit &target,
                      const UnitType &targetType, Terrain attackerTerrain, Terrain targetTerrain) {
    MeleeDetail detail;
    detail.side = sideOf(target.pos, target.facing, target.depth, target.frontage, attacker.pos);
    detail.pursuit = !fighting(target);
    detail.power = meleePower(attacker, attackerType, attackerTerrain);
    detail.pair = pairMul(attacker, attackerType, target, targetType);
    detail.grind = attackerType.cls == UnitClass::Cavalry && attacker.engagedSec > kMeleeGrindSec;
    if (detail.pursuit) {
        detail.sideMul = kMeleePursuit;
    } else if (detail.side == ContactSide::Flank) {
        detail.sideMul = kMeleeFlank;
    } else if (detail.side == ContactSide::Rear) {
        detail.sideMul = kMeleeRear;
    }
    const float defenderPower = detail.pursuit ? kMeleeRoutPower
                                               : meleePower(target, targetType, targetTerrain) *
                                                     pairMul(target, targetType, attacker, attackerType);
    detail.defender = 1.f / std::max(defenderPower, kMeleePowerFloor);
    detail.ratio = detail.power * detail.pair * detail.sideMul * detail.defender;
    return detail;
}

void sizeFrag(Unit &unit, const UnitType &type) {
    if (type.men <= 0.f) {
        return;
    }
    const double ratio = std::clamp(static_cast<double>(unit.maxMen) / static_cast<double>(type.men),
                                    static_cast<double>(kFragMenFloor), 1.0);
    const double front = static_cast<double>(type.frontage) * std::pow(ratio, static_cast<double>(kFragFrontExp));
    const double depth = static_cast<double>(type.depth) * std::pow(ratio, static_cast<double>(kFragDepthExp));
    unit.frontage = static_cast<float>(std::max(static_cast<double>(kFragFrontMin), front));
    unit.depth = static_cast<float>(std::max(static_cast<double>(kFragDepthMin), depth));
}

} // namespace tq
