#pragma once

#include "app/Script.h"
#include "app/Viewport.h"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Keyboard.hpp>

#include <filesystem>
#include <set>
#include <string>
#include <vector>

namespace tq {

// 逻辑分辨率固定，窗口任意比例都用黑边保住纵横比。脚本输入沿用框架。
class Window {
  public:
    void open(const sf::String &title);
    void loadScript(const std::filesystem::path &file);
    void poll();
    void beginDraw();
    void endDraw();
    void draw(const sf::Drawable &drawable);
    sf::RenderTarget &target();
    void close();

    bool isOpen() const { return !done_; }
    bool scripted() const { return scripted_; }
    bool wasKeyPressed(sf::Keyboard::Key key) const;
    bool isKeyDown(sf::Keyboard::Key key) const;
    sf::Vector2u pixelSize() const { return pixelSize_; }
    sf::Vector2u logicalSize() const { return {kWindowWidth, kWindowHeight}; }

    // 世界镜头留给底图。UI 始终盖在逻辑分辨率上，不跟着镜头走。
    void setWorld(sf::Vector2u worldSize);
    void snapCamera(sf::Vector2f target);
    void followCamera(sf::Vector2f target, float elapsed);
    void beginWorld();
    // 棋子画在地图矩形里。视口裁到地图，超出地图的单位不画进深色边。
    void beginMap();
    float pixelsPerMeter() const;
    sf::FloatRect visibleWorld() const;

  private:
    void create();
    void applyUiView();
    void applyView(sf::Vector2f center, sf::Vector2f size, ScreenRect screen);
    void advanceScript();
    void takeScreenshot(const std::filesystem::path &file);
    static sf::Keyboard::Key keyFromName(const std::string &name);

    sf::RenderWindow window_;
    sf::String title_;
    sf::Vector2u pixelSize_{kWindowWidth, kWindowHeight};
    sf::Vector2u worldSize_{kWindowWidth, kWindowHeight};
    sf::Vector2f camera_{kWindowWidth * 0.5f, kWindowHeight * 0.5f};
    bool done_ = false;
    bool fullscreen_ = false;

    std::vector<sf::Keyboard::Key> pressed_;
    std::vector<ScriptEvent> script_;
    std::size_t scriptPos_ = 0;
    bool scripted_ = false;
    sf::Clock scriptClock_;
    std::set<sf::Keyboard::Key> held_;
    std::filesystem::path pendingShot_;
};

} // namespace tq
