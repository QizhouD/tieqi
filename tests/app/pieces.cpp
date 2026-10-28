#include "app/Pieces.h"
#include "../../tools/headless/Policy.h"
#include "core/rules/Constants.h"
#include "core/sim/Battle.h"
#include "data/Load.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <filesystem>

using namespace tq;

TEST_CASE("piece interpolation, blocks and chits follow the prototype") {
    Unit unit;
    unit.prevPos = {0.f, 10.f};
    unit.pos = {10.f, 10.f};
    unit.prevFacing = 0.1f;
    unit.facing = 6.2f;
    const float delta = angDiff(unit.facing, unit.prevFacing);
    CHECK(std::abs(delta) < 1.f);
    const Pose mid = interpolatedPose(unit, 0.5f);
    CHECK(mid.pos.x == doctest::Approx(5.f));
    CHECK(mid.pos.y == doctest::Approx(10.f));
    CHECK(mid.facing == doctest::Approx(unit.prevFacing + delta * 0.5f));
    CHECK(std::abs(angDiff(interpolatedPose(unit, 1.f).facing, unit.facing)) < 1e-4f);
    CHECK(interpolatedPose(unit, 0.f).pos.x == doctest::Approx(unit.prevPos.x));

    CHECK(blockColumns(66.f) == 11);
    CHECK(blockRows(22.f) == 3);
    CHECK(blockDots(66.f, 22.f, 1000.f, 1000.f) == 33);
    CHECK(blockDots(66.f, 22.f, 500.f, 1000.f) == 17);
    CHECK(blockDots(66.f, 22.f, 1.f, 0.f) == 0);
    const Vec2 dot = blockDotLocal(22.f, 66.f, 0, 11, {});
    CHECK(dot.x == doctest::Approx(6.5f));
    CHECK(dot.y == doctest::Approx(-31.3f));
    const Vec2 jitter = routingJitter(1, 0);
    CHECK(jitter.x == doctest::Approx(-4.8f));
    CHECK(jitter.y == doctest::Approx(-3.2f));
    CHECK(chargeLineLength(0, -2, 0.f) == doctest::Approx(2.f));
    CHECK(chargeLineLength(0, 0, 5.f) == doctest::Approx(10.f + 4.8f));

    CHECK(chitPixels(0.84f, 1000.f, 1000.f) == doctest::Approx(34.f));
    CHECK(chitPixels(0.4f, 1000.f, 1000.f) == doctest::Approx(22.f));
    CHECK(chitPixels(1.f, 250.f, 1000.f) == doctest::Approx(34.f * (0.58f + 0.42f * 0.5f)));

    const ChitFace closed = chitFace(UnitState::Broken, kBrokenSec);
    CHECK(closed.scaleX == doctest::Approx(1.f));
    CHECK_FALSE(closed.back);
    const float snapshot = kBrokenSec * 0.5f;
    CHECK(presentedBrokenSec(snapshot, 0.f) == doctest::Approx(snapshot + kTickSeconds));
    CHECK(presentedBrokenSec(snapshot, 0.5f) == doctest::Approx(snapshot + kTickSeconds * 0.5f));
    CHECK(presentedBrokenSec(snapshot, 1.f) == doctest::Approx(snapshot));
    const ChitFace lagged = chitFace(UnitState::Broken, presentedBrokenSec(snapshot, 0.f));
    const ChitFace halfway = chitFace(UnitState::Broken, presentedBrokenSec(snapshot, 0.5f));
    const ChitFace settled = chitFace(UnitState::Broken, presentedBrokenSec(snapshot, 1.f));
    CHECK(lagged.scaleX > halfway.scaleX);
    CHECK(halfway.scaleX > settled.scaleX);
    CHECK_FALSE(lagged.back);
    CHECK(chitFace(UnitState::Broken, presentedBrokenSec(kBrokenSec, 0.f)).scaleX == doctest::Approx(1.f));
    const ChitFace turned = chitFace(UnitState::Broken, kBrokenSec * 0.25f);
    CHECK(turned.back);
    CHECK(turned.scaleX == doctest::Approx(std::sqrt(0.5f)).epsilon(0.02));
    CHECK(chitFace(UnitState::Broken, 0.f).back);
    CHECK(chitFace(UnitState::Routing, 0.f).back);
    CHECK_FALSE(chitFace(UnitState::Steady, 0.f).back);
    CHECK(chitWobble(UnitState::Steady, 1.f, 3) == doctest::Approx(0.f));
    CHECK(chitWobble(UnitState::Wavering, 1.f, 3) == doctest::Approx(std::sin(8.f + 3.f) * 0.06f));
    CHECK(chitWobble(UnitState::Routing, 0.5f, 2) == doctest::Approx(std::sin(0.5f * 13.f + 4.f) * 0.14f));

    const ChitColors ming = chitColors(Side::Ming, false, false);
    CHECK(ming.face == Rgba{0xa8, 0x38, 0x2a});
    CHECK(ming.edge == Rgba{0xd9, 0xb4, 0x66});
    CHECK(ming.glyph == Rgba{0xf4, 0xdb, 0xa4});
    const ChitColors general = chitColors(Side::Ming, true, false);
    CHECK(general.face == Rgba{0xd9, 0xb4, 0x66});
    CHECK(general.glyph == Rgba{0xa8, 0x38, 0x2a});
    const ChitColors jp = chitColors(Side::Jp, false, false);
    CHECK(jp.face == Rgba{0x2f, 0x3a, 0x56});
    CHECK(jp.edge == Rgba{0xaa, 0xb4, 0xc9});
    CHECK(jp.glyph == Rgba{0xef, 0xe5, 0xcb});
    CHECK(chitColors(Side::Jp, true, true).face == Rgba{0xbd, 0xb0, 0x9a});
    CHECK(moraleColor(UnitState::Steady) == Rgba{0x4f, 0x8a, 0x4b});
    CHECK(moraleColor(UnitState::Wavering) == Rgba{0xc9, 0x96, 0x2f});
    CHECK(moraleColor(UnitState::Broken) == Rgba{0xb8, 0x40, 0x2c});
    CHECK(moraleColor(UnitState::Routing) == Rgba{0x85, 0x77, 0x69});

    CHECK(recoverFraction(48.f) == doctest::Approx(1.f));
    CHECK(recoverFraction(49.f) == doctest::Approx(49.f / 72.f));
    CHECK(recoverFraction(0.f) == doctest::Approx(0.f));
    CHECK(reloadFraction(24.f, 48.f) == doctest::Approx(0.5f));
    CHECK(timerArc(UnitState::Steady, 10.f, 5.f, 80.f) == ArcKind::Recover);
    CHECK(timerArc(UnitState::Steady, 0.f, 5.f, 80.f) == ArcKind::Reload);
    CHECK(timerArc(UnitState::Steady, 0.f, 5.f, 0.f) == ArcKind::None);
    CHECK(timerArc(UnitState::Routing, 10.f, 5.f, 80.f) == ArcKind::None);
    CHECK(menFraction(250.f, 1000.f) == doctest::Approx(0.25f));
    CHECK(moraleFraction(125.f) == doctest::Approx(1.f));
}

