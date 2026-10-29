#include "app/dev/Intent.h"
#include "../../tools/headless/Run.h"
#include "core/sim/Battle.h"
#include "data/Load.h"
#include "data/Replay.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <numbers>

using namespace tq;

namespace {

std::filesystem::path sourceRoot() { return TQ_SOURCE_DIR; }

GameData byeokjegwan() { return loadGameData(sourceRoot() / "data", "byeokjegwan.json"); }

std::uint16_t typeIndex(const GameDefs &defs, const std::string &id) {
    for (std::size_t index = 0; index < defs.types.size(); ++index) {
        if (defs.types[index].id == id) {
            return static_cast<std::uint16_t>(index);
        }
    }
    FAIL("unknown type " << id);
    return 0;
}

Command spawn(std::uint16_t type, Vec2 point, float face) {
    Command command;
    command.type = Command::Type::Spawn;
    command.unitType = type;
    command.point = point;
    command.face = face;
    return command;
}

Command rout(UnitId id) {
    Command command;
    command.type = Command::Type::Rout;
    command.units = {id};
    return command;
}

} // namespace

TEST_CASE("a spawn command adds a holding unit of that type on the next tick") {
    const GameData data = byeokjegwan();
    BattleOptions options;
    options.aiEnabled = false;
    Battle battle(data.defs, data.scenario, 1, options);
    const std::size_t before = battle.units().size();
    const float jpBefore = battle.enlisted(Side::Jp);
    const std::uint16_t samurai = typeIndex(data.defs, "samurai");
    battle.submit(spawn(samurai, {300.f, 400.f}, std::numbers::pi_v<float> / 2.f));
    CHECK(battle.units().size() == before);
    battle.step();
    REQUIRE(battle.units().size() == before + 1);
    const UnitId newest = static_cast<UnitId>(battle.units().size() - 1);
    const Unit &unit = battle.unit(newest);
    CHECK(unit.id == newest);
    CHECK(newest == static_cast<UnitId>(before));
    CHECK(unit.type == samurai);
    CHECK(unit.side == Side::Jp);
    CHECK(unit.labelKey == data.defs.types[samurai].nameKey);
    CHECK(unit.men == data.defs.types[samurai].men);
    CHECK(unit.order.type == Order::Type::Hold);
    CHECK(unit.state == UnitState::Steady);
    CHECK(battle.enlisted(Side::Jp) == doctest::Approx(jpBefore + data.defs.types[samurai].men));
    CHECK(dev::unitLabel(battle, unit.id) == "#" + std::to_string(unit.id));
}

TEST_CASE("a spawn point outside the map is clamped and an unknown type is ignored") {
    const GameData data = byeokjegwan();
    BattleOptions options;
    options.aiEnabled = false;
    Battle battle(data.defs, data.scenario, 1, options);
    const std::size_t before = battle.units().size();
    battle.submit(spawn(typeIndex(data.defs, "liaodong"), {-500.f, 1e6f}, 0.f));
    battle.submit(spawn(static_cast<std::uint16_t>(data.defs.types.size()), {300.f, 300.f}, 0.f));
    battle.step();
    REQUIRE(battle.units().size() == before + 1);
    const UnitId newest = static_cast<UnitId>(battle.units().size() - 1);
    const Unit &unit = battle.unit(newest);
    CHECK(unit.side == Side::Ming);
    CHECK(unit.pos.x == doctest::Approx(12.f));
    CHECK(unit.pos.y == doctest::Approx(data.scenario.map.height - 12.f));
}

TEST_CASE("a rout command breaks a fighting unit of either side and leaves others alone") {
    const GameData data = byeokjegwan();
    BattleOptions options;
    options.aiEnabled = false;
    Battle battle(data.defs, data.scenario, 1, options);
    const std::optional<UnitId> samurai = battle.sceneUnit("van_samurai");
    const std::optional<UnitId> riders = battle.sceneUnit("jiading_right");
    REQUIRE(samurai);
    REQUIRE(riders);
    battle.drainEvents();
    battle.submit(rout(*samurai));
    battle.submit(rout(*riders));
    battle.step();
    CHECK(battle.unit(*samurai).state == UnitState::Broken);
    CHECK(battle.unit(*riders).state == UnitState::Broken);
    const std::vector<Event> events = battle.drainEvents();
    const auto broken = std::count_if(events.begin(), events.end(),
                                      [](const Event &event) { return event.type == Event::Type::Broken; });
    CHECK(broken >= 2);
    CHECK(battle.stats().brokenJp >= 1);
    CHECK(battle.stats().brokenMing >= 1);

    const float brokenSec = battle.unit(*samurai).brokenSec;
    battle.submit(rout(*samurai));
    battle.step();
    CHECK(battle.unit(*samurai).brokenSec < brokenSec);
}

