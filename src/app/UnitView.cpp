#include "app/UnitView.h"

#include "app/Pieces.h"
#include "core/rules/Preview.h"

#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/System/String.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
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

sf::Color paint(Rgba color) { return {color.r, color.g, color.b, color.a}; }

sf::Color fade(sf::Color color, float factor) {
    color.a = static_cast<std::uint8_t>(std::clamp(static_cast<float>(color.a) * factor, 0.f, 255.f) + 0.5f);
    return color;
}

sf::Vector2f xy(Vec2 point) { return {point.x, point.y}; }

Vec2 rotate(Vec2 origin, float facing, Vec2 local) {
    const float c = std::cos(facing);
    const float s = std::sin(facing);
    return {origin.x + local.x * c - local.y * s, origin.y + local.x * s + local.y * c};
}

bool chosen(std::span<const UnitId> selected, UnitId id) {
    return std::find(selected.begin(), selected.end(), id) != selected.end();
}

void appendTri(sf::VertexArray &tris, sf::Vector2f a, sf::Vector2f b, sf::Vector2f c, sf::Color color) {
    tris.append(sf::Vertex{a, color});
    tris.append(sf::Vertex{b, color});
    tris.append(sf::Vertex{c, color});
}

void appendQuad(sf::VertexArray &tris, Vec2 a, Vec2 b, Vec2 c, Vec2 d, sf::Color color) {
    appendTri(tris, xy(a), xy(b), xy(c), color);
    appendTri(tris, xy(a), xy(c), xy(d), color);
}

void appendRect(sf::VertexArray &tris, float x, float y, float w, float h, sf::Color color) {
    if (!(w > 0.f) || !(h > 0.f)) {
        return;
    }
    appendQuad(tris, {x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}, color);
}

void appendLocalRect(sf::VertexArray &tris, Vec2 origin, float facing, float x, float y, float w, float h,
                     sf::Color color) {
    if (!(w > 0.f) || !(h > 0.f)) {
        return;
    }
    appendQuad(tris, rotate(origin, facing, {x, y}), rotate(origin, facing, {x + w, y}),
               rotate(origin, facing, {x + w, y + h}), rotate(origin, facing, {x, y + h}), color);
}

void appendStrip(sf::VertexArray &tris, const std::vector<Vec2> &pts, float thickness, sf::Color color) {
    if (pts.size() < 2 || !(thickness > 0.f)) {
        return;
    }
    const float half = thickness * 0.5f;
    for (std::size_t i = 0; i + 1 < pts.size(); ++i) {
        Vec2 dir = pts[i + 1] - pts[i];
        const float len = std::hypot(dir.x, dir.y);
        if (len < 1e-4f) {
            continue;
        }
        dir = {dir.x / len, dir.y / len};
        const Vec2 n{-dir.y * half, dir.x * half};
        appendQuad(tris, pts[i] + n, pts[i] - n, pts[i + 1] - n, pts[i + 1] + n, color);
    }
}

void appendDisc(sf::VertexArray &tris, Vec2 center, float radius, sf::Color color) {
    if (!(radius > 0.f)) {
        return;
    }
    constexpr int steps = 24;
    for (int i = 0; i < steps; ++i) {
        const float a0 = kPi * 2.f * static_cast<float>(i) / static_cast<float>(steps);
        const float a1 = kPi * 2.f * static_cast<float>(i + 1) / static_cast<float>(steps);
        appendTri(tris, xy(center), {center.x + std::cos(a0) * radius, center.y + std::sin(a0) * radius},
                  {center.x + std::cos(a1) * radius, center.y + std::sin(a1) * radius}, color);
    }
}

