#include "app/Effects.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>

using namespace tq;

TEST_CASE("near effect counts and sizes interpolate and clamp") {
    for (const int base : {1, 6, 14, 16}) {
        CHECK(nearFxCount(base, -1.f) == base);
        CHECK(nearFxCount(base, 0.f) == base);
        CHECK(nearFxCount(base, 1.f) == 3 * base);
        CHECK(nearFxCount(base, 2.f) == 3 * base);
        for (int step = 1; step <= 4; ++step) {
            const float blend = static_cast<float>(step) * 0.25f;
            CHECK(nearFxCount(base, blend) >= nearFxCount(base, blend - 0.25f));
            CHECK(nearFxSize(blend) <= nearFxSize(blend - 0.25f));
        }
    }
    CHECK(nearFxCount(1, 0.25f) == 2);
    CHECK(nearFxCount(14, 0.5f) == 28);
    CHECK(nearFxSize(-1.f) == doctest::Approx(1.f));
    CHECK(nearFxSize(0.f) == doctest::Approx(1.f));
    CHECK(nearFxSize(0.5f) == doctest::Approx(0.725f));
    CHECK(nearFxSize(1.f) == doctest::Approx(0.45f));
    CHECK(nearFxSize(2.f) == doctest::Approx(0.45f));
}

TEST_CASE("floating text is rasterised at its on-screen device size") {
    for (const float pixelsPerMeter : {0.83f, 1.66f, 3.32f}) {
        for (const float device : {1.f, 1.2f, 2.f}) {
            for (const float pixels : {11.25f, 15.f, 33.f, 44.f}) {
                const FloaterRaster raster = floaterRaster(pixels, pixelsPerMeter, device);
                // Glyphs are drawn at the size they appear on screen, never smaller and magnified.
                CHECK(raster.size == static_cast<unsigned>(std::lround(pixels * device)));
                // One unit of character size, placed in the world, spans the requested logical pixels.
                const float logical = static_cast<float>(raster.size) * raster.worldScale * pixelsPerMeter;
                CHECK(logical == doctest::Approx(pixels));
            }
        }
    }
    // The near view used to rasterise "Three-eyed guns" at 4 pixels and magnify it about four times.
    CHECK(floaterRaster(11.25f, 3.32f, 1.2f).size == 14u);
    CHECK(floaterRaster(0.1f, 1.f, 1.f).size == 1u);
    CHECK(floaterRaster(15.f, 0.f, 0.f).size >= 1u);
}

TEST_CASE("impact clods appear only toward the near view") {
    for (const float ratio : {0.f, 0.5f, 1.f, 2.7f, 100.f}) {
        CHECK(impactClodCount(ratio, 0.f) == 0);
        CHECK(impactClodCount(ratio, -1.f) == 0);
        CHECK(impactClodCount(ratio, 2.f) == impactClodCount(ratio, 1.f));
    }
    CHECK(impactClodCount(0.f, 1.f) == 4);
    CHECK(impactClodCount(100.f, 1.f) == 12);
    CHECK(impactClodCount(0.5f, 1.f) == 6);
    CHECK(impactClodCount(1.f, 0.5f) == 4);
    CHECK(impactClodCount(100.f, 0.5f) == 6);
}

TEST_CASE("impact shake and hit-stop follow the reduced-motion rule") {
    CHECK(impactShakePx(2.f, false) == doctest::Approx(9.f));
    CHECK(impactHitStopSec(2.f, false) == doctest::Approx(0.14f));
    CHECK(impactShakePx(10.f, false) == doctest::Approx(12.f));
    CHECK(impactHitStopSec(10.f, false) == doctest::Approx(0.19f));
    CHECK(impactShakePx(4.f, true) == doctest::Approx(0.f));
    CHECK(impactHitStopSec(4.f, true) == doctest::Approx(0.04f));
    CHECK(impactDustCount(2.7f) == 16);
    CHECK(impactDustCount(0.4f) == 8);
    CHECK(impactDustCount(4.f) == 16);
}

TEST_CASE("effect time stops while paused or in hit-stop") {
    CHECK(effectSeconds(0.5f, 2.f, 8.f, false) == doctest::Approx(1.f));
    CHECK(effectSeconds(0.5f, 2.f, 8.f, true) == doctest::Approx(0.f));

    int ticks = 0;
    float hit = impactHitStopSec(2.f, false);
    const float frame = 1.f / 60.f;
    for (int frameIndex = 0; frameIndex < 1000 && ticks < 3; ++frameIndex) {
        const bool held = hit > 0.f;
        CHECK(effectSeconds(frame, 1.f, 8.f, held) == doctest::Approx(held ? 0.f : frame));
        if (held) {
            hit = std::max(0.f, hit - frame);
        } else {
            ticks += 1;
        }
    }
    CHECK(ticks == 3);
}
