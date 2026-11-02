#include "app/Formation.h"
#include "core/rules/Constants.h"
#include "core/sim/Battle.h"
#include "data/Load.h"
#include "data/Replay.h"

#include <doctest/doctest.h>

#include <cmath>
#include <filesystem>
#include <vector>

using namespace tq;

namespace {

GameData loadByeok() { return loadGameData(std::filesystem::path(TQ_SOURCE_DIR) / "data", "byeokjegwan.json"); }

// The scripted opening: both cavalry wings charge the vanguard arquebusiers, then the right retainers attack the
// samurai, which gives charges, impacts, melee, breaks, shattering, and routs within a few hundred ticks.
Battle scriptedBattle(const GameData &data) {
    BattleOptions options;
    options.aiEnabled = true;
    return Battle(data.defs, data.scenario, 1, options);
}

void order(Battle &battle, const char *unit, const char *target) {
    Command command;
    command.type = Command::Type::Attack;
    command.units = {*battle.sceneUnit(unit)};
    command.target = *battle.sceneUnit(target);
    battle.submit(command);
}

void script(Battle &battle) {
    if (battle.tick() == 0) {
        order(battle, "liaodong_right", "van_teppo_right");
        order(battle, "jiading_left", "van_teppo_left");
    }
    if (battle.tick() == 300) {
        order(battle, "jiading_right", "van_samurai");
    }
}

std::vector<Soldier> drawn(const FormationLayer &layer, const Battle &battle, float alpha) {
    std::vector<Soldier> soldiers;
    layer.layout(battle, alpha, 0.f, soldiers);
    return soldiers;
}

} // namespace

TEST_CASE("the soldier layer leaves the battle unchanged") {
    const GameData data = loadByeok();
    Battle plain = scriptedBattle(data);
    Battle watched = scriptedBattle(data);
    FormationLayer layer;
    for (int tick = 0; tick < 400; ++tick) {
        script(plain);
        script(watched);
        plain.step();
        watched.step();
        const std::vector<Event> events = watched.drainEvents();
        layer.tick(watched, events);
        drawn(layer, watched, 0.5f);
        REQUIRE(plain.stateHash() == watched.stateHash());
    }
    CHECK(layer.livingSoldiers() > 0);
}

TEST_CASE("soldiers reach the same state at the same tick whatever the frame pacing") {
    const GameData data = loadByeok();
    // Frames per tick: a slow machine runs several ticks per frame, a fast one draws several frames per tick, and a
    // paused game draws without ticking. Only ticks advance the soldiers.
    const auto run = [&](int pattern) {
        Battle battle = scriptedBattle(data);
        FormationLayer layer;
        int frame = 0;
        while (battle.tick() < 360) {
            const int ticks = pattern == 0 ? 1 : pattern == 1 ? (frame % 3 == 0 ? 0 : 1) : 1 + frame % 8;
            for (int n = 0; n < ticks && battle.tick() < 360; ++n) {
                script(battle);
                battle.step();
                const std::vector<Event> events = battle.drainEvents();
                layer.tick(battle, events);
            }
            for (int draw = 0; draw < (pattern == 1 ? 3 : 1); ++draw) {
                drawn(layer, battle, static_cast<float>(draw) * 0.3f);
            }
            frame += 1;
        }
        return layer.hash();
    };
    const std::uint64_t steady = run(0);
    CHECK(run(1) == steady);
    CHECK(run(2) == steady);
}

TEST_CASE("soldiers move continuously, fall where the fighting is, and stay near the rules box") {
    const GameData data = loadByeok();
    Battle battle = scriptedBattle(data);
    FormationLayer layer;
    const float margin = layer.tuning().clampOutside;
    float biggestJump = 0.f;
    std::uint32_t jumpTick = 0;
    UnitId jumpUnit = kNoUnit;
    for (int tick = 0; tick < 700; ++tick) {
        script(battle);
        battle.step();
        const std::vector<Event> events = battle.drainEvents();
        layer.tick(battle, events);
        const std::vector<Soldier> before = drawn(layer, battle, 0.f);
        const std::vector<Soldier> after = drawn(layer, battle, 1.f);
        REQUIRE(before.size() == after.size());
        for (std::size_t i = 0; i < after.size(); ++i) {
            const Vec2 step = after[i].pos - before[i].pos;
            if (std::hypot(step.x, step.y) > biggestJump) {
                biggestJump = std::hypot(step.x, step.y);
                jumpTick = battle.tick();
                jumpUnit = after[i].unit;
            }
            const Unit &unit = battle.unit(after[i].unit);
            if (!unit.contacts.empty() || unit.state != UnitState::Steady || !layer.settled(unit.id)) {
                continue;
            }
            const Vec2 d = after[i].pos - unit.pos;
            const float along = d.x * std::cos(unit.facing) + d.y * std::sin(unit.facing);
            const float across = -d.x * std::sin(unit.facing) + d.y * std::cos(unit.facing);
            CHECK(std::abs(along) <= unit.depth * (0.5f + margin) + 0.01f);
            CHECK(std::abs(across) <= unit.frontage * 0.5f + unit.depth * margin + 0.01f);
        }
    }
    // Nobody teleports, not even when a unit shatters and its men are dealt to the remnants: no soldier moves
    // faster than a running horse, plus a little room for the spacing pushes.
    INFO("tick " << jumpTick << " unit " << jumpUnit);
    CHECK(biggestJump < layer.tuning().maxSpeedCavalry * kTickSeconds * 1.6f);
    CHECK_FALSE(layer.bodies().empty());
    // Bodies fade to half transparency and stay.
    const FallenSoldier &oldest = layer.bodies().front();
    CHECK(oldest.age > layer.tuning().bodyFadeSec);
    CHECK(layer.bodyAlpha(oldest) == doctest::Approx(layer.tuning().bodyMinAlpha));
}

TEST_CASE("clicking a soldier picks its unit") {
    const GameData data = loadByeok();
    Battle battle = scriptedBattle(data);
    FormationLayer layer;
    for (int tick = 0; tick < 20; ++tick) {
        script(battle);
        battle.step();
        const std::vector<Event> events = battle.drainEvents();
        layer.tick(battle, events);
    }
    const std::vector<Soldier> soldiers = drawn(layer, battle, 1.f);
    REQUIRE_FALSE(soldiers.empty());
    for (std::size_t i = 0; i < soldiers.size(); i += 7) {
        CHECK(layer.pick(battle, soldiers[i].pos, 0.5f, 1.f) == soldiers[i].unit);
    }
    CHECK(layer.pick(battle, {-500.f, -500.f}, 2.5f, 1.f) == kNoUnit);
}

TEST_CASE("looseness rises with disorder") {
    const GameData data = loadByeok();
    const Battle battle = scriptedBattle(data);
    const FormationTuning tuning;
    Unit unit = battle.unit(*battle.sceneUnit("liaodong_center"));
    const float steady = unitLooseness(battle, unit, tuning);
    unit.state = UnitState::Wavering;
    const float wavering = unitLooseness(battle, unit, tuning);
    unit.state = UnitState::Routing;
    CHECK(steady < wavering);
    CHECK(unitLooseness(battle, unit, tuning) == doctest::Approx(1.f));
}

