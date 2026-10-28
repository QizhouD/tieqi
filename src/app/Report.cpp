#include "app/Report.h"

#include "core/math/Geometry.h"
#include "core/rules/Constants.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace tq {
namespace {

std::string fill(std::string text, std::initializer_list<std::pair<std::string_view, std::string_view>> pairs) {
    for (const auto &[key, value] : pairs) {
        const std::string token = std::string("{") + std::string(key) + "}";
        for (std::size_t at = 0; (at = text.find(token, at)) != std::string::npos;) {
            text.replace(at, token.size(), value);
            at += value.size();
        }
    }
    return text;
}

std::string fixed(float value, int digits) {
    std::ostringstream out;
    out.setf(std::ios::fixed);
    out.precision(digits);
    out << static_cast<double>(value);
    return out.str();
}

std::string plain(long value) {
    std::ostringstream out;
    out << value;
    return out.str();
}

std::vector<std::string> codepoints(std::string_view text) {
    std::vector<std::string> out;
    for (std::size_t index = 0; index < text.size();) {
        const auto lead = static_cast<unsigned char>(text[index]);
        std::size_t length = 1;
        if ((lead & 0xE0) == 0xC0) {
            length = 2;
        } else if ((lead & 0xF0) == 0xE0) {
            length = 3;
        } else if ((lead & 0xF8) == 0xF0) {
            length = 4;
        }
        if (index + length > text.size()) {
            length = 1;
        }
        out.emplace_back(text.substr(index, length));
        index += length;
    }
    return out;
}

const Unit *findUnit(const Battle &battle, UnitId id) {
    if (id >= battle.units().size()) {
        return nullptr;
    }
    return &battle.unit(id);
}

std::string baseLabel(const StringTable &strings, const Unit &unit) { return strings.text(unit.labelKey); }

std::string withSuffix(const StringTable &strings, const Unit &unit) {
    const std::string base = baseLabel(strings, unit);
    const NameParts parts = displayName(unit);
    if (parts.suffix == NameSuffix::None) {
        return base;
    }
    if (parts.suffix == NameSuffix::Combined) {
        return fill(strings.text("suffix.combined"), {{"name", base}});
    }
    const char *key = parts.suffix == NameSuffix::Remnant ? "suffix.remnant" : "suffix.detachment";
    return fill(strings.text(key), {{"name", base}, {"n", strings.text("num." + plain(parts.numeral))}});
}

bool fightingState(UnitState state) { return state == UnitState::Steady || state == UnitState::Wavering; }

bool aliveState(UnitState state) { return state != UnitState::Gone; }

LogTone toneOf(Side side) { return side == Side::Ming ? LogTone::Ming : LogTone::Jp; }

LogTone otherSide(Side side) { return side == Side::Ming ? LogTone::Jp : LogTone::Ming; }

std::string sideText(const StringTable &strings, ContactSide side) {
    if (side == ContactSide::Flank) {
        return strings.text("side.flank");
    }
    if (side == ContactSide::Rear) {
        return strings.text("side.rear");
    }
    return strings.text("side.front");
}

std::string partsWord(const StringTable &strings, int parts) {
    if (parts == 2) {
        return strings.text("num.liang");
    }
    return strings.text("num." + plain(parts));
}

bool lastOfOrigin(const Battle &battle, UnitId id) {
    const Unit *unit = findUnit(battle, id);
    if (unit == nullptr) {
        return true;
    }
    const UnitId origin = unit->origin == kNoUnit ? unit->id : unit->origin;
    for (const Unit &other : battle.units()) {
        if (other.id == id || other.state == UnitState::Gone) {
            continue;
        }
        const UnitId otherOrigin = other.origin == kNoUnit ? other.id : other.origin;
        if (otherOrigin == origin) {
            return false;
        }
    }
    return true;
}

const char *stateKey(UnitState state) {
    switch (state) {
    case UnitState::Wavering:
        return "state.wavering";
    case UnitState::Broken:
        return "state.broken";
    case UnitState::Routing:
        return "state.routing";
    case UnitState::Steady:
    case UnitState::Gone:
        return "state.steady";
    }
    return "state.steady";
}

const char *modKey(ModKind kind) {
    switch (kind) {
    case ModKind::Impact:
        return "mod.impact";
    case ModKind::Melee:
        return "mod.melee";
    case ModKind::Defense:
        return "mod.defense";
    case ModKind::Speed:
        return "mod.speed";
    case ModKind::Flank:
        return "mod.flank";
    case ModKind::Rear:
        return "mod.rear";
    case ModKind::Wavering:
        return "mod.wavering";
    case ModKind::Routing:
        return "mod.routing";
    case ModKind::Reloading:
        return "mod.reloading";
    case ModKind::Unformed:
        return "mod.unformed";
    case ModKind::Paddy:
        return "mod.paddy";
    case ModKind::Uphill:
        return "mod.uphill";
    case ModKind::Pike:
        return "mod.pike";
    case ModKind::PikeWall:
        return "mod.pike_wall";
    case ModKind::Momentum:
        return "mod.momentum";
    case ModKind::OwnWavering:
        return "mod.own_wavering";
    case ModKind::Strength:
        return "mod.strength";
    case ModKind::CavalryGrind:
        return "mod.cavalry_grind";
    case ModKind::Pursuit:
        return "mod.pursuit";
    }
    return "mod.impact";
}

bool baseMod(ModKind kind) { return kind == ModKind::Impact || kind == ModKind::Melee; }

std::string typeName(const StringTable &strings, const Battle &battle, const Unit &unit) {
    return strings.text(battle.typeOf(unit).nameKey);
}

const Unit *volleyThreat(const Battle &battle, const Unit &attacker, const Unit &target) {
    for (const Unit &other : battle.units()) {
        if (other.side == attacker.side || !fightingState(other.state)) {
            continue;
        }
        const UnitType &type = battle.typeOf(other);
        if (type.range <= 0.f || other.reloadSec > kVolleyReloadSec || other.stillSec < kVolleyStillSec) {
            continue;
        }
        const float nearTarget = std::hypot(other.pos.x - target.pos.x, other.pos.y - target.pos.y);
        const float approach = std::hypot(other.pos.x - attacker.pos.x, other.pos.y - attacker.pos.y);
        if (nearTarget >= kVolleyNearTarget || approach >= type.range + kVolleyApproach) {
            continue;
        }
        const float aim = std::atan2(attacker.pos.y - other.pos.y, attacker.pos.x - other.pos.x);
        if (std::fabs(angDiff(aim, other.facing)) >= kVolleyAim) {
            continue;
        }
        return &other;
    }
    return nullptr;
}

std::string noteText(const StringTable &strings, const Battle &battle, const Unit &attacker, const Unit &target,
                     NoteKind note) {
    if (note == NoteKind::Recovering) {
        const long sec = static_cast<long>(std::ceil(static_cast<double>(attacker.recoverSec)));
        return fill(strings.text("note.recovering"), {{"sec", plain(sec)}});
    }
    if (note == NoteKind::TooClose) {
        return strings.text("note.too_close");
    }
    if (note == NoteKind::OwnWavering) {
        return strings.text("note.wavering");
    }
    const Unit *threat = volleyThreat(battle, attacker, target);
    const std::string name = threat != nullptr ? withSuffix(strings, *threat) : std::string{};
    return fill(strings.text("note.volley"), {{"name", name}});
}

std::vector<UnitId> orderableIds(const Battle &battle, const OrderState &orders) {
    std::vector<UnitId> ids;
    for (UnitId id : orders.selected) {
        const Unit *unit = findUnit(battle, id);
        if (unit != nullptr && unit->side == Side::Ming && fightingState(unit->state)) {
            ids.push_back(id);
        }
    }
    return ids;
}

const Unit *nearGeneral(const Battle &battle, const Unit &unit) {
    const Unit *found = nullptr;
    for (const Unit &other : battle.units()) {
        if (other.id == unit.id || other.side != unit.side || !fightingState(other.state)) {
            continue;
        }
        const UnitType &type = battle.typeOf(other);
        if (!type.general || !(type.aura > 0.f)) {
            continue;
        }
        const float distance = std::hypot(other.pos.x - unit.pos.x, other.pos.y - unit.pos.y);
        if (distance < type.aura && (found == nullptr || other.id < found->id)) {
            found = &other;
        }
    }
    return found;
}

void addTag(std::vector<RosterTag> &tags, const std::string &text, TagInk ink) { tags.push_back(RosterTag{text, ink}); }

std::string joined(const std::vector<std::string> &names, std::string_view separator) {
    std::string out;
    for (std::size_t index = 0; index < names.size(); ++index) {
        if (index > 0) {
            out += separator;
        }
        out += names[index];
    }
    return out;
}

const char *gradeKey(Grade grade) {
    switch (grade) {
    case Grade::Decisive:
        return "grade.decisive";
    case Grade::Minor:
        return "grade.minor";
    case Grade::Pyrrhic:
        return "grade.pyrrhic";
    case Grade::Defeat:
    case Grade::None:
        return "grade.defeat";
    }
    return "grade.defeat";
}

float clockHour(std::string_view startClock) {
    int hour = 7;
    int minute = 0;
    const std::size_t colon = startClock.find(':');
    try {
        if (colon == std::string_view::npos) {
            if (!startClock.empty()) {
                hour = std::stoi(std::string(startClock));
            }
        } else {
            hour = std::stoi(std::string(startClock.substr(0, colon)));
            minute = std::stoi(std::string(startClock.substr(colon + 1)));
        }
    } catch (const std::exception &) {
        hour = 7;
        minute = 0;
    }
    return static_cast<float>(hour) + static_cast<float>(minute) / 60.f;
}

bool seenVolley(const std::vector<UnitId> &volleys, UnitId id) {
    return std::find(volleys.begin(), volleys.end(), id) != volleys.end();
}

} // namespace

