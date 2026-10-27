#include "Run.h"

#include <cmath>
#include <stdexcept>
#include <string>

namespace tq {
namespace {

bool finiteUnit(const Unit &unit) {
    const auto ok = [](float value) { return std::isfinite(value); };
    if (!ok(unit.pos.x) || !ok(unit.pos.y) || !ok(unit.prevPos.x) || !ok(unit.prevPos.y) || !ok(unit.facing) ||
        !ok(unit.prevFacing) || !ok(unit.frontage) || !ok(unit.depth) || !ok(unit.men) || !ok(unit.maxMen) ||
        !ok(unit.morale) || !ok(unit.engagedSec) || !ok(unit.recoverSec) || !ok(unit.reloadSec) || !ok(unit.stillSec) ||
        !ok(unit.speed) || !ok(unit.runDist) || !ok(unit.momentumSec) || !ok(unit.brokenSec) || !ok(unit.routSec) ||
        !ok(unit.knock.x) || !ok(unit.knock.y)) {
        return false;
    }
    return !unit.scatterAngle || ok(*unit.scatterAngle);
}

const char *endKind(BattleEnd end) {
    switch (end) {
    case BattleEnd::CommanderKilled:
        return "commander_killed";
    case BattleEnd::Withdrawn:
        return "withdrawn";
    case BattleEnd::CommanderFled:
        return "commander_fled";
    case BattleEnd::Routed:
        return "routed";
    case BattleEnd::Swept:
        return "swept";
    case BattleEnd::None:
        return "none";
    }
    return "none";
}

const char *gradeName(Grade grade) {
    switch (grade) {
    case Grade::Decisive:
        return "decisive";
    case Grade::Minor:
        return "minor";
    case Grade::Pyrrhic:
        return "pyrrhic";
    case Grade::Defeat:
        return "defeat";
    case Grade::None:
        return "none";
    }
    return "none";
}

std::optional<UnitId> resolveUnit(const Battle &battle, const std::string &id) {
    if (!id.empty() && id.front() == '#') {
        try {
            return static_cast<UnitId>(std::stoul(id.substr(1)));
        } catch (const std::exception &) {
            return std::nullopt;
        }
    }
    return battle.sceneUnit(id);
}

void submitRecorded(Battle &battle, const ReplayCommand &command) {
    Command issued;
    issued.type = command.type;
    if (command.point) {
        issued.point = *command.point;
    }
    for (const std::string &id : command.units) {
        if (const std::optional<UnitId> unit = resolveUnit(battle, id)) {
            issued.units.push_back(*unit);
        }
    }
    if (command.target) {
        issued.target = resolveUnit(battle, *command.target).value_or(kNoUnit);
    }
    if (!issued.units.empty()) {
        battle.submit(std::move(issued));
    }
}

void note(RunResult &result, const Event &event) {
    if (event.type == Event::Type::ScoutWarning && !result.warnTick) {
        result.warnTick = event.tick;
    } else if (event.type == Event::Type::PhaseChanged && event.value == 2.f && !result.phase2Tick) {
        result.phase2Tick = event.tick;
    } else if (event.type == Event::Type::PhaseChanged && event.value == 3.f && !result.phase3Tick) {
        result.phase3Tick = event.tick;
    } else if (event.type == Event::Type::Reinforcement && !result.reinforceTick) {
        result.reinforceTick = event.tick;
    } else if (event.type == Event::Type::Impact) {
        result.impacts += 1;
    }
}

} // namespace

RunResult runBattle(const GameData &data, const RunRequest &request) {
    BattleOptions options;
    options.aiEnabled = request.ai;
    Battle battle(data.defs, data.scenario, request.seed, options);
    PolicyState state;
    RunResult result;
    result.replay.schema = 1;
    result.replay.build = buildId();
    result.replay.scenario = data.scenario.id;
    result.replay.seed = request.seed;
    result.replay.ai = request.ai;
    std::size_t cursor = 0;
    while (battle.tick() < request.maxTicks && !battle.isOver()) {
        if (request.replay) {
            while (cursor < request.commands.size() && request.commands[cursor].tick == battle.tick() + 1) {
                submitRecorded(battle, request.commands[cursor]);
                cursor += 1;
            }
        } else {
            applyPolicy(battle, request.policy, state, result.replay.commands);
        }
        battle.step();
        if (state.phase3At == 0 && battle.phase() >= 3) {
            state.phase3At = battle.tick();
        }
        for (const Unit &unit : battle.units()) {
            if (!finiteUnit(unit)) {
                result.finite = false;
            }
        }
        for (const Event &event : battle.drainEvents()) {
            note(result, event);
            result.log.push_back(event);
        }
    }
    result.endTick = battle.tick();
    result.timeout = !battle.isOver();
    if (!result.timeout) {
        result.endKind = endKind(battle.result().end);
        result.grade = gradeName(battle.result().grade);
        result.savedRate = battle.result().rate;
    }
    result.mingLost = battle.stats().lostMing;
    result.jpLost = battle.stats().lostJp;
    result.mingBroken = battle.stats().brokenMing;
    result.jpBroken = battle.stats().brokenJp;
    result.bestImpact = battle.stats().bestImpact;
    result.events = static_cast<int>(result.log.size());
    result.hash = battle.stateHash();
    return result;
}

} // namespace tq
