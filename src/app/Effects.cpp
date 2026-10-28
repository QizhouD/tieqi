#include "app/Effects.h"

#include "app/TextCache.h"
#include "core/sim/Event.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include <algorithm>
#include <cmath>

namespace tq {
namespace {

sf::Color ink() { return {0x2a, 0x22, 0x1b}; }

sf::Color cinnabar() { return {0xa8, 0x38, 0x2a}; }

sf::Color indigo() { return {0x2f, 0x3a, 0x56}; }

sf::Color gold() { return {0xd9, 0xb4, 0x66}; }

sf::Color sideInk(Side side) { return side == Side::Ming ? cinnabar() : indigo(); }

sf::Color withAlpha(sf::Color color, float alpha) {
    color.a = static_cast<std::uint8_t>(std::clamp(alpha, 0.f, 1.f) * 255.f + 0.5f);
    return color;
}

const Unit *findUnit(const Battle &battle, UnitId id) {
    if (id == kNoUnit || id >= battle.units().size()) {
        return nullptr;
    }
    return &battle.unit(id);
}

sf::Texture makeBlob() {
    constexpr unsigned n = 64;
    sf::Image image({n, n}, sf::Color::Transparent);
    const float center = (static_cast<float>(n) - 1.f) * 0.5f;
    for (unsigned y = 0; y < n; ++y) {
        for (unsigned x = 0; x < n; ++x) {
            const float dist = std::hypot(static_cast<float>(x) - center, static_cast<float>(y) - center) / center;
            if (dist >= 1.f) {
                continue;
            }
            const auto alpha = static_cast<std::uint8_t>((1.f - dist) * 255.f + 0.5f);
            image.setPixel({x, y}, sf::Color(255, 255, 255, alpha));
        }
    }
    sf::Texture texture(image);
    texture.setSmooth(true);
    return texture;
}

void drawBlob(sf::RenderTarget &target, const sf::Texture &blob, sf::Vector2f center, float radius, sf::Color color) {
    if (!(radius > 0.f)) {
        return;
    }
    sf::Sprite sprite(blob);
    const float tex = static_cast<float>(blob.getSize().x);
    const float scale = radius * 2.f / tex;
    sprite.setOrigin({tex * 0.5f, tex * 0.5f});
    sprite.setScale({scale, scale});
    sprite.setPosition(center);
    sprite.setColor(color);
    target.draw(sprite);
}

} // namespace

void Effects::clear() {
    items_.clear();
    hitStop_ = 0.f;
    shake_ = 0.f;
}

float Effects::next() {
    std::uniform_real_distribution<float> unit(0.f, 1.f);
    return unit(rng_);
}

void Effects::add(Item item) {
    const bool heavy =
        item.kind == Kind::Dust || item.kind == Kind::Smoke || item.kind == Kind::Flash || item.kind == Kind::Chip;
    if (heavy && items_.size() >= kMaxItems) {
        return;
    }
    items_.push_back(std::move(item));
}

void Effects::floater(Vec2 pos, const std::string &text, sf::Color color, float pixels) {
    if (text.empty()) {
        return;
    }
    Item item;
    item.kind = Kind::Text;
    item.x = pos.x;
    item.y = pos.y;
    item.span = 1.5f;
    item.pixels = pixels;
    item.color = color;
    item.text = text;
    add(item);
}

void Effects::marker(Vec2 pos, bool attack) {
    Item item;
    item.kind = Kind::Mark;
    item.x = pos.x;
    item.y = pos.y;
    item.span = 0.6f;
    item.wallClock = true;
    item.color = attack ? withAlpha(cinnabar(), 0.9f) : withAlpha(ink(), 0.8f);
    add(item);
}

void Effects::onEvents(const Battle &battle, const StringTable &strings, std::span<const Event> events,
                       bool reducedMotion, std::optional<Vec2> assaultAt) {
    for (const Event &event : events) {
        switch (event.type) {
        case Event::Type::Impact: {
            const float ratio = event.value;
            shake_ = impactShakePx(ratio, reducedMotion);
            hitStop_ = impactHitStopSec(ratio, reducedMotion);
            const int count = impactDustCount(ratio);
            for (int i = 0; i < count; ++i) {
                Item dust;
                dust.kind = Kind::Dust;
                dust.x = event.pos.x + (next() - 0.5f) * 24.f;
                dust.y = event.pos.y + (next() - 0.5f) * 16.f;
                dust.radius = 4.f + next() * 6.f;
                dust.grow = 16.f + next() * 22.f;
                dust.vx = (next() - 0.5f) * 22.f;
                dust.vy = (next() - 0.5f) * 22.f;
                dust.span = 0.9f + next() * 0.6f;
                add(dust);
            }
            const bool pike = (event.flags & kPikeFront) != 0;
            const bool breaks = (event.flags & kBreaks) != 0;
            std::string label = strings.text("fx.blocked");
            sf::Color color = ink();
            float pixels = 24.f;
            if (pike) {
                label = strings.text("fx.pike");
                pixels = 26.f;
            } else if (breaks) {
                label = strings.text("fx.break");
                color = cinnabar();
                pixels = 44.f;
            } else if (ratio >= 1.2f) {
                label = strings.text("fx.charge");
                color = cinnabar();
                pixels = 28.f;
            }
            floater({event.pos.x, event.pos.y - 12.f}, label, color, pixels);
            break;
        }
        case Event::Type::ThreeEyedVolley:
        case Event::Type::TeppoVolley: {
            const Unit *shooter = findUnit(battle, event.a);
            if (shooter == nullptr) {
                break;
            }
            const bool three = event.type == Event::Type::ThreeEyedVolley;
            const float facing = shooter->facing;
            const float along = std::cos(facing);
            const float across = std::sin(facing);
            const int span = three ? 2 : 3;
            const float spread = three ? shooter->frontage / 5.f : shooter->frontage / 7.f;
            for (int i = -span; i <= span; ++i) {
                const float x =
                    shooter->pos.x + along * (shooter->depth * 0.5f + 4.f) - across * static_cast<float>(i) * spread;
                const float y =
                    shooter->pos.y + across * (shooter->depth * 0.5f + 4.f) + along * static_cast<float>(i) * spread;
                Item smoke;
                smoke.kind = Kind::Smoke;
                smoke.x = x;
                smoke.y = y;
                if (three) {
                    smoke.radius = 2.f + next() * 2.f;
                    smoke.grow = 7.f + next() * 5.f;
                    smoke.vx = along * 14.f;
                    smoke.vy = across * 14.f;
                    smoke.span = 1.4f + next() * 0.6f;
                } else {
                    smoke.radius = 3.f + next() * 3.f;
                    smoke.grow = 8.f + next() * 6.f;
                    smoke.vx = along * 10.f + (next() - 0.5f) * 6.f;
                    smoke.vy = across * 10.f + (next() - 0.5f) * 6.f;
                    smoke.span = 2.f + next();
                }
                add(smoke);
                Item flash;
                flash.kind = Kind::Flash;
                flash.x = x;
                flash.y = y;
                flash.span = 0.12f;
                add(flash);
            }
            if (three) {
                floater({shooter->pos.x + along * 30.f, shooter->pos.y + across * 30.f - 10.f},
                        strings.text("fx.volley"), ink(), 15.f);
            }
            break;
        }
        case Event::Type::Broken: {
            const Unit *unit = findUnit(battle, event.a);
            Item ring;
            ring.kind = Kind::Ring;
            ring.x = event.pos.x;
            ring.y = event.pos.y;
            ring.span = 1.1f;
            ring.color = unit != nullptr ? sideInk(unit->side) : cinnabar();
            add(ring);
            break;
        }
        case Event::Type::Shattered: {
            const Unit *unit = findUnit(battle, event.a);
            const float base = unit != nullptr ? unit->facing : 0.f;
            const sf::Color body = unit != nullptr ? sideInk(unit->side) : cinnabar();
            for (int i = 0; i < 14; ++i) {
                const float angle = base + (next() - 0.5f) * 2.6f;
                const float speed = 60.f + next() * 110.f;
                Item chip;
                chip.kind = Kind::Chip;
                chip.x = event.pos.x;
                chip.y = event.pos.y;
                chip.vx = std::cos(angle) * speed;
                chip.vy = std::sin(angle) * speed;
                chip.angle = next() * 6.f;
                chip.spin = (next() - 0.5f) * 16.f;
                chip.size = 3.f + next() * 4.f;
                chip.span = 0.8f + next() * 0.5f;
                chip.color = next() < 0.7f ? body : gold();
                add(chip);
            }
            break;
        }
        case Event::Type::MomentumGained:
            floater({event.pos.x, event.pos.y - 26.f}, strings.text("fx.momentum"), indigo(), 18.f);
            break;
        case Event::Type::Rallied: {
            const Unit *unit = findUnit(battle, event.a);
            floater({event.pos.x, event.pos.y - 20.f}, strings.text("fx.rally"),
                    unit != nullptr ? sideInk(unit->side) : cinnabar(), 20.f);
            break;
        }
        case Event::Type::Merged: {
            const Unit *unit = findUnit(battle, event.a);
            floater({event.pos.x, event.pos.y - 22.f}, strings.text("fx.merge"),
                    unit != nullptr ? sideInk(unit->side) : cinnabar(), 22.f);
            break;
        }
        case Event::Type::Split: {
            Vec2 pos = event.pos;
            const Unit *left = findUnit(battle, event.a);
            const Unit *right = findUnit(battle, event.b);
            if (left != nullptr && right != nullptr) {
                pos = (left->pos + right->pos) * 0.5f;
            }
            floater({pos.x, pos.y - 22.f}, strings.text("fx.split"), left != nullptr ? sideInk(left->side) : cinnabar(),
                    20.f);
            break;
        }
        case Event::Type::Dispersed: {
            const Unit *unit = findUnit(battle, event.a);
            const float pixels = unit != nullptr && unit->isFragment ? 16.f : 22.f;
            floater({event.pos.x, event.pos.y - 16.f}, strings.text("fx.disperse"), ink(), pixels);
            break;
        }
        case Event::Type::Withdrawn:
            floater({event.pos.x, event.pos.y + 30.f}, strings.text("fx.withdraw"), cinnabar(), 20.f);
            break;
        case Event::Type::GeneralDown: {
            floater({event.pos.x, event.pos.y - 20.f}, strings.text("fx.general_down"), cinnabar(), 30.f);
            Item ring;
            ring.kind = Kind::Ring;
            ring.x = event.pos.x;
            ring.y = event.pos.y;
            ring.span = 1.1f;
            ring.color = cinnabar();
            add(ring);
            break;
        }
        case Event::Type::GrandAssault:
            if ((event.flags & kAssaultFailed) == 0 && assaultAt) {
                floater({assaultAt->x, assaultAt->y - 28.f}, strings.text("fx.assault"), indigo(), 32.f);
            }
            break;
        case Event::Type::Breakaway:
        case Event::Type::Fled:
        case Event::Type::ScoutWarning:
        case Event::Type::PhaseChanged:
        case Event::Type::Reinforcement:
        case Event::Type::BattleEnded:
            break;
        }
    }
}

void Effects::advanceHitStop(float wallDt) { hitStop_ = std::max(0.f, hitStop_ - std::max(0.f, wallDt)); }

void Effects::update(float fxDt, float wallDt) {
    shake_ = std::max(0.f, shake_ - std::max(0.f, wallDt) * 30.f);
    for (Item &item : items_) {
        const float dt = item.wallClock ? wallDt : fxDt;
        if (dt > 0.f) {
            item.life += dt;
            if (item.vx != 0.f || item.vy != 0.f) {
                item.x += item.vx * dt;
                item.y += item.vy * dt;
                const float keep = std::pow(0.4f, dt);
                item.vx *= keep;
                item.vy *= keep;
            }
        }
    }
    std::erase_if(items_, [](const Item &item) { return item.life >= item.span; });
}

sf::Vector2f Effects::shakePixels() {
    if (!(shake_ > 0.f)) {
        return {};
    }
    return {(next() - 0.5f) * shake_, (next() - 0.5f) * shake_};
}

void Effects::ensureBlob() const {
    if (blobReady_) {
        return;
    }
    blob_ = makeBlob();
    blobReady_ = true;
}

void Effects::drawDust(sf::RenderTarget &target) const {
    ensureBlob();
    for (const Item &item : items_) {
        if (item.kind != Kind::Dust || !(item.span > 0.f)) {
            continue;
        }
        const float age = item.life / item.span;
        const float radius = item.radius + item.grow * item.life;
        drawBlob(target, blob_, {item.x, item.y}, radius, withAlpha({120, 86, 50}, (1.f - age) * 0.42f));
    }
}

void Effects::drawAir(sf::RenderTarget &target, const sf::Font &font, bool bold, float pixelsPerMeter) const {
    ensureBlob();
    const float scale = std::max(pixelsPerMeter, 1e-3f);
    for (const Item &item : items_) {
        if (!(item.span > 0.f)) {
            continue;
        }
        const float age = std::clamp(item.life / item.span, 0.f, 1.f);
        switch (item.kind) {
        case Kind::Smoke: {
            const float radius = item.radius + item.grow * item.life;
            drawBlob(target, blob_, {item.x, item.y}, radius, withAlpha({240, 234, 222}, (1.f - age) * 0.62f));
            sf::CircleShape ring(radius);
            ring.setOrigin({radius, radius});
            ring.setPosition({item.x, item.y});
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineThickness(0.8f / scale);
            ring.setOutlineColor(withAlpha({70, 58, 46}, (1.f - age) * 0.22f));
            ring.setPointCount(24);
            target.draw(ring);
            break;
        }
        case Kind::Flash:
            drawBlob(target, blob_, {item.x, item.y}, 4.f, withAlpha({255, 206, 120}, 1.f - age));
            break;
        case Kind::Chip: {
            sf::RectangleShape chip({item.size, item.size * 0.7f});
            chip.setOrigin({item.size * 0.5f, item.size * 0.35f});
            chip.setPosition({item.x, item.y});
            chip.setRotation(sf::radians(item.angle + item.spin * item.life));
            chip.setFillColor(withAlpha(item.color, 1.f - age));
            target.draw(chip);
            break;
        }
        case Kind::Ring: {
            const float radius = 20.f + 150.f * age;
            sf::CircleShape ring(radius);
            ring.setOrigin({radius, radius});
            ring.setPosition({item.x, item.y});
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineThickness(3.f / scale);
            ring.setOutlineColor(withAlpha(item.color, (1.f - age) * 0.6f));
            ring.setPointCount(48);
            target.draw(ring);
            break;
        }
        case Kind::Mark: {
            const float radius = (14.f - 10.f * age) / scale * 1.4f;
            if (!(radius > 0.f)) {
                break;
            }
            sf::CircleShape ring(radius);
            ring.setOrigin({radius, radius});
            ring.setPosition({item.x, item.y});
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineThickness(2.f / scale);
            const float base = static_cast<float>(item.color.a) / 255.f;
            ring.setOutlineColor(withAlpha(item.color, base * (1.f - age)));
            ring.setPointCount(28);
            target.draw(ring);
            break;
        }
        case Kind::Text: {
            const float fade = 1.f - std::max(0.f, (item.life - 0.95f) / 0.55f);
            if (!(fade > 0.f)) {
                break;
            }
            const float pop = 1.f + 0.35f * std::max(0.f, 1.f - item.life * 6.f);
            const auto size = static_cast<unsigned>(std::max(1, static_cast<int>(std::lround(item.pixels / scale))));
            const sf::FloatRect bounds = textBounds(font, size, bold, item.text);
            const sf::Vector2f origin{bounds.position.x + bounds.size.x * 0.5f,
                                      bounds.position.y + bounds.size.y * 0.5f};
            const float y = item.y - 18.f * item.life;
            const float thickness = std::max(2.f, item.pixels * 0.14f) / scale;
            drawOutlinedText(target, font, size, bold, item.text, withAlpha(item.color, fade), {item.x, y}, origin,
                             {pop, pop}, withAlpha({239, 229, 203}, fade * 0.9f), thickness);
            break;
        }
        case Kind::Dust:
            break;
        }
    }
}

} // namespace tq