TEST_CASE("dev commands survive a replay round trip and replay to the same hash") {
    const GameData data = byeokjegwan();
    BattleOptions options;
    options.aiEnabled = true;
    Battle live(data.defs, data.scenario, 7, options);
    std::vector<ReplayCommand> recorded;
    const auto issue = [&](Command command) {
        ReplayCommand entry = recordCommand(live, data.defs, command);
        entry.tick = live.tick() + 1;
        recorded.push_back(entry);
        live.submit(std::move(command));
    };
    issue(spawn(typeIndex(data.defs, "teppo"), {500.f, 500.f}, 1.25f));
    live.step();
    const UnitId spawned = static_cast<UnitId>(live.units().size() - 1);
    for (int i = 0; i < 40; ++i) {
        live.step();
    }
    issue(rout(spawned));
    issue(rout(*live.sceneUnit("liaodong_right")));
    for (int i = 0; i < 200; ++i) {
        live.step();
    }

    Replay replay;
    replay.build = buildId();
    replay.scenario = data.scenario.id;
    replay.seed = 7;
    replay.ai = true;
    replay.commands = recorded;
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "tieqi-dev-replay-test.json";
    saveReplay(path, replay);
    const Replay loaded = loadReplay(path);
    std::filesystem::remove(path);
    REQUIRE(loaded.commands.size() == 3u);
    CHECK(loaded.commands[0].type == Command::Type::Spawn);
    CHECK(loaded.commands[0].unitType == "teppo");
    REQUIRE(loaded.commands[0].facing);
    CHECK(*loaded.commands[0].facing == doctest::Approx(1.25f));
    CHECK(loaded.commands[1].type == Command::Type::Rout);
    CHECK(loaded.commands[1].units == std::vector<std::string>{"#" + std::to_string(spawned)});
    CHECK(loaded.commands[2].units == std::vector<std::string>{"liaodong_right"});

    Battle again(data.defs, data.scenario, 7, options);
    std::size_t cursor = 0;
    while (again.tick() < live.tick()) {
        while (cursor < loaded.commands.size() && loaded.commands[cursor].tick == again.tick() + 1) {
            const std::optional<Command> command = resolveCommand(again, data.defs, loaded.commands[cursor]);
            REQUIRE(command);
            again.submit(*command);
            cursor += 1;
        }
        again.step();
    }
    CHECK(cursor == loaded.commands.size());
    CHECK(again.stateHash() == live.stateHash());
}

TEST_CASE("a spawn without a unit type is a replay error") {
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "tieqi-dev-bad-replay.json";
    {
        std::ofstream out(path);
        out << R"({"schema": 1, "build": "x", "scenario": "byeokjegwan", "seed": 1, "ai": true,
                  "commands": [{"tick": 1, "type": "spawn", "units": [], "point": [1, 2]}]})";
    }
    CHECK_THROWS_AS(loadReplay(path), DataError);
    std::filesystem::remove(path);
}

TEST_CASE("scripted runs are unchanged by the dev command plumbing") {
    // The headless tool now resolves replay commands through resolveCommand; a recorded scripted run must still
    // replay to the same hash.
    const GameData data = byeokjegwan();
    RunRequest request;
    request.policy = PolicyKind::Scripted;
    request.seed = 3;
    const RunResult live = runBattle(data, request);
    RunRequest again = request;
    again.replay = true;
    again.commands = live.replay.commands;
    CHECK(runBattle(data, again).hash == live.hash);
}

TEST_CASE("AI intents list fighting Japanese units with their scores") {
    const GameData data = byeokjegwan();
    Battle battle(data.defs, data.scenario, 1);
    for (int i = 0; i < 30; ++i) {
        battle.step();
    }
    const std::vector<dev::AiIntent> intents = dev::aiIntents(battle, data.defs);
    REQUIRE_FALSE(intents.empty());
    UnitId last = 0;
    bool first = true;
    for (const dev::AiIntent &intent : intents) {
        const Unit &unit = battle.unit(intent.id);
        CHECK(unit.side == Side::Jp);
        CHECK((unit.state == UnitState::Steady || unit.state == UnitState::Wavering));
        CHECK((first || intent.id > last));
        first = false;
        last = intent.id;
        CHECK(intent.role == battle.typeOf(unit).ai);
        CHECK(intent.best != kNoUnit);
        CHECK(battle.unit(intent.best).side == Side::Ming);
        if (intent.score) {
            CHECK(*intent.score <= intent.bestScore);
        }
        if (unit.vanguard) {
            CHECK(intent.aggressive == battle.vanguardAggressive());
        } else {
            CHECK(intent.aggressive);
        }
    }
    CHECK_FALSE(battle.vanguardAggressive());
}

TEST_CASE("the unit inspector lists every field in order") {
    const GameData data = byeokjegwan();
    Battle battle(data.defs, data.scenario, 1);
    const auto fields = dev::unitFields(battle, 0);
    REQUIRE(fields.size() > 30u);
    CHECK(fields.front().first == "id");
    CHECK(fields.front().second == "0");
    CHECK(fields[2].first == "type");
    CHECK(fields[2].second == data.defs.types[battle.unit(0).type].id);
    CHECK(dev::unitFields(battle, static_cast<UnitId>(battle.units().size())).empty());
}

TEST_CASE("the timing window keeps the latest samples") {
    dev::TimeSeries series;
    CHECK(series.mean() == 0.f);
    for (std::size_t i = 0; i < dev::TimeSeries::kSize + 10; ++i) {
        series.push(static_cast<float>(i));
    }
    CHECK(series.count() == static_cast<int>(dev::TimeSeries::kSize));
    CHECK(series.max() == static_cast<float>(dev::TimeSeries::kSize + 9));
    CHECK(series.offset() == 10);
    CHECK(series.mean() == doctest::Approx(10.f + (dev::TimeSeries::kSize - 1) / 2.f));
}
