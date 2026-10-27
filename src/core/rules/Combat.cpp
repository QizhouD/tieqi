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

} // namespace tq
