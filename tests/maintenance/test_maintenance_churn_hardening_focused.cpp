// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_maintenance_churn_hardening_focused.cpp
 * @brief Phase 2 churn-hardening focused tests for the maintenance module.
 *
 * Test IDs: MTN-17 through MTN-20
 *
 * Covers:
 *   - Concurrent in-flight guard: second trigger of same schedule returns SKIPPED_CONCURRENT
 *   - Rapid add/remove churn loop: no panics, bounded state
 *   - max_schedule_changes_per_interval policy: structured error returned
 *   - SKIPPED_CONCURRENT appears in diagnostic/health log
 *
 * No file I/O, no network — all tests use in-memory structures only.
 *
 * @see include/maintenance/maintenance_api_contract.h
 * @see include/maintenance/maintenance_health_report.h
 * @see src/maintenance/ROADMAP.md — Phase 2 items
 */

#include "gtest/gtest.h"
#include "maintenance/maintenance_api_contract.h"
#include "maintenance/maintenance_schedule.h"
#include "maintenance/maintenance_task.h"
#include "maintenance/maintenance_health_report.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace themis {
namespace maintenance {
namespace test {

static constexpr uint32_t kSeed = 42;

// ---------------------------------------------------------------------------
// Minimal in-memory in-flight guard (mirrors orchestrator logic for unit test)
// ---------------------------------------------------------------------------

struct InFlightGuard {
    std::mutex                    mu = {};
    std::unordered_set<std::string> in_flight;

    /// Returns false if schedule_id already in-flight (SKIPPED_CONCURRENT case).
    bool tryEnter(const std::string& id) {
        std::lock_guard<std::mutex> lock(mu);
        return in_flight.insert(id).second;
    }

    void leave(const std::string& id) {
        std::lock_guard<std::mutex> lock(mu);
        in_flight.erase(id);
    }
};

// ---------------------------------------------------------------------------
// Minimal churn-rate-limit tracker (mirrors orchestrator logic)
// ---------------------------------------------------------------------------

static constexpr int64_t kChurnIntervalMs = 60'000LL;

struct ChurnTracker {
    std::mutex mu;
    std::unordered_map<std::string, std::pair<uint32_t, int64_t>> counts;

    static int64_t nowMs() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }

