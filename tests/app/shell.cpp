#include "app/Launch.h"
#include "app/SaveData.h"
#include "app/Script.h"
#include "app/Synth.h"
#include "app/TextStyle.h"
#include "app/Viewport.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

using namespace tq;

TEST_CASE("text style keeps brush off Korean and English titles") {
    CHECK(textStyle("zh-Hant", TextRole::Title).face == FontFace::Brush);
    CHECK_FALSE(textStyle("zh-Hant", TextRole::Title).bold);
    CHECK(textStyle("zh-Hant", TextRole::Banner).face == FontFace::Brush);
    CHECK(textStyle("zh-Hant", TextRole::Floater).face == FontFace::Brush);
    CHECK(textStyle("zh-Hant", TextRole::Body).face == FontFace::Body);
    CHECK_FALSE(textStyle("zh-Hant", TextRole::Body).bold);
    CHECK(textStyle("zh-Hant", TextRole::Glyph).face == FontFace::Brush);

    for (const char *language : {"ko", "en", "ja"}) {
        const TextStyle title = textStyle(language, TextRole::Title);
        CHECK(title.face == FontFace::Body);
        CHECK(title.bold);
        CHECK(textStyle(language, TextRole::Banner).face == FontFace::Body);
        CHECK(textStyle(language, TextRole::Floater).bold);
        CHECK(textStyle(language, TextRole::Glyph).face == FontFace::Brush);
        CHECK_FALSE(textStyle(language, TextRole::Glyph).bold);
        CHECK(textStyle(language, TextRole::Body).face == FontFace::Body);
        CHECK_FALSE(textStyle(language, TextRole::Body).bold);
    }
}

TEST_CASE("window fit stays inside 90 percent of the desktop") {
    const WindowFit small = fitWindow(1366, 768);
    CHECK(small.minW == 1229);
    CHECK(small.minH == 691);
    CHECK(small.defaultW == 1229);
    CHECK(small.defaultH == 691);

    const WindowFit hd = fitWindow(1920, 1080);
    CHECK(hd.minW == 1280);
    CHECK(hd.minH == 800);
    CHECK(hd.defaultW == 1555);
    CHECK(hd.defaultH == 972);
    CHECK(hd.defaultW <= 1728);
    CHECK(hd.defaultH <= 972);

    const WindowFit wide = fitWindow(2560, 1440);
    CHECK(wide.defaultW == 1600);
    CHECK(wide.defaultH == 1000);
    CHECK(wide.minW == 1280);
    CHECK(wide.minH == 800);
}

TEST_CASE("battlefield centers the map below the bar and left of the panel") {
    const ScreenRect field = battlefieldRect();
    CHECK(field.x == doctest::Approx(0.f));
    CHECK(field.y == doctest::Approx(56.f));
    CHECK(field.w == doctest::Approx(1220.f));
    CHECK(field.h == doctest::Approx(944.f));

    const MapCamera camera = mapCamera(field, 880.f, 1100.f);
    const float scale = std::min((field.w - 20.f) / 880.f, (field.h - 20.f) / 1100.f);
    CHECK(camera.scale == doctest::Approx(scale));
    CHECK(camera.originX == doctest::Approx(field.x + (field.w - 880.f * scale) * 0.5f));
    CHECK(camera.originY == doctest::Approx(field.y + (field.h - 1100.f * scale) * 0.5f));

    const Vec2 origin = worldFromLogical(camera.originX, camera.originY, 880.f, 1100.f);
    CHECK(origin.x == doctest::Approx(0.f));
    CHECK(origin.y == doctest::Approx(0.f));
    const Vec2 center = worldFromLogical(camera.originX + 440.f * scale, camera.originY + 550.f * scale, 880.f, 1100.f);
    CHECK(center.x == doctest::Approx(440.f));
    CHECK(center.y == doctest::Approx(550.f));
}

TEST_CASE("map rng matches the prototype mulberry32 stream") {
    MapRng rng(1593);
    CHECK(rng() == doctest::Approx(0.7827615959).epsilon(1e-6));
    CHECK(rng() == doctest::Approx(0.9247896769).epsilon(1e-6));
    CHECK(rng() == doctest::Approx(0.5550176848).epsilon(1e-6));
}

TEST_CASE("letterbox keeps the logical aspect") {
    const auto aspect = [](float pixelW, float pixelH, Viewport box) { return (box.w * pixelW) / (box.h * pixelH); };
    const float logical = 1600.f / 1000.f;

    const Viewport same = letterbox(1600.f, 1000.f, 1600.f, 1000.f);
    CHECK(same.x == doctest::Approx(0.f));
    CHECK(same.w == doctest::Approx(1.f));
    CHECK(same.h == doctest::Approx(1.f));

    const Viewport wide = letterbox(1600.f, 800.f, 1600.f, 1000.f);
    CHECK(wide.y == doctest::Approx(0.f));
    CHECK(wide.h == doctest::Approx(1.f));
    CHECK(wide.w < 1.f);
    CHECK(aspect(1600.f, 800.f, wide) == doctest::Approx(logical));

    const Viewport tall = letterbox(1280.f, 1000.f, 1600.f, 1000.f);
    CHECK(tall.x == doctest::Approx(0.f));
    CHECK(tall.w == doctest::Approx(1.f));
    CHECK(tall.h < 1.f);
    CHECK(aspect(1280.f, 1000.f, tall) == doctest::Approx(logical));
}

