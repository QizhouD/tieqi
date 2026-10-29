#pragma once

#include "app/Pointer.h"
#include "app/Script.h"
#include "app/Viewport.h"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>

#include <filesystem>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace tq {

// The logical resolution is fixed. Any window aspect keeps black bars. Scripted input follows the shell.
class Window {
  public:
    void open(const sf::String &title);
    void setTitle(const sf::String &title);
    void loadScript(const std::filesystem::path &file);
    void poll();
    void beginDraw();
    void endDraw();
    void draw(const sf::Drawable &drawable);
    sf::RenderTarget &target();
    void close();
    void unlimitedFrameRate();
    // Sees every event first. Returning true keeps keyboard and pointer input from the game; close and resize
    // still apply.
    void setEventHook(std::function<bool(const sf::Event &)> hook) { hook_ = std::move(hook); }
    sf::RenderWindow &native() { return window_; }

    bool isOpen() const { return !done_; }
    bool scripted() const { return scripted_; }
    bool wasKeyPressed(sf::Keyboard::Key key) const;
    bool isKeyDown(sf::Keyboard::Key key) const;
    const std::vector<sf::Keyboard::Key> &pressedKeys() const { return pressed_; }
    std::vector<PointerSample> takePointers();
    sf::Vector2u pixelSize() const { return pixelSize_; }
    sf::Vector2u logicalSize() const { return {kWindowWidth, kWindowHeight}; }

    // The world camera is for the map. UI stays on the logical resolution and does not follow the camera.
    void setWorld(sf::Vector2u worldSize);
    void snapCamera(sf::Vector2f target);
    void followCamera(sf::Vector2f target, float elapsed);
    void beginUi();
    void beginPixels();
    struct PixelMap {
        float originX = 0.f;
        float originY = 0.f;
        float scale = 1.f;
    };
    PixelMap pixelMap() const;
    void setViewShake(sf::Vector2f pixels);
    void beginWorld();
    // Pieces are drawn in the map rectangle. The viewport is clipped to the map, so units outside it do not paint the
    // dark margin.
    void beginMap();
    float pixelsPerMeter() const;
    sf::FloatRect visibleWorld() const;

  private:
    void create();
    void applyUiView();
    void applyView(sf::Vector2f center, sf::Vector2f size, ScreenRect screen);
    sf::Vector2f shakenCenter(sf::Vector2f center, float scale) const;
    void advanceScript();
    void takeScreenshot(const std::filesystem::path &file);
    void addPointer(PointerSample sample);
    static sf::Keyboard::Key keyFromName(const std::string &name);

    sf::RenderWindow window_;
    sf::String title_;
    sf::Vector2u pixelSize_{kWindowWidth, kWindowHeight};
    sf::Vector2u worldSize_{kWindowWidth, kWindowHeight};
    sf::Vector2f camera_{kWindowWidth * 0.5f, kWindowHeight * 0.5f};
    sf::Vector2f shakePixels_{};
    bool done_ = false;
    bool fullscreen_ = false;

    std::vector<sf::Keyboard::Key> pressed_;
    std::vector<PointerSample> pointers_;
    std::optional<unsigned> touchFinger_;
    std::vector<ScriptEvent> script_;
    std::size_t scriptPos_ = 0;
    bool scripted_ = false;
    bool scriptClockStarted_ = false;
    sf::Clock scriptClock_;
    std::set<sf::Keyboard::Key> held_;
    std::filesystem::path pendingShot_;
    std::function<bool(const sf::Event &)> hook_;
};

} // namespace tq
