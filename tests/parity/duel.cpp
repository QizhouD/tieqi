#include "../BattleAccess.h"
#include "core/sim/Battle.h"
#include "data/Load.h"

#include <doctest/doctest.h>

#include <cmath>
#include <filesystem>
#include <numbers>
#include <string>
#include <vector>

using namespace tq;

namespace {

std::filesystem::path sourceRoot() { return TQ_SOURCE_DIR; }

float radians(float degrees) { return degrees * std::numbers::pi_v<float> / 180.f; }

bool nearTick(std::uint32_t actual, int expected, int tolerance) {
    return std::abs(static_cast<int>(actual) - expected) <= tolerance;
}

bool nearRatio(float actual, float expected) { return std::fabs(actual - expected) <= 0.05f; }

bool nearMen(float actual, float expected) { return std::fabs(actual - expected) <= expected * 0.03f; }

struct Piece {
    std::string id;
    std::string type;
    Vec2 pos;
    float facingDeg = 0.f;
};

const GameDefs &defs() {
    static const GameData data = loadGameData(sourceRoot() / "data", "byeokjegwan.json");
    return data.defs;
}

const StringTable &strings() {
    static const GameData data = loadGameData(sourceRoot() / "data", "byeokjegwan.json");
    return data.zhHant;
}

ScenarioDef duel(const std::vector<Piece> &pieces) {
    ScenarioDef scenario = loadScenario(sourceRoot() / "tests/data/scenarios/duel_flat.json", defs(), strings());
    for (const Piece &piece : pieces) {
        Placement place;
        place.id = piece.id;
        place.labelKey = "unit." + piece.type;
        place.type = defs().indexOf(piece.type).value();
        place.pos = piece.pos;
        place.facing = radians(piece.facingDeg);
        scenario.deployment.push_back(place);
    }
    return scenario;
}

Battle fight(const ScenarioDef &scenario, std::uint64_t seed, bool ai) {
    BattleOptions options;
    options.aiEnabled = ai;
    return Battle(defs(), scenario, seed, options);
}

void readyDefender(Battle &battle, const std::string &id) {
    Unit &unit = testing::BattleAccess::unit(battle, battle.sceneUnit(id).value());
    unit.reloadSec = 0.f;
    unit.stillSec = 16.f;
}

void orderAttack(Battle &battle, const std::string &attacker, const std::string &defender) {
    Command command;
    command.type = Command::Type::Attack;
    command.units = {battle.sceneUnit(attacker).value()};
    command.target = battle.sceneUnit(defender).value();
    battle.submit(command);
}

float livingMen(const Battle &battle, UnitId id) {
    float sum = 0.f;
    for (const Unit &unit : battle.units()) {
        if (unit.state == UnitState::Gone) {
            continue;
        }
        if (unit.id == id || unit.origin == id) {
            sum += unit.men;
        }
    }
    return sum;
}

const Event *firstOf(const std::vector<Event> &events, Event::Type type, UnitId id, bool useA) {
    for (const Event &event : events) {
        if (event.type == type && (useA ? event.a == id : event.b == id)) {
            return &event;
        }
    }
    return nullptr;
}

} // namespace

