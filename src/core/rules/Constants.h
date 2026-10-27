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

inline constexpr float kContactRange = 120.f;
inline constexpr float kContactInflate = 2.5f;
inline constexpr float kSeparateRout = 0.8f;
inline constexpr float kSeparateRoutOther = 0.2f;
inline constexpr float kSeparateMover = 0.85f;
inline constexpr float kSeparateHolder = 0.15f;
inline constexpr float kSeparateEven = 0.5f;
inline constexpr float kFastEnoughSpeed = 0.7f;
inline constexpr float kFastEnoughDist = 35.f;
inline constexpr float kDeployDoneY = 60.f;

inline constexpr float kChargeFlank = 1.8f;
inline constexpr float kChargeRear = 2.4f;
inline constexpr float kChargeWavering = 1.35f;
inline constexpr float kChargeBroken = 2.2f;
inline constexpr float kChargeReloadSec = 2.4f;
inline constexpr float kChargeReloadMul = 1.25f;
inline constexpr float kChargeUnformedSec = 9.6f;
inline constexpr float kChargeUnformedMul = 1.2f;
inline constexpr float kChargePaddy = 0.5f;
inline constexpr float kChargeUphill = 0.8f;
inline constexpr float kChargePike = 0.35f;
inline constexpr float kChargeMomentumStep = 0.1f;
inline constexpr float kChargeOwnWaver = 0.8f;
inline constexpr float kChargeResistSteady = 0.85f;
inline constexpr float kChargeResistFloor = 0.15f;
inline constexpr float kChargeSpeedFloor = 0.5f;
inline constexpr float kModSpeed = 0.95f;
inline constexpr float kModStrength = 0.95f;
inline constexpr float kModDefense = 0.01f;

inline constexpr float kImpactBreak = 1.9f;
inline constexpr float kImpactPushMax = 26.f;
inline constexpr float kImpactPushBase = 6.f;
inline constexpr float kImpactPushScale = 7.f;
inline constexpr float kImpactMorale = 4.f;
inline constexpr float kImpactMoraleScale = 9.f;
inline constexpr float kImpactPikeMorale = 2.f;
inline constexpr float kImpactKill = 0.035f;
inline constexpr float kImpactKillPike = 0.06f;
inline constexpr float kImpactPikeMoraleHit = 14.f;
inline constexpr float kImpactPikeRecover = 72.f;
inline constexpr float kImpactKillSelf = 0.012f;
inline constexpr float kImpactRatioFloor = 0.3f;
inline constexpr float kRecoverCavalry = 48.f;
inline constexpr float kRecoverInfantry = 32.f;
inline constexpr float kBrokenSec = 9.6f;
inline constexpr float kBrokenMorale = 25.f;
inline constexpr float kBreakSplitMen = 150.f;
inline constexpr float kBreakTripleMen = 330.f;
inline constexpr float kThreeEyedMeters = 62.f;
inline constexpr float kThreeEyedMen = 0.015f;
inline constexpr float kThreeEyedMorale = 8.f;

inline constexpr float kPreviewFavored = 1.2f;
inline constexpr float kPreviewEven = 0.8f;
inline constexpr float kVolleyReloadSec = 8.f;
inline constexpr float kVolleyStillSec = 6.4f;
inline constexpr float kVolleyNearTarget = 160.f;
inline constexpr float kVolleyApproach = 220.f;
inline constexpr float kVolleyAim = 1.f;

inline constexpr float kEngagedGain = 4.f;
inline constexpr float kEngagedLoss = 8.f;
inline constexpr float kMeleeWaver = 0.8f;
inline constexpr float kMeleeMomentum = 0.1f;
inline constexpr float kMeleeRecover = 0.85f;
inline constexpr float kMeleePaddy = 0.75f;
inline constexpr float kMeleeGrindSec = 48.f;
inline constexpr float kMeleeGrind = 0.85f;
inline constexpr float kPikeVsCavalry = 1.3f;
inline constexpr float kCavalryVsPike = 0.7f;
inline constexpr float kMeleeFlank = 1.6f;
inline constexpr float kMeleeRear = 2.2f;
inline constexpr float kMeleePursuit = 2.5f;
inline constexpr float kMeleeRoutPower = 0.15f;
inline constexpr float kMeleePowerFloor = 0.1f;
inline constexpr float kMeleeRatioMin = 0.3f;
inline constexpr float kMeleeRatioMax = 3.f;
inline constexpr float kMeleeRandomBase = 0.9f;
inline constexpr float kMeleeRandomSpan = 0.2f;
inline constexpr float kMeleeCasualties = 0.012f;
inline constexpr float kMeleeCavalryRout = 1.4f;
inline constexpr float kMeleeMoraleBase = 0.8f;
inline constexpr float kMeleeMoraleEngaged = 160.f;
inline constexpr float kMeleeMoraleFlank = 1.5f;
inline constexpr float kMeleeMoraleRear = 2.f;
inline constexpr float kMeleeLossMorale = 75.f;
inline constexpr float kMeleeNearGeneral = 0.85f;
inline constexpr float kCapFloor = 0.55f;
inline constexpr float kCapMen = 0.45f;
inline constexpr float kMoraleRecover = 0.5f;
inline constexpr float kMoraleGeneral = 1.f;
inline constexpr float kMoraleBreak = 30.f;
inline constexpr float kMoraleWaver = 55.f;
inline constexpr float kRoutAttrition = 0.007f;
inline constexpr float kRallyRange = 180.f;
inline constexpr float kRallySec = 80.f;
inline constexpr float kRallyGain = 1.5f;
inline constexpr float kRallyMorale = 40.f;
inline constexpr float kRallyMen = 0.25f;
inline constexpr float kBreakAllyRange = 170.f;
inline constexpr float kBreakAllyBase = 4.f;
inline constexpr float kBreakAllyScale = 10.f;
inline constexpr float kBreakAllyGeneral = 0.7f;
inline constexpr float kBreakEnemyRange = 200.f;
inline constexpr float kBreakEnemyGain = 4.f;
inline constexpr float kBreakEnemyCap = 5.f;
inline constexpr float kMomentumCap = 3.f;
inline constexpr float kShotMen = 0.08f;
inline constexpr float kShotFalloff = 0.6f;
inline constexpr float kShotWaver = 0.75f;
inline constexpr float kShotCharge = 1.3f;
inline constexpr float kShotChargeScore = 2.f;
inline constexpr float kShotRangePad = 30.f;
inline constexpr float kShotArc = 0.8f;
inline constexpr float kShotClose = 0.75f;
inline constexpr float kShotReloadJitter = 6.4f;
inline constexpr float kShotMorale = 7.f;
inline constexpr float kShotMoraleScale = 9.f;
inline constexpr float kShotSlow = 0.8f;
inline constexpr float kPreviewMeleeFavored = 1.4f;
inline constexpr float kFragMenFloor = 0.12f;
inline constexpr float kFragFrontMin = 22.f;
inline constexpr float kFragDepthMin = 14.f;
inline constexpr float kFragFrontExp = 0.6f;
inline constexpr float kFragDepthExp = 0.3f;
inline constexpr float kShatterKnock = 8.75f;
inline constexpr float kShatterSpread = 1.9f;
inline constexpr float kShatterJitter = 0.3f;
inline constexpr float kShatterSpeed = 18.75f;
inline constexpr float kShatterSpeedSpan = 10.f;
inline constexpr float kShatterSeek = 400.f;
inline constexpr float kMergeDistance = 46.f;
inline constexpr float kSplitMinMen = 160.f;
inline constexpr float kSplitGap = 2.f;

} // namespace tq
