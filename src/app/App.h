#pragma once

#include "app/AudioManager.h"
#include "app/CameraControl.h"
#include "app/Effects.h"
#include "app/Hud.h"
#include "app/Launch.h"
#include "app/MapView.h"
#include "app/Orders.h"
#include "app/ResourceManager.h"
#include "app/SoldierAtlas.h"
#include "app/SoldierView.h"
#include "app/TextCache.h"
#include "app/TextStyle.h"
#include "app/UnitView.h"
#include "app/Window.h"
#include "core/sim/Battle.h"
#include "data/Load.h"
#include "data/Replay.h"
#ifdef TQ_DEV_TOOLS
#include "app/dev/DevTools.h"
#endif

#include <filesystem>
#include <map>
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
    void applyHud(HudAction action);
    void setLanguage(const std::string &language);
    void saveSettings() const;
    void restart();
    void issue(Command command);
    void queueSfx();
    void playDue();
    const StringTable &active() const;
    const sf::Font &fontFor(const StringTable &table, FontFace face);
    bool frozen() const;
    std::optional<Vec2> assaultPoint() const;
#ifdef TQ_DEV_TOOLS
    void applyDev(dev::DevRequests requests);
#endif

    std::filesystem::path root_;
    std::filesystem::path settingsFile_;
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
    SoldierAtlas soldierAtlas_;
    SoldierView soldierView_;
    // Finished unit portraits found at startup, by unit type id (V4).
    std::map<std::string, const sf::Texture *> portraits_;
    // Presentation time for soldier animation; stops while paused, during hit-stop, and when frozen.
    float animTime_ = 0.f;
    OrderState orders_;
    CameraControl camera_;
    Hud hud_;
    bool running_ = false;
    std::uint64_t seed_ = 1;
    bool aiEnabled_ = true;

    struct Cue {
        float time = 0.f;
        std::string name;
        bool played = false;
    };
    std::vector<Cue> cues_;
    float soundTime_ = 0.f;
    float accumulator_ = 0.f;
    bool perf_ = false;
    std::optional<std::uint32_t> freezeAt_;
    Effects effects_;
    TextCacheGuard textCache_;
#ifdef TQ_DEV_TOOLS
    dev::DevTools dev_;
    // Every command that reached the battle since the last restart, for saving a replay.
    std::vector<ReplayCommand> recorded_;
#endif
};

} // namespace tq
