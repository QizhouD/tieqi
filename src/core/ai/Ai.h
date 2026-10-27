#pragma once

#include "core/defs/UnitType.h"
#include "core/sim/Unit.h"

#include <span>

namespace tq {

float scoreTarget(const Unit &self, const UnitType &selfType, const Unit &target, const UnitType &targetType,
                  std::span<const Unit> units, const GameDefs &defs);

void approach(Unit &self, const Unit &focus, float keep, float mapWidth, float mapHeight);

} // namespace tq
