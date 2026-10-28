#include "data/Load.h"

#include "core/rules/Constants.h"

#include <nlohmann/json.hpp>

#include <cmath>
#include <fstream>
#include <initializer_list>
#include <numbers>
#include <set>
#include <sstream>
#include <utility>

namespace tq {
namespace {

using Json = nlohmann::ordered_json;

struct Reader {
    std::string file;

    [[noreturn]] void fail(const std::string &pointer, const std::string &message) const {
        const std::string at = pointer.empty() ? "/" : pointer;
        throw DataError(file + ":" + at + ": " + message);
    }
};

std::string numText(double value) {
    if (std::isfinite(value) && std::floor(value) == value && std::fabs(value) < 1.0e15) {
        return std::to_string(static_cast<long long>(value));
    }
    std::ostringstream out;
    out << value;
    return out.str();
}

Json readJson(const std::filesystem::path &path, Reader &reader) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw DataError(reader.file + ": could not read");
    }
    std::ostringstream text;
    text << in.rdbuf();
    try {
        return Json::parse(text.str(), nullptr, true, true);
    } catch (const Json::exception &error) {
        throw DataError(reader.file + ": JSON parse failed (" + error.what() + ")");
    }
}

void requireObject(const Json &json, const std::string &pointer, const Reader &reader) {
    if (!json.is_object()) {
        reader.fail(pointer, "expected an object");
    }
}

void rejectUnknown(const Json &json, const std::string &pointer, const Reader &reader,
                   std::initializer_list<const char *> allowed) {
    for (auto it = json.begin(); it != json.end(); ++it) {
        bool known = false;
        for (const char *key : allowed) {
            if (it.key() == key) {
                known = true;
                break;
            }
        }
        if (!known) {
            reader.fail(pointer, "unknown field " + it.key());
        }
    }
}

std::string child(const std::string &pointer, const std::string &key) { return pointer + "/" + key; }

const Json *findField(const Json &json, const std::string &pointer, const char *key, bool required,
                      const Reader &reader) {
    const auto found = json.find(key);
    if (found == json.end()) {
        if (required) {
            reader.fail(child(pointer, key), std::string("missing field ") + key);
        }
        return nullptr;
    }
    return &(*found);
}

double readNumber(const Json &json, const std::string &pointer, const Reader &reader) {
    if (!json.is_number()) {
        reader.fail(pointer, "expected a number");
    }
    return json.get<double>();
}

double fieldNumber(const Json &json, const std::string &pointer, const char *key, const Reader &reader) {
    const Json *value = findField(json, pointer, key, true, reader);
    return readNumber(*value, child(pointer, key), reader);
}

int fieldInt(const Json &json, const std::string &pointer, const char *key, const Reader &reader) {
    const Json *value = findField(json, pointer, key, true, reader);
    const std::string at = child(pointer, key);
    if (!value->is_number_integer()) {
        reader.fail(at, "expected an integer");
    }
    return value->get<int>();
}

bool fieldBool(const Json &json, const std::string &pointer, const char *key, const Reader &reader) {
    const Json *value = findField(json, pointer, key, true, reader);
    const std::string at = child(pointer, key);
    if (!value->is_boolean()) {
        reader.fail(at, "expected a boolean");
    }
    return value->get<bool>();
}

std::string fieldString(const Json &json, const std::string &pointer, const char *key, const Reader &reader) {
    const Json *value = findField(json, pointer, key, true, reader);
    const std::string at = child(pointer, key);
    if (!value->is_string()) {
        reader.fail(at, "expected a string");
    }
    return value->get<std::string>();
}

void requireSchema(const Json &json, const Reader &reader) {
    const Json *schema = findField(json, "", "schema", true, reader);
    if (!schema->is_number_integer() || schema->get<int>() != 1) {
        const std::string actual = schema->is_number() ? numText(schema->get<double>()) : "?";
        reader.fail("/schema", "schema must be 1 (got " + actual + ")");
    }
}

void greaterThanZero(double value, const std::string &pointer, const Reader &reader) {
    if (!(value > 0.0)) {
        reader.fail(pointer, "must be greater than 0 (got " + numText(value) + ")");
    }
}

void requireKey(const StringTable &zh, const std::string &key, const std::string &pointer, const Reader &reader) {
    if (!zh.entries.contains(key)) {
        reader.fail(pointer, "string table is missing key " + key);
    }
}

