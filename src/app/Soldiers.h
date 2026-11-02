#pragma once

#include "core/defs/UnitType.h"
#include "core/math/Vec2.h"
#include "core/sim/Unit.h"

#include <cstdint>
#include <vector>

namespace tq {

// The soldier layer is presentation only. Every frame it is computed from the unit snapshot (interpolated pose,
// frontage, depth, troops, state); it keeps no state and never touches the simulation's random numbers. Each unit
// seeds its own jitter from its id.

inline constexpr float kMenPerCavalrySoldier = 10.f;
inline constexpr float kMenPerInfantrySoldier = 20.f;
inline constexpr int kMaxSoldiersPerUnit = 60;
// Soldiers replace the formation dots from this zoom level up.
inline constexpr float kSoldierZoom = 2.f;

enum class SoldierKind : std::uint8_t { Cavalry, Spear, Gun, Samurai, General };
inline constexpr int kSoldierKinds = 5;

enum class SoldierAnim : std::uint8_t { Idle, March, Charge, Engaged, Waver, Turn, Rout };

// Atlas columns. 0 idle, 1-4 the move cycle, 5-6 strike, 7 idle looking aside.
inline constexpr int kAtlasFrames = 8;
inline constexpr int kFrameIdle = 0;
inline constexpr int kFrameMove = 1;
inline constexpr int kMoveFrames = 4;
inline constexpr int kFrameStrike = 5;
inline constexpr int kFrameAside = 7;
inline constexpr int kAtlasDirections = 8;

struct Soldier {
    Vec2 pos{};
    float facing = 0.f;
    SoldierKind kind = SoldierKind::Samurai;
    Side side = Side::Ming;
    int direction = 0;
    int frame = 0;
    // 1 for a steady unit; lower while routing so the scattered men read as a broken body.
    float shade = 1.f;
    // A dust puff behind a charging rider, drawn under the soldiers. Zero when there is none.
    float dust = 0.f;
    Vec2 dustPos{};
};

SoldierKind soldierKind(const UnitType &type);

// Men per soldier for the unit type: 10 for cavalry, 20 for infantry.
float menPerSoldier(const UnitType &type);

// Soldiers for a number of men, at least one while any are left, at most 60.
int soldierCount(const UnitType &type, float men);

SoldierAnim soldierAnim(const Unit &unit);

// Screen direction 0-7 for a world facing: 0 east, 2 south, 4 west, 6 north.
int soldierDirection(float facing);

// Deterministic per-unit noise in [0, 1). Different salts give independent streams.
float soldierNoise(std::uint32_t unitId, int index, int salt);

// Ranks and files for the unit's full strength, laid out in the formation rectangle. Slot 0 is the front-left;
// slots fill the front rank first, so losses come off the rear ranks.
struct SoldierGrid {
    int files = 1;
    int ranks = 1;
    float fileSpacing = 0.f;
    float rankSpacing = 0.f;
};
SoldierGrid soldierGrid(const UnitType &type, const Unit &unit);

// Local slot position: x along the facing (front positive), y across the frontage.
Vec2 soldierSlot(const SoldierGrid &grid, int index);

// Appends the unit's soldiers. animTime is presentation time that stops while paused.
void layoutSoldiers(const Unit &unit, const UnitType &type, float alpha, float animTime, std::vector<Soldier> &out);

} // namespace tq
