#pragma once

#include "Policy.h"
#include "core/sim/Event.h"
#include "data/Load.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace tq {

struct RunRequest {
    PolicyKind policy = PolicyKind::Passive;
    std::uint64_t seed = 1;
    bool ai = true;
    std::uint32_t maxTicks = 18000;
    bool replay = false;
    std::vector<ReplayCommand> commands;
};

struct RunResult {
    std::uint32_t endTick = 0;
    std::string endKind = "timeout";
    std::string grade = "none";
    float savedRate = 0.f;
    std::optional<std::uint32_t> warnTick;
    std::optional<std::uint32_t> phase2Tick;
    std::optional<std::uint32_t> phase3Tick;
    std::optional<std::uint32_t> reinforceTick;
    float mingLost = 0.f;
    float jpLost = 0.f;
    int mingBroken = 0;
    int jpBroken = 0;
    int impacts = 0;
    float bestImpact = 0.f;
    int events = 0;
    std::uint64_t hash = 0;
    bool timeout = false;
    bool finite = true;
    std::vector<Event> log;
    Replay replay;
};

RunResult runBattle(const GameData &data, const RunRequest &request);

} // namespace tq
