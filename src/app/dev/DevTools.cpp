#include "app/dev/DevTools.h"

#include "core/rules/Constants.h"

#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Time.hpp>

#include <imgui-SFML.h>
#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numbers>
#include <span>
#include <string_view>

namespace tq::dev {
namespace {

struct NamedConstant {
    const char *name;
    float value;
};

// Generated at configure time from src/core/rules/Constants.h.
constexpr NamedConstant kConstants[] = {
#include "app/dev/Constants.inc"
};

constexpr std::string_view kDefaultReplay = "dev-replay.json";

Vec2 drawn(const Unit &unit, float alpha) { return unit.prevPos + (unit.pos - unit.prevPos) * alpha; }

std::string cell(float value) {
    char text[32];
    std::snprintf(text, sizeof(text), "%.4f", static_cast<double>(value));
    return text;
}

bool contains(std::string_view text, std::string_view needle) {
    if (needle.empty()) {
        return true;
    }
    const auto lower = [](char c) { return static_cast<char>(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c); };
    return std::search(text.begin(), text.end(), needle.begin(), needle.end(),
                       [&](char a, char b) { return lower(a) == lower(b); }) != text.end();
}

} // namespace

DevTools::~DevTools() {
    if (attached_) {
        ImGui::SFML::Shutdown();
    }
}

void DevTools::attach(sf::RenderWindow &window) {
    attached_ = ImGui::SFML::Init(window);
    if (attached_) {
        ImGui::GetIO().IniFilename = nullptr;
    }
    std::copy(kDefaultReplay.begin(), kDefaultReplay.end(), replayPath_.begin());
}

bool DevTools::handle(const sf::RenderWindow &window, const sf::Event &event) {
    if (!attached_) {
        return false;
    }
    ImGui::SFML::ProcessEvent(window, event);
    if (!visible_) {
        return false;
    }
    const ImGuiIO &io = ImGui::GetIO();
    const bool mouse = event.is<sf::Event::MouseButtonPressed>() || event.is<sf::Event::MouseButtonReleased>() ||
                       event.is<sf::Event::MouseMoved>() || event.is<sf::Event::MouseWheelScrolled>();
    const bool keys = event.is<sf::Event::KeyPressed>() || event.is<sf::Event::KeyReleased>() ||
                      event.is<sf::Event::TextEntered>();
    return (mouse && io.WantCaptureMouse) || (keys && io.WantCaptureKeyboard);
}

void DevTools::begin(sf::RenderWindow &window, float seconds) {
    if (attached_) {
        ImGui::SFML::Update(window, sf::seconds(std::max(seconds, 1e-4f)));
    }
}

void DevTools::noteTimes(float simMs, float drawMs) {
    sim_.push(simMs);
    draw_.push(drawMs);
}

DevRequests DevTools::build(const DevView &view) {
    DevRequests requests;
    if (!attached_ || !visible_ || view.battle == nullptr || view.defs == nullptr) {
        return requests;
    }
    if (view.selected && !inspected_) {
        inspected_ = view.selected;
    }
    ImGui::SetNextWindowPos(ImVec2(12.f, 12.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(560.f, 620.f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Dev tools (F1 toggles, F2 next tab)", &visible_)) {
        if (ImGui::BeginTabBar("dev")) {
            static constexpr const char *kNames[kTabs] = {"Sim", "Units", "Spawn", "AI", "Constants"};
            for (int index = 0; index < kTabs; ++index) {
                const ImGuiTabItemFlags flags = tabPending_ == index ? ImGuiTabItemFlags_SetSelected : 0;
                if (!ImGui::BeginTabItem(kNames[index], nullptr, flags)) {
                    continue;
                }
                tab_ = index;
                switch (index) {
                case 0:
                    simTab(view, requests);
                    break;
                case 1:
                    unitsTab(view, requests);
                    break;
                case 2:
                    spawnTab(view, requests);
                    break;
                case 3:
                    aiTab(view);
                    break;
                default:
                    constantsTab();
                    break;
                }
                ImGui::EndTabItem();
            }
            tabPending_.reset();
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
    return requests;
}

void DevTools::simTab(const DevView &view, DevRequests &requests) {
    const Battle &battle = *view.battle;
    ImGui::Text("tick %u  (%.1f s)  phase %d", battle.tick(), static_cast<double>(battle.battleSeconds()),
                battle.phase());
    ImGui::Text("running %s  over %s  exit open %s  vanguard released %s", view.running ? "yes" : "no",
                battle.isOver() ? "yes" : "no", battle.exitOpen() ? "yes" : "no",
                battle.vanguardAggressive() ? "yes" : "no");
    ImGui::Text("state hash %016llx", static_cast<unsigned long long>(battle.stateHash()));
    ImGui::BeginDisabled(battle.isOver());
    if (ImGui::Button("Step one tick")) {
        requests.stepTick = true;
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::TextDisabled("works while paused");

    ImGui::SeparatorText("Timing (ms per frame, last 120 frames)");
    char overlay[64];
    std::snprintf(overlay, sizeof(overlay), "sim mean %.3f max %.3f", static_cast<double>(sim_.mean()),
                  static_cast<double>(sim_.max()));
    ImGui::PlotLines("##sim", sim_.data(), sim_.count(), sim_.offset(), overlay, 0.f, std::max(1.f, sim_.max()),
                     ImVec2(-1.f, 60.f));
    std::snprintf(overlay, sizeof(overlay), "render mean %.3f max %.3f", static_cast<double>(draw_.mean()),
                  static_cast<double>(draw_.max()));
    ImGui::PlotLines("##draw", draw_.data(), draw_.count(), draw_.offset(), overlay, 0.f,
                     std::max(1.f, draw_.max()), ImVec2(-1.f, 60.f));

    ImGui::SeparatorText("Replay");
    ImGui::Text("%zu commands recorded since the last restart", view.recorded);
    ImGui::InputText("file", replayPath_.data(), replayPath_.size());
    if (ImGui::Button("Save replay")) {
        requests.save = std::filesystem::path(replayPath_.data());
    }
    ImGui::SameLine();
    if (ImGui::Button("Load replay and restart")) {
        requests.load = std::filesystem::path(replayPath_.data());
    }
    if (!status_.empty()) {
        ImGui::TextWrapped("%s", status_.c_str());
    }
}

void DevTools::unitsTab(const DevView &view, DevRequests &requests) {
    const Battle &battle = *view.battle;
    if (view.selected) {
        ImGui::Text("Selected on the map: %s", unitLabel(battle, *view.selected).c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton("Inspect")) {
            inspected_ = view.selected;
        }
    }
    const ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersInnerV;
    if (ImGui::BeginTable("units", 5, flags, ImVec2(0.f, 200.f))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("id");
        ImGui::TableSetupColumn("unit");
        ImGui::TableSetupColumn("side");
        ImGui::TableSetupColumn("state");
        ImGui::TableSetupColumn("men");
        ImGui::TableHeadersRow();
        for (const Unit &unit : battle.units()) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            char id[16];
            std::snprintf(id, sizeof(id), "%u", static_cast<unsigned>(unit.id));
            if (ImGui::Selectable(id, inspected_ == unit.id, ImGuiSelectableFlags_SpanAllColumns)) {
                inspected_ = unit.id;
            }
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(unitLabel(battle, unit.id).c_str());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(sideName(unit.side));
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(stateName(unit.state));
            ImGui::TableNextColumn();
            ImGui::Text("%.0f", static_cast<double>(unit.men));
        }
        ImGui::EndTable();
    }
    if (!inspected_ || *inspected_ >= battle.units().size()) {
        ImGui::TextDisabled("Pick a unit to inspect.");
        return;
    }
    const Unit &unit = battle.unit(*inspected_);
    const bool canRout = unit.state == UnitState::Steady || unit.state == UnitState::Wavering;
    ImGui::BeginDisabled(!canRout || battle.isOver());
    if (ImGui::Button("Rout this unit")) {
        Command command;
        command.type = Command::Type::Rout;
        command.units = {unit.id};
        requests.commands.push_back(std::move(command));
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::TextDisabled("applies on the next tick");
    if (ImGui::BeginTable("fields", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV)) {
        for (const auto &[name, value] : unitFields(battle, unit.id)) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(name.c_str());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(value.c_str());
        }
        ImGui::EndTable();
    }
}

void DevTools::spawnTab(const DevView &view, DevRequests &requests) {
    const GameDefs &defs = *view.defs;
    if (defs.types.empty()) {
        return;
    }
    if (!spawnPlaced_) {
        spawnPlaced_ = true;
        const std::span<const Unit> units = view.battle->units();
        if (units.empty()) {
            spawnX_ = 0.f;
            spawnY_ = 0.f;
        } else {
            const Unit &first = units.front();
            spawnX_ = first.pos.x;
            spawnY_ = first.pos.y - 120.f;
        }
    }
    spawnType_ = std::clamp(spawnType_, 0, static_cast<int>(defs.types.size()) - 1);
    const UnitType &chosen = defs.types[static_cast<std::size_t>(spawnType_)];
    if (ImGui::BeginCombo("type", chosen.id.c_str())) {
        for (std::size_t index = 0; index < defs.types.size(); ++index) {
            const UnitType &type = defs.types[index];
            const std::string label = type.id + (type.side == Side::Ming ? "  (ming)" : "  (jp)");
            if (ImGui::Selectable(label.c_str(), static_cast<int>(index) == spawnType_)) {
                spawnType_ = static_cast<int>(index);
            }
        }
        ImGui::EndCombo();
    }
    ImGui::InputFloat("x (m)", &spawnX_, 10.f, 50.f, "%.0f");
    ImGui::InputFloat("y (m)", &spawnY_, 10.f, 50.f, "%.0f");
    ImGui::SliderFloat("facing (deg)", &spawnFaceDeg_, -180.f, 180.f, "%.0f");
    ImGui::TextDisabled("-90 faces north. The point is clamped to the map; the unit holds in place.");
    ImGui::BeginDisabled(view.battle->isOver());
    if (ImGui::Button("Spawn")) {
        Command command;
        command.type = Command::Type::Spawn;
        command.unitType = static_cast<std::uint16_t>(spawnType_);
        command.point = {spawnX_, spawnY_};
        command.face = spawnFaceDeg_ * std::numbers::pi_v<float> / 180.f;
        requests.commands.push_back(std::move(command));
    }
    ImGui::EndDisabled();
}

void DevTools::aiTab(const DevView &view) {
    const Battle &battle = *view.battle;
    ImGui::Checkbox("Draw target lines on the map", &lines_);
    ImGui::Text("vanguard released: %s", battle.vanguardAggressive() ? "yes" : "no");
    const std::vector<AiIntent> intents = aiIntents(battle, *view.defs);
    const ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersInnerV |
                                  ImGuiTableFlags_SizingFixedFit;
    if (!ImGui::BeginTable("intents", 6, flags)) {
        return;
    }
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("unit");
    ImGui::TableSetupColumn("role");
    ImGui::TableSetupColumn("order");
    ImGui::TableSetupColumn("score");
    ImGui::TableSetupColumn("best (score)");
    ImGui::TableSetupColumn("aggr");
    ImGui::TableHeadersRow();
    for (const AiIntent &intent : intents) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(unitLabel(battle, intent.id).c_str());
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(roleName(intent.role));
        ImGui::TableNextColumn();
        if (intent.order.type == Order::Type::Attack) {
            ImGui::Text("attack %s", unitLabel(battle, intent.order.target).c_str());
        } else if (intent.order.type == Order::Type::Move) {
            ImGui::Text("move (%.0f, %.0f)", static_cast<double>(intent.order.point.x),
                        static_cast<double>(intent.order.point.y));
        } else {
            ImGui::TextUnformatted("hold");
        }
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(intent.score ? cell(*intent.score).c_str() : "-");
        ImGui::TableNextColumn();
        if (intent.best == kNoUnit) {
            ImGui::TextUnformatted("-");
        } else {
            ImGui::Text("%s (%s)", unitLabel(battle, intent.best).c_str(), cell(intent.bestScore).c_str());
        }
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(intent.aggressive ? "yes" : "no");
    }
    ImGui::EndTable();
}

void DevTools::constantsTab() {
    ImGui::InputText("filter", filter_.data(), filter_.size());
    ImGui::TextDisabled("%zu constants from src/core/rules/Constants.h (read-only)", std::size(kConstants));
    const ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersInnerV;
    if (!ImGui::BeginTable("constants", 2, flags)) {
        return;
    }
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("name");
    ImGui::TableSetupColumn("value");
    ImGui::TableHeadersRow();
    const std::string_view needle(filter_.data());
    for (const NamedConstant &constant : kConstants) {
        if (!contains(constant.name, needle)) {
            continue;
        }
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(constant.name);
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(cell(constant.value).c_str());
    }
    ImGui::EndTable();
}

void DevTools::drawOverlay(sf::RenderTarget &target, const Battle &battle, const GameDefs &defs, float alpha) const {
    if (!visible_ || !lines_) {
        return;
    }
    sf::VertexArray lines(sf::PrimitiveType::Lines);
    const sf::Color attack(0xd0, 0x30, 0x30, 220);
    const sf::Color move(0x30, 0x60, 0xd0, 200);
    for (const AiIntent &intent : aiIntents(battle, defs)) {
        const Vec2 from = drawn(battle.unit(intent.id), alpha);
        if (intent.order.type == Order::Type::Attack && intent.order.target < battle.units().size()) {
            const Vec2 to = drawn(battle.unit(intent.order.target), alpha);
            lines.append(sf::Vertex{{from.x, from.y}, attack});
            lines.append(sf::Vertex{{to.x, to.y}, attack});
        } else if (intent.order.type == Order::Type::Move) {
            lines.append(sf::Vertex{{from.x, from.y}, move});
            lines.append(sf::Vertex{{intent.order.point.x, intent.order.point.y}, move});
        }
    }
    target.draw(lines);
}

void DevTools::render(sf::RenderWindow &window) {
    if (attached_) {
        ImGui::SFML::Render(window);
    }
}

} // namespace tq::dev