TEST_CASE("a scripted replay's broken unit flips with the battle event") {
    const GameData data = loadGameData(std::filesystem::path(TQ_SOURCE_DIR) / "data", "byeokjegwan.json");
    Battle battle(data.defs, data.scenario, 1);
    for (const Unit &unit : battle.units()) {
        if (unit.state == UnitState::Gone) {
            continue;
        }
        const int full = blockColumns(unit.frontage) * blockRows(unit.depth);
        CHECK(blockDots(unit.frontage, unit.depth, unit.men, unit.maxMen) == full);
        const Pose pose = interpolatedPose(unit, 0.35f);
        CHECK(pose.pos.x == doctest::Approx(unit.pos.x));
        CHECK(pose.facing == doctest::Approx(unit.facing));
    }

    PolicyState state;
    std::vector<ReplayCommand> recorded;
    UnitId broken = kNoUnit;
    int stepsAfter = -1;
    for (int n = 0; n < 4000 && !battle.isOver() && stepsAfter < 12; ++n) {
        applyPolicy(battle, PolicyKind::Scripted, state, recorded);
        battle.step();
        if (state.phase3At == 0 && battle.phase() >= 3) {
            state.phase3At = battle.tick();
        }
        const std::vector<Event> events = battle.drainEvents();
        if (stepsAfter < 0) {
            for (const Event &event : events) {
                if (event.type != Event::Type::Broken) {
                    continue;
                }
                broken = event.a;
                stepsAfter = 0;
                const Unit &unit = battle.unit(broken);
                const UnitType &type = battle.typeOf(unit);
                INFO("first break tick " << battle.tick() << " unit " << broken);
                CHECK(unit.state == UnitState::Broken);
                CHECK(unit.brokenSec == doctest::Approx(kBrokenSec));
                const ChitFace face = chitFace(unit.state, unit.brokenSec);
                CHECK_FALSE(face.back);
                CHECK(face.scaleX == doctest::Approx(1.f));
                const ChitFace drawn = chitFace(unit.state, presentedBrokenSec(unit.brokenSec, 0.f));
                CHECK_FALSE(drawn.back);
                CHECK(drawn.scaleX == doctest::Approx(1.f));
                CHECK(timerArc(unit.state, unit.recoverSec, unit.reloadSec, type.range) == ArcKind::None);
                CHECK(menFraction(unit.men, unit.maxMen) == doctest::Approx(unit.men / unit.maxMen));
                CHECK(moraleFraction(unit.morale) == doctest::Approx(std::clamp(unit.morale / 100.f, 0.f, 1.f)));
                if (unit.maxMen < type.men) {
                    CHECK(chitPixels(0.84f, unit.maxMen, type.men) < chitPixels(0.84f, type.men, type.men));
                }
                break;
            }
        } else {
            stepsAfter += 1;
        }
    }

    REQUIRE(broken != kNoUnit);
    const Unit &later = battle.unit(broken);
    INFO("after flip tick " << battle.tick() << " state " << static_cast<int>(later.state));
    CHECK(later.state == UnitState::Routing);
    const ChitFace routed = chitFace(later.state, later.brokenSec);
    CHECK(routed.back);
    CHECK(routed.scaleX == doctest::Approx(1.f));
    CHECK(chitColors(later.side, battle.typeOf(later).general, true).glyph == Rgba{0x2a, 0x22, 0x1b});
}