std::vector<double> fixedNumbers(const Json &json, const std::string &pointer, std::size_t count,
                                 const Reader &reader) {
    if (!json.is_array()) {
        reader.fail(pointer, "expected an array");
    }
    if (json.size() != count) {
        reader.fail(pointer, "length must be " + std::to_string(count));
    }
    std::vector<double> values;
    values.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        values.push_back(readNumber(json[i], child(pointer, std::to_string(i)), reader));
    }
    return values;
}

Vec2 readPoint(const Json &json, const std::string &pointer, const Reader &reader) {
    const std::vector<double> values = fixedNumbers(json, pointer, 2, reader);
    return {static_cast<float>(values[0]), static_cast<float>(values[1])};
}

Side readSide(const std::string &text, const std::string &pointer, const Reader &reader) {
    if (text == "ming") {
        return Side::Ming;
    }
    if (text == "jp") {
        return Side::Jp;
    }
    reader.fail(pointer, "unknown value " + text);
}

UnitClass readClass(const std::string &text, const std::string &pointer, const Reader &reader) {
    if (text == "cavalry") {
        return UnitClass::Cavalry;
    }
    if (text == "infantry") {
        return UnitClass::Infantry;
    }
    reader.fail(pointer, "unknown value " + text);
}

AiRole readRole(const std::string &text, const std::string &pointer, const Reader &reader) {
    if (text == "none") {
        return AiRole::None;
    }
    if (text == "gunner") {
        return AiRole::Gunner;
    }
    if (text == "pike_line") {
        return AiRole::PikeLine;
    }
    if (text == "shock") {
        return AiRole::Shock;
    }
    if (text == "van_general") {
        return AiRole::VanGeneral;
    }
    if (text == "main_general") {
        return AiRole::MainGeneral;
    }
    reader.fail(pointer, "unknown value " + text);
}

void readTraits(const Json &json, const std::string &pointer, UnitType &unit, const Reader &reader) {
    if (!json.is_array()) {
        reader.fail(pointer, "expected an array");
    }
    std::set<std::string> seen;
    for (std::size_t i = 0; i < json.size(); ++i) {
        const std::string at = child(pointer, std::to_string(i));
        if (!json[i].is_string()) {
            reader.fail(at, "expected a string");
        }
        const std::string trait = json[i].get<std::string>();
        if (!seen.insert(trait).second) {
            reader.fail(at, "duplicate trait (got " + trait + ")");
        }
        if (trait == "three_eyed_gun") {
            unit.threeEyedGun = true;
        } else if (trait == "elite") {
            unit.elite = true;
        } else if (trait == "general") {
            unit.general = true;
        } else if (trait == "pike") {
            unit.pike = true;
        } else if (trait == "momentum") {
            unit.momentum = true;
        } else if (trait == "momentum_boost") {
            unit.momentumBoost = true;
        } else {
            reader.fail(at, "unknown trait " + trait);
        }
    }
}

