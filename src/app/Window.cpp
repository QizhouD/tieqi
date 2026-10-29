#include "app/Window.h"

#include "app/Orders.h"

#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Window/WindowEnums.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace tq {
namespace {

sf::FloatRect uiRect() {
    return sf::FloatRect({0.f, 0.f}, {static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight)});
}

sf::Vector2f mapExtent(sf::Vector2u world) {
    return {static_cast<float>(std::max(1u, world.x)), static_cast<float>(std::max(1u, world.y))};
}

bool shiftDown() {
    return sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ||
           sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);
}

PointerSample pointerAt(PointerSample::Type type, PointerSample::Device device, int button, sf::Vector2i pixel,
                        sf::Vector2u pixelSize, bool shift) {
    const Vec2 logical = logicalFromPixel(static_cast<float>(pixel.x), static_cast<float>(pixel.y),
                                          static_cast<float>(pixelSize.x), static_cast<float>(pixelSize.y));
    PointerSample sample;
    sample.type = type;
    sample.device = device;
    sample.button = button;
    sample.x = logical.x;
    sample.y = logical.y;
    sample.shift = shift;
    return sample;
}

int pointerButton(sf::Mouse::Button button) {
    return button == sf::Mouse::Button::Right ? kPointerRight : kPointerLeft;
}

} // namespace

void Window::open(const sf::String &title) {
    title_ = title;
    create();
}

void Window::create() {
    if (fullscreen_) {
        const sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
        window_.create(desktop, title_, sf::State::Fullscreen);
    } else {
        const sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
        const WindowFit fit = fitWindow(desktop.size.x, desktop.size.y);
        pixelSize_ = {fit.defaultW, fit.defaultH};
        window_.create(sf::VideoMode(pixelSize_), title_, sf::Style::Default);
        window_.setMinimumSize(sf::Vector2u{fit.minW, fit.minH});
    }
    pixelSize_ = window_.getSize();
    window_.setFramerateLimit(60);
    window_.setVerticalSyncEnabled(false);
    window_.setKeyRepeatEnabled(false);
    applyUiView();
}

void Window::applyUiView() {
    sf::View view(uiRect());
    const Viewport box = letterbox(static_cast<float>(pixelSize_.x), static_cast<float>(pixelSize_.y),
                                   static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight));
    view.setViewport(sf::FloatRect({box.x, box.y}, {box.w, box.h}));
    window_.setView(view);
}

void Window::setWorld(sf::Vector2u worldSize) {
    worldSize_ = worldSize;
    camera_ = {worldSize.x * 0.5f, worldSize.y * 0.5f};
}

void Window::snapCamera(sf::Vector2f target) { camera_ = target; }

void Window::followCamera(sf::Vector2f target, float elapsed) {
    const float blend = std::min(1.f, 9.f * elapsed);
    camera_ += (target - camera_) * blend;
    const float halfW = static_cast<float>(kWindowWidth) * 0.5f;
    const float halfH = static_cast<float>(kWindowHeight) * 0.5f;
    const float maxX = std::max(halfW, static_cast<float>(worldSize_.x) - halfW);
    const float maxY = std::max(halfH, static_cast<float>(worldSize_.y) - halfH);
    camera_.x = std::clamp(static_cast<float>(std::round(camera_.x)), halfW, maxX);
    camera_.y = std::clamp(static_cast<float>(std::round(camera_.y)), halfH, maxY);
}

void Window::applyView(sf::Vector2f center, sf::Vector2f size, ScreenRect screen) {
    sf::View view(center, size);
    const Viewport box = letterbox(static_cast<float>(pixelSize_.x), static_cast<float>(pixelSize_.y),
                                   static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight));
    const float logicalW = static_cast<float>(kWindowWidth);
    const float logicalH = static_cast<float>(kWindowHeight);
    view.setViewport(sf::FloatRect({box.x + (screen.x / logicalW) * box.w, box.y + (screen.y / logicalH) * box.h},
                                   {(screen.w / logicalW) * box.w, (screen.h / logicalH) * box.h}));
    window_.setView(view);
}

