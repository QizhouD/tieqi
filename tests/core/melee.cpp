#include "../BattleAccess.h"
#include "core/rules/Preview.h"
#include "core/sim/Battle.h"
#include "data/Load.h"

#include <doctest/doctest.h>

#include <cmath>
#include <filesystem>
#include <numbers>

using namespace tq;

namespace {

std::filesystem::path sourceRoot() { return TQ_SOURCE_DIR; }

const float kSouth = static_cast<float>(90.0 * std::numbers::pi / 180.0);
const float kNorth = static_cast<float>(-90.0 * std::numbers::pi / 180.0);
const float kEast = 0.f;

bool near(float actual, float expected) { return std::fabs(actual - expected) < 0.001f; }

void place(Unit &unit, Vec2 pos, float facing) {
    unit.pos = pos;
    unit.facing = facing;
}

void refill(Battle &battle, Unit &unit) {
    const UnitType &type = battle.typeOf(unit);
    unit.men = type.men;
    unit.maxMen = type.men;
    unit.morale = type.morale;
    unit.state = UnitState::Steady;
    unit.gone = GoneReason::None;
    unit.reloadSec = 100.f;
    unit.stillSec = 0.f;
    unit.momentum = 0;
    unit.recoverSec = 0.f;
    unit.engagedSec = 0.f;
    unit.routSec = 0.f;
    unit.charging = false;
    unit.speed = 0.f;
    unit.contacts.clear();
    unit.order = {};
    unit.everBroken = false;
    unit.breaking = false;
}

void quiet(Battle &battle) {
    for (const Unit &unit : battle.units()) {
        refill(battle, testing::BattleAccess::unit(battle, unit.id));
    }
}

void parkExcept(Battle &battle, std::initializer_list<UnitId> keep) {
    for (const Unit &unit : battle.units()) {
        bool stay = false;
        for (UnitId id : keep) {
            stay = stay || id == unit.id;
        }
        if (!stay) {
            testing::BattleAccess::unit(battle, unit.id).pos = {10.f, 10.f};
        }
    }
}

} // namespace

TEST_CASE("melee ratios M1 through M8") {
    const GameData data = loadGameData(sourceRoot() / "data", "byeokjegwan.json");
    Battle battle(data.defs, data.scenario, 1);
    Unit &jiading = testing::BattleAccess::unit(battle, 1);
    Unit &liaodong = testing::BattleAccess::unit(battle, 3);
    Unit &yari = testing::BattleAccess::unit(battle, 6);
    Unit &teppo = testing::BattleAccess::unit(battle, 7);
    Unit &samurai = testing::BattleAccess::unit(battle, 9);
    refill(battle, jiading);
    refill(battle, liaodong);
    refill(battle, yari);
    refill(battle, teppo);
    refill(battle, samurai);

    place(teppo, {700.f, 470.f}, kNorth);
    place(jiading, {700.f, 440.f}, kSouth);
    CHECK(near(previewAttack(battle, 1, 7).ratio, 1.9091f));

    place(yari, {700.f, 470.f}, kNorth);
    CHECK(near(previewAttack(battle, 1, 6).ratio, 0.6652f));
    jiading.recoverSec = 1.f;
    CHECK(near(previewAttack(battle, 1, 6).ratio, 0.5654f));
    jiading.recoverSec = 0.f;

    place(samurai, {700.f, 470.f}, kNorth);
    place(jiading, {700.f, 440.f}, kSouth);
    CHECK(near(previewAttack(battle, 9, 1).ratio, 1.3333f));
    place(jiading, {660.f, 470.f}, kEast);
    CHECK(near(previewAttack(battle, 1, 9).ratio, 1.2f));

    place(teppo, {700.f, 470.f}, kNorth);
    teppo.state = UnitState::Routing;
    place(liaodong, {700.f, 440.f}, kSouth);
    CHECK(near(previewAttack(battle, 3, 7).ratio, 14.1667f));

    place(samurai, {700.f, 470.f}, kNorth);
    samurai.state = UnitState::Steady;
    place(jiading, {700.f, 440.f}, kSouth);
    jiading.engagedSec = 56.f;
    CHECK(near(previewAttack(battle, 1, 9).ratio, 0.6375f));
    jiading.engagedSec = 0.f;

    place(yari, {700.f, 440.f}, kSouth);
    place(jiading, {700.f, 470.f}, kNorth);
    CHECK(near(previewAttack(battle, 6, 1).ratio, 1.5034f));
}

