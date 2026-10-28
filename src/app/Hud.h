#pragma once

#include "app/Pointer.h"
#include "app/Report.h"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace tq {

enum class HudAction : std::uint8_t {
    None,
    ToggleRun,
    Speed0,
    Speed1,
    Speed2,
    ToggleSound,
    Restart,
    Hold,
    Retreat,
    Split,
    Join,
    Cancel,
    ConfirmAttack
};

struct HudFrame {
    const Battle *battle = nullptr;
    const OrderState *orders = nullptr;
    const StringTable *strings = nullptr;
    const sf::Font *body = nullptr;
    const sf::Font *title = nullptr;
    bool titleBold = false;
    const ExitDef *exit = nullptr;
    std::string_view startClock;
    float alpha = 0.f;
    float wallTime = 0.f;
    bool running = false;
    bool muted = false;
    bool reducedMotion = false;
    float speed = 1.f;
    const std::vector<float> *speeds = nullptr;
    float originX = 0.f;
    float originY = 0.f;
    float scale = 1.f;
};

class Hud {
  public:
    void reset();
    void note(const Battle &battle, const StringTable &strings, std::span<const Event> events);

    struct Input {
        bool consumed = false;
        HudAction action = HudAction::None;
    };
    Input handle(const PointerSample &sample);
    void draw(sf::RenderTarget &target, const HudFrame &frame);

    const Journal &journal() const { return journal_; }

  private:
    struct Hit {
        HudAction action = HudAction::None;
        float x = 0.f;
        float y = 0.f;
        float w = 0.f;
        float h = 0.f;
    };

    Journal journal_;
    std::vector<Hit> hits_;
    std::optional<HudAction> armed_;
    int shownBanner_ = 0;
    float bannerStart_ = 0.f;
};

} // namespace tq
