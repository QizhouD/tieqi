#include "app/MingFlags.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <system_error>

namespace tq {
namespace {

constexpr int kStandardW = 512;
constexpr int kStandardH = 595;
constexpr int kPennantW = 256;
constexpr int kPennantH = 160;
constexpr int kPennantStride = 200;

constexpr sf::Color kTabRed{0xc2, 0x33, 0x24};
constexpr sf::Color kSleeve{0xef, 0xe6, 0xd2};
constexpr sf::Color kPaleSilk{0xcf, 0xd3, 0xc4};

// Bilinear sample of the photograph at normalised (u, v).
sf::Color sample(const sf::Image &image, float u, float v) {
    const sf::Vector2u size = image.getSize();
    const float x = std::clamp(u * static_cast<float>(size.x) - 0.5f, 0.f, static_cast<float>(size.x - 1));
    const float y = std::clamp(v * static_cast<float>(size.y) - 0.5f, 0.f, static_cast<float>(size.y - 1));
    const auto x0 = static_cast<unsigned>(x);
    const auto y0 = static_cast<unsigned>(y);
    const unsigned x1 = std::min(x0 + 1, size.x - 1);
    const unsigned y1 = std::min(y0 + 1, size.y - 1);
    const float fx = x - static_cast<float>(x0);
    const float fy = y - static_cast<float>(y0);
    const sf::Color c00 = image.getPixel({x0, y0});
    const sf::Color c10 = image.getPixel({x1, y0});
    const sf::Color c01 = image.getPixel({x0, y1});
    const sf::Color c11 = image.getPixel({x1, y1});
    const auto mix = [&](std::uint8_t a, std::uint8_t b, std::uint8_t c, std::uint8_t d) {
        const float top = static_cast<float>(a) + (static_cast<float>(b) - static_cast<float>(a)) * fx;
        const float bottom = static_cast<float>(c) + (static_cast<float>(d) - static_cast<float>(c)) * fx;
        return static_cast<std::uint8_t>(std::lround(top + (bottom - top) * fy));
    };
    return {mix(c00.r, c10.r, c01.r, c11.r), mix(c00.g, c10.g, c01.g, c11.g), mix(c00.b, c10.b, c01.b, c11.b),
            mix(c00.a, c10.a, c01.a, c11.a)};
}

// Inside the triangle (0, inset), (tipX, h / 2), (0, h - inset)?
bool inTriangle(float x, float y, float inset, float tipX, float h) {
    if (x < 0.f || x > tipX) {
        return false;
    }
    const float mid = h * 0.5f;
    const float top = inset + (mid - inset) * x / tipX;
    const float bottom = (h - inset) - (h - inset - mid) * x / tipX;
    return y >= top && y <= bottom;
}

// On one of `count` tabs along the edge a -> b, on the side `out` (+1 above the top edge, -1 below the bottom edge)?
bool onTab(float x, float y, sf::Vector2f a, sf::Vector2f b, float out, float depth, int count) {
    const sf::Vector2f d = b - a;
    const float length = std::hypot(d.x, d.y);
    const sf::Vector2f dir = d / length;
    const sf::Vector2f normal{dir.y * out, -dir.x * out};
    const sf::Vector2f p{x - a.x, y - a.y};
    const float along = (p.x * dir.x + p.y * dir.y) / length;
    const float away = p.x * normal.x + p.y * normal.y;
    if (along < 0.f || along > 1.f || away < 0.f || away > depth) {
        return false;
    }
    const float slot = along * static_cast<float>(count);
    const float frac = slot - std::floor(slot);
    return frac >= 0.12f && frac <= 0.78f;
}

void drawStandard(sf::Image &sheet, const sf::Image *source) {
    for (int y = 0; y < kStandardH; ++y) {
        for (int x = 0; x < kStandardW; ++x) {
            const float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(kStandardW);
            const float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(kStandardH);
            sf::Color color;
            if (source != nullptr) {
                color = sample(*source, u, v);
            } else {
                const bool border = u < 0.12f || u > 0.88f || v < 0.1f || v > 0.9f;
                color = border ? kTabRed : kPaleSilk;
            }
            sheet.setPixel({static_cast<unsigned>(x), static_cast<unsigned>(y)}, color);
        }
    }
}

void drawPennant(sf::Image &sheet, const sf::Image *source, int variant) {
    const auto w = static_cast<float>(kPennantW);
    const auto h = static_cast<float>(kPennantH);
    constexpr float border = 19.f;
    const float tipX = w - 3.f - border * 2.6f;
    const sf::Vector2f topA{0.f, border};
    const sf::Vector2f bottomA{0.f, h - border};
    const sf::Vector2f tip{tipX, h * 0.5f};
    // The crop of the painted field this variant shows.
    const float cropX = 0.24f + 0.1f * static_cast<float>(variant % 3);
    const float cropY = 0.22f + 0.12f * static_cast<float>(variant % 2);
    const int originY = kPennantStride * variant;
    for (int y = 0; y < kPennantH; ++y) {
        for (int x = 0; x < kPennantW; ++x) {
            const float fx = static_cast<float>(x) + 0.5f;
            const float fy = static_cast<float>(y) + 0.5f;
            sf::Color color = sf::Color::Transparent;
            if (inTriangle(fx, fy, border, tipX, h) || onTab(fx, fy, topA, tip, 1.f, border, 7) ||
                onTab(fx, fy, bottomA, tip, -1.f, border, 7)) {
                color = kTabRed;
            }
            if (inTriangle(fx, fy, border + 16.f, tipX - 30.f, h)) {
                if (source != nullptr) {
                    color = sample(*source, cropX + 0.42f * fx / w, cropY + 0.27f * fy / h);
                    color.a = 255;
                } else {
                    color = kPaleSilk;
                }
            }
            if (fx < 10.f) {
                color = kSleeve;
            }
            sheet.setPixel({static_cast<unsigned>(kStandardW + x), static_cast<unsigned>(originY + y)}, color);
        }
    }
}

} // namespace

sf::IntRect mingStandardRect() { return {{0, 0}, {kStandardW, kStandardH}}; }

sf::IntRect mingPennantRect(int variant) {
    return {{kStandardW, kPennantStride * std::clamp(variant, 0, kMingPennantVariants - 1)}, {kPennantW, kPennantH}};
}

int mingPennantVariant(UnitId id) { return static_cast<int>(id % static_cast<UnitId>(kMingPennantVariants)); }

sf::Image buildMingFlagSheet(const sf::Image *source) {
    sf::Image sheet({kMingFlagSheetWidth, kMingFlagSheetHeight}, sf::Color::Transparent);
    drawStandard(sheet, source);
    for (int variant = 0; variant < kMingPennantVariants; ++variant) {
        drawPennant(sheet, source, variant);
    }
    return sheet;
}

void MingFlags::load(const std::filesystem::path &assetRoot) {
    const std::filesystem::path file = assetRoot / "textures" / "flags" / "gyoryonggi.png";
    sf::Image photo;
    std::error_code error;
    const bool loaded = std::filesystem::exists(file, error) && photo.loadFromFile(file);
    if (!loaded) {
        std::cerr << "banner photograph " << file.string() << " missing or invalid; using placeholder cloths\n";
    }
    if (!texture_.loadFromImage(buildMingFlagSheet(loaded ? &photo : nullptr))) {
        throw std::runtime_error("could not upload the Ming banner cloths");
    }
    texture_.setSmooth(true);
    if (!texture_.generateMipmap()) {
        std::cerr << "Ming banner mipmap failed\n";
    }
}

} // namespace tq
