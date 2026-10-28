#include "app/Hud.h"

#include "app/TextCache.h"
#include "app/TextStyle.h"
#include "core/rules/Constants.h"

#include <SFML/Graphics/RectangleShape.hpp>

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>

namespace tq {
namespace {

sf::Color rgb(unsigned hex, unsigned alpha = 255) {
    return {static_cast<std::uint8_t>((hex >> 16) & 255), static_cast<std::uint8_t>((hex >> 8) & 255),
            static_cast<std::uint8_t>(hex & 255), static_cast<std::uint8_t>(alpha)};
}

const sf::Color kPaper = rgb(0xeee3cc);
const sf::Color kGold = rgb(0xcfaa5c);
const sf::Color kMuted = rgb(0xa8957f);
const sf::Color kLacquer = rgb(0x1e1411);
const sf::Color kLacquer2 = rgb(0x291b16);
const sf::Color kLacquer3 = rgb(0x35241d);
const sf::Color kRule = rgb(0x4b352a);
const sf::Color kCinnabar = rgb(0xa8382a);
const sf::Color kInk = rgb(0x2a221b);
const sf::Color kSilk = rgb(0xebdfc3);
const sf::Color kMing = rgb(0xe48a72);
const sf::Color kJp = rgb(0xa9b8d8);
const sf::Color kOk = rgb(0x86ad80);
const sf::Color kWarn = rgb(0xd8a748);
const sf::Color kBad = rgb(0xdd6249);
const sf::Color kFlee = rgb(0x958677);
const sf::Color kAsh = rgb(0x8aa0b8);
const sf::Color kCream = rgb(0xf6e2b4);

unsigned fontPx(float logical, float scale) {
    const long rounded = std::lround(static_cast<double>(logical) * static_cast<double>(scale));
    return rounded > 1 ? static_cast<unsigned>(rounded) : 1u;
}

std::string speedText(float speed) {
    std::ostringstream out;
    if (std::fabs(speed - std::round(speed)) < 0.05f) {
        out << static_cast<int>(std::lround(speed));
    } else {
        out.setf(std::ios::fixed);
        out.precision(1);
        out << static_cast<double>(speed);
    }
    out << "\u00d7";
    return out.str();
}

sf::Color stateColor(UnitState state) {
    switch (state) {
    case UnitState::Wavering:
        return kWarn;
    case UnitState::Broken:
        return kBad;
    case UnitState::Routing:
        return kFlee;
    case UnitState::Steady:
    case UnitState::Gone:
        return kOk;
    }
    return kOk;
}

sf::Color tagColor(TagInk ink) {
    switch (ink) {
    case TagInk::Hot:
        return kMing;
    case TagInk::Warn:
        return kWarn;
    case TagInk::Bad:
        return kBad;
    case TagInk::Ok:
        return kOk;
    case TagInk::Plain:
        return kPaper;
    }
    return kPaper;
}

sf::Color verdictColor(Verdict verdict) {
    const PreviewPaint paint = previewPaint(verdict);
    return rgb(static_cast<unsigned>(paint.color.r) << 16 | static_cast<unsigned>(paint.color.g) << 8 | paint.color.b,
               255);
}

sf::Color figureColor(ModRow::Ink ink) {
    switch (ink) {
    case ModRow::Ink::Cinnabar:
        return kCinnabar;
    case ModRow::Ink::Ash:
        return kAsh;
    case ModRow::Ink::Note:
        return kWarn;
    case ModRow::Ink::Plain:
        return kPaper;
    }
    return kPaper;
}

struct Pen {
    sf::RenderTarget *target = nullptr;
    float ox = 0.f;
    float oy = 0.f;
    float scale = 1.f;
    const sf::Font *body = nullptr;
    const sf::Font *title = nullptr;
    bool titleBold = false;

    sf::Vector2f px(float x, float y) const { return {std::round(ox + x * scale), std::round(oy + y * scale)}; }

    void fill(float x, float y, float w, float h, sf::Color color) const {
        const sf::Vector2f a = px(x, y);
        const sf::Vector2f b = px(x + w, y + h);
        const float width = std::max(1.f, b.x - a.x);
        const float height = std::max(1.f, b.y - a.y);
        sf::RectangleShape shape({width, height});
        shape.setPosition(a);
        shape.setFillColor(color);
        target->draw(shape);
    }

