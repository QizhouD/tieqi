#include "app/Report.h"
#include "data/Load.h"

#include <doctest/doctest.h>

#include <cstdio>
#include <filesystem>
#include <string>

using namespace tq;

namespace {

GameData loadByeok() { return loadGameData(std::filesystem::path(TQ_SOURCE_DIR) / "data", "byeokjegwan.json"); }

Battle makeBattle(const GameData &data) {
    BattleOptions options;
    options.aiEnabled = false;
    return Battle(data.defs, data.scenario, 1, options);
}

UnitId named(const Battle &battle, const char *id) {
    const std::optional<UnitId> found = battle.sceneUnit(id);
    REQUIRE(found.has_value());
    return *found;
}

} // namespace

TEST_CASE("clock and shichen follow the prototype") {
    const StringTable strings = loadByeok().zhHant;
    CHECK(formatClock(0.f) == "00:00");
    CHECK(formatClock(125.f) == "02:05");
    CHECK(formatShichen(strings, "07:00", 0.f) == "辰初");
    CHECK(formatShichen(strings, "07:00", 3600.f) == "戌初");
    CHECK(groupNumber(999) == "999");
    CHECK(groupNumber(1000) == "1,000");
    CHECK(groupNumber(1234567) == "1,234,567");
}

TEST_CASE("every event is a report, a banner, or left to effects") {
    CHECK(eventPresentation(Event::Type::Impact) == EventPresentation::Report);
    CHECK(eventPresentation(Event::Type::TeppoVolley) == EventPresentation::Report);
    CHECK(eventPresentation(Event::Type::Breakaway) == EventPresentation::Report);
    CHECK(eventPresentation(Event::Type::Broken) == EventPresentation::Report);
    CHECK(eventPresentation(Event::Type::Rallied) == EventPresentation::Report);
    CHECK(eventPresentation(Event::Type::Merged) == EventPresentation::Report);
    CHECK(eventPresentation(Event::Type::Split) == EventPresentation::Report);
    CHECK(eventPresentation(Event::Type::Dispersed) == EventPresentation::Report);
    CHECK(eventPresentation(Event::Type::Fled) == EventPresentation::Report);
    CHECK(eventPresentation(Event::Type::Withdrawn) == EventPresentation::Report);
    CHECK(eventPresentation(Event::Type::GeneralDown) == EventPresentation::Report);
    CHECK(eventPresentation(Event::Type::GrandAssault) == EventPresentation::Report);
    CHECK(eventPresentation(Event::Type::ScoutWarning) == EventPresentation::ReportAndBanner);
    CHECK(eventPresentation(Event::Type::PhaseChanged) == EventPresentation::ReportAndBanner);
    CHECK(eventPresentation(Event::Type::Reinforcement) == EventPresentation::ReportAndBanner);
    CHECK(eventPresentation(Event::Type::ThreeEyedVolley) == EventPresentation::Floater);
    CHECK(eventPresentation(Event::Type::Shattered) == EventPresentation::Floater);
    CHECK(eventPresentation(Event::Type::MomentumGained) == EventPresentation::Floater);
    CHECK(eventPresentation(Event::Type::BattleEnded) == EventPresentation::Settlement);
}

TEST_CASE("journal lines match the string table") {
    const GameData data = loadByeok();
    const StringTable &strings = data.zhHant;
    Battle battle = makeBattle(data);
    const UnitId ming = named(battle, "jiading_left");
    const UnitId jp = named(battle, "van_teppo_right");
    const UnitId general = named(battle, "li_rusong");

    Event impact;
    impact.type = Event::Type::Impact;
    impact.tick = 10;
    impact.a = ming;
    impact.b = jp;
    impact.value = 2.66f;
    impact.side = ContactSide::Front;
    impact.flags = kBreaks;

    Event broken;
    broken.type = Event::Type::Broken;
    broken.tick = 10;
    broken.a = jp;
    broken.value = 3.f;

    Event volley;
    volley.type = Event::Type::TeppoVolley;
    volley.tick = 4;
    volley.a = jp;
    volley.b = ming;

    Event phase;
    phase.type = Event::Type::PhaseChanged;
    phase.tick = 1;
    phase.value = 1.f;

    Event scout;
    scout.type = Event::Type::ScoutWarning;
    scout.tick = 20;

    Event failed;
    failed.type = Event::Type::GrandAssault;
    failed.tick = 30;
    failed.flags = kAssaultFailed;

    Event quiet;
    quiet.type = Event::Type::ThreeEyedVolley;
    quiet.tick = 5;
    quiet.a = ming;
    quiet.b = jp;

    Journal journal;
    journal.append(battle, strings, std::vector<Event>{impact, broken, volley, volley, phase, scout, failed, quiet});

    bool sawImpact = false;
    bool sawBroken = false;
    bool sawVolley = false;
    int volleys = 0;
    for (const LogEntry &line : journal.lines()) {
        if (line.text.find("衝擊 ×2.7") != std::string::npos) {
            sawImpact = true;
            CHECK(line.important);
            CHECK(line.text == "家丁左營 自正面衝擊 立花鐵砲右隊，衝擊 ×2.7");
            CHECK(line.time == "00:08");
        }
        if (line.text.find("散作") != std::string::npos) {
            sawBroken = true;
            CHECK(line.text == "立花鐵砲右隊 陣腳被衝破，崩潰，散作三股");
            CHECK(line.important);
        }
        if (line.text.find("齊射") != std::string::npos) {
            sawVolley = true;
            volleys += 1;
        }
    }
    CHECK(sawImpact);
    CHECK(sawBroken);
    CHECK(sawVolley);
    CHECK(volleys == 1);
    CHECK(journal.objectiveKey() == "obj.warning");
    CHECK(journal.banner().title == "斥候來報");
    CHECK(journal.lines().size() == 6);

    Event phase3;
    phase3.type = Event::Type::PhaseChanged;
    phase3.value = 3.f;
    const std::size_t before = journal.lines().size();
    journal.append(battle, strings, std::vector<Event>{phase3});
    CHECK(journal.lines().size() == before);
    CHECK(journal.banner().title == "三 · 撤退");
    CHECK(journal.objectiveKey() == "obj.phase3");

    Event down;
    down.type = Event::Type::GeneralDown;
    down.a = general;
    journal.append(battle, strings, std::vector<Event>{down});
    CHECK(journal.lines().front().text == "李如松 陣亡");
    CHECK(journal.lines().front().important);

    float mingMen = 0.f;
    for (const Unit &unit : battle.units()) {
        if (unit.side == Side::Ming) {
            mingMen += unit.men;
        }
    }
    CHECK(battle.enlisted(Side::Ming) == doctest::Approx(mingMen));
    const ArmyCard army = armyCard(strings, battle);
    CHECK(army.ming.find(groupNumber(std::lround(mingMen))) != std::string::npos);
    CHECK(army.jp.find("本隊未到") != std::string::npos);
}