void dash(sf::VertexArray &tris, const std::vector<Vec2> &pts, float on, float off, float thickness, sf::Color color) {
    if (pts.size() < 2 || !(on > 0.f)) {
        return;
    }
    float phase = 0.f;
    bool drawing = true;
    std::vector<Vec2> segment;
    segment.push_back(pts.front());
    auto flush = [&] {
        appendStrip(tris, segment, thickness, color);
        segment.clear();
    };
    for (std::size_t i = 0; i + 1 < pts.size(); ++i) {
        Vec2 cursor = pts[i];
        const Vec2 end = pts[i + 1];
        Vec2 delta = end - cursor;
        float remain = std::hypot(delta.x, delta.y);
        if (remain < 1e-4f) {
            continue;
        }
        delta = {delta.x / remain, delta.y / remain};
        while (remain > 1e-4f) {
            const float limit = drawing ? on : off;
            const float step = std::min(remain, limit - phase);
            cursor = cursor + delta * step;
            remain -= step;
            phase += step;
            if (drawing) {
                segment.push_back(cursor);
            }
            if (phase >= limit - 1e-4f) {
                if (drawing) {
                    flush();
                }
                drawing = !drawing;
                phase = 0.f;
                if (drawing) {
                    segment.push_back(cursor);
                }
            }
        }
    }
    if (drawing) {
        flush();
    }
}

std::vector<sf::Vector2f> roundRect(float x, float y, float w, float h, float radius) {
    const float r = std::max(0.f, std::min(radius, std::min(w, h) * 0.5f));
    std::vector<sf::Vector2f> path;
    if (!(r > 0.f)) {
        path.push_back({x, y});
        path.push_back({x + w, y});
        path.push_back({x + w, y + h});
        path.push_back({x, y + h});
        return path;
    }
    auto corner = [&](float cx, float cy, float a0, float a1) {
        constexpr int steps = 5;
        for (int i = 0; i <= steps; ++i) {
            const float t = a0 + (a1 - a0) * (static_cast<float>(i) / static_cast<float>(steps));
            path.push_back({cx + std::cos(t) * r, cy + std::sin(t) * r});
        }
    };
    corner(x + w - r, y + r, -kPi * 0.5f, 0.f);
    corner(x + w - r, y + h - r, 0.f, kPi * 0.5f);
    corner(x + r, y + h - r, kPi * 0.5f, kPi);
    corner(x + r, y + r, kPi, kPi * 1.5f);
    return path;
}

void fillRound(sf::VertexArray &tris, float x, float y, float w, float h, float radius, sf::Color color) {
    const std::vector<sf::Vector2f> path = roundRect(x, y, w, h, radius);
    if (path.size() < 3) {
        return;
    }
    const sf::Vector2f center{x + w * 0.5f, y + h * 0.5f};
    for (std::size_t i = 0; i < path.size(); ++i) {
        appendTri(tris, center, path[i], path[(i + 1) % path.size()], color);
    }
}

std::vector<Vec2> asVec(const std::vector<sf::Vector2f> &path, bool closed) {
    std::vector<Vec2> pts;
    pts.reserve(path.size() + 1);
    for (const sf::Vector2f &point : path) {
        pts.push_back({point.x, point.y});
    }
    if (closed && !pts.empty()) {
        pts.push_back(pts.front());
    }
    return pts;
}

void strokeRound(sf::VertexArray &tris, float x, float y, float w, float h, float radius, float thickness,
                 sf::Color color) {
    appendStrip(tris, asVec(roundRect(x, y, w, h, radius), true), thickness, color);
}

void dashRound(sf::VertexArray &tris, float x, float y, float w, float h, float radius, float on, float off,
               float thickness, sf::Color color) {
    dash(tris, asVec(roundRect(x, y, w, h, radius), true), on, off, thickness, color);
}

void appendArc(sf::VertexArray &tris, Vec2 center, float radius, float fraction, float thickness, sf::Color color) {
    if (!(radius > 0.f) || !(fraction > 0.f)) {
        return;
    }
    const float sweep = kPi * 2.f * std::clamp(fraction, 0.f, 1.f);
    const int steps = std::max(2, static_cast<int>(std::lround(28.f * std::clamp(fraction, 0.f, 1.f))));
    std::vector<Vec2> pts;
    pts.reserve(static_cast<std::size_t>(steps) + 1);
    for (int i = 0; i <= steps; ++i) {
        const float angle = -kPi * 0.5f + sweep * (static_cast<float>(i) / static_cast<float>(steps));
        pts.push_back({center.x + std::cos(angle) * radius, center.y + std::sin(angle) * radius});
    }
    appendStrip(tris, pts, thickness, color);
}

