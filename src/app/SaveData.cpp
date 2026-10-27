#include "app/SaveData.h"

#include <fstream>
#include <sstream>

namespace tq {
namespace {

bool truthy(const std::string &value) { return value == "1" || value == "true"; }

} // namespace

bool SaveData::load(const std::filesystem::path &file) {
    std::ifstream in(file);
    if (!in) {
        return false;
    }
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const std::size_t eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        const std::string key = line.substr(0, eq);
        const std::string value = line.substr(eq + 1);
        try {
            if (key == "language" && (value == "zh-Hant" || value == "en" || value == "ko")) {
                language = value;
            } else if (key == "volume") {
                volume = std::stof(value);
            } else if (key == "speed") {
                speed = std::stof(value);
            } else if (key == "reduced_motion") {
                reducedMotion = truthy(value);
            }
        } catch (const std::exception &) {
        }
    }
    if (volume < 0.f) {
        volume = 0.f;
    }
    if (volume > 100.f) {
        volume = 100.f;
    }
    return true;
}

bool SaveData::save(const std::filesystem::path &file) const {
    std::ofstream out(file, std::ios::trunc);
    if (!out) {
        return false;
    }
    out << "language=" << language << '\n'
        << "volume=" << volume << '\n'
        << "speed=" << speed << '\n'
        << "reduced_motion=" << (reducedMotion ? 1 : 0) << '\n';
    return static_cast<bool>(out);
}

} // namespace tq
