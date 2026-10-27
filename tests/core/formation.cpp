#include "../BattleAccess.h"
#include "core/rules/Combat.h"
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

const float kSouth = std::numbers::pi_v<float> / 2.f;

bool near(float actual, float expected) { return std::fabs(actual - expected) < 0.001f; }

GameData byeokjegwan() { return loadGameData(sourceRoot() / "data", "byeokjegwan.json"); }

void silence(Battle &battle, UnitId keep) {
    for (const Unit &unit : battle.units()) {
        if (unit.id == keep) {
            continue;
        }
        Unit &other = testing::BattleAccess::unit(battle, unit.id);
        other.state = UnitState::Gone;
        other.contacts.clear();
    }
    Unit &unit = testing::BattleAccess::unit(battle, keep);
    unit.contacts.clear();
    unit.reloadSec = 100.f;
    unit.stillSec = 0.f;
}

} // namespace

TEST_CASE("sizeFrag matches the acceptance sizes") {
    const GameData data = byeokjegwan();
    Battle battle(data.defs, data.scenario, 1);
    Unit gun;
    gun.maxMen = 500.f / 3.f;
    sizeFrag(gun, battle.typeOf(battle.unit(7)));
    CHECK(near(gun.frontage, 34.1406f));
    CHECK(near(gun.depth, 15.8229f));

    gun.maxMen = 30.f;
    sizeFrag(gun, battle.typeOf(battle.unit(7)));
    CHECK(gun.frontage == 22.f);
    CHECK(gun.depth == 14.f);

    Unit horse;
    horse.maxMen = 250.f;
    sizeFrag(horse, battle.typeOf(battle.unit(3)));
    CHECK(near(horse.frontage, 42.2243f));
    CHECK(near(horse.depth, 19.4941f));
}

TEST_CASE("shatter numbers each piece and a single piece is only knocked") {
    const GameData data = byeokjegwan();
    Battle battle(data.defs, data.scenario, 1);
    silence(battle, 7);
    Unit &gun = testing::BattleAccess::unit(battle, 7);
    gun.pos = {400.f, 400.f};
    gun.men = 330.f;
    gun.maxMen = 330.f;
    gun.morale = 29.4f;
    const std::size_t before = battle.units().size();
    testing::BattleAccess::combat(battle);

    std::vector<const Unit *> pieces;
    for (const Unit &unit : battle.units()) {
        if (unit.origin == 7 && unit.isFragment) {
            pieces.push_back(&unit);
        }
    }
    CHECK(pieces.size() == 3);
    CHECK(battle.units().size() == before + 2);
    for (const Unit *piece : pieces) {
        CHECK(near(piece->men, 110.f));
        CHECK(near(piece->maxMen, 110.f));
        CHECK(piece->everBroken);
        CHECK(piece->state == UnitState::Broken);
    }
    CHECK(battle.unit(7).fragNo == 1);
    CHECK(battle.unit(static_cast<UnitId>(before)).fragNo == 2);
    CHECK(battle.unit(static_cast<UnitId>(before + 1)).fragNo == 3);
    CHECK(battle.unit(7).vanguard);
    CHECK_FALSE(battle.unit(static_cast<UnitId>(before)).vanguard);
    const std::vector<Event> events = battle.drainEvents();
    CHECK(events.back().type == Event::Type::Shattered);
    CHECK(events.back().value == 3.f);

    Battle single(data.defs, data.scenario, 1);
    silence(single, 7);
    Unit &alone = testing::BattleAccess::unit(single, 7);
    alone.pos = {400.f, 400.f};
    alone.men = 149.f;
    alone.morale = 29.4f;
    const std::size_t singleCount = single.units().size();
    testing::BattleAccess::combat(single);
    const Unit &knocked = single.unit(7);
    CHECK(single.units().size() == singleCount);
    CHECK(knocked.fragNo == 0);
    CHECK_FALSE(knocked.scatterAngle.has_value());
    CHECK(near(std::sqrt(dot(knocked.knock, knocked.knock)), 8.75f));
    CHECK(near(knocked.facing, kSouth));
    CHECK(knocked.men == 149.f);
    bool shattered = false;
    for (const Event &event : single.drainEvents()) {
        shattered = shattered || event.type == Event::Type::Shattered;
    }
    CHECK_FALSE(shattered);
}

