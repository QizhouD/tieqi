#pragma once

#include "core/math/Vec2.h"
#include "core/rules/Constants.h"

#include <cstdint>
#include <optional>

namespace tq {

using UnitId = std::uint32_t;
inline constexpr UnitId kNoUnit = 0xFFFFFFFF;

enum class Side : std::uint8_t { Ming, Jp };
enum class UnitState : std::uint8_t { Steady, Wavering, Broken, Routing, Gone };
enum class GoneReason : std::uint8_t { None, Dispersed, Fled, Withdrawn, GeneralKilled, Merged };
enum class Terrain : std::uint8_t { Open, Road, Hill, Paddy };
enum class ContactSide : std::uint8_t { Front, Flank, Rear };

struct Order {
    enum class Type : std::uint8_t { Hold, Move, Attack } type = Type::Hold;
    Vec2 point{};
    std::optional<float> face;
    UnitId target = kNoUnit;
};

} // namespace tq
