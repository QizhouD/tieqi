#pragma once

#include <filesystem>
#include <string>

namespace tq {

// settings.txt beside the executable. A missing file or a bad line is ignored.
struct SaveData {
    std::string language = "zh-Hant";
    float volume = 70.f;
    float speed = 1.f;
    bool reducedMotion = false;

    bool load(const std::filesystem::path &file);
    bool save(const std::filesystem::path &file) const;
};

} // namespace tq
