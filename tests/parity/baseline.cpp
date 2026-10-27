#include "../../tools/headless/Run.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

using namespace tq;

namespace {

std::filesystem::path sourceRoot() { return TQ_SOURCE_DIR; }

float median(std::vector<float> values) {
    std::sort(values.begin(), values.end());
    const std::size_t count = values.size();
    if (count == 0) {
        return 0.f;
    }
    if (count % 2 == 1) {
        return values[count / 2];
    }
    return (values[count / 2 - 1] + values[count / 2]) * 0.5f;
}

bool phase2Allowed(const RunResult &result) {
    if (!result.phase2Tick) {
        return true;
    }
    const int tick = static_cast<int>(*result.phase2Tick);
    return std::abs(tick - 1000) <= 2 || std::abs(tick - 1800) <= 2;
}

bool isWin(const RunResult &result) {
    return result.grade == "decisive" || result.grade == "minor" || result.grade == "pyrrhic";
}

} // namespace

TEST_CASE("slow full-battle baseline stays inside the published bands") {
    const GameData data = loadGameData(sourceRoot() / "data", "byeokjegwan.json");
    std::vector<RunResult> passive;
    std::vector<RunResult> scripted;
    passive.reserve(30);
    scripted.reserve(30);
    for (std::uint64_t seed = 1; seed <= 30; ++seed) {
        RunRequest request;
        request.seed = seed;
        request.policy = PolicyKind::Passive;
        passive.push_back(runBattle(data, request));
        request.policy = PolicyKind::Scripted;
        scripted.push_back(runBattle(data, request));
    }

    int passiveDefeats = 0;
    int commanderKilled = 0;
    int endedInTime = 0;
    std::vector<float> passiveEnds;
    for (const RunResult &result : passive) {
        INFO("passive seed end " << result.endKind << " tick " << result.endTick << " grade " << result.grade);
        passiveDefeats += result.grade == "defeat" ? 1 : 0;
        commanderKilled += result.endKind == "commander_killed" ? 1 : 0;
        endedInTime += !result.timeout && result.endTick <= 4000 ? 1 : 0;
        passiveEnds.push_back(static_cast<float>(result.endTick));
        CHECK(phase2Allowed(result));
    }
    const float passiveMedian = median(passiveEnds);
    INFO("passive median end tick " << passiveMedian);
    CHECK(passiveDefeats == 30);
    CHECK(commanderKilled == 30);
    CHECK(endedInTime == 30);
    CHECK(passiveMedian >= 1300.f);
    CHECK(passiveMedian <= 2000.f);

    int wins = 0;
    std::vector<float> broken;
    std::vector<float> impacts;
    for (const RunResult &result : scripted) {
        INFO("scripted end " << result.endKind << " grade " << result.grade << " tick " << result.endTick
                             << " reinforce " << (result.reinforceTick ? std::to_string(*result.reinforceTick) : "none")
                             << " broken " << result.jpBroken << " impacts " << result.impacts);
        wins += isWin(result) ? 1 : 0;
        broken.push_back(static_cast<float>(result.jpBroken));
        impacts.push_back(static_cast<float>(result.impacts));
        CHECK(phase2Allowed(result));
        REQUIRE(result.reinforceTick.has_value());
        CHECK(*result.reinforceTick >= 1150);
        CHECK(*result.reinforceTick <= 2250);
    }
    const float brokenMedian = median(broken);
    const float impactMedian = median(impacts);
    INFO("scripted wins " << wins << " jp broken median " << brokenMedian << " impact median " << impactMedian);
    CHECK(wins >= 6);
    CHECK(wins <= 21);
    CHECK(brokenMedian >= 2.f);
    CHECK(brokenMedian <= 4.f);
    CHECK(impactMedian >= 9.f);
    CHECK(impactMedian <= 15.f);
}
