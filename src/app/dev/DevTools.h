#pragma once

// Compiled only with TQ_DEV_TOOLS=ON. Release builds never include this header.

#include "app/dev/Intent.h"
#include "core/sim/Battle.h"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace tq::dev {

struct DevView {
    const Battle *battle = nullptr;
    const GameDefs *defs = nullptr;
    std::optional<UnitId> selected;
    bool running = false;
    std::size_t recorded = 0;
};

// What the panel asked for this frame. The app applies it; the panel never touches the battle.
struct DevRequests {
    bool stepTick = false;
    std::vector<Command> commands;
    std::optional<std::filesystem::path> save;
    std::optional<std::filesystem::path> load;
};

class DevTools {
  public:
    DevTools() = default;
    DevTools(const DevTools &) = delete;
    DevTools &operator=(const DevTools &) = delete;
    ~DevTools();

    void attach(sf::RenderWindow &window);
    // Returns true when ImGui wants this input, so the game should ignore it.
    bool handle(const sf::RenderWindow &window, const sf::Event &event);
    void begin(sf::RenderWindow &window, float seconds);
    DevRequests build(const DevView &view);
    // Target lines in map coordinates; call with the map view active.
    void drawOverlay(sf::RenderTarget &target, const Battle &battle, const GameDefs &defs, float alpha) const;
    void render(sf::RenderWindow &window);

    void toggle() { visible_ = !visible_; }
    // Moves to the next tab. Scripts use it, since scripted keys never reach ImGui.
    void nextTab() { tabPending_ = (tab_ + 1) % kTabs; }
    bool visible() const { return visible_; }
    void noteTimes(float simMs, float drawMs);
    void setStatus(std::string status) { status_ = std::move(status); }

  private:
    void simTab(const DevView &view, DevRequests &requests);
    void unitsTab(const DevView &view, DevRequests &requests);
    void spawnTab(const DevView &view, DevRequests &requests);
    void aiTab(const DevView &view);
    void constantsTab();

    static constexpr int kTabs = 5;
    bool attached_ = false;
    int tab_ = 0;
    std::optional<int> tabPending_;
    bool visible_ = false;
    bool lines_ = true;
    std::optional<UnitId> inspected_;
    int spawnType_ = 0;
    float spawnX_ = 0.f;
    float spawnY_ = 0.f;
    float spawnFaceDeg_ = -90.f;
    bool spawnPlaced_ = false;
    std::array<char, 260> replayPath_{};
    std::array<char, 64> filter_{};
    TimeSeries sim_;
    TimeSeries draw_;
    std::string status_;
};

} // namespace tq::dev