TEST_CASE("duel D1 through D7 stay inside the prototype bands") {
    for (std::uint64_t seed = 1; seed <= 10; ++seed) {
        INFO("D1 seed " << seed);
        Battle battle = fight(
            duel({{"rider", "jiading", {440.f, 420.f}, 90.f}, {"guns", "teppo", {440.f, 700.f}, -90.f}}), seed, false);
        readyDefender(battle, "guns");
        orderAttack(battle, "rider", "guns");
        std::vector<Event> events;
        for (int i = 0; i < 300; ++i) {
            battle.step();
            for (const Event &event : battle.drainEvents()) {
                events.push_back(event);
            }
        }
        const UnitId rider = battle.sceneUnit("rider").value();
        const UnitId guns = battle.sceneUnit("guns").value();
        const Event *impact = firstOf(events, Event::Type::Impact, rider, true);
        REQUIRE(impact != nullptr);
        CHECK(nearTick(impact->tick, 51, 2));
        CHECK(nearRatio(impact->value, 3.30f));
        CHECK((impact->flags & kBreaks) != 0);
        CHECK(battle.unit(rider).everBroken == false);
        CHECK(nearMen(battle.unit(rider).men, 383.f));
        CHECK(battle.unit(guns).everBroken);
        const float remnants = livingMen(battle, guns);
        CHECK(remnants >= 222.f);
        CHECK(remnants <= 227.f);
    }

    for (std::uint64_t seed = 1; seed <= 10; ++seed) {
        INFO("D2 seed " << seed);
        Battle battle = fight(
            duel({{"rider", "liaodong", {440.f, 420.f}, 90.f}, {"guns", "teppo", {440.f, 700.f}, -90.f}}), seed, false);
        readyDefender(battle, "guns");
        orderAttack(battle, "rider", "guns");
        std::vector<Event> events;
        for (int i = 0; i < 300; ++i) {
            battle.step();
            for (const Event &event : battle.drainEvents()) {
                events.push_back(event);
            }
        }
        const Event *impact = firstOf(events, Event::Type::Impact, battle.sceneUnit("rider").value(), true);
        REQUIRE(impact != nullptr);
        CHECK(nearTick(impact->tick, 50, 2));
        CHECK(nearRatio(impact->value, 2.36f));
        CHECK((impact->flags & kBreaks) != 0);
        CHECK(nearMen(battle.unit(battle.sceneUnit("rider").value()).men, 473.f));
        CHECK(battle.unit(battle.sceneUnit("guns").value()).everBroken);
    }

    for (std::uint64_t seed = 1; seed <= 10; ++seed) {
        INFO("D3 seed " << seed);
        Battle battle = fight(
            duel({{"rider", "jiading", {440.f, 420.f}, 90.f}, {"pikes", "yari", {440.f, 700.f}, -90.f}}), seed, false);
        readyDefender(battle, "pikes");
        orderAttack(battle, "rider", "pikes");
        std::vector<Event> events;
        for (int i = 0; i < 600; ++i) {
            battle.step();
            for (const Event &event : battle.drainEvents()) {
                events.push_back(event);
            }
        }
        const UnitId rider = battle.sceneUnit("rider").value();
        const UnitId pikes = battle.sceneUnit("pikes").value();
        const Event *impact = firstOf(events, Event::Type::Impact, rider, true);
        REQUIRE(impact != nullptr);
        CHECK(nearTick(impact->tick, 50, 2));
        CHECK(nearRatio(impact->value, 0.43f));
        CHECK(battle.unit(rider).everBroken);
        CHECK_FALSE(battle.unit(pikes).everBroken);
        CHECK(nearMen(battle.unit(pikes).men, 507.f));
    }

    for (std::uint64_t seed = 1; seed <= 10; ++seed) {
        INFO("D4 seed " << seed);
        Battle battle = fight(
            duel({{"rider", "jiading", {160.f, 700.f}, 0.f}, {"pikes", "yari", {440.f, 700.f}, -90.f}}), seed, false);
        readyDefender(battle, "pikes");
        orderAttack(battle, "rider", "pikes");
        std::vector<Event> events;
        for (int i = 0; i < 300; ++i) {
            battle.step();
            for (const Event &event : battle.drainEvents()) {
                events.push_back(event);
            }
        }
        const UnitId rider = battle.sceneUnit("rider").value();
        const UnitId pikes = battle.sceneUnit("pikes").value();
        const Event *impact = firstOf(events, Event::Type::Impact, rider, true);
        REQUIRE(impact != nullptr);
        CHECK(nearTick(impact->tick, 47, 2));
        CHECK(nearRatio(impact->value, 2.23f));
        CHECK((impact->flags & kBreaks) != 0);
        CHECK(battle.unit(pikes).everBroken);
        CHECK(nearMen(battle.unit(rider).men, 397.f));
    }

    for (std::uint64_t seed = 1; seed <= 10; ++seed) {
        INFO("D5 seed " << seed);
        Battle battle =
            fight(duel({{"rider", "jiading", {440.f, 420.f}, 90.f}, {"swords", "samurai", {440.f, 700.f}, -90.f}}),
                  seed, false);
        readyDefender(battle, "swords");
        orderAttack(battle, "rider", "swords");
        std::vector<Event> events;
        for (int i = 0; i < 600; ++i) {
            battle.step();
            for (const Event &event : battle.drainEvents()) {
                events.push_back(event);
            }
        }
        const UnitId rider = battle.sceneUnit("rider").value();
        const UnitId swords = battle.sceneUnit("swords").value();
        const Event *impact = firstOf(events, Event::Type::Impact, rider, true);
        const Event *broken = firstOf(events, Event::Type::Broken, swords, true);
        REQUIRE(impact != nullptr);
        REQUIRE(broken != nullptr);
        CHECK(nearTick(impact->tick, 50, 2));
        CHECK(nearRatio(impact->value, 1.54f));
        CHECK((impact->flags & kBreaks) == 0);
        CHECK(nearTick(broken->tick, 137, 10));
        CHECK_FALSE(battle.unit(rider).everBroken);
        CHECK(nearMen(battle.unit(rider).men, 321.f));
    }

    for (std::uint64_t seed = 1; seed <= 10; ++seed) {
        INFO("D6 seed " << seed);
        Battle battle = fight(duel({{"rider", "liaodong", {440.f, 600.f}, 90.f},
                                    {"pikes", "yari", {440.f, 700.f}, -90.f},
                                    {"flank", "jiading", {150.f, 700.f}, 0.f}}),
                              seed, false);
        readyDefender(battle, "pikes");
        orderAttack(battle, "rider", "pikes");
        testing::BattleAccess::unit(battle, battle.sceneUnit("rider").value()).recoverSec = 400.f;
        std::vector<Event> events;
        bool ordered = false;
        for (int i = 0; i < 300; ++i) {
            if (!ordered && battle.tick() == 60) {
                orderAttack(battle, "flank", "pikes");
                ordered = true;
            }
            battle.step();
            for (const Event &event : battle.drainEvents()) {
                events.push_back(event);
            }
        }
        const UnitId flank = battle.sceneUnit("flank").value();
        const UnitId pikes = battle.sceneUnit("pikes").value();
        const Event *impact = firstOf(events, Event::Type::Impact, flank, true);
        REQUIRE(impact != nullptr);
        CHECK(nearTick(impact->tick, 110, 2));
        CHECK(impact->value >= 2.32f - 0.05f);
        CHECK(impact->value <= 3.70f + 0.05f);
        CHECK((impact->flags & kBreaks) != 0);
        CHECK(battle.unit(pikes).everBroken);
        CHECK(battle.unit(battle.sceneUnit("rider").value()).everBroken);
    }

    for (std::uint64_t seed = 1; seed <= 10; ++seed) {
        INFO("D7 seed " << seed);
        Battle battle = fight(
            duel({{"rider", "jiading", {440.f, 420.f}, 90.f}, {"guns", "teppo", {440.f, 700.f}, -90.f}}), seed, true);
        readyDefender(battle, "guns");
        orderAttack(battle, "rider", "guns");
        const UnitId rider = battle.sceneUnit("rider").value();
        const UnitId guns = battle.sceneUnit("guns").value();
        std::vector<Event> events;
        bool moved = false;
        for (int i = 0; i < 900; ++i) {
            if (!moved && battle.unit(guns).everBroken) {
                Command command;
                command.type = Command::Type::Move;
                command.units = {rider};
                command.point = {440.f, 150.f};
                battle.submit(command);
                moved = true;
            }
            battle.step();
            for (const Event &event : battle.drainEvents()) {
                events.push_back(event);
            }
        }
        const Event *impact = firstOf(events, Event::Type::Impact, rider, true);
        const Event *merged = nullptr;
        for (const Event &event : events) {
            if (event.type != Event::Type::Merged) {
                continue;
            }
            const bool gunLine = event.a == guns || event.b == guns || battle.unit(event.a).origin == guns ||
                                 battle.unit(event.b).origin == guns;
            if (gunLine) {
                merged = &event;
                break;
            }
        }
        REQUIRE(impact != nullptr);
        REQUIRE(merged != nullptr);
        CHECK(nearTick(impact->tick, 47, 2));
        CHECK(nearRatio(impact->value, 3.14f));
        CHECK(nearTick(merged->tick, 257, 20));
        int fightingPieces = 0;
        float men = 0.f;
        for (const Unit &unit : battle.units()) {
            if (unit.state == UnitState::Gone || (unit.id != guns && unit.origin != guns)) {
                continue;
            }
            if (unit.state == UnitState::Steady || unit.state == UnitState::Wavering) {
                fightingPieces += 1;
                men += unit.men;
            }
        }
        CHECK(fightingPieces == 1);
        CHECK(nearMen(men, 343.f));
    }
}