TEST_CASE("split halves a company and later pieces keep the number") {
    const GameData data = byeokjegwan();
    Battle battle(data.defs, data.scenario, 1);
    Unit &horse = testing::BattleAccess::unit(battle, 3);
    horse.pos = {400.f, 400.f};
    horse.facing = kSouth;
    horse.order.face = kSouth;
    silence(battle, 3);
    Unit &ready = testing::BattleAccess::unit(battle, 3);
    ready.pos = {400.f, 400.f};
    ready.facing = kSouth;
    ready.state = UnitState::Steady;
    ready.men = 500.f;
    ready.maxMen = 500.f;
    Command command;
    command.type = Command::Type::Split;
    command.units = {3};
    battle.submit(command);
    battle.step();

    const Unit &left = battle.unit(3);
    const Unit &right = battle.unit(static_cast<UnitId>(battle.units().size() - 1));
    CHECK(near(left.men, 250.f));
    CHECK(near(right.men, 250.f));
    CHECK(near(left.maxMen, 250.f));
    CHECK(near(right.maxMen, 250.f));
    CHECK(near(left.frontage, 42.2243f));
    CHECK(near(left.depth, 19.4941f));
    CHECK(near(right.frontage, 42.2243f));
    CHECK(near(right.depth, 19.4941f));
    CHECK(near(std::sqrt(dot(left.pos - Vec2{400.f, 400.f}, left.pos - Vec2{400.f, 400.f})), 23.1121f));
    CHECK(near(std::sqrt(dot(right.pos - Vec2{400.f, 400.f}, right.pos - Vec2{400.f, 400.f})), 23.1121f));
    CHECK(left.fragNo == 1);
    CHECK(right.fragNo == 2);
    CHECK(left.origin == 3);
    CHECK(right.origin == 3);
    CHECK_FALSE(left.isFragment);
    CHECK_FALSE(right.isFragment);
    const NameParts leftName = displayName(left);
    CHECK(leftName.suffix == NameSuffix::Detachment);
    CHECK(leftName.numeral == 1);

    silence(battle, 3);
    Unit &again = testing::BattleAccess::unit(battle, 3);
    again.men = 250.f;
    again.maxMen = 250.f;
    again.morale = 29.4f;
    again.state = UnitState::Steady;
    const std::size_t before = battle.units().size();
    testing::BattleAccess::combat(battle);
    CHECK(battle.units().size() == before + 1);
    CHECK(battle.unit(3).fragNo == 3);
    CHECK(battle.unit(static_cast<UnitId>(before)).fragNo == 4);
    CHECK(near(battle.unit(3).men, 125.f));
    CHECK(near(battle.unit(static_cast<UnitId>(before)).men, 125.f));
    CHECK(battle.unit(3).isFragment);
}

TEST_CASE("merge requires the same origin, no contact, and at most 46 meters") {
    const GameData data = byeokjegwan();
    Battle battle(data.defs, data.scenario, 1);
    Unit &left = testing::BattleAccess::unit(battle, 1);
    Unit &right = testing::BattleAccess::unit(battle, 3);
    Unit &extra = testing::BattleAccess::unit(battle, 4);
    for (Unit *unit : {&left, &right, &extra}) {
        unit->state = UnitState::Steady;
        unit->origin = 1;
        unit->isFragment = true;
        unit->rejoin = false;
        unit->contacts.clear();
        unit->knock = {};
        unit->reloadSec = 100.f;
    }
    left.pos = {400.f, 400.f};
    right.pos = {446.f, 400.f};
    extra.pos = {800.f, 800.f};
    left.men = 100.f;
    left.maxMen = 100.f;
    left.morale = 60.f;
    right.men = 300.f;
    right.maxMen = 300.f;
    right.morale = 90.f;
    extra.men = 50.f;
    extra.maxMen = 50.f;
    extra.morale = 80.f;
    testing::BattleAccess::combat(battle);
    CHECK(battle.unit(3).gone == GoneReason::Merged);
    CHECK(near(battle.unit(1).men, 400.f));
    CHECK(near(battle.unit(1).maxMen, 400.f));
    CHECK(near(battle.unit(1).morale, 82.5f));
    CHECK(battle.unit(1).fragNo == 0);
    CHECK(battle.unit(1).isFragment);
    CHECK(displayName(battle.unit(1)).suffix == NameSuffix::Combined);
    CHECK(battle.unit(4).state == UnitState::Steady);

    Unit &rest = testing::BattleAccess::unit(battle, 4);
    rest.pos = battle.unit(1).pos;
    rest.contacts.clear();
    rest.knock = {};
    testing::BattleAccess::combat(battle);
    CHECK(battle.unit(4).gone == GoneReason::Merged);
    CHECK(near(battle.unit(1).men, 450.f));
    CHECK_FALSE(battle.unit(1).isFragment);
    CHECK_FALSE(battle.unit(1).rejoin);
    CHECK(displayName(battle.unit(1)).suffix == NameSuffix::None);

    Battle apart(data.defs, data.scenario, 1);
    Unit &nearLeft = testing::BattleAccess::unit(apart, 1);
    Unit &nearRight = testing::BattleAccess::unit(apart, 3);
    nearLeft.state = UnitState::Steady;
    nearRight.state = UnitState::Steady;
    nearLeft.origin = 1;
    nearRight.origin = 1;
    nearLeft.isFragment = true;
    nearRight.isFragment = true;
    nearLeft.contacts.clear();
    nearRight.contacts.clear();
    nearLeft.knock = {};
    nearRight.knock = {};
    nearLeft.pos = {400.f, 400.f};
    nearRight.pos = {446.1f, 400.f};
    nearLeft.morale = 80.f;
    nearRight.morale = 80.f;
    testing::BattleAccess::combat(apart);
    CHECK(apart.unit(3).state == UnitState::Steady);

    nearRight.pos = {446.f, 400.f};
    nearLeft.contacts.push_back(3);
    testing::BattleAccess::combat(apart);
    CHECK(apart.unit(3).state == UnitState::Steady);
    nearLeft.contacts.clear();
    nearLeft.knock = {1.f, 0.f};
    testing::BattleAccess::combat(apart);
    CHECK(apart.unit(3).state == UnitState::Steady);
    nearLeft.knock = {};
    nearLeft.origin = 9;
    testing::BattleAccess::combat(apart);
    CHECK(apart.unit(3).state == UnitState::Steady);
}

