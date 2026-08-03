// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_distributed_catalog_diagnostics_focused.cpp
 * @brief Phase C1: Diagnostics tests for DistributedMetadataCatalog (stat counters).
 * @note Test IDs: MCH-DC01..MCH-DC04
 *
 * Uses the RecordingMetadataChangeListener and MetadataSnapshot helpers (both
 * header-only) to validate the operator-observable diagnostics surface of the
 * metadata module without requiring a live cluster or RocksDB instance.
 *
 *   MCH-DC01  RecordingMetadataChangeListener starts with zero events
 *   MCH-DC02  onMetadataChanged() increments eventCount()
 *   MCH-DC03  lastEvent() returns the most recently recorded event
 *   MCH-DC04  clear() resets the event recorder to zero
 *
 * Self-contained: header-only types only, no network I/O, no RocksDB.
 * Canonical PRNG seed: kDistributedDiagSeed = 42.
 *
 * @see include/metadata/imetadata_change_listener.h
 * @see src/metadata/ROADMAP.md — Phase C diagnostics items
 */

#include <gtest/gtest.h>

#include "metadata/imetadata_change_listener.h"

#include <string>

using namespace themis::metadata;

namespace {

[[maybe_unused]] static constexpr uint64_t kDistributedDiagSeed = 42;

MetadataChangeEvent makeEvent(MetadataChangeType type, const std::string& table) {
    MetadataChangeEvent ev;
    ev.change_type = type;
    ev.table_name  = table;
    ev.actor       = "test-agent";
    ev.detail      = "automated test event";
    ev.timestamp   = std::chrono::system_clock::now();
    return ev;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// MCH-DC01: RecordingMetadataChangeListener starts with zero events
// ---------------------------------------------------------------------------
TEST(DistributedCatalogDiagnosticsTest, MCHDC01_StartsWithZeroEvents) {
    RecordingMetadataChangeListener rec;
    EXPECT_EQ(rec.eventCount(), 0u);
    EXPECT_FALSE(rec.lastEvent().has_value());
}

// ---------------------------------------------------------------------------
// MCH-DC02: onMetadataChanged() increments eventCount()
// ---------------------------------------------------------------------------
TEST(DistributedCatalogDiagnosticsTest, MCHDC02_OnChangedIncrementsCount) {
    RecordingMetadataChangeListener rec;
    rec.onMetadataChanged(makeEvent(MetadataChangeType::TABLE_CREATED, "orders"));
    rec.onMetadataChanged(makeEvent(MetadataChangeType::TABLE_MODIFIED, "orders"));

    EXPECT_EQ(rec.eventCount(), 2u);
}

// ---------------------------------------------------------------------------
// MCH-DC03: lastEvent() returns the most recently recorded event
// ---------------------------------------------------------------------------
TEST(DistributedCatalogDiagnosticsTest, MCHDC03_LastEventReturnsNewest) {
    RecordingMetadataChangeListener rec;
    rec.onMetadataChanged(makeEvent(MetadataChangeType::TABLE_CREATED,  "users"));
    rec.onMetadataChanged(makeEvent(MetadataChangeType::TABLE_MODIFIED, "products"));

    const auto last = rec.lastEvent();
    ASSERT_TRUE(last.has_value());
    EXPECT_EQ(last->table_name, "products");
    EXPECT_EQ(last->change_type, MetadataChangeType::TABLE_MODIFIED);
}

// ---------------------------------------------------------------------------
// MCH-DC04: clear() resets the event recorder to zero
// ---------------------------------------------------------------------------
TEST(DistributedCatalogDiagnosticsTest, MCHDC04_ClearResetsToZero) {
    RecordingMetadataChangeListener rec;
    rec.onMetadataChanged(makeEvent(MetadataChangeType::TABLE_CREATED, "catalog"));
    ASSERT_EQ(rec.eventCount(), 1u);

    rec.clear();

    EXPECT_EQ(rec.eventCount(), 0u);
    EXPECT_FALSE(rec.lastEvent().has_value());
}
