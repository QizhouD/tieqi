#pragma once

#include <filesystem>
#include <string>

namespace tq {

// 可执行文件旁边的 settings.txt。缺文件或坏行都忽略。
struct SaveData {
    std::string language = "zh-Hant";
    float volume = 70.f;
    float speed = 1.f;
    bool reducedMotion = false;

    bool load(const std::filesystem::path &file);
    bool save(const std::filesystem::path &file) const;
};

} // namespace tq