void appendBlock(sf::VertexArray &tris, const Unit &unit, float alpha, float sc) {
    const Pose pose = interpolatedPose(unit, alpha);
    const bool routing = !isFighting(unit.state);
    const float cover = routing ? 0.45f : 1.f;
    const float halfDepth = unit.depth * 0.5f;
    const float halfFront = unit.frontage * 0.5f;
    const sf::Color fill =
        unit.side == Side::Ming ? fade(rgba(168, 56, 42, 0.2f), cover) : fade(rgba(47, 58, 86, 0.24f), cover);
    appendLocalRect(tris, pose.pos, pose.facing, -halfDepth, -halfFront, unit.depth, unit.frontage, fill);

    const int columns = blockColumns(unit.frontage);
    const int count = blockDots(unit.frontage, unit.depth, unit.men, unit.maxMen);
    const sf::Color dot =
        unit.side == Side::Ming ? fade(rgba(128, 34, 24, 0.78f), cover) : fade(rgba(28, 36, 58, 0.82f), cover);
    for (int i = 0; i < count; ++i) {
        const Vec2 jitter = routing ? routingJitter(unit.id, i) : Vec2{};
        const Vec2 local = blockDotLocal(unit.depth, unit.frontage, i, columns, jitter);
        appendLocalRect(tris, pose.pos, pose.facing, local.x, local.y, 2.6f, 2.6f, dot);
    }

    if (!routing) {
        const sf::Color edge = unit.side == Side::Ming ? rgba(120, 32, 22, 0.7f) : rgba(28, 36, 58, 0.75f);
        const float thin = 1.f / sc;
        const Vec2 corners[4] = {
            rotate(pose.pos, pose.facing, {-halfDepth, -halfFront}),
            rotate(pose.pos, pose.facing, {halfDepth, -halfFront}),
            rotate(pose.pos, pose.facing, {halfDepth, halfFront}),
            rotate(pose.pos, pose.facing, {-halfDepth, halfFront}),
        };
        for (int i = 0; i < 4; ++i) {
            appendStrip(tris, {corners[i], corners[(i + 1) % 4]}, thin, edge);
        }
        appendStrip(tris,
                    {rotate(pose.pos, pose.facing, {halfDepth, -halfFront}),
                     rotate(pose.pos, pose.facing, {halfDepth, halfFront})},
                    3.f, edge);
    }
    if (unit.charging) {
        const sf::Color ink = rgba(42, 34, 27, 0.38f);
        for (int lane = -2; lane <= 2; ++lane) {
            const float across = static_cast<float>(lane) * halfFront / 2.5f;
            const float length = chargeLineLength(unit.id, lane, unit.speed);
            appendStrip(tris,
                        {rotate(pose.pos, pose.facing, {-halfDepth - 3.f, across}),
                         rotate(pose.pos, pose.facing, {-halfDepth - 3.f - length, across})},
                        1.3f, ink);
        }
    }
}

void drawGlyph(sf::RenderTarget &target, const sf::Transform &xform, const sf::Font &font, std::string_view text,
               float worldHeight, float pixelsPerMeter, sf::Color color, Vec2 local) {
    const long rounded = std::lround(worldHeight * pixelsPerMeter);
    const unsigned pixels = rounded > 1 ? static_cast<unsigned>(rounded) : 1u;
    sf::Text shape(font, utf8(text), pixels);
    shape.setFillColor(color);
    const float scale = worldHeight / static_cast<float>(pixels);
    shape.setScale({scale, scale});
    const sf::FloatRect bounds = shape.getLocalBounds();
    shape.setOrigin({bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f});
    shape.setPosition(xy(local));
    target.draw(shape, sf::RenderStates{xform});
}

