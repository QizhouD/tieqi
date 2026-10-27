#include "core/rules/Combat.h"
#include "../BattleAccess.h"
#include "core/rules/Preview.h"
#include "core/sim/Battle.h"
#include "data/Load.h"

#include <doctest/doctest.h>

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <numbers>

using namespace tq;

namespace {

std::filesystem::path sourceRoot() { return TQ_SOURCE_DIR; }

const float kSouth = static_cast<float>(90.0 * std::numbers::pi / 180.0);
const float kNorth = static_cast<float>(-90.0 * std::numbers::pi / 180.0);
const float kEast = 0.f;

bool near(float actual, float expected) { return std::fabs(actual - expected) < 0.001f; }

GameData byeokjegwan() { return loadGameData(sourceRoot() / "data", "byeokjegwan.json"); }

void refill(Battle &battle, Unit &unit) {
    const UnitType &type = battle.typeOf(unit);
    unit.men = type.men;
    unit.maxMen = type.men;
    unit.morale = type.morale;
    unit.state = UnitState::Steady;
    unit.gone = GoneReason::None;
    unit.reloadSec = 0.f;
    unit.stillSec = 16.f;
    unit.momentum = 0;
    unit.speed = 0.f;
    unit.runDist = 0.f;
    unit.recoverSec = 0.f;
    unit.charging = false;
    unit.breaking = false;
    unit.everBroken = false;
    unit.brokenSec = 0.f;
    unit.deploying = false;
    unit.order = {};
    unit.contacts.clear();
}

void place(Unit &unit, Vec2 pos, float facing) {
    unit.pos = pos;
    unit.facing = facing;
}

void parkOthers(Battle &battle, UnitId first, UnitId second) {
    for (const Unit &unit : battle.units()) {
        if (unit.id == first || unit.id == second) {
            continue;
        }
        Unit &parked = testing::BattleAccess::unit(battle, unit.id);
        parked.pos = {10.f, 10.f};
        parked.charging = false;
        parked.order.type = Order::Type::Hold;
    }
}

float ratio(Battle &battle, UnitId attacker, UnitId target, bool predict = true) {
    const Unit &a = battle.unit(attacker);
    const Unit &t = battle.unit(target);
    return chargeCalc(a, battle.typeOf(a), t, battle.typeOf(t), battle.terrainAt(a.pos), battle.terrainAt(t.pos),
                      predict)
        .ratio;
}

} // namespace

TEST_CASE("fastEnough uses both boundaries") {
    Unit unit;
    UnitType type;
    type.run = 8.75f;
    unit.speed = 0.7f * 8.75f;
    unit.runDist = 35.f;
    CHECK(fastEnough(unit, type));
    unit.speed = std::nextafter(0.7f * 8.75f, 0.f);
    CHECK_FALSE(fastEnough(unit, type));
    unit.speed = 0.7f * 8.75f;
    unit.runDist = std::nextafter(35.f, 0.f);
    CHECK_FALSE(fastEnough(unit, type));
}

