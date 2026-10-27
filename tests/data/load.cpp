#include "data/Load.h"

#include <doctest/doctest.h>

#include <cstdint>
#include <filesystem>
#include <numbers>
#include <optional>
#include <string>

using namespace tq;

namespace {

std::filesystem::path sourceRoot() { return TQ_SOURCE_DIR; }

std::string unitsError(const char *file) {
    const std::filesystem::path path = sourceRoot() / "tests/data/bad" / file;
    try {
        loadUnits(path);
    } catch (const DataError &error) {
        return error.what();
    }
    return {};
}

std::string scenarioError(const char *file) {
    const StringTable zh = loadStrings(sourceRoot() / "data/strings/zh-Hant.json");
    const GameDefs defs = loadUnits(sourceRoot() / "data/units.json", &zh);
    try {
        loadScenario(sourceRoot() / "tests/data/bad" / file, defs, zh);
    } catch (const DataError &error) {
        return error.what();
    }
    return {};
}

void expect(const std::string &message, const char *fragment) {
    REQUIRE_FALSE(message.empty());
    CHECK_MESSAGE(message.find(fragment) != std::string::npos, message);
}

const UnitType &typeOf(const GameDefs &defs, const char *id) {
    const UnitType *type = defs.find(id);
    REQUIRE(type != nullptr);
    return *type;
}

float radians(float degrees) { return degrees * std::numbers::pi_v<float> / 180.f; }

} // namespace

