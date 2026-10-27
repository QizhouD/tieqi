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

inline constexpr float kTickSeconds = 0.8f;
inline constexpr float kInitialStillSec = 16.f;
inline constexpr float kReloadRollSec = 16.f;

inline constexpr float kMomentumPeriodSec = 240.f;
inline constexpr float kKnockDecay = 0.05f;
inline constexpr float kKnockDecaySec = 8.f;
inline constexpr float kKnockStop = 0.25f;

inline constexpr float kArriveMeters = 5.f;
inline constexpr float kChargeMinMeters = 45.f;
inline constexpr float kCavalryChargeMeters = 210.f;
inline constexpr float kInfantryChargeMeters = 70.f;
inline constexpr float kChargeKeep = 1.4f;
inline constexpr float kHoldEnemyMeters = 150.f;
inline constexpr float kContactTurn = 0.5f;
inline constexpr float kHoldEnemyTurn = 0.6f;
inline constexpr float kChargeTurn = 0.7f;
inline constexpr float kRoutTurn = 1.6f;

inline constexpr float kCavalryAccel = 0.9375f;
inline constexpr float kInfantryChargeAccel = 0.46875f;
inline constexpr float kInfantryWalkAccel = 0.390625f;
inline constexpr float kSpeedBrake = 0.9375f;

inline constexpr float kCavalryAlignNear = 0.6f;
inline constexpr float kCavalryAlignFar = 1.4f;
inline constexpr float kCavalryAlignMid = 0.65f;
inline constexpr float kCavalryAlignSlow = 0.3f;
inline constexpr float kInfantryAlignNear = 0.5f;
inline constexpr float kInfantryAlignFar = 1.2f;
inline constexpr float kInfantryAlignMid = 0.45f;
inline constexpr float kInfantryAlignSlow = 0.12f;

inline constexpr float kRoutCavalry = 5.f;
inline constexpr float kRoutInfantry = 1.75f;
inline constexpr float kRoutFearMeters = 200.f;
inline constexpr float kRoutFearNear = 1.2f;
inline constexpr float kRoutFearFar = 0.35f;
inline constexpr float kScatterSec = 40.f;
inline constexpr float kScatterWeight = 1.6f;
inline constexpr float kFleeMeters = 12.f;
inline constexpr float kInteriorMeters = 10.f;

inline constexpr float kFormationLimit = 140.f;
inline constexpr float kOrderEdge = 12.f;
inline constexpr float kUnfitMelee = 0.01f;
inline constexpr float kGeneralDownMen = 0.25f;
inline constexpr float kDisperseMenRatio = 0.12f;
inline constexpr float kDisperseMenMin = 25.f;
inline constexpr float kBreakawayMen = 0.03f;
inline constexpr float kBreakawayMorale = 5.f;
inline constexpr float kGeneralAuraKill = 260.f;
inline constexpr float kGeneralAllyMorale = 22.f;
inline constexpr float kGeneralEnemyMorale = 8.f;
inline constexpr float kMoraleMax = 100.f;

} // namespace tq
