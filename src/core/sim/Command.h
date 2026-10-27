#pragma once

#include "core/sim/Types.h"

#include <cstdint>
#include <vector>

namespace tq {

struct Command {
    std::uint32_t tick = 0;
    enum class Type : std::uint8_t { Move, Attack, Hold, Retreat, Split, Join } type = Type::Move;
    std::vector<UnitId> units;
    Vec2 point{};
    UnitId target = kNoUnit;
};

} // namespace tq