void drawChit(sf::RenderTarget &target, const Unit &unit, const UnitType &type, const StringTable &strings,
              const sf::Font &font, float timeSec, float alpha, float sc, bool selected, bool hover) {
    const Pose pose = interpolatedPose(unit, alpha);
    const float pixels = chitPixels(sc, unit.maxMen, type.men);
    const float s = pixels / sc;
    const float h = s * 0.5f;
    const float brokenSec =
        unit.state == UnitState::Broken ? presentedBrokenSec(unit.brokenSec, alpha) : unit.brokenSec;
    const ChitFace face = chitFace(unit.state, brokenSec);
    const ChitColors colors = chitColors(unit.side, type.general, face.back);

    if (selected) {
        sf::VertexArray mark(sf::PrimitiveType::Triangles);
        const float x0 = -unit.depth * 0.5f - 3.f;
        const float y0 = -unit.frontage * 0.5f - 3.f;
        const float x1 = unit.depth * 0.5f + 3.f;
        const float y1 = unit.frontage * 0.5f + 3.f;
        dash(mark,
             {rotate(pose.pos, pose.facing, {x0, y0}), rotate(pose.pos, pose.facing, {x1, y0}),
              rotate(pose.pos, pose.facing, {x1, y1}), rotate(pose.pos, pose.facing, {x0, y1}),
              rotate(pose.pos, pose.facing, {x0, y0})},
             5.f / sc, 3.f / sc, 2.f / sc, paint(kRecoverArc));
        target.draw(mark);
        if (type.general && type.aura > 0.f) {
            sf::VertexArray aura(sf::PrimitiveType::Triangles);
            constexpr int steps = 64;
            std::vector<Vec2> ring;
            ring.reserve(static_cast<std::size_t>(steps) + 1);
            for (int i = 0; i <= steps; ++i) {
                const float angle = kPi * 2.f * static_cast<float>(i) / static_cast<float>(steps);
                ring.push_back({pose.pos.x + std::cos(angle) * type.aura, pose.pos.y + std::sin(angle) * type.aura});
            }
            appendStrip(aura, ring, 1.f / sc, rgba(217, 180, 102, 0.35f));
            target.draw(aura);
        }
    }

    sf::Transform xform;
    xform.translate(xy(pose.pos));
    xform.rotate(sf::radians(chitWobble(unit.state, timeSec, unit.id)));
    xform.scale({face.scaleX, 1.f});

    sf::VertexArray body(sf::PrimitiveType::Triangles);
    const float radius = s * 0.12f;
    fillRound(body, -h + s * 0.05f, -h + s * 0.08f, s, s, radius, rgba(18, 10, 6, 0.4f));
    fillRound(body, -h, -h, s, s, radius, paint(colors.face));
    fillRound(body, -h, -h, s, s * 0.46f, radius, rgba(255, 255, 255, 0.11f));
    if (hover) {
        strokeRound(body, -h - 2.f / sc, -h - 2.f / sc, s + 4.f / sc, s + 4.f / sc, radius, 2.5f / sc,
                    unit.side == Side::Ming ? paint(kRecoverArc) : rgba(168, 56, 42, 1.f));
    }
    const float frame = std::max(1.f / sc, s * 0.06f);
    if (unit.isFragment && !face.back) {
        dashRound(body, -h + s * 0.09f, -h + s * 0.09f, s * 0.82f, s * 0.82f, radius * 0.6f, s * 0.12f, s * 0.08f,
                  frame, paint(colors.edge));
    } else {
        strokeRound(body, -h + s * 0.09f, -h + s * 0.09f, s * 0.82f, s * 0.82f, radius * 0.6f, frame,
                    paint(colors.edge));
    }
    if (type.elite && !face.back) {
        strokeRound(body, -h + s * 0.17f, -h + s * 0.17f, s * 0.66f, s * 0.66f, radius * 0.4f,
                    std::max(0.8f / sc, s * 0.03f), paint(colors.edge));
    }
    target.draw(body, sf::RenderStates{xform});

    const std::string glyph = face.back ? strings.text("glyph.routed") : strings.text(type.glyphKey);
    drawGlyph(target, xform, font, glyph, s * 0.62f, sc, paint(colors.glyph), {0.f, s * 0.04f});
    if (face.back) {
        sf::VertexArray crack(sf::PrimitiveType::Triangles);
        appendStrip(crack,
                    {{-h * 0.8f, -h * 0.6f}, {-h * 0.1f, -h * 0.1f}, {-h * 0.3f, h * 0.3f}, {h * 0.7f, h * 0.75f}},
                    1.f / sc, rgba(42, 34, 27, 0.55f));
        target.draw(crack, sf::RenderStates{xform});
    }

    sf::VertexArray extras(sf::PrimitiveType::Triangles);
    if (isFighting(unit.state)) {
        const float c = std::cos(pose.facing);
        const float sn = std::sin(pose.facing);
        const float base = h + 11.f / sc;
        const float tip = h + 18.f / sc;
        const float half = 5.f / sc;
        const Vec2 forward{c, sn};
        const Vec2 side{-sn, c};
        const sf::Color wedge = unit.side == Side::Ming ? rgba(168, 56, 42, 1.f) : rgba(47, 58, 86, 1.f);
        appendTri(extras, xy(pose.pos + forward * tip), xy(pose.pos + forward * base - side * half),
                  xy(pose.pos + forward * base + side * half), wedge);
    }

    const float barH = 3.f / sc;
    const float barY = pose.pos.y + h + 3.f / sc;
    appendRect(extras, pose.pos.x - h, barY, s, barH * 2.f + 1.f / sc, rgba(18, 10, 6, 0.6f));
    appendRect(extras, pose.pos.x - h, barY, s * menFraction(unit.men, unit.maxMen), barH, paint(kMenBar));
    appendRect(extras, pose.pos.x - h, barY + barH + 1.f / sc, s * moraleFraction(unit.morale), barH,
               paint(moraleColor(unit.state)));

    for (int i = 0; i < unit.momentum; ++i) {
        const float x =
            pose.pos.x - static_cast<float>(unit.momentum - 1) * 4.f / sc + static_cast<float>(i) * 8.f / sc;
        const float y = pose.pos.y - h - 6.f / sc;
        const float hw = 3.f / sc;
        const float hh = 3.5f / sc;
        appendTri(extras, {x, y - hh}, {x + hw, y}, {x, y + hh}, rgba(168, 56, 42, 1.f));
        appendTri(extras, {x, y - hh}, {x, y + hh}, {x - hw, y}, rgba(168, 56, 42, 1.f));
    }

    const ArcKind arc = timerArc(unit.state, unit.recoverSec, unit.reloadSec, type.range);
    float fraction = 0.f;
    sf::Color arcColor = paint(kRecoverArc);
    if (arc == ArcKind::Recover) {
        fraction = recoverFraction(unit.recoverSec);
        arcColor = paint(kRecoverArc);
    } else if (arc == ArcKind::Reload) {
        fraction = reloadFraction(unit.reloadSec, type.reload);
        arcColor = paint(kReloadArc);
    }
    if (fraction > 0.f) {
        const Vec2 at{pose.pos.x + h, pose.pos.y - h};
        const float arcRadius = 5.f / sc;
        appendDisc(extras, at, arcRadius + 1.5f / sc, rgba(18, 10, 6, 0.75f));
        appendArc(extras, at, arcRadius, fraction, 2.f / sc, arcColor);
    }
    target.draw(extras);
}