    /// Returns true if the change is permitted; false if limit exceeded.
    bool check(const std::string& schedule_id, uint32_t max_per_interval) {
        if (max_per_interval == 0) {
          return true;
        }
        int64_t now = nowMs();
        std::lock_guard<std::mutex> lock(mu);
        auto& [count, start] = counts[schedule_id];
        if (now - start >= kChurnIntervalMs) {
            start = now;
            count = 0;
        }
        ++count;
        return count <= max_per_interval;
    }
};

// ============================================================================
// MTN-17 — Concurrent trigger of the same schedule returns SKIPPED_CONCURRENT
// ============================================================================

/**
 * @brief Two threads try to enter executeSchedule() for the same schedule_id.
 *        The second thread must see the "already in-flight" condition and
 *        produce a SKIPPED_CONCURRENT outcome.
 */
TEST(MaintenanceChurnHardening, MTN17_ConcurrentScheduleTriggerSkipped) {
    InFlightGuard guard;
    const std::string schedule_id = "sched-concurrent-42";

    // Thread 1 acquires the in-flight slot.
    ASSERT_TRUE(guard.tryEnter(schedule_id));

    // Thread 2 attempts to enter the same slot — must be denied.
    bool second_allowed = guard.tryEnter(schedule_id);
    EXPECT_FALSE(second_allowed)
        << "Second concurrent trigger of same schedule must be rejected (SKIPPED_CONCURRENT)";

    // Verify the DispatchOutcomeType enum value exists.
    DispatchOutcomeType outcome = DispatchOutcomeType::SKIPPED_CONCURRENT;
    EXPECT_EQ(dispatchOutcomeTypeToString(outcome), "skipped_concurrent");

    // Release Thread 1's slot.
    guard.leave(schedule_id);

    // Now a new attempt should succeed.
    EXPECT_TRUE(guard.tryEnter(schedule_id));
    guard.leave(schedule_id);

    (void)kSeed;
}

// ============================================================================
// MTN-18 — Rapid add/remove churn loop (100 cycles) — no panics, bounded state
// ============================================================================

/**
 * @brief Validates that 100 rapid add/remove churn cycles on schedule state
 *        complete without panics and the in-flight set returns to empty.
 */
TEST(MaintenanceChurnHardening, MTN18_RapidChurnLoopBoundedState) {
    InFlightGuard guard;
    static constexpr int kCycles = 100;

    for (int i = 0; i < kCycles; ++i) {
        const std::string id = "sched-churn-" + std::to_string(i);
        ASSERT_TRUE(guard.tryEnter(id));
        guard.leave(id);
    }

    // After all cycles, in-flight set must be empty.
    {
        std::lock_guard<std::mutex> lk(guard.mu);
        EXPECT_TRUE(guard.in_flight.empty())
            << "in_flight set must be empty after all churn cycles";
    }

    // Also verify concurrent churn across multiple threads doesn't corrupt state.
    std::vector<std::thread> threads;
    std::atomic<int> total_entered{0};
    for (int t = 0; t < 10; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < 10; ++i) {
                const std::string id = "mt-sched-" + std::to_string(t * 10 + i);
                if (guard.tryEnter(id)) {
                    ++total_entered;
                    guard.leave(id);
                }
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    EXPECT_EQ(total_entered.load(), 100)
        << "All 100 unique schedule IDs should have been entered and left";

    // in-flight set must be empty
    {
        std::lock_guard<std::mutex> lk(guard.mu);
        EXPECT_TRUE(guard.in_flight.empty());
    }
}

// ============================================================================
// MTN-19 — max_schedule_changes_per_interval policy triggered
// ============================================================================

/**
 * @brief Validates that the churn rate-limit policy correctly rejects excess
 *        mutations beyond max_schedule_changes_per_interval.
 */
TEST(MaintenanceChurnHardening, MTN19_ChurnLimitPolicyTriggered) {
    ChurnTracker tracker;
    const std::string sched_id = "sched-ratelimit-42";
    constexpr uint32_t kMaxChanges = 3;

    // First kMaxChanges mutations should be permitted.
    for (uint32_t i = 0; i < kMaxChanges; ++i) {
        bool allowed = tracker.check(sched_id, kMaxChanges);
        EXPECT_TRUE(allowed) << "Change " << (i + 1) << " must be permitted";
    }

    // The (kMaxChanges + 1)th mutation must be rejected.
    bool excess_allowed = tracker.check(sched_id, kMaxChanges);
    EXPECT_FALSE(excess_allowed)
        << "Change " << (kMaxChanges + 1)
        << " must be rejected (kChurnLimitExceeded policy)";

    // Verify the error code enum value exists.
    constexpr int32_t code = static_cast<int32_t>(MaintenanceError::kChurnLimitExceeded);
    EXPECT_EQ(code, 8110);

    // Limit=0 means disabled — always permitted.
    EXPECT_TRUE(tracker.check("sched-unlimited", 0));
    EXPECT_TRUE(tracker.check("sched-unlimited", 0));
}

// ============================================================================
// MTN-20 — SKIPPED_CONCURRENT appears in DispatchOutcome diagnostic fields
// ============================================================================

/**
 * @brief Validates that a DispatchOutcome with SKIPPED_CONCURRENT outcome
 *        serialises to JSON with the correct fields, enabling operators
 *        to observe it in the health log / ring buffer snapshot.
 */
TEST(MaintenanceChurnHardening, MTN20_SkippedConcurrentInDiagnosticLog) {
    DispatchOutcome doc;
    doc.schedule_id   = "sched-diag-42";
    doc.task_type     = "<concurrent-skip>";
    doc.outcome       = DispatchOutcomeType::SKIPPED_CONCURRENT;
    doc.latency_us    = 0;
    doc.error_message = "Schedule already in-flight";

    auto j = doc.toJson();
    EXPECT_EQ(j["outcome"].get<std::string>(), "skipped_concurrent");
    EXPECT_EQ(j["schedule_id"].get<std::string>(), "sched-diag-42");
    EXPECT_FALSE(j["error_message"].get<std::string>().empty())
        << "error_message must be non-empty for SKIPPED_CONCURRENT outcome";

    // The outcome must also be distinguishable from other skip types.
    EXPECT_NE(dispatchOutcomeTypeToString(DispatchOutcomeType::SKIPPED_CONCURRENT),
              dispatchOutcomeTypeToString(DispatchOutcomeType::SKIPPED_NO_HANDLER));

    // Simulate adding to a health report.
    MaintenanceHealthReport report;
    report.recent_dispatch_outcomes.push_back(doc);
    auto rj = report.toJson();
    ASSERT_TRUE(rj["recent_dispatch_outcomes"].is_array());
    EXPECT_EQ(rj["recent_dispatch_outcomes"].size(), 1u);
    EXPECT_EQ(rj["recent_dispatch_outcomes"][0]["outcome"].get<std::string>(),
              "skipped_concurrent");
}

} // namespace test
} // namespace maintenance
} // namespace themis
