#include "app/Orders.h"
#include "app/Report.h"
#include "core/sim/Battle.h"
#include "data/Load.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <string>

using namespace tq;

namespace {

GameData loadByeok() { return loadGameData(std::filesystem::path(TQ_SOURCE_DIR) / "data", "byeokjegwan.json"); }

PointerSample at(PointerSample::Type type, Vec2 logical) {
    PointerSample sample;
    sample.type = type;
    sample.x = logical.x;
    sample.y = logical.y;
    return sample;
}

void click(OrderState &state, const Battle &battle, Vec2 world, float mapW, float mapH) {
    const Vec2 logical = logicalFromWorld(world.x, world.y, mapW, mapH);
    applyPointer(state, battle, at(PointerSample::Type::Down, logical), mapW, mapH, 0.f);
    applyPointer(state, battle, at(PointerSample::Type::Up, logical), mapW, mapH, 0.f);
}

} // namespace

TEST_CASE("every unit type has a description in zh-Hant, en, and ko") {
    const GameData data = loadByeok();
    for (const UnitType &type : data.defs.types) {
        const std::string key = "desc.type." + type.id;
        CHECK_MESSAGE(data.zhHant.entries.count(key) == 1, key);
    }
    // The en and ko tables own every zh-Hant key (tests/app/l10n.cpp), so the descriptions exist there too.
}

TEST_CASE("the unit card shows the selected unit with its description, or an inspected enemy without orders") {
    const GameData data = loadByeok();
    BattleOptions options;
    options.aiEnabled = false;
    Battle battle(data.defs, data.scenario, 1, options);
    battle.step();
    const float mapW = data.scenario.map.width;
    const float mapH = data.scenario.map.height;
    const UnitId left = *battle.sceneUnit("jiading_left");
    const UnitId enemy = *battle.sceneUnit("van_teppo_left");

    OrderState state;
    click(state, battle, battle.unit(enemy).pos, mapW, mapH);
    REQUIRE(state.inspected.has_value());
    CHECK(*state.inspected == enemy);
    CHECK(state.selected.empty());
    const RosterCard inspected = rosterCard(data.zhHant, battle, state);
    CHECK(inspected.mode == RosterCard::Mode::One);
    CHECK(inspected.unit == enemy);
    CHECK(inspected.typeId == "teppo");
    CHECK_FALSE(inspected.showOrders);
    CHECK(inspected.description == data.zhHant.text("desc.type.teppo"));

    click(state, battle, battle.unit(left).pos, mapW, mapH);
    CHECK_FALSE(state.inspected.has_value());
    const RosterCard own = rosterCard(data.zhHant, battle, state);
    CHECK(own.unit == left);
    CHECK(own.showOrders);
    CHECK(own.description == data.zhHant.text("desc.type.jiading"));

    OrderState idle;
    click(idle, battle, battle.unit(enemy).pos, mapW, mapH);
    click(idle, battle, {440.f, 150.f}, mapW, mapH);
    CHECK_FALSE(idle.inspected.has_value());
}
