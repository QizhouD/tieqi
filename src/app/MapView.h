#pragma once

#include "app/MapDetail.h"
#include "app/Window.h"
#include "core/defs/ScenarioDef.h"
#include "data/Strings.h"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <cstdint>
#include <random>
#include <string>

namespace tq {

class MapView {
  public:
    void build(const ScenarioDef &scenario, const StringTable &strings, const sf::Font &titleFont, bool titleBold);
    void load(const std::filesystem::path &assetRoot) { detail_.load(assetRoot); }
    const std::vector<PaddyDetail> &paddyDetails() const { return paddyDetails_; }
    void addScar(Vec2 pos, float ratio);
    void draw(Window &window, float timeSec, bool exitOpen, float blend) const;

  private:
    void paintBase(const StringTable &strings, const sf::Font &titleFont, bool titleBold);
    void drawExit(Window &window, float timeSec) const;

    MapDetail detail_;
    std::vector<PaddyDetail> paddyDetails_;
    MapDef map_{};
    Aabb exit_{};
    std::string exitLabel_;
    const sf::Font *titleFont_ = nullptr;
    bool titleBold_ = false;
    sf::RenderTexture base_;
    sf::RenderTexture scars_;
    sf::Texture blob_;
    std::mt19937 scarRng_{1594};
};

} // namespace tq