TEST_CASE("join holds the larger company and canSplit rejects the gates") {
    const GameData data = byeokjegwan();
    Battle battle(data.defs, data.scenario, 1);
    CHECK(canSplit(battle, 3));
    CHECK_FALSE(canSplit(battle, 0));
    Unit &horse = testing::BattleAccess::unit(battle, 3);
    horse.men = 159.9f;
    CHECK_FALSE(canSplit(battle, 3));
    horse.men = 160.f;
    CHECK(canSplit(battle, 3));
    horse.deploying = true;
    CHECK_FALSE(canSplit(battle, 3));
    horse.deploying = false;
    horse.contacts.push_back(1);
    CHECK_FALSE(canSplit(battle, 3));
    horse.contacts.clear();
    horse.men = 500.f;
    horse.pos = {400.f, 400.f};
    horse.facing = kSouth;

    Command split;
    split.type = Command::Type::Split;
    split.units = {3};
    battle.submit(split);
    battle.step();
    const UnitId other = static_cast<UnitId>(battle.units().size() - 1);
    std::vector<UnitId> both = {3, other};
    CHECK(joinGroups(battle, both).size() == 1);
    Unit &fragment = testing::BattleAccess::unit(battle, other);
    fragment.isFragment = true;
    CHECK(joinGroups(battle, both).empty());
    fragment.isFragment = false;

    Unit &anchor = testing::BattleAccess::unit(battle, other);
    Unit &mate = testing::BattleAccess::unit(battle, 3);
    mate.pos = {200.f, 200.f};
    anchor.pos = {400.f, 200.f};
    mate.men = 100.f;
    anchor.men = 300.f;
    mate.contacts.clear();
    anchor.contacts.clear();
    Command join;
    join.type = Command::Type::Join;
    join.units = both;
    battle.submit(join);
    battle.step();
    CHECK(battle.unit(3).rejoin);
    CHECK(battle.unit(other).rejoin);
    CHECK(battle.unit(other).order.type == Order::Type::Hold);
    CHECK(battle.unit(3).order.type == Order::Type::Move);
    CHECK(battle.unit(3).order.point.x == 400.f);
    CHECK(battle.unit(3).order.point.y == 200.f);

    Battle tied(data.defs, data.scenario, 1);
    Unit &tiedHorse = testing::BattleAccess::unit(tied, 3);
    tiedHorse.pos = {400.f, 400.f};
    tiedHorse.facing = kSouth;
    Command tiedSplit;
    tiedSplit.type = Command::Type::Split;
    tiedSplit.units = {3};
    tied.submit(tiedSplit);
    tied.step();
    const UnitId tiedOther = static_cast<UnitId>(tied.units().size() - 1);
    testing::BattleAccess::unit(tied, 3).men = 200.f;
    testing::BattleAccess::unit(tied, tiedOther).men = 200.f;
    testing::BattleAccess::unit(tied, 3).pos = {200.f, 200.f};
    testing::BattleAccess::unit(tied, tiedOther).pos = {400.f, 200.f};
    Command tiedJoin;
    tiedJoin.type = Command::Type::Join;
    tiedJoin.units = {3, tiedOther};
    tied.submit(tiedJoin);
    tied.step();
    CHECK(tied.unit(3).order.type == Order::Type::Hold);
    CHECK(tied.unit(tiedOther).order.type == Order::Type::Move);
}

TEST_CASE("display names cycle from one to ten") {
    Unit unit;
    CHECK(displayName(unit).suffix == NameSuffix::None);
    unit.isFragment = true;
    CHECK(displayName(unit).suffix == NameSuffix::Combined);
    unit.isFragment = false;
    unit.rejoin = true;
    CHECK(displayName(unit).suffix == NameSuffix::Combined);
    unit.rejoin = false;
    unit.isFragment = true;
    unit.fragNo = 1;
    CHECK(displayName(unit).suffix == NameSuffix::Remnant);
    CHECK(displayName(unit).numeral == 1);
    unit.fragNo = 11;
    CHECK(displayName(unit).numeral == 1);
    unit.fragNo = 10;
    CHECK(displayName(unit).numeral == 10);
    unit.isFragment = false;
    unit.fragNo = 2;
    CHECK(displayName(unit).suffix == NameSuffix::Detachment);
    CHECK(displayName(unit).numeral == 2);
}