TEST_CASE("script parses comments, order, shots and resize") {
    std::istringstream in("# note\n\n1 resize 1600 800\n0.2 shot out/a.png\n0.5 press F6\n0.1 quit\n");
    const std::vector<ScriptEvent> events = parseScript(in);
    REQUIRE(events.size() == 4);
    CHECK(events[0].action == "quit");
    CHECK(events[1].action == "shot");
    CHECK(events[1].arg == "out/a.png");
    CHECK(events[2].action == "press");
    CHECK(events[2].arg == "F6");
    CHECK(events[3].action == "resize");
    CHECK(events[3].arg == "1600 800");
}

TEST_CASE("synthesized effects match the prototype durations and are not silent") {
    const SynthSet sfx = synthesizeSfx();
    const auto samples = [](double seconds) { return static_cast<std::size_t>(std::lround(seconds * kSampleRate)); };
    const auto audible = [](const std::vector<std::int16_t> &pcm) {
        int peak = 0;
        for (std::int16_t sample : pcm) {
            peak = std::max(peak, std::abs(static_cast<int>(sample)));
        }
        return peak;
    };

    CHECK(sfx.thud.size() == samples(0.45));
    CHECK(sfx.crack.size() == samples(0.16));
    CHECK(sfx.rout.size() == samples(0.6));
    CHECK(sfx.drum.size() == samples(0.76) + samples(0.75));
    CHECK(audible(sfx.thud) > 1000);
    CHECK(audible(sfx.crack) > 200);
    CHECK(audible(sfx.drum) > 1000);
    CHECK(audible(sfx.rout) > 200);

    float last = -1.f;
    CHECK(allowCrack(1.f, last));
    CHECK_FALSE(allowCrack(1.05f, last));
    CHECK(allowCrack(1.1f, last));
}

TEST_CASE("settings round trip") {
    const std::filesystem::path file = std::filesystem::temp_directory_path() / "tieqi-settings-test.txt";
    SaveData saved;
    saved.language = "ko";
    saved.volume = 40.f;
    saved.speed = 2.f;
    saved.reducedMotion = true;
    REQUIRE(saved.save(file));

    SaveData loaded;
    REQUIRE(loaded.load(file));
    CHECK(loaded.language == "ko");
    CHECK(loaded.volume == doctest::Approx(40.f));
    CHECK(loaded.speed == doctest::Approx(2.f));
    CHECK(loaded.reducedMotion);

    std::ofstream broken(file, std::ios::trunc);
    broken << "volume=nope\nlanguage=xx\nreduced_motion=true\n";
    broken.close();
    SaveData fallback;
    REQUIRE(fallback.load(file));
    CHECK(fallback.language == "zh-Hant");
    CHECK(fallback.volume == doctest::Approx(70.f));
    CHECK(fallback.reducedMotion);
    std::filesystem::remove(file);
}

TEST_CASE("asset root prefers the override, then a data directory, then the executable") {
    const AssetRootRequest overridden{"D:/override", "D:/cwd", "D:/exe", true};
    CHECK(resolveAssetRoot(overridden) == std::filesystem::path("D:/override"));

    const AssetRootRequest current{"", "D:/cwd", "D:/exe", true};
    CHECK(resolveAssetRoot(current) == std::filesystem::path("D:/cwd"));

    const AssetRootRequest executable{"", "D:/cwd", "D:/exe", false};
    CHECK(resolveAssetRoot(executable) == std::filesystem::path("D:/exe"));
}

TEST_CASE("launch options accept the documented flags") {
    char arg0[] = "tieqi";
    char scenarioFlag[] = "--scenario";
    char scenario[] = "duel_flat";
    char seedFlag[] = "--seed";
    char seed[] = "7";
    char scaleFlag[] = "--time-scale";
    char scale[] = "5";
    char *argv[] = {arg0, scenarioFlag, scenario, seedFlag, seed, scaleFlag, scale};
    const LaunchOptions options = parseLaunch(7, argv);
    CHECK(options.scenario == "duel_flat");
    CHECK(options.scenarioSet);
    CHECK(options.seed == 7);
    CHECK(options.seedSet);
    REQUIRE(options.timeScale.has_value());
    CHECK(*options.timeScale == doctest::Approx(5.f));
}
