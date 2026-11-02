#include "app/CameraControl.h"
#include "app/Orders.h"
#include "app/Viewport.h"
#include "core/sim/Battle.h"
#include "data/Load.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <sstream>

using namespace tq;

namespace {

constexpr float kMapW = 880.f;
constexpr float kMapH = 1100.f;

GameData loadByeok() { return loadGameData(std::filesystem::path(TQ_SOURCE_DIR) / "data", "byeokjegwan.json"); }

PointerSample mouse(PointerSample::Type type, int button, Vec2 at) {
    PointerSample sample;
    sample.type = type;
    sample.button = button;
    sample.x = at.x;
    sample.y = at.y;
    return sample;
}

PointerSample wheel(Vec2 at, float notches) {
    PointerSample sample = mouse(PointerSample::Type::Wheel, kPointerLeft, at);
    sample.wheel = notches;
    return sample;
}

} // namespace

TEST_CASE("zoom 1 places the map exactly as before") {
    const ScreenRect field = battlefieldRect();
    const MapCamera plain = mapCamera(field, kMapW, kMapH);
    Zoom panned;
    panned.focus = Vec2{100.f, 100.f};
    const MapCamera ignored = mapCamera(field, kMapW, kMapH, panned);
    CHECK(ignored.scale == plain.scale);
    CHECK(ignored.originX == plain.originX);
    CHECK(ignored.originY == plain.originY);
}

TEST_CASE("wheel zoom keeps the world point under the cursor and stays within 1 to 4") {
    // Near the centre, so the zoomed view is not pushed back by the map edge.
    const Vec2 cursor{600.f, 500.f};
    const Vec2 before = worldFromLogical(cursor.x, cursor.y, kMapW, kMapH);
    const Zoom twice = zoomAround({}, 2.f, cursor.x, cursor.y, kMapW, kMapH);
    CHECK(twice.level == doctest::Approx(2.f));
    const Vec2 after = worldFromLogical(cursor.x, cursor.y, kMapW, kMapH, twice);
    CHECK(after.x == doctest::Approx(before.x).epsilon(1e-4));
    CHECK(after.y == doctest::Approx(before.y).epsilon(1e-4));

    CHECK(zoomAround(twice, 10.f, cursor.x, cursor.y, kMapW, kMapH).level == doctest::Approx(kMaxZoom));
    const Zoom out = zoomAround(twice, 0.1f, cursor.x, cursor.y, kMapW, kMapH);
    CHECK(out.level == doctest::Approx(kMinZoom));
    CHECK_FALSE(out.focus.has_value());
}

TEST_CASE("panning never shows anything beyond the map edge") {
    const ScreenRect field = battlefieldRect();
    Zoom zoom = zoomAround({}, 4.f, field.x + field.w * 0.5f, field.y + field.h * 0.5f, kMapW, kMapH);
    zoom = panBy(zoom, 100000.f, 100000.f, kMapW, kMapH);
    const Vec2 topLeft = worldFromLogical(field.x, field.y, kMapW, kMapH, zoom);
    CHECK(topLeft.x == doctest::Approx(0.f).epsilon(1e-3));
    CHECK(topLeft.y == doctest::Approx(0.f).epsilon(1e-3));
    zoom = panBy(zoom, -100000.f, -100000.f, kMapW, kMapH);
    const Vec2 bottomRight = worldFromLogical(field.x + field.w, field.y + field.h, kMapW, kMapH, zoom);
    CHECK(bottomRight.x == doctest::Approx(kMapW).epsilon(1e-3));
    CHECK(bottomRight.y == doctest::Approx(kMapH).epsilon(1e-3));
}

TEST_CASE("right click orders at zoom 1, right drag pans only when zoomed in") {
    CameraControl camera;
    const Vec2 at{625.f, 521.f};
    CHECK_FALSE(camera.handle(mouse(PointerSample::Type::Down, kPointerRight, at), kMapW, kMapH).consumed);
    CHECK_FALSE(camera.handle(mouse(PointerSample::Type::Move, kPointerLeft, {685.f, 521.f}), kMapW, kMapH).consumed);
    CHECK_FALSE(camera.handle(mouse(PointerSample::Type::Up, kPointerRight, {685.f, 521.f}), kMapW, kMapH).consumed);

    CHECK(camera.handle(wheel(at, 3.f), kMapW, kMapH).consumed);
    CHECK(camera.zoom().level > 1.9f);
    const Vec2 before = worldFromLogical(at.x, at.y, kMapW, kMapH, camera.zoom());
    CHECK_FALSE(camera.handle(mouse(PointerSample::Type::Down, kPointerRight, at), kMapW, kMapH).consumed);
    CHECK_FALSE(camera.handle(mouse(PointerSample::Type::Move, kPointerLeft, {628.f, 521.f}), kMapW, kMapH).consumed);
    const CameraControl::Result started =
        camera.handle(mouse(PointerSample::Type::Move, kPointerLeft, {665.f, 551.f}), kMapW, kMapH);
    CHECK(started.consumed);
    CHECK(started.startedPan);
    CHECK(camera.handle(mouse(PointerSample::Type::Up, kPointerRight, {665.f, 551.f}), kMapW, kMapH).consumed);
    // The ground under the press follows the pointer.
    const Vec2 grabbed = worldFromLogical(665.f, 551.f, kMapW, kMapH, camera.zoom());
    CHECK(grabbed.x == doctest::Approx(before.x).epsilon(1e-3));
    CHECK(grabbed.y == doctest::Approx(before.y).epsilon(1e-3));

    CHECK(camera.handle(mouse(PointerSample::Type::Down, kPointerMiddle, at), kMapW, kMapH).consumed);
    CHECK(camera.handle(mouse(PointerSample::Type::Move, kPointerLeft, {605.f, 501.f}), kMapW, kMapH).consumed);
    CHECK(camera.handle(mouse(PointerSample::Type::Up, kPointerMiddle, {605.f, 501.f}), kMapW, kMapH).consumed);

    camera.reset();
    CHECK(camera.zoom().level == doctest::Approx(1.f));
    CHECK_FALSE(camera.zoom().focus.has_value());
}