UnitType readUnit(const std::string &id, const Json &json, const std::string &pointer, const StringTable *zh,
                  const Reader &reader) {
    requireObject(json, pointer, reader);
    rejectUnknown(json, pointer, reader, {"side",     "class", "ai_role", "name_key", "glyph_key", "men",   "walk",
                                          "run",      "turn",  "impact",  "melee",    "defense",   "armor", "morale",
                                          "frontage", "depth", "range",   "reload",   "aura",      "traits"});
    UnitType unit;
    unit.id = id;
    unit.side = readSide(fieldString(json, pointer, "side", reader), child(pointer, "side"), reader);
    unit.cls = readClass(fieldString(json, pointer, "class", reader), child(pointer, "class"), reader);
    unit.ai = readRole(fieldString(json, pointer, "ai_role", reader), child(pointer, "ai_role"), reader);
    unit.nameKey = fieldString(json, pointer, "name_key", reader);
    unit.glyphKey = fieldString(json, pointer, "glyph_key", reader);
    if (zh != nullptr) {
        requireKey(*zh, unit.nameKey, child(pointer, "name_key"), reader);
        requireKey(*zh, unit.glyphKey, child(pointer, "glyph_key"), reader);
    }
    unit.men = static_cast<float>(fieldNumber(json, pointer, "men", reader));
    unit.walk = static_cast<float>(fieldNumber(json, pointer, "walk", reader));
    unit.run = static_cast<float>(fieldNumber(json, pointer, "run", reader));
    unit.turn = static_cast<float>(fieldNumber(json, pointer, "turn", reader));
    unit.impact = static_cast<float>(fieldNumber(json, pointer, "impact", reader));
    unit.melee = static_cast<float>(fieldNumber(json, pointer, "melee", reader));
    unit.defense = static_cast<float>(fieldNumber(json, pointer, "defense", reader));
    unit.armor = static_cast<float>(fieldNumber(json, pointer, "armor", reader));
    unit.morale = static_cast<float>(fieldNumber(json, pointer, "morale", reader));
    unit.frontage = static_cast<float>(fieldNumber(json, pointer, "frontage", reader));
    unit.depth = static_cast<float>(fieldNumber(json, pointer, "depth", reader));
    greaterThanZero(unit.men, child(pointer, "men"), reader);
    greaterThanZero(unit.walk, child(pointer, "walk"), reader);
    if (!(unit.run == 0.f || unit.run >= unit.walk)) {
        reader.fail(child(pointer, "run"), "must be 0 or at least walk (got " + numText(unit.run) + ")");
    }
    greaterThanZero(unit.turn, child(pointer, "turn"), reader);
    if (unit.impact < 0.f) {
        reader.fail(child(pointer, "impact"), "must be greater than or equal to 0 (got " + numText(unit.impact) + ")");
    }
    greaterThanZero(unit.melee, child(pointer, "melee"), reader);
    greaterThanZero(unit.defense, child(pointer, "defense"), reader);
    greaterThanZero(unit.armor, child(pointer, "armor"), reader);
    if (!(unit.morale > 0.f)) {
        reader.fail(child(pointer, "morale"), "must be greater than 0 (got " + numText(unit.morale) + ")");
    }
    if (unit.morale > 100.f) {
        reader.fail(child(pointer, "morale"), "must be less than or equal to 100 (got " + numText(unit.morale) + ")");
    }
    greaterThanZero(unit.frontage, child(pointer, "frontage"), reader);
    greaterThanZero(unit.depth, child(pointer, "depth"), reader);

    const bool hasRange = json.contains("range");
    const bool hasReload = json.contains("reload");
    if (hasRange != hasReload) {
        reader.fail(pointer, "range and reload must both be set");
    }
    if (hasRange) {
        unit.range = static_cast<float>(readNumber(json.at("range"), child(pointer, "range"), reader));
        unit.reload = static_cast<float>(readNumber(json.at("reload"), child(pointer, "reload"), reader));
        greaterThanZero(unit.range, child(pointer, "range"), reader);
        greaterThanZero(unit.reload, child(pointer, "reload"), reader);
    }
    const Json *traits = findField(json, pointer, "traits", true, reader);
    readTraits(*traits, child(pointer, "traits"), unit, reader);
    if (unit.general) {
        if (!json.contains("aura")) {
            reader.fail(pointer, "a general must have aura");
        }
        unit.aura = static_cast<float>(readNumber(json.at("aura"), child(pointer, "aura"), reader));
        greaterThanZero(unit.aura, child(pointer, "aura"), reader);
    } else if (json.contains("aura")) {
        reader.fail(child(pointer, "aura"), "only a general can have aura");
    }
    return unit;
}

