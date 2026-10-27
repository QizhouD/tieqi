#pragma once

#include <string>
#include <vector>

namespace tq {

// 只给表现层用。规则核心不读它。
struct Config {
    float timeScale = 0.f;
    std::vector<float> playerSpeeds;
    float defaultSpeed = 0.f;
    int maxTicksPerFrame = 0;
    std::string language;
    bool reducedMotion = false;
    bool sound = false;
};

} // namespace tq