void Window::beginUi() { applyUiView(); }

void Window::beginPixels() {
    const float width = static_cast<float>(std::max(1u, pixelSize_.x));
    const float height = static_cast<float>(std::max(1u, pixelSize_.y));
    sf::View view(sf::FloatRect({0.f, 0.f}, {width, height}));
    window_.setView(view);
}

Window::PixelMap Window::pixelMap() const {
    const float width = static_cast<float>(std::max(1u, pixelSize_.x));
    const float height = static_cast<float>(std::max(1u, pixelSize_.y));
    const Viewport box = letterbox(width, height, static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight));
    PixelMap map;
    map.originX = box.x * width;
    map.originY = box.y * height;
    map.scale = box.h * height / static_cast<float>(kWindowHeight);
    if (!(map.scale > 0.f)) {
        map.scale = 1.f;
    }
    return map;
}

void Window::setViewShake(sf::Vector2f pixels) { shakePixels_ = pixels; }

sf::Vector2f Window::shakenCenter(sf::Vector2f center, float scale) const {
    if (scale > 0.f) {
        center.x -= shakePixels_.x / scale;
        center.y -= shakePixels_.y / scale;
    }
    return center;
}

void Window::beginWorld() {
    const ScreenRect field = battlefieldRect();
    const sf::Vector2f map = mapExtent(worldSize_);
    const MapCamera placed = mapCamera(field, map.x, map.y);
    applyView(shakenCenter({map.x * 0.5f, map.y * 0.5f}, placed.scale),
              {field.w / placed.scale, field.h / placed.scale}, field);
}

void Window::beginMap() {
    const sf::Vector2f map = mapExtent(worldSize_);
    const MapCamera placed = mapCamera(battlefieldRect(), map.x, map.y);
    const ScreenRect screen{placed.originX, placed.originY, map.x * placed.scale, map.y * placed.scale};
    applyView(shakenCenter({map.x * 0.5f, map.y * 0.5f}, placed.scale), {map.x, map.y}, screen);
}

float Window::pixelsPerMeter() const {
    const sf::Vector2f map = mapExtent(worldSize_);
    return mapCamera(battlefieldRect(), map.x, map.y).scale;
}

sf::FloatRect Window::visibleWorld() const {
    const ScreenRect field = battlefieldRect();
    const float mapW = static_cast<float>(std::max(1u, worldSize_.x));
    const float mapH = static_cast<float>(std::max(1u, worldSize_.y));
    const MapCamera placed = mapCamera(field, mapW, mapH);
    const float viewW = field.w / placed.scale;
    const float viewH = field.h / placed.scale;
    return sf::FloatRect({mapW * 0.5f - viewW * 0.5f, mapH * 0.5f - viewH * 0.5f}, {viewW, viewH});
}

void Window::addPointer(PointerSample sample) {
    if (!scripted_) {
        pointers_.push_back(sample);
    }
}

std::vector<PointerSample> Window::takePointers() {
    std::vector<PointerSample> taken;
    taken.swap(pointers_);
    return taken;
}

