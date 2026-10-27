#include "core/ai/Ai.h"

#include "core/sim/Battle.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

namespace tq {
namespace {

bool alive(const Unit &unit) { return unit.state != UnitState::Gone; }

bool fighting(const Unit &unit) { return unit.state == UnitState::Steady || unit.state == UnitState::Wavering; }

float distance(Vec2 from, Vec2 to) {
    const Vec2 delta = to - from;
    return std::sqrt(dot(delta, delta));
}

void holdFree(Unit &unit) {
    unit.order = {};
    unit.order.type = Order::Type::Hold;
}

void orderAttack(Unit &unit, UnitId target) {
    unit.order = {};
    unit.order.type = Order::Type::Attack;
    unit.order.target = target;
}

void orderMove(Unit &unit, Vec2 point, float face) {
    unit.order = {};
    unit.order.type = Order::Type::Move;
    unit.order.point = point;
    unit.order.face = face;
}

const Unit *nearestMing(const Battle &battle, const Unit &self, bool fightingOnly) {
    const Unit *best = nullptr;
    float bestDistance = 0.f;
    for (const Unit &other : battle.units()) {
        if (other.side != Side::Ming || !alive(other)) {
            continue;
        }
        if (fightingOnly && !fighting(other)) {
            continue;
        }
        const float gap = distance(self.pos, other.pos);
        if (best == nullptr || gap < bestDistance || (gap == bestDistance && other.id < best->id)) {
            best = &other;
            bestDistance = gap;
        }
    }
    return best;
}

} // namespace

float scoreTarget(const Unit &self, const UnitType &selfType, const Unit &target, const UnitType &targetType,
                  std::span<const Unit> units, const GameDefs &defs) {
    float score = 1.f / (distance(self.pos, target.pos) + kAiScoreDist);
    if (target.state == UnitState::Wavering) {
        score *= kAiScoreWaver;
    }
    if (!fighting(target)) {
        score *= selfType.ai == AiRole::Shock ? kAiScoreUnfitShock : kAiScoreUnfit;
    }
    if (targetType.general) {
        score *= selfType.ai == AiRole::Shock ? kAiScoreGeneralShock : kAiScoreGeneral;
    }
    if (target.recoverSec > 0.f) {
        score *= kAiScoreRecover;
    }
    if (selfType.ai == AiRole::PikeLine) {
        for (const Unit &other : units) {
            if (other.side != Side::Jp || !fighting(other) || defs.types.at(other.type).ai != AiRole::Gunner) {
                continue;
            }
            if (distance(other.pos, target.pos) < kAiPikeGunMeters) {
                score *= kAiScorePikeGun;
                break;
            }
        }
    }
    return score;
}

void approach(Unit &self, const Unit &focus, float keep, float mapWidth, float mapHeight) {
    const Vec2 delta = self.pos - focus.pos;
    const float gap = std::sqrt(dot(delta, delta));
    if (gap <= keep || gap == 0.f) {
        return;
    }
    const Vec2 point{std::clamp(focus.pos.x + delta.x / gap * keep, kOrderEdge, mapWidth - kOrderEdge),
                     std::clamp(focus.pos.y + delta.y / gap * keep, kOrderEdge, mapHeight - kOrderEdge)};
    orderMove(self, point, std::atan2(focus.pos.y - self.pos.y, focus.pos.x - self.pos.x));
}

void Battle::aiThink() {
    std::vector<UnitId> ming;
    std::vector<UnitId> japanese;
    for (const Unit &unit : units_) {
        if (unit.side == Side::Ming && alive(unit)) {
            ming.push_back(unit.id);
        } else if (unit.side == Side::Jp && fighting(unit)) {
            japanese.push_back(unit.id);
        }
    }
    if (ming.empty()) {
        return;
    }
    if (!vanAggro_) {
        const auto holdTicks = static_cast<std::uint32_t>(
            std::lround(static_cast<double>(scenario_.ai.vanguardHoldSec) / static_cast<double>(kTickSeconds)));
        if (tick_ > holdTicks) {
            vanAggro_ = true;
        } else {
            for (UnitId id : japanese) {
                const Unit &unit = units_[id];
                if (!unit.vanguard) {
                    continue;
                }
                for (UnitId target : ming) {
                    if (distance(unit.pos, units_[target].pos) < scenario_.ai.vanguardTriggerDist) {
                        vanAggro_ = true;
                        break;
                    }
                }
                if (vanAggro_) {
                    break;
                }
            }
        }
    }
    for (UnitId id : japanese) {
        Unit &unit = units_[id];
        if (!unit.contacts.empty()) {
            continue;
        }
        const Unit *near = nearestMing(*this, unit, false);
        if (near == nullptr) {
            continue;
        }
        const float nearDistance = distance(unit.pos, near->pos);
        if (unit.deploying) {
            if (unit.order.type == Order::Type::Move && nearDistance > kAiDeployMeters) {
                continue;
            }
            unit.deploying = false;
        }
        const bool aggressive = unit.vanguard ? vanAggro_ : true;
        if (unit.isFragment) {
            const Unit *mate = nullptr;
            float mateDistance = kAiFragSeek;
            for (UnitId otherId : japanese) {
                const Unit &other = units_[otherId];
                if (other.id == unit.id || !other.isFragment || other.origin != unit.origin ||
                    !other.contacts.empty()) {
                    continue;
                }
                const float gap = distance(unit.pos, other.pos);
                if (gap < mateDistance) {
                    mate = &other;
                    mateDistance = gap;
                }
            }
            if (mate != nullptr && nearDistance > kAiFragNear) {
                orderMove(unit, mate->pos, -std::numbers::pi_v<float> / 2.f);
                continue;
            }
        }
        const UnitType &type = typeOf(unit);
        if (type.ai == AiRole::Gunner) {
            const Unit *focus = nearestMing(*this, unit, true);
            if (focus == nullptr) {
                focus = near;
            }
            const float focusDistance = distance(unit.pos, focus->pos);
            if (aggressive && focusDistance > type.range + kAiGunnerAdvance) {
                approach(unit, *focus, type.range - kAiGunnerKeep, scenario_.map.width, scenario_.map.height);
            } else if (unit.order.type == Order::Type::Move && focusDistance <= type.range - kAiGunnerHalt) {
                holdFree(unit);
            } else if (unit.order.type == Order::Type::Attack) {
                holdFree(unit);
            }
        } else if (type.ai == AiRole::MainGeneral) {
            if (nearDistance < kAiAnchorEngage) {
                orderAttack(unit, near->id);
                continue;
            }
            Vec2 sum;
            int count = 0;
            for (UnitId otherId : japanese) {
                const Unit &other = units_[otherId];
                if (typeOf(other).general) {
                    continue;
                }
                sum = sum + other.pos;
                count += 1;
            }
            if (count > 0) {
                const float cx = sum.x / static_cast<float>(count);
                const float cy = sum.y / static_cast<float>(count);
                const float anchorY = std::min(scenario_.map.height - kAiRearPad, cy + kAiAnchorDepth);
                const Vec2 anchor{cx, anchorY};
                if (distance(unit.pos, anchor) > kAiAnchorSlop) {
                    orderMove(unit, anchor, -std::numbers::pi_v<float> / 2.f);
                }
            }
        } else if (type.ai == AiRole::VanGeneral) {
            const Unit *mate = nullptr;
            for (UnitId otherId : japanese) {
                const Unit &other = units_[otherId];
                if (other.id == unit.id || other.contacts.empty() ||
                    distance(unit.pos, other.pos) >= kAiSupportMeters) {
                    continue;
                }
                mate = &other;
                break;
            }
            if (mate != nullptr) {
                UnitId target = mate->contacts.front();
                for (UnitId contact : mate->contacts) {
                    if (contact < units_.size() && fighting(units_[contact])) {
                        target = contact;
                        break;
                    }
                }
                orderAttack(unit, target);
                continue;
            }
            const Unit *samurai = nullptr;
            for (UnitId otherId : japanese) {
                const Unit &other = units_[otherId];
                if (typeOf(other).ai == AiRole::Shock && other.vanguard) {
                    samurai = &other;
                    break;
                }
            }
            if (samurai != nullptr && samurai->contacts.empty()) {
                const Vec2 back =
                    samurai->pos - Vec2{std::cos(samurai->facing), std::sin(samurai->facing)} * kAiFollowMeters;
                orderMove(unit, back, samurai->facing);
            } else if (nearDistance < kAiVanCharge && aggressive) {
                orderAttack(unit, near->id);
            }
        } else if (type.ai == AiRole::PikeLine || type.ai == AiRole::Shock) {
            if (!aggressive) {
                if (unit.order.type != Order::Type::Hold) {
                    holdFree(unit);
                }
                continue;
            }
            const Unit *best = nullptr;
            float bestScore = 0.f;
            for (UnitId target : ming) {
                const Unit &candidate = units_[target];
                const float score = scoreTarget(unit, type, candidate, typeOf(candidate), units_, defs_);
                if (best == nullptr || score > bestScore) {
                    best = &candidate;
                    bestScore = score;
                }
            }
            if (unit.order.type == Order::Type::Attack && unit.order.target < units_.size() &&
                alive(units_[unit.order.target])) {
                const Unit &current = units_[unit.order.target];
                const float currentScore = scoreTarget(unit, type, current, typeOf(current), units_, defs_);
                if (currentScore >= bestScore * kAiSwitchKeep) {
                    continue;
                }
            }
            if (best != nullptr) {
                orderAttack(unit, best->id);
            }
        }
    }
}

} // namespace tq
