#pragma once

#include "core/defs/ScenarioDef.h"
#include "core/defs/UnitType.h"
#include "core/sim/Command.h"
#include "core/sim/Event.h"
#include "core/sim/Rng.h"
#include "core/sim/Unit.h"

#include <cstdint>
#include <span>
#include <vector>

namespace tq {

namespace testing {
class BattleAccess;
}

struct BattleOptions {
    bool aiEnabled = true;
};

struct BattleStats {
    float lostMing = 0.f;
    float lostJp = 0.f;
    int brokenMing = 0;
    int brokenJp = 0;
    int impacts = 0;
    float bestImpact = 0.f;
    float exitedMen = 0.f;
};

struct BattleResult {
    bool over = false;
};

class Battle {
  public:
    Battle(const GameDefs &defs, const ScenarioDef &scenario, std::uint64_t seed, BattleOptions opts = {});

    void submit(Command cmd);
    void step();
    std::vector<Event> drainEvents();

    std::uint32_t tick() const { return tick_; }
    float battleSeconds() const { return static_cast<float>(tick_) * kTickSeconds; }
    std::span<const Unit> units() const { return units_; }
    const Unit &unit(UnitId id) const;
    const UnitType &typeOf(const Unit &unit) const;
    int phase() const { return phase_; }
    bool exitOpen() const { return exitOpen_; }
    Terrain terrainAt(Vec2 point) const;
    bool isOver() const { return result_.over; }
    const BattleResult &result() const { return result_; }
    const BattleStats &stats() const { return stats_; }
    std::uint64_t stateHash() const;

  private:
    friend class testing::BattleAccess;
    void apply(const Command &cmd);
    void moveUnit(Unit &unit);
    void routMove(Unit &unit);
    void clampPos(Unit &unit) const;
    void contactsStep();
    void impact(Unit &attacker, Unit &target);
    void mingVolley(Unit &attacker, Unit &target);
    void breakUnit(Unit &unit);
    void kill(Unit &unit, float amount);
    void generalDown(Unit &unit);
    void disperse(Unit &unit);
    void fled(Unit &unit);
    void withdraw(Unit &unit);
    const Unit *nearestEnemy(const Unit &self, float limit) const;
    const Unit *primaryContact(const Unit &self) const;
    void emit(Event::Type type, UnitId a, Vec2 pos);

    GameDefs defs_;
    ScenarioDef scenario_;
    BattleOptions options_;
    Rng rng_;
    std::vector<Unit> units_;
    std::vector<Command> commands_;
    std::vector<Event> events_;
    BattleStats stats_;
    BattleResult result_;
    std::uint32_t tick_ = 0;
    int phase_ = 0;
    bool exitOpen_ = false;
};

} // namespace tq
