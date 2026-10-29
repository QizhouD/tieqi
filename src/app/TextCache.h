#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/System/Vector2.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace sf {
class Font;
class RenderTarget;
} // namespace sf

namespace tq {

// Keeps one shaped sf::Text alive for every font, pixel size, and bold flag,
// and reuses a shaped text when the string is unchanged. Destroying the last
// text of a size releases SFML's HarfBuzz shaper and the next one re-reads the font.
sf::FloatRect textBounds(const sf::Font &font, unsigned size, bool bold, std::string_view value);

void drawText(sf::RenderTarget &target, const sf::Font &font, unsigned size, bool bold, std::string_view value,
              sf::Color fill, sf::Vector2f position, sf::Vector2f origin, sf::Vector2f scale,
              const sf::RenderStates &states = sf::RenderStates::Default);

void drawOutlinedText(sf::RenderTarget &target, const sf::Font &font, unsigned size, bool bold, std::string_view value,
                      sf::Color fill, sf::Vector2f position, sf::Vector2f origin, sf::Vector2f scale, sf::Color outline,
                      float thickness);

std::vector<std::string> wrapText(const sf::Font &font, std::string_view value, unsigned size, float maxWidth,
                                  bool bold);

void clearTextCache();

class TextCacheGuard {
  public:
    TextCacheGuard() = default;
    TextCacheGuard(const TextCacheGuard &) = delete;
    TextCacheGuard &operator=(const TextCacheGuard &) = delete;
    ~TextCacheGuard();
};

} // namespace tq
