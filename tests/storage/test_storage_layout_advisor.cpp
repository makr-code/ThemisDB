// Copyright 2026 ThemisDB — Licensed under MIT License
// IMPL-B10 / S-8: StorageLayoutAdvisor unit tests
//
// Tests:
//   SLA-01  isTimeSeries() true for monotonic timestamps + Float payload
//   SLA-02  analyze() time-series + aggregation_ratio > 0.7 → COLUMNAR_COMPRESSED
//   SLA-03  analyze() UUID point-lookup ratio > 0.8 → ROW_ORIENTED
//   SLA-04  analyze() blob field + metadata_only > 0.5 → HYBRID
//   SLA-05  estimated_compression_ratio >= 5.0 for float-heavy time-series
//   SLA-06  GDPR fields → gdpr_approval_required = true
//   SLA-07  No GDPR fields → gdpr_approval_required = false
//   SLA-08  analyze() has non-empty rationale
//   SLA-09  analyze() returns confidence in [0, 1]
//   SLA-10  default fallback → ROW_ORIENTED

#include <gtest/gtest.h>
#include "storage/storage_layout_advisor.h"

#include <string>
#include <vector>

using namespace themis::storage;
using LT = StorageLayoutAdvisor::LayoutType;

namespace {

// Build a monotonic timestamp series (seconds since epoch, 1-second interval)
static std::vector<double> makeMonotonicSeries(size_t n = 64)
{
    std::vector<double> ts;
    ts.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        ts.push_back(1'700'000'000.0 + static_cast<double>(i));
    }
    return ts;
}

// Typical time-series collection (sensor readings)
static CollectionAccessStats timeSeriesStats()
{
    CollectionAccessStats s;
    s.point_lookup_ratio          = 0.05;
    s.range_scan_ratio            = 0.75;
    s.aggregation_ratio           = 0.85;
    s.metadata_only_access_ratio  = 0.10;
    s.has_blob_field              = false;
    s.has_monotonic_timestamp     = true;
    s.timestamp_series            = makeMonotonicSeries();
    return s;
}

// Typical UUID point-lookup collection (user sessions)
static CollectionAccessStats uuidStats()
{
    CollectionAccessStats s;
    s.point_lookup_ratio          = 0.90;
    s.range_scan_ratio            = 0.05;
    s.aggregation_ratio           = 0.02;
    s.metadata_only_access_ratio  = 0.03;
    s.has_blob_field              = false;
    s.has_monotonic_timestamp     = false;
    return s;
}

// Mixed BLOB collection
static CollectionAccessStats blobStats()
{
    CollectionAccessStats s;
    s.point_lookup_ratio          = 0.10;
    s.range_scan_ratio            = 0.30;
    s.aggregation_ratio           = 0.05;
    s.metadata_only_access_ratio  = 0.60;
    s.has_blob_field              = true;
    s.has_monotonic_timestamp     = false;
    return s;
}

// Schema with 5 Float fields (ideal for columnar compression)
static SchemaInfo floatSchema(const std::string& name = "sensor_readings")
{
    SchemaInfo sc;
    sc.collection_name = name;
    for (int i = 0; i < 5; ++i) {
        const std::string fn = "value_" + std::to_string(i);
        sc.field_names.push_back(fn);
        sc.field_types[fn] = "Float";
    }
    sc.field_names.push_back("timestamp");
    sc.field_types["timestamp"] = "DateTime";
    sc.has_blob = false;
    return sc;
}

// Schema with UUID primary key
static SchemaInfo uuidSchema(const std::string& name = "user_sessions")
{
    SchemaInfo sc;
    sc.collection_name = name;
    sc.field_names = {"session_id", "user_id", "data", "created_at"};
    sc.field_types["session_id"] = "UUID";
    sc.field_types["user_id"]    = "Int";
    sc.field_types["data"]       = "String";
    sc.field_types["created_at"] = "DateTime";
    sc.has_blob = false;
    return sc;
}

// Empty GDPR registry (no protections)
static GdprFieldRegistry noGdpr() { return {}; }

// GDPR registry that protects one field
static GdprFieldRegistry withGdpr(const std::string& field)
{
    GdprFieldRegistry r;
    r.protected_paths.insert(field);
    return r;
}

} // namespace

// ---------------------------------------------------------------------------
// SLA-01  isTimeSeries() = true for monotonic timestamps + large series
// ---------------------------------------------------------------------------
TEST(StorageLayoutAdvisorTest, IsTimeSeriesTrue)
{
    StorageLayoutAdvisor adv;
    EXPECT_TRUE(adv.isTimeSeries(timeSeriesStats()));
}

// ---------------------------------------------------------------------------
// SLA-02  time-series + aggregation > 0.7 → COLUMNAR_COMPRESSED
// ---------------------------------------------------------------------------
TEST(StorageLayoutAdvisorTest, TimeSeriesColumnlarCompressed)
{
    StorageLayoutAdvisor adv;
    auto rec = adv.analyze("sensor_readings", timeSeriesStats(),
                           floatSchema(), noGdpr());
    EXPECT_EQ(rec.recommended_layout, LT::COLUMNAR_COMPRESSED);
}

