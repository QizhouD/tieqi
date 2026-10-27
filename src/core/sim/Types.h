#pragma once

#include <cstdint>

namespace tq {

// 顺序与开发文档「规则核心 API」一致。UnitState 等由 C2 补上。
enum class Side : std::uint8_t { Ming, Jp };
enum class Terrain : std::uint8_t { Open, Road, Hill, Paddy };
enum class ContactSide : std::uint8_t { Front, Flank, Rear };

} // namespace tq
