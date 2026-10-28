#include "app/MapView.h"

#include "app/Viewport.h"

#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/String.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace tq {
namespace {

constexpr float kPi = 3.14159265f;

sf::String utf8(std::string_view text) { return sf::String::fromUtf8(text.begin(), text.end()); }

sf::Color rgba(int r, int g, int b, float a) {
    return {static_cast<std::uint8_t>(r), static_cast<std::uint8_t>(g), static_cast<std::uint8_t>(b),
            static_cast<std::uint8_t>(std::clamp(a, 0.f, 1.f) * 255.f + 0.5f)};
}

sf::Texture makeBlob() {
    constexpr unsigned n = 64;
    sf::Image image({n, n}, sf::Color::Transparent);
    const float center = (n - 1) * 0.5f;
    for (unsigned y = 0; y < n; ++y) {
        for (unsigned x = 0; x < n; ++x) {
            const float dist = std::hypot(static_cast<float>(x) - center, static_cast<float>(y) - center) / center;
            if (dist >= 1.f) {
                continue;
            }
            const float alpha = 1.f - dist;
            image.setPixel({x, y}, sf::Color(255, 255, 255, static_cast<std::uint8_t>(alpha * 255.f + 0.5f)));
        }
    }
    return sf::Texture(image);
}

void drawBlob(sf::RenderTarget &target, const sf::Texture &blob, sf::Vector2f center, float radius, sf::Color color) {
    sf::Sprite sprite(blob);
    const sf::Vector2u size = blob.getSize();
    sprite.setOrigin({size.x * 0.5f, size.y * 0.5f});
    sprite.setPosition(center);
    sprite.setScale({(2.f * radius) / static_cast<float>(size.x), (2.f * radius) / static_cast<float>(size.y)});
    sprite.setColor(color);
    target.draw(sprite);
}

void appendLine(sf::VertexArray &lines, sf::Vector2f a, sf::Vector2f b, sf::Color color) {
    lines.append(sf::Vertex{a, color});
    lines.append(sf::Vertex{b, color});
}

Vec2 quadratic(Vec2 a, Vec2 control, Vec2 end, float t) {
    const float u = 1.f - t;
    return a * (u * u) + control * (2.f * u * t) + end * (t * t);
}

std::vector<Vec2> smoothRoad(const std::vector<Vec2> &pts) {
    std::vector<Vec2> out;
    if (pts.empty()) {
        return out;
    }
    out.push_back(pts.front());
    if (pts.size() == 1) {
        return out;
    }
    Vec2 current = pts.front();
    for (std::size_t i = 1; i + 1 < pts.size(); ++i) {
        const Vec2 mid = (pts[i] + pts[i + 1]) * 0.5f;
        for (int s = 1; s <= 12; ++s) {
            out.push_back(quadratic(current, pts[i], mid, static_cast<float>(s) / 12.f));
        }
        current = mid;
    }
    out.push_back(pts.back());
    return out;
}

void appendStrip(sf::VertexArray &triangles, const std::vector<Vec2> &pts, float width, sf::Color color) {
    if (pts.size() < 2) {
        return;
    }
    const float half = width * 0.5f;
    for (std::size_t i = 0; i + 1 < pts.size(); ++i) {
        Vec2 dir = pts[i + 1] - pts[i];
        const float len = std::hypot(dir.x, dir.y);
        if (len < 1e-4f) {
            continue;
        }
        dir = {dir.x / len, dir.y / len};
        const Vec2 n{-dir.y * half, dir.x * half};
        const sf::Vector2f a{pts[i].x + n.x, pts[i].y + n.y};
        const sf::Vector2f b{pts[i].x - n.x, pts[i].y - n.y};
        const sf::Vector2f c{pts[i + 1].x + n.x, pts[i + 1].y + n.y};
        const sf::Vector2f d{pts[i + 1].x - n.x, pts[i + 1].y - n.y};
        triangles.append(sf::Vertex{a, color});
        triangles.append(sf::Vertex{b, color});
        triangles.append(sf::Vertex{c, color});
        triangles.append(sf::Vertex{c, color});
        triangles.append(sf::Vertex{b, color});
        triangles.append(sf::Vertex{d, color});
    }
}

void drawDashed(sf::RenderTarget &target, const std::vector<Vec2> &pts, float on, float off, float thickness,
                sf::Color color) {
    if (pts.size() < 2) {
        return;
    }
    float phase = 0.f;
    bool drawing = true;
    std::vector<Vec2> segment;
    segment.push_back(pts.front());
    auto flush = [&] {
        if (segment.size() >= 2) {
            sf::VertexArray triangles(sf::PrimitiveType::Triangles);
            appendStrip(triangles, segment, thickness, color);
            target.draw(triangles);
        }
        segment.clear();
    };
    for (std::size_t i = 0; i + 1 < pts.size(); ++i) {
        Vec2 a = pts[i];
        const Vec2 b = pts[i + 1];
        Vec2 delta = b - a;
        float remain = std::hypot(delta.x, delta.y);
        if (remain < 1e-4f) {
            continue;
        }
        delta = {delta.x / remain, delta.y / remain};
        while (remain > 1e-4f) {
            const float limit = drawing ? on : off;
            const float step = std::min(remain, limit - phase);
            a = a + delta * step;
            remain -= step;
            phase += step;
            if (drawing) {
                segment.push_back(a);
            }
            if (phase >= limit - 1e-4f) {
                if (drawing) {
                    flush();
                }
                drawing = !drawing;
                phase = 0.f;
                if (drawing) {
                    segment.push_back(a);
                }
            }
        }
    }
    if (drawing) {
        flush();
    }
}

enum class Baseline { Alphabetic, Middle };

void drawLabel(sf::RenderTarget &target, const sf::Font &font, std::string_view text, float size, Vec2 pos,
               sf::Color color, TextAlign align, Baseline baseline, bool bold) {
    const long rounded = std::lround(size * kMapPixelsPerMeter);
    const unsigned pixels = rounded > 1 ? static_cast<unsigned>(rounded) : 1;
    sf::Text shape(font, utf8(text), pixels);
    if (bold) {
        shape.setStyle(sf::Text::Bold);
    }
    shape.setFillColor(color);
    shape.setScale({1.f / kMapPixelsPerMeter, 1.f / kMapPixelsPerMeter});
    const sf::FloatRect bounds = shape.getLocalBounds();
    float originX = bounds.position.x;
    if (align == TextAlign::Center) {
        originX = bounds.position.x + bounds.size.x * 0.5f;
    }
    float originY = static_cast<float>(shape.getCharacterSize());
    if (baseline == Baseline::Middle) {
        originY = bounds.position.y + bounds.size.y * 0.5f;
    }
    shape.setOrigin({originX, originY});
    shape.setPosition({pos.x, pos.y});
    target.draw(shape);
}

void drawDashedRect(sf::RenderTarget &target, const Aabb &box, float on, float off, float thickness, sf::Color color) {
    const std::vector<Vec2> pts = {
        {box.x, box.y}, {box.x + box.w, box.y}, {box.x + box.w, box.y + box.h}, {box.x, box.y + box.h}, {box.x, box.y},
    };
    drawDashed(target, pts, on, off, thickness, color);
}

} // namespace

