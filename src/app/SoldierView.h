#pragma once

#include "app/Formation.h"
#include "app/SoldierAtlas.h"
#include "app/Soldiers.h"
#include "core/sim/Battle.h"
#include "data/Strings.h"

#include <SFML/Graphics/Font.hpp>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <vector>

namespace tq {

// Draws every soldier of every unit from one atlas in a single VertexArray, sorted by y so nearer men overlap
// farther ones. Dust puffs go underneath; banners go on top. A Ming banner shows the character from
// glyph.flag.<unit id without "unit.">, or else glyph.flag.<unit type id>, in the brush font.
class SoldierView {
  public:
    void draw(sf::RenderTarget &target, const Battle &battle, const FormationLayer &formation,
              const SoldierAtlas &atlas, float alpha, float animTime, const StringTable &strings,
              const sf::Font &brush, float opacity = 1.f);
    std::size_t lastCount() const { return soldiers_.size(); }

  private:
    void drawBanners(sf::RenderTarget &target, const Battle &battle, const SoldierAtlas &atlas, float alpha,
                     float animTime, const StringTable &strings, const sf::Font &brush, float opacity);

    std::vector<Soldier> soldiers_;
    sf::VertexArray tris_{sf::PrimitiveType::Triangles};
};

} // namespace tq
