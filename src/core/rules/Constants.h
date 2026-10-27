#pragma once

namespace tq {

// 几何与地形。来源是开发文档「规则规格」；代码里不再写这些字面量。
inline constexpr float kContactSidePad = 14.f;

inline constexpr float kTerrainPaddyCavalry = 0.6f;
inline constexpr float kTerrainPaddyInfantry = 0.7f;
inline constexpr float kTerrainHillCavalry = 0.8f;
inline constexpr float kTerrainHillInfantry = 0.85f;
inline constexpr float kTerrainRoad = 1.1f;
inline constexpr float kTerrainOpen = 1.f;

// 组的出生点可以落在地图外，但不能超过这一距离。
inline constexpr float kSpawnOutsideMeters = 90.f;

} // namespace tq
