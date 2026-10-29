#include "app/Orders.h"

#include "app/Viewport.h"
#include "core/rules/Constants.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>

namespace tq {
namespace {

bool ordersOpen(const Battle &battle) { return battle.phase() > 0 && !battle.isOver(); }

bool insideBattlefield(float x, float y) {
    const ScreenRect field = battlefieldRect();
    return x >= field.x && y >= field.y && x < field.x + field.w && y < field.y + field.h;
}

bool containsId(const std::vector<UnitId> &ids, UnitId id) {
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

void eraseId(std::vector<UnitId> &ids, UnitId id) { ids.erase(std::remove(ids.begin(), ids.end(), id), ids.end()); }

bool aliveUnit(const Battle &battle, UnitId id) {
    return id < battle.units().size() && battle.unit(id).state != UnitState::Gone;
}

bool orderable(const OrderState &state, const Battle &battle) {
    for (UnitId id : state.selected) {
        if (!aliveUnit(battle, id)) {
            continue;
        }
        const Unit &unit = battle.unit(id);
        if (unit.side == Side::Ming && isFighting(unit.state)) {
            return true;
        }
    }
    return false;
}

Command makeCommand(const OrderState &state, const Battle &battle, Command::Type type) {
    Command command;
    command.tick = battle.tick() + 1;
    command.type = type;
    command.units = state.selected;
    return command;
}

void issueMove(OrderState &state, const Battle &battle, Vec2 point, std::vector<Command> &issued) {
    if (!orderable(state, battle)) {
        return;
    }
    Command command = makeCommand(state, battle, Command::Type::Move);
    command.point = point;
    state.pending.reset();
    issued.push_back(std::move(command));
}

void issueAttack(OrderState &state, const Battle &battle, UnitId target, std::vector<Command> &issued) {
    if (!orderable(state, battle) || !aliveUnit(battle, target)) {
        return;
    }
    Command command = makeCommand(state, battle, Command::Type::Attack);
    command.target = target;
    state.pending.reset();
    issued.push_back(std::move(command));
}

void updateHover(OrderState &state, const Battle &battle, float worldX, float worldY, float pixelsPerMeter,
                 float alpha) {
    const UnitId hit = pickUnit(battle, worldX, worldY, pixelsPerMeter, alpha);
    if (aliveUnit(battle, hit) && battle.unit(hit).side == Side::Jp) {
        state.hover = hit;
    } else {
        state.hover.reset();
    }
}

void boxSelect(OrderState &state, const Battle &battle, float x0, float y0, float x1, float y1, bool shift, float mapW,
               float mapH, float alpha) {
    const Vec2 start = worldFromLogical(x0, y0, mapW, mapH);
    const Vec2 end = worldFromLogical(x1, y1, mapW, mapH);
    const float minX = std::min(start.x, end.x);
    const float maxX = std::max(start.x, end.x);
    const float minY = std::min(start.y, end.y);
    const float maxY = std::max(start.y, end.y);
    std::vector<UnitId> picked;
    for (const Unit &unit : battle.units()) {
        if (unit.side != Side::Ming || unit.state == UnitState::Gone) {
            continue;
        }
        const Pose pose = interpolatedPose(unit, alpha);
        if (pose.pos.x >= minX && pose.pos.x <= maxX && pose.pos.y >= minY && pose.pos.y <= maxY) {
            picked.push_back(unit.id);
        }
    }
    if (picked.empty()) {
        return;
    }
    if (!shift) {
        state.selected = std::move(picked);
        return;
    }
    for (UnitId id : picked) {
        if (!containsId(state.selected, id)) {
            state.selected.push_back(id);
        }
    }
}

void handleClick(OrderState &state, const Battle &battle, Vec2 world, int button, PointerSample::Device device,
                 bool shift, float pixelsPerMeter, float alpha, std::vector<Command> &issued) {
    const UnitId hit = pickUnit(battle, world.x, world.y, pixelsPerMeter, alpha);
    const bool hitUnit = aliveUnit(battle, hit);
    const Unit *unit = hitUnit ? &battle.unit(hit) : nullptr;
    if (button == kPointerRight) {
        if (unit != nullptr && unit->side == Side::Jp) {
            issueAttack(state, battle, hit, issued);
        } else if (unit == nullptr) {
            issueMove(state, battle, world, issued);
        }
        return;
    }
    if (unit != nullptr && unit->side == Side::Ming) {
        if (shift) {
            if (containsId(state.selected, hit)) {
                eraseId(state.selected, hit);
            } else {
                state.selected.push_back(hit);
            }
        } else {
            state.selected = {hit};
        }
        state.pending.reset();
        return;
    }
    if (!orderable(state, battle)) {
        return;
    }
    if (unit != nullptr && unit->side == Side::Jp) {
        if (device != PointerSample::Device::Mouse && !(state.pending && *state.pending == hit)) {
            state.pending = hit;
            return;
        }
        issueAttack(state, battle, hit, issued);
        return;
    }
    issueMove(state, battle, world, issued);
}

std::string lowerKey(std::string name) {
    for (char &letter : name) {
        if (letter >= 'A' && letter <= 'Z') {
            letter = static_cast<char>(letter - 'A' + 'a');
        }
    }
    return name;
}

PointerSample sampleAt(PointerSample::Type type, PointerSample::Device device, int button, bool shift, float x,
                       float y) {
    PointerSample sample;
    sample.type = type;
    sample.device = device;
    sample.button = button;
    sample.shift = shift;
    sample.x = x;
    sample.y = y;
    return sample;
}

void dropGone(OrderState &state, const Battle &battle) {
    std::erase_if(state.selected, [&](UnitId id) { return !aliveUnit(battle, id); });
    if (state.hover && !aliveUnit(battle, *state.hover)) {
        state.hover.reset();
    }
    if (state.pending && !aliveUnit(battle, *state.pending)) {
        state.pending.reset();
    }
}

} // namespace

UnitId pickUnit(const Battle &battle, float worldX, float worldY, float pixelsPerMeter, float alpha) {
    const float scale = std::max(pixelsPerMeter, 1e-3f);
    UnitId best = kNoUnit;
    float bestDistance = 0.f;
    for (const Unit &unit : battle.units()) {
        if (unit.state == UnitState::Gone) {
            continue;
        }
        const Pose pose = interpolatedPose(unit, alpha);
        const float dx = worldX - pose.pos.x;
        const float dy = worldY - pose.pos.y;
        const float halfChit = chitPixels(scale, unit.maxMen, battle.typeOf(unit).men) / scale * 0.5f + 4.f / scale;
        bool hit = std::abs(dx) <= halfChit && std::abs(dy) <= halfChit;
        if (!hit) {
            const float cosine = std::cos(pose.facing);
            const float sine = std::sin(pose.facing);
            const float localX = dx * cosine + dy * sine;
            const float localY = -dx * sine + dy * cosine;
            hit = std::abs(localX) <= unit.depth * 0.5f + 3.f && std::abs(localY) <= unit.frontage * 0.5f + 3.f;
        }
        if (!hit) {
            continue;
        }
        const float distance = dx * dx + dy * dy;
        if (best == kNoUnit || distance < bestDistance) {
            best = unit.id;
            bestDistance = distance;
        }
    }
    return best;
}

std::vector<Command> applyPointer(OrderState &state, const Battle &battle, const PointerSample &sample, float mapW,
                                  float mapH, float alpha) {
    std::vector<Command> issued;
    const bool inside = insideBattlefield(sample.x, sample.y);
    const float scale = mapCamera(battlefieldRect(), mapW, mapH).scale;
    const Vec2 world = worldFromLogical(sample.x, sample.y, mapW, mapH);

    if (sample.type == PointerSample::Type::Move) {
        if (sample.device == PointerSample::Device::Mouse) {
            if (!inside) {
                state.hover.reset();
            } else {
                updateHover(state, battle, world.x, world.y, scale, alpha);
            }
        }
        const bool canDrag = state.pressing && state.pressButton == kPointerLeft &&
                             state.pressDevice == PointerSample::Device::Mouse && ordersOpen(battle);
        if (canDrag) {
            const float dx = sample.x - state.pressX;
            const float dy = sample.y - state.pressY;
            if (!state.box && std::hypot(dx, dy) > kDragThresholdPx) {
                const Vec2 down = worldFromLogical(state.pressX, state.pressY, mapW, mapH);
                if (pickUnit(battle, down.x, down.y, scale, alpha) == kNoUnit) {
                    state.box = true;
                    state.boxX0 = state.pressX;
                    state.boxY0 = state.pressY;
                }
            }
            if (state.box) {
                state.boxX1 = sample.x;
                state.boxY1 = sample.y;
            }
        }
        return issued;
    }

    if (sample.type == PointerSample::Type::Down) {
        if (!inside) {
            return issued;
        }
        state.pressing = true;
        state.pressX = sample.x;
        state.pressY = sample.y;
        state.pressButton = sample.button;
        state.pressDevice = sample.device;
        state.box = false;
        return issued;
    }

    if (!state.pressing && !state.box) {
        return issued;
    }
    const bool wasBox = state.box;
    const float boxX0 = state.boxX0;
    const float boxY0 = state.boxY0;
    const float boxX1 = sample.x;
    const float boxY1 = sample.y;
    const int button = state.pressButton;
    const PointerSample::Device device = state.pressDevice;
    const bool shift = sample.shift;
    state.pressing = false;
    state.box = false;
    if (!ordersOpen(battle)) {
        return issued;
    }
    if (wasBox) {
        boxSelect(state, battle, boxX0, boxY0, boxX1, boxY1, shift, mapW, mapH, alpha);
        return issued;
    }
    if (!inside) {
        return issued;
    }
    handleClick(state, battle, world, button, device, shift, scale, alpha, issued);
    return issued;
}

OrderEffect applyKey(OrderState &state, const Battle &battle, OrderKey key, const std::vector<float> &speeds) {
    OrderEffect effect;
    if (key == OrderKey::Space) {
        effect.toggleRun = true;
        return effect;
    }
    if (key == OrderKey::Speed1 || key == OrderKey::Speed2 || key == OrderKey::Speed3) {
        const std::size_t index = key == OrderKey::Speed1 ? 0 : key == OrderKey::Speed2 ? 1 : 2;
        if (index < speeds.size()) {
            effect.speed = speeds[index];
        }
        return effect;
    }
    if (!ordersOpen(battle)) {
        return effect;
    }
    if (key == OrderKey::Cancel) {
        state.selected.clear();
        state.pending.reset();
        state.box = false;
        state.pressing = false;
        return effect;
    }
    if (key == OrderKey::All) {
        state.selected.clear();
        for (const Unit &unit : battle.units()) {
            if (unit.side == Side::Ming && isFighting(unit.state)) {
                state.selected.push_back(unit.id);
            }
        }
        return effect;
    }
    if (!orderable(state, battle)) {
        return effect;
    }
    Command::Type type = Command::Type::Hold;
    if (key == OrderKey::Retreat) {
        type = Command::Type::Retreat;
    } else if (key == OrderKey::Split) {
        type = Command::Type::Split;
    } else if (key == OrderKey::Join) {
        type = Command::Type::Join;
    } else if (key != OrderKey::Hold) {
        return effect;
    }
    effect.commands.push_back(makeCommand(state, battle, type));
    return effect;
}

std::vector<Command> confirmPending(OrderState &state, const Battle &battle) {
    std::vector<Command> issued;
    if (!state.pending || !ordersOpen(battle)) {
        return issued;
    }
    issueAttack(state, battle, *state.pending, issued);
    return issued;
}

void noteEvents(OrderState &state, const Battle &battle, std::span<const Event> events) {
    for (const Event &event : events) {
        if (event.type == Event::Type::Split) {
            const auto found = std::find(state.selected.begin(), state.selected.end(), event.a);
            if (found != state.selected.end() && !containsId(state.selected, event.b)) {
                state.selected.insert(found + 1, event.b);
            }
        } else if (event.type == Event::Type::Merged) {
            const bool had = containsId(state.selected, event.a) || containsId(state.selected, event.b);
            eraseId(state.selected, event.b);
            if (had && !containsId(state.selected, event.a)) {
                state.selected.push_back(event.a);
            }
        }
    }
    dropGone(state, battle);
}

bool isPointerAction(std::string_view action) {
    return action == "click" || action == "rclick" || action == "shiftclick" || action == "touchclick" ||
           action == "move" || action == "drag" || action == "shiftdrag";
}

std::vector<PointerSample> samplesForScript(const ScriptEvent &event) {
    std::istringstream args(event.arg);
    if (event.action == "click" || event.action == "rclick" || event.action == "shiftclick" ||
        event.action == "touchclick") {
        float x = 0.f;
        float y = 0.f;
        if (!(args >> x >> y)) {
            return {};
        }
        const int button = event.action == "rclick" ? kPointerRight : kPointerLeft;
        const auto device = event.action == "touchclick" ? PointerSample::Device::Touch : PointerSample::Device::Mouse;
        const bool shift = event.action == "shiftclick";
        std::vector<PointerSample> samples;
        if (device == PointerSample::Device::Mouse) {
            samples.push_back(sampleAt(PointerSample::Type::Move, device, kPointerLeft, shift, x, y));
        }
        samples.push_back(sampleAt(PointerSample::Type::Down, device, button, shift, x, y));
        samples.push_back(sampleAt(PointerSample::Type::Up, device, button, shift, x, y));
        return samples;
    }
    if (event.action == "move") {
        float x = 0.f;
        float y = 0.f;
        if (!(args >> x >> y)) {
            return {};
        }
        return {sampleAt(PointerSample::Type::Move, PointerSample::Device::Mouse, kPointerLeft, false, x, y)};
    }
    if (event.action == "drag" || event.action == "shiftdrag") {
        float x0 = 0.f;
        float y0 = 0.f;
        float x1 = 0.f;
        float y1 = 0.f;
        if (!(args >> x0 >> y0 >> x1 >> y1)) {
            return {};
        }
        const bool shift = event.action == "shiftdrag";
        return {sampleAt(PointerSample::Type::Down, PointerSample::Device::Mouse, kPointerLeft, shift, x0, y0),
                sampleAt(PointerSample::Type::Move, PointerSample::Device::Mouse, kPointerLeft, shift, x1, y1),
                sampleAt(PointerSample::Type::Up, PointerSample::Device::Mouse, kPointerLeft, shift, x1, y1)};
    }
    return {};
}

std::optional<OrderKey> orderKeyFromScript(const std::string &action, const std::string &arg) {
    if (action != "press" && action != "down") {
        return std::nullopt;
    }
    const std::string name = lowerKey(arg);
    if (name == "space") {
        return OrderKey::Space;
    }
    if (name == "escape" || name == "esc") {
        return OrderKey::Cancel;
    }
    if (name == "1") {
        return OrderKey::Speed1;
    }
    if (name == "2") {
        return OrderKey::Speed2;
    }
    if (name == "3") {
        return OrderKey::Speed3;
    }
    if (name == "h") {
        return OrderKey::Hold;
    }
    if (name == "r") {
        return OrderKey::Retreat;
    }
    if (name == "f") {
        return OrderKey::Split;
    }
    if (name == "g") {
        return OrderKey::Join;
    }
    if (name == "a") {
        return OrderKey::All;
    }
    return std::nullopt;
}

std::string commandText(const Command &command) {
    const char *name = "Move";
    switch (command.type) {
    case Command::Type::Move:
        name = "Move";
        break;
    case Command::Type::Attack:
        name = "Attack";
        break;
    case Command::Type::Hold:
        name = "Hold";
        break;
    case Command::Type::Retreat:
        name = "Retreat";
        break;
    case Command::Type::Split:
        name = "Split";
        break;
    case Command::Type::Join:
        name = "Join";
        break;
    case Command::Type::Spawn:
        name = "Spawn";
        break;
    case Command::Type::Rout:
        name = "Rout";
        break;
    }
    std::ostringstream out;
    out << name;
    for (std::size_t i = 0; i < command.units.size(); ++i) {
        out << (i == 0 ? ' ' : ',') << command.units[i];
    }
    if (command.type == Command::Type::Spawn) {
        out << ' ' << command.unitType << ' ' << command.point.x << ' ' << command.point.y;
    } else if (command.type == Command::Type::Move) {
        out << ' ' << command.point.x << ' ' << command.point.y;
    } else if (command.type == Command::Type::Attack) {
        out << ' ' << command.target;
    }
    return out.str();
}

ControlPlayback playControlScript(Battle &battle, std::istream &script, float timeScale,
                                  const std::vector<float> &speeds, float mapW, float mapH, float speed) {
    ControlPlayback play;
    play.speed = speed;
    const std::vector<ScriptEvent> events = parseScript(script);
    float clock = 0.f;
    for (const ScriptEvent &event : events) {
        if (play.running && !battle.isOver() && event.time > clock) {
            const double rate =
                static_cast<double>(timeScale) / static_cast<double>(kTickSeconds) * static_cast<double>(play.speed);
            if (rate > 0.0) {
                const int done = static_cast<int>(std::floor(static_cast<double>(clock) * rate + 1e-6));
                const int next = static_cast<int>(std::floor(static_cast<double>(event.time) * rate + 1e-6));
                for (int step = done; step < next; ++step) {
                    battle.step();
                    const std::vector<Event> drained = battle.drainEvents();
                    noteEvents(play.state, battle, drained);
                }
            }
        }
        clock = std::max(clock, event.time);
        if (const std::optional<OrderKey> key = orderKeyFromScript(event.action, event.arg)) {
            OrderEffect effect = applyKey(play.state, battle, *key, speeds);
            if (effect.toggleRun) {
                toggleRunning(play.running, battle);
            }
            if (effect.speed) {
                play.speed = *effect.speed;
            }
            for (const Command &command : effect.commands) {
                play.commands.push_back(command);
                battle.submit(command);
            }
        }
        for (const PointerSample &sample : samplesForScript(event)) {
            for (const Command &command : applyPointer(play.state, battle, sample, mapW, mapH, 0.f)) {
                play.commands.push_back(command);
                battle.submit(command);
            }
        }
    }
    return play;
}

} // namespace tq
