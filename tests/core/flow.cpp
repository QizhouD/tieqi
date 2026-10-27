#include "../BattleAccess.h"
#include "core/sim/Battle.h"
#include "data/Load.h"

#include <doctest/doctest.h>

#include <cstdint>
#include <filesystem>
#include <optional>

using namespace tq;

namespace {

std::filesystem::path sourceRoot() { return TQ_SOURCE_DIR; }

GameData byeokjegwan() { return loadGameData(sourceRoot() / "data", "byeokjegwan.json"); }

void grid(Battle &battle) {
    const float xs[] = {20.f, 270.f, 520.f, 770.f};
    const float ys[] = {20.f, 270.f, 520.f};
    int index = 0;
    for (const Unit &unit : battle.units()) {
        Unit &live = testing::BattleAccess::unit(battle, unit.id);
        live.pos = {xs[index % 4], ys[index / 4]};
        live.order = {};
        live.order.type = Order::Type::Hold;
        live.speed = 0.f;
        live.charging = false;
        live.reloadSec = 10000.f;
        live.contacts.clear();
        index += 1;
    }
}

void isolate(Battle &battle) {
    for (const Unit &unit : battle.units()) {
        Unit &live = testing::BattleAccess::unit(battle, unit.id);
        if (live.state == UnitState::Gone) {
            continue;
        }
        live.order = {};
        live.order.type = Order::Type::Hold;
        live.speed = 0.f;
        live.charging = false;
        live.reloadSec = 10000.f;
        live.contacts.clear();
    }
}

void markThreeVanguard(Battle &battle) {
    int marked = 0;
    for (const Unit &unit : battle.units()) {
        if (!unit.vanguard || marked >= 3) {
            continue;
        }
        testing::BattleAccess::unit(battle, unit.id).everBroken = true;
        marked += 1;
    }
    REQUIRE(marked == 3);
}

} // namespace

TEST_CASE("the clock warns, changes phase, and reinforces on the converted ticks") {
    const GameData data = byeokjegwan();
    Battle battle(data.defs, data.scenario, 1);
    grid(battle);
    markThreeVanguard(battle);
    std::optional<std::uint32_t> warning;
    std::optional<std::uint32_t> phase2;
    std::optional<std::uint32_t> phase3;
    std::optional<std::uint32_t> reinforcement;
    bool assaultFailed = true;
    float wingMorale = -1.f;
    for (int i = 0; i < 2150; ++i) {
        battle.step();
        if (battle.tick() == 1000) {
            CHECK(battle.units().size() == 17);
            CHECK(battle.exitOpen());
        }
        if (battle.tick() == 1900) {
            CHECK(battle.units().size() == 19);
        }
        isolate(battle);
        for (const Event &event : battle.drainEvents()) {
            if (event.type == Event::Type::ScoutWarning) {
                warning = event.tick;
            } else if (event.type == Event::Type::PhaseChanged && event.value == 2.f) {
                phase2 = event.tick;
            } else if (event.type == Event::Type::PhaseChanged && event.value == 3.f) {
                phase3 = event.tick;
                for (const Unit &unit : battle.units()) {
                    if (unit.labelKey == "unit.wing_samurai_left") {
                        wingMorale = unit.morale;
                    }
                }
            } else if (event.type == Event::Type::Reinforcement) {
                reinforcement = event.tick;
            } else if (event.type == Event::Type::GrandAssault) {
                assaultFailed = (event.flags & kAssaultFailed) != 0;
            }
        }
    }
    CHECK(warning == 800);
    CHECK(phase2 == 1000);
    CHECK(phase3 == 1900);
    CHECK(reinforcement == 2150);
    CHECK_FALSE(assaultFailed);
    CHECK(wingMorale == 92.f);
    CHECK(battle.units().size() == 21);
    CHECK(battle.phase() == 3);
    CHECK(battle.exitOpen());
}

TEST_CASE("a threatened commander brings the reinforcement early") {
    const GameData data = byeokjegwan();
    Battle battle(data.defs, data.scenario, 1);
    grid(battle);
    markThreeVanguard(battle);
    testing::BattleAccess::unit(battle, 0).men = 1.f;
    std::optional<std::uint32_t> reinforcement;
    for (int i = 0; i < 1200; ++i) {
        battle.step();
        isolate(battle);
        for (const Event &event : battle.drainEvents()) {
            if (event.type == Event::Type::Reinforcement) {
                reinforcement = event.tick;
            }
        }
    }
    CHECK(reinforcement == 1200);
    CHECK(battle.phase() == 2);
}

TEST_CASE("without a broken vanguard the schedule stays on the long clock") {
    const GameData data = byeokjegwan();
    Battle battle(data.defs, data.scenario, 1);
    grid(battle);
    std::optional<std::uint32_t> warning;
    std::optional<std::uint32_t> phase2;
    std::optional<std::uint32_t> phase3;
    bool assaultFailed = false;
    for (int i = 0; i < 2700; ++i) {
        battle.step();
        if (battle.tick() == 1800) {
            for (const Unit &unit : battle.units()) {
                if (unit.labelKey == "unit.kobayakawa") {
                    Unit &general = testing::BattleAccess::unit(battle, unit.id);
                    general.state = UnitState::Gone;
                    general.gone = GoneReason::Fled;
                }
            }
        }
        isolate(battle);
        for (const Event &event : battle.drainEvents()) {
            if (event.type == Event::Type::ScoutWarning) {
                warning = event.tick;
            } else if (event.type == Event::Type::PhaseChanged && event.value == 2.f) {
                phase2 = event.tick;
            } else if (event.type == Event::Type::PhaseChanged && event.value == 3.f) {
                phase3 = event.tick;
            } else if (event.type == Event::Type::GrandAssault) {
                assaultFailed = (event.flags & kAssaultFailed) != 0;
            }
        }
    }
    CHECK(warning == 1600);
    CHECK(phase2 == 1800);
    CHECK(phase3 == 2700);
    CHECK(assaultFailed);
}

