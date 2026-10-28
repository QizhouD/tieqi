#include "app/App.h"

#include "app/SaveData.h"
#include "app/TextStyle.h"
#include "core/rules/Constants.h"
#include "data/Replay.h"

#include <SFML/System/Clock.hpp>
#include <SFML/System/String.hpp>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <optional>
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

void playEventAudio(AudioManager &audio, const Event &event) {
    switch (event.type) {
    case Event::Type::Impact: {
        const float intensity = std::min(1.f, 0.4f + 0.25f * event.value);
        audio.play("thud", intensity * 100.f);
        break;
    }
    case Event::Type::ThreeEyedVolley:
    case Event::Type::TeppoVolley:
        audio.play("crack");
        break;
    case Event::Type::Broken:
        audio.play("rout");
        break;
    case Event::Type::PhaseChanged:
    case Event::Type::Reinforcement:
        audio.play("drum");
        break;
    default:
        break;
    }
}

std::optional<OrderKey> orderKeyFromKeyboard(sf::Keyboard::Key key) {
    switch (key) {
    case sf::Keyboard::Key::Space:
        return OrderKey::Space;
    case sf::Keyboard::Key::Num1:
        return OrderKey::Speed1;
    case sf::Keyboard::Key::Num2:
        return OrderKey::Speed2;
    case sf::Keyboard::Key::Num3:
        return OrderKey::Speed3;
    case sf::Keyboard::Key::H:
        return OrderKey::Hold;
    case sf::Keyboard::Key::R:
        return OrderKey::Retreat;
    case sf::Keyboard::Key::F:
        return OrderKey::Split;
    case sf::Keyboard::Key::G:
        return OrderKey::Join;
    case sf::Keyboard::Key::A:
        return OrderKey::All;
    case sf::Keyboard::Key::Escape:
        return OrderKey::Cancel;
    default:
        return std::nullopt;
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
    perf_ = options.perf;
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
    if (perf_) {
        speed_ = 2.f;
    }
    audio_.setMuted(!data_.config.sound || audio_.masterVolume() <= 0.f);
    audio_.loadDefaults(root_ / "assets" / "audio");

    seed_ = seed;
    aiEnabled_ = ai;
    freezeAt_ = options.freezeAt;
    BattleOptions battleOptions;
    battleOptions.aiEnabled = aiEnabled_;
    battle_.emplace(data_.defs, data_.scenario, seed_, battleOptions);

    window_.open(utf8(active().text("game.title")));
    const MapDef &map = data_.scenario.map;
    window_.setWorld({static_cast<unsigned>(map.width), static_cast<unsigned>(map.height)});
    window_.snapCamera({map.width * 0.5f, map.height * 0.5f});
    const TextStyle title = textStyle(active().language, TextRole::Title);
    mapView_.build(data_.scenario, active(), fontFor(active(), title.face), title.bold);
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
    const std::vector<Event> events = battle_->drainEvents();
    for (const Event &event : events) {
        if (event.type == Event::Type::Impact) {
            mapView_.addScar(event.pos, event.value);
        }
    }
    noteEvents(orders_, *battle_, events);
    hud_.note(*battle_, active(), events);
    effects_.onEvents(*battle_, active(), events, reducedMotion_, assaultPoint());
    for (const Event &event : events) {
        playEventAudio(audio_, event);
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

bool App::frozen() const {
    if (perf_ || !freezeAt_ || !battle_) {
        return false;
    }
    return battle_->tick() >= *freezeAt_;
}

std::optional<Vec2> App::assaultPoint() const {
    if (!battle_ || !data_.scenario.flow.enabled) {
        return std::nullopt;
    }
    const std::optional<UnitId> id = battle_->sceneUnit(data_.scenario.flow.phase3.grandAssault.general);
    if (!id) {
        return std::nullopt;
    }
    const Unit &unit = battle_->unit(*id);
    if (unit.state != UnitState::Steady && unit.state != UnitState::Wavering) {
        return std::nullopt;
    }
    return unit.pos;
}

void App::drawFrame() {
    window_.setViewShake(effects_.shakePixels());
    window_.beginWorld();
    const bool exitOpen = battle_ && battle_->exitOpen();
    mapView_.draw(window_, soundTime_, exitOpen);
    if (!battle_) {
        return;
    }
    const TextStyle glyph = textStyle(active().language, TextRole::Glyph);
    const TextStyle body = textStyle(active().language, TextRole::Body);
    window_.beginMap();
    const TextStyle floater = textStyle(active().language, TextRole::Floater);
    const float alpha = std::clamp(accumulator_, 0.f, 1.f);
    unitView_.draw(
        window_, *battle_, active(), fontFor(active(), glyph.face), fontFor(active(), body.face), soundTime_, alpha,
        orders_, [&] { effects_.drawDust(window_.target()); },
        [&] {
            effects_.drawAir(window_.target(), fontFor(active(), floater.face), floater.bold, window_.pixelsPerMeter());
        });
    window_.beginPixels();
    const Window::PixelMap pixels = window_.pixelMap();
    const TextStyle title = textStyle(active().language, TextRole::Title);
    HudFrame frame;
    frame.battle = &*battle_;
    frame.orders = &orders_;
    frame.strings = &active();
    frame.body = &fontFor(active(), FontFace::Body);
    frame.title = &fontFor(active(), title.face);
    frame.titleBold = title.bold;
    frame.exit = &data_.scenario.exit;
    frame.startClock = data_.scenario.startClock;
    frame.alpha = std::clamp(accumulator_, 0.f, 1.f);
    frame.wallTime = soundTime_;
    frame.running = running_;
    frame.muted = audio_.muted();
    frame.reducedMotion = reducedMotion_;
    frame.speed = speed_;
    frame.speeds = &data_.config.playerSpeeds;
    frame.originX = pixels.originX;
    frame.originY = pixels.originY;
    frame.scale = pixels.scale;
    hud_.draw(window_.target(), frame);
}

void App::issue(Command command) {
    if (!battle_ || command.units.empty()) {
        return;
    }
    if (command.type == Command::Type::Move) {
        effects_.marker(command.point, false);
    } else if (command.type == Command::Type::Attack && command.target < battle_->units().size()) {
        effects_.marker(battle_->unit(command.target).pos, true);
    }
    if (window_.scripted()) {
        std::cout << commandText(command) << '\n';
    }
    battle_->submit(std::move(command));
}

void App::applyHud(HudAction action) {
    if (!battle_) {
        return;
    }
    const std::vector<float> &speeds = data_.config.playerSpeeds;
    if (action == HudAction::ToggleRun) {
        toggleRunning(running_, *battle_);
        return;
    }
    if (action == HudAction::ToggleSound) {
        audio_.setMuted(!audio_.muted());
        return;
    }
    if (action == HudAction::Restart) {
        restart();
        return;
    }
    if (action == HudAction::Speed0 || action == HudAction::Speed1 || action == HudAction::Speed2) {
        const std::size_t index = action == HudAction::Speed0 ? 0 : action == HudAction::Speed1 ? 1 : 2;
        if (index < speeds.size()) {
            speed_ = speeds[index];
        }
        return;
    }
    if (action == HudAction::ConfirmAttack) {
        for (Command command : confirmPending(orders_, *battle_)) {
            issue(std::move(command));
        }
        return;
    }
    OrderKey key = OrderKey::Cancel;
    if (action == HudAction::Hold) {
        key = OrderKey::Hold;
    } else if (action == HudAction::Retreat) {
        key = OrderKey::Retreat;
    } else if (action == HudAction::Split) {
        key = OrderKey::Split;
    } else if (action == HudAction::Join) {
        key = OrderKey::Join;
    }
    OrderEffect effect = applyKey(orders_, *battle_, key, speeds);
    for (Command &command : effect.commands) {
        issue(std::move(command));
    }
}

void App::restart() {
    BattleOptions options;
    options.aiEnabled = aiEnabled_;
    battle_.emplace(data_.defs, data_.scenario, seed_, options);
    orders_ = {};
    hud_.reset();
    effects_.clear();
    accumulator_ = 0.f;
    running_ = false;
    replayCursor_ = 0;
    const TextStyle title = textStyle(active().language, TextRole::Title);
    mapView_.build(data_.scenario, active(), fontFor(active(), title.face), title.bold);
}

void App::handleInput() {
    if (!battle_) {
        return;
    }
    const float alpha = std::clamp(accumulator_, 0.f, 1.f);
    const float mapW = data_.scenario.map.width;
    const float mapH = data_.scenario.map.height;
    for (const PointerSample &sample : window_.takePointers()) {
        const Hud::Input input = hud_.handle(sample);
        if (input.action != HudAction::None) {
            applyHud(input.action);
        }
        if (input.consumed) {
            continue;
        }
        for (Command command : applyPointer(orders_, *battle_, sample, mapW, mapH, alpha)) {
            issue(std::move(command));
        }
    }
    for (sf::Keyboard::Key key : window_.pressedKeys()) {
        const std::optional<OrderKey> order = orderKeyFromKeyboard(key);
        if (!order) {
            continue;
        }
        OrderEffect effect = applyKey(orders_, *battle_, *order, data_.config.playerSpeeds);
        if (effect.toggleRun) {
            toggleRunning(running_, *battle_);
        }
        if (effect.speed) {
            speed_ = *effect.speed;
        }
        for (Command &command : effect.commands) {
            issue(std::move(command));
        }
    }
}

namespace {

void printPerf(std::vector<float> samples) {
    if (samples.empty()) {
        std::cout << "perf mean 0.0 ms p95 0.0 ms max 0.0 ms\n";
        return;
    }
    const double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
    const double mean = sum / static_cast<double>(samples.size());
    std::sort(samples.begin(), samples.end());
    const std::size_t count = samples.size();
    const auto rank = static_cast<std::size_t>(std::ceil(0.95 * static_cast<double>(count)));
    const std::size_t index = std::min(count - 1, rank == 0 ? 0 : rank - 1);
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "perf mean " << mean << " ms p95 " << samples[index] << " ms max " << samples.back() << " ms\n";
}

} // namespace

void App::run() {
    if (perf_) {
        window_.unlimitedFrameRate();
        speed_ = 2.f;
        running_ = true;
    }
    sf::Clock frameClock;
    std::vector<float> samples;
    bool frameOpen = false;
    while (window_.isOpen()) {
        const float raw = frameClock.restart().asSeconds();
        if (perf_ && frameOpen) {
            samples.push_back(raw * 1000.f);
        }
        frameOpen = true;
        const float frame = std::min(raw, 0.25f);
        window_.poll();
        handleInput();
        if (window_.wasKeyPressed(sf::Keyboard::Key::F6)) {
            queueSfx();
        }
        soundTime_ += frame;
        playDue();
        const bool simHold = !battle_ || !running_ || battle_->isOver() || frozen();
        const bool hitStop = !simHold && effects_.blocking();
        if (!simHold) {
            if (hitStop) {
                effects_.advanceHitStop(frame);
            } else {
                accumulator_ += frame * data_.config.timeScale / kTickSeconds * speed_;
                for (int n = 0; accumulator_ >= 1.f && n < data_.config.maxTicksPerFrame; ++n) {
                    if (frozen()) {
                        break;
                    }
                    stepSim();
                    accumulator_ -= 1.f;
                }
            }
        }
        effects_.update(effectSeconds(frame, speed_, data_.config.timeScale, simHold || hitStop), frame);
        window_.beginDraw();
        drawFrame();
        window_.endDraw();
        if (perf_ && battle_ && battle_->isOver()) {
            samples.push_back(frameClock.restart().asSeconds() * 1000.f);
            printPerf(samples);
            window_.close();
        }
    }
}

} // namespace tq
