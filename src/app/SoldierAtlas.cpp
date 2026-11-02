#include "app/SoldierAtlas.h"

#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <vector>

namespace tq {
namespace {

constexpr float kPi = std::numbers::pi_v<float>;
constexpr unsigned kAtlasWidth = kAtlasCellPx * kAtlasFrames;
constexpr unsigned kAtlasHeight = kAtlasCellPx * kAtlasRows;
// Placeholder figures are drawn in cell pixels and outlined in dark ink.
constexpr float kOutline = 0.8f;
const sf::Color kInk{0x1a, 0x12, 0x0c, 230};

struct Palette {
    sf::Color coat;
    sf::Color trim;
    sf::Color helmet;
    sf::Color skin{0xd8, 0xb4, 0x8a};
    sf::Color horse{0x6b, 0x4a, 0x2f};
    sf::Color horseDark{0x3d, 0x2a, 0x1c};
    sf::Color metal{0x30, 0x2c, 0x28};
    sf::Color wood{0x5a, 0x42, 0x2a};
    sf::Color flag;
};

Palette palette(Side side, bool general) {
    Palette p;
    if (side == Side::Ming) {
        p.coat = general ? sf::Color{0xd9, 0xb4, 0x66} : sf::Color{0xa8, 0x38, 0x2a};
        p.trim = general ? sf::Color{0xa8, 0x38, 0x2a} : sf::Color{0xd9, 0xb4, 0x66};
        p.helmet = sf::Color{0x3a, 0x2a, 0x22};
        p.flag = sf::Color{0xc2, 0x3b, 0x2a};
    } else {
        p.coat = general ? sf::Color{0xef, 0xe5, 0xcb} : sf::Color{0x2f, 0x3a, 0x56};
        p.trim = general ? sf::Color{0x2f, 0x3a, 0x56} : sf::Color{0xaa, 0xb4, 0xc9};
        p.helmet = sf::Color{0x1b, 0x1b, 0x1f};
        p.flag = sf::Color{0xef, 0xe5, 0xcb};
    }
    return p;
}

// A point in the figure's own frame: f forward, r to the right, z up, in cell pixels.
struct Local {
    float f = 0.f;
    float r = 0.f;
    float z = 0.f;
};

struct Part {
    enum class Shape : std::uint8_t { Disc, Bar } shape = Shape::Disc;
    Local a{};
    Local b{};
    float size = 1.f;
    sf::Color color{};
    bool outline = true;
};

class Painter {
  public:
    Painter(sf::Vector2f anchor, int direction) : anchor_(anchor) {
        const float angle = static_cast<float>(direction) * kPi * 0.25f;
        // Oblique view: the ground plane is squashed to half height, up is straight up the screen.
        forward_ = {std::cos(angle), std::sin(angle) * 0.5f};
        right_ = {-std::sin(angle), std::cos(angle) * 0.5f};
    }

    sf::Vector2f at(Local p) const {
        return {anchor_.x + forward_.x * p.f + right_.x * p.r, anchor_.y + forward_.y * p.f + right_.y * p.r - p.z};
    }

    // Parts nearer the viewer (lower on screen at the same height) are drawn later.
    float depth(const Part &part) const {
        const Local mid{(part.a.f + part.b.f) * 0.5f, (part.a.r + part.b.r) * 0.5f, 0.f};
        return forward_.y * mid.f + right_.y * mid.r;
    }

    void paint(sf::VertexArray &tris, std::vector<Part> parts) const {
        std::stable_sort(parts.begin(), parts.end(),
                         [this](const Part &left, const Part &right) { return depth(left) < depth(right); });
        for (const Part &part : parts) {
            if (part.outline) {
                emit(tris, part, kOutline, kInk);
            }
        }
        for (const Part &part : parts) {
            emit(tris, part, 0.f, part.color);
        }
    }

  private:
    void emit(sf::VertexArray &tris, const Part &part, float grow, sf::Color color) const {
        if (part.shape == Part::Shape::Disc) {
            disc(tris, at(part.a), part.size + grow, color);
            return;
        }
        bar(tris, at(part.a), at(part.b), part.size + grow * 2.f, color);
        disc(tris, at(part.a), part.size * 0.5f + grow, color);
        disc(tris, at(part.b), part.size * 0.5f + grow, color);
    }

