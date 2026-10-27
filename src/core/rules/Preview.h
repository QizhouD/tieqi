#pragma once

#include "core/sim/Battle.h"

#include <cstdint>
#include <vector>

namespace tq {

enum class ModKind : std::uint8_t {
    Impact,
    Melee,
    Defense,
    Speed,
    Flank,
    Rear,
    Wavering,
    Routing,
    Reloading,
    Unformed,
    Paddy,
    Uphill,
    Pike,
    PikeWall,
    Momentum,
    OwnWavering,
    Strength,
    CavalryGrind,
    Pursuit
};

enum class NoteKind : std::uint8_t { Recovering, TooClose, VolleyThreat, OwnWavering };

enum class Verdict : std::uint8_t { Breaks, Favored, Even, Unfavored, PikeWall, Pursuit };

struct Modifier {
    ModKind kind = ModKind::Impact;
    float value = 0.f;
};

struct Preview {
    enum class Kind : std::uint8_t { Charge, Melee } kind = Kind::Melee;
    float ratio = 0.f;
    ContactSide side = ContactSide::Front;
    Verdict verdict = Verdict::Unfavored;
    std::vector<Modifier> mods;
    std::vector<NoteKind> notes;
};

Preview previewAttack(const Battle &battle, UnitId attacker, UnitId target);

} // namespace tq
