#include "core/ai/Ai.h"
#include "../BattleAccess.h"
#include "core/sim/Battle.h"
#include "data/Load.h"

#include <doctest/doctest.h>

#include <cmath>
#include <filesystem>
#include <numbers>
#include <vector>

using namespace tq;

namespace {

std::filesystem::path sourceRoot() { return TQ_SOURCE_DIR; }

GameData byeokjegwan() { return loadGameData(sourceRoot() / "data", "byeokjegwan.json"); }

bool near(float actual, float expected) { return std::fabs(actual - expected) < 0.0001f; }

void only(Battle &battle, const std::vector<UnitId> &ids) {
    for (const Unit &unit : battle.units()) {
        bool stays = false;
        for (UnitId id : ids) {
            stays = stays || id == unit.id;
        }
        if (!stays) {
            Unit &live = testing::BattleAccess::unit(battle, unit.id);
            live.state = UnitState::Gone;
            live.contacts.clear();
        }
    }
}

UnitId findRole(const Battle &battle, AiRole role) {
    for (const Unit &unit : battle.units()) {
        if (battle.typeOf(unit).ai == role) {
            return unit.id;
        }
    }
    REQUIRE(false);
    return kNoUnit;
}

UnitId findSide(const Battle &battle, Side side, UnitId skip = kNoUnit) {
    for (const Unit &unit : battle.units()) {
        if (unit.side == side && unit.id != skip) {
            return unit.id;
        }
    }
    REQUIRE(false);
    return kNoUnit;
}

} // namespace

TEST_CASE("scoreTarget applies each multiplier and keeps the gunner boundary strict") {
    UnitType shock;
    shock.ai = AiRole::Shock;
    UnitType pike;
    pike.ai = AiRole::PikeLine;
    UnitType gunner;
    gunner.ai = AiRole::Gunner;
    UnitType soldier;
    UnitType general = soldier;
    general.general = true;
    GameDefs defs;
    defs.types = {gunner};

    Unit self;
    self.pos = {0.f, 0.f};
    self.type = 0;
    Unit target;
    target.pos = {0.f, 0.f};
    target.state = UnitState::Steady;
    const float base = 1.f / 60.f;
    CHECK(near(scoreTarget(self, shock, target, soldier, {}, defs), base));

    target.state = UnitState::Wavering;
    CHECK(near(scoreTarget(self, shock, target, soldier, {}, defs), base * 1.6f));

    target.state = UnitState::Routing;
    CHECK(near(scoreTarget(self, shock, target, soldier, {}, defs), base * 1.6f));
    CHECK(near(scoreTarget(self, pike, target, soldier, {}, defs), base * 0.5f));

    target.state = UnitState::Steady;
    CHECK(near(scoreTarget(self, shock, target, general, {}, defs), base * 1.3f));
    CHECK(near(scoreTarget(self, pike, target, general, {}, defs), base * 1.05f));

    target.recoverSec = 1.f;
    target.state = UnitState::Wavering;
    CHECK(near(scoreTarget(self, shock, target, general, {}, defs), base * 1.6f * 1.3f * 1.3f));

    target = {};
    target.pos = {0.f, 0.f};
    target.state = UnitState::Steady;
    Unit gun;
    gun.side = Side::Jp;
    gun.type = 0;
    gun.state = UnitState::Steady;
    gun.pos = {150.f, 0.f};
    CHECK(near(scoreTarget(self, pike, target, soldier, std::span<const Unit>(&gun, 1), defs), base));
    gun.pos = {149.f, 0.f};
    CHECK(near(scoreTarget(self, pike, target, soldier, std::span<const Unit>(&gun, 1), defs), base * 1.7f));
    CHECK(near(scoreTarget(self, shock, target, soldier, std::span<const Unit>(&gun, 1), defs), base));
}