    static void disc(sf::VertexArray &tris, sf::Vector2f c, float radius, sf::Color color) {
        constexpr int kSteps = 12;
        for (int i = 0; i < kSteps; ++i) {
            const float a0 = 2.f * kPi * static_cast<float>(i) / kSteps;
            const float a1 = 2.f * kPi * static_cast<float>(i + 1) / kSteps;
            tris.append({c, color});
            tris.append({{c.x + std::cos(a0) * radius, c.y + std::sin(a0) * radius}, color});
            tris.append({{c.x + std::cos(a1) * radius, c.y + std::sin(a1) * radius}, color});
        }
    }

    static void bar(sf::VertexArray &tris, sf::Vector2f a, sf::Vector2f b, float width, sf::Color color) {
        sf::Vector2f d = b - a;
        const float length = std::hypot(d.x, d.y);
        if (length < 1e-3f) {
            return;
        }
        d /= length;
        const sf::Vector2f n{-d.y * width * 0.5f, d.x * width * 0.5f};
        tris.append({a + n, color});
        tris.append({a - n, color});
        tris.append({b - n, color});
        tris.append({a + n, color});
        tris.append({b - n, color});
        tris.append({b + n, color});
    }

    sf::Vector2f anchor_;
    sf::Vector2f forward_;
    sf::Vector2f right_;
};

Part disc(Local at, float radius, sf::Color color, bool outline = true) {
    return {Part::Shape::Disc, at, at, radius, color, outline};
}

Part bar(Local a, Local b, float width, sf::Color color, bool outline = true) {
    return {Part::Shape::Bar, a, b, width, color, outline};
}

// Gait offset of a leg for a frame: zero when standing, a swing through the move cycle otherwise.
float stride(int frame, float legPhase, float reach) {
    if (frame < kFrameMove || frame >= kFrameMove + kMoveFrames) {
        return frame >= kFrameStrike && frame < kFrameStrike + 2 && legPhase > 0.f ? reach * 0.5f : 0.f;
    }
    const float phase = static_cast<float>(frame - kFrameMove) / static_cast<float>(kMoveFrames) * 2.f * kPi;
    return std::sin(phase + legPhase) * reach;
}

bool striking(int frame) { return frame == kFrameStrike || frame == kFrameStrike + 1; }

void rider(std::vector<Part> &parts, const Palette &p, int frame, bool general, Side side) {
    const float lean = frame >= kFrameMove && frame < kFrameMove + kMoveFrames ? 0.8f : 0.f;
    const float aside = frame == kFrameAside ? 0.9f : 0.f;
    parts.push_back(bar({0.f, 0.f, 11.f}, {0.6f + lean, 0.f, 17.f}, 3.6f, p.coat));
    parts.push_back(bar({0.f, -1.6f, 12.f}, {0.6f + lean, -1.6f, 16.f}, 1.f, p.trim, false));
    parts.push_back(disc({0.8f + lean, aside, 19.f}, 1.9f, p.skin));
    parts.push_back(disc({0.6f + lean, aside, 20.f}, 1.7f, p.helmet, false));
    const bool thrust = striking(frame);
    const float reach = frame == kFrameStrike + 1 ? 13.f : 10.f;
    if (side == Side::Ming) {
        // Lance with a three-eyed gun slung; held level when striking.
        parts.push_back(thrust ? bar({1.f, 2.f, 14.f}, {reach, 2.2f, 14.5f}, 1.f, p.metal)
                               : bar({-3.f, 2.f, 10.f}, {6.f, 2.4f, 24.f}, 1.f, p.wood));
    } else {
        parts.push_back(thrust ? bar({1.f, 2.f, 15.f}, {reach - 3.f, 2.2f, 12.f}, 1.2f, p.metal)
                               : bar({1.f, 2.f, 14.f}, {4.f, 2.4f, 19.f}, 1.2f, p.metal));
    }
    if (general) {
        parts.push_back(bar({-1.5f, 0.f, 14.f}, {-1.5f, 0.f, 24.5f}, 0.8f, p.wood));
        parts.push_back(bar({-1.5f, 0.f, 20.5f}, {-1.5f, 0.f, 24.f}, 3.2f, p.flag));
    }
}

void horse(std::vector<Part> &parts, const Palette &p, int frame) {
    const bool running = frame >= kFrameMove && frame < kFrameMove + kMoveFrames;
    const float headDrop = running ? 1.5f : 0.f;
    const float legs[4][3] = {{4.f, -1.6f, 0.f}, {4.f, 1.6f, kPi}, {-4.f, -1.6f, kPi}, {-4.f, 1.6f, 0.f}};
    for (const auto &leg : legs) {
        const float swing = stride(frame, leg[2], 3.f);
        parts.push_back(bar({leg[0], leg[1], 7.f}, {leg[0] + swing, leg[1], 0.5f}, 1.4f, p.horseDark));
    }
    parts.push_back(disc({-4.f, 0.f, 8.f}, 3.3f, p.horse));
    parts.push_back(disc({0.f, 0.f, 8.4f}, 3.5f, p.horse));
    parts.push_back(disc({4.f, 0.f, 8.6f}, 3.2f, p.horse));
    parts.push_back(bar({5.f, 0.f, 9.5f}, {7.5f, 0.f, 13.f - headDrop}, 2.6f, p.horse));
    parts.push_back(disc({8.8f, 0.f, 12.5f - headDrop}, 1.9f, p.horse));
    parts.push_back(bar({-6.8f, 0.f, 9.f}, {-8.8f, 0.f, 4.5f}, 1.2f, p.horseDark));
    parts.push_back(bar({-1.5f, -2.6f, 8.f}, {1.5f, -2.6f, 8.f}, 1.2f, p.trim, false));
}

void footman(std::vector<Part> &parts, const Palette &p, int frame, SoldierKind kind, Side side) {
    const float legs[2][2] = {{-1.f, 0.f}, {1.f, kPi}};
    for (const auto &leg : legs) {
        const float swing = stride(frame, leg[1], 2.6f);
        parts.push_back(bar({0.f, leg[0], 7.f}, {swing, leg[0], 0.5f}, 1.6f, p.helmet));
    }
    const float lean = frame >= kFrameMove && frame < kFrameMove + kMoveFrames ? 0.7f : 0.f;
    const float aside = frame == kFrameAside ? 0.9f : 0.f;
    parts.push_back(bar({0.f, 0.f, 7.f}, {lean, 0.f, 13.f}, 3.8f, p.coat));
    parts.push_back(bar({0.f, -1.7f, 8.f}, {lean, -1.7f, 12.f}, 1.f, p.trim, false));
    parts.push_back(disc({lean + 0.2f, aside, 15.f}, 1.8f, p.skin));
    const bool thrust = striking(frame);
    switch (kind) {
    case SoldierKind::Spear: {
        // Jingasa, and a long spear: upright at rest, levelled to fight.
        parts.push_back(bar({lean, -2.8f + aside, 16.2f}, {lean, 2.8f + aside, 16.2f}, 1.4f, p.helmet));
        const float reach = frame == kFrameStrike + 1 ? 19.f : 16.f;
        parts.push_back(thrust ? bar({-4.f, 1.6f, 11.f}, {reach, 1.6f, 11.5f}, 1.f, p.wood)
                               : bar({-2.f, 1.6f, 3.f}, {5.f, 1.6f, 24.5f}, 1.f, p.wood));
        break;
    }
    case SoldierKind::Gun: {
        parts.push_back(bar({lean, -2.6f + aside, 16.f}, {lean, 2.6f + aside, 16.f}, 1.3f, p.helmet));
        if (thrust) {
            parts.push_back(bar({-1.f, 1.2f, 12.5f}, {10.f, 1.2f, 12.5f}, 1.2f, p.metal));
            if (frame == kFrameStrike + 1) {
                parts.push_back(disc({11.5f, 1.2f, 12.5f}, 1.6f, sf::Color{0xf2, 0xc6, 0x6a}, false));
            }
        } else {
            parts.push_back(bar({-1.f, 1.4f, 9.f}, {3.f, 1.4f, 17.f}, 1.2f, p.metal));
        }
        break;
    }
    case SoldierKind::Samurai:
    case SoldierKind::Cavalry:
    case SoldierKind::General: {
        parts.push_back(disc({lean + 0.1f, aside, 16.f}, 1.7f, p.helmet, false));
        parts.push_back(bar({lean - 0.6f, aside, 17.2f}, {lean + 0.9f, aside, 18.6f}, 0.7f, p.trim, false));
        parts.push_back(thrust ? bar({1.f, 1.8f, 12.f}, {frame == kFrameStrike ? 6.f : 8.f, 1.8f, 8.f}, 1.f, p.metal)
                               : bar({1.f, 1.8f, 12.f}, {3.f, 1.8f, 19.f}, 1.f, p.metal));
        break;
    }
    }
    if (side == Side::Jp) {
        // Sashimono on the back.
        parts.push_back(bar({-1.6f, 0.f, 11.f}, {-1.6f, 0.f, 21.f}, 0.6f, p.wood, false));
        parts.push_back(bar({-1.6f, 0.f, 18.f}, {-1.6f, 0.f, 21.5f}, 2.6f, p.flag));
    }
}

void figure(sf::VertexArray &tris, sf::Vector2f anchor, Side side, SoldierKind kind, int direction, int frame) {
    const bool general = kind == SoldierKind::General;
    const Palette p = palette(side, general);
    std::vector<Part> parts;
    if (kind == SoldierKind::Cavalry || general) {
        horse(parts, p, frame);
        rider(parts, p, frame, general, side);
    } else {
        footman(parts, p, frame, kind, side);
    }
    // Ground shadow first, without outline.
    std::vector<Part> shadow{disc({0.f, 0.f, 0.f}, kind == SoldierKind::Cavalry || general ? 5.f : 3.f,
                                  sf::Color{0x20, 0x16, 0x10, 70}, false)};
    const Painter painter(anchor, direction);
    painter.paint(tris, shadow);
    painter.paint(tris, parts);
}

void flatRect(sf::VertexArray &tris, sf::FloatRect box, sf::Color color) {
    const sf::Vector2f a = box.position;
    const sf::Vector2f c = box.position + box.size;
    tris.append({a, color});
    tris.append({{c.x, a.y}, color});
    tris.append({c, color});
    tris.append({a, color});
    tris.append({c, color});
    tris.append({{a.x, c.y}, color});
}

void flatDisc(sf::VertexArray &tris, sf::Vector2f center, float radius, sf::Color color) {
    constexpr int kSteps = 24;
    for (int i = 0; i < kSteps; ++i) {
        const float a0 = 2.f * kPi * static_cast<float>(i) / kSteps;
        const float a1 = 2.f * kPi * static_cast<float>(i + 1) / kSteps;
        tris.append({center, color});
        tris.append({{center.x + std::cos(a0) * radius, center.y + std::sin(a0) * radius}, color});
        tris.append({{center.x + std::cos(a1) * radius, center.y + std::sin(a1) * radius}, color});
    }
}

void flatRing(sf::VertexArray &tris, sf::Vector2f center, float radius, float width, sf::Color color) {
    constexpr int kSteps = 32;
    for (int i = 0; i < kSteps; ++i) {
        const float a0 = 2.f * kPi * static_cast<float>(i) / kSteps;
        const float a1 = 2.f * kPi * static_cast<float>(i + 1) / kSteps;
        const sf::Vector2f o0{std::cos(a0), std::sin(a0)};
        const sf::Vector2f o1{std::cos(a1), std::sin(a1)};
        const float inner = radius - width;
        tris.append({center + o0 * inner, color});
        tris.append({center + o0 * radius, color});
        tris.append({center + o1 * radius, color});
        tris.append({center + o0 * inner, color});
        tris.append({center + o1 * radius, color});
        tris.append({center + o1 * inner, color});
    }
}

// The extras row: a dust puff and three placeholder banner cloths, each filling its cell.
void extras(sf::VertexArray &tris, float top) {
    const float cell = static_cast<float>(kAtlasCellPx);
    const sf::Vector2f dust{cell * (kAtlasDustColumn + 0.5f), top + cell * 0.5f};
    for (int ring = 0; ring < 8; ++ring) {
        const float radius = 14.f - static_cast<float>(ring) * 1.6f;
        flatDisc(tris, dust, radius, sf::Color{0xb8, 0xa0, 0x7a, static_cast<std::uint8_t>(18 + ring * 6)});
    }

    // Ming unit banner: vermilion with a gold border; the character is drawn over it at run time.
    const float ming = cell * kAtlasMingBannerColumn;
    flatRect(tris, {{ming, top}, {cell, cell}}, sf::Color{0xd9, 0xb4, 0x66});
    flatRect(tris, {{ming + 2.f, top + 2.f}, {cell - 4.f, cell - 4.f}}, sf::Color{0xb0, 0x30, 0x24});
    flatRect(tris, {{ming + 2.f, top + 2.f}, {cell - 4.f, 6.f}}, sf::Color{0xc2, 0x3b, 0x2a});

    // Japanese sashimono: off-white with a black placeholder crest. Vanguard (Tachibana) and main body (Kobayakawa).
    const sf::Color cloth{0xef, 0xe5, 0xcb};
    const sf::Color crest{0x1b, 0x1b, 0x1f};
    const float van = cell * kAtlasJpVanguardBannerColumn;
    flatRect(tris, {{van, top}, {cell, cell}}, cloth);
    flatRing(tris, {van + cell * 0.5f, top + cell * 0.45f}, 9.f, 2.2f, crest);
    flatRect(tris, {{van + cell * 0.5f - 1.2f, top + cell * 0.45f - 9.f}, {2.4f, 18.f}}, crest);
    flatRect(tris, {{van + cell * 0.5f - 9.f, top + cell * 0.45f - 1.2f}, {18.f, 2.4f}}, crest);
    const float main = cell * kAtlasJpMainBannerColumn;
    flatRect(tris, {{main, top}, {cell, cell}}, cloth);
    for (int i = 0; i < 3; ++i) {
        const float angle = -kPi * 0.5f + static_cast<float>(i) * 2.f * kPi / 3.f;
        const sf::Vector2f at{main + cell * 0.5f + std::cos(angle) * 5.5f, top + cell * 0.47f + std::sin(angle) * 5.5f};
        flatDisc(tris, at, 4.2f, crest);
        flatDisc(tris, at + sf::Vector2f{std::cos(angle + 1.6f), std::sin(angle + 1.6f)} * 2.6f, 2.2f, crest);
    }
}

} // namespace

sf::IntRect SoldierAtlas::cell(int row, int column) {
    return {{column * kAtlasCellPx, row * kAtlasCellPx}, {kAtlasCellPx, kAtlasCellPx}};
}

sf::Image SoldierAtlas::placeholder() {
    sf::RenderTexture canvas;
    if (!canvas.resize({kAtlasWidth, kAtlasHeight})) {
        throw std::runtime_error("could not create the soldier atlas");
    }
    canvas.clear(sf::Color::Transparent);
    sf::VertexArray tris(sf::PrimitiveType::Triangles);
    for (Side side : {Side::Ming, Side::Jp}) {
        for (int kind = 0; kind < kSoldierKinds; ++kind) {
            for (int direction = 0; direction < kAtlasDirections; ++direction) {
                const int row = atlasRow(side, static_cast<SoldierKind>(kind), direction);
                for (int frame = 0; frame < kAtlasFrames; ++frame) {
                    const sf::Vector2f anchor{static_cast<float>(frame * kAtlasCellPx) + kAtlasAnchorX,
                                              static_cast<float>(row * kAtlasCellPx) + kAtlasAnchorY};
                    figure(tris, anchor, side, static_cast<SoldierKind>(kind), direction, frame);
                }
            }
        }
    }
    extras(tris, static_cast<float>(kAtlasExtraRow * kAtlasCellPx));
    canvas.draw(tris);
    canvas.display();
    return canvas.getTexture().copyToImage();
}

void SoldierAtlas::load(const std::filesystem::path &assetRoot) {
    const std::filesystem::path file = assetRoot / "textures" / "soldiers" / "soldiers.png";
    bool loaded = false;
    std::error_code error;
    if (std::filesystem::exists(file, error)) {
        sf::Image image;
        if (image.loadFromFile(file) && image.getSize() == sf::Vector2u{kAtlasWidth, kAtlasHeight}) {
            loaded = texture_.loadFromImage(image);
        } else {
            std::cerr << "soldier atlas " << file.string() << " must be " << kAtlasWidth << "x" << kAtlasHeight
                      << "; using the placeholder\n";
        }
    }
    if (!loaded && !texture_.loadFromImage(placeholder())) {
        throw std::runtime_error("could not upload the soldier atlas");
    }
    texture_.setSmooth(true);
    if (!texture_.generateMipmap()) {
        std::cerr << "soldier atlas mipmap failed\n";
    }
}

} // namespace tq