TEST_CASE("selection and orders stay accurate when zoomed in") {
    const GameData data = loadByeok();
    BattleOptions options;
    options.aiEnabled = false;
    Battle battle(data.defs, data.scenario, 1, options);
    battle.step();
    const UnitId left = *battle.sceneUnit("jiading_left");
    const UnitId enemy = *battle.sceneUnit("van_teppo_right");
    const Zoom zoom = zoomAround({}, 3.f, 600.f, 400.f, kMapW, kMapH);

    OrderState state;
    const Vec2 unitAt = logicalFromWorld(battle.unit(left).pos.x, battle.unit(left).pos.y, kMapW, kMapH, zoom);
    applyPointer(state, battle, mouse(PointerSample::Type::Down, kPointerLeft, unitAt), kMapW, kMapH, 0.f, zoom);
    applyPointer(state, battle, mouse(PointerSample::Type::Up, kPointerLeft, unitAt), kMapW, kMapH, 0.f, zoom);
    REQUIRE(state.selected == std::vector<UnitId>{left});

    const Vec2 ground = logicalFromWorld(420.f, 300.f, kMapW, kMapH, zoom);
    applyPointer(state, battle, mouse(PointerSample::Type::Down, kPointerLeft, ground), kMapW, kMapH, 0.f, zoom);
    const std::vector<Command> moved =
        applyPointer(state, battle, mouse(PointerSample::Type::Up, kPointerLeft, ground), kMapW, kMapH, 0.f, zoom);
    REQUIRE(moved.size() == 1);
    CHECK(moved[0].point.x == doctest::Approx(420.f).epsilon(1e-3));
    CHECK(moved[0].point.y == doctest::Approx(300.f).epsilon(1e-3));

    const Zoom far = zoomAround({}, 2.f, 600.f, 800.f, kMapW, kMapH);
    const Vec2 foe = logicalFromWorld(battle.unit(enemy).pos.x, battle.unit(enemy).pos.y, kMapW, kMapH, far);
    applyPointer(state, battle, mouse(PointerSample::Type::Down, kPointerRight, foe), kMapW, kMapH, 0.f, far);
    const std::vector<Command> attacked =
        applyPointer(state, battle, mouse(PointerSample::Type::Up, kPointerRight, foe), kMapW, kMapH, 0.f, far);
    REQUIRE(attacked.size() == 1);
    CHECK(attacked[0].type == Command::Type::Attack);
    CHECK(attacked[0].target == enemy);
}

TEST_CASE("scripts can wheel and pan") {
    std::istringstream in("0 wheel 500 400 2\n0.5 mdrag 500 400 450 350\n1 rdrag 500 400 520 420\n");
    const std::vector<ScriptEvent> events = parseScript(in);
    REQUIRE(events.size() == 3);
    for (const ScriptEvent &event : events) {
        CHECK(isPointerAction(event.action));
    }
    const std::vector<PointerSample> wheeled = samplesForScript(events[0]);
    REQUIRE(wheeled.size() == 2);
    CHECK(wheeled[1].type == PointerSample::Type::Wheel);
    CHECK(wheeled[1].wheel == doctest::Approx(2.f));
    const std::vector<PointerSample> panned = samplesForScript(events[1]);
    REQUIRE(panned.size() >= 4);
    CHECK(panned[1].type == PointerSample::Type::Down);
    CHECK(panned[1].button == kPointerMiddle);
    CHECK(panned.back().type == PointerSample::Type::Up);
    CHECK(samplesForScript(events[2])[1].button == kPointerRight);
}

TEST_CASE("the near view starts at 2x and still picks units") {
    CHECK_FALSE(nearView(zoomAround({}, 1.9f, 625.f, 521.f, kMapW, kMapH)));
    CHECK(nearView(zoomAround({}, 2.f, 625.f, 521.f, kMapW, kMapH)));

    const GameData data = loadByeok();
    BattleOptions options;
    options.aiEnabled = false;
    Battle battle(data.defs, data.scenario, 1, options);
    battle.step();
    const UnitId li = *battle.sceneUnit("li_rusong");
    const Zoom zoom = zoomAround({}, 3.f, 625.f, 300.f, kMapW, kMapH);
    const float scale = mapCamera(battlefieldRect(), kMapW, kMapH, zoom).scale;
    const Vec2 at = battle.unit(li).pos;
    CHECK(pickUnit(battle, at.x, at.y, scale, 0.f, kNearChitScale) == li);

    OrderState state;
    const Vec2 click = logicalFromWorld(at.x, at.y, kMapW, kMapH, zoom);
    applyPointer(state, battle, mouse(PointerSample::Type::Down, kPointerLeft, click), kMapW, kMapH, 0.f, zoom);
    applyPointer(state, battle, mouse(PointerSample::Type::Up, kPointerLeft, click), kMapW, kMapH, 0.f, zoom);
    CHECK(state.selected == std::vector<UnitId>{li});
}
