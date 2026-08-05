// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_metadata_hardening_phase23_focused.cpp
 * @brief Phase 2/3 hardening focused tests for the metadata module.
 * @note Test IDs: MCH-01..MCH-08
 *
 * Validates hardening items delivered in Phase 2 (Core Implementation)
 * and Phase 3 (Error Handling and Edge Cases):
 *
 *   MCH-01  ConsistencyIssue toJSON produces all required diagnostic fields
 *   MCH-02  ConsistencyIssue with empty column_name serializes without crash
 *   MCH-03  ConsistencyIssue issue_type "orphan_key" is preserved round-trip
 *   MCH-04  ConsistencyIssue issue_type "stale_stats" is preserved round-trip
 *   MCH-05  ConsistencyIssue issue_type "missing_constraint" is preserved round-trip
 *   MCH-06  ConsistencyIssue issue_type "schema_mismatch" is preserved round-trip
 *   MCH-07  ColumnRef toString returns canonical "table.column" format
 *   MCH-08  ColumnRef toJSON / fromJSON round-trip preserves both fields
 *
 * All tests are self-contained: no network I/O, no filesystem I/O, no RocksDB.
 * Canonical PRNG seed: kPhase23Seed = 42 (unused here; declared for governance).
 *
 * @see include/metadata/schema_consistency_checker.h
 * @see include/metadata/column_lineage.h
 * @see src/metadata/ROADMAP.md — Phase 2 / Phase 3 items
 */

#include <gtest/gtest.h>

#include "metadata/schema_consistency_checker.h"
#include "metadata/column_lineage.h"

#include <string>

using namespace themis;
using namespace themis::metadata;

namespace {

// Canonical seed — retained for governance parity.
[[maybe_unused]] static constexpr uint64_t kPhase23Seed = 42;

// ---------------------------------------------------------------------------
// Helper: build a populated ConsistencyIssue
// ---------------------------------------------------------------------------
ConsistencyIssue makeIssue(std::string_view type,
                            std::string_view table,
                            std::string_view column,
                            std::string_view detail) {
    ConsistencyIssue ci;
    ci.issue_type   = std::string(type);
    ci.table_name   = std::string(table);
    ci.column_name  = std::string(column);
    ci.detail       = std::string(detail);
    return ci;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// MCH-01: ConsistencyIssue toJSON produces all required diagnostic fields
// ---------------------------------------------------------------------------
TEST(MetadataHardeningPhase23Test, MCH01_ConsistencyIssueJsonHasAllFields) {
    const auto ci = makeIssue("orphan_key", "orders", "id", "Key prefix unknown");
    const auto j  = ci.toJSON();

    EXPECT_TRUE(j.contains("issue_type"))  << "JSON missing 'issue_type'";
    EXPECT_TRUE(j.contains("table_name"))  << "JSON missing 'table_name'";
    EXPECT_TRUE(j.contains("column_name")) << "JSON missing 'column_name'";
    EXPECT_TRUE(j.contains("detail"))      << "JSON missing 'detail'";
}

// ---------------------------------------------------------------------------
// MCH-02: ConsistencyIssue with empty column_name serializes without crash
// ---------------------------------------------------------------------------
TEST(MetadataHardeningPhase23Test, MCH02_ConsistencyIssueEmptyColumnSerializes) {
    const auto ci = makeIssue("stale_stats", "users", /*column=*/"", "No stats refresh");
    ASSERT_NO_THROW({
        const auto j = ci.toJSON();
        EXPECT_EQ(j["column_name"].get<std::string>(), "");
    });
}

// ---------------------------------------------------------------------------
// MCH-03: ConsistencyIssue issue_type "orphan_key" is preserved round-trip
// ---------------------------------------------------------------------------
TEST(MetadataHardeningPhase23Test, MCH03_IssueTypeOrphanKeyPreserved) {
    const auto ci = makeIssue("orphan_key", "products", "sku", "Prefix mismatch");
    EXPECT_EQ(ci.toJSON()["issue_type"].get<std::string>(), "orphan_key");
}

// ---------------------------------------------------------------------------
// MCH-04: ConsistencyIssue issue_type "stale_stats" is preserved round-trip
// ---------------------------------------------------------------------------
TEST(MetadataHardeningPhase23Test, MCH04_IssueTypeStaleStatsPreserved) {
    const auto ci = makeIssue("stale_stats", "events", "ts", "Stats > 24 h old");
    EXPECT_EQ(ci.toJSON()["issue_type"].get<std::string>(), "stale_stats");
}

// ---------------------------------------------------------------------------
// MCH-05: ConsistencyIssue issue_type "missing_constraint" is preserved
// ---------------------------------------------------------------------------
TEST(MetadataHardeningPhase23Test, MCH05_IssueTypeMissingConstraintPreserved) {
    const auto ci = makeIssue("missing_constraint", "sessions", "", "No constraints");
    EXPECT_EQ(ci.toJSON()["issue_type"].get<std::string>(), "missing_constraint");
}

// ---------------------------------------------------------------------------
// MCH-06: ConsistencyIssue issue_type "schema_mismatch" is preserved
// ---------------------------------------------------------------------------
TEST(MetadataHardeningPhase23Test, MCH06_IssueTypeSchemaMismatchPreserved) {
    const auto ci = makeIssue("schema_mismatch", "catalog", "version",
                               "Schema version incompatible");
    EXPECT_EQ(ci.toJSON()["issue_type"].get<std::string>(), "schema_mismatch");
}

// ---------------------------------------------------------------------------
// MCH-07: ColumnRef toString returns canonical "table.column" format
// ---------------------------------------------------------------------------
TEST(MetadataHardeningPhase23Test, MCH07_ColumnRefToStringFormat) {
    ColumnRef ref;
    ref.table_name  = "orders";
    ref.column_name = "customer_id";

    EXPECT_EQ(ref.toString(), "orders.customer_id");
}

// ---------------------------------------------------------------------------
// MCH-08: ColumnRef toJSON / fromJSON round-trip preserves both fields
// ---------------------------------------------------------------------------
TEST(MetadataHardeningPhase23Test, MCH08_ColumnRefJsonRoundTrip) {
    ColumnRef original;
    original.table_name  = "shipments";
    original.column_name = "tracking_id";

    const auto j       = original.toJSON();
    const auto restored = ColumnRef::fromJSON(j);

    EXPECT_EQ(restored.table_name,  original.table_name);
    EXPECT_EQ(restored.column_name, original.column_name);
    EXPECT_EQ(restored, original);
}
