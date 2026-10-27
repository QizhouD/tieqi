#include "core/rules/Preview.h"

#include "core/math/Geometry.h"
#include "core/rules/Combat.h"

#include <cmath>

namespace tq {
namespace {

bool fighting(const Unit &unit) { return unit.state == UnitState::Steady || unit.state == UnitState::Wavering; }

float lengthOf(Vec2 value) { return std::sqrt(dot(value, value)); }

void addMod(Preview &preview, ModKind kind, float value) { preview.mods.push_back(Modifier{kind, value}); }

Verdict chargeVerdict(const ChargeDetail &detail, const Unit &target) {
    if (detail.pikeFront) {
        return Verdict::PikeWall;
    }
    if (!fighting(target)) {
        return Verdict::Pursuit;
    }
    if (detail.ratio >= kImpactBreak) {
        return Verdict::Breaks;
    }
    if (detail.ratio >= kPreviewFavored) {
        return Verdict::Favored;
    }
    if (detail.ratio >= kPreviewEven) {
        return Verdict::Even;
    }
    return Verdict::Unfavored;
}

void addChargeMods(Preview &preview, const ChargeDetail &detail) {
    if (detail.base != 1.f) {
        addMod(preview, ModKind::Impact, detail.base);
    }
    if (detail.speed < kModSpeed) {
        addMod(preview, ModKind::Speed, detail.speed);
    }
    if (detail.side == ContactSide::Flank) {
        addMod(preview, ModKind::Flank, detail.sideMul);
    } else if (detail.side == ContactSide::Rear) {
        addMod(preview, ModKind::Rear, detail.sideMul);
    }
    if (detail.stateMul == kChargeWavering) {
        addMod(preview, ModKind::Wavering, detail.stateMul);
    } else if (detail.stateMul == kChargeBroken) {
        addMod(preview, ModKind::Routing, detail.stateMul);
    }
    if (detail.gunMul == kChargeReloadMul) {
        addMod(preview, ModKind::Reloading, detail.gunMul);
    } else if (detail.gunMul == kChargeUnformedMul) {
        addMod(preview, ModKind::Unformed, detail.gunMul);
    }
    if (detail.paddy) {
        addMod(preview, ModKind::Paddy, kChargePaddy);
    }
    if (detail.uphill) {
        addMod(preview, ModKind::Uphill, kChargeUphill);
    }
    if (detail.pikeFront) {
        addMod(preview, ModKind::Pike, detail.pikeMul);
    }
    if (detail.momentum != 1.f) {
        addMod(preview, ModKind::Momentum, detail.momentum);
    }
    if (detail.ownWaver != 1.f) {
        addMod(preview, ModKind::OwnWavering, detail.ownWaver);
    }
    if (detail.strength < kModStrength) {
        addMod(preview, ModKind::Strength, detail.strength);
    }
    if (std::fabs(detail.defense - 1.f) > kModDefense) {
        addMod(preview, ModKind::Defense, 1.f / detail.defense);
    }
}

} // namespace

Preview previewAttack(const Battle &battle, UnitId attackerId, UnitId targetId) {
    const Unit &attacker = battle.unit(attackerId);
    const Unit &target = battle.unit(targetId);
    const UnitType &attackerType = battle.typeOf(attacker);
    const UnitType &targetType = battle.typeOf(target);
    const float distance = lengthOf(target.pos - attacker.pos);
    const bool canCharge = attackerType.run > 0.f && attacker.recoverSec <= 0.f && distance > kChargeMinMeters;

    Preview preview;
    if (attackerType.run > 0.f && attacker.recoverSec > 0.f) {
        preview.notes.push_back(NoteKind::Recovering);
    }
    if (attackerType.run > 0.f && distance <= kChargeMinMeters) {
        preview.notes.push_back(NoteKind::TooClose);
    }
    if (canCharge) {
        for (const Unit &other : battle.units()) {
            if (other.side == attacker.side || !fighting(other)) {
                continue;
            }
            const UnitType &otherType = battle.typeOf(other);
            if (otherType.range <= 0.f || other.reloadSec > kVolleyReloadSec || other.stillSec < kVolleyStillSec) {
                continue;
            }
            if (lengthOf(other.pos - target.pos) >= kVolleyNearTarget) {
                continue;
            }
            if (lengthOf(other.pos - attacker.pos) >= otherType.range + kVolleyApproach) {
                continue;
            }
            const float aim = std::atan2(attacker.pos.y - other.pos.y, attacker.pos.x - other.pos.x);
            if (std::fabs(angDiff(aim, other.facing)) >= kVolleyAim) {
                continue;
            }
            preview.notes.push_back(NoteKind::VolleyThreat);
            break;
        }
    }
    if (attacker.state == UnitState::Wavering) {
        preview.notes.push_back(NoteKind::OwnWavering);
    }

    preview.side = sideOf(target.pos, target.facing, target.depth, target.frontage, attacker.pos);
    if (!canCharge) {
        const MeleeDetail detail = meleeCalc(attacker, attackerType, target, targetType, battle.terrainAt(attacker.pos),
                                             battle.terrainAt(target.pos));
        preview.kind = Preview::Kind::Melee;
        preview.ratio = detail.ratio;
        preview.side = detail.side;
        if (detail.ratio >= kPreviewMeleeFavored) {
            preview.verdict = Verdict::Favored;
        } else if (detail.ratio >= kPreviewEven) {
            preview.verdict = Verdict::Even;
        } else {
            preview.verdict = Verdict::Unfavored;
        }
        if (detail.power != 1.f) {
            addMod(preview, ModKind::Melee, detail.power);
        }
        if (detail.pair < 1.f) {
            addMod(preview, ModKind::PikeWall, detail.pair);
        }
        if (detail.grind) {
            addMod(preview, ModKind::CavalryGrind, kMeleeGrind);
        }
        if (detail.sideMul > 1.f) {
            const ModKind kind = detail.pursuit ? ModKind::Pursuit
                                                : (detail.side == ContactSide::Flank ? ModKind::Flank : ModKind::Rear);
            addMod(preview, kind, detail.sideMul);
        }
        if (detail.defender != 1.f) {
            addMod(preview, ModKind::Melee, detail.defender);
        }
        return preview;
    }
    const ChargeDetail detail = chargeCalc(attacker, attackerType, target, targetType, battle.terrainAt(attacker.pos),
                                           battle.terrainAt(target.pos), true);
    preview.kind = Preview::Kind::Charge;
    preview.ratio = detail.ratio;
    preview.side = detail.side;
    preview.verdict = chargeVerdict(detail, target);
    addChargeMods(preview, detail);
    return preview;
}

} // namespace tq
