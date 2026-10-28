#pragma once

#include "app/Pieces.h"
#include "app/Pointer.h"
#include "app/Script.h"
#include "core/rules/Preview.h"
#include "core/sim/Battle.h"

#include <istream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace tq {

// A drag starts a box only after this many logical pixels. A drag that begins on a unit stays a click.
inline constexpr float kDragThresholdPx = 6.f;

enum class OrderKey : std::uint8_t { Space, Speed1, Speed2, Speed3, Hold, Retreat, Split, Join, All, Cancel };

struct OrderState {
    std::vector<UnitId> selected;
    std::optional<UnitId> hover;
    std::optional<UnitId> pending;
    bool pressing = false;
    float pressX = 0.f;
    float pressY = 0.f;
    int pressButton = kPointerLeft;
    PointerSample::Device pressDevice = PointerSample::Device::Mouse;
    bool box = false;
    float boxX0 = 0.f;
    float boxY0 = 0.f;
    float boxX1 = 0.f;
    float boxY1 = 0.f;
};

struct OrderEffect {
    std::vector<Command> commands;
    bool toggleRun = false;
    std::optional<float> speed;
};

struct ControlPlayback {
    std::vector<Command> commands;
    OrderState state;
    bool running = false;
    float speed = 1.f;
};

struct PreviewPaint {
    Rgba color{};
    float arrowAlpha = 1.f;
};

// Favored is green, even is gold, unfavored is cinnabar. Breaks and pursuit are favored; a pike wall is not.
inline PreviewPaint previewPaint(Verdict verdict) {
    switch (verdict) {
    case Verdict::Breaks:
    case Verdict::Favored:
    case Verdict::Pursuit:
        return {{0x4f, 0x8a, 0x4b, 255}, 0.85f};
    case Verdict::Even:
        return {{0xc9, 0x96, 0x2f, 255}, 0.9f};
    case Verdict::Unfavored:
    case Verdict::PikeWall:
        return {{0xb8, 0x40, 0x2c, 255}, 0.9f};
    }
    return {{0xb8, 0x40, 0x2c, 255}, 0.9f};
}

inline void toggleRunning(bool &running, const Battle &battle) {
    if (battle.isOver()) {
        return;
    }
    if (battle.phase() == 0) {
        running = true;
    } else {
        running = !running;
    }
}

// Several hits keep the one closest to the pointer. A tie keeps the earlier unit.
UnitId pickUnit(const Battle &battle, float worldX, float worldY, float pixelsPerMeter, float alpha);

std::vector<Command> applyPointer(OrderState &state, const Battle &battle, const PointerSample &sample, float mapW,
                                  float mapH, float alpha);

OrderEffect applyKey(OrderState &state, const Battle &battle, OrderKey key, const std::vector<float> &speeds);

// Confirm Attack issues the pending attack. With nothing pending, it does not order.
std::vector<Command> confirmPending(OrderState &state, const Battle &battle);

void noteEvents(OrderState &state, const Battle &battle, std::span<const Event> events);

bool isPointerAction(std::string_view action);
std::vector<PointerSample> samplesForScript(const ScriptEvent &event);
std::optional<OrderKey> orderKeyFromScript(const std::string &action, const std::string &arg);

std::string commandText(const Command &command);

// Advance by script time. An event applies after the ticks that finished before that timestamp.
ControlPlayback playControlScript(Battle &battle, std::istream &script, float timeScale,
                                  const std::vector<float> &speeds, float mapW, float mapH, float speed);

} // namespace tq
