#include "app/MapDetail.h"
#include "app/Viewport.h"
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace tq {
namespace {
bool overlaps(Aabb a, Aabb b) {
    return b.w > 0.f && b.h > 0.f && a.x <= b.x + b.w && a.x + a.w >= b.x && a.y <= b.y + b.h && a.y + a.h >= b.y;
}
Aabb grow(Aabb b, float n) { return {b.x - n, b.y - n, b.w + 2.f * n, b.h + 2.f * n}; }
float roadDistance(const MapDef &map, Vec2 p) {
    float distance = map.width + map.height;
    for (std::size_t i = 1; i < map.road.points.size(); ++i) {
        const Vec2 a = map.road.points[i - 1], d = map.road.points[i] - a;
        const float len = dot(d, d);
        const float t = len > 0.f ? std::clamp(dot(p - a, d) / len, 0.f, 1.f) : 0.f;
        const Vec2 delta = p - (a + d * t);
        distance = std::min(distance, std::hypot(delta.x, delta.y));
    }
    return distance;
}
bool onHill(const MapDef &map, Vec2 p) {
    for (const auto &h : map.hills) {
        if (h.rx <= 0.f || h.ry <= 0.f)
            continue;
        const float x = (p.x - h.cx) / h.rx, y = (p.y - h.cy) / h.ry;
        if (x * x + y * y <= 1.f)
            return true;
    }
    return false;
}
void quad(sf::VertexArray &out, const std::array<Vec2, 4> &points, sf::Color color, float texScale = 0.f) {
    for (const unsigned i : {0u, 1u, 2u, 0u, 2u, 3u}) {
        const auto p = points[i];
        out.append({{p.x, p.y}, color, {p.x * texScale, p.y * texScale}});
    }
}
void band(sf::VertexArray &out, Vec2 a, Vec2 b, float width, sf::Color color) {
    const Vec2 d = b - a;
    const float length = std::hypot(d.x, d.y);
    if (length < 0.001f)
        return;
    const Vec2 n{-d.y / length * width * 0.5f, d.x / length * width * 0.5f};
    quad(out, {{a + n, b + n, b - n, a - n}}, color);
}
sf::Image placeholder(unsigned size, bool trees, bool paddy) {
    sf::Image image({trees ? size * kTreeCells : size, size},
                    trees ? sf::Color::Transparent : sf::Color{248, 247, 244});
    MapRng rng(1597);
    if (trees) {
        for (unsigned cell = 0; cell < kTreeCells; ++cell) {
            for (unsigned y = 20; y < 185; ++y)
                for (unsigned x = 25; x < 168; ++x) {
                    const float dx = static_cast<float>(x) - 96.f;
                    const float dy = static_cast<float>(y);
                    sf::Color color = sf::Color::Transparent;
                    if (std::abs(dx) < 3.f && y > 65)
                        color = {57, 51, 37, 235};
                    for (int blob = 0; blob < 3; ++blob) {
                        const float bx = static_cast<float>(blob - 1) * 23.f;
                        const float by = 65.f + static_cast<float>(blob % 2) * 35.f;
                        if ((dx - bx) * (dx - bx) + (dy - by) * (dy - by) < 32.f * 32.f)
                            color = {65, 88, 65, 210};
                    }
                    image.setPixel({cell * size + x, y}, color);
                }
        }
    } else {
        for (unsigned i = 0; i < size * size / 12; ++i) {
            const auto x = static_cast<unsigned>(rng() * static_cast<float>(size));
            const auto y = static_cast<unsigned>(rng() * static_cast<float>(size));
            for (unsigned k = 0; k < (paddy ? 4u : 1u); ++k)
                image.setPixel({(x + k) % size, y}, {225, 224, 217});
        }
    }
    return image;
}
void loadTexture(sf::Texture &texture, const std::filesystem::path &file, unsigned size, bool trees, bool paddy) {
    sf::Image image;
    std::error_code error;
    const sf::Vector2u expected{trees ? size * kTreeCells : size, size};
    const bool loaded = std::filesystem::exists(file, error) && image.loadFromFile(file) && image.getSize() == expected;
    if (!loaded) {
        std::cerr << "map detail " << file.string() << " missing or invalid (expected " << expected.x << 'x'
                  << expected.y << "); using placeholder\n";
        image = placeholder(size, trees, paddy);
    }
    // Tile alpha is not part of the multiply contract.
    if (!trees)
        for (unsigned y = 0; y < size; ++y)
            for (unsigned x = 0; x < size; ++x) {
                auto color = image.getPixel({x, y});
                color.a = 255;
                image.setPixel({x, y}, color);
            }
    if (!texture.loadFromImage(image))
        throw std::runtime_error("could not upload map detail");
    texture.setSmooth(true);
    texture.setRepeated(!trees);
    if (!texture.generateMipmap())
        std::cerr << "map detail mipmap failed: " << file.string() << '\n';
}
} // namespace

bool treeAllowed(const MapDef &map, const MapTree &tree, Aabb exit) {
    const float size = kTreeCellMeters * tree.scale;
    const Aabb box{tree.pos.x - size * 0.5f, tree.pos.y - kTreeAnchorY / kDetailPixelsPerMeter * tree.scale, size,
                   size};
    if (box.x < 0.f || box.y < 0.f || box.x + box.w > map.width || box.y + box.h > map.height)
        return false;
    for (const auto &p : map.paddies)
        if (overlaps(box, grow(p, 7.f)))
            return false; // 4 m jitter plus 3 m clearance.
    // Include the whole canopy, also conservatively covering the smoothed road's bend.
    if (roadDistance(map, tree.pos) < map.road.halfWidth + 6.f + size)
        return false;
    if (std::hypot(tree.pos.x - map.postStation.x, tree.pos.y - map.postStation.y) < 18.f + size)
        return false;
    if (overlaps(box, exit) || overlaps(box, {20.f, 20.f, 42.f, 42.f}) || overlaps(box, {26.f, 1050.f, 350.f, 32.f}))
        return false;
    for (const auto &label : map.labels) {
        const float width = label.size * 8.f;
        const float x = label.pos.x - (label.align == TextAlign::Center ? width * 0.5f : 0.f);
        if (overlaps(box, {x - 3.f, label.pos.y - label.size * 1.2f, width + 6.f, label.size * 1.5f}))
            return false;
    }
    return true;
}
std::vector<MapTree> placeTrees(const MapDef &map, std::uint32_t seed, Aabb exit) {
    MapRng rng(seed);
    std::vector<MapTree> result;
    if (map.width <= 0.f || map.height <= 0.f)
        return result;
    // Fixed grove centres, then clustered candidates; a small uniform tail supplies roadside trees.
    std::vector<Vec2> centers;
    for (const auto &hill : map.hills)
        centers.push_back({hill.cx, hill.cy});
    for (int i = 0; i < 8; ++i)
        centers.push_back({i % 2 == 0 ? 45.f : map.width - 45.f, (static_cast<float>(i) + 0.5f) * map.height / 8.f});
    centers.push_back(map.postStation);
    for (int attempt = 0; attempt < 20000 && result.size() < 230; ++attempt) {
        Vec2 p{rng() * map.width, rng() * map.height};
        if (attempt % 10 < 8) {
            const auto index =
                std::min(static_cast<std::size_t>(rng() * static_cast<float>(centers.size())), centers.size() - 1);
            p = centers[index] + Vec2{(rng() + rng() - 1.f) * 110.f, (rng() + rng() - 1.f) * 130.f};
        }
        const bool hill = onHill(map, p);
        const bool margin = p.x < map.width * 0.2f || p.x > map.width * 0.8f;
        if (!hill && !margin && roadDistance(map, p) > 65.f && rng() > 0.08f)
            continue;
        MapTree tree{p, static_cast<unsigned>(rng() * static_cast<float>(kTreeCells)), 0.85f + rng() * 0.3f,
                     rng() < 0.5f};
        if (hill && rng() < 0.8f)
            tree.variant = rng() < 0.5f ? 0u : 6u;
        if (!treeAllowed(map, tree, exit))
            continue;
        if (std::any_of(result.begin(), result.end(),
                        [&](const MapTree &other) { return std::hypot(p.x - other.pos.x, p.y - other.pos.y) < 9.f; }))
            continue;
        result.push_back(tree);
    }
    std::stable_sort(result.begin(), result.end(),
                     [](const MapTree &a, const MapTree &b) { return a.pos.y < b.pos.y; });
    return result;
}
sf::VertexArray buildRidges(const std::vector<PaddyDetail> &paddies) {
    sf::VertexArray out(sf::PrimitiveType::Triangles);
    for (std::size_t i = 0; i < paddies.size(); ++i) {
        MapRng rng(1599u + static_cast<std::uint32_t>(i));
        const auto ridge = [&](Vec2 a, Vec2 b) {
            const Vec2 d = b - a;
            const float length = std::hypot(d.x, d.y);
            if (length < 0.001f)
                return;
            Vec2 n{-d.y / length, d.x / length};
            if (n.x + n.y < 0.f)
                n = -n;
            band(out, a, b, kBankWidth, {238, 226, 187, 160});
            band(out, a + n * 0.45f, b + n * 0.45f, 0.5f, {87, 74, 46, 140});
            for (int tick = 0; tick < 3; ++tick) {
                const Vec2 p = a + d * (0.1f + rng() * 0.8f);
                band(out, p, p + Vec2{-0.3f, -0.6f - rng() * 0.4f}, 0.18f, {122, 111, 69, 145});
            }
        };
        const auto &p = paddies[i];
        for (std::size_t edge = 0; edge < 4; ++edge)
            ridge(p.quad[edge], p.quad[(edge + 1) % 4]);
        for (const auto &b : p.bunds)
            ridge(b.a, b.b);
    }
    return out;
}
sf::BlendMode mapDetailBlendMode() {
    return sf::BlendMode(sf::BlendMode::Factor::DstColor, sf::BlendMode::Factor::OneMinusSrcAlpha,
                         sf::BlendMode::Equation::Add, sf::BlendMode::Factor::Zero, sf::BlendMode::Factor::One,
                         sf::BlendMode::Equation::Add);
}
sf::Color mapDetailTileColor(float blend) {
    const auto tile = static_cast<std::uint8_t>(255.f * std::clamp(blend, 0.f, 1.f) * kMapDetailStrength + 0.5f);
    return {tile, tile, tile, tile};
}
void MapDetail::load(const std::filesystem::path &assetRoot) {
    const auto root = assetRoot / "textures" / "map";
    loadTexture(treesTexture_, root / "trees.png", kTreeCellPx, true, false);
    loadTexture(groundTexture_, root / "ground_detail.png", kGroundPx, false, false);
    loadTexture(paddyTexture_, root / "paddy_detail.png", kPaddyPx, false, true);
}
void MapDetail::build(const MapDef &map, Aabb exit, const std::vector<PaddyDetail> &paddies) {
    ground_.clear();
    paddies_.clear();
    trees_.clear();
    ridgeColors_.clear();
    blend_ = -1.f;
    quad(ground_, {{{0.f, 0.f}, {map.width, 0.f}, {map.width, map.height}, {0.f, map.height}}}, sf::Color::White,
         kDetailPixelsPerMeter);
    for (const auto &p : paddies)
        quad(paddies_, p.quad, sf::Color::White, kDetailPixelsPerMeter);
    ridges_ = buildRidges(paddies);
    for (std::size_t i = 0; i < ridges_.getVertexCount(); ++i)
        ridgeColors_.push_back(ridges_[i].color);
    for (const auto &t : placeTrees(map, 1593, exit)) {
        const float sc = t.scale / kDetailPixelsPerMeter;
        const float left = t.pos.x - kTreeAnchorX * sc, top = t.pos.y - kTreeAnchorY * sc,
                    size = static_cast<float>(kTreeCellPx) * sc;
        const std::array<Vec2, 4> points{
            {{left, top}, {left + size, top}, {left + size, top + size}, {left, top + size}}};
        const float u = static_cast<float>(t.variant * kTreeCellPx);
        const float lo = u + 0.5f, hi = u + static_cast<float>(kTreeCellPx) - 0.5f;
        const std::array<sf::Vector2f, 4> uv{{{t.mirror ? hi : lo, 0.5f},
                                              {t.mirror ? lo : hi, 0.5f},
                                              {t.mirror ? lo : hi, static_cast<float>(kTreeCellPx) - 0.5f},
                                              {t.mirror ? hi : lo, static_cast<float>(kTreeCellPx) - 0.5f}}};
        for (const unsigned i : {0u, 1u, 2u, 0u, 2u, 3u})
            trees_.append({{points[i].x, points[i].y}, sf::Color::White, uv[i]});
    }
}
void MapDetail::fade(float blend) const {
    blend = std::clamp(blend, 0.f, 1.f);
    if (blend == blend_)
        return;
    blend_ = blend;
    const auto alpha = static_cast<std::uint8_t>(255.f * blend + 0.5f);
    const sf::Color tile = mapDetailTileColor(blend);
    for (auto *array : {&ground_, &paddies_})
        for (std::size_t i = 0; i < array->getVertexCount(); ++i)
            (*array)[i].color = tile;
    for (std::size_t i = 0; i < trees_.getVertexCount(); ++i)
        trees_[i].color = {255, 255, 255, alpha};
    for (std::size_t i = 0; i < ridges_.getVertexCount(); ++i) {
        auto color = ridgeColors_[i];
        color.a = static_cast<std::uint8_t>(static_cast<float>(color.a) * blend + 0.5f);
        ridges_[i].color = color;
    }
}
void MapDetail::drawGround(sf::RenderTarget &target, float blend) const {
    if (blend <= 0.f)
        return;
    fade(blend);
    sf::RenderStates states;
    states.blendMode = mapDetailBlendMode();
    states.texture = &groundTexture_;
    target.draw(ground_, states);
    states.texture = &paddyTexture_;
    target.draw(paddies_, states);
    target.draw(ridges_);
}
void MapDetail::drawTrees(sf::RenderTarget &target, float blend) const {
    if (blend <= 0.f)
        return;
    fade(blend);
    sf::RenderStates states;
    states.texture = &treesTexture_;
    target.draw(trees_, states);
}
} // namespace tq