MapDef readMap(const Json &json, const std::string &pointer, const StringTable &zh, const Reader &reader) {
    requireObject(json, pointer, reader);
    rejectUnknown(json, pointer, reader, {"width", "height", "paddies", "hills", "road", "post_station", "labels"});
    MapDef map;
    map.width = static_cast<float>(fieldNumber(json, pointer, "width", reader));
    map.height = static_cast<float>(fieldNumber(json, pointer, "height", reader));
    greaterThanZero(map.width, child(pointer, "width"), reader);
    greaterThanZero(map.height, child(pointer, "height"), reader);

    const Json *paddies = findField(json, pointer, "paddies", true, reader);
    if (!paddies->is_array()) {
        reader.fail(child(pointer, "paddies"), "expected an array");
    }
    for (std::size_t i = 0; i < paddies->size(); ++i) {
        const std::string at = child(child(pointer, "paddies"), std::to_string(i));
        const std::vector<double> box = fixedNumbers((*paddies)[i], at, 4, reader);
        map.paddies.push_back({static_cast<float>(box[0]), static_cast<float>(box[1]), static_cast<float>(box[2]),
                               static_cast<float>(box[3])});
    }

    const Json *hills = findField(json, pointer, "hills", true, reader);
    if (!hills->is_array()) {
        reader.fail(child(pointer, "hills"), "expected an array");
    }
    for (std::size_t i = 0; i < hills->size(); ++i) {
        const std::string at = child(child(pointer, "hills"), std::to_string(i));
        const std::vector<double> hill = fixedNumbers((*hills)[i], at, 4, reader);
        map.hills.push_back({static_cast<float>(hill[0]), static_cast<float>(hill[1]), static_cast<float>(hill[2]),
                             static_cast<float>(hill[3])});
    }

    const Json *road = findField(json, pointer, "road", true, reader);
    const std::string roadAt = child(pointer, "road");
    requireObject(*road, roadAt, reader);
    rejectUnknown(*road, roadAt, reader, {"half_width", "points"});
    map.road.halfWidth = static_cast<float>(fieldNumber(*road, roadAt, "half_width", reader));
    greaterThanZero(map.road.halfWidth, child(roadAt, "half_width"), reader);
    const Json *points = findField(*road, roadAt, "points", true, reader);
    if (!points->is_array()) {
        reader.fail(child(roadAt, "points"), "expected an array");
    }
    for (std::size_t i = 0; i < points->size(); ++i) {
        map.road.points.push_back(readPoint((*points)[i], child(child(roadAt, "points"), std::to_string(i)), reader));
    }

    const Json *post = findField(json, pointer, "post_station", true, reader);
    map.postStation = readPoint(*post, child(pointer, "post_station"), reader);

    const Json *labels = findField(json, pointer, "labels", true, reader);
    if (!labels->is_array()) {
        reader.fail(child(pointer, "labels"), "expected an array");
    }
    for (std::size_t i = 0; i < labels->size(); ++i) {
        const std::string at = child(child(pointer, "labels"), std::to_string(i));
        requireObject((*labels)[i], at, reader);
        rejectUnknown((*labels)[i], at, reader, {"key", "pos", "size", "align", "faint"});
        MapLabel label;
        label.key = fieldString((*labels)[i], at, "key", reader);
        requireKey(zh, label.key, child(at, "key"), reader);
        const Json *pos = findField((*labels)[i], at, "pos", true, reader);
        label.pos = readPoint(*pos, child(at, "pos"), reader);
        label.size = static_cast<float>(fieldNumber((*labels)[i], at, "size", reader));
        greaterThanZero(label.size, child(at, "size"), reader);
        const std::string align = fieldString((*labels)[i], at, "align", reader);
        if (align == "left") {
            label.align = TextAlign::Left;
        } else if (align == "center") {
            label.align = TextAlign::Center;
        } else {
            reader.fail(child(at, "align"), "unknown value " + align);
        }
        if ((*labels)[i].contains("faint")) {
            const Json &faint = (*labels)[i].at("faint");
            if (!faint.is_boolean()) {
                reader.fail(child(at, "faint"), "expected a boolean");
            }
            label.faint = faint.get<bool>();
        }
        map.labels.push_back(std::move(label));
    }
    return map;
}

bool insideMap(Vec2 point, float width, float height) {
    return point.x >= 0.f && point.x <= width && point.y >= 0.f && point.y <= height;
}

bool spawnNearMap(Vec2 point, float width, float height) {
    return point.x >= -kSpawnOutsideMeters && point.x <= width + kSpawnOutsideMeters &&
           point.y >= -kSpawnOutsideMeters && point.y <= height + kSpawnOutsideMeters;
}

float degreesToRadians(double degrees) { return static_cast<float>(degrees * std::numbers::pi / 180.0); }

std::uint16_t requireType(const GameDefs &defs, const std::string &id, const std::string &pointer,
                          const Reader &reader) {
    const std::optional<std::uint16_t> index = defs.indexOf(id);
    if (!index) {
        reader.fail(pointer, "unknown unit type (got " + id + ")");
    }
    return *index;
}

void rememberId(std::set<std::string> &ids, const std::string &id, const std::string &pointer, const Reader &reader) {
    if (id.empty()) {
        reader.fail(pointer, "must not be empty");
    }
    if (!ids.insert(id).second) {
        reader.fail(pointer, "duplicate unit id (got " + id + ")");
    }
}

Placement readDeployment(const Json &json, const std::string &pointer, const GameDefs &defs, const MapDef &map,
                         const StringTable &zh, std::set<std::string> &ids, const Reader &reader) {
    requireObject(json, pointer, reader);
    rejectUnknown(json, pointer, reader, {"id", "type", "pos", "facing_deg", "label_key", "vanguard"});
    Placement unit;
    unit.id = fieldString(json, pointer, "id", reader);
    rememberId(ids, unit.id, child(pointer, "id"), reader);
    unit.type = requireType(defs, fieldString(json, pointer, "type", reader), child(pointer, "type"), reader);
    const Json *pos = findField(json, pointer, "pos", true, reader);
    unit.pos = readPoint(*pos, child(pointer, "pos"), reader);
    if (!insideMap(unit.pos, map.width, map.height)) {
        reader.fail(child(pointer, "pos"), "a deployed unit must start inside the map");
    }
    unit.facing = degreesToRadians(fieldNumber(json, pointer, "facing_deg", reader));
    unit.labelKey = fieldString(json, pointer, "label_key", reader);
    requireKey(zh, unit.labelKey, child(pointer, "label_key"), reader);
    if (json.contains("vanguard")) {
        const Json &flag = json.at("vanguard");
        if (!flag.is_boolean()) {
            reader.fail(child(pointer, "vanguard"), "expected a boolean");
        }
        unit.vanguard = flag.get<bool>();
    }
    return unit;
}

