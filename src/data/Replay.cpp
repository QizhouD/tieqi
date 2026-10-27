#include "data/Replay.h"

#include "data/Load.h"

#include <nlohmann/json.hpp>

#include <fstream>
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
    fail(file, "未知命令 " + name);
}

} // namespace

std::string buildId() {
#ifdef TQ_BUILD_ID
    return TQ_BUILD_ID;
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
        commands.push_back(std::move(item));
    }
    Json root = Json::object();
    root["schema"] = replay.schema;
    root["build"] = replay.build;
    root["scenario"] = replay.scenario;
    root["seed"] = replay.seed;
    root["ai"] = replay.ai;
    root["commands"] = std::move(commands);
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        fail(path.string(), "无法写入");
    }
    out << root.dump(2) << '\n';
}

Replay loadReplay(const std::filesystem::path &path) {
    const std::string file = path.string();
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        fail(file, "无法读取");
    }
    Json root;
    try {
        root = Json::parse(in, nullptr, true, true);
    } catch (const Json::exception &error) {
        fail(file, error.what());
    }
    if (!root.is_object() || !root.contains("schema") || root["schema"] != 1) {
        fail(file, "schema 必须为 1");
    }
    for (auto it = root.begin(); it != root.end(); ++it) {
        const std::string key = it.key();
        if (key != "schema" && key != "build" && key != "scenario" && key != "seed" && key != "ai" &&
            key != "commands") {
            fail(file, "未知字段 " + key);
        }
    }
    if (!root.contains("build") || !root["build"].is_string() || !root.contains("scenario") ||
        !root["scenario"].is_string() || !root.contains("seed") || !root["seed"].is_number_unsigned() ||
        !root.contains("ai") || !root["ai"].is_boolean() || !root.contains("commands") ||
        !root["commands"].is_array()) {
        fail(file, "缺少构建号、场景、种子、ai 或命令列表");
    }
    Replay replay;
    replay.build = root["build"].get<std::string>();
    replay.scenario = root["scenario"].get<std::string>();
    replay.seed = root["seed"].get<std::uint64_t>();
    replay.ai = root["ai"].get<bool>();
    for (const Json &item : root["commands"]) {
        if (!item.is_object() || !item.contains("tick") || !item["tick"].is_number_unsigned() ||
            !item.contains("type") || !item["type"].is_string() || !item.contains("units") ||
            !item["units"].is_array()) {
            fail(file, "命令格式不对");
        }
        ReplayCommand command;
        command.tick = item["tick"].get<std::uint32_t>();
        command.type = commandType(file, item["type"].get<std::string>());
        for (const Json &unit : item["units"]) {
            if (!unit.is_string()) {
                fail(file, "命令单位必须是场景 id");
            }
            command.units.push_back(unit.get<std::string>());
        }
        if (item.contains("point")) {
            if (!item["point"].is_array() || item["point"].size() != 2) {
                fail(file, "point 必须是两个数");
            }
            command.point = Vec2{item["point"][0].get<float>(), item["point"][1].get<float>()};
        }
        if (item.contains("target")) {
            if (!item["target"].is_string()) {
                fail(file, "target 必须是场景 id");
            }
            command.target = item["target"].get<std::string>();
        }
        replay.commands.push_back(std::move(command));
    }
    return replay;
}

} // namespace tq
