#pragma once

#include <array>
#include <cstddef>

namespace tq {

// Every deformation parameter of the soldier layer (V5). Presentation only: none of this reaches the rules core,
// and none of it belongs in src/core/rules/Constants.h. Distances are metres, times battlefield seconds.
// The dev tools panel shows each field as a slider (see kFormationSliders).
struct FormationTuning {
    // Spring stiffness (natural frequency, 1/s) toward the target point.
    float stiffness = 1.6f;
    float chargeStiffness = 3.0f;
    float routStiffness = 2.2f;
    float rallyStiffness = 0.7f;
    // How far looseness 1 spreads the grid, as a fraction of the slot offset, and the per-soldier wobble.
    float looseSpread = 0.8f;
    float looseJitter = 2.5f;
    // Charge: the middle bulges forward, fast horses lead, and ranks open up with the distance run.
    float chargeBulge = 7.f;
    float chargeLead = 5.f;
    float chargeOpenPerMeter = 0.006f;
    float chargeOpenMax = 0.6f;
    // Impact: the attacker's front drives in up to this fraction of the defender's depth; the defender is dented.
    float impactDepthFraction = 0.34f;
    float impactRadius = 16.f;
    float impactFlank = 1.5f;
    float impactRear = 2.f;
    float impactRelaxSec = 6.f;
    // Melee: the contact line becomes a ragged band; overlapping ends curl inward.
    float meleeBand = 3.f;
    float meleeCurl = 0.6f;
    // Breakaway and rout: stragglers chase their target this much more slowly.
    float stragglerShare = 0.18f;
    float stragglerLag = 0.35f;
    // Rout: the body stretches into a stream behind its front.
    float routSpread = 1.4f;
    float routStream = 1.2f;
    // Rally and merge: gather on the banner, then re-form ranks.
    float gatherSec = 4.f;
    float reformSec = 8.f;
    float gatherRadius = 6.f;
    // Extra looseness in paddies.
    float paddyLoose = 0.3f;
    // Bodies fade to bodyMinAlpha over bodyFadeSec and stay until the battle ends; the oldest go past bodyCap.
    float bodyFadeSec = 30.f;
    float bodyMinAlpha = 0.5f;
    float bodyCap = 2400.f;
    // Top running speed of a soldier, m/s (cavalry, infantry); the spring never moves anyone faster.
    float maxSpeedCavalry = 12.f;
    float maxSpeedInfantry = 5.f;
    // Soldiers closer than this push each other apart (cavalry, infantry).
    float spacingCavalry = 2.4f;
    float spacingInfantry = 1.5f;
    // A unit that is not engaged, shattering, or routing keeps its soldiers within this fraction of its depth
    // outside the rules box.
    float clampOutside = 0.25f;
};

struct FormationSlider {
    const char *name;
    float FormationTuning::*field;
    float min;
    float max;
};

inline constexpr std::array<FormationSlider, 33> kFormationSliders{{
    {"stiffness", &FormationTuning::stiffness, 0.2f, 6.f},
    {"charge stiffness", &FormationTuning::chargeStiffness, 0.2f, 8.f},
    {"rout stiffness", &FormationTuning::routStiffness, 0.2f, 8.f},
    {"rally stiffness", &FormationTuning::rallyStiffness, 0.1f, 4.f},
    {"loose spread", &FormationTuning::looseSpread, 0.f, 2.f},
    {"loose jitter (m)", &FormationTuning::looseJitter, 0.f, 8.f},
    {"charge bulge (m)", &FormationTuning::chargeBulge, 0.f, 20.f},
    {"charge lead (m)", &FormationTuning::chargeLead, 0.f, 15.f},
    {"charge open per m", &FormationTuning::chargeOpenPerMeter, 0.f, 0.03f},
    {"charge open max", &FormationTuning::chargeOpenMax, 0.f, 1.5f},
    {"impact depth (of defender)", &FormationTuning::impactDepthFraction, 0.f, 0.8f},
    {"impact radius (m)", &FormationTuning::impactRadius, 2.f, 40.f},
    {"impact flank x", &FormationTuning::impactFlank, 1.f, 3.f},
    {"impact rear x", &FormationTuning::impactRear, 1.f, 4.f},
    {"impact relax (s)", &FormationTuning::impactRelaxSec, 0.5f, 20.f},
    {"melee band (m)", &FormationTuning::meleeBand, 0.f, 10.f},
    {"melee curl", &FormationTuning::meleeCurl, 0.f, 2.f},
    {"straggler share", &FormationTuning::stragglerShare, 0.f, 0.6f},
    {"straggler lag", &FormationTuning::stragglerLag, 0.05f, 1.f},
    {"rout spread", &FormationTuning::routSpread, 0.f, 4.f},
    {"rout stream", &FormationTuning::routStream, 0.f, 6.f},
    {"gather (s)", &FormationTuning::gatherSec, 0.f, 15.f},
    {"re-form (s)", &FormationTuning::reformSec, 0.5f, 30.f},
    {"gather radius (m)", &FormationTuning::gatherRadius, 1.f, 20.f},
    {"paddy loose", &FormationTuning::paddyLoose, 0.f, 1.f},
    {"body fade (s)", &FormationTuning::bodyFadeSec, 1.f, 120.f},
    {"body min alpha", &FormationTuning::bodyMinAlpha, 0.f, 1.f},
    {"body cap", &FormationTuning::bodyCap, 0.f, 6000.f},
    {"max speed cavalry (m/s)", &FormationTuning::maxSpeedCavalry, 2.f, 30.f},
    {"max speed infantry (m/s)", &FormationTuning::maxSpeedInfantry, 1.f, 15.f},
    {"spacing cavalry (m)", &FormationTuning::spacingCavalry, 0.5f, 5.f},
    {"spacing infantry (m)", &FormationTuning::spacingInfantry, 0.5f, 4.f},
    {"clamp outside (of depth)", &FormationTuning::clampOutside, 0.f, 1.f},
}};

} // namespace tq
