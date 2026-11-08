#pragma once
#include "core/defs/MapDef.h"
#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <array>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace sf {
class RenderTarget;
}
namespace tq {
inline constexpr unsigned kTreeCellPx = 192, kTreeCells = 8;
inline constexpr float kTreeCellMeters = 24.f, kTreeAnchorX = 96.f, kTreeAnchorY = 184.f;
inline constexpr float kDetailPixelsPerMeter = 8.f;
inline constexpr unsigned kGroundPx = 512, kPaddyPx = 256;
inline constexpr float kGroundMeters = 64.f, kPaddyMeters = 32.f;
inline constexpr float kBankWidth = 1.4f;
inline constexpr float kMapDetailStrength = 1.f;
struct MapTree {
    Vec2 pos;
    unsigned variant = 0;
    float scale = 1.f;
    bool mirror = false;
};
struct BundSegment {
    Vec2 a, b;
};
struct PaddyDetail {
    std::array<Vec2, 4> quad{};
    std::vector<BundSegment> bunds;
};
// Exit belongs to ScenarioDef, not MapDef. Supplying it keeps this function pure for every scenario.
bool treeAllowed(const MapDef &map, const MapTree &tree, Aabb exit = {});
std::vector<MapTree> placeTrees(const MapDef &map, std::uint32_t seed, Aabb exit = {});
sf::VertexArray buildRidges(const std::vector<PaddyDetail> &paddies);
// Tiles multiply the map: with the vertex colour below, the result is dst * lerp(1, tile, blend * strength).
sf::BlendMode mapDetailBlendMode();
sf::Color mapDetailTileColor(float blend);
class MapDetail {
  public:
    void load(const std::filesystem::path &assetRoot);
    void build(const MapDef &map, Aabb exit, const std::vector<PaddyDetail> &paddies);
    void drawGround(sf::RenderTarget &target, float blend) const;
    void drawTrees(sf::RenderTarget &target, float blend) const;

  private:
    void fade(float blend) const;
    sf::Texture treesTexture_, groundTexture_, paddyTexture_;
    mutable sf::VertexArray ground_{sf::PrimitiveType::Triangles};
    mutable sf::VertexArray paddies_{sf::PrimitiveType::Triangles};
    mutable sf::VertexArray ridges_{sf::PrimitiveType::Triangles};
    mutable sf::VertexArray trees_{sf::PrimitiveType::Triangles};
    std::vector<sf::Color> ridgeColors_;
    mutable float blend_ = -1.f;
};
} // namespace tq