TEST_CASE("charge ratios C1 through C16") {
    const GameData data = byeokjegwan();
    Battle battle(data.defs, data.scenario, 1);
    Unit &jiading = testing::BattleAccess::unit(battle, 1);
    Unit &liaodong = testing::BattleAccess::unit(battle, 3);
    Unit &teppo = testing::BattleAccess::unit(battle, 7);
    Unit &yari = testing::BattleAccess::unit(battle, 6);
    Unit &samurai = testing::BattleAccess::unit(battle, 9);
    Unit &rusong = testing::BattleAccess::unit(battle, 0);
    Unit &general = testing::BattleAccess::unit(battle, 10);
    refill(battle, jiading);
    refill(battle, liaodong);
    refill(battle, teppo);
    refill(battle, yari);
    refill(battle, samurai);
    refill(battle, rusong);
    general.type = *data.defs.indexOf("kobayakawa");
    refill(battle, general);

    place(teppo, {700.f, 470.f}, kNorth);
    place(jiading, {700.f, 440.f}, kSouth);
    CHECK(near(ratio(battle, 1, 7), 2.6667f));
    place(jiading, {700.f, 400.f}, kSouth);
    const Preview front = previewAttack(battle, 1, 7);
    CHECK(front.kind == Preview::Kind::Charge);
    CHECK(near(front.ratio, 2.6667f));
    CHECK(front.verdict == Verdict::Breaks);

    teppo.reloadSec = 40.f;
    CHECK(near(ratio(battle, 1, 7), 3.3333f));
    teppo.reloadSec = 0.f;
    teppo.stillSec = 4.f;
    CHECK(near(ratio(battle, 1, 7), 3.2f));
    teppo.stillSec = 16.f;

    place(liaodong, {700.f, 440.f}, kSouth);
    CHECK(near(ratio(battle, 3, 7), 1.9167f));

    place(yari, {700.f, 470.f}, kNorth);
    place(jiading, {700.f, 440.f}, kSouth);
    CHECK(near(ratio(battle, 1, 6), 0.4308f));
    place(jiading, {700.f, 400.f}, kSouth);
    CHECK(previewAttack(battle, 1, 6).verdict == Verdict::PikeWall);
    place(jiading, {660.f, 470.f}, kEast);
    CHECK(near(ratio(battle, 1, 6), 2.2154f));
    place(jiading, {700.f, 500.f}, kNorth);
    CHECK(near(ratio(battle, 1, 6), 2.9538f));

    place(samurai, {700.f, 470.f}, kNorth);
    place(jiading, {700.f, 440.f}, kSouth);
    CHECK(near(ratio(battle, 1, 9), 1.5238f));
    samurai.state = UnitState::Wavering;
    CHECK(near(ratio(battle, 1, 9), 2.4202f));
    samurai.state = UnitState::Steady;

    jiading.men = 200.f;
    place(teppo, {700.f, 470.f}, kNorth);
    CHECK(near(ratio(battle, 1, 7), 1.8856f));
    jiading.men = jiading.maxMen;

    samurai.momentum = 2;
    place(samurai, {700.f, 440.f}, kSouth);
    place(jiading, {700.f, 470.f}, kNorth);
    CHECK(near(ratio(battle, 9, 1), 1.32f));

    place(general, {700.f, 470.f}, kNorth);
    place(rusong, {700.f, 440.f}, kSouth);
    CHECK(near(ratio(battle, 0, 10), 1.0435f));

    place(jiading, {300.f, 400.f}, kSouth);
    place(teppo, {300.f, 440.f}, kNorth);
    CHECK(near(ratio(battle, 1, 7), 1.0667f));
    place(jiading, {560.f, 900.f}, kSouth);
    place(teppo, {560.f, 985.f}, kNorth);
    CHECK(near(ratio(battle, 1, 7), 2.1333f));

    place(teppo, {700.f, 470.f}, kNorth);
    place(jiading, {700.f, 440.f}, kSouth);
    teppo.state = UnitState::Routing;
    CHECK(near(ratio(battle, 1, 7), 6.902f));
    teppo.state = UnitState::Steady;

    jiading.speed = 4.375f;
    CHECK(near(ratio(battle, 1, 7, false), 1.3333f));
}

TEST_CASE("impact applies the C2 charge result") {
    const GameData data = byeokjegwan();
    Battle battle(data.defs, data.scenario, 1);
    Unit &jiading = testing::BattleAccess::unit(battle, 1);
    Unit &teppo = testing::BattleAccess::unit(battle, 7);
    refill(battle, jiading);
    refill(battle, teppo);
    parkOthers(battle, jiading.id, teppo.id);
    place(jiading, {700.f, 440.f}, kSouth);
    place(teppo, {700.f, 460.f}, kNorth);
    teppo.reloadSec = 40.f;
    jiading.charging = true;
    jiading.speed = battle.typeOf(jiading).run;
    jiading.runDist = 35.f;
    const std::size_t before = battle.units().size();
    testing::BattleAccess::contacts(battle);
    const Unit &broken = battle.unit(7);
    const Unit &rider = battle.unit(1);

    float remnantMen = 0.f;
    int remnants = 0;
    for (const Unit &unit : battle.units()) {
        if (unit.origin == broken.id && unit.isFragment) {
            remnantMen += unit.men;
            remnants += 1;
            CHECK(near(unit.men, 441.667f / 3.f));
        }
    }
    CHECK(remnants == 3);
    CHECK(near(remnantMen, 441.667f));
    CHECK(broken.morale == 25.f);
    CHECK(broken.state == UnitState::Broken);
    CHECK(broken.brokenSec == 9.6f);
    CHECK(near(broken.pos.y, 486.f));
    CHECK(near(broken.pos.x, 700.f));
    CHECK(near(rider.men, 400.f - 2.629f));
    CHECK(rider.recoverSec == 48.f);
    CHECK(rider.speed == 0.f);
    CHECK_FALSE(rider.charging);
    CHECK(battle.stats().impacts == 1);
    CHECK(near(battle.stats().bestImpact, 3.3333f));
    CHECK(battle.stats().brokenJp == 1);
    CHECK(battle.units().size() == before + 2);

    const std::vector<Event> events = battle.drainEvents();
    REQUIRE(events.size() == 3);
    CHECK(events[0].type == Event::Type::Impact);
    CHECK(near(events[0].value, 3.3333f));
    CHECK(events[0].side == ContactSide::Front);
    CHECK((events[0].flags & kBreaks) != 0);
    CHECK(events[1].type == Event::Type::Broken);
    CHECK(events[1].value == 3.f);
    CHECK(events[2].type == Event::Type::Shattered);
    CHECK(events[2].value == 3.f);
}

