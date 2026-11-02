#include "app/SoldierView.h"

#include <SFML/Graphics/RenderStates.hpp>

#include <algorithm>

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

} // namespace

void SoldierView::draw(sf::RenderTarget &target, const Battle &battle, const SoldierAtlas &atlas, float alpha,
                       float animTime) {
    soldiers_.clear();
    for (const Unit &unit : battle.units()) {
        layoutSoldiers(unit, battle.typeOf(unit), alpha, animTime, soldiers_);
    }
    std::stable_sort(soldiers_.begin(), soldiers_.end(),
                     [](const Soldier &left, const Soldier &right) { return left.pos.y < right.pos.y; });
    tris_.clear();
    const sf::IntRect dust = SoldierAtlas::cell(kAtlasExtraRow, kAtlasDustColumn);
    for (const Soldier &soldier : soldiers_) {
        if (soldier.dust > 0.f) {
            const auto a = static_cast<std::uint8_t>(std::clamp(soldier.dust, 0.f, 1.f) * 255.f);
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
                     sf::Color{shade, shade, shade, 255});
    }
    sf::RenderStates states;
    states.texture = &atlas.texture();
    target.draw(tris_, states);
}

} // namespace tq