void appendArrow(sf::VertexArray &tris, Vec2 from, Vec2 to, sf::Color color, float width, float head, bool dashed,
                 float on, float off) {
    const Vec2 delta = to - from;
    const float length = std::hypot(delta.x, delta.y);
    if (!(length > 1e-3f)) {
        return;
    }
    const float angle = std::atan2(delta.y, delta.x);
    const float back = std::min(length, head * 0.8f);
    const Vec2 shaftEnd{to.x - std::cos(angle) * back, to.y - std::sin(angle) * back};
    if (dashed) {
        dash(tris, {from, shaftEnd}, on, off, width, color);
    } else {
        appendStrip(tris, {from, shaftEnd}, width, color);
    }
    const Vec2 left{to.x - std::cos(angle - 0.4f) * head, to.y - std::sin(angle - 0.4f) * head};
    const Vec2 right{to.x - std::cos(angle + 0.4f) * head, to.y - std::sin(angle + 0.4f) * head};
    appendTri(tris, xy(to), xy(left), xy(right), color);
}

Vec2 stopShort(Vec2 from, Vec2 to, float gap) {
    const Vec2 delta = to - from;
    const float distance = std::hypot(delta.x, delta.y);
    if (!(distance > 1e-3f)) {
        return from;
    }
    const float scale = std::max(0.f, distance - gap) / distance;
    return from + delta * scale;
}

