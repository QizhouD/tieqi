#include "app/App.h"
#include "app/Launch.h"

#include <exception>
#include <filesystem>
#include <iostream>

int main(int argc, char **argv) {
    try {
        const tq::LaunchOptions options = tq::parseLaunch(argc, argv);
        const std::filesystem::path executableDir = tq::executableDirectory(argc > 0 ? argv[0] : nullptr);
        const std::filesystem::path current = std::filesystem::current_path();
        std::error_code error;
        const bool currentHasData = std::filesystem::exists(current / "data", error);
        const std::filesystem::path root =
            tq::resolveAssetRoot({options.dataRoot, current, executableDir, currentHasData});
        tq::App app(root, options, executableDir);
        app.run();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