Placement readSpawn(const Json &json, const std::string &pointer, const GameDefs &defs, const MapDef &map,
                    const StringTable &zh, std::set<std::string> &ids, const Json &group,
                    const std::string &groupPointer, const Reader &reader) {
    requireObject(json, pointer, reader);
    rejectUnknown(json, pointer, reader, {"id", "type", "spawn", "rally", "facing_deg", "label_key", "vanguard"});
    Placement unit;
    unit.id = fieldString(json, pointer, "id", reader);
    rememberId(ids, unit.id, child(pointer, "id"), reader);
    unit.type = requireType(defs, fieldString(json, pointer, "type", reader), child(pointer, "type"), reader);
    const Json *spawn = findField(json, pointer, "spawn", true, reader);
    unit.pos = readPoint(*spawn, child(pointer, "spawn"), reader);
    if (!spawnNearMap(unit.pos, map.width, map.height)) {
        reader.fail(child(pointer, "spawn"), "spawn is more than 90 m outside the map");
    }
    const Json *rally = findField(json, pointer, "rally", true, reader);
    unit.rally = readPoint(*rally, child(pointer, "rally"), reader);
    if (json.contains("facing_deg")) {
        unit.facing = degreesToRadians(readNumber(json.at("facing_deg"), child(pointer, "facing_deg"), reader));
    } else if (group.contains("facing_deg")) {
        unit.facing = degreesToRadians(readNumber(group.at("facing_deg"), child(groupPointer, "facing_deg"), reader));
    } else {
        reader.fail(pointer, "missing field facing_deg");
    }
    if (group.contains("rally_facing_deg")) {
        unit.rallyFacing =
            degreesToRadians(readNumber(group.at("rally_facing_deg"), child(groupPointer, "rally_facing_deg"), reader));
    }
    unit.labelKey = fieldString(json, pointer, "label_key", reader);
    requireKey(zh, unit.labelKey, child(pointer, "label_key"), reader);
    if (json.contains("vanguard")) {
        const Json &flag = json.at("vanguard");
        if (!flag.is_boolean()) {
            reader.fail(child(pointer, "vanguard"), "expected a boolean");
        }
        unit.vanguard = flag.get<bool>();
    }
    return unit;
}

