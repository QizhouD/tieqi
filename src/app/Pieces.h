#pragma once

#include "core/math/Geometry.h"
#include "core/rules/Constants.h"
#include "core/sim/Unit.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace tq {

// 棋子和方阵的尺寸、朝向、翻面都从单位快照算出来，画的时候不再改模拟。
// 公式对照原型 drawBlock / drawChit，以及表现层规格。

struct Rgba {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;

    constexpr bool operator==(const Rgba &) const = default;
};

struct Pose {
    Vec2 pos{};
    float facing = 0.f;
};

struct ChitFace {
    float scaleX = 1.f;
    bool back = false;
};

struct ChitColors {
    Rgba face{};
    Rgba edge{};
    Rgba glyph{};
};

enum class ArcKind : std::uint8_t { None, Recover, Reload };

inline bool isFighting(UnitState state) { return state == UnitState::Steady || state == UnitState::Wavering; }

inline Pose interpolatedPose(const Unit &unit, float alpha) {
    Pose pose;
    pose.pos = unit.prevPos + (unit.pos - unit.prevPos) * alpha;
    pose.facing = unit.prevFacing + angDiff(unit.facing, unit.prevFacing) * alpha;
    return pose;
}

inline int blockColumns(float frontage) { return std::max(1, static_cast<int>(std::floor(frontage / 6.f))); }

inline int blockRows(float depth) { return std::max(1, static_cast<int>(std::floor(depth / 6.f))); }

inline int blockDots(float frontage, float depth, float men, float maxMen) {
    if (!(maxMen > 0.f)) {
        return 0;
    }
    const int columns = blockColumns(frontage);
    const int rows = blockRows(depth);
    return static_cast<int>(std::lround(static_cast<float>(columns * rows) * men / maxMen));
}

// 点的左上角，局部坐标：x 沿纵深，正面在 +x；y 沿正面宽度。
inline Vec2 blockDotLocal(float depth, float frontage, int index, int columns, Vec2 jitter) {
    const int safeColumns = std::max(1, columns);
    const int row = index / safeColumns;
    const int column = index % safeColumns;
    const float halfDepth = depth * 0.5f;
    const float halfFrontage = frontage * 0.5f;
    return {halfDepth - 4.5f - static_cast<float>(row) * 6.f + jitter.x,
            -halfFrontage + 1.7f + static_cast<float>(column) * 6.f + jitter.y};
}

inline Vec2 routingJitter(std::uint32_t id, int index) {
    const int along = (static_cast<int>(id) * 7 + index * 13) % 7 - 3;
    const int across = (index * 11 + static_cast<int>(id)) % 7 - 3;
    return {static_cast<float>(along) * 1.6f, static_cast<float>(across) * 1.6f};
}

// 原型 cur 是游戏秒速度。本项目 speed 已除以 8，线长仍按 cur × 0.12 米。
inline float chargeLineLength(std::uint32_t id, int lane, float speedMetersPerSecond) {
    const int mod = (static_cast<int>(id) + lane * 3) % 4;
    return 10.f + static_cast<float>(mod) * 4.f + speedMetersPerSecond * 0.96f;
}

inline float chitPixels(float pixelsPerMeter, float maxMen, float typeMen) {
    const float base = std::clamp(48.f * pixelsPerMeter, 22.f, 34.f);
    if (typeMen > 0.f && maxMen < typeMen) {
        const float ratio = std::clamp(maxMen / typeMen, 0.f, 1.f);
        return base * (0.58f + 0.42f * std::sqrt(ratio));
    }
    return base;
}

// 和位置一样落后一个 tick：alpha 为 0 时画上一拍结束时的 brokenSec。
inline float presentedBrokenSec(float brokenSec, float alpha) { return brokenSec + kTickSeconds * (1.f - alpha); }

inline ChitFace chitFace(UnitState state, float brokenSec) {
    if (state == UnitState::Broken) {
        const float progress = std::clamp(1.f - brokenSec / kBrokenSec, 0.f, 1.f);
        const float cosine = std::cos(progress * std::numbers::pi_v<float>);
        return {std::max(std::abs(cosine), 0.06f), cosine < 0.f};
    }
    if (state == UnitState::Routing) {
        return {1.f, true};
    }
    return {};
}

inline float chitWobble(UnitState state, float timeSec, std::uint32_t id) {
    if (state == UnitState::Wavering) {
        return std::sin(timeSec * 8.f + static_cast<float>(id)) * 0.06f;
    }
    if (state == UnitState::Routing) {
        return std::sin(timeSec * 13.f + static_cast<float>(id) * 2.f) * 0.14f;
    }
    return 0.f;
}

inline ChitColors chitColors(Side side, bool general, bool back) {
    if (back) {
        return {{0xbd, 0xb0, 0x9a}, {0x6e, 0x62, 0x53}, {0x2a, 0x22, 0x1b}};
    }
    if (side == Side::Ming) {
        if (general) {
            return {{0xd9, 0xb4, 0x66}, {0xa8, 0x38, 0x2a}, {0xa8, 0x38, 0x2a}};
        }
        return {{0xa8, 0x38, 0x2a}, {0xd9, 0xb4, 0x66}, {0xf4, 0xdb, 0xa4}};
    }
    if (general) {
        return {{0xef, 0xe5, 0xcb}, {0x2f, 0x3a, 0x56}, {0x2f, 0x3a, 0x56}};
    }
    return {{0x2f, 0x3a, 0x56}, {0xaa, 0xb4, 0xc9}, {0xef, 0xe5, 0xcb}};
}

inline Rgba moraleColor(UnitState state) {
    switch (state) {
    case UnitState::Steady:
        return {0x4f, 0x8a, 0x4b};
    case UnitState::Wavering:
        return {0xc9, 0x96, 0x2f};
    case UnitState::Broken:
        return {0xb8, 0x40, 0x2c};
    case UnitState::Routing:
    case UnitState::Gone:
        return {0x85, 0x77, 0x69};
    }
    return {0x85, 0x77, 0x69};
}

inline constexpr Rgba kMenBar{0xef, 0xe5, 0xcb};
inline constexpr Rgba kRecoverArc{0xd9, 0xb4, 0x66};
inline constexpr Rgba kReloadArc{0xd8, 0xd0, 0xc0};

inline float menFraction(float men, float maxMen) {
    if (!(maxMen > 0.f)) {
        return 0.f;
    }
    return std::clamp(men / maxMen, 0.f, 1.f);
}

inline float moraleFraction(float morale) { return std::clamp(morale / 100.f, 0.f, 1.f); }

inline float recoverFraction(float recoverSec) {
    if (!(recoverSec > 0.f)) {
        return 0.f;
    }
    const float denom = recoverSec > 48.f ? 72.f : 48.f;
    return std::clamp(recoverSec / denom, 0.f, 1.f);
}

inline float reloadFraction(float reloadSec, float typeReload) {
    if (!(reloadSec > 0.f) || !(typeReload > 0.f)) {
        return 0.f;
    }
    return std::clamp(reloadSec / typeReload, 0.f, 1.f);
}

inline ArcKind timerArc(UnitState state, float recoverSec, float reloadSec, float typeRange) {
    if (!isFighting(state)) {
        return ArcKind::None;
    }
    if (recoverSec > 0.f) {
        return ArcKind::Recover;
    }
    if (typeRange > 0.f && reloadSec > 0.f) {
        return ArcKind::Reload;
    }
    return ArcKind::None;
}

} // namespace tq
