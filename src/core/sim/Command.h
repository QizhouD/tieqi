#pragma once

#include "core/sim/Types.h"

#include <cstdint>
#include <vector>

namespace tq {

struct Command {
    std::uint32_t tick = 0;
    // Spawn and Rout come only from the dev tools. They ignore the side filter that player orders use.
    enum class Type : std::uint8_t { Move, Attack, Hold, Retreat, Split, Join, Spawn, Rout } type = Type::Move;
    std::vector<UnitId> units;
    Vec2 point{};
    UnitId target = kNoUnit;
    // Spawn only: index into GameDefs::types and the new unit's facing.
    std::uint16_t unitType = 0;
    float face = 0.f;
};

} // namespace tq
