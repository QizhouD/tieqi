#include "app/Orders.h"
#include "app/Viewport.h"
#include "core/rules/Constants.h"
#include "core/sim/Battle.h"
#include "data/Load.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>
#include <sstream>
#include <string>

using namespace tq;

namespace {

GameData loadByeok() { return loadGameData(std::filesystem::path(TQ_SOURCE_DIR) / "data", "byeokjegwan.json"); }

Battle makeBattle(const GameData &data) {
    BattleOptions options;
    options.aiEnabled = false;
    return Battle(data.defs, data.scenario, 1, options);
}

UnitId named(const Battle &battle, const char *id) {
    const std::optional<UnitId> found = battle.sceneUnit(id);
    REQUIRE(found.has_value());
    return *found;
}

std::string text(float value) {
    std::ostringstream out;
    out.setf(std::ios::fixed);
    out.precision(4);
    out << value;
    return out.str();
}

Vec2 logicalAt(Vec2 world, float mapW, float mapH) { return logicalFromWorld(world.x, world.y, mapW, mapH); }

bool contains(const std::vector<UnitId> &ids, UnitId id) { return std::find(ids.begin(), ids.end(), id) != ids.end(); }

} // namespace

TEST_CASE("pointer pixels round-trip onto the map") {
    const float mapW = 880.f;
    const float mapH = 1100.f;
    const Vec2 logical = logicalFromWorld(440.f, 180.f, mapW, mapH);
    const Vec2 world = worldFromLogical(logical.x, logical.y, mapW, mapH);
    CHECK(world.x == doctest::Approx(440.f));
    CHECK(world.y == doctest::Approx(180.f));

    const Vec2 same = logicalFromPixel(610.f, 276.f, 1600.f, 1000.f);
    CHECK(same.x == doctest::Approx(610.f));
    CHECK(same.y == doctest::Approx(276.f));
    const Vec2 centered = logicalFromPixel(1000.f, 500.f, 2000.f, 1000.f);
    CHECK(centered.x == doctest::Approx(800.f));
    CHECK(centered.y == doctest::Approx(500.f));

    CHECK(previewPaint(Verdict::Favored).color == Rgba{0x4f, 0x8a, 0x4b, 255});
    CHECK(previewPaint(Verdict::Breaks).color == previewPaint(Verdict::Pursuit).color);
    CHECK(previewPaint(Verdict::Even).color == Rgba{0xc9, 0x96, 0x2f, 255});
    CHECK(previewPaint(Verdict::Unfavored).color == Rgba{0xb8, 0x40, 0x2c, 255});
    CHECK(previewPaint(Verdict::PikeWall).color == previewPaint(Verdict::Unfavored).color);
}

TEST_CASE("a pointer script selects, boxes, moves, attacks, splits and joins") {
    const GameData data = loadByeok();
    Battle battle = makeBattle(data);
    const float mapW = data.scenario.map.width;
    const float mapH = data.scenario.map.height;
    const float scale = mapCamera(battlefieldRect(), mapW, mapH).scale;
    const UnitId left = named(battle, "jiading_left");
    const UnitId right = named(battle, "jiading_right");
    const UnitId enemy = named(battle, "van_teppo_right");
    const std::size_t before = battle.units().size();
    CHECK(pickUnit(battle, 350.f, 280.f, scale, 0.f) == kNoUnit);
    CHECK(pickUnit(battle, battle.unit(left).pos.x, battle.unit(left).pos.y, scale, 0.f) == left);
    CHECK(pickUnit(battle, battle.unit(left).pos.x + 10.f, battle.unit(left).pos.y, scale, 0.f) == left);
    CHECK(pickUnit(battle, battle.unit(enemy).pos.x, battle.unit(enemy).pos.y, scale, 0.f) == enemy);

    const Vec2 click = logicalAt(battle.unit(left).pos, mapW, mapH);
    const Vec2 drag0 = logicalAt({350.f, 280.f}, mapW, mapH);
    const Vec2 drag1 = logicalAt({530.f, 360.f}, mapW, mapH);
    const Vec2 ground = logicalAt({440.f, 180.f}, mapW, mapH);
    const Vec2 foe = logicalAt(battle.unit(enemy).pos, mapW, mapH);

    std::ostringstream script;
    script << "0 press Space\n";
    script << "0.25 click " << text(click.x) << ' ' << text(click.y) << '\n';
    script << "0.50 drag " << text(drag0.x) << ' ' << text(drag0.y) << ' ' << text(drag1.x) << ' ' << text(drag1.y)
           << '\n';
    script << "0.75 click " << text(ground.x) << ' ' << text(ground.y) << '\n';
    script << "1.00 click " << text(foe.x) << ' ' << text(foe.y) << '\n';
    script << "1.25 press F\n";
    script << "1.75 press G\n";
    std::istringstream in(script.str());
    const ControlPlayback play = playControlScript(battle, in, data.config.timeScale, data.config.playerSpeeds, mapW,
                                                   mapH, data.config.defaultSpeed);

    std::string logged;
    for (const Command &command : play.commands) {
        logged += commandText(command);
        logged += '\n';
    }
    INFO(logged);
    REQUIRE(play.commands.size() == 4);
    CHECK(play.commands[0].type == Command::Type::Move);
    CHECK(play.commands[0].units == std::vector<UnitId>{left, right});
    CHECK(play.commands[0].point.x == doctest::Approx(440.f).epsilon(0.001));
    CHECK(play.commands[0].point.y == doctest::Approx(180.f).epsilon(0.001));
    CHECK(play.commands[1].type == Command::Type::Attack);
    CHECK(play.commands[1].units == std::vector<UnitId>{left, right});
    CHECK(play.commands[1].target == enemy);
    CHECK(play.commands[2].type == Command::Type::Split);
    CHECK(play.commands[2].units == std::vector<UnitId>{left, right});
    CHECK(play.commands[3].type == Command::Type::Join);
    CHECK(play.commands[3].units.size() == 4);
    CHECK(play.commands[3].units == play.state.selected);
    CHECK(contains(play.commands[3].units, left));
    CHECK(contains(play.commands[3].units, right));
    CHECK(play.commands[0].tick > 0);
    CHECK(play.commands[0].tick < play.commands[3].tick);
    CHECK(battle.phase() == 1);
    CHECK(battle.units().size() == before + 2);
    CHECK(play.running);
}