EventPresentation eventPresentation(Event::Type type) {
    switch (type) {
    case Event::Type::ThreeEyedVolley:
    case Event::Type::Shattered:
    case Event::Type::MomentumGained:
        return EventPresentation::Floater;
    case Event::Type::BattleEnded:
        return EventPresentation::Settlement;
    case Event::Type::ScoutWarning:
    case Event::Type::PhaseChanged:
    case Event::Type::Reinforcement:
        return EventPresentation::ReportAndBanner;
    case Event::Type::Impact:
    case Event::Type::TeppoVolley:
    case Event::Type::Breakaway:
    case Event::Type::Broken:
    case Event::Type::Rallied:
    case Event::Type::Merged:
    case Event::Type::Split:
    case Event::Type::Dispersed:
    case Event::Type::Fled:
    case Event::Type::Withdrawn:
    case Event::Type::GeneralDown:
    case Event::Type::GrandAssault:
        return EventPresentation::Report;
    }
    return EventPresentation::Report;
}

std::string groupNumber(long value) {
    const bool negative = value < 0;
    unsigned long magnitude = static_cast<unsigned long>(negative ? -value : value);
    std::string digits = plain(static_cast<long>(magnitude));
    std::string grouped;
    const int length = static_cast<int>(digits.size());
    for (int index = 0; index < length; ++index) {
        if (index > 0 && (length - index) % 3 == 0) {
            grouped.push_back(',');
        }
        grouped.push_back(digits[static_cast<std::size_t>(index)]);
    }
    if (negative) {
        grouped.insert(grouped.begin(), '-');
    }
    return grouped;
}