void MapView::build(const ScenarioDef &scenario, const StringTable &strings, const sf::Font &titleFont,
                    bool titleBold) {
    map_ = scenario.map;
    exit_ = scenario.exit.rect;
    exitLabel_ = strings.text("ui.exit");
    titleFont_ = &titleFont;
    titleBold_ = titleBold;
    blob_ = makeBlob();
    const auto size = sf::Vector2u{static_cast<unsigned>(std::ceil(map_.width * kMapPixelsPerMeter)),
                                   static_cast<unsigned>(std::ceil(map_.height * kMapPixelsPerMeter))};
    if (!base_.resize(size) || !scars_.resize(size)) {
        throw std::runtime_error("无法创建底图纹理");
    }
    base_.setSmooth(true);
    scars_.setSmooth(true);
    const sf::View world({map_.width * 0.5f, map_.height * 0.5f}, {map_.width, map_.height});
    base_.setView(world);
    scars_.setView(world);
    scars_.clear(sf::Color::Transparent);
    scars_.display();
    paintBase(strings, titleFont, titleBold);
}

void MapView::paintBase(const StringTable &strings, const sf::Font &titleFont, bool titleBold) {
    const float w = map_.width;
    const float h = map_.height;
    MapRng rng(1593);

    sf::VertexArray silk(sf::PrimitiveType::Triangles, 6);
    const sf::Color c0{0xeb, 0xdf, 0xc2};
    const sf::Color c1{0xdd, 0xcd, 0xa9};
    silk[0] = {{0.f, 0.f}, c0};
    silk[1] = {{w, 0.f}, rgba(0xe4, 0xd6, 0xb6, 1.f)};
    silk[2] = {{w, h}, c1};
    silk[3] = {{0.f, 0.f}, c0};
    silk[4] = {{w, h}, c1};
    silk[5] = {{0.f, h}, rgba(0xe4, 0xd6, 0xb6, 1.f)};
    base_.clear(c0);
    base_.draw(silk);

    for (int i = 0; i < 16; ++i) {
        const float x = rng() * w;
        const float y = rng() * h;
        const float r = 60.f + rng() * 190.f;
        drawBlob(base_, blob_, {x, y}, r, rgba(140, 105, 60, 0.07f));
    }

    sf::VertexArray grain(sf::PrimitiveType::Lines);
    for (float y = 0.f; y < h; y += 3.f) {
        const sf::Color color = rgba(120, 95, 60, 0.025f + rng() * 0.04f);
        appendLine(grain, {0.f, y + rng()}, {w, y + rng()}, color);
    }
    for (float x = 0.f; x < w; x += 4.f) {
        const sf::Color color = rgba(255, 250, 235, 0.03f + rng() * 0.05f);
        appendLine(grain, {x, 0.f}, {x + rng() * 2.f, h}, color);
    }
    base_.draw(grain);

    for (const Aabb &paddy : map_.paddies) {
        const auto jitter = [&] { return (rng() - 0.5f) * 8.f; };
        sf::ConvexShape quad(4);
        quad.setPoint(0, {paddy.x + jitter(), paddy.y + jitter()});
        quad.setPoint(1, {paddy.x + paddy.w + jitter(), paddy.y + jitter()});
        quad.setPoint(2, {paddy.x + paddy.w + jitter(), paddy.y + paddy.h + jitter()});
        quad.setPoint(3, {paddy.x + jitter(), paddy.y + paddy.h + jitter()});
        quad.setFillColor(rgba(138, 158, 118, 0.36f));
        quad.setOutlineThickness(0.f);
        base_.draw(quad);

        sf::VertexArray bunds(sf::PrimitiveType::Lines);
        const sf::Color bund = rgba(78, 88, 54, 0.42f);
        for (float gx = paddy.x + 16.f; gx < paddy.x + paddy.w; gx += 22.f + rng() * 8.f) {
            appendLine(bunds, {gx, paddy.y - 4.f}, {gx + (rng() - 0.5f) * 6.f, paddy.y + paddy.h + 4.f}, bund);
        }
        for (float gy = paddy.y + 14.f; gy < paddy.y + paddy.h; gy += 20.f + rng() * 8.f) {
            appendLine(bunds, {paddy.x - 4.f, gy}, {paddy.x + paddy.w + 4.f, gy + (rng() - 0.5f) * 6.f}, bund);
        }
        const sf::Color glint = rgba(250, 248, 232, 0.4f);
        const int glints = static_cast<int>(paddy.w * paddy.h / 450.f);
        for (int i = 0; i < glints; ++i) {
            const float px = paddy.x + rng() * paddy.w;
            const float py = paddy.y + rng() * paddy.h;
            appendLine(bunds, {px, py}, {px + 4.f + rng() * 5.f, py}, glint);
        }
        base_.draw(bunds);
        quad.setFillColor(sf::Color::Transparent);
        quad.setOutlineColor(rgba(60, 66, 44, 0.5f));
        quad.setOutlineThickness(1.f);
        base_.draw(quad);
    }

    for (const Ellipse &hill : map_.hills) {
        sf::RenderStates states;
        states.transform.translate({hill.cx, hill.cy});
        states.transform.scale({1.f, hill.ry / hill.rx});
        {
            sf::Sprite sprite(blob_);
            const sf::Vector2u size = blob_.getSize();
            sprite.setOrigin({size.x * 0.5f, size.y * 0.5f});
            sprite.setPosition({0.f, -hill.rx * 0.2f});
            sprite.setScale(
                {(2.f * hill.rx) / static_cast<float>(size.x), (2.f * hill.rx) / static_cast<float>(size.y)});
            sprite.setColor(rgba(70, 120, 96, 0.44f));
            base_.draw(sprite, states);
            sprite.setPosition({0.f, -hill.rx * 0.25f});
            sprite.setScale({(2.f * hill.rx * 0.55f) / static_cast<float>(size.x),
                             (2.f * hill.rx * 0.55f) / static_cast<float>(size.y)});
            sprite.setColor(rgba(50, 94, 126, 0.32f));
            base_.draw(sprite, states);
        }
        for (const float k : {0.95f, 0.72f, 0.48f}) {
            const sf::Color stroke = rgba(42, 34, 27, 0.2f + 0.12f * (1.f - k));
            sf::VertexArray contour(sf::PrimitiveType::Lines);
            float a = rng() * kPi * 2.f;
            const float end = a + kPi * 2.f;
            while (a < end) {
                const float len = 0.4f + rng() * 0.9f;
                sf::Vector2f prev{};
                bool first = true;
                for (float t = a; t < std::min(end, a + len); t += 0.05f) {
                    const float jj = 1.f + (rng() - 0.5f) * 0.03f;
                    const sf::Vector2f p{hill.cx + std::cos(t) * hill.rx * k * jj,
                                         hill.cy + std::sin(t) * hill.ry * k * jj};
                    if (!first) {
                        appendLine(contour, prev, p, stroke);
                    }
                    prev = p;
                    first = false;
                }
                a += len + 0.12f + rng() * 0.3f;
            }
            base_.draw(contour);
        }
        sf::VertexArray ticks(sf::PrimitiveType::Lines);
        const int n = static_cast<int>(std::floor(hill.rx * hill.ry / 650.f));
        const sf::Color tick = rgba(42, 34, 27, 0.16f);
        for (int i = 0; i < n; ++i) {
            const float t = rng() * kPi * 2.f;
            const float rr = std::sqrt(rng()) * 0.9f;
            const float x = hill.cx + std::cos(t) * hill.rx * rr;
            const float y = hill.cy + std::sin(t) * hill.ry * rr;
            appendLine(ticks, {x, y}, {x - 4.f - rng() * 5.f, y + 3.f + rng() * 4.f}, tick);
        }
        base_.draw(ticks);
    }

    const std::vector<Vec2> road = smoothRoad(map_.road.points);
    {
        sf::VertexArray band(sf::PrimitiveType::Triangles);
        appendStrip(band, road, 22.f, rgba(168, 126, 72, 0.34f));
        base_.draw(band);
        drawDashed(base_, road, 6.f, 7.f, 1.f, rgba(60, 45, 30, 0.38f));
    }

    {
        const Vec2 s = map_.postStation;
        sf::RectangleShape shadow({26.f, 12.f});
        shadow.setPosition({s.x - 13.f, s.y - 2.f});
        shadow.setFillColor(rgba(42, 34, 27, 0.12f));
        shadow.setOutlineColor(rgba(42, 34, 27, 0.75f));
        shadow.setOutlineThickness(1.3f);
        base_.draw(shadow);
        sf::VertexArray roof(sf::PrimitiveType::LineStrip);
        const sf::Color roofInk = rgba(42, 34, 27, 0.75f);
        const Vec2 left{-19.f, -1.f};
        const Vec2 leftCtrl{-12.f, -4.f};
        const Vec2 ridgeL{-9.f, -11.f};
        const Vec2 ridgeR{9.f, -11.f};
        const Vec2 rightCtrl{12.f, -4.f};
        const Vec2 right{19.f, -1.f};
        for (int i = 0; i <= 8; ++i) {
            const Vec2 p = quadratic(left, leftCtrl, ridgeL, static_cast<float>(i) / 8.f);
            roof.append({{s.x + p.x, s.y + p.y}, roofInk});
        }
        roof.append({{s.x + ridgeR.x, s.y + ridgeR.y}, roofInk});
        for (int i = 1; i <= 8; ++i) {
            const Vec2 p = quadratic(ridgeR, rightCtrl, right, static_cast<float>(i) / 8.f);
            roof.append({{s.x + p.x, s.y + p.y}, roofInk});
        }
        base_.draw(roof);
        sf::RectangleShape door({6.f, 7.f});
        door.setPosition({s.x - 3.f, s.y + 3.f});
        door.setFillColor(rgba(42, 34, 27, 0.7f));
        base_.draw(door);
    }

    const sf::Color ink = rgba(42, 34, 27, 0.82f);
    const sf::Color faint = rgba(42, 34, 27, 0.55f);
    for (const MapLabel &label : map_.labels) {
        drawLabel(base_, titleFont, strings.text(label.key), label.size, label.pos, label.faint ? faint : ink,
                  label.align, Baseline::Alphabetic, titleBold);
    }

    sf::RectangleShape seal({38.f, 38.f});
    seal.setPosition({22.f, 22.f});
    seal.setFillColor(sf::Color(0xa8, 0x38, 0x2a));
    base_.draw(seal);
    sf::RectangleShape ring({30.f, 30.f});
    ring.setPosition({26.f, 26.f});
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineColor(rgba(236, 226, 200, 0.85f));
    ring.setOutlineThickness(1.5f);
    base_.draw(ring);
    drawLabel(base_, titleFont, strings.text("ui.north"), 28.f, {41.f, 42.f}, sf::Color(0xef, 0xe5, 0xcb),
              TextAlign::Center, Baseline::Middle, titleBold);

    sf::VertexArray scale(sf::PrimitiveType::Lines);
    const sf::Color scaleInk = rgba(42, 34, 27, 0.8f);
    appendLine(scale, {30.f, 1070.f}, {190.f, 1070.f}, scaleInk);
    appendLine(scale, {30.f, 1064.f}, {30.f, 1076.f}, scaleInk);
    appendLine(scale, {110.f, 1066.f}, {110.f, 1074.f}, scaleInk);
    appendLine(scale, {190.f, 1064.f}, {190.f, 1076.f}, scaleInk);
    base_.draw(scale);
    drawLabel(base_, titleFont, strings.text("ui.scale"), 18.f, {198.f, 1076.f}, ink, TextAlign::Left,
              Baseline::Alphabetic, titleBold);
    base_.display();
    if (!base_.generateMipmap()) {
        std::cerr << "底图 mipmap 生成失败\n";
    }
}

