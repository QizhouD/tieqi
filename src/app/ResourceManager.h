#pragma once

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <filesystem>
#include <string>
#include <unordered_map>

namespace tq {

// 字体和纹理按路径缓存。返回的引用在 clear() 之前一直有效。
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
