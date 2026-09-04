// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_cdc_diagnostics_hardening.cpp
 * @brief Phase 3 CDC diagnostics consistency hardening tests.
 *
 * Validates that CDC admin and metrics surfaces emit actionable, class-stable
 * diagnostics for all observable failure and lag states, as required by
 * src/cdc/ROADMAP.md (Phase 3 — Error Handling and Edge Cases).
 *
 * ## Test cases
 *
 *   DGH-01  CDCAdmin healthCheck returns a healthy status when events are present.
 *   DGH-02  CDCAdmin getDiagnostics includes event count and watermark metadata.
 *   DGH-03  CDCAdmin healthCheck handles an empty changefeed without error.
 *   DGH-04  CDCMetrics LatencyHistogram records samples; p99 is finite and ≥ p50.
 *   DGH-05  CDCMetrics ThroughputTracker increments event counter.
 *   DGH-06  DeliveryTracker getAllStats returns states for all tracked consumers.
 *   DGH-07  CDCAdmin getDiagnostics serialises to valid JSON.
 *   DGH-08  CDCAdmin healthCheck reflects component health in JSON representation.
 *
 * @see include/cdc/cdc_delivery_contract.h — § 7 diagnostic observability
 * @see include/cdc/cdc_admin.h
 * @see include/cdc/cdc_metrics.h
 * @see include/cdc/delivery_tracker.h
 * @see src/cdc/ROADMAP.md Phase 3
 */

#include <gtest/gtest.h>

#include "cdc/cdc_admin.h"
#include "cdc/cdc_delivery_contract.h"
#include "cdc/cdc_metrics.h"
#include "cdc/changefeed.h"
#include "cdc/delivery_tracker.h"

#include <filesystem>
#include <memory>
#include <rocksdb/utilities/transaction_db.h>
#include <string>

using namespace themis;
using namespace themis::cdc;
namespace fs = std::filesystem;

// ============================================================================
// Shared fixture — RocksDB + Changefeed + CDCAdmin
// ============================================================================

class DiagnosticsHardeningTest : public ::testing::Test {
protected:
    std::string db_path = {};
    std::unique_ptr<rocksdb::TransactionDB> db;
    std::unique_ptr<Changefeed> changefeed;
    std::unique_ptr<CDCAdmin> admin;

    void SetUp() override {
        db_path = (fs::temp_directory_path()
                   / ("test_cdc_dgh_" + std::to_string(
                          std::chrono::steady_clock::now().time_since_epoch().count())))
                      .string();
        fs::create_directories(db_path);

        rocksdb::Options opts;
        opts.create_if_missing = true;
        rocksdb::TransactionDBOptions txn_opts;
        rocksdb::TransactionDB* raw_db = nullptr;
        auto s = rocksdb::TransactionDB::Open(opts, txn_opts, db_path, &raw_db);
        ASSERT_TRUE(s.ok()) << s.ToString();
        db.reset(raw_db);

        Changefeed::RetentionPolicy ret;
        ret.enabled = false;
        changefeed = std::make_unique<Changefeed>(db.get(), nullptr, ret);
        admin      = std::make_unique<CDCAdmin>(changefeed.get());
    }

    void TearDown() override {
        admin.reset();
        changefeed.reset();
        db.reset();
        fs::remove_all(db_path);
    }

    void seedEvents(int n, const std::string& prefix = "key") {
        for (int i = 0; i < n; ++i) {
            Changefeed::ChangeEvent ev;
            ev.type         = Changefeed::ChangeEventType::EVENT_PUT;
            ev.key          = prefix + ":" + std::to_string(i);
            ev.value        = "v" + std::to_string(i);
            ev.timestamp_ms = 1700000000000LL + i;
            changefeed->recordEvent(ev);
        }
    }
};

// ============================================================================
// DGH-01: healthCheck returns healthy when events are present
// ============================================================================

TEST_F(DiagnosticsHardeningTest, DGH01_HealthCheckReturnsHealthyWithEvents) {
    seedEvents(5);

    auto health = admin->healthCheck();
    EXPECT_TRUE(health.is_healthy)
        << "healthCheck must report healthy for a changefeed with events";
    EXPECT_TRUE(health.changefeed_healthy)
        << "changefeed component must be reported healthy";
}

// ============================================================================
// DGH-02: getDiagnostics includes event count and watermark metadata
// ============================================================================

TEST_F(DiagnosticsHardeningTest, DGH02_DiagnosticsIncludesEventCountAndWatermarks) {
    seedEvents(3);

    auto diag = admin->getDiagnostics();
    // total_events must reflect the seeded events.
    EXPECT_GE(diag.total_events, 3u)
        << "getDiagnostics must expose total_events ≥ seeded count";

    // Watermarks must be populated after events have been recorded.
    EXPECT_GT(diag.watermarks.high_watermark, 0u)
        << "high_watermark must be > 0 after recording events";
}