bool outlined(const OrderState &orders, UnitId id) {
    return (orders.hover && *orders.hover == id) || (orders.pending && *orders.pending == id);
}

const Unit *firstOrderable(const Battle &battle, const OrderState &orders) {
    for (UnitId id : orders.selected) {
        if (id >= battle.units().size()) {
            continue;
        }
        const Unit &unit = battle.unit(id);
        if (unit.side == Side::Ming && isFighting(unit.state)) {
            return &unit;
        }
    }
    return nullptr;
}

void drawOrders(sf::RenderTarget &target, const Battle &battle, const OrderState &orders, float alpha, float sc) {
    sf::VertexArray lines(sf::PrimitiveType::Triangles);
    const sf::Color ink = rgba(42, 34, 27, 0.6f);
    const sf::Color attack = rgba(168, 56, 42, 0.75f);
    for (UnitId id : orders.selected) {
        if (id >= battle.units().size()) {
            continue;
        }
        const Unit &unit = battle.unit(id);
        if (!isFighting(unit.state)) {
            continue;
        }
        const Pose pose = interpolatedPose(unit, alpha);
        if (unit.order.type == Order::Type::Move) {
            dash(lines, {pose.pos, unit.order.point}, 6.f / sc, 5.f / sc, 1.4f / sc, ink);
            constexpr int steps = 24;
            std::vector<Vec2> ring;
            ring.reserve(static_cast<std::size_t>(steps) + 1);
            const float radius = 4.f / sc;
            for (int i = 0; i <= steps; ++i) {
                const float angle = kPi * 2.f * static_cast<float>(i) / static_cast<float>(steps);
                ring.push_back(
                    {unit.order.point.x + std::cos(angle) * radius, unit.order.point.y + std::sin(angle) * radius});
            }
            appendStrip(lines, ring, 1.4f / sc, ink);
        } else if (unit.order.type == Order::Type::Attack && unit.order.target < battle.units().size()) {
            const Unit &targetUnit = battle.unit(unit.order.target);
            if (targetUnit.state == UnitState::Gone) {
                continue;
            }
            const Pose targetPose = interpolatedPose(targetUnit, alpha);
            const Vec2 tip = stopShort(pose.pos, targetPose.pos, 22.f);
            appendArrow(lines, pose.pos, tip, attack, 2.f / sc, 9.f / sc, false, 0.f, 0.f);
        }
    }
    target.draw(lines);
}