TEST_CASE("gunners advance, halt, and never keep an attack order") {
    const GameData data = byeokjegwan();
    Battle far(data.defs, data.scenario, 1);
    const UnitId gunner = findRole(far, AiRole::Gunner);
    const UnitId pike = findRole(far, AiRole::PikeLine);
    const UnitId ming = findSide(far, Side::Ming);
    only(far, {gunner, pike, ming});
    Unit &gun = testing::BattleAccess::unit(far, gunner);
    Unit &mate = testing::BattleAccess::unit(far, pike);
    Unit &target = testing::BattleAccess::unit(far, ming);
    target.pos = {400.f, 400.f};
    mate.pos = {400.f, 450.f};
    mate.vanguard = true;
    gun.pos = {400.f, 800.f};
    gun.vanguard = true;
    gun.order.type = Order::Type::Attack;
    gun.order.target = ming;
    testing::BattleAccess::think(far);
    CHECK(gun.order.type == Order::Type::Move);
    const float keep = far.typeOf(gun).range - 40.f;
    const float dx = gun.order.point.x - target.pos.x;
    const float dy = gun.order.point.y - target.pos.y;
    CHECK(near(std::sqrt(dx * dx + dy * dy), keep));
    CHECK(near(*gun.order.face, std::atan2(target.pos.y - gun.pos.y, target.pos.x - gun.pos.x)));

    Battle halt(data.defs, data.scenario, 1);
    const UnitId halted = findRole(halt, AiRole::Gunner);
    const UnitId enemy = findSide(halt, Side::Ming);
    only(halt, {halted, enemy});
    Unit &shooter = testing::BattleAccess::unit(halt, halted);
    Unit &focus = testing::BattleAccess::unit(halt, enemy);
    shooter.pos = {100.f, 100.f};
    shooter.vanguard = true;
    const float range = halt.typeOf(shooter).range;
    focus.pos = {100.f, 100.f + (range - 20.f)};
    shooter.order.type = Order::Type::Move;
    shooter.order.point = {0.f, 0.f};
    testing::BattleAccess::think(halt);
    CHECK(shooter.order.type == Order::Type::Hold);
    CHECK_FALSE(shooter.order.face.has_value());

    focus.pos = {100.f, 100.f + 170.f};
    shooter.order.type = Order::Type::Move;
    shooter.order.point = {12.f, 12.f};
    testing::BattleAccess::think(halt);
    CHECK(shooter.order.type == Order::Type::Move);
    CHECK(shooter.order.point.x == 12.f);

    shooter.order.type = Order::Type::Attack;
    shooter.order.target = enemy;
    testing::BattleAccess::think(halt);
    CHECK(shooter.order.type == Order::Type::Hold);
}

TEST_CASE("the vanguard general supports a fight and follows the van samurai") {
    const GameData data = byeokjegwan();
    Battle support(data.defs, data.scenario, 1);
    const UnitId general = findRole(support, AiRole::VanGeneral);
    const UnitId pike = findRole(support, AiRole::PikeLine);
    const UnitId ming = findSide(support, Side::Ming);
    only(support, {general, pike, ming});
    Unit &leader = testing::BattleAccess::unit(support, general);
    Unit &line = testing::BattleAccess::unit(support, pike);
    Unit &enemy = testing::BattleAccess::unit(support, ming);
    leader.pos = {300.f, 300.f};
    leader.contacts.clear();
    line.pos = {300.f, 400.f};
    line.contacts = {ming};
    enemy.pos = {20.f, 20.f};
    testing::BattleAccess::think(support);
    CHECK(leader.order.type == Order::Type::Attack);
    CHECK(leader.order.target == ming);

    Battle follow(data.defs, data.scenario, 1);
    const UnitId van = findRole(follow, AiRole::VanGeneral);
    const UnitId shock = findRole(follow, AiRole::Shock);
    const UnitId other = findSide(follow, Side::Ming);
    only(follow, {van, shock, other});
    Unit &tachibana = testing::BattleAccess::unit(follow, van);
    Unit &samurai = testing::BattleAccess::unit(follow, shock);
    tachibana.contacts.clear();
    samurai.contacts.clear();
    samurai.vanguard = true;
    samurai.pos = {500.f, 500.f};
    samurai.facing = 0.f;
    tachibana.pos = {100.f, 100.f};
    testing::BattleAccess::unit(follow, other).pos = {20.f, 900.f};
    testing::BattleAccess::think(follow);
    CHECK(tachibana.order.type == Order::Type::Move);
    CHECK(near(tachibana.order.point.x, 445.f));
    CHECK(near(tachibana.order.point.y, 500.f));
    CHECK(near(*tachibana.order.face, 0.f));
}