void Window::poll() {
    pressed_.clear();
    pointers_.clear();
    while (const std::optional event = window_.pollEvent()) {
        const bool taken = hook_ && hook_(*event);
        if (taken && !event->is<sf::Event::Closed>() && !event->is<sf::Event::Resized>()) {
            continue;
        }
        if (event->is<sf::Event::Closed>()) {
            done_ = true;
        } else if (const auto *key = event->getIf<sf::Event::KeyPressed>()) {
            if (!scripted_) {
                if (key->code == sf::Keyboard::Key::F5) {
                    fullscreen_ = !fullscreen_;
                    create();
                } else {
                    pressed_.push_back(key->code);
                }
            }
        } else if (const auto *resized = event->getIf<sf::Event::Resized>()) {
            pixelSize_ = resized->size;
            applyUiView();
        } else if (const auto *pressed = event->getIf<sf::Event::MouseButtonPressed>()) {
            if (pressed->button == sf::Mouse::Button::Left || pressed->button == sf::Mouse::Button::Right) {
                addPointer(pointerAt(PointerSample::Type::Down, PointerSample::Device::Mouse,
                                     pointerButton(pressed->button), pressed->position, pixelSize_, shiftDown()));
            }
        } else if (const auto *released = event->getIf<sf::Event::MouseButtonReleased>()) {
            if (released->button == sf::Mouse::Button::Left || released->button == sf::Mouse::Button::Right) {
                addPointer(pointerAt(PointerSample::Type::Up, PointerSample::Device::Mouse,
                                     pointerButton(released->button), released->position, pixelSize_, shiftDown()));
            }
        } else if (const auto *moved = event->getIf<sf::Event::MouseMoved>()) {
            addPointer(pointerAt(PointerSample::Type::Move, PointerSample::Device::Mouse, kPointerLeft, moved->position,
                                 pixelSize_, shiftDown()));
        } else if (const auto *began = event->getIf<sf::Event::TouchBegan>()) {
            if (!touchFinger_) {
                touchFinger_ = began->finger;
                addPointer(pointerAt(PointerSample::Type::Down, PointerSample::Device::Touch, kPointerLeft,
                                     began->position, pixelSize_, false));
            }
        } else if (const auto *touchMoved = event->getIf<sf::Event::TouchMoved>()) {
            if (touchFinger_ && *touchFinger_ == touchMoved->finger) {
                addPointer(pointerAt(PointerSample::Type::Move, PointerSample::Device::Touch, kPointerLeft,
                                     touchMoved->position, pixelSize_, false));
            }
        } else if (const auto *ended = event->getIf<sf::Event::TouchEnded>()) {
            if (touchFinger_ && *touchFinger_ == ended->finger) {
                addPointer(pointerAt(PointerSample::Type::Up, PointerSample::Device::Touch, kPointerLeft,
                                     ended->position, pixelSize_, false));
                touchFinger_.reset();
            }
        }
    }
    if (scripted_) {
        advanceScript();
    }
}

bool Window::wasKeyPressed(sf::Keyboard::Key key) const {
    return std::find(pressed_.begin(), pressed_.end(), key) != pressed_.end();
}

bool Window::isKeyDown(sf::Keyboard::Key key) const {
    if (scripted_) {
        return held_.count(key) > 0 || wasKeyPressed(key);
    }
    return sf::Keyboard::isKeyPressed(key);
}

sf::Keyboard::Key Window::keyFromName(const std::string &name) {
    static const std::unordered_map<std::string, sf::Keyboard::Key> named = {
        {"Enter", sf::Keyboard::Key::Enter}, {"Escape", sf::Keyboard::Key::Escape}, {"Esc", sf::Keyboard::Key::Escape},
        {"Space", sf::Keyboard::Key::Space}, {"LShift", sf::Keyboard::Key::LShift}, {"Up", sf::Keyboard::Key::Up},
        {"Down", sf::Keyboard::Key::Down},   {"Left", sf::Keyboard::Key::Left},     {"Right", sf::Keyboard::Key::Right},
        {"Tab", sf::Keyboard::Key::Tab},     {"F1", sf::Keyboard::Key::F1},         {"F2", sf::Keyboard::Key::F2},
        {"F3", sf::Keyboard::Key::F3},       {"F4", sf::Keyboard::Key::F4},         {"F5", sf::Keyboard::Key::F5},
        {"F6", sf::Keyboard::Key::F6},
    };
    if (const auto found = named.find(name); found != named.end()) {
        return found->second;
    }
    if (name.size() == 1 && name[0] >= 'A' && name[0] <= 'Z') {
        return static_cast<sf::Keyboard::Key>(static_cast<int>(sf::Keyboard::Key::A) + (name[0] - 'A'));
    }
    if (name.size() == 1 && name[0] >= 'a' && name[0] <= 'z') {
        return static_cast<sf::Keyboard::Key>(static_cast<int>(sf::Keyboard::Key::A) + (name[0] - 'a'));
    }
    if (name.size() == 1 && name[0] >= '0' && name[0] <= '9') {
        return static_cast<sf::Keyboard::Key>(static_cast<int>(sf::Keyboard::Key::Num0) + (name[0] - '0'));
    }
    if (name.size() == 4 && name.rfind("Num", 0) == 0 && name[3] >= '0' && name[3] <= '9') {
        return static_cast<sf::Keyboard::Key>(static_cast<int>(sf::Keyboard::Key::Num0) + (name[3] - '0'));
    }
    return sf::Keyboard::Key::Unknown;
}