TEST_CASE("byeokjegwan data matches the scenario file") {
    const GameData data = loadGameData(sourceRoot() / "data", "byeokjegwan.json");
    CHECK(data.defs.types.size() == 8);
    CHECK(data.defs.types[0].id == "jiading");
    CHECK(data.defs.types[2].id == "lirusong");
    CHECK(data.defs.types[3].id == "teppo");

    const UnitType &jiading = typeOf(data.defs, "jiading");
    CHECK(jiading.side == Side::Ming);
    CHECK(jiading.cls == UnitClass::Cavalry);
    CHECK(jiading.men == doctest::Approx(400.f));
    CHECK(jiading.walk == doctest::Approx(4.25f));
    CHECK(jiading.run == doctest::Approx(8.75f));
    CHECK(jiading.frontage == doctest::Approx(60.f));
    CHECK(jiading.depth == doctest::Approx(24.f));
    CHECK(jiading.threeEyedGun);
    CHECK(jiading.elite);
    CHECK_FALSE(jiading.general);

    const UnitType &teppo = typeOf(data.defs, "teppo");
    CHECK(teppo.cls == UnitClass::Infantry);
    CHECK(teppo.ai == AiRole::Gunner);
    CHECK(teppo.frontage == doctest::Approx(66.f));
    CHECK(teppo.depth == doctest::Approx(22.f));
    CHECK(teppo.range == doctest::Approx(175.f));
    CHECK(teppo.reload == doctest::Approx(48.f));
    CHECK(teppo.melee == doctest::Approx(0.55f));

    const UnitType &yari = typeOf(data.defs, "yari");
    CHECK(yari.pike);
    CHECK(yari.defense == doctest::Approx(1.3f));
    CHECK(typeOf(data.defs, "samurai").momentum);
    CHECK(typeOf(data.defs, "samurai").run == doctest::Approx(2.25f));
    CHECK(typeOf(data.defs, "tachibana").momentumBoost);
    CHECK(typeOf(data.defs, "tachibana").aura == doctest::Approx(150.f));
    CHECK(typeOf(data.defs, "lirusong").general);
    CHECK(typeOf(data.defs, "lirusong").aura == doctest::Approx(170.f));
    CHECK(typeOf(data.defs, "kobayakawa").run == doctest::Approx(0.f));
    CHECK(typeOf(data.defs, "kobayakawa").aura == doctest::Approx(240.f));
    CHECK(typeOf(data.defs, "kobayakawa").ai == AiRole::MainGeneral);

    const ScenarioDef &scenario = data.scenario;
    CHECK(scenario.id == "byeokjegwan");
    CHECK(scenario.nameKey == "scn.byeokjegwan");
    CHECK(scenario.startClock == "07:00");
    CHECK(scenario.map.width == doctest::Approx(880.f));
    CHECK(scenario.map.height == doctest::Approx(1100.f));
    CHECK(scenario.map.paddies.size() == 5);
    CHECK(scenario.map.hills.size() == 9);
    CHECK(scenario.map.hills.back().cx == doctest::Approx(520.f));
    CHECK(scenario.map.hills.back().cy == doctest::Approx(985.f));
    CHECK(scenario.map.road.halfWidth == doctest::Approx(14.f));
    CHECK(scenario.map.road.points.size() == 5);
    CHECK(scenario.map.postStation.x == doctest::Approx(470.f));
    CHECK(scenario.map.postStation.y == doctest::Approx(122.f));
    CHECK(scenario.map.labels.size() == 8);
    CHECK(scenario.map.labels[0].align == TextAlign::Left);
    CHECK(scenario.map.labels[4].faint);
    CHECK(scenario.commander == "li_rusong");
    CHECK(scenario.exit.rect.x == doctest::Approx(360.f));
    CHECK(scenario.exit.rect.h == doctest::Approx(48.f));
    CHECK(scenario.exit.followRadius == doctest::Approx(260.f));
    CHECK(scenario.exit.retreatYOpen == doctest::Approx(20.f));
    CHECK(scenario.exit.retreatYClosed == doctest::Approx(60.f));

    REQUIRE(scenario.deployment.size() == 11);
    CHECK(scenario.deployment[0].id == "li_rusong");
    const std::optional<std::uint16_t> li = data.defs.indexOf("lirusong");
    REQUIRE(li.has_value());
    CHECK(scenario.deployment[0].type == *li);
    CHECK(scenario.deployment[0].pos.x == doctest::Approx(440.f));
    CHECK(scenario.deployment[0].pos.y == doctest::Approx(250.f));
    CHECK(scenario.deployment[0].facing == doctest::Approx(radians(90.f)));
    CHECK_FALSE(scenario.deployment[0].vanguard);
    CHECK(scenario.deployment[6].id == "van_yari");
    CHECK(scenario.deployment[6].vanguard);
    CHECK(scenario.deployment[6].facing == doctest::Approx(radians(-90.f)));
    CHECK(scenario.deployment[10].id == "tachibana");

    REQUIRE(scenario.groups.size() == 3);
    CHECK(scenario.groups[0].id == "jp_main");
    CHECK(scenario.groups[1].id == "jp_wings");
    CHECK(scenario.groups[2].id == "ming_yang");
    REQUIRE(scenario.groups[0].units.size() == 6);
    REQUIRE(scenario.groups[1].units.size() == 2);
    REQUIRE(scenario.groups[2].units.size() == 2);
    CHECK(scenario.groups[0].units[0].id == "kobayakawa");
    CHECK(scenario.groups[0].units[0].pos.y == doctest::Approx(1140.f));
    CHECK(scenario.groups[0].units[0].rally.y == doctest::Approx(975.f));
    CHECK(scenario.groups[0].units[0].facing == doctest::Approx(radians(-90.f)));
    CHECK(scenario.groups[0].units[0].rallyFacing == doctest::Approx(radians(-90.f)));
    CHECK(scenario.groups[0].units[1].id == "main_teppo_left");
    CHECK(scenario.groups[0].units[5].id == "main_samurai");
    CHECK(scenario.groups[1].units[0].id == "wing_samurai_left");
    CHECK(scenario.groups[1].units[0].pos.x == doctest::Approx(-50.f));
    CHECK(scenario.groups[1].units[0].facing == doctest::Approx(radians(0.f)));
    CHECK(scenario.groups[1].units[1].facing == doctest::Approx(radians(180.f)));
    CHECK(scenario.groups[2].units[0].id == "yang_left");
    CHECK(scenario.groups[2].units[0].facing == doctest::Approx(radians(90.f)));
    CHECK(scenario.groups[2].units[1].id == "yang_right");

    CHECK(scenario.flow.enabled);
    CHECK(scenario.flow.scoutWarning.atSec == doctest::Approx(1280.f));
    CHECK(scenario.flow.scoutWarning.earlyAtSec == doctest::Approx(640.f));
    CHECK(scenario.flow.scoutWarning.earlyVanguardBroken == 3);
    CHECK(scenario.flow.scoutWarning.leadSec == doctest::Approx(160.f));
    CHECK(scenario.flow.phase2.spawnGroup == "jp_main");
    CHECK(scenario.flow.phase2.openExit);
    CHECK(scenario.flow.phase3.spawnGroup == "jp_wings");
    CHECK(scenario.flow.phase3.afterPhase2Sec == doctest::Approx(720.f));
    CHECK(scenario.flow.phase3.grandAssault.general == "kobayakawa");
    CHECK(scenario.flow.phase3.grandAssault.moraleAdd == doctest::Approx(12.f));
    CHECK(scenario.flow.phase3.grandAssault.capOverType == doctest::Approx(10.f));
    REQUIRE(scenario.flow.reinforcements.size() == 1);
    CHECK(scenario.flow.reinforcements[0].id == "yang_yuan");
    CHECK(scenario.flow.reinforcements[0].spawnGroup == "ming_yang");
    CHECK(scenario.flow.reinforcements[0].afterPhase3Sec == doctest::Approx(200.f));
    CHECK(scenario.flow.reinforcements[0].danger.enemyWithin == doctest::Approx(220.f));
    CHECK(scenario.flow.reinforcements[0].danger.menBelow == doctest::Approx(0.7f));
    CHECK(scenario.ai.vanguardHoldSec == doctest::Approx(360.f));
    CHECK(scenario.ai.vanguardTriggerDist == doctest::Approx(200.f));
    CHECK(scenario.result.decisiveRate == doctest::Approx(0.6f));
    CHECK(scenario.result.decisiveEnemyBroken == 6);
    CHECK(scenario.result.minorRate == doctest::Approx(0.35f));

    CHECK(data.config.timeScale == doctest::Approx(8.f));
    CHECK(data.config.playerSpeeds.size() == 3);
    CHECK(data.config.defaultSpeed == doctest::Approx(1.f));
    CHECK(data.config.maxTicksPerFrame == 8);
    CHECK(data.config.language == "zh-Hant");
    CHECK_FALSE(data.config.reducedMotion);
    CHECK(data.config.sound);
    CHECK(data.zhHant.text("ui.start") == "開戰");
    CHECK(data.zhHant.text("ui.tip.4").find("48") != std::string::npos);
    CHECK(data.zhHant.text("num.liang") == "兩");
}

