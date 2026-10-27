#include "Policy.h"

#include <string>

namespace tq {
namespace {

bool alive(const Unit &unit) { return unit.state != UnitState::Gone; }

bool fighting(const Unit &unit) { return unit.state == UnitState::Steady || unit.state == UnitState::Wavering; }

void record(Battle &battle, std::vector<ReplayCommand> &recorded, ReplayCommand command,
            const std::vector<UnitId> &units, UnitId target) {
    if (units.empty()) {
        return;
    }
    Command issued;
    issued.type = command.type;
    issued.units = units;
    issued.target = target;
    if (command.point) {
        issued.point = *command.point;
    }
    command.tick = battle.tick() + 1;
    recorded.push_back(command);
    battle.submit(std::move(issued));
}

std::vector<UnitId> resolve(const Battle &battle, const std::vector<std::string> &ids) {
    std::vector<UnitId> units;
    for (const std::string &id : ids) {
        if (const std::optional<UnitId> found = battle.sceneUnit(id)) {
            units.push_back(*found);
        }
    }
    return units;
}

} // namespace

void applyPolicy(Battle &battle, PolicyKind policy, PolicyState &state, std::vector<ReplayCommand> &recorded) {
    if (policy != PolicyKind::Scripted) {
        return;
    }
    const std::uint32_t tick = battle.tick();
    if (tick == 0) {
        record(battle, recorded,
               ReplayCommand{0, Command::Type::Attack, {"liaodong_right"}, std::nullopt, "van_teppo_right"},
               resolve(battle, {"liaodong_right"}), battle.sceneUnit("van_teppo_right").value_or(kNoUnit));
        record(battle, recorded,
               ReplayCommand{0, Command::Type::Attack, {"jiading_left"}, std::nullopt, "van_teppo_left"},
               resolve(battle, {"jiading_left"}), battle.sceneUnit("van_teppo_left").value_or(kNoUnit));
    }
    if (!state.samuraiOrdered && tick >= 300) {
        const std::optional<UnitId> riders = battle.sceneUnit("jiading_right");
        const std::optional<UnitId> samurai = battle.sceneUnit("van_samurai");
        if (riders && samurai && alive(battle.unit(*samurai)) && fighting(battle.unit(*riders))) {
            record(battle, recorded,
                   ReplayCommand{0, Command::Type::Attack, {"jiading_right"}, std::nullopt, "van_samurai"}, {*riders},
                   *samurai);
            state.samuraiOrdered = true;
        }
    }
    if (state.phase3At > 0 && !state.withdrew && tick + 1 >= state.phase3At + 300) {
        for (const Unit &unit : battle.units()) {
            if (unit.side != Side::Ming || !fighting(unit)) {
                continue;
            }
            const std::string scene = battle.sceneName(unit.id).value_or("#" + std::to_string(unit.id));
            record(battle, recorded, ReplayCommand{0, Command::Type::Move, {scene}, Vec2{440.f, 20.f}, std::nullopt},
                   {unit.id}, kNoUnit);
        }
        state.withdrew = true;
    }
}

} // namespace tq
