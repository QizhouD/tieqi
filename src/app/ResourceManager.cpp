#include "app/ResourceManager.h"

#include <stdexcept>
#include <utility>

namespace tq {

const sf::Font &ResourceManager::font(const std::filesystem::path &file) {
    const std::string key = file.string();
    if (const auto found = fonts_.find(key); found != fonts_.end()) {
        return found->second;
    }
    sf::Font loaded;
    if (!loaded.openFromFile(file)) {
        throw std::runtime_error("找不到字体: " + key);
    }
    const auto inserted = fonts_.emplace(key, std::move(loaded));
    return inserted.first->second;
}

const sf::Texture &ResourceManager::texture(const std::filesystem::path &file) {
    const std::string key = file.string();
    if (const auto found = textures_.find(key); found != textures_.end()) {
        return found->second;
    }
    sf::Texture loaded;
    if (!loaded.loadFromFile(file)) {
        throw std::runtime_error("找不到纹理: " + key);
    }
    const auto inserted = textures_.emplace(key, std::move(loaded));
    return inserted.first->second;
}

void ResourceManager::clear() {
    fonts_.clear();
    textures_.clear();
}

} // namespace tq
