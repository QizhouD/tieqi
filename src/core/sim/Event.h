#pragma once

#include "core/sim/Types.h"

#include <cstdint>

namespace tq {

inline constexpr std::uint8_t kPikeFront = 1;
inline constexpr std::uint8_t kBreaks = 2;
inline constexpr std::uint8_t kImportant = 4;
inline constexpr std::uint8_t kAssaultFailed = 8;

struct Event {
    enum class Type : std::uint8_t {
        Impact,
        ThreeEyedVolley,
        TeppoVolley,
        Breakaway,
        Broken,
        Shattered,
        Rallied,
        Merged,
        Split,
        Dispersed,
        Fled,
        Withdrawn,
        GeneralDown,
        MomentumGained,
        ScoutWarning,
        PhaseChanged,
        Reinforcement,
        GrandAssault,
        BattleEnded
    } type = Type::Impact;
    std::uint32_t tick = 0;
    UnitId a = kNoUnit;
    UnitId b = kNoUnit;
    Vec2 pos{};
    float value = 0.f;
    ContactSide side = ContactSide::Front;
    std::uint8_t flags = 0;
};

} // namespace tq
