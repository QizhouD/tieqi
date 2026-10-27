#include "core/rules/Terrain.h"
#include "core/defs/MapDef.h"

#include <doctest/doctest.h>

using namespace tq;

namespace {

MapDef byeokjegwanMap() {
    MapDef map;
    map.width = 880.f;
    map.height = 1100.f;
    map.paddies = {
        {230.f, 250.f, 150.f, 150.f}, {500.f, 280.f, 160.f, 140.f}, {250.f, 720.f, 150.f, 150.f},
        {500.f, 760.f, 150.f, 130.f}, {330.f, 520.f, 80.f, 70.f},
    };
    map.hills = {
        {40.f, 180.f, 150.f, 250.f},  {70.f, 620.f, 160.f, 300.f},  {40.f, 1000.f, 150.f, 200.f},
        {850.f, 220.f, 150.f, 270.f}, {820.f, 700.f, 160.f, 280.f}, {860.f, 1040.f, 140.f, 160.f},
        {250.f, 460.f, 80.f, 60.f},   {640.f, 600.f, 80.f, 70.f},   {520.f, 985.f, 120.f, 68.f},
    };
    map.road.halfWidth = 14.f;
    map.road.points = {{440.f, -10.f}, {425.f, 240.f}, {455.f, 520.f}, {430.f, 800.f}, {460.f, 1110.f}};
    return map;
}

} // namespace

TEST_CASE("terrainAt on the Byeokjegwan map") {
    const MapDef map = byeokjegwanMap();
    CHECK(terrainAt(map, {700.f, 470.f}) == Terrain::Open);
    CHECK(terrainAt(map, {300.f, 300.f}) == Terrain::Paddy);
    CHECK(terrainAt(map, {560.f, 985.f}) == Terrain::Hill);
    CHECK(terrainAt(map, {440.f, 100.f}) == Terrain::Road);
    CHECK(terrainAt(map, {230.f, 250.f}) == Terrain::Paddy);
}

TEST_CASE("terrainSpeed follows the class table") {
    CHECK(terrainSpeed(UnitClass::Cavalry, Terrain::Paddy) == doctest::Approx(0.6f));
    CHECK(terrainSpeed(UnitClass::Infantry, Terrain::Paddy) == doctest::Approx(0.7f));
    CHECK(terrainSpeed(UnitClass::Cavalry, Terrain::Hill) == doctest::Approx(0.8f));
    CHECK(terrainSpeed(UnitClass::Infantry, Terrain::Hill) == doctest::Approx(0.85f));
    CHECK(terrainSpeed(UnitClass::Cavalry, Terrain::Road) == doctest::Approx(1.1f));
    CHECK(terrainSpeed(UnitClass::Infantry, Terrain::Road) == doctest::Approx(1.1f));
    CHECK(terrainSpeed(UnitClass::Cavalry, Terrain::Open) == doctest::Approx(1.f));
    CHECK(terrainSpeed(UnitClass::Infantry, Terrain::Open) == doctest::Approx(1.f));
}
