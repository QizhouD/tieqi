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

float percentile(std::vector<float> values, double portion) {
    std::sort(values.begin(), values.end());
    if (values.empty()) {
        return 0.f;
    }
    const std::size_t index = std::min(
        values.size() - 1, static_cast<std::size_t>(std::ceil(portion * static_cast<double>(values.size()))) - 1);
    return values[index];
}

bool phase2Allowed(const RunResult &result) {
    if (!result.phase2Tick) {
        return true;
    }
    if (!result.warnTick) {
        return false;
    }
    const int phase2 = static_cast<int>(*result.phase2Tick);
    const int warn = static_cast<int>(*result.warnTick);
    return std::abs(phase2 - (warn + 200)) <= 2 && phase2 >= 1000 && phase2 <= 1800;
}

bool isWin(const RunResult &result) {
    return result.grade == "decisive" || result.grade == "minor" || result.grade == "pyrrhic";
}

int countWins(const std::vector<RunResult> &results) {
    int wins = 0;
    for (const RunResult &result : results) {
        wins += isWin(result) ? 1 : 0;
    }
    return wins;
}

int countCommanderKilled(const std::vector<RunResult> &results) {
    int killed = 0;
    for (const RunResult &result : results) {
        killed += result.endKind == "commander_killed" ? 1 : 0;
    }
    return killed;
}

std::vector<float> column(const std::vector<RunResult> &results, auto pick) {
    std::vector<float> values;
    values.reserve(results.size());
    for (const RunResult &result : results) {
        values.push_back(static_cast<float>(pick(result)));
    }
    return values;
}

std::vector<float> presentTicks(const std::vector<RunResult> &results, auto pick) {
    std::vector<float> values;
    for (const RunResult &result : results) {
        if (const auto tick = pick(result)) {
            values.push_back(static_cast<float>(*tick));
        }
    }
    return values;
}

} // namespace

TEST_CASE("slow full-battle baseline stays inside the published bands") {
    const GameData data = loadGameData(sourceRoot() / "data", "byeokjegwan.json");
    std::vector<RunResult> passive;
    std::vector<RunResult> scripted;
    passive.reserve(100);
    scripted.reserve(100);
    for (std::uint64_t seed = 1; seed <= 100; ++seed) {
        RunRequest request;
        request.seed = seed;
        request.policy = PolicyKind::Passive;
        passive.push_back(runBattle(data, request));
        request.policy = PolicyKind::Scripted;
        scripted.push_back(runBattle(data, request));
    }

    for (const RunResult &result : passive) {
        INFO("passive seed end " << result.endKind << " tick " << result.endTick);
        CHECK(phase2Allowed(result));
    }
    for (const RunResult &result : scripted) {
        INFO("scripted seed end " << result.endKind << " tick " << result.endTick);
        CHECK(phase2Allowed(result));
    }

    CHECK(std::abs(countWins(passive) - 2) <= 15);
    CHECK(std::abs(countCommanderKilled(passive) - 98) <= 15);
    CHECK(std::abs(countWins(scripted) - 49) <= 15);
    CHECK(std::abs(countCommanderKilled(scripted) - 51) <= 15);

    const float passiveEnd = percentile(column(passive, [](const RunResult &result) { return result.endTick; }), 0.5);
    const float scriptedEnd = percentile(column(scripted, [](const RunResult &result) { return result.endTick; }), 0.5);
    INFO("end median passive " << passiveEnd << " scripted " << scriptedEnd);
    CHECK(passiveEnd >= 1575.f);
    CHECK(passiveEnd <= 1765.f);
    CHECK(scriptedEnd >= 2268.f);
    CHECK(scriptedEnd <= 2885.f);

    const float passiveBroken =
        percentile(column(passive, [](const RunResult &result) { return result.jpBroken; }), 0.5);
    const float scriptedBroken =
        percentile(column(scripted, [](const RunResult &result) { return result.jpBroken; }), 0.5);
    INFO("jp broken median passive " << passiveBroken << " scripted " << scriptedBroken);
    CHECK(passiveBroken >= 0.f);
    CHECK(passiveBroken <= 1.f);
    CHECK(scriptedBroken >= 2.f);
    CHECK(scriptedBroken <= 3.f);

    const float passiveImpacts =
        percentile(column(passive, [](const RunResult &result) { return result.impacts; }), 0.5);
    const float scriptedImpacts =
        percentile(column(scripted, [](const RunResult &result) { return result.impacts; }), 0.5);
    INFO("impact median passive " << passiveImpacts << " scripted " << scriptedImpacts);
    CHECK(passiveImpacts >= 4.f);
    CHECK(passiveImpacts <= 6.f);
    CHECK(scriptedImpacts >= 9.f);
    CHECK(scriptedImpacts <= 13.f);

    const std::vector<float> passiveYang =
        presentTicks(passive, [](const RunResult &result) { return result.reinforceTick; });
    const std::vector<float> scriptedYang =
        presentTicks(scripted, [](const RunResult &result) { return result.reinforceTick; });
    const float passiveYangMedian = percentile(passiveYang, 0.5);
    const float scriptedYangMedian = percentile(scriptedYang, 0.5);
    INFO("yang median passive " << passiveYangMedian << " scripted " << scriptedYangMedian);
    // The prototype passive median is 2003. The port's 2000 is 3 ticks early from 0.1 accumulation; up to 4 ticks early
    // is allowed.
    CHECK(passiveYangMedian >= 2003.f - 4.f);
    CHECK(passiveYangMedian <= 2003.f);
    CHECK(scriptedYangMedian >= 1663.f);
    CHECK(scriptedYangMedian <= 2003.f);

    const float scriptedSaved =
        percentile(column(scripted, [](const RunResult &result) { return result.savedRate; }), 0.5);
    INFO("scripted saved-rate median " << scriptedSaved);
    CHECK(scriptedSaved >= 0.04234489933961343);
    CHECK(scriptedSaved <= 0.4384374131301025);
}
