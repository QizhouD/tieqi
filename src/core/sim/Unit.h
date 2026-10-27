#pragma once

#include "core/sim/Types.h"

#include <optional>
#include <string>
#include <vector>

namespace tq {

struct Unit {
    UnitId id = kNoUnit;
    std::uint16_t type = 0;
    Side side = Side::Ming;
    std::string labelKey;
    std::uint16_t fragNo = 0;
    Vec2 pos{};
    Vec2 prevPos{};
    float facing = 0.f;
    float prevFacing = 0.f;
    float frontage = 0.f;
    float depth = 0.f;
    float men = 0.f;
    float maxMen = 0.f;
    float morale = 0.f;
    UnitState state = UnitState::Steady;
    GoneReason gone = GoneReason::None;
    Order order;
    std::vector<UnitId> contacts;
    float engagedSec = 0.f;
    float recoverSec = 0.f;
    float reloadSec = 0.f;
    float stillSec = kInitialStillSec;
    float speed = 0.f;
    float runDist = 0.f;
    bool charging = false;
    bool volleyDone = false;
    bool breaking = false;
    int momentum = 0;
    float momentumSec = 0.f;
    float brokenSec = 0.f;
    float routSec = 0.f;
    Vec2 knock{};
    std::optional<float> scatterAngle;
    bool everBroken = false;
    bool isFragment = false;
    bool rejoin = false;
    bool vanguard = false;
    bool deploying = false;
    UnitId origin = kNoUnit;
};

} // namespace tq