    void stroke(float x, float y, float w, float h, sf::Color color) const {
        const sf::Vector2f a = px(x, y);
        const sf::Vector2f b = px(x + w, y + h);
        const float right = std::max(a.x + 1.f, b.x);
        const float bottom = std::max(a.y + 1.f, b.y);
        sf::RectangleShape top({right - a.x, 1.f});
        top.setPosition(a);
        top.setFillColor(color);
        target->draw(top);
        sf::RectangleShape edge({1.f, bottom - a.y});
        edge.setPosition(a);
        edge.setFillColor(color);
        target->draw(edge);
        edge.setPosition({right - 1.f, a.y});
        target->draw(edge);
        sf::RectangleShape low({right - a.x, 1.f});
        low.setPosition({a.x, bottom - 1.f});
        low.setFillColor(color);
        target->draw(low);
    }

    float measure(const sf::Font &font, std::string_view text, unsigned size, bool bold) const {
        return textBounds(font, size, bold, text).size.x;
    }

    void text(const sf::Font &font, std::string_view value, float size, float x, float y, sf::Color color, bool bold,
              float align = 0.f) const {
        const unsigned pixels = fontPx(size, scale);
        const sf::FloatRect bounds = textBounds(font, pixels, bold, value);
        const sf::Vector2f snapped = px(x, y);
        float left = snapped.x;
        if (align > 0.f) {
            left -= std::round(bounds.size.x * align);
        }
        drawText(*target, font, pixels, bold, value, color, {left, snapped.y}, {bounds.position.x, bounds.position.y},
                 {1.f, 1.f});
    }