std::string formatClock(float battleSeconds) {
    int seconds = 0;
    if (battleSeconds > 0.f) {
        seconds = static_cast<int>(std::floor(static_cast<double>(battleSeconds)));
    }
    const int minutes = seconds / 60;
    seconds %= 60;
    std::ostringstream out;
    out.width(2);
    out.fill('0');
    out << minutes;
    out << ':';
    out.width(2);
    out.fill('0');
    out << seconds;
    return out.str();
}

std::string formatShichen(const StringTable &strings, std::string_view startClock, float battleSeconds) {
    const float hour = clockHour(startClock) + std::max(0.f, battleSeconds) * 12.f / 3600.f;
    const float turned = std::fmod(hour + 1.f, 24.f);
    const float wrapped = turned < 0.f ? turned + 24.f : turned;
    const int index = static_cast<int>(std::floor(wrapped / 2.f));
    const auto names = codepoints(strings.text("ui.shichen_names"));
    const std::string name = names.empty() ? std::string{} : names[static_cast<std::size_t>(index) % names.size()];
    const bool early = std::fmod(wrapped, 2.f) < 1.f;
    return name + strings.text(early ? "ui.shichen_early" : "ui.shichen_late");
}

float displaySeconds(std::uint32_t tick, float alpha) {
    if (tick == 0) {
        return 0.f;
    }
    const float along = std::clamp(alpha, 0.f, 1.f);
    return (static_cast<float>(tick - 1) + along) * kTickSeconds;
}

std::string unitLabel(const StringTable &strings, const Battle &battle, UnitId id) {
    const Unit *unit = findUnit(battle, id);
    if (unit == nullptr) {
        return {};
    }
    return withSuffix(strings, *unit);
}

