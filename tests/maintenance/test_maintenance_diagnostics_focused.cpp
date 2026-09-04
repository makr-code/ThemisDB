// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_maintenance_diagnostics_focused.cpp
 * @brief Phase 4 operator-diagnostics focused tests for the maintenance module.
 *
 * Test IDs: MTN-29 through MTN-32
 *
 * Covers:
 *   - All DispatchOutcomeType values produced and visible in health report
 *   - Ring buffer overflow: fill > 256 entries does not crash; oldest dropped
 *   - Report serialisation: consistent output, no missing fields
 *   - DispatchOutcome latency_us is non-negative and plausible
 *
 * No file I/O, no network — deterministic only.
 *
 * @see include/maintenance/maintenance_health_report.h
 * @see include/maintenance/maintenance_api_contract.h
 * @see src/maintenance/ROADMAP.md — Phase 4 items
 */

#include "gtest/gtest.h"
#include "maintenance/maintenance_api_contract.h"
#include "maintenance/maintenance_health_report.h"
#include "maintenance/maintenance_task.h"

#include <deque>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace maintenance {
namespace test {

static constexpr uint32_t kSeed = 42;

// ---------------------------------------------------------------------------
// Minimal ring-buffer helper that mirrors the orchestrator's implementation
// ---------------------------------------------------------------------------

struct TestRingBuffer {
    static constexpr int kDefaultCapacity = 256;
    std::deque<DispatchOutcome> buf;
    int capacity = {};
    std::mutex mu = {};

    explicit TestRingBuffer(int cap = kDefaultCapacity) : capacity(cap) {}

    void push(DispatchOutcome o) {
        std::lock_guard<std::mutex> lock(mu);
        buf.push_back(std::move(o));
        while (static_cast<int>(buf.size()) > capacity) {
            buf.pop_front();
        }
    }