FlowDef readFlow(const Json &json, const std::string &pointer, const std::vector<SpawnGroup> &groups,
                 const Reader &reader) {
    requireObject(json, pointer, reader);
    rejectUnknown(json, pointer, reader, {"scout_warning", "phase2", "phase3", "reinforcements"});
    FlowDef flow;
    flow.enabled = true;
    const std::string scoutAt = child(pointer, "scout_warning");
    const Json *scout = findField(json, pointer, "scout_warning", true, reader);
    requireObject(*scout, scoutAt, reader);
    rejectUnknown(*scout, scoutAt, reader, {"at_sec", "early_at_sec", "early_vanguard_broken", "lead_sec"});
    flow.scoutWarning.atSec = static_cast<float>(fieldNumber(*scout, scoutAt, "at_sec", reader));
    flow.scoutWarning.earlyAtSec = static_cast<float>(fieldNumber(*scout, scoutAt, "early_at_sec", reader));
    flow.scoutWarning.earlyVanguardBroken = fieldInt(*scout, scoutAt, "early_vanguard_broken", reader);
    flow.scoutWarning.leadSec = static_cast<float>(fieldNumber(*scout, scoutAt, "lead_sec", reader));

    const std::string phase2At = child(pointer, "phase2");
    const Json *phase2 = findField(json, pointer, "phase2", true, reader);
    requireObject(*phase2, phase2At, reader);
    rejectUnknown(*phase2, phase2At, reader, {"spawn_group", "open_exit"});
    flow.phase2.spawnGroup = fieldString(*phase2, phase2At, "spawn_group", reader);
    flow.phase2.openExit = fieldBool(*phase2, phase2At, "open_exit", reader);

    const std::string phase3At = child(pointer, "phase3");
    const Json *phase3 = findField(json, pointer, "phase3", true, reader);
    requireObject(*phase3, phase3At, reader);
    rejectUnknown(*phase3, phase3At, reader, {"after_phase2_sec", "spawn_group", "grand_assault"});
    flow.phase3.afterPhase2Sec = static_cast<float>(fieldNumber(*phase3, phase3At, "after_phase2_sec", reader));
    flow.phase3.spawnGroup = fieldString(*phase3, phase3At, "spawn_group", reader);
    const std::string assaultAt = child(phase3At, "grand_assault");
    const Json *assault = findField(*phase3, phase3At, "grand_assault", true, reader);
    requireObject(*assault, assaultAt, reader);
    rejectUnknown(*assault, assaultAt, reader, {"general", "morale_add", "cap_over_type"});
    flow.phase3.grandAssault.general = fieldString(*assault, assaultAt, "general", reader);
    flow.phase3.grandAssault.moraleAdd = static_cast<float>(fieldNumber(*assault, assaultAt, "morale_add", reader));
    flow.phase3.grandAssault.capOverType =
        static_cast<float>(fieldNumber(*assault, assaultAt, "cap_over_type", reader));

    const Json *reinforcements = findField(json, pointer, "reinforcements", true, reader);
    if (!reinforcements->is_array()) {
        reader.fail(child(pointer, "reinforcements"), "expected an array");
    }
    for (std::size_t i = 0; i < reinforcements->size(); ++i) {
        const std::string at = child(child(pointer, "reinforcements"), std::to_string(i));
        requireObject((*reinforcements)[i], at, reader);
        rejectUnknown((*reinforcements)[i], at, reader, {"id", "spawn_group", "after_phase3_sec", "danger"});
        ReinforcementDef item;
        item.id = fieldString((*reinforcements)[i], at, "id", reader);
        item.spawnGroup = fieldString((*reinforcements)[i], at, "spawn_group", reader);
        item.afterPhase3Sec = static_cast<float>(fieldNumber((*reinforcements)[i], at, "after_phase3_sec", reader));
        const std::string dangerAt = child(at, "danger");
        const Json *danger = findField((*reinforcements)[i], at, "danger", true, reader);
        requireObject(*danger, dangerAt, reader);
        rejectUnknown(*danger, dangerAt, reader, {"after_phase2_sec", "enemy_within", "men_below"});
        item.danger.afterPhase2Sec = static_cast<float>(fieldNumber(*danger, dangerAt, "after_phase2_sec", reader));
        item.danger.enemyWithin = static_cast<float>(fieldNumber(*danger, dangerAt, "enemy_within", reader));
        item.danger.menBelow = static_cast<float>(fieldNumber(*danger, dangerAt, "men_below", reader));
        flow.reinforcements.push_back(std::move(item));
    }

    const auto groupExists = [&](const std::string &id) {
        for (const SpawnGroup &group : groups) {
            if (group.id == id) {
                return true;
            }
        }
        return false;
    };
    if (!groupExists(flow.phase2.spawnGroup)) {
        reader.fail(child(phase2At, "spawn_group"), "unknown group (got " + flow.phase2.spawnGroup + ")");
    }
    if (!groupExists(flow.phase3.spawnGroup)) {
        reader.fail(child(phase3At, "spawn_group"), "unknown group (got " + flow.phase3.spawnGroup + ")");
    }
    for (std::size_t i = 0; i < flow.reinforcements.size(); ++i) {
        if (!groupExists(flow.reinforcements[i].spawnGroup)) {
            reader.fail(child(child(child(pointer, "reinforcements"), std::to_string(i)), "spawn_group"),
                        "unknown group (got " + flow.reinforcements[i].spawnGroup + ")");
        }
    }
    bool generalFound = false;
    for (const SpawnGroup &group : groups) {
        for (const Placement &unit : group.units) {
            if (unit.id == flow.phase3.grandAssault.general) {
                generalFound = true;
            }
        }
    }
    if (!generalFound) {
        reader.fail(child(assaultAt, "general"),
                    "must name a unit in the group (got " + flow.phase3.grandAssault.general + ")");
    }
    return flow;
}

} // namespace

GameDefs loadUnits(const std::filesystem::path &path, const StringTable *zhHant) {
    Reader reader{path.generic_string()};
    const Json json = readJson(path, reader);
    requireObject(json, "", reader);
    requireSchema(json, reader);
    rejectUnknown(json, "", reader, {"schema", "units"});
    const Json *units = findField(json, "", "units", true, reader);
    requireObject(*units, "/units", reader);
    GameDefs defs;
    for (auto it = units->begin(); it != units->end(); ++it) {
        defs.types.push_back(readUnit(it.key(), it.value(), "/units/" + it.key(), zhHant, reader));
    }
    return defs;
}

