#pragma once

#include "app/Window.h"
#include "core/sim/Battle.h"
#include "data/Strings.h"

#include <SFML/Graphics/Font.hpp>

#include <optional>
#include <span>

namespace tq {

// 方阵、棋子、朝向楔形、兵力与士气条、气势、计时弧、翻面和选中框。
// 选中和悬停由调用方传入；点选留到 A3。
class UnitView {
  public:
    void draw(Window &window, const Battle &battle, const StringTable &strings, const sf::Font &glyphFont,
              float timeSec, float alpha, std::span<const UnitId> selected, std::optional<UnitId> hover) const;
};

} // namespace tq
