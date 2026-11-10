#include "app/Soldiers.h"

#include "app/Pieces.h"
#include "core/rules/Constants.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace tq {
namespace {

constexpr float kPi = std::numbers::pi_v<float>;

// Frames per second of presentation time for the move cycle.
constexpr float kWalkRateCavalry = 7.f;
constexpr float kWalkRateInfantry = 5.f;
constexpr float kChargeRate = 12.f;
constexpr float kRoutRate = 10.f;
constexpr float kStrikeRate = 3.f;
// Routing men spread to their widest after this many battlefield seconds.
constexpr float kRoutSpreadSec = 24.f;

std::uint32_t mix(std::uint32_t value) {
    value ^= value >> 16;
    value *= 0x7feb352dU;
    value ^= value >> 15;
    value *= 0x846ca68bU;
    value ^= value >> 16;
    return value;
}

float fract(float value) { return value - std::floor(value); }

Vec2 rotated(Vec2 origin, float facing, Vec2 local) {
    const float c = std::cos(facing);
    const float s = std::sin(facing);
    return {origin.x + local.x * c - local.y * s, origin.y + local.x * s + local.y * c};
}

int cycleFrame(float animTime, float rate, float phase) {
    const int step = static_cast<int>(std::floor(animTime * rate + phase * static_cast<float>(kMoveFrames)));
    return kFrameMove + ((step % kMoveFrames) + kMoveFrames) % kMoveFrames;
}

} // namespace

SoldierKind soldierKind(const UnitType &type) {
    if (type.cls == UnitClass::Cavalry) {
        return SoldierKind::Cavalry;
    }
    if (type.range > 0.f) {
        return SoldierKind::Gun;
    }
    if (type.pike) {
        return SoldierKind::Spear;
    }
    return SoldierKind::Samurai;
}

float menPerSoldier(const UnitType &type) {
    return type.cls == UnitClass::Cavalry ? kMenPerCavalrySoldier : kMenPerInfantrySoldier;
}

int soldierCount(const UnitType &type, float men) {
    if (!(men > 0.f)) {
        return 0;
    }
    const float count = std::ceil(men / menPerSoldier(type) - 1e-3f);
    return std::clamp(static_cast<int>(count), 1, kMaxSoldiersPerUnit);
}

SoldierAnim soldierAnim(const Unit &unit) {
    switch (unit.state) {
    case UnitState::Routing:
        return SoldierAnim::Rout;
    case UnitState::Broken:
        return SoldierAnim::Turn;
    case UnitState::Gone:
        return SoldierAnim::Idle;
    case UnitState::Steady:
    case UnitState::Wavering:
        break;
    }
    if (!unit.contacts.empty()) {
        return SoldierAnim::Engaged;
    }
    if (unit.charging) {
        return SoldierAnim::Charge;
    }
    if (unit.speed > 0.05f) {
        return SoldierAnim::March;
    }
    return unit.state == UnitState::Wavering ? SoldierAnim::Waver : SoldierAnim::Idle;
}

int soldierDirection(float facing) {
    const int step = static_cast<int>(std::lround(facing / (kPi * 0.25f)));
    return ((step % kAtlasDirections) + kAtlasDirections) % kAtlasDirections;
}

float soldierNoise(std::uint32_t unitId, int index, int salt) {
    const std::uint32_t key = mix(unitId * 0x9e3779b9U ^ mix(static_cast<std::uint32_t>(index) * 0x85ebca6bU +
                                                             static_cast<std::uint32_t>(salt) * 0xc2b2ae35U));
    return static_cast<float>(key >> 8) / 16777216.f;
}

SoldierGrid soldierGrid(const UnitType &type, const Unit &unit) {
    SoldierGrid grid;
    const int full = std::max(1, soldierCount(type, unit.maxMen));
    const float frontage = std::max(unit.frontage, 1.f);
    const float depth = std::max(unit.depth, 1.f);
    const float files = std::round(std::sqrt(static_cast<float>(full) * frontage / depth));
    grid.files = std::clamp(static_cast<int>(files), 1, full);
    grid.ranks = (full + grid.files - 1) / grid.files;
    grid.fileSpacing = frontage / static_cast<float>(grid.files);
    grid.rankSpacing = depth / static_cast<float>(grid.ranks);
    // Remember the rectangle through the spacing; slots are measured from its front-left corner.
    return grid;
}

Vec2 soldierSlot(const SoldierGrid &grid, int index) {
    const int rank = index / std::max(1, grid.files);
    const int file = index % std::max(1, grid.files);
    const float depth = grid.rankSpacing * static_cast<float>(grid.ranks);
    const float frontage = grid.fileSpacing * static_cast<float>(grid.files);
    return {depth * 0.5f - grid.rankSpacing * (static_cast<float>(rank) + 0.5f),
            -frontage * 0.5f + grid.fileSpacing * (static_cast<float>(file) + 0.5f)};
}

BannerKind bannerKind(const Battle &battle, const Unit &unit) {
    if (!isFighting(unit.state)) {
        return BannerKind::None;
    }
    if (unit.side == Side::Ming) {
        // The general's own unit (Li Rusong's guard) carries the standard.
        return battle.typeOf(unit).general ? BannerKind::MingStandard : BannerKind::Ming;
    }
    bool vanguard = unit.vanguard;
    if (!vanguard && unit.origin != kNoUnit && unit.origin < battle.units().size()) {
        vanguard = battle.unit(unit.origin).vanguard;
    }
    return vanguard ? BannerKind::JpVanguard : BannerKind::JpMain;
}