TEST_CASE("a scenario without flow stays in phase 1") {
    GameData data = byeokjegwan();
    data.scenario.flow.enabled = false;
    Battle battle(data.defs, data.scenario, 1);
    const std::size_t before = battle.units().size();
    battle.step();
    CHECK(battle.phase() == 1);
    CHECK_FALSE(battle.exitOpen());
    bool changed = false;
    for (const Event &event : battle.drainEvents()) {
        changed = changed || (event.type == Event::Type::PhaseChanged && event.value == 1.f);
    }
    CHECK(changed);
    for (int i = 0; i < 2000; ++i) {
        battle.step();
    }
    CHECK(battle.phase() == 1);
    CHECK_FALSE(battle.exitOpen());
    CHECK(battle.units().size() == before);
    CHECK_FALSE(battle.isOver());
}

TEST_CASE("five endings and the grade boundaries") {
    const GameData data = byeokjegwan();
    const auto finish = [&](auto &&prepare) {
        Battle battle(data.defs, data.scenario, 1);
        prepare(battle);
        battle.step();
        return battle.result();
    };

    const BattleResult killed = finish([](Battle &battle) {
        Unit &commander = testing::BattleAccess::unit(battle, 0);
        commander.state = UnitState::Gone;
        commander.gone = GoneReason::GeneralKilled;
    });
    CHECK(killed.end == BattleEnd::CommanderKilled);
    CHECK(killed.grade == Grade::Defeat);

    const BattleResult withdrawn = finish([](Battle &battle) {
        Unit &commander = testing::BattleAccess::unit(battle, 0);
        commander.state = UnitState::Gone;
        commander.gone = GoneReason::Withdrawn;
    });
    CHECK(withdrawn.end == BattleEnd::Withdrawn);

    const BattleResult fled = finish([](Battle &battle) {
        Unit &commander = testing::BattleAccess::unit(battle, 0);
        commander.state = UnitState::Gone;
        commander.gone = GoneReason::Fled;
    });
    CHECK(fled.end == BattleEnd::CommanderFled);
    CHECK(fled.grade == Grade::Defeat);

    const BattleResult routed = finish([](Battle &battle) {
        for (const Unit &unit : battle.units()) {
            if (unit.side != Side::Ming) {
                continue;
            }
            Unit &live = testing::BattleAccess::unit(battle, unit.id);
            live.state = UnitState::Gone;
            live.gone = GoneReason::Dispersed;
        }
    });
    CHECK(routed.end == BattleEnd::Routed);
    CHECK(routed.grade == Grade::Defeat);

    const BattleResult swept = finish([](Battle &battle) {
        testing::BattleAccess::setPhase(battle, 3);
        for (const Unit &unit : battle.units()) {
            if (unit.side != Side::Jp) {
                continue;
            }
            Unit &live = testing::BattleAccess::unit(battle, unit.id);
            live.state = UnitState::Gone;
            live.gone = GoneReason::Fled;
        }
    });
    CHECK(swept.end == BattleEnd::Swept);
    CHECK(swept.grade == Grade::Decisive);

    float total = 0.f;
    {
        Battle battle(data.defs, data.scenario, 1);
        for (const Unit &unit : battle.units()) {
            if (unit.side == Side::Ming) {
                total += battle.typeOf(unit).men;
            }
        }
    }
    const auto graded = [&](float saved, int broken) {
        Battle battle(data.defs, data.scenario, 1);
        for (const Unit &unit : battle.units()) {
            if (unit.id == 0) {
                continue;
            }
            Unit &live = testing::BattleAccess::unit(battle, unit.id);
            live.state = UnitState::Gone;
            live.gone = GoneReason::Dispersed;
        }
        Unit &commander = testing::BattleAccess::unit(battle, 0);
        commander.state = UnitState::Gone;
        commander.gone = GoneReason::Withdrawn;
        testing::BattleAccess::stats(battle).exitedMen = saved;
        testing::BattleAccess::stats(battle).brokenJp = broken;
        battle.step();
        return battle.result();
    };
    const float decisive = data.scenario.result.decisiveRate;
    const float minor = data.scenario.result.minorRate;
    CHECK(graded(total * decisive, data.scenario.result.decisiveEnemyBroken).grade == Grade::Decisive);
    CHECK(graded(total * decisive, data.scenario.result.decisiveEnemyBroken - 1).grade == Grade::Minor);
    CHECK(graded(total * minor, 0).grade == Grade::Minor);
    CHECK(graded(total * minor - 1.f, 0).grade == Grade::Pyrrhic);
}
