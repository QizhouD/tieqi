#pragma once

#include "core/defs/UnitType.h"
#include "core/sim/Unit.h"

namespace tq {

struct ChargeDetail {
    float ratio = 0.f;
    ContactSide side = ContactSide::Front;
    bool pikeFront = false;
    float base = 1.f;
    float speed = 1.f;
    float sideMul = 1.f;
    float stateMul = 1.f;
    float gunMul = 1.f;
    bool paddy = false;
    bool uphill = false;
    float pikeMul = 1.f;
    float momentum = 1.f;
    float ownWaver = 1.f;
    float strength = 1.f;
    float defense = 1.f;
};

ChargeDetail chargeCalc(const Unit &attacker, const UnitType &attackerType, const Unit &target,
                        const UnitType &targetType, Terrain attackerTerrain, Terrain targetTerrain, bool predict);

inline bool fastEnough(const Unit &unit, const UnitType &type) {
    return unit.speed >= kFastEnoughSpeed * type.run && unit.runDist >= kFastEnoughDist;
}

} // namespace tq
