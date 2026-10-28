#pragma once

#include "app/Orders.h"
#include "app/Window.h"
#include "data/Strings.h"

#include <SFML/Graphics/Font.hpp>

namespace tq {

// Blocks, chits, order lines, the odds arrow, and the drag box.
class UnitView {
  public:
    void draw(Window &window, const Battle &battle, const StringTable &strings, const sf::Font &glyphFont,
              const sf::Font &bodyFont, float timeSec, float alpha, const OrderState &orders) const;
};

} // namespace tq
