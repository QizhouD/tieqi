#pragma once

#include <cstdint>

namespace tq {

inline constexpr int kPointerLeft = 0;
inline constexpr int kPointerRight = 1;

// 逻辑像素。脚本和窗口都先变成这个，再交给下令。
struct PointerSample {
    enum class Type : std::uint8_t { Move, Down, Up } type = Type::Move;
    enum class Device : std::uint8_t { Mouse, Touch } device = Device::Mouse;
    int button = kPointerLeft;
    float x = 0.f;
    float y = 0.f;
    bool shift = false;
};

} // namespace tq
