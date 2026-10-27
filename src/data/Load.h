#pragma once

#include "core/defs/Config.h"
#include "core/defs/ScenarioDef.h"
#include "core/defs/UnitType.h"
#include "data/Strings.h"

#include <filesystem>
#include <stdexcept>
#include <string>

namespace tq {

struct DataError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct GameData {
    GameDefs defs;
    ScenarioDef scenario;
    Config config;
    StringTable zhHant;
};

GameDefs loadUnits(const std::filesystem::path &path, const StringTable *zhHant = nullptr);
Config loadConfig(const std::filesystem::path &path);
StringTable loadStrings(const std::filesystem::path &path, const StringTable *fallback = nullptr);
ScenarioDef loadScenario(const std::filesystem::path &path, const GameDefs &defs, const StringTable &zhHant);
GameData loadGameData(const std::filesystem::path &dataDir, const std::string &scenarioFile);

} // namespace tq