void MapView::addScar(Vec2 pos, float ratio) {
    std::uniform_real_distribution<float> unit(0.f, 1.f);
    for (int i = 0; i < 4; ++i) {
        const float px = pos.x + (unit(scarRng_) - 0.5f) * 20.f;
        const float py = pos.y + (unit(scarRng_) - 0.5f) * 14.f;
        const float rr = 6.f + unit(scarRng_) * 8.f * std::min(2.f, ratio);
        drawBlob(scars_, blob_, {px, py}, rr, rgba(92, 62, 36, 0.13f));
    }
    scars_.display();
}

void MapView::drawExit(Window &window, float timeSec) const {
    const float a = 0.5f + 0.5f * std::sin(timeSec * 3.f);
    sf::RectangleShape fill({exit_.w, exit_.h});
    fill.setPosition({exit_.x, exit_.y});
    fill.setFillColor(rgba(207, 170, 92, 0.14f + 0.1f * a));
    window.draw(fill);
    const ScreenRect field = battlefieldRect();
    const float sc = mapScale(field.w, field.h, map_.width, map_.height);
    Aabb inner{exit_.x + 1.f, exit_.y + 1.f, exit_.w - 2.f, exit_.h - 2.f};
    drawDashedRect(window.target(), inner, 8.f / sc, 5.f / sc, 1.6f / sc, rgba(156, 124, 63, 0.9f));
    if (titleFont_) {
        drawLabel(window.target(), *titleFont_, exitLabel_, 24.f,
                  {exit_.x + exit_.w * 0.5f, exit_.y + exit_.h * 0.5f + 2.f}, rgba(42, 34, 27, 0.85f),
                  TextAlign::Center, Baseline::Middle, titleBold_);
    }
}

