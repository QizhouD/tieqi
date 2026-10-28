#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace tq {

struct LaunchOptions {
    std::string scenario = "byeokjegwan";
    std::uint64_t seed = 1;
    std::optional<float> timeScale;
    std::filesystem::path replay;
    std::filesystem::path script;
    std::filesystem::path dataRoot;
    bool scenarioSet = false;
    bool seedSet = false;
    bool perf = false;
    std::optional<std::uint32_t> freezeAt;
};

struct AssetRootRequest {
    std::filesystem::path overrideRoot;
    std::filesystem::path currentDir;
    std::filesystem::path executableDir;
    bool currentHasData = false;
};

LaunchOptions parseLaunch(int argc, char **argv);
std::filesystem::path resolveAssetRoot(const AssetRootRequest &request);
std::filesystem::path executableDirectory(const char *argv0);

} // namespace tq