TEST_CASE("morale thresholds waver, hold, and break") {
    const GameData data = loadGameData(sourceRoot() / "data", "byeokjegwan.json");
    Battle battle(data.defs, data.scenario, 1);
    quiet(battle);
    Unit &teppo = testing::BattleAccess::unit(battle, 7);
    teppo.contacts.push_back(1);
    teppo.morale = 54.9f;
    testing::BattleAccess::combat(battle);
    CHECK(teppo.state == UnitState::Wavering);

    teppo.state = UnitState::Steady;
    teppo.morale = 55.f;
    testing::BattleAccess::combat(battle);
    CHECK(teppo.state == UnitState::Steady);

    teppo.men = 330.f;
    teppo.morale = 29.9f;
    teppo.state = UnitState::Steady;
    testing::BattleAccess::combat(battle);
    CHECK(teppo.state == UnitState::Broken);
    const std::vector<Event> three = battle.drainEvents();
    REQUIRE_FALSE(three.empty());
    CHECK(three.back().type == Event::Type::Broken);
    CHECK(three.back().value == 3.f);

    quiet(battle);
    Unit &again = testing::BattleAccess::unit(battle, 7);
    again.contacts.push_back(1);
    again.men = 329.f;
    again.morale = 29.9f;
    testing::BattleAccess::combat(battle);
    CHECK(battle.drainEvents().back().value == 2.f);

    quiet(battle);
    Unit &single = testing::BattleAccess::unit(battle, 7);
    single.contacts.push_back(1);
    single.men = 149.f;
    single.morale = 29.9f;
    testing::BattleAccess::combat(battle);
    CHECK(battle.drainEvents().back().value == 1.f);
}

TEST_CASE("teppo volley respects range, arc, priority, and the charge bonus") {
    const GameData data = loadGameData(sourceRoot() / "data", "byeokjegwan.json");
    Battle battle(data.defs, data.scenario, 1);
    quiet(battle);
    parkExcept(battle, {7, 1, 3, 8});
    Unit &teppo = testing::BattleAccess::unit(battle, 7);
    Unit &farUnit = testing::BattleAccess::unit(battle, 3);
    teppo.reloadSec = 0.f;
    teppo.stillSec = 16.f;
    place(teppo, {400.f, 400.f}, kEast);

    place(farUnit, {400.f + 175.f, 400.f}, kEast);
    farUnit.charging = true;
    const float before = farUnit.men;
    testing::BattleAccess::combat(battle);
    CHECK(near(before - farUnit.men, 500.f * 0.08f * 0.4f * 1.3f));

    quiet(battle);
    Unit &shooter = testing::BattleAccess::unit(battle, 7);
    Unit &miss = testing::BattleAccess::unit(battle, 1);
    shooter.reloadSec = 0.f;
    shooter.stillSec = 16.f;
    place(shooter, {400.f, 400.f}, kEast);
    place(miss, {400.f + 175.1f, 400.f}, kEast);
    miss.charging = true;
    const float untouched = miss.men;
    testing::BattleAccess::combat(battle);
    CHECK(miss.men == untouched);

    quiet(battle);
    Unit &arc = testing::BattleAccess::unit(battle, 7);
    Unit &edge = testing::BattleAccess::unit(battle, 1);
    arc.reloadSec = 0.f;
    arc.stillSec = 16.f;
    place(arc, {400.f, 400.f}, kEast);
    place(edge, {400.f + std::cos(0.5f) * 100.f, 400.f + std::sin(0.5f) * 100.f}, kEast);
    const float edgeMen = edge.men;
    testing::BattleAccess::combat(battle);
    CHECK(edge.men < edgeMen);

    quiet(battle);
    Unit &wide = testing::BattleAccess::unit(battle, 7);
    Unit &outside = testing::BattleAccess::unit(battle, 1);
    wide.reloadSec = 0.f;
    wide.stillSec = 16.f;
    place(wide, {400.f, 400.f}, kEast);
    place(outside, {400.f + std::cos(1.2f) * 100.f, 400.f + std::sin(1.2f) * 100.f}, kEast);
    const float outsideMen = outside.men;
    testing::BattleAccess::combat(battle);
    CHECK(outside.men == outsideMen);

    quiet(battle);
    Unit &gun = testing::BattleAccess::unit(battle, 7);
    Unit &close = testing::BattleAccess::unit(battle, 1);
    Unit &charging = testing::BattleAccess::unit(battle, 3);
    gun.reloadSec = 0.f;
    gun.stillSec = 16.f;
    place(gun, {400.f, 400.f}, kEast);
    place(close, {440.f, 400.f}, kEast);
    place(charging, {500.f, 400.f}, kEast);
    charging.charging = true;
    const float closeMen = close.men;
    const float chargingMen = charging.men;
    testing::BattleAccess::combat(battle);
    CHECK(close.men == closeMen);
    CHECK(charging.men < chargingMen);

    quiet(battle);
    Unit &plainGun = testing::BattleAccess::unit(battle, 7);
    Unit &plain = testing::BattleAccess::unit(battle, 3);
    place(testing::BattleAccess::unit(battle, 1), {10.f, 10.f}, kEast);
    plainGun.reloadSec = 0.f;
    plainGun.stillSec = 16.f;
    place(plainGun, {400.f, 400.f}, kEast);
    place(plain, {500.f, 400.f}, kEast);
    const float plainBefore = plain.men;
    testing::BattleAccess::combat(battle);
    const float falloff = 1.f - 0.6f * 100.f / 175.f;
    CHECK(near(plainBefore - plain.men, 500.f * 0.08f * falloff));

    quiet(battle);
    Unit &chargeGun = testing::BattleAccess::unit(battle, 7);
    Unit &charged = testing::BattleAccess::unit(battle, 3);
    place(testing::BattleAccess::unit(battle, 1), {10.f, 10.f}, kEast);
    chargeGun.reloadSec = 0.f;
    chargeGun.stillSec = 16.f;
    place(chargeGun, {400.f, 400.f}, kEast);
    place(charged, {500.f, 400.f}, kEast);
    charged.charging = true;
    const float chargedBefore = charged.men;
    testing::BattleAccess::combat(battle);
    CHECK(near(chargedBefore - charged.men, 500.f * 0.08f * falloff * 1.3f));
}

