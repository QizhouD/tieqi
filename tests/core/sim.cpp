#include "core/sim/Battle.h"
#include "core/sim/Rng.h"
#include "data/Load.h"

#include <doctest/doctest.h>

#include <cmath>
#include <cstdint>
#include <filesystem>

using namespace tq;

namespace {

std::filesystem::path sourceRoot() { return TQ_SOURCE_DIR; }

GameData byeokjegwan() { return loadGameData(sourceRoot() / "data", "byeokjegwan.json"); }

Battle makeBattle(const GameData &data, std::uint64_t seed) { return Battle(data.defs, data.scenario, seed); }

void submit(Battle &battle, Command::Type type, std::vector<UnitId> units, Vec2 point = {}, UnitId target = kNoUnit) {
    Command cmd;
    cmd.type = type;
    cmd.units = std::move(units);
    cmd.point = point;
    cmd.target = target;
    battle.submit(cmd);
}

bool near(float actual, float expected) { return std::fabs(actual - expected) < 0.001f; }

} // namespace

namespace tq::testing {

class BattleAccess {
  public:
    static Unit &unit(Battle &battle, UnitId id) { return battle.units_.at(id); }
};

} // namespace tq::testing

TEST_CASE("rng is deterministic and stays below one") {
    Rng first(1);
    Rng second(1);
    CHECK(first.next() == second.next());
    const float sample = first.uniform();
    CHECK(sample >= 0.f);
    CHECK(sample < 1.f);
    Rng third(1);
    third.next();
    CHECK(third.uniform() == sample);
}

TEST_CASE("move reaches the point and then holds") {
    const GameData data = byeokjegwan();
    Battle battle = makeBattle(data, 1);
    const Vec2 start = battle.unit(1).pos;
    battle.step();
    CHECK(battle.unit(1).pos.x == start.x);
    CHECK(battle.unit(1).pos.y == start.y);

    submit(battle, Command::Type::Move, {1}, {400.f, 600.f});
    bool arrived = false;
    for (int i = 0; i < 400; ++i) {
        battle.step();
        const Unit &unit = battle.unit(1);
        if (unit.order.type == Order::Type::Hold) {
            const float dx = unit.pos.x - 400.f;
            const float dy = unit.pos.y - 600.f;
            CHECK(std::sqrt(dx * dx + dy * dy) < 5.f);
            CHECK(unit.speed == 0.f);
            arrived = true;
            break;
        }
    }
    CHECK(arrived);
}

TEST_CASE("cavalry turn rate and alignment follow the spec") {
    const GameData data = byeokjegwan();
    Battle battle = makeBattle(data, 1);
    const Unit &before = battle.unit(1);
    const float facing0 = before.facing;
    const Vec2 pos0 = before.pos;
    CHECK(battle.terrainAt(pos0) == Terrain::Open);
    submit(battle, Command::Type::Move, {1}, {700.f, 330.f});
    battle.step();
    const Unit &unit = battle.unit(1);

    const float facing1 = facing0 - 0.225f * 0.8f;
    const float diff = std::fabs(facing1);
    CHECK(diff > 0.6f);
    CHECK(diff < 1.4f);
    CHECK(near(unit.facing, facing1));
    CHECK(near(unit.speed, 0.9375f * 0.8f));
    const float travel = (0.9375f * 0.8f) * 0.65f * 0.8f;
    CHECK(near(unit.pos.x, pos0.x + std::cos(facing1) * travel));
    CHECK(near(unit.pos.y, pos0.y + std::sin(facing1) * travel));
    CHECK(unit.prevPos.x == pos0.x);
    CHECK(unit.prevPos.y == pos0.y);
}

TEST_CASE("infantry turn rate and alignment follow the spec") {
    const GameData data = byeokjegwan();
    Battle battle = makeBattle(data, 1);
    Unit &ordered = testing::BattleAccess::unit(battle, 7);
    const float facing0 = ordered.facing;
    const Vec2 pos0 = ordered.pos;
    CHECK(battle.terrainAt(pos0) == Terrain::Open);
    ordered.order.type = Order::Type::Move;
    ordered.order.point = {520.f, 660.f};
    battle.step();
    const Unit &unit = battle.unit(7);

    const float facing1 = facing0 + 0.1125f * 0.8f;
    const float diff = std::fabs(facing1);
    CHECK(diff > 1.2f);
    CHECK(near(unit.facing, facing1));
    const float speed = 0.390625f * 0.8f;
    CHECK(near(unit.speed, speed));
    const float travel = speed * 0.12f * 0.8f;
    CHECK(near(unit.pos.x, pos0.x + std::cos(facing1) * travel));
    CHECK(near(unit.pos.y, pos0.y + std::sin(facing1) * travel));
}

TEST_CASE("multi-select move keeps clamped offsets") {
    const GameData data = byeokjegwan();
    Battle battle = makeBattle(data, 1);
    submit(battle, Command::Type::Move, {3, 5}, {0.f, 100.f});
    battle.step();
    CHECK(battle.unit(3).order.type == Order::Type::Move);
    CHECK(battle.unit(3).order.point.x == 12.f);
    CHECK(battle.unit(3).order.point.y == 100.f);
    CHECK(battle.unit(5).order.point.x == 140.f);
    CHECK(battle.unit(5).order.point.y == 100.f);
    REQUIRE(battle.unit(3).order.face.has_value());
    REQUIRE(battle.unit(5).order.face.has_value());
    CHECK(near(*battle.unit(3).order.face, *battle.unit(5).order.face));
}

TEST_CASE("retreat attack and ignored commands") {
    const GameData data = byeokjegwan();
    Battle battle = makeBattle(data, 1);
    submit(battle, Command::Type::Retreat, {0});
    battle.step();
    CHECK(battle.unit(0).order.type == Order::Type::Move);
    CHECK(battle.unit(0).order.point.x == 440.f);
    CHECK(battle.unit(0).order.point.y == 60.f);

    Battle attack = makeBattle(data, 1);
    submit(attack, Command::Type::Attack, {1}, {}, 6);
    attack.step();
    CHECK(attack.unit(1).order.type == Order::Type::Attack);
    CHECK(attack.unit(1).order.target == 6);

    Battle ignored = makeBattle(data, 1);
    submit(ignored, Command::Type::Attack, {1}, {}, 0);
    submit(ignored, Command::Type::Move, {6}, {100.f, 100.f});
    ignored.step();
    CHECK(ignored.unit(1).order.type == Order::Type::Hold);
    CHECK(ignored.unit(6).order.type == Order::Type::Hold);
}

TEST_CASE("same seed matches for 3000 ticks and another seed differs") {
    const GameData data = byeokjegwan();
    Battle left = makeBattle(data, 1);
    Battle right = makeBattle(data, 1);
    submit(left, Command::Type::Move, {1}, {400.f, 600.f});
    submit(right, Command::Type::Move, {1}, {400.f, 600.f});
    for (int i = 0; i < 3000; ++i) {
        CHECK(left.stateHash() == right.stateHash());
        left.step();
        right.step();
    }
    CHECK(left.stateHash() == right.stateHash());

    Battle other = makeBattle(data, 2);
    bool differed = other.stateHash() != makeBattle(data, 1).stateHash();
    Battle baseline = makeBattle(data, 1);
    for (int i = 0; i < 64 && !differed; ++i) {
        other.step();
        baseline.step();
        differed = other.stateHash() != baseline.stateHash();
    }
    CHECK(differed);
}
