/**
 * @file test_column_lineage.cpp
 * @brief Unit tests for ColumnLineageTracker
 *
 * Tests cover:
 * - recordDerivation with auto-assigned entry_id and timestamp
 * - getColumnLineage returns all entries for a target column
 * - getUpstreamColumns performs transitive BFS upward through the DAG
 * - getDownstreamColumns performs transitive BFS downward through the DAG
 * - getColumnProvenance returns column, entries, upstream, and downstream
 * - exportTableLineage returns only entries for the requested table
 * - exportAllLineage returns all recorded entries
 * - totalEntryCount tracks all recorded entries
 * - transformationTypeToString covers all enum values
 * - transformationTypeFromString round-trips all known values
 * - ColumnRef equality, toString, toJSON, and fromJSON
 * - Diamond-dependency DAG (shared ancestor) is handled correctly
 */

#include <gtest/gtest.h>
#include "metadata/column_lineage.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using namespace themis::metadata;
using json = nlohmann::json;

// ─── TransformationType helpers ──────────────────────────────────────────────

TEST(TransformationTypeTest, AllValuesToStringNonEmpty) {
    const std::vector<TransformationType> all = {
        TransformationType::DIRECT_COPY,
        TransformationType::RENAME,
        TransformationType::CAST,
        TransformationType::COMPUTED,
        TransformationType::AGGREGATION,
        TransformationType::ANONYMIZATION,
        TransformationType::ENRICHMENT,
        TransformationType::CUSTOM
    };
    for (auto t : all) {
        EXPECT_FALSE(transformationTypeToString(t).empty());
    }
}

TEST(TransformationTypeTest, KnownStringValues) {
    EXPECT_EQ(transformationTypeToString(TransformationType::DIRECT_COPY),   "DIRECT_COPY");
    EXPECT_EQ(transformationTypeToString(TransformationType::RENAME),        "RENAME");
    EXPECT_EQ(transformationTypeToString(TransformationType::CAST),          "CAST");
    EXPECT_EQ(transformationTypeToString(TransformationType::COMPUTED),      "COMPUTED");
    EXPECT_EQ(transformationTypeToString(TransformationType::AGGREGATION),   "AGGREGATION");
    EXPECT_EQ(transformationTypeToString(TransformationType::ANONYMIZATION), "ANONYMIZATION");
    EXPECT_EQ(transformationTypeToString(TransformationType::ENRICHMENT),    "ENRICHMENT");
    EXPECT_EQ(transformationTypeToString(TransformationType::CUSTOM),        "CUSTOM");
}

TEST(TransformationTypeTest, RoundTripFromString) {
    const std::vector<TransformationType> all = {
        TransformationType::DIRECT_COPY,
        TransformationType::RENAME,
        TransformationType::CAST,
        TransformationType::COMPUTED,
        TransformationType::AGGREGATION,
        TransformationType::ANONYMIZATION,
        TransformationType::ENRICHMENT,
        TransformationType::CUSTOM
    };
    for (auto t : all) {
        EXPECT_EQ(transformationTypeFromString(transformationTypeToString(t)), t);
    }
}

TEST(TransformationTypeTest, FromStringCaseInsensitive) {
    EXPECT_EQ(transformationTypeFromString("computed"),    TransformationType::COMPUTED);
    EXPECT_EQ(transformationTypeFromString("Computed"),    TransformationType::COMPUTED);
    EXPECT_EQ(transformationTypeFromString("AGGREGATION"), TransformationType::AGGREGATION);
}

TEST(TransformationTypeTest, UnknownStringMapsToCustom) {
    EXPECT_EQ(transformationTypeFromString("unknown_xyz"), TransformationType::CUSTOM);
}

// ─── ColumnRef ────────────────────────────────────────────────────────────────

TEST(ColumnRefTest, ToString) {
    ColumnRef ref{"users", "email"};
    EXPECT_EQ(ref.toString(), "users.email");
}

