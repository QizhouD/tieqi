#include "app/SoldierView.h"

#include "app/Crests.h"

#include <SFML/Graphics/RenderStates.hpp>

#include <algorithm>
#include <cmath>

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

// A small filled polygon (a fan) for the tassel under a Ming spear tip.
void appendDisc(sf::VertexArray &tris, sf::Vector2f centre, sf::Vector2f radius, sf::Color color) {
    constexpr int kSteps = 10;
    constexpr float kTau = 6.2831853f;
    for (int i = 0; i < kSteps; ++i) {
        const float a0 = kTau * static_cast<float>(i) / kSteps;
        const float a1 = kTau * static_cast<float>(i + 1) / kSteps;
        tris.append({centre, color});
        tris.append({{centre.x + std::cos(a0) * radius.x, centre.y + std::sin(a0) * radius.y}, color});
        tris.append({{centre.x + std::cos(a1) * radius.x, centre.y + std::sin(a1) * radius.y}, color});
    }
}

} // namespace

void SoldierView::draw(sf::RenderTarget &target, const Battle &battle, const FormationLayer &formation,
                       const SoldierAtlas &atlas, const MingFlags &flags, float alpha, float animTime,
                       float opacity) {
    const auto fadeTo = [opacity](float alpha) {
        return static_cast<std::uint8_t>(std::clamp(alpha * opacity, 0.f, 1.f) * 255.f + 0.5f);
    };
    soldiers_.clear();
    formation.layout(battle, alpha, animTime, soldiers_);
    std::stable_sort(soldiers_.begin(), soldiers_.end(),
                     [](const Soldier &left, const Soldier &right) { return left.pos.y < right.pos.y; });
    tris_.clear();
    // Bodies lie under everything, in the atlas's fallen frame, slightly darkened.
    const float cellScale = kAtlasCellMeters / static_cast<float>(kAtlasCellPx);
    for (const FallenSoldier &body : formation.bodies()) {
        const sf::IntRect cell = SoldierAtlas::cell(atlasRow(body.side, body.kind, soldierDirection(body.facing)),
                                                    kFrameFallen);
        const auto shade = static_cast<std::uint8_t>(190);
        const float half = static_cast<float>(kAtlasCellPx) * 0.5f * cellScale;
        appendSprite(tris_, {body.pos.x - half, body.pos.y - half},
                     {kAtlasCellMeters, kAtlasCellMeters}, cell,
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
    drawBanners(target, battle, atlas, flags, alpha, animTime, opacity);
}

void SoldierView::drawBanners(sf::RenderTarget &target, const Battle &battle, const SoldierAtlas &atlas,
                              const MingFlags &flags, float alpha, float animTime, float opacity) {
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
    sf::VertexArray cloths(sf::PrimitiveType::Triangles);
    tris_.clear();
    const sf::Color steel = alphaOf({0xcf, 0xc8, 0xb8});
    const sf::Color tassel = alphaOf({0xc8, 0x32, 0x1e});
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
        if (banner.kind == BannerKind::Ming || banner.kind == BannerKind::MingStandard) {
            // Spear tip and red tassel at the top of the pole, then the cloth from the pole; its free edge waves.
            poles.append({{x, top - 2.2f}, steel});
            poles.append({{x - 0.45f, top - 0.2f}, steel});
            poles.append({{x + 0.45f, top - 0.2f}, steel});
            appendDisc(poles, {x, top + 0.15f}, {0.6f, 0.45f}, tassel);
            const bool standard = banner.kind == BannerKind::MingStandard;
            const float w = standard ? kMingStandardWidth : kMingPennantWidth;
            const float h = standard ? kMingStandardHeight : kMingPennantHeight;
            const sf::IntRect cell = standard ? mingStandardRect() : mingPennantRect(mingPennantVariant(item.unit));
            const sf::Vector2f cloth[4] = {{x + 0.25f, top + 0.4f},
                                           {x + 0.25f + w, top + 0.4f + banner.sway},
                                           {x + 0.25f + w, top + 0.4f + h + banner.sway},
                                           {x + 0.25f, top + 0.4f + h}};
            appendQuad(cloths, cloth, cell, alphaOf(sf::Color::White));
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
    sf::RenderStates clothStates;
    clothStates.texture = &flags.texture();
    target.draw(cloths, clothStates);

    // Japanese crests over their cloths: Tachibana on the vanguard, Kobayakawa on the main body.
    sf::VertexArray crests(sf::PrimitiveType::Triangles);
    const sf::Color ink = alphaOf({0x1b, 0x1b, 0x1f});
    for (const Placed &item : placed) {
        const Banner &banner = item.banner;
        if (banner.kind != BannerKind::JpVanguard && banner.kind != BannerKind::JpMain) {
            continue;
        }
        const float top = banner.base.y - kBannerPole + 0.3f;
        const sf::Vector2f centre{banner.base.x + 0.25f + kJpBannerWidth * 0.5f + banner.sway * 0.15f,
                                  top + kJpBannerHeight * 0.28f};
        const float radius = kJpBannerWidth * 0.4f;
        if (banner.kind == BannerKind::JpVanguard) {
            appendGionMamori(crests, centre, radius, ink);
        } else {
            appendMitsudomoe(crests, centre, radius, ink);
        }
    }
    target.draw(crests);
}

} // namespace tq
