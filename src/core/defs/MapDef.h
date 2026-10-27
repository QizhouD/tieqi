#pragma once

#include "core/math/Vec2.h"

#include <cstdint>
#include <string>
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

enum class TextAlign : std::uint8_t { Left, Center };

struct MapLabel {
    std::string key;
    Vec2 pos{};
    float size = 0.f;
    TextAlign align = TextAlign::Left;
    bool faint = false;
};

struct MapDef {
    float width = 0.f;
    float height = 0.f;
    std::vector<Aabb> paddies;
    std::vector<Ellipse> hills;
    RoadDef road;
    Vec2 postStation{};
    std::vector<MapLabel> labels;
};

} // namespace tq
