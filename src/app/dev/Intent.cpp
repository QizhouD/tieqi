#include "app/dev/Intent.h"

#include "core/ai/Ai.h"

#include <algorithm>
#include <iomanip>
#include <numeric>
#include <sstream>

namespace tq::dev {
namespace {

bool alive(const Unit &unit) { return unit.state != UnitState::Gone; }

bool fighting(const Unit &unit) { return unit.state == UnitState::Steady || unit.state == UnitState::Wavering; }

std::string number(float value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << value;
    return out.str();
}

std::string point(Vec2 value) { return "(" + number(value.x) + ", " + number(value.y) + ")"; }

std::string flag(bool value) { return value ? "true" : "false"; }

} // namespace

std::vector<AiIntent> aiIntents(const Battle &battle, const GameDefs &defs) {
    std::vector<AiIntent> intents;
    const std::span<const Unit> units = battle.units();
    for (const Unit &self : units) {
        if (self.side != Side::Jp || !fighting(self)) {
            continue;
        }
        const UnitType &selfType = battle.typeOf(self);
        AiIntent intent;
        intent.id = self.id;
        intent.role = selfType.ai;
        intent.order = self.order;
        intent.aggressive = !self.vanguard || battle.vanguardAggressive();
        for (const Unit &target : units) {
            if (target.side != Side::Ming || !alive(target)) {
                continue;
            }
            const float score = scoreTarget(self, selfType, target, battle.typeOf(target), units, defs);
            if (intent.best == kNoUnit || score > intent.bestScore) {
                intent.best = target.id;
                intent.bestScore = score;
            }
            if (self.order.type == Order::Type::Attack && self.order.target == target.id) {
                intent.score = score;
            }
        }
        intents.push_back(intent);
    }
    return intents;
}

std::string unitLabel(const Battle &battle, UnitId id) {
    return battle.sceneName(id).value_or("#" + std::to_string(id));
}

const char *roleName(AiRole role) {
    switch (role) {
    case AiRole::None:
        return "none";
    case AiRole::Gunner:
        return "gunner";
    case AiRole::PikeLine:
        return "pike_line";
    case AiRole::Shock:
        return "shock";
    case AiRole::VanGeneral:
        return "van_general";
    case AiRole::MainGeneral:
        return "main_general";
    }
    return "none";
}

const char *stateName(UnitState state) {
    switch (state) {
    case UnitState::Steady:
        return "steady";
    case UnitState::Wavering:
        return "wavering";
    case UnitState::Broken:
        return "broken";
    case UnitState::Routing:
        return "routing";
    case UnitState::Gone:
        return "gone";
    }
    return "gone";
}

const char *orderName(Order::Type type) {
    switch (type) {
    case Order::Type::Hold:
        return "hold";
    case Order::Type::Move:
        return "move";
    case Order::Type::Attack:
        return "attack";
    }
    return "hold";
}

const char *sideName(Side side) { return side == Side::Ming ? "ming" : "jp"; }

std::vector<std::pair<std::string, std::string>> unitFields(const Battle &battle, UnitId id) {
    std::vector<std::pair<std::string, std::string>> fields;
    if (id >= battle.units().size()) {
        return fields;
    }
    const Unit &unit = battle.unit(id);
    const UnitType &type = battle.typeOf(unit);
    std::string contacts;
    for (UnitId other : unit.contacts) {
        contacts += (contacts.empty() ? "" : ", ") + unitLabel(battle, other);
    }
    std::string order = orderName(unit.order.type);
    if (unit.order.type == Order::Type::Move) {
        order += " " + point(unit.order.point);
    } else if (unit.order.type == Order::Type::Attack) {
        order += " " + unitLabel(battle, unit.order.target);
    }
    if (unit.order.face) {
        order += " face " + number(*unit.order.face);
    }
    fields.emplace_back("id", std::to_string(unit.id));
    fields.emplace_back("label", unitLabel(battle, unit.id));
    fields.emplace_back("type", type.id);
    fields.emplace_back("side", sideName(unit.side));
    fields.emplace_back("labelKey", unit.labelKey);
    fields.emplace_back("fragNo", std::to_string(unit.fragNo));
    fields.emplace_back("pos", point(unit.pos));
    fields.emplace_back("facing", number(unit.facing));
    fields.emplace_back("frontage x depth", number(unit.frontage) + " x " + number(unit.depth));
    fields.emplace_back("men / maxMen", number(unit.men) + " / " + number(unit.maxMen));
    fields.emplace_back("morale", number(unit.morale));
    fields.emplace_back("state", stateName(unit.state));
    fields.emplace_back("order", order);
    fields.emplace_back("contacts", contacts.empty() ? "-" : contacts);
    fields.emplace_back("engagedSec", number(unit.engagedSec));
    fields.emplace_back("recoverSec", number(unit.recoverSec));
    fields.emplace_back("reloadSec", number(unit.reloadSec));
    fields.emplace_back("stillSec", number(unit.stillSec));
    fields.emplace_back("speed", number(unit.speed));
    fields.emplace_back("runDist", number(unit.runDist));
    fields.emplace_back("charging", flag(unit.charging));
    fields.emplace_back("volleyDone", flag(unit.volleyDone));
    fields.emplace_back("breaking", flag(unit.breaking));
    fields.emplace_back("momentum", std::to_string(unit.momentum) + " (" + number(unit.momentumSec) + " s)");
    fields.emplace_back("brokenSec", number(unit.brokenSec));
    fields.emplace_back("routSec", number(unit.routSec));
    fields.emplace_back("knock", point(unit.knock));
    fields.emplace_back("scatterAngle", unit.scatterAngle ? number(*unit.scatterAngle) : "-");
    fields.emplace_back("everBroken", flag(unit.everBroken));
    fields.emplace_back("isFragment", flag(unit.isFragment));
    fields.emplace_back("rejoin", flag(unit.rejoin));
    fields.emplace_back("vanguard", flag(unit.vanguard));
    fields.emplace_back("deploying", flag(unit.deploying));
    fields.emplace_back("origin", unit.origin == kNoUnit ? "-" : unitLabel(battle, unit.origin));
    return fields;
}

void TimeSeries::push(float ms) {
    if (values_.size() < kSize) {
        values_.push_back(ms);
        return;
    }
    values_[next_] = ms;
    next_ = (next_ + 1) % kSize;
}

float TimeSeries::mean() const {
    if (values_.empty()) {
        return 0.f;
    }
    return std::accumulate(values_.begin(), values_.end(), 0.f) / static_cast<float>(values_.size());
}

float TimeSeries::max() const {
    if (values_.empty()) {
        return 0.f;
    }
    return *std::max_element(values_.begin(), values_.end());
}

} // namespace tq::dev