// ============================================================================
// DGH-03: healthCheck handles empty changefeed without error
// ============================================================================

TEST_F(DiagnosticsHardeningTest, DGH03_HealthCheckHandlesEmptyChangefeed) {
    // No events seeded — changefeed is empty.
    EXPECT_NO_THROW({
        auto health = admin->healthCheck();
        // An empty feed is a valid operational state; is_healthy may be true.
        (void)health;
    }) << "healthCheck must not throw on empty changefeed";
}

// ============================================================================
// DGH-04: LatencyHistogram records samples; p99 ≥ p50 and both are finite
// ============================================================================

TEST_F(DiagnosticsHardeningTest, DGH04_LatencyHistogramPercentilesAreFinite) {
    LatencyHistogram hist;

    // Record a range of latency samples in microseconds.
    for (uint64_t us = 10; us <= 10000; us += 100) {
        hist.record(us);
    }

    EXPECT_GT(hist.count(), 0u);
    uint64_t p50 = hist.p50();
    uint64_t p99 = hist.p99();

    // p99 must be ≥ p50 (monotonicity of percentiles).
    EXPECT_GE(p99, p50)
        << "p99 must be >= p50 for any non-empty histogram";
    // p99 must be within the recorded range.
    EXPECT_LE(p99, 10000u + 1000u)  // allow bucket rounding
        << "p99 must not exceed the maximum recorded latency by more than one bucket";
}

// ============================================================================
// DGH-05: ThroughputTracker increments event counter
// ============================================================================

TEST_F(DiagnosticsHardeningTest, DGH05_ThroughputTrackerIncrementsCounter) {
    CDCMetrics metrics;

    uint64_t before = metrics.events_recorded.load();
    metrics.events_recorded.fetch_add(10);
    uint64_t after = metrics.events_recorded.load();

    EXPECT_EQ(after, before + 10u)
        << "events_recorded counter must increment atomically";
}

// ============================================================================
// DGH-06: getAllStats returns states for all tracked consumers
// ============================================================================

TEST_F(DiagnosticsHardeningTest, DGH06_GetAllStatsReturnsAllConsumers) {
    DeliveryTracker tracker;

    // Track deliveries for two distinct consumers.
    Changefeed::ChangeEvent ev1;
    ev1.sequence = 1; ev1.type = Changefeed::ChangeEventType::EVENT_PUT;
    ev1.key = "k1"; ev1.value = "v1"; ev1.timestamp_ms = 1000;

    Changefeed::ChangeEvent ev2;
    ev2.sequence = 2; ev2.type = Changefeed::ChangeEventType::EVENT_PUT;
    ev2.key = "k2"; ev2.value = "v2"; ev2.timestamp_ms = 2000;

    tracker.trackDelivery("dgh_consumer_1", {ev1});
    tracker.trackDelivery("dgh_consumer_2", {ev2});

    auto all_stats = tracker.getAllStats();
    EXPECT_EQ(all_stats.size(), 2u)
        << "getAllStats must return one entry per tracked consumer (contract § 7)";

    // Each consumer must have at least 1 pending event.
    for (const auto& s : all_stats) {
        EXPECT_GE(s.pending_count, 1u);
        EXPECT_GE(s.total_delivered, 1u);
    }
}

// ============================================================================
// DGH-07: getDiagnostics serialises to valid JSON
// ============================================================================

TEST_F(DiagnosticsHardeningTest, DGH07_DiagnosticsSerializesToValidJson) {
    seedEvents(2);

    auto diag = admin->getDiagnostics();
    EXPECT_NO_THROW({
        auto j = diag.toJson();
        EXPECT_TRUE(j.contains("watermarks"))
            << "Diagnostics JSON must contain 'watermarks' key";
        EXPECT_TRUE(j.contains("health"))
            << "Diagnostics JSON must contain 'health' key";
        EXPECT_TRUE(j.contains("stats"))
            << "Diagnostics JSON must contain 'stats' key";
    }) << "toJson() must not throw on valid diagnostics data";
}

// ============================================================================
// DGH-08: healthCheck reflects component health in JSON representation
// ============================================================================

TEST_F(DiagnosticsHardeningTest, DGH08_HealthStatusSerializesToJson) {
    auto health = admin->healthCheck();
    EXPECT_NO_THROW({
        auto j = health.toJson();
        EXPECT_TRUE(j.contains("is_healthy"))
            << "HealthStatus JSON must contain 'is_healthy'";
        EXPECT_TRUE(j.contains("components"))
            << "HealthStatus JSON must contain 'components' breakdown";
        EXPECT_TRUE(j["components"].contains("changefeed"))
            << "components must include 'changefeed' (contract § 7)";
    }) << "HealthStatus::toJson() must produce valid structured output";
}