void Window::loadScript(const std::filesystem::path &file) {
    std::ifstream in(file);
    if (!in) {
        throw std::runtime_error("script not found: " + file.string());
    }
    script_ = parseScript(in);
    scriptPos_ = 0;
    scripted_ = true;
    held_.clear();
    scriptClockStarted_ = false;
}

void Window::advanceScript() {
    if (!scriptClockStarted_) {
        scriptClock_.restart();
        scriptClockStarted_ = true;
    }
    const float now = scriptClock_.getElapsedTime().asSeconds();
    while (scriptPos_ < script_.size() && script_[scriptPos_].time <= now) {
        const ScriptEvent event = script_[scriptPos_];
        scriptPos_ += 1;
        if (event.action == "down" || event.action == "up" || event.action == "press") {
            const sf::Keyboard::Key key = keyFromName(event.arg);
            if (event.action == "up") {
                held_.erase(key);
            } else {
                if (event.action == "down") {
                    held_.insert(key);
                }
                pressed_.push_back(key);
            }
        } else if (event.action == "shot") {
            pendingShot_ = event.arg;
        } else if (event.action == "quit") {
            done_ = true;
        } else if (isPointerAction(event.action)) {
            for (const PointerSample &sample : samplesForScript(event)) {
                pointers_.push_back(sample);
            }
        } else if (event.action == "resize") {
            unsigned width = 0;
            unsigned height = 0;
            std::istringstream size(event.arg);
            if ((size >> width >> height) && width > 0 && height > 0) {
                window_.setMinimumSize(sf::Vector2u{width, height});
                window_.setSize({width, height});
                pixelSize_ = {width, height};
                applyUiView();
            }
        } else {
            std::cerr << "unknown script action: " << event.action << '\n';
        }
    }
}

void Window::takeScreenshot(const std::filesystem::path &file) {
    if (file.has_parent_path()) {
        std::error_code error;
        std::filesystem::create_directories(file.parent_path(), error);
    }
    const sf::Vector2u size = window_.getSize();
    if (size.x == 0 || size.y == 0) {
        return;
    }
    sf::Texture texture(size);
    texture.update(window_);
    if (!texture.copyToImage().saveToFile(file)) {
        std::cerr << "screenshot failed: " << file.string() << '\n';
    }
}

void Window::beginDraw() {
    window_.clear(sf::Color(0x10, 0x0a, 0x08));
    applyUiView();
}

void Window::endDraw() {
    if (!pendingShot_.empty()) {
        takeScreenshot(pendingShot_);
        pendingShot_.clear();
    }
    window_.display();
}

void Window::draw(const sf::Drawable &drawable) { window_.draw(drawable); }

sf::RenderTarget &Window::target() { return window_; }

void Window::close() { done_ = true; }

void Window::unlimitedFrameRate() {
    window_.setFramerateLimit(0);
    window_.setVerticalSyncEnabled(false);
}

} // namespace tq