std::vector<ModRow> modifierRows(const StringTable &strings, const Battle &battle, UnitId attackerId, UnitId targetId,
                                 const Preview &preview) {
    const Unit *attacker = findUnit(battle, attackerId);
    const Unit *target = findUnit(battle, targetId);
    std::vector<ModRow> rows;
    int meleeCount = 0;
    for (const Modifier &mod : preview.mods) {
        ModRow row;
        std::string name;
        if (mod.kind == ModKind::Impact && attacker != nullptr) {
            name = typeName(strings, battle, *attacker);
        } else if (mod.kind == ModKind::Defense && target != nullptr) {
            name = typeName(strings, battle, *target);
        } else if (mod.kind == ModKind::Melee) {
            const Unit *named = meleeCount == 0 ? attacker : target;
            if (named != nullptr) {
                name = typeName(strings, battle, *named);
            }
            meleeCount += 1;
        }
        row.label = fill(strings.text(modKey(mod.kind)), {{"name", name}});
        if (mod.kind == ModKind::CavalryGrind) {
            row.ink = ModRow::Ink::Note;
            rows.push_back(std::move(row));
            continue;
        }
        // The attacker's own impact or melee is a plain number. Resist and the defender's melee stay multipliers.
        if (baseMod(mod.kind) && (mod.kind != ModKind::Melee || meleeCount == 1)) {
            row.figure = fixed(mod.value, 2);
            row.ink = ModRow::Ink::Plain;
        } else {
            row.figure = std::string("\u00d7") + fixed(mod.value, 2);
            if (mod.value > 1.001f) {
                row.ink = ModRow::Ink::Cinnabar;
            } else if (mod.value < 0.999f) {
                row.ink = ModRow::Ink::Ash;
            }
        }
        rows.push_back(std::move(row));
    }
    if (attacker != nullptr && target != nullptr) {
        for (NoteKind note : preview.notes) {
            ModRow row;
            row.label = noteText(strings, battle, *attacker, *target, note);
            row.ink = ModRow::Ink::Note;
            rows.push_back(std::move(row));
        }
    }
    return rows;
}

PreviewCard previewCard(const StringTable &strings, const Battle &battle, const OrderState &orders) {
    PreviewCard card;
    const std::vector<UnitId> attackers = orderableIds(battle, orders);
    const std::optional<UnitId> targetId = orders.pending ? orders.pending : orders.hover;
    if (attackers.empty() || !targetId) {
        return card;
    }
    const Unit *target = findUnit(battle, *targetId);
    const Unit *attacker = findUnit(battle, attackers.front());
    if (attacker == nullptr || target == nullptr || !aliveState(target->state) || target->side == attacker->side) {
        return card;
    }
    const Preview preview = previewAttack(battle, attacker->id, target->id);
    card.visible = true;
    card.confirm = orders.pending.has_value();
    card.verdictKind = preview.verdict;
    const char *titleKey = preview.kind == Preview::Kind::Charge ? "ui.preview_charge" : "ui.preview_melee";
    card.title = fill(strings.text(titleKey), {{"side", sideText(strings, preview.side)}});
    if (attackers.size() > 1) {
        card.title += fill(strings.text("ui.preview_by"), {{"name", withSuffix(strings, *attacker)}});
    }
    card.ratio = std::string("\u00d7") + fixed(preview.ratio, 2);
    const char *verdictKey = "verdict.unfavored_melee";
    if (preview.verdict == Verdict::PikeWall) {
        verdictKey = "verdict.pike";
    } else if (preview.verdict == Verdict::Pursuit) {
        verdictKey = "verdict.pursuit";
    } else if (preview.verdict == Verdict::Breaks) {
        verdictKey = "verdict.breaks";
    } else if (preview.verdict == Verdict::Favored) {
        verdictKey = preview.kind == Preview::Kind::Charge ? "verdict.favored_charge" : "verdict.favored_melee";
    } else if (preview.verdict == Verdict::Even) {
        verdictKey = preview.kind == Preview::Kind::Charge ? "verdict.even_charge" : "verdict.even_melee";
    } else if (preview.kind == Preview::Kind::Charge) {
        verdictKey = "verdict.unfavored_charge";
    }
    card.verdict = strings.text(verdictKey);
    card.rows = modifierRows(strings, battle, attacker->id, target->id, preview);
    return card;
}

