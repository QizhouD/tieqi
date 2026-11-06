#include "app/Crests.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace tq {
namespace {

constexpr float kPi = std::numbers::pi_v<float>;

void triangle(sf::VertexArray &tris, sf::Vector2f a, sf::Vector2f b, sf::Vector2f c, sf::Color ink) {
    tris.append({a, ink});
    tris.append({b, ink});
    tris.append({c, ink});
}

void disc(sf::VertexArray &tris, sf::Vector2f centre, float radius, sf::Color ink) {
    constexpr int kSteps = 28;
    for (int i = 0; i < kSteps; ++i) {
        const float a0 = 2.f * kPi * static_cast<float>(i) / kSteps;
        const float a1 = 2.f * kPi * static_cast<float>(i + 1) / kSteps;
        triangle(tris, centre, centre + sf::Vector2f{std::cos(a0), std::sin(a0)} * radius,
                 centre + sf::Vector2f{std::cos(a1), std::sin(a1)} * radius, ink);
    }
}

void ring(sf::VertexArray &tris, sf::Vector2f centre, float radius, float width, sf::Color ink) {
    constexpr int kSteps = 40;
    const float inner = std::max(0.f, radius - width);
    for (int i = 0; i < kSteps; ++i) {
        const float a0 = 2.f * kPi * static_cast<float>(i) / kSteps;
        const float a1 = 2.f * kPi * static_cast<float>(i + 1) / kSteps;
        const sf::Vector2f o0{std::cos(a0), std::sin(a0)};
        const sf::Vector2f o1{std::cos(a1), std::sin(a1)};
        triangle(tris, centre + o0 * inner, centre + o0 * radius, centre + o1 * radius, ink);
        triangle(tris, centre + o0 * inner, centre + o1 * radius, centre + o1 * inner, ink);
    }
}

void bar(sf::VertexArray &tris, sf::Vector2f a, sf::Vector2f b, float width, sf::Color ink) {
    sf::Vector2f d = b - a;
    const float length = std::hypot(d.x, d.y);
    if (length < 1e-5f) {
        return;
    }
    d /= length;
    const sf::Vector2f n{-d.y * width * 0.5f, d.x * width * 0.5f};
    triangle(tris, a + n, a - n, b - n, ink);
    triangle(tris, a + n, b - n, b + n, ink);
}

sf::Vector2f polar(sf::Vector2f centre, float radius, float angle) {
    return centre + sf::Vector2f{std::cos(angle), std::sin(angle)} * radius;
}

} // namespace

void appendMitsudomoe(sf::VertexArray &tris, sf::Vector2f centre, float radius, sf::Color ink) {
    // Each tomoe: a round head set inside the circle and a tail that spirals out to the rim, thinning to a point.
    // Screen y points down, so a negative angle step sweeps counterclockwise as seen.
    const float headRadius = radius * 0.3f;
    const float headOrbit = radius * 0.42f;
    constexpr int kSteps = 28;
    constexpr float kSweep = kPi * 0.85f;
    for (int k = 0; k < 3; ++k) {
        const float start = -kPi * 0.5f + static_cast<float>(k) * 2.f * kPi / 3.f;
        disc(tris, polar(centre, headOrbit, start), headRadius, ink);
        const auto outerAt = [&](float t) { return headOrbit + headRadius + (radius - headOrbit - headRadius) * t; };
        const auto widthAt = [&](float t) { return headRadius * 2.f * std::pow(1.f - t, 0.8f); };
        for (int i = 0; i < kSteps; ++i) {
            const float t0 = static_cast<float>(i) / kSteps;
            const float t1 = static_cast<float>(i + 1) / kSteps;
            const float a0 = start - kSweep * t0;
            const float a1 = start - kSweep * t1;
            const float o0 = outerAt(t0);
            const float o1 = outerAt(t1);
            triangle(tris, polar(centre, o0, a0), polar(centre, o0 - widthAt(t0), a0),
                     polar(centre, o1 - widthAt(t1), a1), ink);
            triangle(tris, polar(centre, o0, a0), polar(centre, o1 - widthAt(t1), a1), polar(centre, o1, a1), ink);
        }
    }
}

void appendGionMamori(sf::VertexArray &tris, sf::Vector2f centre, float radius, sf::Color ink) {
    // Two amulet scrolls crossed as an X, tied with a knot where they cross, each end finished with a small cross.
    const float rod = radius * 0.2f;
    for (float angle : {kPi * 0.25f, kPi * 0.75f}) {
        const sf::Vector2f dir{std::cos(angle), std::sin(angle)};
        const sf::Vector2f side{-dir.y, dir.x};
        const sf::Vector2f a = centre - dir * (radius * 0.72f);
        const sf::Vector2f b = centre + dir * (radius * 0.72f);
        bar(tris, a, b, rod, ink);
        for (const sf::Vector2f end : {a, b}) {
            const sf::Vector2f out = end == a ? -dir : dir;
            const sf::Vector2f tip = end + out * (radius * 0.2f);
            bar(tris, end, tip, rod * 0.5f, ink);
            bar(tris, tip - side * (radius * 0.12f), tip + side * (radius * 0.12f), rod * 0.45f, ink);
        }
    }
    // The knot: a ring with two short ribbon tails.
    ring(tris, centre, radius * 0.24f, radius * 0.1f, ink);
    bar(tris, centre + sf::Vector2f{0.f, radius * 0.2f}, centre + sf::Vector2f{-radius * 0.14f, radius * 0.48f},
        rod * 0.5f, ink);
    bar(tris, centre + sf::Vector2f{0.f, radius * 0.2f}, centre + sf::Vector2f{radius * 0.14f, radius * 0.48f},
        rod * 0.5f, ink);
}

} // namespace tq
