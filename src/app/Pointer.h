#pragma once

#include <cstdint>

namespace tq {

inline constexpr int kPointerLeft = 0;
inline constexpr int kPointerRight = 1;
inline constexpr int kPointerMiddle = 2;

// Logical pixels. Scripts and the window both convert into this before orders see the pointer.
struct PointerSample {
    enum class Type : std::uint8_t { Move, Down, Up, Wheel } type = Type::Move;
    enum class Device : std::uint8_t { Mouse, Touch } device = Device::Mouse;
    int button = kPointerLeft;
    float x = 0.f;
    float y = 0.f;
    bool shift = false;
    // Wheel notches; positive zooms in.
    float wheel = 0.f;
};

} // namespace tq
