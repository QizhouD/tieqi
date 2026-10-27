#pragma once

#include "core/sim/Battle.h"

namespace tq::testing {

class BattleAccess {
  public:
    static Unit &unit(Battle &battle, UnitId id) { return battle.units_.at(id); }
    static void contacts(Battle &battle) { battle.contactsStep(); }
    static void setExitOpen(Battle &battle, bool open) { battle.exitOpen_ = open; }
};

} // namespace tq::testing
