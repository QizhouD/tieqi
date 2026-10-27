#include "core/math/Geometry.h"

#include <doctest/doctest.h>

#include <numbers>

using namespace tq;

namespace {

constexpr float kNorth = -std::numbers::pi_v<float> / 2.f;
constexpr float kTeppoDepth = 22.f;
constexpr float kTeppoFrontage = 66.f;

Obb boxAt(Vec2 center, float depth, float frontage) { return obb(center, 0.f, depth, frontage, 0.f); }

} // namespace

TEST_CASE("angDiff normalizes into (-pi, pi]") {
    const float pi = std::numbers::pi_v<float>;
    CHECK(angDiff(0.f, 0.f) == doctest::Approx(0.f));
    CHECK(angDiff(1.f, 0.f) == doctest::Approx(1.f));
    CHECK(angDiff(0.f, 1.f) == doctest::Approx(-1.f));
    CHECK(angDiff(pi, 0.f) == doctest::Approx(pi));
    CHECK(angDiff(0.f, pi) == doctest::Approx(pi));
}

TEST_CASE("turnToward steps at most the given amount") {
    CHECK(turnToward(0.f, 0.1f, 0.2f) == doctest::Approx(0.1f));
    CHECK(turnToward(0.f, 1.f, 0.2f) == doctest::Approx(0.2f));
    CHECK(turnToward(0.f, -1.f, 0.25f) == doctest::Approx(-0.25f));
}

TEST_CASE("sideOf teppo facing north") {
    const Vec2 defender{500.f, 500.f};
    const auto at = [&](Vec2 attacker) { return sideOf(defender, kNorth, kTeppoDepth, kTeppoFrontage, attacker); };
    CHECK(at({500.f, 470.f}) == ContactSide::Front);
    CHECK(at({540.f, 500.f}) == ContactSide::Flank);
    CHECK(at({500.f, 530.f}) == ContactSide::Rear);
    CHECK(at({530.f, 474.f}) == ContactSide::Front);
    CHECK(at({540.f, 470.f}) == ContactSide::Front);
    CHECK(at({530.f, 526.f}) == ContactSide::Rear);
}

TEST_CASE("sat reports overlap and points from A toward B") {
    const Obb a = boxAt({0.f, 0.f}, 20.f, 40.f);
    const auto hit = sat(a, boxAt({15.f, 0.f}, 20.f, 40.f));
    REQUIRE(hit.has_value());
    CHECK(hit->overlap == doctest::Approx(5.f));
    CHECK(hit->axis.x == doctest::Approx(1.f));
    CHECK(hit->axis.y == doctest::Approx(0.f));
}

TEST_CASE("sat misses separated and touching boxes") {
    const Obb a = boxAt({0.f, 0.f}, 20.f, 40.f);
    CHECK_FALSE(sat(a, boxAt({40.f, 0.f}, 20.f, 40.f)).has_value());
    CHECK_FALSE(sat(a, boxAt({20.f, 0.f}, 20.f, 40.f)).has_value());
}

TEST_CASE("sat flips the axis so it points toward B") {
    const Obb a = boxAt({0.f, 0.f}, 20.f, 40.f);
    const auto hit = sat(a, boxAt({-15.f, 0.f}, 20.f, 40.f));
    REQUIRE(hit.has_value());
    CHECK(hit->overlap == doctest::Approx(5.f));
    CHECK(hit->axis.x == doctest::Approx(-1.f));
    CHECK(hit->axis.y == doctest::Approx(0.f));
}

TEST_CASE("sat keeps the first axis when overlaps tie") {
    const Obb a = boxAt({0.f, 0.f}, 20.f, 20.f);
    const auto hit = sat(a, boxAt({5.f, 5.f}, 20.f, 20.f));
    REQUIRE(hit.has_value());
    CHECK(hit->overlap == doctest::Approx(15.f));
    CHECK(hit->axis.x == doctest::Approx(1.f));
    CHECK(hit->axis.y == doctest::Approx(0.f));
}