TEST_CASE("duel_flat omits flow and keeps a flat map") {
    const StringTable zh = loadStrings(sourceRoot() / "data/strings/zh-Hant.json");
    const GameDefs defs = loadUnits(sourceRoot() / "data/units.json", &zh);
    const ScenarioDef scenario = loadScenario(sourceRoot() / "tests/data/scenarios/duel_flat.json", defs, zh);
    CHECK_FALSE(scenario.flow.enabled);
    CHECK(scenario.map.width == doctest::Approx(880.f));
    CHECK(scenario.map.height == doctest::Approx(1100.f));
    CHECK(scenario.map.paddies.empty());
    CHECK(scenario.map.hills.empty());
    CHECK(scenario.map.road.points.empty());
    REQUIRE(scenario.deployment.size() == 1);
    CHECK(scenario.deployment[0].id == "li_rusong");
    CHECK(scenario.deployment[0].pos.x == doctest::Approx(100.f));
    CHECK(scenario.deployment[0].pos.y == doctest::Approx(60.f));
    CHECK(scenario.deployment[0].facing == doctest::Approx(radians(90.f)));
    CHECK(scenario.groups.empty());
    CHECK(scenario.commander == "li_rusong");
}

TEST_CASE("missing string keys fall back to zh-Hant and then the key") {
    const StringTable zh = loadStrings(sourceRoot() / "data/strings/zh-Hant.json");
    StringTable en = loadStrings(sourceRoot() / "data/strings/en.json", &zh);
    CHECK(en.text("ui.start") == "開戰");
    REQUIRE_FALSE(en.warnings.empty());
    CHECK(en.warnings[0].find("ui.start") != std::string::npos);
    CHECK(en.text("no.such.key") == "no.such.key");
    CHECK(zh.text("game.title") == "餘暉：壬辰 1593");
}

