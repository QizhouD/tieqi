#pragma once

#include "app/SoldierAtlas.h"
#include "app/Soldiers.h"
#include "core/sim/Battle.h"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <vector>

namespace tq {

// Draws every soldier of every unit from one atlas in a single VertexArray, sorted by y so nearer men overlap
// farther ones. Dust puffs go underneath.
class SoldierView {
  public:
    void draw(sf::RenderTarget &target, const Battle &battle, const SoldierAtlas &atlas, float alpha, float animTime);
    std::size_t lastCount() const { return soldiers_.size(); }

  private:
    std::vector<Soldier> soldiers_;
    sf::VertexArray tris_{sf::PrimitiveType::Triangles};
};

} // namespace tq
