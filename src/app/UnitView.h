#pragma once

#include "app/Orders.h"
#include "app/Window.h"
#include "data/Strings.h"

#include <SFML/Graphics/Font.hpp>

#include <functional>

namespace tq {

// Blocks, chits, order lines, the odds arrow, and the drag box.
// afterBlocks runs above the blocks and under the order lines. afterChits runs above the chits and under the preview.
// With soldiers, the soldier layer stands in for the formation dots, so blocks keep only a faint footprint.
class UnitView {
  public:
    void draw(Window &window, const Battle &battle, const StringTable &strings, const sf::Font &glyphFont,
              const sf::Font &bodyFont, float timeSec, float alpha, const OrderState &orders,
              const std::function<void()> &afterBlocks = {}, const std::function<void()> &afterChits = {},
              bool soldiers = false) const;
};

} // namespace tq
