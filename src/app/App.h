#pragma once

#include "app/AudioManager.h"
#include "app/Launch.h"
#include "app/MapView.h"
#include "app/Orders.h"
#include "app/ResourceManager.h"
#include "app/TextStyle.h"
#include "app/UnitView.h"
#include "app/Window.h"
#include "core/sim/Battle.h"
#include "data/Load.h"
#include "data/Replay.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace tq {

class App {
  public:
    App(std::filesystem::path root, LaunchOptions options, std::filesystem::path executableDir);
    void run();

  private:
    void stepSim();
    void drawFrame();
    void handleInput();
    void issue(Command command);
    void queueSfx();
    void playDue();
    const StringTable &active() const;
    const sf::Font &fontFor(const StringTable &table, FontFace face);

    std::filesystem::path root_;
    GameData data_;
    StringTable en_;
    StringTable ko_;
    std::string language_ = "zh-Hant";
    float speed_ = 1.f;
    bool reducedMotion_ = false;
    std::optional<Battle> battle_;
    std::vector<ReplayCommand> replay_;
    std::size_t replayCursor_ = 0;
    ResourceManager resources_;
    AudioManager audio_;
    Window window_;
    MapView mapView_;
    UnitView unitView_;
    OrderState orders_;
    bool running_ = false;

    struct Cue {
        float time = 0.f;
        std::string name;
        bool played = false;
    };
    std::vector<Cue> cues_;
    float soundTime_ = 0.f;
    float accumulator_ = 0.f;
};

} // namespace tq
