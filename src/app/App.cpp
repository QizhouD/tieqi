#include "app/App.h"

#include "app/SaveData.h"
#include "app/TextStyle.h"
#include "core/rules/Constants.h"
#include "data/Replay.h"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/String.hpp>

#include <algorithm>
#include <cmath>
#include <string_view>

namespace tq {
namespace {

sf::String utf8(std::string_view text) { return sf::String::fromUtf8(text.begin(), text.end()); }

const sf::Color kLacquer{0x1e, 0x14, 0x11};
const sf::Color kPaper{0xee, 0xe3, 0xcc};
const sf::Color kGold{0xcf, 0xaa, 0x5c};
const sf::Color kCinnabar{0xa8, 0x38, 0x2a};
const sf::Color kSealFg{0xf4, 0xdb, 0xa4};
const sf::Color kRing{0xd9, 0xb4, 0x66};

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

void drawText(Window &window, const sf::Font &font, std::string_view text, unsigned size, sf::Vector2f pos,
              sf::Color color, bool bold) {
    sf::Text shape(font, utf8(text), size);
    shape.setPosition(pos);
    shape.setFillColor(color);
    if (bold) {
        shape.setStyle(sf::Text::Bold);
    }
    window.draw(shape);
}

void drawGlyph(Window &window, const sf::Font &font, std::string_view text, sf::Vector2f center) {
    constexpr float side = 220.f;
    sf::RectangleShape seal({side, side});
    seal.setOrigin({side * 0.5f, side * 0.5f});
    seal.setPosition(center);
    seal.setFillColor(kCinnabar);
    seal.setOutlineColor(kRing);
    seal.setOutlineThickness(8.f);
    window.draw(seal);

    sf::Text shape(font, utf8(text), 140);
    shape.setFillColor(kSealFg);
    const sf::FloatRect bounds = shape.getLocalBounds();
    shape.setOrigin({bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f});
    shape.setPosition(center);
    window.draw(shape);
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
    battle_->drainEvents();
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
    sf::RectangleShape field({static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});
    field.setFillColor(kLacquer);
    window_.draw(field);

    const TextStyle zhTitle = textStyle(data_.zhHant.language, TextRole::Title);
    const TextStyle zhBody = textStyle(data_.zhHant.language, TextRole::Body);
    const TextStyle koTitle = textStyle(ko_.language, TextRole::Title);
    const TextStyle enTitle = textStyle(en_.language, TextRole::Title);
    const TextStyle glyph = textStyle(data_.zhHant.language, TextRole::Glyph);

    drawText(window_, fontFor(data_.zhHant, zhTitle.face), data_.zhHant.text("game.title"), 72, {88.f, 72.f}, kGold,
             zhTitle.bold);
    drawText(window_, fontFor(data_.zhHant, zhBody.face), data_.zhHant.text("ui.subtitle"), 36, {88.f, 180.f}, kPaper,
             zhBody.bold);
    drawText(window_, fontFor(ko_, koTitle.face), ko_.text("place.byeokjegwan"), 64, {88.f, 300.f}, kPaper,
             koTitle.bold);
    drawText(window_, fontFor(en_, enTitle.face), en_.text("game.title"), 64, {88.f, 420.f}, kPaper, enTitle.bold);
    drawGlyph(window_, fontFor(data_.zhHant, glyph.face), data_.zhHant.text("glyph.cavalry"), {220.f, 760.f});
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
