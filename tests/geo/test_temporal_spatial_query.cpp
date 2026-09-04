/**
 * Tests for TemporalSpatialQuery
 *
 * Verifies temporal-spatial queries: location at time T, entities in bbox
 * at time T, and entities within distance at time T.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "geo/temporal_spatial_query.h"
#include "temporal/system_versioned_table.h"

#include <cmath>
#include <thread>
#include <chrono>

using namespace themis::geo;
using namespace themisdb::temporal;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Build a minimal GeoJSON Point string.
static std::string makePointJson(double lon, double lat) {
    return "{\"type\":\"Point\",\"coordinates\":[" +
           std::to_string(lon) + "," + std::to_string(lat) + "]}";
}

/// Build a small square GeoJSON Polygon string centred at (lon, lat).
/// The outer ring has 5 vertices (first == last, closed ring).
static std::string makePolygonJson(double lon, double lat,
                                   double half_deg = 0.001) {
    auto v = [](double x, double y) {
        return "[" + std::to_string(x) + "," + std::to_string(y) + "]";
    };
    return "{\"type\":\"Polygon\",\"coordinates\":[[" +
           v(lon - half_deg, lat - half_deg) + "," +
           v(lon + half_deg, lat - half_deg) + "," +
           v(lon + half_deg, lat + half_deg) + "," +
           v(lon - half_deg, lat + half_deg) + "," +
           v(lon - half_deg, lat - half_deg) +
           "]]}";
}

/// Insert a document with a GeoJSON point in the "location" field.
static void insertEntity(SystemVersionedTable& table,
                         const std::string& key,
                         double lon, double lat) {
    nlohmann::json doc;
    doc["location"] = makePointJson(lon, lat);
    table.insert(key, doc);
}

// ---------------------------------------------------------------------------
// extractGeometry
// ---------------------------------------------------------------------------

TEST(TemporalSpatialQuery, ExtractGeometry_String_ParsesPoint) {
    VersionedDocument doc;
    doc.key = "e1";
    doc.data["location"] = makePointJson(13.4050, 52.5200); // Berlin

    auto geom = TemporalSpatialQuery::extractGeometry(doc);
    ASSERT_TRUE(geom.has_value());
    EXPECT_TRUE(geom->isPoint());
    ASSERT_FALSE(geom->coords.empty());
    EXPECT_NEAR(geom->coords[0].x, 13.4050, 1e-6);
    EXPECT_NEAR(geom->coords[0].y, 52.5200, 1e-6);
}

TEST(TemporalSpatialQuery, ExtractGeometry_EmbeddedObject_ParsesPoint) {
    VersionedDocument doc;
    doc.key = "e1";
    // Embed the GeoJSON geometry as a JSON object (not a string)
    doc.data["location"] = nlohmann::json::parse(makePointJson(2.3522, 48.8566));

    auto geom = TemporalSpatialQuery::extractGeometry(doc);
    ASSERT_TRUE(geom.has_value());
    EXPECT_TRUE(geom->isPoint());
    EXPECT_NEAR(geom->coords[0].x, 2.3522, 1e-6);
}

TEST(TemporalSpatialQuery, ExtractGeometry_FieldMissing_ReturnsNullopt) {
    VersionedDocument doc;
    doc.key = "e1";
    doc.data["name"] = "Alice";

    auto geom = TemporalSpatialQuery::extractGeometry(doc);
    EXPECT_FALSE(geom.has_value());
}

TEST(TemporalSpatialQuery, ExtractGeometry_InvalidJson_ReturnsNullopt) {
    VersionedDocument doc;
    doc.key = "e1";
    doc.data["location"] = "not-valid-geojson";

    auto geom = TemporalSpatialQuery::extractGeometry(doc);
    EXPECT_FALSE(geom.has_value());
}

TEST(TemporalSpatialQuery, ExtractGeometry_CustomField) {
    VersionedDocument doc;
    doc.key = "e1";
    doc.data["geo"] = makePointJson(0.0, 0.0);

    auto geom = TemporalSpatialQuery::extractGeometry(doc, "geo");
    EXPECT_TRUE(geom.has_value());
    auto missing = TemporalSpatialQuery::extractGeometry(doc); // default "location"
    EXPECT_FALSE(missing.has_value());
}

// ---------------------------------------------------------------------------
// locationAtTime
// ---------------------------------------------------------------------------

class LocationAtTimeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Insert entity at Berlin
        insertEntity(table, "bus1", 13.4050, 52.5200);
        t_berlin = now();

        // Small delay so t_berlin < t_paris
        std::this_thread::sleep_for(std::chrono::milliseconds(2));

        // Update entity to Paris
        nlohmann::json update;
        update["location"] = makePointJson(2.3522, 48.8566);
        table.update("bus1", update);
        t_paris = now();
    }

    SystemVersionedTable table{"vehicles", "node_a"};
    Timestamp t_berlin{0};
    Timestamp t_paris{0};
};

TEST_F(LocationAtTimeTest, Returns_Berlin_AtEarlyTime) {
    auto geom = TemporalSpatialQuery::locationAtTime(table, "bus1", t_berlin);
    ASSERT_TRUE(geom.has_value());
    ASSERT_FALSE(geom->coords.empty());
    EXPECT_NEAR(geom->coords[0].x, 13.4050, 1e-4);
    EXPECT_NEAR(geom->coords[0].y, 52.5200, 1e-4);
}

TEST_F(LocationAtTimeTest, Returns_Paris_AfterUpdate) {
    auto geom = TemporalSpatialQuery::locationAtTime(table, "bus1", t_paris);
    ASSERT_TRUE(geom.has_value());
    ASSERT_FALSE(geom->coords.empty());
    EXPECT_NEAR(geom->coords[0].x, 2.3522, 1e-4);
    EXPECT_NEAR(geom->coords[0].y, 48.8566, 1e-4);
}

TEST_F(LocationAtTimeTest, UnknownKey_ReturnsNullopt) {
    auto geom = TemporalSpatialQuery::locationAtTime(table, "missing", t_paris);
    EXPECT_FALSE(geom.has_value());
}

TEST_F(LocationAtTimeTest, BeforeInsert_ReturnsNullopt) {
    auto geom = TemporalSpatialQuery::locationAtTime(table, "bus1", kMinTimestamp);
    EXPECT_FALSE(geom.has_value());
}

// ---------------------------------------------------------------------------
// allLocationsAtTime
// ---------------------------------------------------------------------------

TEST(AllLocationsAtTime, ReturnsAllEntitiesWithGeometry) {
    SystemVersionedTable table{"fleet", "n"};
    insertEntity(table, "v1", 13.4, 52.5); // Berlin
    insertEntity(table, "v2", 2.35, 48.85); // Paris
    insertEntity(table, "v3_no_geo", 0, 0);
    // Overwrite v3's location field so it has no location
    table.update("v3_no_geo", {{"name", "no location doc"}});
    // Remove location from v3 by doing another update without it
    SystemVersionedTable table2{"fleet2", "n"};
    table2.insert("v1", {{"location", makePointJson(13.4, 52.5)}});
    table2.insert("v2", {{"location", makePointJson(2.35, 48.85)}});
    table2.insert("v3", {{"name", "no location"}});

    auto locs = TemporalSpatialQuery::allLocationsAtTime(table2, now());
    // v1 and v2 have location; v3 does not
    EXPECT_EQ(locs.size(), 2u);
    bool has_v1 = false, has_v2 = false;
    for (const auto& [k, g] : locs) {
        if (k == "v1") {
          has_v1 = true;
        }
        if (k == "v2") {
          has_v2 = true;
        }
    }
    EXPECT_TRUE(has_v1);
    EXPECT_TRUE(has_v2);
}

TEST(AllLocationsAtTime, EmptyTable_ReturnsEmpty) {
    SystemVersionedTable table{"empty", "n"};
    auto locs = TemporalSpatialQuery::allLocationsAtTime(table, now());
    EXPECT_TRUE(locs.empty());
}

// ---------------------------------------------------------------------------
// entitiesInBBoxAtTime
// ---------------------------------------------------------------------------

TEST(EntitiesInBBoxAtTime, FiltersCorrectly) {
    SystemVersionedTable table{"sites", "n"};
    // Berlin ≈ (13.4, 52.5) — inside Germany bounding box
    insertEntity(table, "berlin", 13.4050, 52.5200);
    // Paris ≈ (2.35, 48.85) — outside Germany bounding box
    insertEntity(table, "paris", 2.3522, 48.8566);

    // Rough bounding box of Germany
    MBR germany{5.8, 47.2, 15.1, 55.1};
    auto result = TemporalSpatialQuery::entitiesInBBoxAtTime(table, germany, now());

    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].key, "berlin");
}

TEST(EntitiesInBBoxAtTime, EmptyBBox_ReturnsEmpty) {
    SystemVersionedTable table{"sites", "n"};
    insertEntity(table, "berlin", 13.4050, 52.5200);

    // Degenerate bounding box (min > max)
    MBR empty{100.0, 100.0, 0.0, 0.0};
    auto result = TemporalSpatialQuery::entitiesInBBoxAtTime(table, empty, now());
    EXPECT_TRUE(result.empty());
}

TEST(EntitiesInBBoxAtTime, BBoxContainsAll_ReturnsAll) {
    SystemVersionedTable table{"cities", "n"};
    insertEntity(table, "a", 13.4, 52.5);
    insertEntity(table, "b", 2.35, 48.85);

    MBR world{-180.0, -90.0, 180.0, 90.0};
    auto result = TemporalSpatialQuery::entitiesInBBoxAtTime(table, world, now());
    EXPECT_EQ(result.size(), 2u);
}

// ---------------------------------------------------------------------------
// entitiesWithinDistanceAtTime
// ---------------------------------------------------------------------------

TEST(EntitiesWithinDistanceAtTime, FindsNearbyEntities) {
    SystemVersionedTable table{"stations", "n"};
    // Berlin Mitte
    insertEntity(table, "berlin_mitte", 13.4050, 52.5200);
    // Potsdamer Platz ~1.5 km south of Mitte
    insertEntity(table, "potsdamer_platz", 13.3760, 52.5096);
    // Paris — far away
    insertEntity(table, "paris", 2.3522, 48.8566);

    // Query within 5 km of Berlin Mitte
    auto result = TemporalSpatialQuery::entitiesWithinDistanceAtTime(
        table, 13.4050, 52.5200, 5000.0, now());

    EXPECT_EQ(result.size(), 2u);
    bool has_mitte = false, has_potsdamer = false;
    for (const auto& doc : result) {
        if (doc.key == "berlin_mitte") {
          has_mitte = true;
        }
        if (doc.key == "potsdamer_platz") {
          has_potsdamer = true;
        }
    }
    EXPECT_TRUE(has_mitte);
    EXPECT_TRUE(has_potsdamer);
}

TEST(EntitiesWithinDistanceAtTime, NegativeDistance_ReturnsEmpty) {
    SystemVersionedTable table{"t", "n"};
    insertEntity(table, "e1", 13.4, 52.5);

    auto result = TemporalSpatialQuery::entitiesWithinDistanceAtTime(
        table, 13.4, 52.5, -100.0, now());
    EXPECT_TRUE(result.empty());
}

TEST(EntitiesWithinDistanceAtTime, ZeroDistance_ReturnsEmpty) {
    SystemVersionedTable table{"t", "n"};
    insertEntity(table, "e1", 13.4, 52.5);
    // Different position — won't hit with distance 0
    auto result = TemporalSpatialQuery::entitiesWithinDistanceAtTime(
        table, 2.35, 48.85, 0.0, now());
    EXPECT_TRUE(result.empty());
}

// ---------------------------------------------------------------------------
// entitiesWithinDistanceAtTimeSorted
// ---------------------------------------------------------------------------

TEST(EntitiesWithinDistanceAtTimeSorted, SortedByDistance) {
    SystemVersionedTable table{"nodes", "n"};
    // Three points at increasing distances from Berlin Mitte (13.4050, 52.5200)
    insertEntity(table, "close",  13.4060, 52.5210); // ~0.1 km
    insertEntity(table, "medium", 13.4200, 52.5300); // ~2 km
    insertEntity(table, "far",    13.5000, 52.5500); // ~8 km

    auto result = TemporalSpatialQuery::entitiesWithinDistanceAtTimeSorted(
        table, 13.4050, 52.5200, 20000.0, now());

    ASSERT_EQ(result.size(), 3u);
    // Must be sorted ascending by distance
    EXPECT_LE(result[0].second, result[1].second);
    EXPECT_LE(result[1].second, result[2].second);
    EXPECT_EQ(result[0].first.key, "close");
}

TEST(EntitiesWithinDistanceAtTimeSorted, NegativeDistance_ReturnsEmpty) {
    SystemVersionedTable table{"t", "n"};
    insertEntity(table, "e1", 13.4, 52.5);

    auto result = TemporalSpatialQuery::entitiesWithinDistanceAtTimeSorted(
        table, 13.4, 52.5, -1.0, now());
    EXPECT_TRUE(result.empty());
}

// ---------------------------------------------------------------------------
// Non-Point geometry: centroid path
// ---------------------------------------------------------------------------

// The spec requires: "Point geometries use their single coordinate directly;
// all other types use GeometryInfo::computeCentroid()."

TEST(TemporalSpatialQuery, ExtractGeometry_Polygon_ParsesSuccessfully) {
    VersionedDocument doc;
    doc.key = "p1";
    // Square polygon centred at Berlin
    doc.data["location"] = makePolygonJson(13.4050, 52.5200);

    auto geom = TemporalSpatialQuery::extractGeometry(doc);
    ASSERT_TRUE(geom.has_value());
    EXPECT_TRUE(geom->isPolygon());
    EXPECT_FALSE(geom->rings.empty());
}

TEST(EntitiesInBBoxAtTime, PolygonGeometry_UsesCentroid) {
    SystemVersionedTable table{"poly_bbox", "n"};
    // Square polygon centred at Berlin (centroid ≈ Berlin)
    nlohmann::json doc;
    doc["location"] = makePolygonJson(13.4050, 52.5200);
    table.insert("berlin_poly", doc);
    // Square polygon centred at Paris
    nlohmann::json doc2;
    doc2["location"] = makePolygonJson(2.3522, 48.8566);
    table.insert("paris_poly", doc2);

    // Rough bounding box of Germany — contains Berlin centroid, not Paris
    MBR germany{5.8, 47.2, 15.1, 55.1};
    auto result = TemporalSpatialQuery::entitiesInBBoxAtTime(table, germany, now());

    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].key, "berlin_poly");
}

TEST(EntitiesWithinDistanceAtTime, PolygonGeometry_UsesCentroid) {
    SystemVersionedTable table{"poly_dist", "n"};
    // Square polygon centred at Berlin Mitte — centroid ≈ Berlin Mitte
    nlohmann::json doc;
    doc["location"] = makePolygonJson(13.4050, 52.5200);
    table.insert("berlin_poly", doc);
    // Square polygon centred at Paris — far away
    nlohmann::json doc2;
    doc2["location"] = makePolygonJson(2.3522, 48.8566);
    table.insert("paris_poly", doc2);

    // Query within 5 km of Berlin Mitte
    auto result = TemporalSpatialQuery::entitiesWithinDistanceAtTime(
        table, 13.4050, 52.5200, 5000.0, now());

    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].key, "berlin_poly");
}

// ---------------------------------------------------------------------------
// Temporal correctness: historical timestamps
// ---------------------------------------------------------------------------

TEST(AllLocationsAtTime, HistoricalTimestamp_ExcludesLaterInserts) {
    SystemVersionedTable table{"hist_all", "n"};
    insertEntity(table, "v1", 13.4, 52.5); // inserted first
    const Timestamp t1 = now();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    insertEntity(table, "v2", 2.35, 48.85); // inserted later

    // At t1 only v1 was alive
    auto locs_at_t1 = TemporalSpatialQuery::allLocationsAtTime(table, t1);
    ASSERT_EQ(locs_at_t1.size(), 1u);
    EXPECT_EQ(locs_at_t1[0].first, "v1");

    // At now() both are alive
    auto locs_now = TemporalSpatialQuery::allLocationsAtTime(table, now());
    EXPECT_EQ(locs_now.size(), 2u);
}

TEST(EntitiesInBBoxAtTime, HistoricalTimestamp_ExcludesLaterInserts) {
    SystemVersionedTable table{"hist_bbox", "n"};
    // Berlin inside Germany bbox
    insertEntity(table, "berlin", 13.4050, 52.5200);
    const Timestamp t1 = now();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    // Hamburg (also inside Germany bbox) inserted after t1
    insertEntity(table, "hamburg", 10.0, 53.55);

    MBR germany{5.8, 47.2, 15.1, 55.1};

    // At t1 only Berlin was alive
    auto at_t1 = TemporalSpatialQuery::entitiesInBBoxAtTime(table, germany, t1);
    ASSERT_EQ(at_t1.size(), 1u);
    EXPECT_EQ(at_t1[0].key, "berlin");

    // At now() both are inside Germany bbox
    auto at_now = TemporalSpatialQuery::entitiesInBBoxAtTime(table, germany, now());
    EXPECT_EQ(at_now.size(), 2u);
}

TEST(EntitiesWithinDistanceAtTime, DeletedEntity_NotReturnedAfterDelete) {
    SystemVersionedTable table{"del_dist", "n"};
    insertEntity(table, "bus", 13.4050, 52.5200); // Berlin
    const Timestamp t_before = now();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    table.deleteRow("bus");
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    const Timestamp t_after = now();

    // Before delete: entity should be found within 1 km of Berlin
    auto before = TemporalSpatialQuery::entitiesWithinDistanceAtTime(
        table, 13.4050, 52.5200, 1000.0, t_before);
    EXPECT_EQ(before.size(), 1u);

    // After delete: entity must not appear
    auto after = TemporalSpatialQuery::entitiesWithinDistanceAtTime(
        table, 13.4050, 52.5200, 1000.0, t_after);
    EXPECT_TRUE(after.empty());
}

TEST(EntitiesWithinDistanceAtTimeSorted, ZeroDistance_ReturnsEmpty) {
    SystemVersionedTable table{"t", "n"};
    insertEntity(table, "e1", 13.4, 52.5);

    auto result = TemporalSpatialQuery::entitiesWithinDistanceAtTimeSorted(
        table, 13.4, 52.5, 0.0, now());
    EXPECT_TRUE(result.empty());
}