void MapView::draw(Window &window, float timeSec, bool exitOpen) const {
    const ScreenRect field = battlefieldRect();
    const float sc = mapScale(field.w, field.h, map_.width, map_.height);
    sf::RectangleShape ground({map_.width + 18.f, map_.height + 18.f});
    ground.setPosition({-9.f, -9.f});
    ground.setFillColor(sf::Color(0x10, 0x0a, 0x08));
    window.draw(ground);

    sf::RectangleShape frame({map_.width + 10.f, map_.height + 10.f});
    frame.setPosition({-5.f, -5.f});
    frame.setFillColor(sf::Color::Transparent);
    frame.setOutlineColor(sf::Color(0x9c, 0x7c, 0x3f));
    frame.setOutlineThickness(1.2f / sc);
    window.draw(frame);

    sf::Sprite mapSprite(base_.getTexture());
    mapSprite.setScale({1.f / kMapPixelsPerMeter, 1.f / kMapPixelsPerMeter});
    window.draw(mapSprite);
    sf::Sprite scarSprite(scars_.getTexture());
    scarSprite.setScale({1.f / kMapPixelsPerMeter, 1.f / kMapPixelsPerMeter});
    window.draw(scarSprite);

    if (exitOpen) {
        drawExit(window, timeSec);
    }
}

} // namespace tq