TEST_CASE("the main general anchors behind the line unless the enemy is close") {
    const GameData data = byeokjegwan();
    const auto index = data.defs.indexOf("kobayakawa");
    REQUIRE(index.has_value());
    Battle battle(data.defs, data.scenario, 1);
    const UnitId general = findRole(battle, AiRole::VanGeneral);
    const UnitId pike = findRole(battle, AiRole::PikeLine);
    const UnitId gunner = findRole(battle, AiRole::Gunner);
    const UnitId ming = findSide(battle, Side::Ming);
    only(battle, {general, pike, gunner, ming});
    Unit &leader = testing::BattleAccess::unit(battle, general);
    leader.type = *index;
    leader.pos = {200.f, 800.f};
    leader.contacts.clear();
    Unit &left = testing::BattleAccess::unit(battle, pike);
    Unit &right = testing::BattleAccess::unit(battle, gunner);
    left.pos = {100.f, 200.f};
    right.pos = {300.f, 200.f};
    left.contacts.clear();
    right.contacts.clear();
    Unit &enemy = testing::BattleAccess::unit(battle, ming);
    enemy.pos = {200.f, 950.f};
    testing::BattleAccess::think(battle);
    CHECK(leader.order.type == Order::Type::Move);
    CHECK(near(leader.order.point.x, 200.f));
    CHECK(near(leader.order.point.y, 330.f));
    CHECK(near(*leader.order.face, -std::numbers::pi_v<float> / 2.f));

    enemy.pos = {200.f, 850.f};
    testing::BattleAccess::think(battle);
    CHECK(leader.order.type == Order::Type::Attack);
    CHECK(leader.order.target == ming);
}

TEST_CASE("a deploying unit is taken over inside 170 meters") {
    const GameData data = byeokjegwan();
    Battle battle(data.defs, data.scenario, 1);
    const UnitId gunner = findRole(battle, AiRole::Gunner);
    const UnitId ming = findSide(battle, Side::Ming);
    only(battle, {gunner, ming});
    Unit &shooter = testing::BattleAccess::unit(battle, gunner);
    Unit &enemy = testing::BattleAccess::unit(battle, ming);
    shooter.deploying = true;
    shooter.vanguard = true;
    shooter.pos = {100.f, 100.f};
    shooter.order.type = Order::Type::Move;
    shooter.order.point = {1.f, 1.f};
    enemy.pos = {100.f, 271.f};
    testing::BattleAccess::think(battle);
    CHECK(shooter.deploying);
    CHECK(shooter.order.point.x == 1.f);

    enemy.pos = {100.f, 270.f};
    testing::BattleAccess::think(battle);
    CHECK_FALSE(shooter.deploying);
}

