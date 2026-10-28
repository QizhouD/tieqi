#pragma once

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <filesystem>
#include <string>
#include <unordered_map>

namespace tq {

// Fonts and textures are cached by path. The returned reference stays valid until clear().
class ResourceManager {
  public:
    const sf::Font &font(const std::filesystem::path &file);
    const sf::Texture &texture(const std::filesystem::path &file);
    void clear();

  private:
    std::unordered_map<std::string, sf::Font> fonts_;
    std::unordered_map<std::string, sf::Texture> textures_;
};

} // namespace tq
