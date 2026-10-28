#pragma once

#include "core/math/Vec2.h"
#include "core/rules/Constants.h"
#include "core/sim/Types.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <optional>

namespace tq {

// a - b, wrapped to (-pi, pi]. Exactly -pi becomes +pi, matching the spec's open interval.
inline float angDiff(float a, float b) {
    const float pi = std::numbers::pi_v<float>;
    const float twoPi = 2.f * pi;
    float delta = std::fmod(a - b, twoPi);
    if (delta > pi) {
        delta -= twoPi;
    } else if (delta <= -pi) {
        delta += twoPi;
    }
    return delta;
}

// facing += clamp(angDiff(target, facing), −step, step)
inline float turnToward(float facing, float target, float step) {
    return facing + std::clamp(angDiff(target, facing), -step, step);
}

struct Obb {
    Vec2 center;
    Vec2 axis0;
    Vec2 axis1;
    float halfDepth = 0.f;
    float halfFrontage = 0.f;
};

inline Obb obb(Vec2 center, float facing, float depth, float frontage, float inflate) {
    const float along = std::cos(facing);
    const float across = std::sin(facing);
    Obb box;
    box.center = center;
    box.axis0 = {along, across};
    box.axis1 = {-across, along};
    box.halfDepth = depth * 0.5f + inflate;
    box.halfFrontage = frontage * 0.5f + inflate;
    return box;
}

struct SatHit {
    Vec2 axis;
    float overlap = 0.f;
};

inline std::optional<SatHit> sat(const Obb &a, const Obb &b) {
    const Vec2 axes[] = {a.axis0, a.axis1, b.axis0, b.axis1};
    float minOverlap = std::numeric_limits<float>::infinity();
    Vec2 axis{};
    for (const Vec2 &normal : axes) {
        const float centerA = dot(a.center, normal);
        const float radiusA =
            a.halfDepth * std::fabs(dot(a.axis0, normal)) + a.halfFrontage * std::fabs(dot(a.axis1, normal));
        const float centerB = dot(b.center, normal);
        const float radiusB =
            b.halfDepth * std::fabs(dot(b.axis0, normal)) + b.halfFrontage * std::fabs(dot(b.axis1, normal));
        const float overlap =
            std::min(centerA + radiusA, centerB + radiusB) - std::max(centerA - radiusA, centerB - radiusB);
        if (overlap <= 0.f) {
            return std::nullopt;
        }
        if (overlap < minOverlap) {
            minOverlap = overlap;
            axis = normal;
        }
    }
    if (dot(b.center - a.center, axis) < 0.f) {
        axis = -axis;
    }
    return SatHit{axis, minOverlap};
}

// Which side of the defender a is on. Facing 0 is east and pi/2 is south.
inline ContactSide sideOf(Vec2 defenderPos, float facing, float depth, float frontage, Vec2 attackerPos) {
    const float along = std::cos(facing);
    const float across = std::sin(facing);
    const Vec2 forward{along, across};
    const Vec2 lateral{-across, along};
    const Vec2 delta = attackerPos - defenderPos;
    const float lx = dot(delta, forward);
    const float ly = dot(delta, lateral);
    const float nx = lx / (depth * 0.5f + kContactSidePad);
    const float ny = ly / (frontage * 0.5f + kContactSidePad);
    if (std::fabs(nx) >= std::fabs(ny)) {
        return lx > 0.f ? ContactSide::Front : ContactSide::Rear;
    }
    return ContactSide::Flank;
}

} // namespace tq