    std::vector<std::string> wrap(const sf::Font &font, std::string_view value, float size, float maxLogical,
                                  bool bold) const {
        const unsigned pixels = fontPx(size, scale);
        return wrapText(font, value, pixels, std::max(8.f, maxLogical * scale), bold);
    }
};

float bannerAlpha(float age, bool reduced) {
    const float stretch = reduced ? 1.5f : 1.f;
    const float fadeIn = 0.35f * stretch;
    const float hold = 2.5f * stretch;
    const float fadeOut = 0.7f * stretch;
    if (age < 0.f || age >= fadeIn + hold + fadeOut) {
        return 0.f;
    }
    if (age < fadeIn) {
        return age / fadeIn;
    }
    if (age < fadeIn + hold) {
        return 1.f;
    }
    return 1.f - (age - fadeIn - hold) / fadeOut;
}

} // namespace

void Hud::reset() {
    journal_.reset();
    hits_.clear();
    armed_.reset();
    shownBanner_ = 0;
    bannerStart_ = 0.f;
}

void Hud::note(const Battle &battle, const StringTable &strings, std::span<const Event> events) {
    journal_.append(battle, strings, events);
}

Hud::Input Hud::handle(const PointerSample &sample) {
    Input input;
    if (sample.type == PointerSample::Type::Move || sample.button != kPointerLeft) {
        return input;
    }
    const Hit *found = nullptr;
    for (auto it = hits_.rbegin(); it != hits_.rend(); ++it) {
        if (sample.x >= it->x && sample.y >= it->y && sample.x < it->x + it->w && sample.y < it->y + it->h) {
            found = &(*it);
            break;
        }
    }
    if (sample.type == PointerSample::Type::Down) {
        if (found != nullptr) {
            armed_ = found->action;
            input.consumed = true;
        } else {
            armed_.reset();
        }
        return input;
    }
    if (armed_ && found != nullptr && found->action == *armed_) {
        input.consumed = true;
        input.action = *armed_;
    }
    armed_.reset();
    return input;
}

void Hud::draw(sf::RenderTarget &target, const HudFrame &frame) {
    if (frame.battle == nullptr || frame.orders == nullptr || frame.strings == nullptr || frame.body == nullptr ||
        frame.title == nullptr || frame.speeds == nullptr) {
        return;
    }
    const Battle &battle = *frame.battle;
    const OrderState &orders = *frame.orders;
    const StringTable &strings = *frame.strings;
    Pen pen;
    pen.target = &target;
    pen.ox = frame.originX;
    pen.oy = frame.originY;
    pen.scale = frame.scale > 0.f ? frame.scale : 1.f;
    pen.body = frame.body;
    pen.title = frame.title;
    pen.titleBold = frame.titleBold;
    hits_.clear();

    const auto click = [&](HudAction action, float x, float y, float w, float h) {
        hits_.push_back(Hit{action, x, y, w, h});
    };
    const auto button = [&](float x, float y, float w, float h, std::string_view label, HudAction action, bool enabled,
                            bool pressed, bool accent) {
        sf::Color fill = pressed ? kGold : kLacquer3;
        sf::Color ink = pressed ? kLacquer : kPaper;
        if (accent && enabled) {
            fill = kCinnabar;
            ink = kCream;
        }
        if (!enabled) {
            fill = rgb(0x35241d, 120);
            ink = rgb(0xeee3cc, 120);
        }
        pen.fill(x, y, w, h, fill);
        pen.stroke(x, y, w, h, pressed ? kGold : kRule);
        pen.text(*pen.body, label, 13.f, x + w * 0.5f, y + 6.f, ink, false, 0.5f);
        if (enabled && action != HudAction::None) {
            click(action, x, y, w, h);
        }
    };

    pen.fill(0.f, 0.f, 1600.f, 54.f, kLacquer);
    pen.fill(0.f, 54.f, 16.f, 946.f, kLacquer);
    pen.fill(1234.f, 54.f, 366.f, 946.f, kLacquer);
    pen.fill(16.f, 988.f, 1218.f, 12.f, kLacquer);
    pen.stroke(16.f, 54.f, 1218.f, 934.f, kRule);

    const TextStyle titleStyle = textStyle(strings.language, TextRole::Title);
    const sf::Font &titleFont = titleStyle.face == FontFace::Brush ? *pen.title : *pen.body;
    const bool titleBold = titleStyle.bold;
    pen.text(titleFont, strings.text("ui.battle"), 32.f, 16.f, 8.f, kGold, titleBold);
    const float titleWidth =
        pen.measure(titleFont, strings.text("ui.battle"), fontPx(32.f, pen.scale), titleBold) / pen.scale;
    pen.text(*pen.body, strings.text("ui.subtitle"), 12.f, 28.f + titleWidth, 22.f, kMuted, false);

    pen.text(*pen.body, strings.text("ui.phase"), 11.f, 430.f, 8.f, kMuted, false);
    const int phase = battle.phase();
    const std::string phaseKey = "phase." + std::to_string(std::clamp(phase, 0, 3));
    pen.text(titleFont, strings.text(phaseKey), 20.f, 430.f, 24.f, kPaper, titleBold);

    const float seconds = battle.isOver() ? battle.battleSeconds() : displaySeconds(battle.tick(), frame.alpha);
    const std::string shichen = formatShichen(strings, frame.startClock, seconds);
    pen.text(*pen.body, shichen, 11.f, 560.f, 8.f, kMuted, false);
    pen.text(*pen.body, formatClock(seconds), 17.f, 560.f, 26.f, kPaper, false);

    const bool ended = battle.isOver();
    const char *runKey = "ui.pause";
    if (!ended && phase == 0) {
        runKey = "ui.start";
    } else if (!ended && !frame.running) {
        runKey = "ui.resume";
    }
    const std::string runLabel = strings.text(runKey) + " " + strings.text("ui.key.space");
    float cursor = 1584.f;
    const auto place = [&](float width) {
        cursor -= width;
        const float x = cursor;
        cursor -= 6.f;
        return x;
    };
    const float restartW = 64.f;
    const float soundW = 40.f;
    const float speedW = 52.f;
    const float runW =
        std::max(88.f, pen.measure(*pen.body, runLabel, fontPx(13.f, pen.scale), false) / pen.scale + 22.f);
    button(place(restartW), 12.f, restartW, 30.f, strings.text("ui.restart"), HudAction::Restart, true, false, false);
    button(place(soundW), 12.f, soundW, 30.f, strings.text("ui.sound"), HudAction::ToggleSound, true, !frame.muted,
           false);
    const std::vector<float> &speeds = *frame.speeds;
    const int speedCount = std::min(3, static_cast<int>(speeds.size()));
    cursor -= speedW * static_cast<float>(speedCount);
    const float speedX = cursor;
    cursor -= 6.f;
    for (int index = 0; index < speedCount; ++index) {
        const float x = speedX + speedW * static_cast<float>(index);
        const bool pressed = std::fabs(frame.speed - speeds[static_cast<std::size_t>(index)]) < 0.02f;
        const HudAction action = index == 0 ? HudAction::Speed0 : index == 1 ? HudAction::Speed1 : HudAction::Speed2;
        button(x, 12.f, speedW, 30.f, speedText(speeds[static_cast<std::size_t>(index)]), action, true, pressed, false);
    }
    button(place(runW), 12.f, runW, 30.f, runLabel, HudAction::ToggleRun, !ended, false, false);

    const float panelX = 1248.f;
    const float panelW = 336.f;
    const float innerX = panelX + 14.f;
    const float innerW = panelW - 28.f;
    float y = 55.f;
    const auto cardOpen = [&](float height) {
        pen.fill(panelX, y, panelW, height, kLacquer2);
        pen.stroke(panelX, y, panelW, height, kRule);
        y += 12.f;
    };
    const auto cardClose = [&](float top, float height) { y = top + height + 10.f; };
    const auto heading = [&](std::string_view label) {
        pen.text(*pen.body, label, 12.f, innerX, y, kGold, false);
        y += 22.f;
    };

    const std::string objective = strings.text(journal_.objectiveKey());
    const auto objectiveLines = pen.wrap(*pen.body, objective, 13.5f, innerW, false);
    const float ordersH = 12.f + 22.f + 18.f * static_cast<float>(objectiveLines.size()) + 8.f;
    const float ordersTop = y;
    cardOpen(ordersH);
    heading(strings.text("ui.orders"));
    for (const std::string &line : objectiveLines) {
        pen.text(*pen.body, line, 13.5f, innerX, y, kPaper, false);
        y += 18.f;
    }
    cardClose(ordersTop, ordersH);

    const RosterCard roster = rosterCard(strings, battle, orders);
    float rosterH = 12.f + 22.f + 8.f;
    if (roster.mode == RosterCard::Mode::Empty) {
        const auto empty = pen.wrap(*pen.body, strings.text("ui.roster_empty"), 13.f, innerW, false);
        rosterH += 18.f * static_cast<float>(std::max<std::size_t>(1, empty.size()));
    } else if (roster.mode == RosterCard::Mode::One) {
        rosterH += 42.f + 40.f;
        float rows = roster.tags.empty() ? 0.f : 1.f;
        float tagX = 0.f;
        for (const RosterTag &tag : roster.tags) {
            const float tw = pen.measure(*pen.body, tag.text, fontPx(11.5f, pen.scale), false) / pen.scale + 12.f;
            if (tagX > 0.f && tagX + tw > innerW) {
                tagX = 0.f;
                rows += 1.f;
            }
            tagX += tw + 5.f;
        }
        rosterH += 20.f * rows;
    } else {
        rosterH += 20.f + 28.f * static_cast<float>(std::min<std::size_t>(roster.rows.size(), 4));
    }
    if (roster.showOrders) {
        rosterH += 6.f + 28.f + 6.f + 28.f;
    }
    const float rosterTop = y;
    cardOpen(rosterH);
    heading(strings.text("ui.roster"));
    if (roster.mode == RosterCard::Mode::Empty) {
        const auto empty = pen.wrap(*pen.body, strings.text("ui.roster_empty"), 13.f, innerW, false);
        for (const std::string &line : empty) {
            pen.text(*pen.body, line, 13.f, innerX, y, kMuted, false);
            y += 18.f;
        }
    } else if (roster.mode == RosterCard::Mode::One) {
        const sf::Color chip = roster.general ? kGold : kCinnabar;
        const sf::Color glyphInk = roster.general ? kCinnabar : kCream;
        pen.fill(innerX, y, 34.f, 34.f, chip);
        pen.text(titleFont, roster.glyph, 22.f, innerX + 17.f, y + 4.f, glyphInk, titleBold, 0.5f);
        pen.text(*pen.body, roster.name, 15.f, innerX + 44.f, y, kPaper, false);
        pen.text(*pen.body, roster.typeLine, 12.f, innerX + 44.f, y + 18.f, kMuted, false);
        const float stateW = pen.measure(*pen.body, roster.state, fontPx(12.f, pen.scale), false) / pen.scale + 16.f;
        pen.stroke(innerX + innerW - stateW, y, stateW, 20.f, stateColor(roster.stateKind));
        pen.text(*pen.body, roster.state, 12.f, innerX + innerW - stateW * 0.5f, y + 2.f, stateColor(roster.stateKind),
                 false, 0.5f);
        y += 42.f;
        const auto meter = [&](std::string_view label, float fraction, const std::string &value, sf::Color bar) {
            pen.text(*pen.body, label, 12.f, innerX, y, kMuted, false);
            pen.fill(innerX + 40.f, y + 6.f, innerW - 118.f, 6.f, rgb(0x160e0b));
            pen.fill(innerX + 40.f, y + 6.f, (innerW - 118.f) * fraction, 6.f, bar);
            pen.text(*pen.body, value, 12.f, innerX + innerW, y, kPaper, false, 1.f);
            y += 20.f;
        };
        meter(strings.text("ui.men"), roster.menFraction, roster.menText, kPaper);
        meter(strings.text("ui.morale"), roster.moraleFraction, roster.moraleText, stateColor(roster.stateKind));
        float tagX = innerX;
        for (const RosterTag &tag : roster.tags) {
            const float tw = pen.measure(*pen.body, tag.text, fontPx(11.5f, pen.scale), false) / pen.scale + 12.f;
            if (tagX > innerX && tagX + tw > innerX + innerW) {
                tagX = innerX;
                y += 20.f;
            }
            pen.fill(tagX, y, tw, 16.f, rgb(0x1a110e));
            pen.stroke(tagX, y, tw, 16.f, kRule);
            pen.text(*pen.body, tag.text, 11.5f, tagX + 6.f, y + 1.f, tagColor(tag.ink), false);
            tagX += tw + 5.f;
        }
        if (!roster.tags.empty()) {
            y += 20.f;
        }
    } else {
        pen.text(*pen.body, roster.manyHead, 13.f, innerX, y, kMuted, false);
        y += 20.f;
        const std::size_t shown = std::min<std::size_t>(roster.rows.size(), 4);
        for (std::size_t index = 0; index < shown; ++index) {
            const RosterCard::Row &row = roster.rows[index];
            const sf::Color chip = row.general ? kGold : kCinnabar;
            pen.fill(innerX, y, 22.f, 22.f, chip);
            pen.text(titleFont, row.glyph, 14.f, innerX + 11.f, y + 3.f, row.general ? kCinnabar : kCream, titleBold,
                     0.5f);
            pen.text(*pen.body, row.name, 13.f, innerX + 30.f, y + 3.f, kPaper, false);
            pen.text(*pen.body, row.state, 12.f, innerX + innerW, y + 3.f, stateColor(row.stateKind), false, 1.f);
            y += 28.f;
        }
    }
    if (roster.showOrders) {
        y += 6.f;
        const float bw = (innerW - 16.f) / 3.f;
        const auto orderButton = [&](float x, std::string_view label, HudAction action, bool enabled) {
            button(x, y, bw, 28.f, label, action, enabled, false, false);
        };
        orderButton(innerX, strings.text("ui.hold") + " H", HudAction::Hold, true);
        orderButton(innerX + bw + 8.f, strings.text("ui.retreat") + " R", HudAction::Retreat, true);
        orderButton(innerX + (bw + 8.f) * 2.f, strings.text("ui.split") + " F", HudAction::Split, roster.canSplit);
        y += 34.f;
        orderButton(innerX, strings.text("ui.join") + " G", HudAction::Join, roster.canJoin);
        orderButton(innerX + bw + 8.f, strings.text("ui.cancel") + " Esc", HudAction::Cancel, true);
    }
    cardClose(rosterTop, rosterH);

    const PreviewCard preview = previewCard(strings, battle, orders);
    if (preview.visible) {
        const float rowH = 16.f;
        const float previewH =
            12.f + 36.f + 28.f + rowH * static_cast<float>(preview.rows.size()) + (preview.confirm ? 44.f : 12.f);
        const float previewTop = y;
        cardOpen(previewH);
        pen.text(*pen.body, preview.title, 12.f, innerX, y, kGold, false);
        pen.text(*pen.body, preview.ratio, 24.f, innerX + innerW, y - 4.f, verdictColor(preview.verdictKind), false,
                 1.f);
        y += 28.f;
        pen.text(titleFont, preview.verdict, 18.f, innerX, y, verdictColor(preview.verdictKind), titleBold);
        y += 28.f;
        for (const ModRow &row : preview.rows) {
            if (row.ink == ModRow::Ink::Note) {
                const auto notes = pen.wrap(*pen.body, row.label, 12.f, innerW, false);
                for (const std::string &note : notes) {
                    pen.text(*pen.body, note, 12.f, innerX, y, kWarn, false);
                    y += rowH;
                }
                continue;
            }
            pen.text(*pen.body, row.label, 12.5f, innerX, y, kPaper, false);
            pen.text(*pen.body, row.figure, 12.5f, innerX + innerW, y, figureColor(row.ink), false, 1.f);
            y += rowH;
        }
        if (preview.confirm) {
            y += 6.f;
            button(innerX, y, innerW, 30.f, strings.text("ui.confirm_attack"), HudAction::ConfirmAttack, true, false,
                   true);
        }
        cardClose(previewTop, previewH);
    }

    const ArmyCard armies = armyCard(strings, battle);
    const auto mingLines = pen.wrap(*pen.body, armies.ming, 12.f, innerW - 36.f, false);
    const auto jpLines = pen.wrap(*pen.body, armies.jp, 12.f, innerW - 36.f, false);
    const float armyH = 12.f + 22.f + 22.f + 16.f * static_cast<float>(std::max<std::size_t>(1, mingLines.size())) +
                        22.f + 16.f * static_cast<float>(std::max<std::size_t>(1, jpLines.size())) + 8.f;
    const float armyTop = y;
    cardOpen(armyH);
    heading(strings.text("ui.armies"));
    pen.text(titleFont, strings.text("side.ming"), 18.f, innerX, y, kMing, titleBold);
    pen.fill(innerX + 36.f, y + 8.f, innerW - 36.f, 6.f, rgb(0x160e0b));
    pen.fill(innerX + 36.f, y + 8.f, (innerW - 36.f) * armies.mingFraction, 6.f, kMing);
    y += 22.f;
    for (const std::string &line : mingLines) {
        pen.text(*pen.body, line, 12.f, innerX + 36.f, y, kMuted, false);
        y += 16.f;
    }
    pen.text(titleFont, strings.text("side.jp"), 18.f, innerX, y, kJp, titleBold);
    pen.fill(innerX + 36.f, y + 8.f, innerW - 36.f, 6.f, rgb(0x160e0b));
    pen.fill(innerX + 36.f, y + 8.f, (innerW - 36.f) * armies.jpFraction, 6.f, kJp);
    y += 22.f;
    for (const std::string &line : jpLines) {
        pen.text(*pen.body, line, 12.f, innerX + 36.f, y, kMuted, false);
        y += 16.f;
    }
    cardClose(armyTop, armyH);

    const float terrainH = 148.f;
    const float logTop = y;
    const float logBottom = 55.f + 932.f - terrainH - 10.f;
    const float logH = std::max(80.f, logBottom - logTop);
    cardOpen(logH);
    heading(strings.text("ui.log"));
    if (journal_.lines().empty()) {
        pen.text(*pen.body, strings.text("end.none"), 12.5f, innerX, y, kMuted, false);
    }
    for (const LogEntry &entry : journal_.lines()) {
        if (y + 16.f > logTop + logH - 8.f) {
            break;
        }
        pen.text(*pen.body, entry.time, 12.f, innerX, y, kMuted, false);
        sf::Color ink = kPaper;
        if (entry.tone == LogTone::Ming) {
            ink = kMing;
        } else if (entry.tone == LogTone::Jp) {
            ink = kJp;
        }
        const auto wrapped = pen.wrap(*pen.body, entry.text, 12.5f, innerW - 46.f, false);
        for (std::size_t index = 0; index < wrapped.size(); ++index) {
            if (y + 16.f > logTop + logH - 8.f) {
                break;
            }
            pen.text(*pen.body, wrapped[index], 12.5f, innerX + 46.f, y, ink, entry.important);
            y += 16.f;
        }
    }
    cardClose(logTop, logH);

    const float terrainTop = y;
    cardOpen(terrainH);
    heading(strings.text("ui.terrain"));
    const auto legend = [&](sf::Color swatch, const char *key) {
        pen.fill(innerX, y + 2.f, 18.f, 12.f, swatch);
        const auto lines = pen.wrap(*pen.body, strings.text(key), 12.f, innerW - 28.f, false);
        for (const std::string &line : lines) {
            pen.text(*pen.body, line, 12.f, innerX + 28.f, y, kMuted, false);
            y += 16.f;
        }
    };
    legend(rgb(0xb7bf98), "ui.legend.paddy");
    legend(rgb(0x6f9a86), "ui.legend.hill");
    legend(rgb(0xcfb48a), "ui.legend.road");
    legend(rgb(0xbdb09a), "ui.legend.rout");
    legend(kCinnabar, "ui.legend.frag");
    cardClose(terrainTop, terrainH);

    const bool intro = !frame.running && battle.phase() == 0 && !ended;
    if (intro || ended) {
        pen.fill(17.f, 55.f, 1216.f, 932.f, rgb(0x100a08, 150));
        const float cardW = 560.f;
        const float cardX = 17.f + (1216.f - cardW) * 0.5f;
        const float cardY = 70.f;
        const float textX = cardX + 24.f;
        const float textW = cardW - 48.f;
        if (intro) {
            const float cardH = 900.f;
            pen.fill(cardX, cardY, cardW, cardH, kSilk);
            pen.stroke(cardX, cardY, cardW, cardH, rgb(0xb9a57c));
            float textY = cardY + 18.f;
            pen.text(*pen.body, strings.text("ui.intro_eyebrow"), 12.f, textX, textY, rgb(0x7a6346), false);
            textY += 18.f;
            pen.text(titleFont, strings.text("scn.byeokjegwan"), 40.f, textX, textY, kCinnabar, titleBold);
            textY += 52.f;
            for (const std::string &line : pen.wrap(*pen.body, strings.text("ui.intro_body"), 14.f, textW, false)) {
                pen.text(*pen.body, line, 14.f, textX, textY, kInk, false);
                textY += 20.f;
            }
            pen.text(*pen.body, strings.text("ui.tips_title"), 12.f, textX, textY + 6.f, rgb(0x7a6346), false);
            textY += 26.f;
            for (int tip = 1; tip <= 7; ++tip) {
                const std::string body = strings.text("ui.tip." + std::to_string(tip));
                const auto lines = pen.wrap(*pen.body, body, 13.f, textW - 16.f, false);
                for (std::size_t index = 0; index < lines.size(); ++index) {
                    const std::string shown = index == 0 ? std::string("\u00b7 ") + lines[index] : lines[index];
                    pen.text(*pen.body, shown, 13.f, textX + (index == 0 ? 0.f : 12.f), textY, kInk, false);
                    textY += 18.f;
                }
            }
            pen.text(*pen.body, strings.text("ui.help_title"), 12.f, textX, textY + 4.f, rgb(0x7a6346), false);
            textY += 22.f;
            const char *helpKeys[] = {"ui.help.pick", "ui.help.ground", "ui.help.hover", "ui.help.keys",
                                      "ui.help.touch"};
            const char *helpBody[] = {"ui.help.pick_body", "ui.help.ground_body", "ui.help.hover_body",
                                      "ui.help.keys_body", "ui.help.touch_body"};
            for (int index = 0; index < 5; ++index) {
                pen.text(*pen.body, strings.text(helpKeys[index]), 13.f, textX, textY, kInk, false);
                const float labelW =
                    pen.measure(*pen.body, strings.text(helpKeys[index]), fontPx(13.f, pen.scale), false) / pen.scale;
                const auto lines =
                    pen.wrap(*pen.body, strings.text(helpBody[index]), 13.f, textW - labelW - 16.f, false);
                for (std::size_t line = 0; line < lines.size(); ++line) {
                    pen.text(*pen.body, lines[line], 13.f, textX + labelW + 12.f, textY, kInk, false);
                    textY += 18.f;
                }
            }
            const float goW = 120.f;
            button(cardX + (cardW - goW) * 0.5f, textY + 8.f, goW, 36.f, strings.text("ui.start"), HudAction::ToggleRun,
                   true, false, true);
            textY += 52.f;
            for (const std::string &line : pen.wrap(*pen.body, strings.text("ui.intro_fine"), 12.f, textW, false)) {
                pen.text(*pen.body, line, 12.f, textX, textY, rgb(0x7a6346), false);
                textY += 16.f;
            }
        } else if (frame.exit != nullptr) {
            const std::vector<std::string> followers = battle.result().end == BattleEnd::Withdrawn
                                                           ? withdrawalCompany(battle, strings, *frame.exit)
                                                           : std::vector<std::string>{};
            const EndCard card = formatEnd(strings, battle.result(), battle.stats(), battle.enlisted(Side::Ming),
                                           battle.battleSeconds(), followers, journal_.important());
            const auto bodyLines = pen.wrap(*pen.body, card.body, 15.f, textW, false);
            const auto historyLines = pen.wrap(*pen.body, strings.text("end.history"), 12.f, textW, false);
            const std::size_t eventLines = std::max<std::size_t>(card.events.size(), 1);
            float cardH = 18.f + 20.f + 78.f + 22.f * static_cast<float>(bodyLines.size()) + 6.f + 64.f + 20.f +
                          18.f * static_cast<float>(eventLines) + 8.f + 16.f * static_cast<float>(historyLines.size()) +
                          8.f + 36.f + 16.f;
            pen.fill(cardX, cardY, cardW, cardH, kSilk);
            pen.stroke(cardX, cardY, cardW, cardH, rgb(0xb9a57c));
            float textY = cardY + 18.f;
            pen.text(*pen.body, card.eyebrow, 12.f, textX, textY, rgb(0x7a6346), false);
            textY += 20.f;
            pen.text(titleFont, card.grade, 64.f, textX, textY, card.victory ? kCinnabar : rgb(0x4a3d32), titleBold);
            textY += 78.f;
            for (const std::string &line : bodyLines) {
                pen.text(*pen.body, line, 15.f, textX, textY, kInk, false);
                textY += 22.f;
            }
            textY += 6.f;
            const float statW = textW / 5.f;
            for (std::size_t index = 0; index < card.stats.size(); ++index) {
                const float x = textX + statW * static_cast<float>(index);
                pen.fill(x, textY, statW - 8.f, 1.f, rgb(0xc5b18a));
                pen.text(*pen.body, card.stats[index].label, 11.5f, x, textY + 6.f, rgb(0x7a6346), false);
                pen.text(*pen.body, card.stats[index].value, 16.f, x, textY + 24.f, kInk, false);
            }
            textY += 64.f;
            pen.text(*pen.body, strings.text("end.events"), 12.f, textX, textY, rgb(0x7a6346), false);
            textY += 20.f;
            if (card.events.empty()) {
                pen.text(*pen.body, strings.text("end.none"), 13.f, textX, textY, kInk, false);
                textY += 18.f;
            }
            for (const std::string &line : card.events) {
                pen.text(*pen.body, line, 13.f, textX, textY, kInk, false);
                textY += 18.f;
            }
            textY += 8.f;
            for (const std::string &line : historyLines) {
                pen.text(*pen.body, line, 12.f, textX, textY, rgb(0x7a6346), false);
                textY += 16.f;
            }
            const float goW = 120.f;
            button(cardX + (cardW - goW) * 0.5f, textY + 8.f, goW, 36.f, strings.text("end.again"), HudAction::Restart,
                   true, false, true);
        }
    }

    if (battle.phase() > 0 && !frame.running && !ended) {
        const std::string paused = strings.text("ui.paused");
        const float width = pen.measure(*pen.body, paused, fontPx(12.f, pen.scale), false) / pen.scale + 20.f;
        const float x = 17.f + 1216.f - width - 12.f;
        pen.fill(x, 66.f, width, 22.f, rgb(0x140d0a, 180));
        pen.stroke(x, 66.f, width, 22.f, rgb(0x8e7443));
        pen.text(*pen.body, paused, 12.f, x + width * 0.5f, 69.f, kGold, false, 0.5f);
    }

    if (journal_.banner().serial != shownBanner_) {
        shownBanner_ = journal_.banner().serial;
        bannerStart_ = frame.wallTime;
    }
    const float age = frame.wallTime - bannerStart_;
    const float alpha = journal_.banner().serial == 0 ? 0.f : bannerAlpha(age, frame.reducedMotion);
    if (alpha > 0.01f) {
        const unsigned fade = static_cast<unsigned>(std::clamp(alpha, 0.f, 1.f) * 255.f);
        const float center = 17.f + 608.f;
        pen.text(titleFont, journal_.banner().title, 48.f, center, 55.f + 932.f * 0.11f, rgb(0xcfaa5c, fade), titleBold,
                 0.5f);
        const std::string sub = journal_.banner().subtitle;
        const float subW = pen.measure(*pen.body, sub, fontPx(14.f, pen.scale), false) / pen.scale + 24.f;
        const float subY = 55.f + 932.f * 0.11f + 58.f;
        pen.fill(center - subW * 0.5f, subY, subW, 24.f, rgb(0x140d0a, static_cast<unsigned>(fade * 0.66f)));
        pen.text(*pen.body, sub, 14.f, center, subY + 3.f, rgb(0xeee3cc, fade), false, 0.5f);
    }
}

} // namespace tq
