#pragma once

#include "app/FormationTuning.h"
#include "app/Soldiers.h"
#include "core/sim/Battle.h"
#include "core/sim/Event.h"

#include <cstdint>
#include <span>
#include <vector>

namespace tq {

// Sub-steps the soldier layer takes per simulation tick. The layer advances only with the simulation, so the same
// replay gives the same soldiers at the same tick whatever the frame rate, the speed, or --freeze-at.
inline constexpr int kFormationSubsteps = 4;

// How loose a unit's ranks are, 0 to 1. This is the one value the soldier layer takes from each unit beyond its pose.
// For now the presentation layer estimates it from the unit state; once the rules core has formation integrity
// (rules phase 2), that value replaces this estimate.
float unitLooseness(const Battle &battle, const Unit &unit, const FormationTuning &tuning);

// A fallen soldier. Bodies stay where they fell until the battle ends.
struct FallenSoldier {
    Vec2 pos{};
    float facing = 0.f;
    Side side = Side::Ming;
    SoldierKind kind = SoldierKind::Samurai;
    // Battlefield seconds since the fall.
    float age = 0.f;
};

// Soldiers with state (V5). Each soldier keeps a position and a velocity and chases a target: its slot in the
// unit's grid plus deformation offsets driven by the unit state and the battle events. Presentation only: it reads
// the battle and never writes it, and its noise is hashed from soldier seeds, never drawn from tq::Rng.
class FormationLayer {
  public:
    void reset();
    void setTuning(const FormationTuning &tuning) { tuning_ = tuning; }
    const FormationTuning &tuning() const { return tuning_; }

    // Call once after every Battle::step() with that step's events.
    void tick(const Battle &battle, std::span<const Event> events);

    // Soldiers for drawing, interpolated between the last two ticks by alpha.
    void layout(const Battle &battle, float alpha, float animTime, std::vector<Soldier> &out) const;
    const std::vector<FallenSoldier> &bodies() const { return bodies_; }
    float bodyAlpha(const FallenSoldier &body) const;

    // The unit whose soldier is nearest to a world point within radius, or kNoUnit.
    UnitId pick(const Battle &battle, Vec2 world, float radius, float alpha) const;

    // FNV-1a over every soldier and body, for determinism tests.
    std::uint64_t hash() const;
    std::size_t livingSoldiers() const;
    // A unit calm for kSettleSec keeps every soldier within the clamp margin of its rules box.
    bool settled(UnitId unit) const { return unit < units_.size() && units_[unit].calmSec >= kSettleSec; }
    static constexpr float kSettleSec = 2.f;

  private:
    struct Trooper {
        Vec2 pos{};
        Vec2 vel{};
        Vec2 prev{};
        float facing = 0.f;
        float prevFacing = 0.f;
        float stride = 0.f;
        int slot = 0;
        std::uint32_t seed = 0;
        // In the front band of a melee this substep; drawn striking.
        bool fighting = false;
    };
    struct Dent {
        Vec2 local{};
        Vec2 dir{};
        float depth = 0.f;
        float age = 0.f;
        bool thrust = false;
    };
    struct Body {
        std::vector<Trooper> troopers;
        std::vector<Dent> dents;
        bool started = false;
        Vec2 threat{1.f, 0.f};
        bool hasThreat = false;
        float rallyAge = -1.f;
        // Battlefield seconds since the unit was last engaged, broken, routing, or rallying.
        float calmSec = 0.f;
        std::uint32_t nextSeed = 0;
    };

    void apply(const Battle &battle, const Event &event, std::size_t knownUnits);
    void shatter(const Battle &battle, UnitId source, std::size_t knownUnits);
    void split(const Battle &battle, UnitId left, UnitId right);
    void settleCount(const Battle &battle, const Unit &unit, Body &body);
    void substep(const Battle &battle, float fraction, float dt);
    struct Aim {
        Vec2 pos{};
        float facing = 0.f;
        float stiffness = 1.f;
        bool fighting = false;
    };
    Aim target(const Battle &battle, const Unit &unit, const Body &body, const Trooper &trooper, float fraction) const;
    void separate(Body &body, float spacing);
    void separatePair(Body &left, Body &right, float spacing);
    void fall(Side side, SoldierKind kind, const Trooper &trooper);
    static SoldierKind kindOf(const UnitType &type, const Trooper &trooper, int files);
    Trooper fresh(Body &body, std::uint32_t unitId, Vec2 pos, float facing);

    FormationTuning tuning_;
    std::vector<Body> units_;
    std::vector<FallenSoldier> bodies_;
};

} // namespace tq
