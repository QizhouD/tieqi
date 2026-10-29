#pragma once

#include "core/defs/UnitType.h"
#include "core/sim/Battle.h"
#include "core/sim/Command.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace tq {

struct ReplayCommand {
    std::uint32_t tick = 0;
    Command::Type type = Command::Type::Move;
    std::vector<std::string> units;
    std::optional<Vec2> point;
    std::optional<std::string> target;
    // Spawn only: the unit type id and the facing in radians.
    std::optional<std::string> unitType;
    std::optional<float> facing;
};

struct Replay {
    int schema = 1;
    std::string build;
    std::string scenario;
    std::uint64_t seed = 1;
    bool ai = true;
    std::vector<ReplayCommand> commands;
};

std::string buildId();
void saveReplay(const std::filesystem::path &path, const Replay &replay);
Replay loadReplay(const std::filesystem::path &path);

// Units are named by scene id, or "#" plus UnitId when they have none. Returns nothing when the command has no unit
// left to order (spawn needs a known unit type instead).
std::optional<Command> resolveCommand(const Battle &battle, const GameDefs &defs, const ReplayCommand &command);
// The inverse of resolveCommand for a command just submitted. The tick is left at 0.
ReplayCommand recordCommand(const Battle &battle, const GameDefs &defs, const Command &command);

} // namespace tq