RosterCard rosterCard(const StringTable &strings, const Battle &battle, const OrderState &orders) {
    RosterCard card;
    std::vector<const Unit *> selected;
    for (UnitId id : orders.selected) {
        const Unit *unit = findUnit(battle, id);
        if (unit != nullptr && aliveState(unit->state)) {
            selected.push_back(unit);
        }
    }
    if (selected.empty()) {
        return card;
    }
    card.showOrders = true;
    for (const Unit *unit : selected) {
        if (canSplit(battle, unit->id)) {
            card.canSplit = true;
            break;
        }
    }
    card.canJoin = !joinGroups(battle, orders.selected).empty();
    if (selected.size() > 1) {
        card.mode = RosterCard::Mode::Many;
        float men = 0.f;
        for (const Unit *unit : selected) {
            men += unit->men;
            RosterCard::Row row;
            row.glyph = strings.text(battle.typeOf(*unit).glyphKey);
            row.general = battle.typeOf(*unit).general;
            row.name = withSuffix(strings, *unit);
            row.state = strings.text(stateKey(unit->state));
            row.stateKind = unit->state;
            card.rows.push_back(std::move(row));
        }
        card.manyHead = fill(strings.text("ui.selected"), {{"n", plain(static_cast<long>(selected.size()))},
                                                           {"men", groupNumber(std::lround(men))}});
        return card;
    }
    const Unit &unit = *selected.front();
    const UnitType &type = battle.typeOf(unit);
    card.mode = RosterCard::Mode::One;
    card.glyph = strings.text(type.glyphKey);
    card.general = type.general;
    card.name = withSuffix(strings, unit);
    card.typeLine = strings.text(type.nameKey);
    if (type.elite) {
        card.typeLine += " \u00b7 ";
        card.typeLine += strings.text("tag.elite");
    }
    card.state = strings.text(stateKey(unit.state));
    card.stateKind = unit.state;
    card.menFraction = unit.maxMen > 0.f ? std::clamp(unit.men / unit.maxMen, 0.f, 1.f) : 0.f;
    card.menText = groupNumber(std::lround(unit.men)) + " / " + groupNumber(std::lround(unit.maxMen));
    card.moraleFraction = std::clamp(unit.morale / 100.f, 0.f, 1.f);
    card.moraleText = groupNumber(std::lround(unit.morale));
    if (unit.charging) {
        addTag(card.tags, strings.text("tag.charging"), TagInk::Hot);
    }
    if (unit.breaking) {
        addTag(card.tags, strings.text("tag.breaking"), TagInk::Warn);
    }
    if (!unit.contacts.empty()) {
        addTag(card.tags, strings.text("tag.contact"), TagInk::Hot);
        bool flanked = false;
        for (UnitId contact : unit.contacts) {
            const Unit *other = findUnit(battle, contact);
            if (other == nullptr || !aliveState(other->state)) {
                continue;
            }
            const ContactSide side = sideOf(unit.pos, unit.facing, unit.depth, unit.frontage, other->pos);
            if (side != ContactSide::Front) {
                flanked = true;
            }
        }
        if (flanked) {
            addTag(card.tags, strings.text("tag.flanked"), TagInk::Bad);
        }
    }
    if (fightingState(unit.state) && unit.recoverSec > 0.f) {
        const long sec = static_cast<long>(std::ceil(static_cast<double>(unit.recoverSec)));
        addTag(card.tags, fill(strings.text("tag.recover"), {{"sec", plain(sec)}}), TagInk::Warn);
    }
    if (type.range > 0.f && fightingState(unit.state)) {
        if (unit.reloadSec > 0.f) {
            const long sec = static_cast<long>(std::ceil(static_cast<double>(unit.reloadSec)));
            addTag(card.tags, fill(strings.text("tag.reload"), {{"sec", plain(sec)}}), TagInk::Warn);
        } else if (unit.stillSec >= kChargeUnformedSec) {
            addTag(card.tags, strings.text("tag.loaded"), TagInk::Ok);
        } else {
            addTag(card.tags, strings.text("tag.moving_no_fire"), TagInk::Warn);
        }
    }
    if (unit.momentum != 0) {
        addTag(card.tags, fill(strings.text("tag.momentum"), {{"n", plain(unit.momentum)}}), TagInk::Hot);
    }
    if (unit.isFragment && fightingState(unit.state)) {
        addTag(card.tags, strings.text("tag.fragment"), TagInk::Warn);
    } else if (unit.origin != kNoUnit && fightingState(unit.state)) {
        addTag(card.tags, strings.text(unit.rejoin ? "tag.rejoin" : "tag.split"), TagInk::Plain);
    }
    if (const Unit *general = nearGeneral(battle, unit)) {
        addTag(card.tags, fill(strings.text("tag.aura"), {{"name", withSuffix(strings, *general)}}), TagInk::Ok);
    }
    const Terrain ground = battle.terrainAt(unit.pos);
    if (ground == Terrain::Paddy) {
        addTag(card.tags, strings.text("tag.paddy"), TagInk::Bad);
    } else if (ground == Terrain::Hill) {
        addTag(card.tags, strings.text("tag.hill"), TagInk::Plain);
    } else if (ground == Terrain::Road) {
        addTag(card.tags, strings.text("tag.road"), TagInk::Plain);
    }
    if (fightingState(unit.state)) {
        if (unit.order.type == Order::Type::Attack) {
            const Unit *target = findUnit(battle, unit.order.target);
            if (target != nullptr && aliveState(target->state)) {
                addTag(card.tags, fill(strings.text("tag.target"), {{"name", withSuffix(strings, *target)}}),
                       TagInk::Plain);
            } else {
                addTag(card.tags, strings.text("tag.hold"), TagInk::Plain);
            }
        } else if (unit.order.type == Order::Type::Move) {
            addTag(card.tags, strings.text("tag.moving"), TagInk::Plain);
        } else {
            addTag(card.tags, strings.text("tag.hold"), TagInk::Plain);
        }
    }
    return card;
}

