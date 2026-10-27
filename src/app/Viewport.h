#pragma once

#include <cmath>

namespace tq {

inline constexpr unsigned kWindowWidth = 1600;
inline constexpr unsigned kWindowHeight = 1000;
inline constexpr unsigned kMinWindowWidth = 1280;
inline constexpr unsigned kMinWindowHeight = 800;

// 归一化视口。窗口和逻辑画面比例不同时留黑边，画面本身不拉伸。
struct Viewport {
    float x = 0.f;
    float y = 0.f;
    float w = 1.f;
    float h = 1.f;
};

inline Viewport letterbox(float pixelW, float pixelH, float viewW, float viewH) {
    if (!(pixelW > 0.f) || !(pixelH > 0.f) || !(viewW > 0.f) || !(viewH > 0.f)) {
        return {};
    }
    const float windowRatio = pixelW / pixelH;
    const float viewRatio = viewW / viewH;
    if (std::abs(windowRatio - viewRatio) <= 1e-4f) {
        return {};
    }
    if (windowRatio > viewRatio) {
        const float width = viewRatio / windowRatio;
        return {(1.f - width) * 0.5f, 0.f, width, 1.f};
    }
    const float height = windowRatio / viewRatio;
    return {0.f, (1.f - height) * 0.5f, 1.f, height};
}

} // namespace tq
