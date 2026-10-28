#pragma once

#include "core/defs/ScenarioDef.h"
#include "core/defs/UnitType.h"
#include "core/sim/Command.h"
#include "core/sim/Event.h"
#include "core/sim/Rng.h"
#include "core/sim/Unit.h"

#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
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

enum class BattleEnd : std::uint8_t { None, CommanderKilled, Withdrawn, CommanderFled, Routed, Swept };

enum class Grade : std::uint8_t { None, Decisive, Minor, Pyrrhic, Defeat };

struct BattleResult {
    bool over = false;
    BattleEnd end = BattleEnd::None;
    Grade grade = Grade::None;
    float saved = 0.f;
    float rate = 0.f;
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
    std::optional<UnitId> sceneUnit(const std::string &id) const;
    std::optional<std::string> sceneName(UnitId id) const;
    const UnitType &typeOf(const Unit &unit) const;
    int phase() const { return phase_; }
    bool exitOpen() const { return exitOpen_; }
    Terrain terrainAt(Vec2 point) const;
    bool isOver() const { return result_.over; }
    const BattleResult &result() const { return result_; }
    const BattleStats &stats() const { return stats_; }
    // 开局兵力加上后来生成的援军。残部和分队不另计。只读，不参与结算。
    float enlisted(Side side) const;
    std::uint64_t stateHash() const;

  private:
    friend class testing::BattleAccess;
    void apply(const Command &cmd);
    void aiThink();
    void advancePhase();
    void spawnGroup(const std::string &groupId);
    void checkVictory();
    const Unit *commander() const;
    int vanguardBroken() const;
    bool anyAlive(Side side) const;
    void moveUnit(Unit &unit);
    void routMove(Unit &unit);
    void clampPos(Unit &unit) const;
    void contactsStep();
    void combatTick();
    void tryFire(Unit &shooter);
    const Unit *nearGeneral(const Unit &unit) const;
    void impact(Unit &attacker, Unit &target);
    void mingVolley(Unit &attacker, Unit &target);
    void breakUnit(Unit &unit, UnitId from = kNoUnit);
    void shatter(UnitId id, int parts, UnitId from);
    void mergeStep(UnitId limit);
    void splitUnit(UnitId id);
    Unit makeUnit(std::uint16_t typeIndex, Vec2 pos, float facing, bool countTotal, bool rollReload);
    std::uint16_t nextFrag(UnitId origin);
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
    float totalMing_ = 0.f;
    float totalJp_ = 0.f;
    std::vector<std::uint16_t> fragCount_;
    std::map<std::string, UnitId> sceneIds_;
    std::vector<UnitId> vanguardIds_;
    bool vanAggro_ = false;
    bool warned_ = false;
    std::uint32_t mainAt_ = 0;
    std::uint32_t phase2At_ = 0;
    std::uint32_t phase3At_ = 0;
    std::vector<bool> reinforcementArrived_;
};

bool canSplit(const Battle &battle, UnitId id);
std::vector<std::vector<UnitId>> joinGroups(const Battle &battle, std::span<const UnitId> selection);

} // namespace tq