TEST_CASE("orders wait for the first tick and still accept input while paused") {
    const GameData data = loadByeok();
    Battle battle = makeBattle(data);
    const float mapW = data.scenario.map.width;
    const float mapH = data.scenario.map.height;
    const UnitId left = named(battle, "jiading_left");
    const Vec2 click = logicalAt(battle.unit(left).pos, mapW, mapH);

    std::ostringstream early;
    early << "0 click " << text(click.x) << ' ' << text(click.y) << "\n0.50 press H\n";
    std::istringstream earlyIn(early.str());
    const ControlPlayback blocked =
        playControlScript(battle, earlyIn, data.config.timeScale, data.config.playerSpeeds, mapW, mapH, 1.f);
    CHECK(blocked.commands.empty());
    CHECK(blocked.state.selected.empty());
    CHECK(battle.tick() == 0);
    CHECK_FALSE(blocked.running);

    Battle paused = makeBattle(data);
    std::ostringstream later;
    later << "0 press Space\n";
    later << "0.25 click " << text(click.x) << ' ' << text(click.y) << '\n';
    later << "0.40 press Space\n";
    later << "1.00 press H\n";
    later << "1.20 press R\n";
    std::istringstream laterIn(later.str());
    const ControlPlayback play =
        playControlScript(paused, laterIn, data.config.timeScale, data.config.playerSpeeds, mapW, mapH, 1.f);
    std::string logged;
    for (const Command &command : play.commands) {
        logged += commandText(command);
        logged += '\n';
    }
    INFO(logged);
    REQUIRE(play.commands.size() == 2);
    CHECK(play.commands[0].type == Command::Type::Hold);
    CHECK(play.commands[1].type == Command::Type::Retreat);
    CHECK(play.commands[0].units == std::vector<UnitId>{left});
    CHECK_FALSE(play.running);
    CHECK(paused.phase() == 1);
    CHECK(paused.tick() < 10);
}