void drawPreview(sf::RenderTarget &target, const Battle &battle, const OrderState &orders, const sf::Font &font,
                 float alpha, float sc) {
    const Unit *attacker = firstOrderable(battle, orders);
    const std::optional<UnitId> targetId = orders.pending ? orders.pending : orders.hover;
    if (attacker == nullptr || !targetId || *targetId >= battle.units().size()) {
        return;
    }
    const Unit &targetUnit = battle.unit(*targetId);
    if (targetUnit.state == UnitState::Gone || targetUnit.side == attacker->side) {
        return;
    }
    const Preview preview = previewAttack(battle, attacker->id, targetUnit.id);
    const PreviewPaint paintColor = previewPaint(preview.verdict);
    const Pose from = interpolatedPose(*attacker, alpha);
    const Pose to = interpolatedPose(targetUnit, alpha);
    const sf::Color arrow = rgba(paintColor.color.r, paintColor.color.g, paintColor.color.b, paintColor.arrowAlpha);
    sf::VertexArray lines(sf::PrimitiveType::Triangles);
    appendArrow(lines, from.pos, stopShort(from.pos, to.pos, 24.f), arrow, 2.2f / sc, 9.f / sc, true, 8.f / sc,
                4.f / sc);
    target.draw(lines);

    char number[32];
    std::snprintf(number, sizeof(number), "%.1f", static_cast<double>(preview.ratio));
    const std::string label = std::string("\u00d7") + number;
    sf::Text text(font, utf8(label), 15);
    text.setFillColor(rgba(paintColor.color.r, paintColor.color.g, paintColor.color.b, 1.f));
    text.setScale({1.f / sc, 1.f / sc});
    const sf::FloatRect bounds = text.getLocalBounds();
    const float textWidth = bounds.size.x / sc;
    const float left = to.pos.x + 26.f / sc;
    const float top = to.pos.y - 22.f / sc;
    sf::VertexArray backing(sf::PrimitiveType::Triangles);
    appendRect(backing, left - 5.f / sc, top - 11.f / sc, textWidth + 10.f / sc, 22.f / sc, rgba(30, 20, 17, 0.85f));
    target.draw(backing);
    text.setOrigin({bounds.position.x, bounds.position.y + bounds.size.y * 0.5f});
    text.setPosition({left, top + 1.f / sc});
    target.draw(text);
}

void drawDragBox(sf::RenderTarget &target, float x0, float y0, float x1, float y1) {
    const float left = std::min(x0, x1);
    const float top = std::min(y0, y1);
    const float width = std::abs(x1 - x0);
    const float height = std::abs(y1 - y0);
    if (!(width > 0.f) || !(height > 0.f)) {
        return;
    }
    sf::VertexArray box(sf::PrimitiveType::Triangles);
    const sf::Color fill = rgba(207, 170, 92, 0.12f);
    const sf::Color edge = rgba(207, 170, 92, 0.9f);
    appendRect(box, left, top, width, height, fill);
    appendRect(box, left, top, width, 1.f, edge);
    appendRect(box, left, top + height - 1.f, width, 1.f, edge);
    appendRect(box, left, top, 1.f, height, edge);
    appendRect(box, left + width - 1.f, top, 1.f, height, edge);
    target.draw(box);
}

} // namespace

void UnitView::draw(Window &window, const Battle &battle, const StringTable &strings, const sf::Font &glyphFont,
                    const sf::Font &bodyFont, float timeSec, float alpha, const OrderState &orders) const {
    alpha = std::clamp(alpha, 0.f, 1.f);
    const float sc = std::max(window.pixelsPerMeter(), 1e-3f);
    sf::VertexArray blocks(sf::PrimitiveType::Triangles);
    for (const Unit &unit : battle.units()) {
        if (unit.state == UnitState::Gone) {
            continue;
        }
        appendBlock(blocks, unit, alpha, sc);
    }
    window.target().draw(blocks);
    drawOrders(window.target(), battle, orders, alpha, sc);

    auto drawSide = [&](Side side) {
        for (const Unit &unit : battle.units()) {
            if (unit.state == UnitState::Gone || unit.side != side) {
                continue;
            }
            drawChit(window.target(), unit, battle.typeOf(unit), strings, glyphFont, timeSec, alpha, sc,
                     chosen(orders.selected, unit.id), outlined(orders, unit.id));
        }
    };
    drawSide(Side::Jp);
    drawSide(Side::Ming);
    drawPreview(window.target(), battle, orders, bodyFont, alpha, sc);
    if (orders.box) {
        window.beginUi();
        drawDragBox(window.target(), orders.boxX0, orders.boxY0, orders.boxX1, orders.boxY1);
    }
}

} // namespace tq