TEST_CASE("only the moving side takes 0.85 of the overlap") {
    const GameData data = byeokjegwan();
    Battle battle(data.defs, data.scenario, 1);
    Unit &jiading = testing::BattleAccess::unit(battle, 1);
    Unit &teppo = testing::BattleAccess::unit(battle, 7);
    refill(battle, jiading);
    refill(battle, teppo);
    jiading.depth = 22.f;
    jiading.frontage = 22.f;
    teppo.depth = 22.f;
    teppo.frontage = 22.f;
    place(jiading, {200.f, 800.f}, kEast);
    place(teppo, {220.f, 800.f}, kEast);
    jiading.order.type = Order::Type::Move;
    teppo.order.type = Order::Type::Hold;
    testing::BattleAccess::contacts(battle);
    CHECK(near(jiading.pos.x, 198.3f));
    CHECK(near(teppo.pos.x, 220.3f));
    CHECK(jiading.pos.y == 800.f);
    CHECK(teppo.pos.y == 800.f);
}

TEST_CASE("three-eyed volley fires inside 62 meters") {
    const GameData data = byeokjegwan();
    Battle battle(data.defs, data.scenario, 1);
    Unit &jiading = testing::BattleAccess::unit(battle, 1);
    Unit &teppo = testing::BattleAccess::unit(battle, 7);
    refill(battle, jiading);
    refill(battle, teppo);
    parkOthers(battle, jiading.id, teppo.id);
    place(jiading, {700.f, 440.f}, kSouth);
    place(teppo, {700.f, 490.f}, kNorth);
    jiading.order.type = Order::Type::Attack;
    jiading.order.target = teppo.id;
    jiading.charging = true;
    jiading.speed = 0.f;
    jiading.runDist = 0.f;
    jiading.volleyDone = false;
    battle.step();
    CHECK(jiading.volleyDone);
    CHECK(near(teppo.men, 492.5f));
    CHECK(near(teppo.morale, 58.f));
    bool found = false;
    for (const Event &event : battle.drainEvents()) {
        if (event.type == Event::Type::ThreeEyedVolley) {
            found = true;
            CHECK(event.a == jiading.id);
            CHECK(event.b == teppo.id);
        }
    }
    CHECK(found);
}

TEST_CASE("deploying ends past the line and an open exit withdraws") {
    const GameData data = byeokjegwan();
    Battle battle(data.defs, data.scenario, 1);
    Unit &jiading = testing::BattleAccess::unit(battle, 1);
    refill(battle, jiading);
    place(jiading, {440.f, 60.f}, kSouth);
    jiading.deploying = true;
    testing::BattleAccess::contacts(battle);
    CHECK(jiading.deploying);
    jiading.pos.y = 61.f;
    testing::BattleAccess::contacts(battle);
    CHECK_FALSE(jiading.deploying);

    const float men = jiading.men;
    place(jiading, {440.f, 20.f}, kSouth);
    testing::BattleAccess::setExitOpen(battle, true);
    testing::BattleAccess::contacts(battle);
    CHECK(jiading.state == UnitState::Gone);
    CHECK(jiading.gone == GoneReason::Withdrawn);
    CHECK(battle.stats().exitedMen == men);
}