Config loadConfig(const std::filesystem::path &path) {
    Reader reader{path.generic_string()};
    const Json json = readJson(path, reader);
    requireObject(json, "", reader);
    requireSchema(json, reader);
    rejectUnknown(json, "", reader,
                  {"schema", "time_scale", "player_speeds", "default_speed", "max_ticks_per_frame", "language",
                   "reduced_motion", "sound"});
    Config config;
    config.timeScale = static_cast<float>(fieldNumber(json, "", "time_scale", reader));
    greaterThanZero(config.timeScale, "/time_scale", reader);
    const Json *speeds = findField(json, "", "player_speeds", true, reader);
    if (!speeds->is_array()) {
        reader.fail("/player_speeds", "expected an array");
    }
    if (speeds->empty()) {
        reader.fail("/player_speeds", "must not be empty");
    }
    for (std::size_t i = 0; i < speeds->size(); ++i) {
        const double speed = readNumber((*speeds)[i], "/player_speeds/" + std::to_string(i), reader);
        greaterThanZero(speed, "/player_speeds/" + std::to_string(i), reader);
        config.playerSpeeds.push_back(static_cast<float>(speed));
    }
    config.defaultSpeed = static_cast<float>(fieldNumber(json, "", "default_speed", reader));
    bool speedKnown = false;
    for (float speed : config.playerSpeeds) {
        if (speed == config.defaultSpeed) {
            speedKnown = true;
        }
    }
    if (!speedKnown) {
        reader.fail("/default_speed",
                    "default speed is not in player_speeds (got " + numText(config.defaultSpeed) + ")");
    }
    config.maxTicksPerFrame = fieldInt(json, "", "max_ticks_per_frame", reader);
    greaterThanZero(config.maxTicksPerFrame, "/max_ticks_per_frame", reader);
    config.language = fieldString(json, "", "language", reader);
    if (config.language != "zh-Hant" && config.language != "en" && config.language != "ko") {
        reader.fail("/language", "unknown value " + config.language);
    }
    config.reducedMotion = fieldBool(json, "", "reduced_motion", reader);
    config.sound = fieldBool(json, "", "sound", reader);
    return config;
}

StringTable loadStrings(const std::filesystem::path &path, const StringTable *fallback) {
    Reader reader{path.generic_string()};
    const Json json = readJson(path, reader);
    requireObject(json, "", reader);
    requireSchema(json, reader);
    rejectUnknown(json, "", reader, {"schema", "language", "fonts", "strings"});
    StringTable table;
    table.fallback = fallback;
    table.language = fieldString(json, "", "language", reader);
    const Json *fonts = findField(json, "", "fonts", true, reader);
    requireObject(*fonts, "/fonts", reader);
    rejectUnknown(*fonts, "/fonts", reader, {"body", "brush"});
    table.fontBody = fieldString(*fonts, "/fonts", "body", reader);
    table.fontBrush = fieldString(*fonts, "/fonts", "brush", reader);
    const Json *strings = findField(json, "", "strings", true, reader);
    requireObject(*strings, "/strings", reader);
    for (auto it = strings->begin(); it != strings->end(); ++it) {
        if (!it.value().is_string()) {
            reader.fail("/strings/" + it.key(), "expected a string");
        }
        table.entries.emplace(it.key(), it.value().get<std::string>());
    }
    return table;
}