TEST(ColumnRefTest, EqualityOperator) {
    ColumnRef a{"t", "c"};
    ColumnRef b{"t", "c"};
    ColumnRef c{"t", "d"};
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

TEST(ColumnRefTest, ToJSONContainsTableAndColumn) {
    ColumnRef ref{"orders", "total_amount"};
    json j = ref.toJSON();
    EXPECT_EQ(j["table"],  "orders");
    EXPECT_EQ(j["column"], "total_amount");
}

TEST(ColumnRefTest, FromJSONRoundTrip) {
    ColumnRef original{"events", "event_type"};
    ColumnRef parsed = ColumnRef::fromJSON(original.toJSON());
    EXPECT_EQ(parsed, original);
}

// ─── ColumnLineageEntry::toJSON ───────────────────────────────────────────────

TEST(ColumnLineageEntryTest, ToJSONRequiredFields) {
    ColumnLineageEntry entry;
    entry.entry_id    = "e-001";
    entry.target_column = {"derived_table", "full_name"};
    entry.source_columns = {{"users", "first_name"}, {"users", "last_name"}};
    entry.transformation = TransformationType::COMPUTED;
    entry.transformation_expression = "first_name || ' ' || last_name";
    entry.timestamp_ms = 1700000000000LL;

    json j = entry.toJSON();
    EXPECT_EQ(j["entry_id"],   "e-001");
    EXPECT_EQ(j["transformation"], "COMPUTED");
    EXPECT_EQ(j["timestamp_ms"],   1700000000000LL);
    ASSERT_EQ(j["source_columns"].size(), 2u);
    EXPECT_EQ(j["transformation_expression"], "first_name || ' ' || last_name");
}

TEST(ColumnLineageEntryTest, ToJSONOmitsEmptyOptionals) {
    ColumnLineageEntry entry;
    entry.entry_id   = "e-002";
    entry.target_column = {"t", "c"};
    entry.transformation = TransformationType::DIRECT_COPY;
    entry.timestamp_ms = 1000LL;

    json j = entry.toJSON();
    EXPECT_FALSE(j.contains("transformation_expression"));
    EXPECT_FALSE(j.contains("performed_by"));
    EXPECT_FALSE(j.contains("metadata"));
}

// ─── ColumnLineageTracker fixture ─────────────────────────────────────────────

class ColumnLineageTrackerTest : public ::testing::Test {
protected:
    ColumnLineageTracker tracker;

    /// Helper: build a simple entry with one source
    static ColumnLineageEntry makeEntry(
            const std::string& src_table, const std::string& src_col,
            const std::string& tgt_table, const std::string& tgt_col,
            TransformationType type = TransformationType::DIRECT_COPY,
            int64_t ts = 1000LL) {
        ColumnLineageEntry e;
        e.source_columns = {{src_table, src_col}};
        e.target_column  = {tgt_table, tgt_col};
        e.transformation = type;
        e.timestamp_ms   = ts;
        return e;
    }
};

// ─── recordDerivation ─────────────────────────────────────────────────────────

TEST_F(ColumnLineageTrackerTest, RecordAssignsEntryIdWhenEmpty) {
    ColumnLineageEntry e;
    e.target_column  = {"t", "c"};
    e.transformation = TransformationType::DIRECT_COPY;
    e.timestamp_ms   = 1000LL;

    tracker.recordDerivation(e);

    auto record = tracker.getColumnLineage({"t", "c"});
    ASSERT_EQ(record.entries.size(), 1u);
    EXPECT_FALSE(record.entries[0].entry_id.empty());
}

TEST_F(ColumnLineageTrackerTest, RecordAssignsTimestampWhenZero) {
    ColumnLineageEntry e;
    e.target_column  = {"t", "ts_col"};
    e.transformation = TransformationType::RENAME;
    e.timestamp_ms   = 0;

    tracker.recordDerivation(e);

    auto record = tracker.getColumnLineage({"t", "ts_col"});
    ASSERT_EQ(record.entries.size(), 1u);
    EXPECT_GT(record.entries[0].timestamp_ms, 0LL);
}

TEST_F(ColumnLineageTrackerTest, RecordPreservesProvidedFields) {
    ColumnLineageEntry e;
    e.entry_id       = "explicit-id";
    e.target_column  = {"t", "col"};
    e.source_columns = {{"src_t", "src_c"}};
    e.transformation = TransformationType::CAST;
    e.transformation_expression = "CAST(src_c AS INTEGER)";
    e.performed_by   = "etl-service";
    e.timestamp_ms   = 9999LL;

    tracker.recordDerivation(e);

    auto record = tracker.getColumnLineage({"t", "col"});
    ASSERT_EQ(record.entries.size(), 1u);
    EXPECT_EQ(record.entries[0].entry_id,    "explicit-id");
    EXPECT_EQ(record.entries[0].performed_by, "etl-service");
    EXPECT_EQ(record.entries[0].timestamp_ms, 9999LL);
    EXPECT_EQ(record.entries[0].transformation_expression, "CAST(src_c AS INTEGER)");
}

TEST_F(ColumnLineageTrackerTest, GetColumnLineageForUnknownColumnReturnsEmptyRecord) {
    auto record = tracker.getColumnLineage({"no_table", "no_col"});
    EXPECT_TRUE(record.entries.empty());
    EXPECT_EQ(record.column.table_name,  "no_table");
    EXPECT_EQ(record.column.column_name, "no_col");
}

// ─── getUpstreamColumns ───────────────────────────────────────────────────────

TEST_F(ColumnLineageTrackerTest, UpstreamColumns_DirectParent) {
    // src → tgt
    tracker.recordDerivation(makeEntry("src", "a", "tgt", "b"));

    auto up = tracker.getUpstreamColumns({"tgt", "b"});
    ASSERT_EQ(up.size(), 1u);
    EXPECT_EQ(up[0].table_name,  "src");
    EXPECT_EQ(up[0].column_name, "a");
}

TEST_F(ColumnLineageTrackerTest, UpstreamColumns_ThreeGenerations) {
    // root → mid → leaf
    tracker.recordDerivation(makeEntry("t", "root", "t", "mid", TransformationType::RENAME, 1000));
    tracker.recordDerivation(makeEntry("t", "mid",  "t", "leaf", TransformationType::CAST,  2000));

    auto up = tracker.getUpstreamColumns({"t", "leaf"});
    // Should contain both "mid" and "root"
    ASSERT_EQ(up.size(), 2u);
    EXPECT_EQ(up[0].column_name, "mid");   // nearest first
    EXPECT_EQ(up[1].column_name, "root");
}

TEST_F(ColumnLineageTrackerTest, UpstreamColumns_MultipleSourcesMerged) {
    // first_name + last_name → full_name
    ColumnLineageEntry e;
    e.target_column  = {"users", "full_name"};
    e.source_columns = {{"users", "first_name"}, {"users", "last_name"}};
    e.transformation = TransformationType::COMPUTED;
    e.timestamp_ms   = 1000LL;
    tracker.recordDerivation(e);

    auto up = tracker.getUpstreamColumns({"users", "full_name"});
    ASSERT_EQ(up.size(), 2u);
}

TEST_F(ColumnLineageTrackerTest, UpstreamColumns_RootHasNoUpstream) {
    tracker.recordDerivation(makeEntry("s", "src_c", "t", "tgt_c"));
    // The source column itself has no derivation entry
    auto up = tracker.getUpstreamColumns({"s", "src_c"});
    EXPECT_TRUE(up.empty());
}

TEST_F(ColumnLineageTrackerTest, UpstreamColumns_UnknownColumnReturnsEmpty) {
    auto up = tracker.getUpstreamColumns({"x", "y"});
    EXPECT_TRUE(up.empty());
}

// ─── getDownstreamColumns ─────────────────────────────────────────────────────

TEST_F(ColumnLineageTrackerTest, DownstreamColumns_DirectChild) {
    tracker.recordDerivation(makeEntry("src", "a", "dst", "b"));

    auto down = tracker.getDownstreamColumns({"src", "a"});
    ASSERT_EQ(down.size(), 1u);
    EXPECT_EQ(down[0].table_name,  "dst");
    EXPECT_EQ(down[0].column_name, "b");
}

TEST_F(ColumnLineageTrackerTest, DownstreamColumns_ThreeGenerations) {
    tracker.recordDerivation(makeEntry("t", "a", "t", "b", TransformationType::RENAME, 1000));
    tracker.recordDerivation(makeEntry("t", "b", "t", "c", TransformationType::RENAME, 2000));

    auto down = tracker.getDownstreamColumns({"t", "a"});
    ASSERT_EQ(down.size(), 2u);
    EXPECT_EQ(down[0].column_name, "b");
    EXPECT_EQ(down[1].column_name, "c");
}

TEST_F(ColumnLineageTrackerTest, DownstreamColumns_LeafHasNoDescendants) {
    tracker.recordDerivation(makeEntry("s", "a", "d", "b"));
    auto down = tracker.getDownstreamColumns({"d", "b"});
    EXPECT_TRUE(down.empty());
}

TEST_F(ColumnLineageTrackerTest, DownstreamColumns_UnknownColumnReturnsEmpty) {
    auto down = tracker.getDownstreamColumns({"x", "y"});
    EXPECT_TRUE(down.empty());
}

// ─── Diamond DAG ──────────────────────────────────────────────────────────────

TEST_F(ColumnLineageTrackerTest, DiamondDAGUpstreamDeduplicates) {
    // a → b, a → c, b+c → d  (diamond)
    tracker.recordDerivation(makeEntry("t", "a", "t", "b", TransformationType::RENAME, 1000));
    tracker.recordDerivation(makeEntry("t", "a", "t", "c", TransformationType::RENAME, 1001));

    ColumnLineageEntry e;
    e.target_column  = {"t", "d"};
    e.source_columns = {{"t", "b"}, {"t", "c"}};
    e.transformation = TransformationType::COMPUTED;
    e.timestamp_ms   = 2000LL;
    tracker.recordDerivation(e);

    auto up = tracker.getUpstreamColumns({"t", "d"});
    // Should contain b, c, a — but a appears only once
    EXPECT_EQ(up.size(), 3u);
    // Verify 'a' is present exactly once
    size_t count_a = 0;
    for (const auto& ref : up) {
        if (ref.column_name == "a") {
          ++count_a;
        }
    }
    EXPECT_EQ(count_a, 1u);
}

TEST_F(ColumnLineageTrackerTest, DiamondDAGDownstreamDeduplicates) {
    // a → b, a → c, b → d, c → d
    tracker.recordDerivation(makeEntry("t", "a", "t", "b", TransformationType::RENAME, 1000));
    tracker.recordDerivation(makeEntry("t", "a", "t", "c", TransformationType::RENAME, 1001));
    tracker.recordDerivation(makeEntry("t", "b", "t", "d", TransformationType::RENAME, 2000));
    tracker.recordDerivation(makeEntry("t", "c", "t", "d", TransformationType::RENAME, 2001));

    auto down = tracker.getDownstreamColumns({"t", "a"});
    // b, c, d — each exactly once
    EXPECT_EQ(down.size(), 3u);
    size_t count_d = 0;
    for (const auto& ref : down) {
        if (ref.column_name == "d") {
          ++count_d;
        }
    }
    EXPECT_EQ(count_d, 1u);
}

// ─── getColumnProvenance ──────────────────────────────────────────────────────

TEST_F(ColumnLineageTrackerTest, ProvenanceContainsAllFields) {
    tracker.recordDerivation(makeEntry("src", "x", "dst", "y", TransformationType::CAST, 1000));

    json prov = tracker.getColumnProvenance({"dst", "y"});
    EXPECT_TRUE(prov.contains("column"));
    EXPECT_TRUE(prov.contains("entries"));
    EXPECT_TRUE(prov.contains("upstream_columns"));
    EXPECT_TRUE(prov.contains("downstream_columns"));
    EXPECT_EQ(prov["column"]["table"],  "dst");
    EXPECT_EQ(prov["column"]["column"], "y");
    ASSERT_EQ(prov["entries"].size(),         1u);
    ASSERT_EQ(prov["upstream_columns"].size(), 1u);
    EXPECT_EQ(prov["downstream_columns"].size(), 0u);
}

TEST_F(ColumnLineageTrackerTest, ProvenanceForUnknownColumnIsEmpty) {
    json prov = tracker.getColumnProvenance({"no", "col"});
    EXPECT_EQ(prov["entries"].size(),            0u);
    EXPECT_EQ(prov["upstream_columns"].size(),   0u);
    EXPECT_EQ(prov["downstream_columns"].size(), 0u);
}

// ─── exportTableLineage ───────────────────────────────────────────────────────

TEST_F(ColumnLineageTrackerTest, ExportTableLineageFiltersCorrectly) {
    tracker.recordDerivation(makeEntry("t1", "a", "t1", "b"));
    tracker.recordDerivation(makeEntry("t2", "c", "t2", "d"));

    json t1_lineage = tracker.exportTableLineage("t1");
    ASSERT_EQ(t1_lineage.size(), 1u);
    EXPECT_EQ(t1_lineage[0]["column"]["table"], "t1");

    json t2_lineage = tracker.exportTableLineage("t2");
    ASSERT_EQ(t2_lineage.size(), 1u);
    EXPECT_EQ(t2_lineage[0]["column"]["table"], "t2");
}

TEST_F(ColumnLineageTrackerTest, ExportTableLineageForUnknownTableIsEmpty) {
    json result = tracker.exportTableLineage("no_such_table");
    EXPECT_TRUE(result.is_array());
    EXPECT_TRUE(result.empty());
}

// ─── exportAllLineage ─────────────────────────────────────────────────────────

TEST_F(ColumnLineageTrackerTest, ExportAllLineageContainsAllEntries) {
    tracker.recordDerivation(makeEntry("t", "a", "t", "b", TransformationType::RENAME, 1000));
    tracker.recordDerivation(makeEntry("t", "b", "t", "c", TransformationType::RENAME, 2000));

    json all = tracker.exportAllLineage();
    EXPECT_EQ(all["total_entries"], 2u);
    ASSERT_TRUE(all.contains("entries"));
    EXPECT_EQ(all["entries"].size(), 2u);
}

// ─── totalEntryCount ──────────────────────────────────────────────────────────

TEST_F(ColumnLineageTrackerTest, TotalEntryCountMatchesRecorded) {
    EXPECT_EQ(tracker.totalEntryCount(), 0u);

    for (int i = 0; i < 4; ++i) {
        ColumnLineageEntry e;
        e.target_column  = {"t", "col_" + std::to_string(i)};
        e.transformation = TransformationType::DIRECT_COPY;
        e.timestamp_ms   = 1000LL * (i + 1);
        tracker.recordDerivation(e);
    }
    EXPECT_EQ(tracker.totalEntryCount(), 4u);
}

// ─── ColumnLineageRecord::toJSON ──────────────────────────────────────────────

TEST(ColumnLineageRecordTest, ToJSONEmptyRecord) {
    ColumnLineageRecord rec;
    rec.column = {"tbl", "col"};
    json j = rec.toJSON();
    EXPECT_EQ(j["entry_count"], 0u);
    EXPECT_TRUE(j["entries"].is_array());
    EXPECT_TRUE(j["entries"].empty());
}

TEST(ColumnLineageRecordTest, ToJSONWithEntries) {
    ColumnLineageRecord rec;
    rec.column = {"tbl", "col"};
    ColumnLineageEntry e;
    e.entry_id   = "e1";
    e.target_column = {"tbl", "col"};
    e.transformation = TransformationType::COMPUTED;
    e.timestamp_ms   = 1000LL;
    rec.entries.push_back(e);
    json j = rec.toJSON();
    EXPECT_EQ(j["entry_count"], 1u);
    ASSERT_EQ(j["entries"].size(), 1u);
    EXPECT_EQ(j["entries"][0]["entry_id"], "e1");
}
