#pragma once

namespace tq {

struct Vec2 {
    float x = 0.f;
    float y = 0.f;

    constexpr Vec2() = default;
    constexpr Vec2(float xIn, float yIn) : x(xIn), y(yIn) {}

    constexpr Vec2 operator+(Vec2 other) const { return {x + other.x, y + other.y}; }
    constexpr Vec2 operator-(Vec2 other) const { return {x - other.x, y - other.y}; }
    constexpr Vec2 operator*(float scale) const { return {x * scale, y * scale}; }
    constexpr Vec2 operator-() const { return {-x, -y}; }
};

constexpr float dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }

} // namespace tq
