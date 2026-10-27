#pragma once

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

} // namespace tq
