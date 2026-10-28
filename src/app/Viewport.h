#pragma once

#include "core/math/Vec2.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace tq {

inline constexpr unsigned kWindowWidth = 1600;
inline constexpr unsigned kWindowHeight = 1000;
inline constexpr unsigned kMinWindowWidth = 1280;
inline constexpr unsigned kMinWindowHeight = 800;
// A4 做顶栏和面板时按原型在 1600×1000 下量到的画布重设战场矩形：
// 画布 (17, 55)、1216×932。.field 外框是 (16, 54)、1218×934，多出来的是 1px 边框。
inline constexpr unsigned kTopBarHeight = 56;
inline constexpr unsigned kSidePanelWidth = 380;
inline constexpr float kDesktopFit = 0.9f;
inline constexpr float kMapMarginPx = 10.f;
inline constexpr float kMapPixelsPerMeter = 1.5f;

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

// 默认窗口不超过桌面的 90%，没有最大尺寸。最小取 1280×800 与桌面 90% 中较小的一边。
struct WindowFit {
    unsigned defaultW = kWindowWidth;
    unsigned defaultH = kWindowHeight;
    unsigned minW = kMinWindowWidth;
    unsigned minH = kMinWindowHeight;
};

inline unsigned floorPixels(float value) {
    if (!(value > 1.f)) {
        return 1;
    }
    return static_cast<unsigned>(std::floor(value));
}

inline WindowFit fitWindow(unsigned desktopW, unsigned desktopH, unsigned logicalW = kWindowWidth,
                           unsigned logicalH = kWindowHeight, unsigned preferredMinW = kMinWindowWidth,
                           unsigned preferredMinH = kMinWindowHeight, float fraction = kDesktopFit) {
    const unsigned limitW = floorPixels(static_cast<float>(desktopW) * fraction);
    const unsigned limitH = floorPixels(static_cast<float>(desktopH) * fraction);
    WindowFit fit;
    fit.minW = std::min(preferredMinW, limitW);
    fit.minH = std::min(preferredMinH, limitH);
    float scale = 1.f;
    if (logicalW > 0 && logicalH > 0) {
        scale = std::min(1.f, std::min(static_cast<float>(limitW) / static_cast<float>(logicalW),
                                       static_cast<float>(limitH) / static_cast<float>(logicalH)));
    }
    unsigned width = floorPixels(static_cast<float>(logicalW) * scale);
    unsigned height = floorPixels(static_cast<float>(logicalH) * scale);
    width = std::clamp(width, fit.minW, limitW);
    height = std::clamp(height, fit.minH, limitH);
    fit.defaultW = width;
    fit.defaultH = height;
    return fit;
}

// 逻辑画面里的矩形。战场是去掉顶栏和右侧面板后的区域，A2、A3 都用它。
struct ScreenRect {
    float x = 0.f;
    float y = 0.f;
    float w = 0.f;
    float h = 0.f;
};

inline ScreenRect battlefieldRect(float logicalW = static_cast<float>(kWindowWidth),
                                  float logicalH = static_cast<float>(kWindowHeight)) {
    return {0.f, static_cast<float>(kTopBarHeight), std::max(0.f, logicalW - static_cast<float>(kSidePanelWidth)),
            std::max(0.f, logicalH - static_cast<float>(kTopBarHeight))};
}

inline float mapScale(float pixelW, float pixelH, float mapW, float mapH, float margin = kMapMarginPx) {
    if (!(pixelW > 2.f * margin) || !(pixelH > 2.f * margin) || !(mapW > 0.f) || !(mapH > 0.f)) {
        return 1.f;
    }
    return std::min((pixelW - 2.f * margin) / mapW, (pixelH - 2.f * margin) / mapH);
}

// 地图在战场里居中。origin 是世界坐标 (0, 0) 在逻辑画面上的像素位置。
struct MapCamera {
    float scale = 1.f;
    float originX = 0.f;
    float originY = 0.f;
};

inline MapCamera mapCamera(ScreenRect field, float mapW, float mapH) {
    const float scale = mapScale(field.w, field.h, mapW, mapH);
    return {scale, field.x + (field.w - mapW * scale) * 0.5f, field.y + (field.h - mapH * scale) * 0.5f};
}

inline Vec2 worldFromLogical(float x, float y, float mapW, float mapH) {
    const MapCamera camera = mapCamera(battlefieldRect(), mapW, mapH);
    return {(x - camera.originX) / camera.scale, (y - camera.originY) / camera.scale};
}

// 与原型 buildMap 相同的 Mulberry32，种子 1593。只给底图纹理用。
class MapRng {
  public:
    explicit MapRng(std::uint32_t seed) : seed_(static_cast<std::int32_t>(seed)) {}

    float operator()() {
        seed_ = static_cast<std::int32_t>(static_cast<std::uint32_t>(seed_) + 0x6D2B79F5u);
        const auto urshift = [](std::int32_t value, unsigned bits) {
            return static_cast<std::int32_t>(static_cast<std::uint32_t>(value) >> bits);
        };
        const auto imul = [](std::int32_t a, std::int32_t b) {
            return static_cast<std::int32_t>(static_cast<std::uint32_t>(a) * static_cast<std::uint32_t>(b));
        };
        std::int32_t t = imul(seed_ ^ urshift(seed_, 15), 1 | seed_);
        t = static_cast<std::int32_t>(static_cast<std::uint32_t>(t + imul(t ^ urshift(t, 7), 61 | t)) ^
                                      static_cast<std::uint32_t>(t));
        return static_cast<float>(static_cast<std::uint32_t>(t ^ urshift(t, 14))) / 4294967296.f;
    }

  private:
    std::int32_t seed_ = 0;
};

} // namespace tq
