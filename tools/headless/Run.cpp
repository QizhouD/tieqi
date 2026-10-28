#include "Run.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

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

const char *eventName(Event::Type type) {
    switch (type) {
    case Event::Type::Impact:
        return "Impact";
    case Event::Type::ThreeEyedVolley:
        return "ThreeEyedVolley";
    case Event::Type::TeppoVolley:
        return "TeppoVolley";
    case Event::Type::Breakaway:
        return "Breakaway";
    case Event::Type::Broken:
        return "Broken";
    case Event::Type::Shattered:
        return "Shattered";
    case Event::Type::Rallied:
        return "Rallied";
    case Event::Type::Merged:
        return "Merged";
    case Event::Type::Split:
        return "Split";
    case Event::Type::Dispersed:
        return "Dispersed";
    case Event::Type::Fled:
        return "Fled";
    case Event::Type::Withdrawn:
        return "Withdrawn";
    case Event::Type::GeneralDown:
        return "GeneralDown";
    case Event::Type::MomentumGained:
        return "MomentumGained";
    case Event::Type::ScoutWarning:
        return "ScoutWarning";
    case Event::Type::PhaseChanged:
        return "PhaseChanged";
    case Event::Type::Reinforcement:
        return "Reinforcement";
    case Event::Type::GrandAssault:
        return "GrandAssault";
    case Event::Type::BattleEnded:
        return "BattleEnded";
    }
    return "Impact";
}

const char *stateName(UnitState state) {
    switch (state) {
    case UnitState::Steady:
        return "Steady";
    case UnitState::Wavering:
        return "Wavering";
    case UnitState::Broken:
        return "Broken";
    case UnitState::Routing:
        return "Routing";
    case UnitState::Gone:
        return "Gone";
    }
    return "Gone";
}

const char *orderName(Order::Type type) {
    switch (type) {
    case Order::Type::Hold:
        return "Hold";
    case Order::Type::Move:
        return "Move";
    case Order::Type::Attack:
        return "Attack";
    }
    return "Hold";
}

struct Named {
    std::string id;
    int seq = 0;
};

Named nameOf(const Battle &battle, UnitId id) {
    Named named;
    if (id == kNoUnit || id >= battle.units().size()) {
        return named;
    }
    const Unit &unit = battle.units()[id];
    const UnitId origin = unit.origin == kNoUnit ? id : unit.origin;
    const bool piece = unit.origin != kNoUnit || unit.fragNo > 0;
    named.seq = piece ? static_cast<int>(unit.fragNo) : 0;
    if (const std::optional<std::string> scene = battle.sceneName(piece ? origin : id)) {
        named.id = *scene;
    } else {
        named.id = "#" + std::to_string(piece ? origin : id);
    }
    return named;
}

void jsonString(std::ostream &out, const std::string &text) {
    out << '"';
    for (unsigned char c : text) {
        if (c == '"' || c == '\\') {
            out << '\\';
        }
        out << static_cast<char>(c);
    }
    out << '"';
}

void jsonFloat(std::ostream &out, float value) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.8g", static_cast<double>(value));
    out << buffer;
}

void writeEventLine(std::ostream &out, const Battle &battle, const Event &event) {
    const Named a = nameOf(battle, event.a);
    const Named b = nameOf(battle, event.b);
    out << "{\"kind\":\"event\",\"tick\":" << event.tick << ",\"type\":\"" << eventName(event.type) << "\",\"a\":";
    jsonString(out, a.id);
    out << ",\"aSeq\":" << a.seq << ",\"b\":";
    jsonString(out, b.id);
    out << ",\"bSeq\":" << b.seq << ",\"value\":";
    jsonFloat(out, event.value);
    out << "}\n";
}

void writeStateLine(std::ostream &out, const Battle &battle) {
    struct Row {
        Named name;
        const Unit *unit = nullptr;
    };
    std::vector<Row> rows;
    rows.reserve(battle.units().size());
    for (const Unit &unit : battle.units()) {
        rows.push_back(Row{nameOf(battle, unit.id), &unit});
    }
    std::sort(rows.begin(), rows.end(), [](const Row &left, const Row &right) {
        if (left.name.id != right.name.id) {
            return left.name.id < right.name.id;
        }
        return left.name.seq < right.name.seq;
    });
    out << "{\"kind\":\"state\",\"tick\":" << battle.tick() << ",\"units\":[";
    bool first = true;
    for (const Row &row : rows) {
        const Unit &unit = *row.unit;
        if (!first) {
            out << ',';
        }
        first = false;
        const Named target = unit.order.type == Order::Type::Attack ? nameOf(battle, unit.order.target) : Named{};
        out << "{\"id\":";
        jsonString(out, row.name.id);
        out << ",\"seq\":" << row.name.seq << ",\"x\":";
        jsonFloat(out, unit.pos.x);
        out << ",\"y\":";
        jsonFloat(out, unit.pos.y);
        out << ",\"facing\":";
        jsonFloat(out, unit.facing);
        out << ",\"men\":";
        jsonFloat(out, unit.men);
        out << ",\"morale\":";
        jsonFloat(out, unit.morale);
        out << ",\"state\":\"" << stateName(unit.state) << "\",\"order\":\"" << orderName(unit.order.type)
            << "\",\"target\":";
        jsonString(out, target.id);
        out << ",\"targetSeq\":" << target.seq << ",\"still\":";
        jsonFloat(out, unit.stillSec);
        out << ",\"reload\":";
        jsonFloat(out, unit.reloadSec);
        out << ",\"kx\":";
        jsonFloat(out, unit.knock.x);
        out << ",\"ky\":";
        jsonFloat(out, unit.knock.y);
        out << '}';
    }
    out << "]}\n";
}

} // namespace

RunResult runBattle(const GameData &data, const RunRequest &request) {
    BattleOptions options;
    options.aiEnabled = request.ai;
    Battle battle(data.defs, data.scenario, request.seed, options);
    PolicyState state;
    RunResult result;
    std::ofstream events;
    if (request.eventsOut) {
        if (request.eventsOut->has_parent_path()) {
            std::filesystem::create_directories(request.eventsOut->parent_path());
        }
        events.open(*request.eventsOut, std::ios::binary);
        if (!events) {
            throw std::runtime_error("could not write " + request.eventsOut->string());
        }
        writeStateLine(events, battle);
    }
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
            if (events) {
                writeEventLine(events, battle, event);
            }
        }
        if (events) {
            writeStateLine(events, battle);
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
