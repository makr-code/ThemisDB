// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_column_lineage_traversal_edge_focused.cpp
 * @brief Phase B2: Traversal edge-case tests for ColumnLineageTracker.
 * @note Test IDs: MCH-LN01..MCH-LN04
 *
 *   MCH-LN01  ColumnRef equality and inequality operators work correctly
 *   MCH-LN02  ColumnRef::toString() returns "table.column" canonical format
 *   MCH-LN03  ColumnRef::toJSON() / fromJSON() round-trip preserves all fields
 *   MCH-LN04  TransformationType string conversion round-trips for all types
 *
 * Self-contained: no RocksDB, no network I/O, no background threads.
 * Canonical PRNG seed: kLineageSeed = 42.
 *
 * @see include/metadata/column_lineage.h
 * @see src/metadata/ROADMAP.md — Phase B lineage traversal items
 */

#include <gtest/gtest.h>

#include "metadata/column_lineage.h"

#include <string>
#include <vector>

using namespace themis::metadata;

namespace {

[[maybe_unused]] static constexpr uint64_t kLineageSeed = 42;

} // anonymous namespace

// ---------------------------------------------------------------------------
// MCH-LN01: ColumnRef equality and inequality operators work correctly
// ---------------------------------------------------------------------------
TEST(ColumnLineageTraversalEdgeTest, MCHLLN01_ColumnRefEquality) {
    ColumnRef a{"orders", "customer_id"};
    ColumnRef b{"orders", "customer_id"};
    ColumnRef c{"orders", "order_date"};
    ColumnRef d{"users",  "customer_id"};

    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
    EXPECT_NE(a, d);
}

// ---------------------------------------------------------------------------
// MCH-LN02: ColumnRef::toString() returns "table.column" canonical format
// ---------------------------------------------------------------------------
TEST(ColumnLineageTraversalEdgeTest, MCHLLN02_ColumnRefToString) {
    ColumnRef ref{"shipments", "tracking_id"};
    EXPECT_EQ(ref.toString(), "shipments.tracking_id");

    ColumnRef empty{"", ""};
    EXPECT_EQ(empty.toString(), ".");
}

// ---------------------------------------------------------------------------
// MCH-LN03: ColumnRef::toJSON() / fromJSON() round-trip preserves all fields
// ---------------------------------------------------------------------------
TEST(ColumnLineageTraversalEdgeTest, MCHLLN03_ColumnRefJsonRoundTrip) {
    ColumnRef original{"products", "sku"};
    const auto j        = original.toJSON();
    const auto restored = ColumnRef::fromJSON(j);

    EXPECT_EQ(restored.table_name,  original.table_name);
    EXPECT_EQ(restored.column_name, original.column_name);
    EXPECT_EQ(restored, original);
}

// ---------------------------------------------------------------------------
// MCH-LN04: TransformationType string conversion round-trips for all types
// ---------------------------------------------------------------------------
TEST(ColumnLineageTraversalEdgeTest, MCHLLN04_TransformationTypeRoundTrip) {
    const std::vector<TransformationType> all_types = {
        TransformationType::DIRECT_COPY,
        TransformationType::RENAME,
        TransformationType::CAST,
        TransformationType::COMPUTED,
        TransformationType::AGGREGATION,
        TransformationType::ANONYMIZATION,
        TransformationType::ENRICHMENT,
        TransformationType::CUSTOM,
    };

    for (const auto t : all_types) {
        const std::string s = transformationTypeToString(t);
        EXPECT_FALSE(s.empty()) << "transformationTypeToString() returned empty for type";

        const TransformationType restored = transformationTypeFromString(s);
        EXPECT_EQ(restored, t)
            << "Round-trip failed for: " << s;
    }
}
