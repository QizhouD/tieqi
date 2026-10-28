#include "app/App.h"

#include "app/SaveData.h"
#include "app/TextStyle.h"
#include "core/rules/Constants.h"
#include "data/Replay.h"

#include <SFML/System/Clock.hpp>
#include <SFML/System/String.hpp>

#include <algorithm>
#include <string_view>

namespace tq {
namespace {

sf::String utf8(std::string_view text) { return sf::String::fromUtf8(text.begin(), text.end()); }

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

void submitRecorded(Battle &battle, const ReplayCommand &command) {
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
    if (!issued.units.empty()) {
        battle.submit(std::move(issued));
    }
}

} // namespace

App::App(std::filesystem::path root, LaunchOptions options, std::filesystem::path executableDir)
    : root_(std::move(root)) {
    std::uint64_t seed = options.seed;
    bool ai = true;
    if (!options.replay.empty()) {
        Replay replay = loadReplay(options.replay);
        if (!options.scenarioSet) {
            options.scenario = replay.scenario;
        }
        if (!options.seedSet) {
            seed = replay.seed;
        }
        ai = replay.ai;
        replay_ = std::move(replay.commands);
        std::stable_sort(replay_.begin(), replay_.end(),
                         [](const ReplayCommand &left, const ReplayCommand &right) { return left.tick < right.tick; });
    }

    data_ = loadGameData(root_ / "data", options.scenario + ".json");
    en_ = loadStrings(root_ / "data" / "strings" / "en.json", &data_.zhHant);
    ko_ = loadStrings(root_ / "data" / "strings" / "ko.json", &data_.zhHant);
    if (options.timeScale) {
        data_.config.timeScale = *options.timeScale;
    }

    language_ = data_.config.language;
    speed_ = data_.config.defaultSpeed;
    reducedMotion_ = data_.config.reducedMotion;
    audio_.setMasterVolume(70.f);

    SaveData saved;
    if (saved.load(executableDir / "settings.txt")) {
        language_ = saved.language;
        reducedMotion_ = saved.reducedMotion;
        audio_.setMasterVolume(saved.volume);
        const auto &speeds = data_.config.playerSpeeds;
        if (std::find(speeds.begin(), speeds.end(), saved.speed) != speeds.end()) {
            speed_ = saved.speed;
        }
    }
    audio_.setMuted(!data_.config.sound || audio_.masterVolume() <= 0.f);
    audio_.loadDefaults(root_ / "assets" / "audio");

    BattleOptions battleOptions;
    battleOptions.aiEnabled = ai;
    battle_.emplace(data_.defs, data_.scenario, seed, battleOptions);

    window_.open(utf8(active().text("game.title")));
    const MapDef &map = data_.scenario.map;
    window_.setWorld({static_cast<unsigned>(map.width), static_cast<unsigned>(map.height)});
    window_.snapCamera({map.width * 0.5f, map.height * 0.5f});
    mapView_.build(data_.scenario, active(), fontFor(active(), FontFace::Brush));
    if (!options.script.empty()) {
        window_.loadScript(options.script);
    }
}

const StringTable &App::active() const {
    if (language_ == "ko") {
        return ko_;
    }
    if (language_ == "en") {
        return en_;
    }
    return data_.zhHant;
}

const sf::Font &App::fontFor(const StringTable &table, FontFace face) {
    const std::string &file = face == FontFace::Brush ? table.fontBrush : table.fontBody;
    return resources_.font(root_ / "assets" / "fonts" / file);
}

void App::stepSim() {
    if (!battle_ || battle_->isOver()) {
        return;
    }
    while (replayCursor_ < replay_.size() && replay_[replayCursor_].tick == battle_->tick() + 1) {
        submitRecorded(*battle_, replay_[replayCursor_]);
        replayCursor_ += 1;
    }
    battle_->step();
    for (const Event &event : battle_->drainEvents()) {
        if (event.type == Event::Type::Impact) {
            mapView_.addScar(event.pos, event.value);
        }
    }
}

void App::queueSfx() {
    cues_.push_back({soundTime_, "thud", false});
    cues_.push_back({soundTime_ + 0.50f, "crack", false});
    cues_.push_back({soundTime_ + 0.70f, "drum", false});
    cues_.push_back({soundTime_ + 2.30f, "rout", false});
}

void App::playDue() {
    for (Cue &cue : cues_) {
        if (!cue.played && soundTime_ >= cue.time) {
            audio_.play(cue.name);
            cue.played = true;
        }
    }
}

void App::drawFrame() {
    window_.beginWorld();
    const bool exitOpen = battle_ && battle_->exitOpen();
    mapView_.draw(window_, soundTime_, exitOpen);
}

void App::run() {
    sf::Clock frameClock;
    while (window_.isOpen()) {
        const float frame = std::min(frameClock.restart().asSeconds(), 0.25f);
        window_.poll();
        if (window_.wasKeyPressed(sf::Keyboard::Key::F6)) {
            queueSfx();
        }
        soundTime_ += frame;
        playDue();
        if (battle_ && !battle_->isOver()) {
            accumulator_ += frame * data_.config.timeScale / kTickSeconds * speed_;
            for (int n = 0; accumulator_ >= 1.f && n < data_.config.maxTicksPerFrame; ++n) {
                stepSim();
                accumulator_ -= 1.f;
            }
        }
        window_.beginDraw();
        drawFrame();
        window_.endDraw();
    }
}

} // namespace tq