ScenarioDef loadScenario(const std::filesystem::path &path, const GameDefs &defs, const StringTable &zhHant) {
    Reader reader{path.generic_string()};
    const Json json = readJson(path, reader);
    requireObject(json, "", reader);
    requireSchema(json, reader);
    rejectUnknown(json, "", reader,
                  {"schema", "id", "name_key", "start_clock", "map", "commander", "exit", "deployment", "groups",
                   "flow", "ai", "result"});
    ScenarioDef scenario;
    scenario.id = fieldString(json, "", "id", reader);
    scenario.nameKey = fieldString(json, "", "name_key", reader);
    requireKey(zhHant, scenario.nameKey, "/name_key", reader);
    scenario.startClock = fieldString(json, "", "start_clock", reader);
    const Json *map = findField(json, "", "map", true, reader);
    scenario.map = readMap(*map, "/map", zhHant, reader);

    const Json *deployment = findField(json, "", "deployment", true, reader);
    if (!deployment->is_array()) {
        reader.fail("/deployment", "expected an array");
    }
    std::set<std::string> ids;
    for (std::size_t i = 0; i < deployment->size(); ++i) {
        scenario.deployment.push_back(readDeployment((*deployment)[i], "/deployment/" + std::to_string(i), defs,
                                                     scenario.map, zhHant, ids, reader));
    }

    const Json *groups = findField(json, "", "groups", true, reader);
    requireObject(*groups, "/groups", reader);
    for (auto it = groups->begin(); it != groups->end(); ++it) {
        const std::string at = "/groups/" + it.key();
        requireObject(it.value(), at, reader);
        rejectUnknown(it.value(), at, reader, {"facing_deg", "rally_facing_deg", "units"});
        SpawnGroup group;
        group.id = it.key();
        const Json *units = findField(it.value(), at, "units", true, reader);
        if (!units->is_array()) {
            reader.fail(child(at, "units"), "expected an array");
        }
        for (std::size_t i = 0; i < units->size(); ++i) {
            group.units.push_back(readSpawn((*units)[i], child(child(at, "units"), std::to_string(i)), defs,
                                            scenario.map, zhHant, ids, it.value(), at, reader));
        }
        scenario.groups.push_back(std::move(group));
    }

    scenario.commander = fieldString(json, "", "commander", reader);
    const Placement *commander = nullptr;
    for (const Placement &unit : scenario.deployment) {
        if (unit.id == scenario.commander) {
            commander = &unit;
        }
    }
    const bool mingGeneral =
        commander != nullptr && defs.types[commander->type].side == Side::Ming && defs.types[commander->type].general;
    if (!mingGeneral) {
        reader.fail("/commander",
                    "must name a Ming general in the opening deployment (got " + scenario.commander + ")");
    }

    const Json *exit = findField(json, "", "exit", true, reader);
    requireObject(*exit, "/exit", reader);
    rejectUnknown(*exit, "/exit", reader, {"rect", "follow_radius", "retreat_y_open", "retreat_y_closed"});
    const Json *rectJson = findField(*exit, "/exit", "rect", true, reader);
    const std::vector<double> rect = fixedNumbers(*rectJson, "/exit/rect", 4, reader);
    scenario.exit.rect = {static_cast<float>(rect[0]), static_cast<float>(rect[1]), static_cast<float>(rect[2]),
                          static_cast<float>(rect[3])};
    scenario.exit.followRadius = static_cast<float>(fieldNumber(*exit, "/exit", "follow_radius", reader));
    scenario.exit.retreatYOpen = static_cast<float>(fieldNumber(*exit, "/exit", "retreat_y_open", reader));
    scenario.exit.retreatYClosed = static_cast<float>(fieldNumber(*exit, "/exit", "retreat_y_closed", reader));

    if (json.contains("flow")) {
        scenario.flow = readFlow(json.at("flow"), "/flow", scenario.groups, reader);
    }

    const Json *ai = findField(json, "", "ai", true, reader);
    requireObject(*ai, "/ai", reader);
    rejectUnknown(*ai, "/ai", reader, {"vanguard_hold_sec", "vanguard_trigger_dist"});
    scenario.ai.vanguardHoldSec = static_cast<float>(fieldNumber(*ai, "/ai", "vanguard_hold_sec", reader));
    scenario.ai.vanguardTriggerDist = static_cast<float>(fieldNumber(*ai, "/ai", "vanguard_trigger_dist", reader));

    const Json *result = findField(json, "", "result", true, reader);
    requireObject(*result, "/result", reader);
    rejectUnknown(*result, "/result", reader, {"decisive_rate", "decisive_enemy_broken", "minor_rate"});
    scenario.result.decisiveRate = static_cast<float>(fieldNumber(*result, "/result", "decisive_rate", reader));
    scenario.result.decisiveEnemyBroken = fieldInt(*result, "/result", "decisive_enemy_broken", reader);
    scenario.result.minorRate = static_cast<float>(fieldNumber(*result, "/result", "minor_rate", reader));
    return scenario;
}

GameData loadGameData(const std::filesystem::path &dataDir, const std::string &scenarioFile) {
    GameData data;
    data.zhHant = loadStrings(dataDir / "strings" / "zh-Hant.json");
    data.defs = loadUnits(dataDir / "units.json", &data.zhHant);
    data.config = loadConfig(dataDir / "config.json");
    data.scenario = loadScenario(dataDir / "scenarios" / scenarioFile, data.defs, data.zhHant);
    return data;
}

} // namespace tq