ArmyCard armyCard(const StringTable &strings, const Battle &battle) {
    float mingMen = battle.stats().exitedMen;
    float jpMen = 0.f;
    int fightingMing = 0;
    int withdrawn = 0;
    for (const Unit &unit : battle.units()) {
        if (unit.gone == GoneReason::Withdrawn) {
            withdrawn += 1;
        }
        if (!aliveState(unit.state)) {
            continue;
        }
        if (unit.side == Side::Ming) {
            mingMen += unit.men;
            if (fightingState(unit.state)) {
                fightingMing += 1;
            }
        } else {
            jpMen += unit.men;
        }
    }
    const float mingMax = std::max(battle.enlisted(Side::Ming), 1.f);
    const float jpMax = std::max(battle.enlisted(Side::Jp), 1.f);
    ArmyCard card;
    card.mingFraction = std::clamp(mingMen / mingMax, 0.f, 1.f);
    card.jpFraction = std::clamp(jpMen / jpMax, 0.f, 1.f);
    const std::string mingCount = groupNumber(std::lround(mingMen));
    const std::string mingTotal = groupNumber(std::lround(battle.enlisted(Side::Ming)));
    const std::string jpCount = groupNumber(std::lround(jpMen));
    const std::string jpTotal = groupNumber(std::lround(battle.enlisted(Side::Jp)));
    if (withdrawn > 0) {
        card.ming = fill(strings.text("ui.army.ming_exited"),
                         {{"men", mingCount}, {"max", mingTotal}, {"n", plain(fightingMing)}, {"k", plain(withdrawn)}});
    } else {
        card.ming =
            fill(strings.text("ui.army.ming"), {{"men", mingCount}, {"max", mingTotal}, {"n", plain(fightingMing)}});
    }
    const char *jpKey = battle.phase() < 2 ? "ui.army.jp_waiting" : "ui.army.jp";
    card.jp = fill(strings.text(jpKey), {{"men", jpCount}, {"max", jpTotal}, {"n", plain(battle.stats().brokenJp)}});
    return card;
}

std::vector<std::string> withdrawalCompany(const Battle &battle, const StringTable &strings, const ExitDef &exit) {
    const Vec2 center{exit.rect.x + exit.rect.w * 0.5f, exit.rect.y + exit.rect.h * 0.5f};
    std::vector<std::string> names;
    for (const Unit &unit : battle.units()) {
        if (unit.side != Side::Ming || !fightingState(unit.state) || !unit.contacts.empty()) {
            continue;
        }
        const float distance = std::hypot(unit.pos.x - center.x, unit.pos.y - center.y);
        if (distance < exit.followRadius) {
            names.push_back(withSuffix(strings, unit));
        }
    }
    return names;
}