TEST_CASE("preview card and end numbers come from the battle") {
    const GameData data = loadByeok();
    const StringTable &strings = data.zhHant;
    Battle battle = makeBattle(data);
    OrderState orders;
    orders.selected = {named(battle, "jiading_left"), named(battle, "jiading_right")};
    orders.hover = named(battle, "van_teppo_right");
    const PreviewCard card = previewCard(strings, battle, orders);
    CHECK(card.visible);
    CHECK_FALSE(card.confirm);
    CHECK(card.title.find("衝擊預估") != std::string::npos);
    CHECK(card.title.find("家丁左營") != std::string::npos);
    const Preview preview = previewAttack(battle, orders.selected.front(), *orders.hover);
    char ratio[32];
    std::snprintf(ratio, sizeof(ratio), "%.2f", static_cast<double>(preview.ratio));
    CHECK(card.ratio == std::string("\u00d7") + ratio);
    CHECK_FALSE(card.verdict.empty());

    orders.pending = orders.hover;
    CHECK(previewCard(strings, battle, orders).confirm);

    Preview grind;
    grind.mods.push_back(Modifier{ModKind::Melee, 1.5f});
    grind.mods.push_back(Modifier{ModKind::CavalryGrind, 0.85f});
    grind.mods.push_back(Modifier{ModKind::Flank, 1.8f});
    grind.mods.push_back(Modifier{ModKind::Paddy, 0.5f});
    const std::vector<ModRow> rows = modifierRows(strings, battle, orders.selected.front(), *orders.hover, grind);
    REQUIRE(rows.size() == 4);
    CHECK(rows[0].figure == "1.50");
    CHECK(rows[0].ink == ModRow::Ink::Plain);
    CHECK(rows[1].figure.empty());
    CHECK(rows[1].ink == ModRow::Ink::Note);
    CHECK(rows[2].figure == "\u00d71.80");
    CHECK(rows[2].ink == ModRow::Ink::Cinnabar);
    CHECK(rows[3].figure == "\u00d70.50");
    CHECK(rows[3].ink == ModRow::Ink::Ash);

    BattleResult result;
    result.over = true;
    result.end = BattleEnd::Withdrawn;
    result.grade = Grade::Minor;
    result.saved = 1234.f;
    result.rate = 0.4f;
    BattleStats stats;
    stats.lostMing = 10.f;
    stats.lostJp = 20.f;
    stats.brokenJp = 3;
    stats.bestImpact = 2.f;
    stats.exitedMen = 1.f;
    LogEntry important;
    important.time = "01:00";
    important.text = "要事";
    important.important = true;
    const EndCard end = formatEnd(strings, result, stats, 3000.f, 125.f, {"楊元左營"}, {important});
    CHECK(end.grade == "小勝");
    CHECK(end.victory);
    CHECK(end.eyebrow == "戰報 · 歷時 02:05");
    CHECK(end.stats.size() == 5);
    CHECK(end.stats[0].value == "1,234 / 3,000");
    CHECK(end.stats[1].value == "10");
    CHECK(end.stats[2].value == "20");
    CHECK(end.stats[3].value == "3 隊");
    CHECK(end.stats[4].value == "\u00d72.0");
    CHECK(end.body.find("李如松撤往坡州") != std::string::npos);
    CHECK(end.body.find("楊元左營") != std::string::npos);
    CHECK(end.events.size() == 1);
}

TEST_CASE("roster shows the selected company") {
    const GameData data = loadByeok();
    Battle battle = makeBattle(data);
    OrderState orders;
    orders.selected = {named(battle, "jiading_left")};
    const RosterCard one = rosterCard(data.zhHant, battle, orders);
    CHECK(one.mode == RosterCard::Mode::One);
    CHECK(one.name == "家丁左營");
    CHECK(one.showOrders);
    CHECK(one.canSplit);
    CHECK_FALSE(one.canJoin);
    CHECK(one.menText.find("400") != std::string::npos);

    orders.selected.push_back(named(battle, "li_rusong"));
    const RosterCard many = rosterCard(data.zhHant, battle, orders);
    CHECK(many.mode == RosterCard::Mode::Many);
    CHECK(many.rows.size() == 2);
}
