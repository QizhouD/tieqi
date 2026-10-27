#pragma once

#include "core/math/Vec2.h"

#include <vector>

namespace tq {

struct Aabb {
    float x = 0.f;
    float y = 0.f;
    float w = 0.f;
    float h = 0.f;
};

struct Ellipse {
    float cx = 0.f;
    float cy = 0.f;
    float rx = 0.f;
    float ry = 0.f;
};

struct RoadDef {
    float halfWidth = 0.f;
    std::vector<Vec2> points;
};

// 地形用的地图。地名、驿舍由 D1 补进同一结构。
struct MapDef {
    float width = 0.f;
    float height = 0.f;
    std::vector<Aabb> paddies;
    std::vector<Ellipse> hills;
    RoadDef road;
};

} // namespace tq
