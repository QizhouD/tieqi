#pragma once

#include "app/Orders.h"
#include "app/Window.h"
#include "data/Strings.h"

#include <SFML/Graphics/Font.hpp>

#include <functional>

namespace tq {

// Blocks, chits, order lines, the odds arrow, and the drag box.
// afterBlocks runs above the blocks and under the order lines. afterChits runs above the chits and under the preview.
// nearBlend fades the blocks out toward the near view (1), where the soldier layer replaces them; chitScale sizes
// the counters.
class UnitView {
  public:
    void draw(Window &window, const Battle &battle, const StringTable &strings, const sf::Font &glyphFont,
              const sf::Font &bodyFont, float timeSec, float alpha, const OrderState &orders,
              const std::function<void()> &afterBlocks = {}, const std::function<void()> &afterChits = {},
              float nearBlend = 0.f, float chitScale = 1.f) const;
};

} // namespace tq
