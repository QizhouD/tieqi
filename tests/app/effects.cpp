#include "app/Effects.h"

#include <doctest/doctest.h>

#include <algorithm>

using namespace tq;

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
