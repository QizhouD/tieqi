#include "app/MapDetail.h"
#include "app/Viewport.h"
#include "data/Load.h"
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <algorithm>
#include <cmath>
#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
using namespace tq;

TEST_CASE("map detail layout matches the art tools") {
    std::ifstream in(std::filesystem::path(TQ_SOURCE_DIR) / "tools/art/map_art_layout.py");
    REQUIRE(in.good());
    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    const auto number = [&](const std::string &name) {
        const auto at = text.find("\n" + name + " = ");
        REQUIRE(at != std::string::npos);
        return std::stof(text.substr(at + name.size() + 4));
    };
    CHECK(number("TREE_CELL_PX") == kTreeCellPx);
    CHECK(number("TREE_CELLS") == kTreeCells);
    CHECK(number("TREE_WIDTH") == kTreeCellPx * kTreeCells);
    CHECK(number("TREE_HEIGHT") == kTreeCellPx);
    CHECK(number("TREE_CELL_METERS") == kTreeCellMeters);
    CHECK(number("TREE_ANCHOR_X") == kTreeAnchorX);
    CHECK(number("TREE_ANCHOR_Y") == kTreeAnchorY);
    CHECK(number("PIXELS_PER_METER") == kDetailPixelsPerMeter);
    CHECK(number("GROUND_PX") == kGroundPx);
    CHECK(number("GROUND_METERS") == kGroundMeters);
    CHECK(number("PADDY_PX") == kPaddyPx);
    CHECK(number("PADDY_METERS") == kPaddyMeters);
}

TEST_CASE("map trees are deterministic groves clear of terrain and annotations") {
    const auto data = loadGameData(std::filesystem::path(TQ_SOURCE_DIR) / "data", "byeokjegwan.json");
    const auto &map = data.scenario.map;
    const auto trees = placeTrees(map, 1593, data.scenario.exit.rect);
    const auto again = placeTrees(map, 1593, data.scenario.exit.rect);
    REQUIRE(trees.size() == again.size());
    CHECK(trees.size() >= 150);
    CHECK(trees.size() <= 300);
    std::size_t hillCount = 0, middleCount = 0;
    for (std::size_t i = 0; i < trees.size(); ++i) {
        const auto &t = trees[i];
        CHECK(t.pos.x == again[i].pos.x);
        CHECK(t.pos.y == again[i].pos.y);
        CHECK(t.scale == again[i].scale);
        CHECK(t.variant == again[i].variant);
        CHECK(t.mirror == again[i].mirror);
        CHECK(t.scale >= 0.85f);
        CHECK(t.scale <= 1.15f);
        CHECK(t.variant < kTreeCells);
        CHECK(t.pos.x >= 0.f);
        CHECK(t.pos.x <= map.width);
        CHECK(t.pos.y >= 0.f);
        CHECK(t.pos.y <= map.height);
        if (i)
            CHECK(t.pos.y >= trees[i - 1].pos.y);
        CHECK(treeAllowed(map, t, data.scenario.exit.rect));
        bool hill = false;
        for (const auto &h : map.hills) {
            const float x = (t.pos.x - h.cx) / h.rx, y = (t.pos.y - h.cy) / h.ry;
            hill = hill || x * x + y * y <= 1.f;
        }
        if (hill)
            ++hillCount;
        if (!hill && t.pos.x > map.width / 3.f && t.pos.x < map.width * 2.f / 3.f)
            ++middleCount;
    }
    // Estimate areas on a fixed grid so this checks density, not just total counts.
    int hillArea = 0, middleArea = 0;
    for (float y = 5.f; y < map.height; y += 10.f)
        for (float x = 5.f; x < map.width; x += 10.f) {
            bool hill = false;
            for (const auto &h : map.hills) {
                const float dx = (x - h.cx) / h.rx, dy = (y - h.cy) / h.ry;
                hill = hill || dx * dx + dy * dy <= 1.f;
            }
            if (hill)
                ++hillArea;
            if (!hill && x > map.width / 3.f && x < map.width * 2.f / 3.f)
                ++middleArea;
        }
    CHECK(static_cast<float>(hillCount) / static_cast<float>(hillArea) >
          static_cast<float>(middleCount) / static_cast<float>(middleArea));
}

