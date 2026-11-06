#pragma once

#include "app/Soldiers.h"

#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <filesystem>

namespace tq {

// One atlas holds every soldier frame. The layout is fixed so a finished PNG can replace the placeholder without code
// changes:
// - square cells of kAtlasCellPx pixels; a cell spans kAtlasCellMeters on the map;
// - column = frame (see Soldiers.h: 0 idle, 1-4 move cycle, 5-6 strike, 7 idle looking aside, 8 fallen);
// - row = (side * kSoldierKinds + kind) * 8 + direction, side 0 Ming and 1 Japanese, kind in SoldierKind order,
//   direction 0 east, then clockwise in 45 degree steps (2 south, 4 west, 6 north);
// - the last row holds extras: column 0 a dust puff, column 1 the Ming unit banner cloth (its character is drawn at
//   run time), column 2 the Japanese vanguard (Tachibana) sashimono cloth, column 3 the Japanese main body
//   (Kobayakawa) sashimono cloth; a banner cloth fills its whole cell, and the Japanese crests are vector shapes
//   drawn over it at run time (Crests.h);
// - the figure stands with its feet at (kAtlasAnchorX, kAtlasAnchorY) in the cell.
inline constexpr int kAtlasCellPx = 32;
inline constexpr float kAtlasCellMeters = 9.f;
inline constexpr float kAtlasAnchorX = 16.f;
inline constexpr float kAtlasAnchorY = 26.f;
inline constexpr int kAtlasFigureRows = 2 * kSoldierKinds * kAtlasDirections;
inline constexpr int kAtlasExtraRow = kAtlasFigureRows;
inline constexpr int kAtlasRows = kAtlasFigureRows + 1;
inline constexpr int kAtlasDustColumn = 0;
inline constexpr int kAtlasMingBannerColumn = 1;
inline constexpr int kAtlasJpVanguardBannerColumn = 2;
inline constexpr int kAtlasJpMainBannerColumn = 3;

inline int atlasRow(Side side, SoldierKind kind, int direction) {
    return ((side == Side::Ming ? 0 : 1) * kSoldierKinds + static_cast<int>(kind)) * kAtlasDirections + direction;
}

class SoldierAtlas {
  public:
    // Loads assets/textures/soldiers/soldiers.png when it exists and has the right size; otherwise draws the
    // placeholder silhouettes.
    void load(const std::filesystem::path &assetRoot);
    const sf::Texture &texture() const { return texture_; }
    static sf::IntRect cell(int row, int column);
    // The placeholder atlas as an image, for --dump-atlas.
    static sf::Image placeholder();

  private:
    sf::Texture texture_;
};

} // namespace tq
