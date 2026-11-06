#include "app/SoldierView.h"

#include "app/TextCache.h"

#include <SFML/Graphics/RenderStates.hpp>

#include <algorithm>
#include <string>
#include <string_view>

namespace tq {
namespace {

constexpr float kDustMeters = 11.f;

void appendSprite(sf::VertexArray &tris, sf::Vector2f topLeft, sf::Vector2f size, sf::IntRect cell,
                  sf::Color color) {
    const sf::Vector2f t0{static_cast<float>(cell.position.x), static_cast<float>(cell.position.y)};
    const sf::Vector2f t1{t0.x + static_cast<float>(cell.size.x), t0.y + static_cast<float>(cell.size.y)};
    const sf::Vector2f p1{topLeft.x + size.x, topLeft.y + size.y};
    const sf::Vertex a{topLeft, color, t0};
    const sf::Vertex b{{p1.x, topLeft.y}, color, {t1.x, t0.y}};
    const sf::Vertex c{p1, color, t1};
    const sf::Vertex d{{topLeft.x, p1.y}, color, {t0.x, t1.y}};
    tris.append(a);
    tris.append(b);
    tris.append(c);
    tris.append(a);
    tris.append(c);
    tris.append(d);
}

void appendQuad(sf::VertexArray &tris, const sf::Vector2f (&corner)[4], sf::IntRect cell, sf::Color color) {
    const sf::Vector2f t0{static_cast<float>(cell.position.x) + 0.5f, static_cast<float>(cell.position.y) + 0.5f};
    const sf::Vector2f t1{static_cast<float>(cell.position.x + cell.size.x) - 0.5f,
                          static_cast<float>(cell.position.y + cell.size.y) - 0.5f};
    const sf::Vertex a{corner[0], color, t0};
    const sf::Vertex b{corner[1], color, {t1.x, t0.y}};
    const sf::Vertex c{corner[2], color, t1};
    const sf::Vertex d{corner[3], color, {t0.x, t1.y}};
    tris.append(a);
    tris.append(b);
    tris.append(c);
    tris.append(a);
    tris.append(c);
    tris.append(d);
}

bool hasKey(const StringTable &strings, const std::string &key) {
    for (const StringTable *table = &strings; table != nullptr; table = table->fallback) {
        if (table->entries.find(key) != table->entries.end()) {
            return true;
        }
    }
    return false;
}

std::string flagText(const StringTable &strings, const Unit &unit, const UnitType &type) {
    constexpr std::string_view kUnitPrefix = "unit.";
    if (unit.labelKey.rfind(kUnitPrefix, 0) == 0) {
        const std::string key = "glyph.flag." + unit.labelKey.substr(kUnitPrefix.size());
        if (hasKey(strings, key)) {
            return strings.text(key);
        }
    }
    const std::string key = "glyph.flag." + type.id;
    return hasKey(strings, key) ? strings.text(key) : std::string{};
}

} // namespace

void SoldierView::draw(sf::RenderTarget &target, const Battle &battle, const FormationLayer &formation,
                       const SoldierAtlas &atlas, float alpha, float animTime, const StringTable &strings,
                       const sf::Font &brush, float opacity) {
    const auto fadeTo = [opacity](float alpha) {
        return static_cast<std::uint8_t>(std::clamp(alpha * opacity, 0.f, 1.f) * 255.f + 0.5f);
    };
    soldiers_.clear();
    formation.layout(battle, alpha, animTime, soldiers_);
    std::stable_sort(soldiers_.begin(), soldiers_.end(),
                     [](const Soldier &left, const Soldier &right) { return left.pos.y < right.pos.y; });
    tris_.clear();
    // Bodies lie under everything: the standing figure squashed flat and darkened.
    const float cellScale = kAtlasCellMeters / static_cast<float>(kAtlasCellPx);
    for (const FallenSoldier &body : formation.bodies()) {
        const float lying = 0.38f;
        const sf::IntRect cell = SoldierAtlas::cell(atlasRow(body.side, body.kind, soldierDirection(body.facing)),
                                                    kFrameIdle);
        const auto shade = static_cast<std::uint8_t>(150);
        appendSprite(tris_,
                     {body.pos.x - kAtlasAnchorX * cellScale, body.pos.y - kAtlasAnchorY * cellScale * lying},
                     {kAtlasCellMeters, kAtlasCellMeters * lying}, cell,
                     sf::Color{shade, shade, shade, fadeTo(formation.bodyAlpha(body))});
    }
    const sf::IntRect dust = SoldierAtlas::cell(kAtlasExtraRow, kAtlasDustColumn);
    for (const Soldier &soldier : soldiers_) {
        if (soldier.dust > 0.f) {
            const std::uint8_t a = fadeTo(soldier.dust);
            appendSprite(tris_, {soldier.dustPos.x - kDustMeters * 0.5f, soldier.dustPos.y - kDustMeters * 0.5f},
                         {kDustMeters, kDustMeters}, dust, sf::Color{255, 255, 255, a});
        }
    }
    const float scale = kAtlasCellMeters / static_cast<float>(kAtlasCellPx);
    const sf::Vector2f size{kAtlasCellMeters, kAtlasCellMeters};
    for (const Soldier &soldier : soldiers_) {
        const auto shade = static_cast<std::uint8_t>(std::clamp(soldier.shade, 0.f, 1.f) * 255.f);
        const sf::IntRect cell = SoldierAtlas::cell(atlasRow(soldier.side, soldier.kind, soldier.direction),
                                                    soldier.frame);
        appendSprite(tris_, {soldier.pos.x - kAtlasAnchorX * scale, soldier.pos.y - kAtlasAnchorY * scale}, size, cell,
                     sf::Color{shade, shade, shade, fadeTo(1.f)});
    }
    sf::RenderStates states;
    states.texture = &atlas.texture();
    target.draw(tris_, states);
    drawBanners(target, battle, atlas, alpha, animTime, strings, brush, opacity);
}

void SoldierView::drawBanners(sf::RenderTarget &target, const Battle &battle, const SoldierAtlas &atlas, float alpha,
                              float animTime, const StringTable &strings, const sf::Font &brush, float opacity) {
    const auto alphaOf = [opacity](sf::Color color) {
        color.a = static_cast<std::uint8_t>(std::clamp(opacity, 0.f, 1.f) * static_cast<float>(color.a) + 0.5f);
        return color;
    };
    struct Placed {
        Banner banner;
        UnitId unit = kNoUnit;
    };
    std::vector<Placed> placed;
    for (const Unit &unit : battle.units()) {
        const Banner banner = unitBanner(battle, unit, alpha, animTime);
        if (banner.kind != BannerKind::None) {
            placed.push_back({banner, unit.id});
        }
    }
    std::stable_sort(placed.begin(), placed.end(),
                     [](const Placed &left, const Placed &right) { return left.banner.base.y < right.banner.base.y; });

    sf::VertexArray poles(sf::PrimitiveType::Triangles);
    tris_.clear();
    const sf::Color wood = alphaOf({0x4a, 0x36, 0x24});
    for (const Placed &item : placed) {
        const Banner &banner = item.banner;
        const float x = banner.base.x;
        const float y = banner.base.y;
        const float top = y - kBannerPole;
        poles.append({{x - 0.25f, top}, wood});
        poles.append({{x + 0.25f, top}, wood});
        poles.append({{x + 0.25f, y}, wood});
        poles.append({{x - 0.25f, top}, wood});
        poles.append({{x + 0.25f, y}, wood});
        poles.append({{x - 0.25f, y}, wood});
        if (banner.kind == BannerKind::Ming) {
            // A square flag from the top of the pole; the free edge waves.
            const float w = kMingBannerWidth;
            const float h = kMingBannerHeight;
            const sf::Vector2f cloth[4] = {{x + 0.25f, top + 0.4f},
                                           {x + 0.25f + w, top + 0.4f + banner.sway},
                                           {x + 0.25f + w, top + 0.4f + h + banner.sway},
                                           {x + 0.25f, top + 0.4f + h}};
            appendQuad(tris_, cloth, SoldierAtlas::cell(kAtlasExtraRow, kAtlasMingBannerColumn),
                       alphaOf(sf::Color::White));
        } else {
            // A tall sashimono hanging from a crossbar; the bottom swings.
            const float w = kJpBannerWidth;
            const float h = kJpBannerHeight;
            const int column =
                banner.kind == BannerKind::JpVanguard ? kAtlasJpVanguardBannerColumn : kAtlasJpMainBannerColumn;
            const sf::Vector2f cloth[4] = {{x + 0.25f, top + 0.3f},
                                           {x + 0.25f + w, top + 0.3f},
                                           {x + 0.25f + w + banner.sway * 0.6f, top + 0.3f + h},
                                           {x + 0.25f + banner.sway * 0.6f, top + 0.3f + h}};
            appendQuad(tris_, cloth, SoldierAtlas::cell(kAtlasExtraRow, column), alphaOf(sf::Color::White));
        }
    }
    target.draw(poles);
    sf::RenderStates states;
    states.texture = &atlas.texture();
    target.draw(tris_, states);

    // The character on each Ming banner in the brush font, shaped once at a fixed pixel size and scaled to the map.
    constexpr float kGlyphMeters = 5.5f;
    constexpr unsigned kGlyphPixels = 48;
    const float scale = kGlyphMeters / static_cast<float>(kGlyphPixels);
    for (const Placed &item : placed) {
        if (item.banner.kind != BannerKind::Ming) {
            continue;
        }
        const Unit &unit = battle.unit(item.unit);
        const std::string text = flagText(strings, unit, battle.typeOf(unit));
        if (text.empty()) {
            continue;
        }
        const sf::FloatRect bounds = textBounds(brush, kGlyphPixels, false, text);
        const sf::Vector2f center{item.banner.base.x + 0.25f + kMingBannerWidth * 0.5f,
                                  item.banner.base.y - kBannerPole + 0.4f + kMingBannerHeight * 0.5f +
                                      item.banner.sway * 0.5f};
        drawText(target, brush, kGlyphPixels, false, text, alphaOf({0xf4, 0xdb, 0xa4}), center,
                 {bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f},
                 {scale, scale});
    }
}

} // namespace tq