TEST_CASE("fragments regroup on the nearest mate") {
    const GameData data = byeokjegwan();
    Battle battle(data.defs, data.scenario, 1);
    const UnitId pike = findRole(battle, AiRole::PikeLine);
    const UnitId shock = findRole(battle, AiRole::Shock);
    const UnitId gunner = findRole(battle, AiRole::Gunner);
    const UnitId ming = findSide(battle, Side::Ming);
    only(battle, {pike, shock, gunner, ming});
    Unit &self = testing::BattleAccess::unit(battle, pike);
    Unit &nearMate = testing::BattleAccess::unit(battle, shock);
    Unit &farMate = testing::BattleAccess::unit(battle, gunner);
    Unit &enemy = testing::BattleAccess::unit(battle, ming);
    self.pos = {200.f, 400.f};
    self.isFragment = true;
    self.origin = pike;
    self.vanguard = true;
    self.contacts.clear();
    nearMate.pos = {280.f, 400.f};
    nearMate.isFragment = true;
    nearMate.origin = pike;
    nearMate.contacts.clear();
    farMate.pos = {400.f, 400.f};
    farMate.isFragment = true;
    farMate.origin = pike;
    farMate.contacts.clear();
    enemy.pos = {200.f, 600.f};
    testing::BattleAccess::think(battle);
    CHECK(self.order.type == Order::Type::Move);
    CHECK(near(self.order.point.x, nearMate.pos.x));
    CHECK(near(self.order.point.y, nearMate.pos.y));
    CHECK(near(*self.order.face, -std::numbers::pi_v<float> / 2.f));

    nearMate.pos = {460.f, 400.f};
    farMate.pos = {460.f, 400.f};
    self.order.type = Order::Type::Move;
    testing::BattleAccess::think(battle);
    CHECK(self.order.type == Order::Type::Hold);
}

TEST_CASE("target switches lag and ties keep the smaller id") {
    const GameData data = byeokjegwan();
    Battle battle(data.defs, data.scenario, 1);
    const UnitId shock = findRole(battle, AiRole::Shock);
    std::vector<UnitId> ming;
    for (const Unit &unit : battle.units()) {
        if (unit.side == Side::Ming && !battle.typeOf(unit).general) {
            ming.push_back(unit.id);
        }
    }
    REQUIRE(ming.size() >= 2);
    const UnitId first = ming[0];
    const UnitId second = ming[1];
    only(battle, {shock, first, second});
    Unit &samurai = testing::BattleAccess::unit(battle, shock);
    Unit &close = testing::BattleAccess::unit(battle, first);
    Unit &away = testing::BattleAccess::unit(battle, second);
    samurai.vanguard = false;
    samurai.pos = {200.f, 500.f};
    samurai.contacts.clear();
    close.pos = {200.f, 600.f};
    away.pos = {200.f, 900.f};
    samurai.order.type = Order::Type::Attack;
    samurai.order.target = second;
    testing::BattleAccess::think(battle);
    CHECK(samurai.order.target == first);

    away.pos = {200.f, 620.f};
    samurai.order.type = Order::Type::Attack;
    samurai.order.target = second;
    testing::BattleAccess::think(battle);
    CHECK(samurai.order.target == second);

    close.pos = {200.f, 700.f};
    away.pos = {200.f, 700.f};
    samurai.order.type = Order::Type::Hold;
    testing::BattleAccess::think(battle);
    CHECK(samurai.order.target == std::min(first, second));
}

TEST_CASE("vanguard aggression waits until the hold time is exceeded") {
    const GameData data = byeokjegwan();
    Battle battle(data.defs, data.scenario, 1);
    const UnitId pike = findRole(battle, AiRole::PikeLine);
    const UnitId ming = findSide(battle, Side::Ming);
    only(battle, {pike, ming});
    Unit &line = testing::BattleAccess::unit(battle, pike);
    Unit &enemy = testing::BattleAccess::unit(battle, ming);
    line.vanguard = true;
    line.pos = {100.f, 100.f};
    line.order.type = Order::Type::Hold;
    enemy.pos = {700.f, 100.f};
    for (int i = 0; i < 450; ++i) {
        battle.step();
    }
    CHECK(battle.tick() == 450);
    CHECK_FALSE(testing::BattleAccess::vanAggro(battle));
    CHECK(battle.unit(pike).order.type == Order::Type::Hold);
    for (int i = 0; i < 10; ++i) {
        battle.step();
    }
    CHECK(testing::BattleAccess::vanAggro(battle));
    CHECK(battle.unit(pike).order.type == Order::Type::Attack);
    CHECK(battle.unit(pike).order.target == ming);
}
