#pragma once

#include "core/sim/Battle.h"
#include "data/Replay.h"

#include <cstdint>

namespace tq {

enum class PolicyKind { Passive, Scripted };

struct PolicyState {
    bool samuraiOrdered = false;
    bool withdrew = false;
    std::uint32_t phase3At = 0;
};

void applyPolicy(Battle &battle, PolicyKind policy, PolicyState &state, std::vector<ReplayCommand> &recorded);

} // namespace tq