EndCard formatEnd(const StringTable &strings, const BattleResult &result, const BattleStats &stats, float enlistedMing,
                  float battleSeconds, const std::vector<std::string> &followers,
                  const std::vector<LogEntry> &important) {
    EndCard card;
    card.eyebrow = fill(strings.text("end.report"), {{"time", formatClock(battleSeconds)}});
    card.grade = strings.text(gradeKey(result.grade));
    card.victory = result.end == BattleEnd::Withdrawn || result.end == BattleEnd::Swept;
    if (result.end == BattleEnd::Swept) {
        card.body = strings.text("end.sweep");
    } else if (result.end == BattleEnd::Withdrawn) {
        card.body = strings.text("end.withdraw");
        if (!followers.empty()) {
            card.body += fill(strings.text("end.follow"), {{"names", joined(followers, strings.text("ui.list_sep"))}});
        }
    } else if (result.end == BattleEnd::CommanderKilled) {
        card.body = strings.text("end.dead");
    } else if (result.end == BattleEnd::CommanderFled) {
        card.body = strings.text("end.fled");
    } else {
        card.body = strings.text("end.rout");
    }
    const auto push = [&](const char *key, std::string value) {
        card.stats.push_back(EndStat{strings.text(key), std::move(value)});
    };
    push("ui.stat.withdrawn", groupNumber(std::lround(result.saved)) + " / " + groupNumber(std::lround(enlistedMing)));
    push("ui.stat.ming_lost", groupNumber(std::lround(stats.lostMing)));
    push("ui.stat.jp_lost", groupNumber(std::lround(stats.lostJp)));
    push("ui.stat.jp_broken", fill(strings.text("ui.stat.teams"), {{"n", plain(stats.brokenJp)}}));
    push("ui.stat.best_impact", std::string("\u00d7") + fixed(stats.bestImpact, 1));
    const std::size_t begin = important.size() > 7 ? important.size() - 7 : 0;
    for (std::size_t index = begin; index < important.size(); ++index) {
        card.events.push_back(important[index].time + " " + important[index].text);
    }
    return card;
}

void Journal::reset() {
    lines_.clear();
    important_.clear();
    volleys_.clear();
    objective_ = "obj.briefing";
    banner_ = {};
}