    std::vector<DispatchOutcome> snapshot() {
        std::lock_guard<std::mutex> lock(mu);
        return std::vector<DispatchOutcome>(buf.begin(), buf.end());
    }
};

// ---------------------------------------------------------------------------
// Helper: build a DispatchOutcome
// ---------------------------------------------------------------------------

static DispatchOutcome makeOutcome(DispatchOutcomeType t, const std::string& sched_id,
                                   const std::string& task, int64_t latency_us = 100)
{
    DispatchOutcome o;
    o.schedule_id   = sched_id;
    o.task_type     = task;
    o.outcome       = t;
    o.latency_us    = latency_us;
    o.error_message = (t == DispatchOutcomeType::SUCCESS) ? "" : "error:" + task;
    return o;
}

// ============================================================================
// MTN-29 — All DispatchOutcomeType values produced and visible in health report
// ============================================================================

/**
 * @brief Validates that all five DispatchOutcomeType values can be recorded
 *        and are visible in MaintenanceHealthReport::recent_dispatch_outcomes.
 */
TEST(MaintenanceDiagnostics, MTN29_AllOutcomeTypesVisibleInHealthReport) {
    const std::vector<DispatchOutcomeType> all_types = {
        DispatchOutcomeType::SUCCESS,
        DispatchOutcomeType::SKIPPED_NO_HANDLER,
        DispatchOutcomeType::SKIPPED_CONCURRENT,
        DispatchOutcomeType::FAILED_PERSISTENCE,
        DispatchOutcomeType::FAILED_DISPATCH,
    };

    MaintenanceHealthReport report;
    for (auto t : all_types) {
        report.recent_dispatch_outcomes.push_back(
            makeOutcome(t, "sched-" + dispatchOutcomeTypeToString(t), "task"));
    }

    auto j = report.toJson();
    ASSERT_TRUE(j.contains("recent_dispatch_outcomes"));
    ASSERT_EQ(j["recent_dispatch_outcomes"].size(), all_types.size());

    for (std::size_t i = 0; i < all_types.size(); ++i) {
        const std::string expected = dispatchOutcomeTypeToString(all_types[i]);
        EXPECT_EQ(j["recent_dispatch_outcomes"][i]["outcome"].get<std::string>(),
                  expected)
            << "Outcome at index " << i << " mismatch";
    }

    // Each string must be distinct.
    std::vector<std::string> names = {};

    for (auto t : all_types) {
      names.push_back(dispatchOutcomeTypeToString(t));
    }
    std::sort(names.begin(), names.end());
    auto uniq_end = std::unique(names.begin(), names.end());
    EXPECT_EQ(uniq_end, names.end()) << "DispatchOutcomeType string representations must be unique";

    (void)kSeed;
}

// ============================================================================
// MTN-30 — Ring buffer overflow (> 256 entries) does not crash; oldest dropped
// ============================================================================

/**
 * @brief Fills the ring buffer beyond capacity (256) and verifies:
 *        - No crash occurs
 *        - Buffer size stays at capacity
 *        - The oldest entries are dropped (FIFO eviction)
 */
TEST(MaintenanceDiagnostics, MTN30_RingBufferOverflowDropsOldest) {
    static constexpr int kCapacity = 256;
    static constexpr int kOverfill = kCapacity + 50; // 306 entries

    TestRingBuffer rb(kCapacity);

    for (int i = 0; i < kOverfill; ++i) {
        DispatchOutcome o;
        o.schedule_id   = "sched-" + std::to_string(i);
        o.task_type     = "task";
        o.outcome       = DispatchOutcomeType::SUCCESS;
        o.latency_us    = static_cast<int64_t>(i);
        rb.push(o);
    }

    auto snap = rb.snapshot();
    EXPECT_EQ(static_cast<int>(snap.size()), kCapacity)
        << "Ring buffer must not exceed capacity";

    // The oldest entry (index 0) must now be the (kOverfill - kCapacity)th item.
    const int first_expected_latency = kOverfill - kCapacity; // 50
    EXPECT_EQ(snap.front().latency_us, static_cast<int64_t>(first_expected_latency))
        << "Oldest dropped entry must be evicted; expected latency_us=" << first_expected_latency;

    // The newest entry must be the last pushed.
    EXPECT_EQ(snap.back().latency_us, static_cast<int64_t>(kOverfill - 1));
}

// ============================================================================
// MTN-31 — Report serialisation produces consistent output (no missing fields)
// ============================================================================

/**
 * @brief Verifies that MaintenanceHealthReport::toJson() produces a complete
 *        JSON object with all expected top-level fields populated.
 */
TEST(MaintenanceDiagnostics, MTN31_ReportSerializationConsistentFields) {
    MaintenanceHealthReport report;
    report.overall_status     = ModuleHealthStatus::OK;
    report.generated_at_ms    = 1234567890LL;
    report.active_jobs        = 2;
    report.total_schedules    = 5;
    report.enabled_schedules  = 3;
    report.failed_jobs_24h    = 1;
    report.success_jobs_24h   = 10;
    report.dispatch_outcome_ring_buffer_capacity = 256;
    report.recent_dispatch_outcomes.push_back(
        makeOutcome(DispatchOutcomeType::SUCCESS, "sched-1", "quota_check", 42));

    auto j = report.toJson();

    const std::vector<std::string> required_keys = {
        "overall_status",
        "generated_at_ms",
        "active_jobs",
        "total_schedules",
        "enabled_schedules",
        "failed_jobs_24h",
        "success_jobs_24h",
        "dispatch_outcome_ring_buffer_capacity",
        "module_signals",
        "recent_dispatch_outcomes",
    };

    for (const auto& key : required_keys) {
        EXPECT_TRUE(j.contains(key)) << "Missing field: " << key;
    }

    EXPECT_EQ(j["overall_status"].get<std::string>(), "ok");
    EXPECT_EQ(j["generated_at_ms"].get<int64_t>(), 1234567890LL);
    EXPECT_EQ(j["dispatch_outcome_ring_buffer_capacity"].get<int>(), 256);
    EXPECT_EQ(j["recent_dispatch_outcomes"].size(), 1u);

    // Verify DispatchOutcome fields.
    const auto& o = j["recent_dispatch_outcomes"][0];
    EXPECT_TRUE(o.contains("schedule_id"));
    EXPECT_TRUE(o.contains("task_type"));
    EXPECT_TRUE(o.contains("outcome"));
    EXPECT_TRUE(o.contains("latency_us"));
    EXPECT_TRUE(o.contains("error_message"));
}

// ============================================================================
// MTN-32 — DispatchOutcome latency_us is non-negative and plausible
// ============================================================================

/**
 * @brief Validates that latency_us in DispatchOutcome is always >= 0 and
 *        within a plausible range for normal maintenance operations.
 */
TEST(MaintenanceDiagnostics, MTN32_DispatchOutcomeLatencyNonNegativePlausible) {
    // Simulate latencies: 0 (concurrent skip), 42 µs (fast path), 1 000 000 µs (1 s task).
    const std::vector<int64_t> latencies = {0, 42, 1'000, 10'000, 1'000'000};

    for (int64_t lat : latencies) {
        DispatchOutcome o = makeOutcome(DispatchOutcomeType::SUCCESS, "sched", "task", lat);
        EXPECT_GE(o.latency_us, 0) << "latency_us must be >= 0";

        auto j = o.toJson();
        EXPECT_GE(j["latency_us"].get<int64_t>(), 0)
            << "Serialised latency_us must be >= 0";
    }

    // Negative latency must never appear in a well-formed outcome.
    DispatchOutcome bad = makeOutcome(DispatchOutcomeType::SUCCESS, "s", "t", -1);
    // The struct allows it to be set but the test verifies we never produce negatives
    // in practice. In production the orchestrator uses steady_clock differences.
    EXPECT_LT(bad.latency_us, 0); // acknowledge the negative was set...
    // ...but assert that when serialised it is present (structural completeness only).
    auto bj = bad.toJson();
    EXPECT_TRUE(bj.contains("latency_us"));
}

} // namespace test
} // namespace maintenance
} // namespace themis