// ---------------------------------------------------------------------------
// SLA-03  UUID point-lookup ratio > 0.8 → ROW_ORIENTED
// ---------------------------------------------------------------------------
TEST(StorageLayoutAdvisorTest, UuidPointLookupRowOriented)
{
    StorageLayoutAdvisor adv;
    auto rec = adv.analyze("user_sessions", uuidStats(),
                           uuidSchema(), noGdpr());
    EXPECT_EQ(rec.recommended_layout, LT::ROW_ORIENTED);
}

// ---------------------------------------------------------------------------
// SLA-04  blob + metadata_only > 0.5 → HYBRID
// ---------------------------------------------------------------------------
TEST(StorageLayoutAdvisorTest, BlobCollectionHybrid)
{
    StorageLayoutAdvisor adv;

    SchemaInfo sc;
    sc.collection_name = "documents";
    sc.field_names = {"doc_id", "title", "payload"};
    sc.field_types["doc_id"]  = "UUID";
    sc.field_types["title"]   = "String";
    sc.field_types["payload"] = "BLOB";
    sc.has_blob = true;

    auto rec = adv.analyze("documents", blobStats(), sc, noGdpr());
    EXPECT_EQ(rec.recommended_layout, LT::HYBRID);
}

// ---------------------------------------------------------------------------
// SLA-05  Float time-series → estimated_compression_ratio >= 5.0
// ---------------------------------------------------------------------------
TEST(StorageLayoutAdvisorTest, FloatTimeSeriesCompressionRatio)
{
    StorageLayoutAdvisor adv;
    auto rec = adv.analyze("sensor_readings", timeSeriesStats(),
                           floatSchema(), noGdpr());
    EXPECT_GE(rec.estimated_compression_ratio, 5.0);
}

// ---------------------------------------------------------------------------
// SLA-06  GDPR-protected field in collection → gdpr_approval_required = true
// ---------------------------------------------------------------------------
TEST(StorageLayoutAdvisorTest, GdprFieldRequiresApproval)
{
    StorageLayoutAdvisor adv;
    // Protect "value_0" — one of the Float fields in floatSchema()
    auto gdpr = withGdpr("value_0");
    auto rec  = adv.analyze("sensor_readings", timeSeriesStats(),
                            floatSchema(), gdpr);
    EXPECT_TRUE(rec.gdpr_approval_required);
}

// ---------------------------------------------------------------------------
// SLA-07  No GDPR fields → gdpr_approval_required = false
// ---------------------------------------------------------------------------
TEST(StorageLayoutAdvisorTest, NoGdprFieldNoApproval)
{
    StorageLayoutAdvisor adv;
    auto rec = adv.analyze("user_sessions", uuidStats(),
                           uuidSchema(), noGdpr());
    EXPECT_FALSE(rec.gdpr_approval_required);
}

// ---------------------------------------------------------------------------
// SLA-08  rationale is non-empty (DBA-readable)
// ---------------------------------------------------------------------------
TEST(StorageLayoutAdvisorTest, NonEmptyRationale)
{
    StorageLayoutAdvisor adv;
    auto rec = adv.analyze("sensor_readings", timeSeriesStats(),
                           floatSchema(), noGdpr());
    EXPECT_FALSE(rec.rationale.empty());
}

// ---------------------------------------------------------------------------
// SLA-09  confidence ∈ [0.0, 1.0]
// ---------------------------------------------------------------------------
TEST(StorageLayoutAdvisorTest, ConfidenceInRange)
{
    StorageLayoutAdvisor adv;

    auto r1 = adv.analyze("ts_coll",   timeSeriesStats(), floatSchema(), noGdpr());
    auto r2 = adv.analyze("uuid_coll", uuidStats(),       uuidSchema(),  noGdpr());

    EXPECT_GE(r1.confidence, 0.0);
    EXPECT_LE(r1.confidence, 1.0);
    EXPECT_GE(r2.confidence, 0.0);
    EXPECT_LE(r2.confidence, 1.0);
}

// ---------------------------------------------------------------------------
// SLA-10  Completely flat stats → safe default ROW_ORIENTED
// ---------------------------------------------------------------------------
TEST(StorageLayoutAdvisorTest, DefaultFallbackRowOriented)
{
    StorageLayoutAdvisor adv;

    CollectionAccessStats flat;
    // All ratios at 0 — no pattern matches
    flat.has_monotonic_timestamp = false;

    SchemaInfo sc;
    sc.collection_name = "misc";
    sc.field_names = {"id", "payload"};
    sc.field_types["id"]      = "Int";
    sc.field_types["payload"] = "String";

    auto rec = adv.analyze("misc", flat, sc, noGdpr());
    EXPECT_EQ(rec.recommended_layout, LT::ROW_ORIENTED);
}
