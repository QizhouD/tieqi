#include "app/Soldiers.h"
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

GameData loadByeok() { return loadGameData(std::filesystem::path(TQ_SOURCE_DIR) / "data", "byeokjegwan.json"); }

Battle makeBattle(const GameData &data) {
    BattleOptions options;
    options.aiEnabled = false;
    return Battle(data.defs, data.scenario, 1, options);
}

const Unit &named(const Battle &battle, const char *id) { return battle.unit(*battle.sceneUnit(id)); }

} // namespace

TEST_CASE("one soldier stands for 10 cavalry or 20 infantry, at most 60") {
    const GameData data = loadByeok();
    const Battle battle = makeBattle(data);
    const UnitType &jiading = battle.typeOf(named(battle, "jiading_left"));
    const UnitType &teppo = battle.typeOf(named(battle, "van_teppo_left"));
    CHECK(soldierKind(jiading) == SoldierKind::Cavalry);
    CHECK(soldierKind(teppo) == SoldierKind::Gun);
    CHECK(soldierKind(battle.typeOf(named(battle, "van_yari"))) == SoldierKind::Spear);
    CHECK(soldierKind(battle.typeOf(named(battle, "van_samurai"))) == SoldierKind::Samurai);
    CHECK(soldierCount(jiading, 400.f) == 40);
    CHECK(soldierCount(jiading, 391.f) == 40);
    CHECK(soldierCount(jiading, 2000.f) == kMaxSoldiersPerUnit);
    CHECK(soldierCount(teppo, 500.f) == 25);
    CHECK(soldierCount(teppo, 3.f) == 1);
    CHECK(soldierCount(teppo, 0.f) == 0);
}

TEST_CASE("soldiers stand in ranks inside the formation and losses come off the rear") {
    const GameData data = loadByeok();
    const Battle battle = makeBattle(data);
    Unit unit = named(battle, "jiading_left");
    const UnitType &type = battle.typeOf(unit);
    unit.prevPos = unit.pos;
    unit.prevFacing = unit.facing;

    std::vector<Soldier> full;
    layoutSoldiers(unit, type, 1.f, 0.f, full);
    REQUIRE(full.size() == 40);
    const SoldierGrid grid = soldierGrid(type, unit);
    CHECK(grid.files * grid.ranks >= 40);
    for (const Soldier &soldier : full) {
        const Vec2 d = soldier.pos - unit.pos;
        const float along = d.x * std::cos(unit.facing) + d.y * std::sin(unit.facing);
        const float across = -d.x * std::sin(unit.facing) + d.y * std::cos(unit.facing);
        CHECK(std::abs(along) <= unit.depth * 0.5f + 0.01f);
        CHECK(std::abs(across) <= unit.frontage * 0.5f + 0.01f);
    }

    Unit hurt = unit;
    hurt.men = 300.f;
    std::vector<Soldier> fewer;
    layoutSoldiers(hurt, type, 1.f, 0.f, fewer);
    REQUIRE(fewer.size() == 30);
    for (std::size_t i = 0; i < fewer.size(); ++i) {
        CHECK(fewer[i].pos.x == full[i].pos.x);
        CHECK(fewer[i].pos.y == full[i].pos.y);
    }
    // Slot order runs from the front rank back, so the dropped slots are the rearmost.
    const Vec2 front = soldierSlot(grid, 0);
    const Vec2 back = soldierSlot(grid, 39);
    CHECK(front.x > back.x);
}

TEST_CASE("soldier jitter is fixed per unit id") {
    const GameData data = loadByeok();
    const Battle battle = makeBattle(data);
    Unit unit = named(battle, "liaodong_center");
    const UnitType &type = battle.typeOf(unit);
    std::vector<Soldier> first;
    std::vector<Soldier> second;
    layoutSoldiers(unit, type, 0.5f, 3.25f, first);
    layoutSoldiers(unit, type, 0.5f, 3.25f, second);
    REQUIRE(first.size() == second.size());
    for (std::size_t i = 0; i < first.size(); ++i) {
        CHECK(first[i].pos.x == second[i].pos.x);
        CHECK(first[i].pos.y == second[i].pos.y);
        CHECK(first[i].frame == second[i].frame);
    }
    CHECK(soldierNoise(unit.id, 3, 1) == soldierNoise(unit.id, 3, 1));
    CHECK(soldierNoise(unit.id, 3, 1) != soldierNoise(unit.id + 1, 3, 1));
    for (int i = 0; i < 200; ++i) {
        const float value = soldierNoise(7, i, 2);
        CHECK(value >= 0.f);
        CHECK(value < 1.f);
    }
}

