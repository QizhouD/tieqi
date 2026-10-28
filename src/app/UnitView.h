#pragma once

#include "app/Orders.h"
#include "app/Window.h"
#include "data/Strings.h"

#include <SFML/Graphics/Font.hpp>

namespace tq {

// 方阵、棋子、命令线、胜算箭头和框选矩形。
class UnitView {
  public:
    void draw(Window &window, const Battle &battle, const StringTable &strings, const sf::Font &glyphFont,
              const sf::Font &bodyFont, float timeSec, float alpha, const OrderState &orders) const;
};

} // namespace tq
