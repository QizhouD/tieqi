#include "data/Replay.h"

#include "data/Load.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace tq {
namespace {

using Json = nlohmann::ordered_json;

[[noreturn]] void fail(const std::string &file, const std::string &message) { throw DataError(file + ": " + message); }

const char *commandName(Command::Type type) {
    switch (type) {
    case Command::Type::Move:
        return "move";
    case Command::Type::Attack:
        return "attack";
    case Command::Type::Hold:
        return "hold";
    case Command::Type::Retreat:
        return "retreat";
    case Command::Type::Split:
        return "split";
    case Command::Type::Join:
        return "join";
    case Command::Type::Spawn:
        return "spawn";
    case Command::Type::Rout:
        return "rout";
    }
    return "move";
}

Command::Type commandType(const std::string &file, const std::string &name) {
    if (name == "move") {
        return Command::Type::Move;
    }
    if (name == "attack") {
        return Command::Type::Attack;
    }
    if (name == "hold") {
        return Command::Type::Hold;
    }
    if (name == "retreat") {
        return Command::Type::Retreat;
    }
    if (name == "split") {
        return Command::Type::Split;
    }
    if (name == "join") {
        return Command::Type::Join;
    }
    if (name == "spawn") {
        return Command::Type::Spawn;
    }
    if (name == "rout") {
        return Command::Type::Rout;
    }
    fail(file, "unknown command " + name);
}

} // namespace

std::string buildId() {
#ifdef TQ_BUILD_ID
    return TQ_BUILD_ID;
#else
    return "unknown";
#endif
}

std::string gameVersion() {
#ifdef TQ_VERSION
    return TQ_VERSION;
#else
    return "unknown";
#endif
}

void saveReplay(const std::filesystem::path &path, const Replay &replay) {
    Json commands = Json::array();
    for (const ReplayCommand &command : replay.commands) {
        Json item = Json::object();
        item["tick"] = command.tick;
        item["type"] = commandName(command.type);
        item["units"] = command.units;
        if (command.point) {
            item["point"] = Json::array({command.point->x, command.point->y});
        }
        if (command.target) {
            item["target"] = *command.target;
        }
        if (command.unitType) {
            item["unit_type"] = *command.unitType;
        }
        if (command.facing) {
            item["facing"] = *command.facing;
        }
        commands.push_back(std::move(item));
    }
    Json root = Json::object();
    root["schema"] = replay.schema;
    root["version"] = replay.version;
    root["build"] = replay.build;
    root["scenario"] = replay.scenario;
    root["seed"] = replay.seed;
    root["ai"] = replay.ai;
    root["commands"] = std::move(commands);
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        fail(path.string(), "could not write");
    }
    out << root.dump(2) << '\n';
}

Replay loadReplay(const std::filesystem::path &path) {
    const std::string file = path.string();
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        fail(file, "could not read");
    }
    Json root;
    try {
        root = Json::parse(in, nullptr, true, true);
    } catch (const Json::exception &error) {
        fail(file, error.what());
    }
    if (!root.is_object() || !root.contains("schema") || root["schema"] != 1) {
        fail(file, "schema must be 1");
    }
    for (auto it = root.begin(); it != root.end(); ++it) {
        const std::string key = it.key();
        if (key != "schema" && key != "version" && key != "build" && key != "scenario" && key != "seed" &&
            key != "ai" && key != "commands") {
            fail(file, "unknown field " + key);
        }
    }
    if (!root.contains("build") || !root["build"].is_string() || !root.contains("scenario") ||
        !root["scenario"].is_string() || !root.contains("seed") || !root["seed"].is_number_unsigned() ||
        !root.contains("ai") || !root["ai"].is_boolean() || !root.contains("commands") ||
        !root["commands"].is_array()) {
        fail(file, "missing build, scenario, seed, ai, or commands");
    }
    Replay replay;
    replay.version.clear();
    if (root.contains("version")) {
        if (!root["version"].is_string()) {
            fail(file, "version must be a string");
        }
        replay.version = root["version"].get<std::string>();
    }
    replay.build = root["build"].get<std::string>();
    replay.scenario = root["scenario"].get<std::string>();
    replay.seed = root["seed"].get<std::uint64_t>();
    replay.ai = root["ai"].get<bool>();
    for (const Json &item : root["commands"]) {
        if (!item.is_object() || !item.contains("tick") || !item["tick"].is_number_unsigned() ||
            !item.contains("type") || !item["type"].is_string() || !item.contains("units") ||
            !item["units"].is_array()) {
            fail(file, "bad command");
        }
        ReplayCommand command;
        command.tick = item["tick"].get<std::uint32_t>();
        command.type = commandType(file, item["type"].get<std::string>());
        for (const Json &unit : item["units"]) {
            if (!unit.is_string()) {
                fail(file, "command units must be scene ids");
            }
            command.units.push_back(unit.get<std::string>());
        }
        if (item.contains("point")) {
            if (!item["point"].is_array() || item["point"].size() != 2) {
                fail(file, "point must be two numbers");
            }
            command.point = Vec2{item["point"][0].get<float>(), item["point"][1].get<float>()};
        }
        if (item.contains("target")) {
            if (!item["target"].is_string()) {
                fail(file, "target must be a scene id");
            }
            command.target = item["target"].get<std::string>();
        }
        if (item.contains("unit_type")) {
            if (!item["unit_type"].is_string()) {
                fail(file, "unit_type must be a unit type id");
            }
            command.unitType = item["unit_type"].get<std::string>();
        }
        if (item.contains("facing")) {
            if (!item["facing"].is_number()) {
                fail(file, "facing must be a number");
            }
            command.facing = item["facing"].get<float>();
        }
        if (command.type == Command::Type::Spawn && (!command.unitType || !command.point)) {
            fail(file, "spawn needs unit_type and point");
        }
        replay.commands.push_back(std::move(command));
    }
    return replay;
}

namespace {

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

std::string unitName(const Battle &battle, UnitId id) {
    return battle.sceneName(id).value_or("#" + std::to_string(id));
}

} // namespace

std::optional<Command> resolveCommand(const Battle &battle, const GameDefs &defs, const ReplayCommand &command) {
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
    if (command.type == Command::Type::Spawn) {
        for (std::size_t index = 0; index < defs.types.size(); ++index) {
            if (command.unitType && defs.types[index].id == *command.unitType) {
                issued.unitType = static_cast<std::uint16_t>(index);
                issued.face = command.facing.value_or(0.f);
                return issued;
            }
        }
        return std::nullopt;
    }
    if (issued.units.empty()) {
        return std::nullopt;
    }
    return issued;
}

ReplayCommand recordCommand(const Battle &battle, const GameDefs &defs, const Command &command) {
    ReplayCommand recorded;
    recorded.type = command.type;
    for (UnitId id : command.units) {
        recorded.units.push_back(unitName(battle, id));
    }
    if (command.type == Command::Type::Move || command.type == Command::Type::Spawn) {
        recorded.point = command.point;
    }
    if (command.type == Command::Type::Attack) {
        recorded.target = unitName(battle, command.target);
    }
    if (command.type == Command::Type::Spawn && command.unitType < defs.types.size()) {
        recorded.unitType = defs.types[command.unitType].id;
        recorded.facing = command.face;
    }
    return recorded;
}

} // namespace tq
