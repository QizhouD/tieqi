#pragma once

#include "app/Formation.h"
#include "app/MingFlags.h"
#include "app/SoldierAtlas.h"
#include "app/Soldiers.h"
#include "core/sim/Battle.h"
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <vector>

namespace tq {

// Draws every soldier of every unit from one atlas in a single VertexArray, sorted by y so nearer men overlap
// farther ones. Dust puffs go underneath; banners go on top. Ming banners are cloths from MingFlags (the commander's
// standard and pennants) and carry no characters; Japanese sashimono carry their crests (Crests.h).
class SoldierView {
  public:
    void draw(sf::RenderTarget &target, const Battle &battle, const FormationLayer &formation,
              const SoldierAtlas &atlas, const MingFlags &flags, float alpha, float animTime, float opacity = 1.f);
    std::size_t lastCount() const { return soldiers_.size(); }

  private:
    void drawBanners(sf::RenderTarget &target, const Battle &battle, const SoldierAtlas &atlas, const MingFlags &flags,
                     float alpha, float animTime, float opacity);

    std::vector<Soldier> soldiers_;
    sf::VertexArray tris_{sf::PrimitiveType::Triangles};
};

} // namespace tq