TEST_CASE("map ridges include outline banks bunds and three grass ticks per segment") {
    PaddyDetail p;
    p.quad = {{{10.f, 10.f}, {50.f, 10.f}, {50.f, 50.f}, {10.f, 50.f}}};
    p.bunds.push_back({{30.f, 10.f}, {30.f, 50.f}});
    const auto ridges = buildRidges({p});
    CHECK(ridges.getVertexCount() == 5 * (12 + 3 * 6));
    for (std::size_t i = 0; i < ridges.getVertexCount(); ++i) {
        CHECK(ridges[i].position.x >= 10.f - kBankWidth);
        CHECK(ridges[i].position.x <= 50.f + kBankWidth);
        CHECK(ridges[i].position.y >= 10.f - kBankWidth);
        CHECK(ridges[i].position.y <= 50.f + kBankWidth);
    }
    CHECK(nearBlend(Zoom{1.79f, {}}) == 0.f);
    CHECK(nearBlend(Zoom{2.2f, {}}) == 1.f);
}
TEST_CASE("map tree exclusions cover canopies as well as anchors") {
    MapDef map;
    map.width = 880.f;
    map.height = 1100.f;
    const auto allowed = [&](Vec2 p) { return treeAllowed(map, MapTree{p, 0, 1.f, false}, {360.f, 0.f, 160.f, 48.f}); };
    CHECK(allowed({300.f, 300.f}));
    CHECK_FALSE(allowed({5.f, 300.f}));
    CHECK_FALSE(allowed({41.f, 65.f}));
    CHECK_FALSE(allowed({190.f, 1070.f}));
    CHECK_FALSE(allowed({400.f, 60.f}));
    map.paddies.push_back({280.f, 280.f, 40.f, 40.f});
    CHECK_FALSE(allowed({300.f, 300.f}));
    CHECK_FALSE(allowed({330.f, 320.f}));
    map.paddies.clear();
    map.postStation = {300.f, 300.f};
    CHECK_FALSE(allowed({317.f, 300.f}));
    map.postStation = {};
    map.road.halfWidth = 14.f;
    map.road.points = {{300.f, 0.f}, {300.f, 1100.f}};
    CHECK_FALSE(allowed({319.f, 300.f}));
    map.road.points.clear();
    map.labels.push_back({"test.label", {300.f, 300.f}, 20.f, TextAlign::Left, false});
    CHECK_FALSE(allowed({440.f, 300.f}));
    map.labels.back().align = TextAlign::Center;
    CHECK_FALSE(allowed({230.f, 300.f}));
    CHECK(allowed({440.f, 300.f}));
}
TEST_CASE("map detail multiply fade interpolates without brightening the base") {
    MapDetail detail;
    detail.load(std::filesystem::path(TQ_SOURCE_DIR) / "assets");
    MapDef map;
    map.width = 8.f;
    map.height = 8.f;
    detail.build(map, {}, {});
    sf::RenderTexture target({64, 64});
    target.setView(sf::View({4.f, 4.f}, {8.f, 8.f}));
    const sf::Color base{180, 160, 130};
    const auto sample = [&](float blend) {
        target.clear(base);
        detail.drawGround(target, blend);
        target.display();
        return target.getTexture().copyToImage().getPixel({32, 32});
    };
    const auto far = sample(0.f), full = sample(1.f);
    CHECK(far == base);
    CHECK(full.r < base.r);
    CHECK(full.g < base.g);
    CHECK(full.b < base.b);
    for (const float blend : {0.25f, 0.5f, 0.75f, 1.f, 0.f}) {
        const auto color = sample(blend);
        CHECK(std::abs(static_cast<float>(color.r) - (base.r * (1.f - blend) + full.r * blend)) <= 2.f);
        CHECK(std::abs(static_cast<float>(color.g) - (base.g * (1.f - blend) + full.g * blend)) <= 2.f);
        CHECK(std::abs(static_cast<float>(color.b) - (base.b * (1.f - blend) + full.b * blend)) <= 2.f);
        CHECK(color.a == 255);
    }
}