TEST_CASE("unit files report each validation rule") {
    expect(unitsError("missing_field.json"), "missing_field.json:/units/jiading/men: 缺少字段 men");
    expect(unitsError("wrong_type.json"), "wrong_type.json:/units/jiading/men: 类型应为数字");
    expect(unitsError("unknown_field.json"), "unknown_field.json:/units/jiading: 未知字段 extra");
    expect(unitsError("schema.json"), "schema.json:/schema: schema 必须为 1（实际为 2）");
    expect(unitsError("bad_side.json"), "bad_side.json:/units/jiading/side: 未知取值 nope");
    expect(unitsError("men.json"), "men.json:/units/jiading/men: 必须大于 0（实际为 0）");
    expect(unitsError("walk.json"), "walk.json:/units/jiading/walk: 必须大于 0（实际为 0）");
    expect(unitsError("run.json"), "run.json:/units/jiading/run: 必须为 0 或不小于 walk（实际为 1）");
    expect(unitsError("turn.json"), "turn.json:/units/jiading/turn: 必须大于 0（实际为 0）");
    expect(unitsError("impact.json"), "impact.json:/units/jiading/impact: 必须大于或等于 0（实际为 -1）");
    expect(unitsError("melee.json"), "melee.json:/units/jiading/melee: 必须大于 0（实际为 0）");
    expect(unitsError("defense.json"), "defense.json:/units/jiading/defense: 必须大于 0（实际为 0）");
    expect(unitsError("armor.json"), "armor.json:/units/jiading/armor: 必须大于 0（实际为 0）");
    expect(unitsError("morale_low.json"), "morale_low.json:/units/jiading/morale: 必须大于 0（实际为 0）");
    expect(unitsError("morale_high.json"), "morale_high.json:/units/jiading/morale: 必须小于或等于 100（实际为 101）");
    expect(unitsError("frontage.json"), "frontage.json:/units/jiading/frontage: 必须大于 0（实际为 0）");
    expect(unitsError("depth.json"), "depth.json:/units/jiading/depth: 必须大于 0（实际为 0）");
    expect(unitsError("range_only.json"), "range_only.json:/units/jiading: 射程与装填必须同时出现");
    expect(unitsError("range_zero.json"), "range_zero.json:/units/jiading/range: 必须大于 0（实际为 0）");
    expect(unitsError("general_no_aura.json"), "general_no_aura.json:/units/jiading: 将领必须有 aura");
    expect(unitsError("aura_without_general.json"), "aura_without_general.json:/units/jiading/aura: 非将领不能有 aura");
    expect(unitsError("bad_trait.json"), "bad_trait.json:/units/jiading/traits/0: 未知特性 nope");
}

TEST_CASE("scenario files report each validation rule") {
    expect(scenarioError("unknown_type.json"), "unknown_type.json:/deployment/0/type: 兵种不存在（实际为 nope）");
    expect(scenarioError("duplicate_id.json"), "duplicate_id.json:/deployment/1/id: 单位 id 重复（实际为 li_rusong）");
    expect(scenarioError("commander.json"), "commander.json:/commander: 必须指向开局的明军将领（实际为 teppo）");
    expect(scenarioError("general_ref.json"),
           "general_ref.json:/flow/phase3/grand_assault/general: 必须指向组里的单位（实际为 nobody）");
    expect(scenarioError("spawn_group.json"), "spawn_group.json:/flow/phase2/spawn_group: 组不存在（实际为 missing）");
    expect(scenarioError("outside_map.json"), "outside_map.json:/deployment/0/pos: 开局单位必须在地图范围内");
    expect(scenarioError("spawn_far.json"), "spawn_far.json:/groups/jp_main/units/0/spawn: 出生点超出地图外 90 米");
    expect(scenarioError("missing_string.json"),
           "missing_string.json:/deployment/0/label_key: 字符串表缺少键 unit.nope");
}
