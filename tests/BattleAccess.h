#pragma once

#include "core/sim/Battle.h"

namespace tq::testing {

class BattleAccess {
  public:
    static Unit &unit(Battle &battle, UnitId id) { return battle.units_.at(id); }
    static void contacts(Battle &battle) { battle.contactsStep(); }
    static void combat(Battle &battle) { battle.combatTick(); }
    static void kill(Battle &battle, UnitId id, float amount) { battle.kill(battle.units_.at(id), amount); }
    static void setExitOpen(Battle &battle, bool open) { battle.exitOpen_ = open; }
    static BattleStats &stats(Battle &battle) { return battle.stats_; }
    static void setPhase(Battle &battle, int phase) { battle.phase_ = phase; }
};

} // namespace tq::testing
