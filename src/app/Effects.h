#pragma once

#include "core/sim/Battle.h"
#include "data/Strings.h"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace tq {

// Wall-clock seconds of battle-effect time. Paused, frozen, and hit-stop all hold it at zero.
inline float effectSeconds(float wallDt, float speed, float timeScale, bool hold) {
    if (hold || !(wallDt > 0.f)) {
        return 0.f;
    }
    return wallDt * speed * timeScale / 8.f;
}

inline float impactShakePx(float ratio, bool reducedMotion) {
    if (reducedMotion) {
        return 0.f;
    }
    return std::min(12.f, 3.f + 3.f * ratio);
}

inline float impactHitStopSec(float ratio, bool reducedMotion) {
    if (reducedMotion) {
        return 0.04f;
    }
    return 0.07f + std::min(0.12f, 0.035f * ratio);
}

inline int impactDustCount(float ratio) {
    const int scaled = static_cast<int>(std::lround(4.f * ratio));
    return 6 + std::min(10, std::max(0, scaled));
}

// Near-view presentation tuning: finer particles and ratio-scaled impact debris.
inline constexpr int kNearFxCountMul = 3;
inline constexpr float kNearFxSizeMul = 0.45f;
inline constexpr float kImpactClodRatioMul = 3.f;

inline int nearFxCount(int base, float blend) {
    return static_cast<int>(std::lround(static_cast<float>(base) *
                                        (1.f + static_cast<float>(kNearFxCountMul - 1) * std::clamp(blend, 0.f, 1.f))));
}

inline float nearFxSize(float blend) { return 1.f + (kNearFxSizeMul - 1.f) * std::clamp(blend, 0.f, 1.f); }

inline int impactClodCount(float ratio, float blend) {
    const float count = 4.f + std::min(8.f, std::round(kImpactClodRatioMul * ratio));
    return static_cast<int>(std::lround(count * std::clamp(blend, 0.f, 1.f)));
}

// Dust, smoke, chips, rings, floaters, and click marks. The generator is presentation-only.
class Effects {
  public:
    void clear();
    void onEvents(const Battle &battle, const StringTable &strings, std::span<const Event> events, bool reducedMotion,
                  float nearBlend, std::optional<Vec2> assaultAt);
    void marker(Vec2 pos, bool attack);

    bool blocking() const { return hitStop_ > 0.f; }
    void advanceHitStop(float wallDt);
    void update(float fxDt, float wallDt);
    sf::Vector2f shakePixels();

    void drawDust(sf::RenderTarget &target) const;
    // Floating text keeps a fixed screen size (times textScale) and rises at a fixed screen speed whatever the zoom;
    // texts that would overlap are stacked upward in the order they appeared.
    void drawAir(sf::RenderTarget &target, const sf::Font &font, bool bold, float pixelsPerMeter,
                 float textScale = 1.f) const;

  private:
    enum class Kind : std::uint8_t { Dust, Smoke, Flash, Chip, Ring, Mark, Text };

    struct Item {
        Kind kind = Kind::Dust;
        float x = 0.f;
        float y = 0.f;
        float vx = 0.f;
        float vy = 0.f;
        float radius = 0.f;
        float grow = 0.f;
        float life = 0.f;
        float span = 1.f;
        float spin = 0.f;
        float angle = 0.f;
        float size = 1.f;
        float pixels = 20.f;
        bool wallClock = false;
        sf::Color color;
        std::string text;
    };

    void add(Item item);
    void floater(Vec2 pos, const std::string &text, sf::Color color, float pixels);
    float next();
    void ensureBlob() const;

    std::vector<Item> items_;
    float hitStop_ = 0.f;
    float shake_ = 0.f;
    std::mt19937 rng_{1594u};
    mutable sf::Texture blob_;
    mutable bool blobReady_ = false;
    static constexpr std::size_t kMaxItems = 1600;
};

} // namespace tq
