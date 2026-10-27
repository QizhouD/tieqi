#pragma once

#include "core/sim/Types.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tq {

enum class UnitClass : std::uint8_t { Cavalry, Infantry };
enum class AiRole : std::uint8_t { None, Gunner, PikeLine, Shock, VanGeneral, MainGeneral };

struct UnitType {
    std::string id;
    std::string nameKey;
    std::string glyphKey;
    Side side = Side::Ming;
    UnitClass cls = UnitClass::Infantry;
    AiRole ai = AiRole::None;
    float men = 0.f;
    float walk = 0.f;
    float run = 0.f;
    float turn = 0.f;
    float impact = 0.f;
    float melee = 0.f;
    float defense = 0.f;
    float armor = 0.f;
    float morale = 0.f;
    float frontage = 0.f;
    float depth = 0.f;
    float range = 0.f;
    float reload = 0.f;
    float aura = 0.f;
    bool threeEyedGun = false;
    bool elite = false;
    bool general = false;
    bool pike = false;
    bool momentum = false;
    bool momentumBoost = false;
};

struct GameDefs {
    std::vector<UnitType> types;

    const UnitType *find(std::string_view id) const {
        for (const UnitType &type : types) {
            if (type.id == id) {
                return &type;
            }
        }
        return nullptr;
    }

    std::optional<std::uint16_t> indexOf(std::string_view id) const {
        for (std::size_t i = 0; i < types.size(); ++i) {
            if (types[i].id == id) {
                return static_cast<std::uint16_t>(i);
            }
        }
        return std::nullopt;
    }
};

} // namespace tq