TEST_CASE("routing rallies only past 80 seconds and 180 meters") {
    const GameData data = loadGameData(sourceRoot() / "data", "byeokjegwan.json");
    Battle battle(data.defs, data.scenario, 1);
    quiet(battle);
    parkExcept(battle, {7, 1});
    Unit &teppo = testing::BattleAccess::unit(battle, 7);
    Unit &hunter = testing::BattleAccess::unit(battle, 1);
    place(teppo, {400.f, 400.f}, kNorth);
    teppo.state = UnitState::Routing;
    teppo.routSec = 80.f;
    teppo.morale = 39.f;
    testing::BattleAccess::combat(battle);
    CHECK(teppo.state == UnitState::Routing);

    teppo.routSec = 80.1f;
    place(hunter, {400.f, 579.f}, kSouth);
    testing::BattleAccess::combat(battle);
    CHECK(teppo.state == UnitState::Routing);

    place(hunter, {400.f, 580.f}, kSouth);
    teppo.morale = 39.f;
    testing::BattleAccess::combat(battle);
    CHECK(teppo.state == UnitState::Wavering);
    CHECK(teppo.order.face.has_value());
    bool rallied = false;
    for (const Event &event : battle.drainEvents()) {
        rallied = rallied || event.type == Event::Type::Rallied;
    }
    CHECK(rallied);
}

TEST_CASE("a fallen general changes morale inside 260 meters") {
    const GameData data = loadGameData(sourceRoot() / "data", "byeokjegwan.json");
    Battle battle(data.defs, data.scenario, 1);
    quiet(battle);
    Unit &rusong = testing::BattleAccess::unit(battle, 0);
    Unit &ally = testing::BattleAccess::unit(battle, 1);
    Unit &farAlly = testing::BattleAccess::unit(battle, 3);
    Unit &enemy = testing::BattleAccess::unit(battle, 7);
    Unit &farEnemy = testing::BattleAccess::unit(battle, 9);
    place(rusong, {400.f, 400.f}, kSouth);
    place(ally, {400.f, 600.f}, kSouth);
    place(farAlly, {400.f, 660.f}, kSouth);
    place(enemy, {600.f, 400.f}, kNorth);
    place(farEnemy, {660.f, 400.f}, kNorth);
    const float allyMorale = ally.morale;
    const float farAllyMorale = farAlly.morale;
    const float enemyMorale = enemy.morale;
    const float farEnemyMorale = farEnemy.morale;
    testing::BattleAccess::kill(battle, 0, 160.f);
    CHECK(rusong.gone == GoneReason::GeneralKilled);
    CHECK(near(ally.morale, allyMorale - 22.f));
    CHECK(farAlly.morale == farAllyMorale);
    CHECK(near(enemy.morale, enemyMorale + 8.f));
    CHECK(farEnemy.morale == farEnemyMorale);
}

TEST_CASE("breaking scares nearby troops and gives momentum") {
    const GameData data = loadGameData(sourceRoot() / "data", "byeokjegwan.json");
    Battle battle(data.defs, data.scenario, 1);
    quiet(battle);
    Unit &teppo = testing::BattleAccess::unit(battle, 7);
    Unit &samurai = testing::BattleAccess::unit(battle, 9);
    Unit &ally = testing::BattleAccess::unit(battle, 6);
    Unit &jiading = testing::BattleAccess::unit(battle, 1);
    place(teppo, {400.f, 400.f}, kNorth);
    place(samurai, {400.f, 500.f}, kNorth);
    place(ally, {500.f, 400.f}, kSouth);
    place(jiading, {500.f, 400.f}, kSouth);
    teppo.contacts.push_back(samurai.id);
    teppo.men = 400.f;
    teppo.morale = 29.9f;
    const float allyMorale = ally.morale;
    const float jiadingMorale = jiading.morale;
    testing::BattleAccess::combat(battle);
    const float expectedDrop = 4.f + 10.f * (1.f - 100.f / 170.f);
    CHECK(near(ally.morale, allyMorale - expectedDrop));
    CHECK(near(jiading.morale, jiadingMorale + 4.f));
    CHECK(samurai.momentum == 1);
}
