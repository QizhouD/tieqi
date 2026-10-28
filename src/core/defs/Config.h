#pragma once

#include <string>
#include <vector>

namespace tq {

// Presentation only. The rules core does not read this.
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
