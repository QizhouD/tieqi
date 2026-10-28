#include "app/Launch.h"

#include <stdexcept>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__linux__)
#include <filesystem>
#endif

namespace tq {
namespace {

std::string requireValue(int argc, char **argv, int &index) {
    if (index + 1 >= argc) {
        throw std::runtime_error(std::string("missing value for ") + argv[index]);
    }
    index += 1;
    return argv[index];
}

} // namespace

LaunchOptions parseLaunch(int argc, char **argv) {
    LaunchOptions options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--scenario") {
            options.scenario = requireValue(argc, argv, i);
            options.scenarioSet = true;
        } else if (arg == "--seed") {
            const std::string text = requireValue(argc, argv, i);
            try {
                options.seed = std::stoull(text);
            } catch (const std::exception &) {
                throw std::runtime_error("seed is not an integer: " + text);
            }
            options.seedSet = true;
        } else if (arg == "--time-scale") {
            const std::string text = requireValue(argc, argv, i);
            try {
                options.timeScale = std::stof(text);
            } catch (const std::exception &) {
                throw std::runtime_error("time scale is not a number: " + text);
            }
            if (!(*options.timeScale > 0.f)) {
                throw std::runtime_error("time scale must be greater than 0");
            }
        } else if (arg == "--replay") {
            options.replay = std::filesystem::absolute(requireValue(argc, argv, i));
        } else if (arg == "--script") {
            options.script = std::filesystem::absolute(requireValue(argc, argv, i));
        } else if (arg == "--data-root") {
            options.dataRoot = std::filesystem::absolute(requireValue(argc, argv, i));
        } else if (arg == "--perf") {
            options.perf = true;
        } else if (arg == "--freeze-at") {
            const std::string text = requireValue(argc, argv, i);
            if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos) {
                throw std::runtime_error("freeze tick is not an integer: " + text);
            }
            try {
                options.freezeAt = static_cast<std::uint32_t>(std::stoul(text));
            } catch (const std::exception &) {
                throw std::runtime_error("freeze tick is not an integer: " + text);
            }
        } else {
            throw std::runtime_error("unknown argument: " + arg);
        }
    }
    return options;
}

std::filesystem::path resolveAssetRoot(const AssetRootRequest &request) {
    if (!request.overrideRoot.empty()) {
        return request.overrideRoot;
    }
    if (request.currentHasData) {
        return request.currentDir;
    }
    return request.executableDir;
}

std::filesystem::path executableDirectory(const char *argv0) {
    std::error_code error;
#ifdef _WIN32
    wchar_t buffer[MAX_PATH];
    const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (length > 0 && length < MAX_PATH) {
        return std::filesystem::path(buffer).parent_path();
    }
#elif defined(__linux__)
    const std::filesystem::path self = std::filesystem::read_symlink("/proc/self/exe", error);
    if (!error) {
        return self.parent_path();
    }
#endif
    const std::filesystem::path invoked = std::filesystem::absolute(argv0 != nullptr ? argv0 : ".", error);
    if (error) {
        return std::filesystem::current_path();
    }
    return invoked.parent_path();
}

} // namespace tq