void Journal::append(const Battle &battle, const StringTable &strings, std::span<const Event> events) {
    std::vector<UnitId> impactBroken;
    for (const Event &event : events) {
        if (event.type == Event::Type::Impact && (event.flags & kBreaks) != 0) {
            impactBroken.push_back(event.b);
        }
    }
    for (const Event &event : events) {
        LogEntry line;
        line.time = formatClock(static_cast<float>(event.tick) * kTickSeconds);
        const Unit *primary = findUnit(battle, event.a);
        const Unit *second = findUnit(battle, event.b);
        bool keep = false;
        switch (event.type) {
        case Event::Type::Impact:
            if (primary != nullptr && second != nullptr) {
                const bool pike = (event.flags & kPikeFront) != 0;
                const char *key = pike ? "log.impact_pike" : "log.impact";
                line.text = fill(strings.text(key), {{"a", withSuffix(strings, *primary)},
                                                     {"b", withSuffix(strings, *second)},
                                                     {"side", sideText(strings, event.side)},
                                                     {"r", fixed(event.value, 1)}});
                line.tone = toneOf(primary->side);
                line.important = pike || (event.flags & kBreaks) != 0;
                keep = true;
            }
            break;
        case Event::Type::TeppoVolley:
            if (primary != nullptr && second != nullptr && !seenVolley(volleys_, event.a)) {
                volleys_.push_back(event.a);
                line.text = fill(strings.text("log.volley"),
                                 {{"a", withSuffix(strings, *primary)}, {"b", withSuffix(strings, *second)}});
                line.tone = toneOf(primary->side);
                keep = true;
            }
            break;
        case Event::Type::Breakaway:
            if (primary != nullptr) {
                line.text = fill(strings.text("log.breakaway"), {{"a", withSuffix(strings, *primary)}});
                line.tone = toneOf(primary->side);
                keep = true;
            }
            break;
        case Event::Type::Broken:
            if (primary != nullptr) {
                const bool fromImpact =
                    std::find(impactBroken.begin(), impactBroken.end(), event.a) != impactBroken.end();
                const int parts = static_cast<int>(std::lround(event.value));
                const char *key = "log.broken";
                if (fromImpact && parts > 1) {
                    key = "log.broken_impact_scatter";
                } else if (fromImpact) {
                    key = "log.broken_impact";
                } else if (parts > 1) {
                    key = "log.broken_scatter";
                }
                line.text = fill(strings.text(key),
                                 {{"a", withSuffix(strings, *primary)}, {"parts", partsWord(strings, parts)}});
                line.tone = otherSide(primary->side);
                line.important = true;
                keep = true;
            }
            break;
        case Event::Type::Rallied:
            if (primary != nullptr) {
                line.text = fill(strings.text("log.rallied"), {{"a", withSuffix(strings, *primary)}});
                line.tone = toneOf(primary->side);
                keep = true;
            }
            break;
        case Event::Type::Merged:
            if (primary != nullptr) {
                const bool remnant = second != nullptr && second->isFragment;
                const char *key = remnant ? "log.merged_frag" : "log.merged_join";
                line.text = fill(strings.text(key), {{"name", baseLabel(strings, *primary)},
                                                     {"n", groupNumber(std::lround(event.value))}});
                line.tone = toneOf(primary->side);
                keep = true;
            }
            break;
        case Event::Type::Split:
            if (primary != nullptr) {
                line.text = fill(strings.text("log.split"), {{"name", baseLabel(strings, *primary)},
                                                             {"n", groupNumber(std::lround(primary->men))}});
                line.tone = toneOf(primary->side);
                keep = true;
            }
            break;
        case Event::Type::Dispersed:
            if (primary != nullptr && lastOfOrigin(battle, event.a)) {
                if (primary->isFragment) {
                    line.text = fill(strings.text("log.dispersed_remnant"), {{"name", baseLabel(strings, *primary)}});
                } else {
                    line.text = fill(strings.text("log.dispersed"), {{"a", withSuffix(strings, *primary)}});
                }
                line.tone = otherSide(primary->side);
                keep = true;
            }
            break;
        case Event::Type::Fled:
            if (primary != nullptr && lastOfOrigin(battle, event.a)) {
                if (primary->isFragment) {
                    line.text = fill(strings.text("log.fled_remnant"), {{"name", baseLabel(strings, *primary)}});
                } else {
                    line.text = fill(strings.text("log.fled"), {{"a", withSuffix(strings, *primary)}});
                }
                line.tone = otherSide(primary->side);
                keep = true;
            }
            break;
        case Event::Type::Withdrawn:
            if (primary != nullptr) {
                line.text = fill(strings.text("log.withdrawn"),
                                 {{"a", withSuffix(strings, *primary)}, {"n", groupNumber(std::lround(primary->men))}});
                line.tone = LogTone::Ming;
                line.important = battle.typeOf(*primary).general;
                keep = true;
            }
            break;
        case Event::Type::GeneralDown:
            if (primary != nullptr) {
                line.text = fill(strings.text("log.general_down"), {{"a", withSuffix(strings, *primary)}});
                line.tone = otherSide(primary->side);
                line.important = true;
                keep = true;
            }
            break;
        case Event::Type::ScoutWarning:
            line.text = strings.text("log.scout");
            line.tone = LogTone::Ming;
            line.important = true;
            keep = true;
            objective_ = "obj.warning";
            banner_.serial += 1;
            banner_.title = strings.text("banner.scout");
            banner_.subtitle = strings.text("banner.scout_sub");
            break;
        case Event::Type::PhaseChanged: {
            const int phase = static_cast<int>(std::lround(event.value));
            if (phase == 1) {
                line.text = strings.text("log.phase1");
                line.tone = LogTone::Ming;
                line.important = true;
                keep = true;
                objective_ = "obj.phase1";
                banner_.serial += 1;
                banner_.title = strings.text("banner.phase1");
                banner_.subtitle = strings.text("banner.phase1_sub");
            } else if (phase == 2) {
                line.text = strings.text("log.phase2");
                line.tone = LogTone::Jp;
                line.important = true;
                keep = true;
                objective_ = "obj.phase2";
                banner_.serial += 1;
                banner_.title = strings.text("banner.phase2");
                banner_.subtitle = strings.text("banner.phase2_sub");
            } else if (phase == 3) {
                objective_ = "obj.phase3";
                banner_.serial += 1;
                banner_.title = strings.text("banner.phase3");
                banner_.subtitle = strings.text("banner.phase3_sub");
            }
            break;
        }
        case Event::Type::Reinforcement:
            line.text = strings.text("log.reinforcement");
            line.tone = LogTone::Ming;
            line.important = true;
            keep = true;
            banner_.serial += 1;
            banner_.title = strings.text("banner.reinforcement");
            banner_.subtitle = strings.text("banner.reinforcement_sub");
            break;
        case Event::Type::GrandAssault:
            if ((event.flags & kAssaultFailed) != 0) {
                line.text = strings.text("log.assault_failed");
                line.tone = LogTone::Ming;
            } else {
                line.text = strings.text("log.assault");
                line.tone = LogTone::Jp;
            }
            line.important = true;
            keep = true;
            break;
        case Event::Type::ThreeEyedVolley:
        case Event::Type::Shattered:
        case Event::Type::MomentumGained:
        case Event::Type::BattleEnded:
            break;
        }
        if (!keep) {
            continue;
        }
        lines_.insert(lines_.begin(), line);
        if (lines_.size() > 90) {
            lines_.pop_back();
        }
        if (line.important) {
            important_.push_back(line);
        }
    }
}

} // namespace tq