Banner unitBanner(const Battle &battle, const Unit &unit, float alpha, float animTime) {
    Banner banner;
    banner.kind = bannerKind(battle, unit);
    if (banner.kind == BannerKind::None) {
        return banner;
    }
    const Pose pose = interpolatedPose(unit, alpha);
    banner.base = rotated(pose.pos, pose.facing, {-unit.depth * 0.3f, 0.f});
    const float pace = unit.charging ? 7.f : unit.speed > 0.05f ? 4.5f : 2.5f;
    banner.sway = std::sin(animTime * pace + static_cast<float>(unit.id) * 1.7f) * (unit.charging ? 1.2f : 0.7f);
    return banner;
}

void layoutSoldiers(const Unit &unit, const UnitType &type, float alpha, float animTime, std::vector<Soldier> &out) {
    if (unit.state == UnitState::Gone) {
        return;
    }
    const Pose pose = interpolatedPose(unit, alpha);
    const SoldierGrid grid = soldierGrid(type, unit);
    const int count = soldierCount(type, unit.men);
    const SoldierAnim anim = soldierAnim(unit);
    const SoldierKind base = soldierKind(type);
    const bool cavalry = type.cls == UnitClass::Cavalry;
    const float walkRate = cavalry ? kWalkRateCavalry : kWalkRateInfantry;
    const float broken = std::clamp(1.f - presentedBrokenSec(unit.brokenSec, alpha) / kBrokenSec, 0.f, 1.f);
    const float spread = std::clamp(unit.routSec / kRoutSpreadSec, 0.f, 1.f);
    const std::uint32_t id = unit.id;

    for (int i = 0; i < count; ++i) {
        const float phase = soldierNoise(id, i, 3);
        Vec2 local = soldierSlot(grid, i);
        local.x += (soldierNoise(id, i, 1) - 0.5f) * grid.rankSpacing * 0.3f;
        local.y += (soldierNoise(id, i, 2) - 0.5f) * grid.fileSpacing * 0.3f;
        float facing = pose.facing;
        Soldier soldier;
        soldier.unit = unit.id;
        soldier.side = unit.side;
        soldier.kind = type.general && i == grid.files / 2 ? SoldierKind::General : base;
        soldier.frame = kFrameIdle;
        const int rank = i / std::max(1, grid.files);
        const int file = i % std::max(1, grid.files);

        switch (anim) {
        case SoldierAnim::Idle:
            soldier.frame = soldierNoise(id, i, 4) < 0.15f ? kFrameAside : kFrameIdle;
            facing += (soldierNoise(id, i, 5) - 0.5f) * 0.15f;
            break;
        case SoldierAnim::March:
            soldier.frame = cycleFrame(animTime, walkRate, phase);
            break;
        case SoldierAnim::Charge: {
            soldier.frame = cycleFrame(animTime, kChargeRate, phase);
            local.x += std::sin(animTime * 6.f + phase * 2.f * kPi) * 1.2f;
            if (cavalry && soldierNoise(id, i, 6) < 0.5f) {
                const float puff = fract(animTime * 1.5f + phase);
                soldier.dust = (1.f - puff) * 0.55f;
                soldier.dustPos = rotated(pose.pos, pose.facing,
                                          {local.x - grid.rankSpacing * 0.6f - puff * 5.f,
                                           local.y + (soldierNoise(id, i, 7) - 0.5f) * 2.f});
            }
            break;
        }
        case SoldierAnim::Engaged:
            if (rank == 0) {
                // The front rank steps in and out of the line in turn.
                local.x += (file % 2 == 0 ? 1.f : -1.f) * grid.rankSpacing * 0.35f +
                           std::sin(animTime * 5.f + phase * 2.f * kPi) * 0.8f;
                const int strike = static_cast<int>(std::floor(animTime * kStrikeRate + phase * 2.f));
                soldier.frame = kFrameStrike + ((strike % 2) + 2) % 2;
            } else {
                local.x += std::sin(animTime * 2.f + phase * 2.f * kPi) * 0.4f;
                soldier.frame = soldierNoise(id, i, 4) < 0.3f ? kFrameAside : kFrameIdle;
            }
            break;
        case SoldierAnim::Waver:
            soldier.frame = soldierNoise(id, i, 4) < 0.4f ? kFrameAside : kFrameIdle;
            facing += std::sin(animTime * 3.f + phase * 2.f * kPi) * 0.5f;
            local.x += std::sin(animTime * 4.f + phase * 5.f) * 0.5f;
            break;
        case SoldierAnim::Turn: {
            // The unit already faces away from the attacker; each man turns from facing the enemy, some later.
            const float delay = soldierNoise(id, i, 4) * 0.5f;
            const float turn = std::clamp((broken - delay) / 0.5f, 0.f, 1.f);
            const float sign = soldierNoise(id, i, 5) < 0.5f ? 1.f : -1.f;
            facing += kPi * (1.f - turn) * sign;
            soldier.frame = turn > 0.5f ? cycleFrame(animTime, kRoutRate, phase) : kFrameIdle;
            local = local * (1.f + broken * 0.3f);
            break;
        }
        case SoldierAnim::Rout: {
            const float angle = soldierNoise(id, i, 4) * 2.f * kPi;
            local = local * (1.f + spread * 1.4f) +
                    Vec2{std::cos(angle), std::sin(angle)} * (spread * grid.rankSpacing * 2.5f);
            facing += (soldierNoise(id, i, 5) - 0.5f) * 1.4f;
            soldier.frame = cycleFrame(animTime, kRoutRate, phase);
            soldier.shade = 0.8f;
            break;
        }
        }
        soldier.pos = rotated(pose.pos, pose.facing, local);
        soldier.facing = facing;
        soldier.direction = soldierDirection(facing);
        out.push_back(soldier);
    }
}

} // namespace tq