TEST_CASE("speed, touch confirm, shift, box and selection keys") {
    const GameData data = loadByeok();
    const float mapW = data.scenario.map.width;
    const float mapH = data.scenario.map.height;
    const float scale = mapCamera(battlefieldRect(), mapW, mapH).scale;

    Battle half = makeBattle(data);
    std::istringstream halfScript("0 press 1\n");
    const ControlPlayback slowed =
        playControlScript(half, halfScript, data.config.timeScale, data.config.playerSpeeds, mapW, mapH, 1.f);
    CHECK(slowed.speed == doctest::Approx(0.5f));
    CHECK(half.tick() == 0);

    Battle speeds = makeBattle(data);
    std::istringstream speedScript("0 press 3\n");
    const ControlPlayback speed =
        playControlScript(speeds, speedScript, data.config.timeScale, data.config.playerSpeeds, mapW, mapH, 1.f);
    CHECK(speed.speed == doctest::Approx(2.f));
    CHECK(speed.commands.empty());

    Battle touch = makeBattle(data);
    const UnitId left = named(touch, "jiading_left");
    const UnitId enemy = named(touch, "van_teppo_right");
    const Vec2 mine = logicalAt(touch.unit(left).pos, mapW, mapH);
    const Vec2 foe = logicalAt(touch.unit(enemy).pos, mapW, mapH);
    std::ostringstream touchScript;
    touchScript << "0 press Space\n";
    touchScript << "0.25 click " << text(mine.x) << ' ' << text(mine.y) << '\n';
    touchScript << "0.40 touchclick " << text(foe.x) << ' ' << text(foe.y) << '\n';
    touchScript << "0.50 touchclick " << text(foe.x) << ' ' << text(foe.y) << '\n';
    std::istringstream touchIn(touchScript.str());
    const ControlPlayback touched =
        playControlScript(touch, touchIn, data.config.timeScale, data.config.playerSpeeds, mapW, mapH, 1.f);
    REQUIRE(touched.commands.size() == 1);
    CHECK(touched.commands[0].type == Command::Type::Attack);
    CHECK(touched.commands[0].target == enemy);
    CHECK_FALSE(touched.state.pending.has_value());

    Battle mouse = makeBattle(data);
    std::ostringstream mouseScript;
    mouseScript << "0 press Space\n";
    mouseScript << "0.25 click " << text(mine.x) << ' ' << text(mine.y) << '\n';
    mouseScript << "0.40 click " << text(foe.x) << ' ' << text(foe.y) << '\n';
    std::istringstream mouseIn(mouseScript.str());
    const ControlPlayback clicked =
        playControlScript(mouse, mouseIn, data.config.timeScale, data.config.playerSpeeds, mapW, mapH, 1.f);
    REQUIRE(clicked.commands.size() == 1);
    CHECK(clicked.commands[0].type == Command::Type::Attack);
    CHECK(clicked.state.hover == enemy);

    Battle shift = makeBattle(data);
    const UnitId right = named(shift, "jiading_right");
    const UnitId general = named(shift, "li_rusong");
    const Vec2 generalAt = logicalAt(shift.unit(general).pos, mapW, mapH);
    const Vec2 rightAt = logicalAt(shift.unit(right).pos, mapW, mapH);
    const Vec2 drag0 = logicalAt({350.f, 280.f}, mapW, mapH);
    const Vec2 drag1 = logicalAt({530.f, 360.f}, mapW, mapH);
    const Vec2 nudge = logicalFromWorld(100.f, 100.f, mapW, mapH);
    CHECK(pickUnit(shift, 100.f, 100.f, scale, 0.f) == kNoUnit);
    std::ostringstream shiftScript;
    shiftScript << "0 press Space\n";
    shiftScript << "0.25 click " << text(generalAt.x) << ' ' << text(generalAt.y) << '\n';
    shiftScript << "0.40 shiftclick " << text(rightAt.x) << ' ' << text(rightAt.y) << '\n';
    shiftScript << "0.50 shiftclick " << text(rightAt.x) << ' ' << text(rightAt.y) << '\n';
    shiftScript << "0.70 shiftdrag " << text(drag0.x) << ' ' << text(drag0.y) << ' ' << text(drag1.x) << ' '
                << text(drag1.y) << '\n';
    shiftScript << "0.90 drag " << text(nudge.x) << ' ' << text(nudge.y) << ' ' << text(nudge.x + 3.f) << ' '
                << text(nudge.y) << '\n';
    std::istringstream shiftIn(shiftScript.str());
    Battle shiftBattle = makeBattle(data);
    const ControlPlayback shifted =
        playControlScript(shiftBattle, shiftIn, data.config.timeScale, data.config.playerSpeeds, mapW, mapH, 1.f);
    CHECK(shifted.state.selected == std::vector<UnitId>{general, left, right});
    REQUIRE(shifted.commands.size() == 1);
    CHECK(shifted.commands[0].type == Command::Type::Move);

    Battle all = makeBattle(data);
    std::ostringstream allScript;
    allScript << "0 press Space\n0.25 click " << text(generalAt.x) << ' ' << text(generalAt.y)
              << "\n0.40 press A\n0.50 press Esc\n";
    std::istringstream allIn(allScript.str());
    const ControlPlayback cleared =
        playControlScript(all, allIn, data.config.timeScale, data.config.playerSpeeds, mapW, mapH, 1.f);
    CHECK(cleared.state.selected.empty());
    CHECK_FALSE(cleared.state.pending.has_value());

    Battle every = makeBattle(data);
    std::istringstream everyIn("0 press Space\n0.25 press A\n");
    const ControlPlayback everyone =
        playControlScript(every, everyIn, data.config.timeScale, data.config.playerSpeeds, mapW, mapH, 1.f);
    CHECK(everyone.state.selected.size() == 6);
    CHECK(everyone.state.selected.front() == general);

    OrderState state;
    Event split;
    split.type = Event::Type::Split;
    split.a = left;
    split.b = right;
    state.selected = {left};
    noteEvents(state, shift, std::span<const Event>(&split, 1));
    CHECK(state.selected == std::vector<UnitId>{left, right});
    Event merged;
    merged.type = Event::Type::Merged;
    merged.a = left;
    merged.b = right;
    noteEvents(state, shift, std::span<const Event>(&merged, 1));
    CHECK(state.selected == std::vector<UnitId>{left});
}

