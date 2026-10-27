#pragma once

#include "core/defs/MapDef.h"
#include "core/defs/UnitType.h"
#include "core/math/Vec2.h"
#include "core/rules/Constants.h"
#include "core/sim/Types.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace tq {

inline float distanceToSegment(Vec2 point, Vec2 start, Vec2 end) {
    const Vec2 span = end - start;
    const float lengthSquared = dot(span, span);
    const Vec2 fromStart = point - start;
    if (lengthSquared == 0.f) {
        return std::sqrt(dot(fromStart, fromStart));
    }
    const float t = std::clamp(dot(fromStart, span) / lengthSquared, 0.f, 1.f);
    const Vec2 closest = start + span * t;
    const Vec2 offset = point - closest;
    return std::sqrt(dot(offset, offset));
}

inline bool contains(const Aabb &box, Vec2 point) {
    return point.x >= box.x && point.x <= box.x + box.w && point.y >= box.y && point.y <= box.y + box.h;
}

inline bool contains(const Ellipse &hill, Vec2 point) {
    if (hill.rx == 0.f || hill.ry == 0.f) {
        return false;
    }
    const float alongX = (point.x - hill.cx) / hill.rx;
    const float alongY = (point.y - hill.cy) / hill.ry;
    return alongX * alongX + alongY * alongY < 1.f;
}

// 水田（含边界）、山地椭圆、大路，先命中先返回。
inline Terrain terrainAt(const MapDef &map, Vec2 point) {
    for (const Aabb &paddy : map.paddies) {
        if (contains(paddy, point)) {
            return Terrain::Paddy;
        }
    }
    for (const Ellipse &hill : map.hills) {
        if (contains(hill, point)) {
            return Terrain::Hill;
        }
    }
    const std::vector<Vec2> &points = map.road.points;
    for (std::size_t i = 0; i + 1 < points.size(); ++i) {
        if (distanceToSegment(point, points[i], points[i + 1]) < map.road.halfWidth) {
            return Terrain::Road;
        }
    }
    return Terrain::Open;
}

inline float terrainSpeed(UnitClass unitClass, Terrain terrain) {
    const bool cavalry = unitClass == UnitClass::Cavalry;
    switch (terrain) {
    case Terrain::Paddy:
        return cavalry ? kTerrainPaddyCavalry : kTerrainPaddyInfantry;
    case Terrain::Hill:
        return cavalry ? kTerrainHillCavalry : kTerrainHillInfantry;
    case Terrain::Road:
        return kTerrainRoad;
    case Terrain::Open:
        return kTerrainOpen;
    }
    return kTerrainOpen;
}

} // namespace tq
