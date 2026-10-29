#include "data/Replay.h"
#include "../../tools/headless/Run.h"
#include "core/sim/Battle.h"
#include "data/Load.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>

using namespace tq;

namespace {

std::filesystem::path sourceRoot() { return TQ_SOURCE_DIR; }

GameData byeokjegwan() { return loadGameData(sourceRoot() / "data", "byeokjegwan.json"); }

bool sameEvent(const Event &left, const Event &right) {
    return left.type == right.type && left.tick == right.tick && left.a == right.a && left.b == right.b &&
           left.pos.x == right.pos.x && left.pos.y == right.pos.y && left.value == right.value &&
           left.side == right.side && left.flags == right.flags;
}

} // namespace

TEST_CASE("the same seed and commands hash identically for 3000 ticks") {
    const GameData data = byeokjegwan();
    BattleOptions options;
    options.aiEnabled = false;
    Battle left(data.defs, data.scenario, 1, options);
    Battle right(data.defs, data.scenario, 1, options);
    Command command;
    command.type = Command::Type::Move;
    command.units = {1};
    command.point = {400.f, 600.f};
    left.submit(command);
    right.submit(command);
    for (int i = 0; i < 3000; ++i) {
        CHECK(left.stateHash() == right.stateHash());
        left.step();
        right.step();
    }
    CHECK(left.stateHash() == right.stateHash());

    Battle other(data.defs, data.scenario, 2, options);
    Battle baseline(data.defs, data.scenario, 1, options);
    bool differed = other.stateHash() != baseline.stateHash();
    for (int i = 0; i < 64 && !differed; ++i) {
        other.step();
        baseline.step();
        differed = other.stateHash() != baseline.stateHash();
    }
    CHECK(differed);
}

TEST_CASE("two scripted runs emit the same events") {
    const GameData data = byeokjegwan();
    RunRequest request;
    request.policy = PolicyKind::Scripted;
    request.seed = 1;
    const RunResult first = runBattle(data, request);
    const RunResult second = runBattle(data, request);
    CHECK_FALSE(first.timeout);
    CHECK(first.finite);
    CHECK(first.impacts > 0);
    CHECK(first.hash == second.hash);
    CHECK(first.endKind == second.endKind);
    CHECK(first.grade == second.grade);
    REQUIRE(first.log.size() == second.log.size());
    for (std::size_t i = 0; i < first.log.size(); ++i) {
        CHECK(sameEvent(first.log[i], second.log[i]));
    }
}

TEST_CASE("a scripted replay round-trips to the same result") {
    const GameData data = byeokjegwan();
    RunRequest request;
    request.policy = PolicyKind::Scripted;
    request.seed = 1;
    const RunResult live = runBattle(data, request);
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "tieqi-replay-test.json";
    saveReplay(path, live.replay);
    const Replay loaded = loadReplay(path);
    std::filesystem::remove(path);
    CHECK(loaded.build == buildId());
    CHECK(loaded.scenario == "byeokjegwan");
    CHECK(loaded.seed == 1);
    CHECK(loaded.ai);
    CHECK(loaded.commands.size() == live.replay.commands.size());

    RunRequest again = request;
    again.replay = true;
    again.commands = loaded.commands;
    const RunResult replayed = runBattle(data, again);
    CHECK(replayed.hash == live.hash);
    CHECK(replayed.endKind == live.endKind);
    CHECK(replayed.grade == live.grade);
    CHECK(replayed.savedRate == live.savedRate);
    REQUIRE(replayed.log.size() == live.log.size());
    for (std::size_t i = 0; i < live.log.size(); ++i) {
        CHECK(sameEvent(replayed.log[i], live.log[i]));
    }
}

TEST_CASE("replays carry the game version and older ones still load") {
    CHECK(gameVersion() == TQ_PROJECT_VERSION);
    Replay replay;
    CHECK(replay.version == gameVersion());
    replay.build = buildId();
    replay.scenario = "byeokjegwan";
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "tieqi-replay-version.json";
    saveReplay(path, replay);
    CHECK(loadReplay(path).version == gameVersion());

    {
        std::ofstream out(path);
        out << R"({"schema": 1, "build": "x", "scenario": "byeokjegwan", "seed": 1, "ai": true, "commands": []})";
    }
    CHECK(loadReplay(path).version.empty());
    {
        std::ofstream out(path);
        out << R"({"schema": 1, "version": 3, "build": "x", "scenario": "byeokjegwan", "seed": 1, "ai": true,
                   "commands": []})";
    }
    CHECK_THROWS_AS(loadReplay(path), DataError);
    std::filesystem::remove(path);
}
