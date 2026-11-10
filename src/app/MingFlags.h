#pragma once

#include "core/sim/Types.h"

#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <filesystem>

namespace tq {

// Ming banner cloths (V9), built once at load from a photograph of a real dragon banner
// (assets/textures/flags/gyoryonggi.png, public domain; provenance in its README). The commander's guard carries the
// whole banner as its standard; every other Ming unit carries a triangular pennant whose field is a crop of the
// banner's painted silk, inside a red border cut into tabs, with a white hoist sleeve. Ming banners carry no
// characters.
//
// Sheet layout: the standard at (0, 0), 512 x 595 (the photograph's aspect); pennant variant v at (512, 200 v),
// 256 x 160, the hoist on the left and the point at the right-hand middle.
inline constexpr unsigned kMingFlagSheetWidth = 768;
inline constexpr unsigned kMingFlagSheetHeight = 600;
inline constexpr int kMingPennantVariants = 3;

sf::IntRect mingStandardRect();
sf::IntRect mingPennantRect(int variant);
// Which pennant crop a unit shows: by ascending UnitId, so neighbours differ.
int mingPennantVariant(UnitId id);
// Builds the sheet. Without a photograph (missing file) it draws plain placeholder cloths in the same layout.
sf::Image buildMingFlagSheet(const sf::Image *source);

class MingFlags {
  public:
    void load(const std::filesystem::path &assetRoot);
    const sf::Texture &texture() const { return texture_; }

  private:
    sf::Texture texture_;
};

} // namespace tq
