#pragma once

#include "app/Orders.h"
#include "core/defs/ScenarioDef.h"
#include "data/Strings.h"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace tq {

// Except floaters and the settlement overlay, every event is a report, a banner, or both. Floaters are A5.
enum class EventPresentation : std::uint8_t { Report, Banner, ReportAndBanner, Floater, Settlement };

enum class LogTone : std::uint8_t { Neutral, Ming, Jp };

struct LogEntry {
    std::string time;
    std::string text;
    LogTone tone = LogTone::Neutral;
    bool important = false;
};

struct BannerCopy {
    int serial = 0;
    std::string title;
    std::string subtitle;
};

struct ModRow {
    std::string label;
    std::string figure;
    enum class Ink : std::uint8_t { Plain, Cinnabar, Ash, Note } ink = Ink::Plain;
};

struct PreviewCard {
    bool visible = false;
    bool confirm = false;
    std::string title;
    std::string ratio;
    std::string verdict;
    Verdict verdictKind = Verdict::Even;
    std::vector<ModRow> rows;
};

enum class TagInk : std::uint8_t { Plain, Hot, Warn, Bad, Ok };

struct RosterTag {
    std::string text;
    TagInk ink = TagInk::Plain;
};

struct RosterCard {
    enum class Mode : std::uint8_t { Empty, One, Many } mode = Mode::Empty;
    std::string glyph;
    bool general = false;
    std::string name;
    std::string typeLine;
    std::string state;
    UnitState stateKind = UnitState::Steady;
    float menFraction = 0.f;
    std::string menText;
    float moraleFraction = 0.f;
    std::string moraleText;
    std::vector<RosterTag> tags;
    std::string manyHead;
    struct Row {
        std::string glyph;
        bool general = false;
        std::string name;
        std::string state;
        UnitState stateKind = UnitState::Steady;
    };
    std::vector<Row> rows;
    bool showOrders = false;
    bool canSplit = false;
    bool canJoin = false;
};

struct ArmyCard {
    std::string ming;
    std::string jp;
    float mingFraction = 0.f;
    float jpFraction = 0.f;
};

struct EndStat {
    std::string label;
    std::string value;
};

struct EndCard {
    std::string eyebrow;
    std::string grade;
    bool victory = false;
    std::string body;
    std::vector<EndStat> stats;
    std::vector<std::string> events;
};

class Journal {
  public:
    void reset();
    void append(const Battle &battle, const StringTable &strings, std::span<const Event> events);

    const std::vector<LogEntry> &lines() const { return lines_; }
    const std::vector<LogEntry> &important() const { return important_; }
    const std::string &objectiveKey() const { return objective_; }
    const BannerCopy &banner() const { return banner_; }

  private:
    std::vector<LogEntry> lines_;
    std::vector<LogEntry> important_;
    std::vector<UnitId> volleys_;
    std::string objective_ = "obj.briefing";
    BannerCopy banner_;
};

EventPresentation eventPresentation(Event::Type type);

std::string groupNumber(long value);
std::string formatClock(float battleSeconds);
std::string formatShichen(const StringTable &strings, std::string_view startClock, float battleSeconds);
float displaySeconds(std::uint32_t tick, float alpha);

std::string unitLabel(const StringTable &strings, const Battle &battle, UnitId id);

std::vector<ModRow> modifierRows(const StringTable &strings, const Battle &battle, UnitId attacker, UnitId target,
                                 const Preview &preview);
PreviewCard previewCard(const StringTable &strings, const Battle &battle, const OrderState &orders);
RosterCard rosterCard(const StringTable &strings, const Battle &battle, const OrderState &orders);
ArmyCard armyCard(const StringTable &strings, const Battle &battle);

std::vector<std::string> withdrawalCompany(const Battle &battle, const StringTable &strings, const ExitDef &exit);
EndCard formatEnd(const StringTable &strings, const BattleResult &result, const BattleStats &stats, float enlistedMing,
                  float battleSeconds, const std::vector<std::string> &followers,
                  const std::vector<LogEntry> &important);

} // namespace tq