TEST_CASE("right click moves on empty ground and attacks an enemy") {
    const GameData data = loadByeok();
    Battle battle = makeBattle(data);
    const float mapW = data.scenario.map.width;
    const float mapH = data.scenario.map.height;
    const UnitId left = named(battle, "jiading_left");
    const UnitId enemy = named(battle, "van_teppo_right");
    const Vec2 mine = logicalAt(battle.unit(left).pos, mapW, mapH);
    const Vec2 foe = logicalAt(battle.unit(enemy).pos, mapW, mapH);
    const Vec2 ground = logicalAt({440.f, 180.f}, mapW, mapH);
    std::ostringstream script;
    script << "0 press Space\n";
    script << "0.25 click " << text(mine.x) << ' ' << text(mine.y) << '\n';
    script << "0.40 rclick " << text(mine.x) << ' ' << text(mine.y) << '\n';
    script << "0.55 rclick " << text(ground.x) << ' ' << text(ground.y) << '\n';
    script << "0.70 rclick " << text(foe.x) << ' ' << text(foe.y) << '\n';
    std::istringstream in(script.str());
    const ControlPlayback play =
        playControlScript(battle, in, data.config.timeScale, data.config.playerSpeeds, mapW, mapH, 1.f);
    REQUIRE(play.commands.size() == 2);
    CHECK(play.commands[0].type == Command::Type::Move);
    CHECK(play.commands[0].point.x == doctest::Approx(440.f).epsilon(0.001));
    CHECK(play.commands[1].type == Command::Type::Attack);
    CHECK(play.commands[1].target == enemy);
    CHECK(play.state.selected == std::vector<UnitId>{left});
}

TEST_CASE("mouse script clicks hover before they press") {
    ScriptEvent click;
    click.action = "click";
    click.arg = "10 20";
    const std::vector<PointerSample> samples = samplesForScript(click);
    REQUIRE(samples.size() == 3);
    CHECK(samples[0].type == PointerSample::Type::Move);
    CHECK(samples[1].type == PointerSample::Type::Down);
    CHECK(samples[2].type == PointerSample::Type::Up);
    CHECK(samples[0].x == doctest::Approx(10.f));
    CHECK(samples[0].y == doctest::Approx(20.f));

    ScriptEvent right;
    right.action = "rclick";
    right.arg = "3 4";
    const std::vector<PointerSample> rightSamples = samplesForScript(right);
    REQUIRE(rightSamples.size() == 3);
    CHECK(rightSamples[0].type == PointerSample::Type::Move);
    CHECK(rightSamples[1].button == kPointerRight);

    ScriptEvent shift;
    shift.action = "shiftclick";
    shift.arg = "5 6";
    const std::vector<PointerSample> shiftSamples = samplesForScript(shift);
    REQUIRE(shiftSamples.size() == 3);
    CHECK(shiftSamples[0].type == PointerSample::Type::Move);
    CHECK(shiftSamples[0].shift);

    ScriptEvent touch;
    touch.action = "touchclick";
    touch.arg = "7 8";
    const std::vector<PointerSample> touchSamples = samplesForScript(touch);
    REQUIRE(touchSamples.size() == 2);
    CHECK(touchSamples[0].type == PointerSample::Type::Down);
    CHECK(touchSamples[0].device == PointerSample::Device::Touch);
}

TEST_CASE("confirm attack orders the pending target") {
    const GameData data = loadByeok();
    Battle battle = makeBattle(data);
    battle.step();
    const UnitId enemy = named(battle, "van_teppo_right");
    OrderState state;
    state.selected = {named(battle, "jiading_left")};
    state.pending = enemy;
    const std::vector<Command> issued = confirmPending(state, battle);
    REQUIRE(issued.size() == 1);
    CHECK(issued[0].type == Command::Type::Attack);
    CHECK(issued[0].target == enemy);
    CHECK_FALSE(state.pending.has_value());

    OrderState idle;
    CHECK(confirmPending(idle, battle).empty());
}