TEST_CASE("animation follows the unit state") {
    const GameData data = loadByeok();
    const Battle battle = makeBattle(data);
    Unit unit = named(battle, "jiading_left");
    CHECK(soldierAnim(unit) == SoldierAnim::Idle);
    unit.speed = 3.f;
    CHECK(soldierAnim(unit) == SoldierAnim::March);
    unit.charging = true;
    CHECK(soldierAnim(unit) == SoldierAnim::Charge);
    unit.contacts.push_back(7);
    CHECK(soldierAnim(unit) == SoldierAnim::Engaged);
    unit.contacts.clear();
    unit.charging = false;
    unit.speed = 0.f;
    unit.state = UnitState::Wavering;
    CHECK(soldierAnim(unit) == SoldierAnim::Waver);
    unit.state = UnitState::Broken;
    CHECK(soldierAnim(unit) == SoldierAnim::Turn);
    unit.state = UnitState::Routing;
    CHECK(soldierAnim(unit) == SoldierAnim::Rout);

    CHECK(soldierDirection(0.f) == 0);
    CHECK(soldierDirection(std::numbers::pi_v<float> * 0.5f) == 2);
    CHECK(soldierDirection(std::numbers::pi_v<float>) == 4);
    CHECK(soldierDirection(-std::numbers::pi_v<float> * 0.5f) == 6);
    CHECK(soldierDirection(-std::numbers::pi_v<float> * 0.25f) == 7);
}

TEST_CASE("routing soldiers scatter and charging riders raise dust") {
    const GameData data = loadByeok();
    const Battle battle = makeBattle(data);
    Unit unit = named(battle, "liaodong_center");
    const UnitType &type = battle.typeOf(unit);
    unit.prevPos = unit.pos;
    unit.prevFacing = unit.facing;
    const auto spread = [&](const std::vector<Soldier> &soldiers) {
        float total = 0.f;
        for (const Soldier &soldier : soldiers) {
            const Vec2 d = soldier.pos - unit.pos;
            total += std::hypot(d.x, d.y);
        }
        return total / static_cast<float>(soldiers.size());
    };
    std::vector<Soldier> steady;
    layoutSoldiers(unit, type, 1.f, 0.f, steady);
    unit.state = UnitState::Routing;
    unit.routSec = 30.f;
    std::vector<Soldier> routed;
    layoutSoldiers(unit, type, 1.f, 0.f, routed);
    CHECK(spread(routed) > spread(steady) * 1.5f);

    unit.state = UnitState::Steady;
    unit.charging = true;
    unit.speed = 8.f;
    std::vector<Soldier> charging;
    layoutSoldiers(unit, type, 1.f, 1.f, charging);
    int dusty = 0;
    for (const Soldier &soldier : charging) {
        dusty += soldier.dust > 0.f ? 1 : 0;
        CHECK(soldier.frame >= kFrameMove);
        CHECK(soldier.frame < kFrameMove + kMoveFrames);
    }
    CHECK(dusty > 0);
}

TEST_CASE("drawing soldiers every tick leaves the battle unchanged") {
    const GameData data = loadByeok();
    Battle plain = makeBattle(data);
    Battle watched = makeBattle(data);
    std::vector<Soldier> soldiers;
    for (int tick = 0; tick < 400; ++tick) {
        plain.step();
        watched.step();
        soldiers.clear();
        for (const Unit &unit : watched.units()) {
            layoutSoldiers(unit, watched.typeOf(unit), 0.5f, static_cast<float>(tick) * 0.1f, soldiers);
        }
        REQUIRE(plain.stateHash() == watched.stateHash());
    }
    CHECK_FALSE(soldiers.empty());
}

TEST_CASE("banners: Ming units carry one, Japanese crests follow the source unit, broken units drop theirs") {
    const GameData data = loadByeok();
    const Battle battle = makeBattle(data);
    const Unit &li = named(battle, "li_rusong");
    CHECK(bannerKind(battle, li) == BannerKind::Ming);
    CHECK(bannerKind(battle, named(battle, "van_teppo_left")) == BannerKind::JpVanguard);

    Unit fragment = named(battle, "van_yari");
    fragment.vanguard = false;
    fragment.origin = named(battle, "van_yari").id;
    CHECK(bannerKind(battle, fragment) == BannerKind::JpVanguard);
    fragment.origin = kNoUnit;
    CHECK(bannerKind(battle, fragment) == BannerKind::JpMain);

    Unit broken = li;
    broken.state = UnitState::Broken;
    CHECK(bannerKind(battle, broken) == BannerKind::None);
    CHECK(unitBanner(battle, broken, 1.f, 0.f).kind == BannerKind::None);

    Unit still = li;
    still.prevPos = still.pos;
    still.prevFacing = still.facing;
    const Banner banner = unitBanner(battle, still, 1.f, 0.f);
    const Vec2 offset = banner.base - still.pos;
    CHECK(std::hypot(offset.x, offset.y) == doctest::Approx(still.depth * 0.3f));
}

TEST_CASE("every Ming unit type has a banner character in every language") {
    const GameData data = loadByeok();
    for (const UnitType &type : data.defs.types) {
        if (type.side != Side::Ming) {
            continue;
        }
        const std::string key = "glyph.flag." + type.id;
        CHECK_MESSAGE(data.zhHant.entries.count(key) == 1, key);
    }
    CHECK(data.zhHant.entries.count("glyph.flag.yang_left") == 1);
    CHECK(data.zhHant.entries.count("glyph.flag.yang_right") == 1);
}
