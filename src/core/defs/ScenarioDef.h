#pragma once

#include "core/defs/MapDef.h"

#include <cstdint>
#include <string>
#include <vector>

namespace tq {

struct Placement {
    std::string id;
    std::string labelKey;
    std::uint16_t type = 0;
    Vec2 pos{};
    float facing = 0.f;
    Vec2 rally{};
    float rallyFacing = 0.f;
    bool vanguard = false;
};

struct SpawnGroup {
    std::string id;
    std::vector<Placement> units;
};

struct ExitDef {
    Aabb rect;
    float followRadius = 0.f;
    float retreatYOpen = 0.f;
    float retreatYClosed = 0.f;
};

struct ScoutWarningDef {
    float atSec = 0.f;
    float earlyAtSec = 0.f;
    int earlyVanguardBroken = 0;
    float leadSec = 0.f;
};

struct Phase2Def {
    std::string spawnGroup;
    bool openExit = false;
};

struct GrandAssaultDef {
    std::string general;
    float moraleAdd = 0.f;
    float capOverType = 0.f;
};

struct Phase3Def {
    float afterPhase2Sec = 0.f;
    std::string spawnGroup;
    GrandAssaultDef grandAssault;
};

struct DangerDef {
    float afterPhase2Sec = 0.f;
    float enemyWithin = 0.f;
    float menBelow = 0.f;
};

struct ReinforcementDef {
    std::string id;
    std::string spawnGroup;
    float afterPhase3Sec = 0.f;
    DangerDef danger;
};

// enabled false means the scenario omitted flow. C6 keeps that as phase 1 with the exit closed.
struct FlowDef {
    bool enabled = false;
    ScoutWarningDef scoutWarning;
    Phase2Def phase2;
    Phase3Def phase3;
    std::vector<ReinforcementDef> reinforcements;
};

struct AiDef {
    float vanguardHoldSec = 0.f;
    float vanguardTriggerDist = 0.f;
};

struct ResultDef {
    float decisiveRate = 0.f;
    int decisiveEnemyBroken = 0;
    float minorRate = 0.f;
};

struct ScenarioDef {
    std::string id;
    std::string nameKey;
    std::string startClock;
    MapDef map;
    std::string commander;
    ExitDef exit;
    std::vector<Placement> deployment;
    std::vector<SpawnGroup> groups;
    FlowDef flow;
    AiDef ai;
    ResultDef result;
};

} // namespace tq
