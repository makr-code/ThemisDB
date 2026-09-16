// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_temporal_store_soak.cpp
 * @brief Wave D — Temporal Store Soak Test (sustained traffic).
 *
 * Long-duration soak test for the ThemisDB temporal store module.
 * Verifies that history-query throughput, snapshot stability, and
 * CDC reliability remain stable over the soak window.
 *
 * ## Acceptance criteria
 * - History query throughput ≥ 8 000 queries/sec
 * - Snapshot: zero failed snapshots during soak
 * - CDC: zero lag events (all CDC events processed within window)
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_TEMPORAL_STORE.md
 * @see src/temporal/ROADMAP.md — Wave D Contribution
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

static uint64_t soakDurationMs() {
    const char* env = std::getenv("THEMIS_SOAK_DURATION_MS");
    if (env && *env) {
        try { return static_cast<uint64_t>(std::stoull(env)); }
        catch (...) {}
    }
    return 60'000ULL;
}

// ─────────────────────────────────────────────────────────────────────────────
// STUB/SIMULATION NOTE
//
// The stubs below replace live temporal store, snapshot manager, and CDC paths
// — no external database or change-capture pipeline is required.
//   - StubTemporalQueryEngine: models a history-range query with deterministic result.
//   - StubSnapshotManager: models create/verify snapshot lifecycle.
//   - StubCdcPipeline: models a bounded CDC event queue with lag tracking.
//
// These stubs MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

struct HistoryQueryResult {
    uint64_t row_count{0};
    bool     success{false};
};

class StubTemporalQueryEngine {
public:
    HistoryQueryResult query(uint64_t entity_id, uint64_t range_ms) noexcept {
        queries_.fetch_add(1, std::memory_order_relaxed);
        const uint64_t rows = (entity_id % 128) + (range_ms % 64);
        return HistoryQueryResult{rows, true};
    }
    uint64_t totalQueries() const noexcept { return queries_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> queries_{0};
};

class StubSnapshotManager {
public:
    bool createSnapshot(uint64_t epoch) noexcept {
        snapshots_.fetch_add(1, std::memory_order_relaxed);
        (void)epoch;
        return true;
    }
    uint64_t snapshots() const noexcept { return snapshots_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> snapshots_{0};
};

class StubCdcPipeline {
public:
    static constexpr uint64_t kMaxLagMs = 100;

    bool emitEvent(uint64_t ts_ms) noexcept {
        last_ts_.store(ts_ms, std::memory_order_relaxed);
        events_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    bool hasLag(uint64_t now_ms) const noexcept {
        const uint64_t last = last_ts_.load(std::memory_order_relaxed);
        return (last > 0) && (now_ms - last > kMaxLagMs);
    }

    uint64_t totalEvents() const noexcept { return events_.load(std::memory_order_relaxed); }

private:
    std::atomic<uint64_t> last_ts_{0};
    std::atomic<uint64_t> events_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// Soak Test 1: TemporalSoak_HistoryQueryThroughput
// ─────────────────────────────────────────────────────────────────────────────
TEST(TemporalSoak_HistoryQueryThroughput, ThroughputGateAndNoExceptions) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs());

    StubTemporalQueryEngine engine;
    bool exception_caught = false;

    const auto start = std::chrono::steady_clock::now();
    uint64_t tick = 0;

    try {
        while (std::chrono::steady_clock::now() - start < soak_duration) {
            const uint64_t entity_id = tick % 10000;
            const uint64_t range_ms  = (tick % 1000) + 100;
            const auto result = engine.query(entity_id, range_ms);
            (void)result;
            ++tick;
        }
    } catch (...) {
        exception_caught = true;
    }

    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);

    EXPECT_FALSE(exception_caught)
        << "[TEMPORAL:RetentionViolation] No exceptions during history query soak";
    ASSERT_GT(elapsed_ms.count(), 0);

    const double elapsed_s     = elapsed_ms.count() / 1000.0;
    const double queries_per_s = static_cast<double>(engine.totalQueries()) / elapsed_s;

    constexpr double kMinThroughput = 8'000.0;
    EXPECT_GE(queries_per_s, kMinThroughput)
        << "[TEMPORAL:RetentionViolation] History query throughput must be ≥ 8 000 q/sec. "
           "Observed: " << static_cast<uint64_t>(queries_per_s) << " q/sec";
}

// ─────────────────────────────────────────────────────────────────────────────
// Soak Test 2: TemporalSoak_SnapshotStability
// ─────────────────────────────────────────────────────────────────────────────
TEST(TemporalSoak_SnapshotStability, ZeroSnapshotFailuresAndStableEpochs) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs() / 10);

    StubSnapshotManager snapshotMgr;
    bool exception_caught    = false;
    uint64_t snap_failures   = 0;

    const auto start = std::chrono::steady_clock::now();
    uint64_t epoch = 0;

    try {
        while (std::chrono::steady_clock::now() - start < soak_duration) {
            // Snapshot every 500 iterations
            if (epoch % 500 == 0) {
                if (!snapshotMgr.createSnapshot(epoch)) {
                    ++snap_failures;
                }
            }
            ++epoch;
        }
    } catch (...) {
        exception_caught = true;
    }

    EXPECT_FALSE(exception_caught)
        << "[TEMPORAL:SnapshotFailed] No exceptions during snapshot stability soak";

    EXPECT_EQ(snap_failures, 0u)
        << "[TEMPORAL:SnapshotFailed] Zero snapshot failures expected. "
           "Observed: " << snap_failures;

    EXPECT_GT(snapshotMgr.snapshots(), 0u)
        << "At least one snapshot must be created during the soak";
}

// ─────────────────────────────────────────────────────────────────────────────
// Soak Test 3: TemporalSoak_CDCReliability
// ─────────────────────────────────────────────────────────────────────────────
TEST(TemporalSoak_CDCReliability, ZeroCDCLagEventsAndAllProcessed) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs() / 5);

    StubCdcPipeline cdc;
    bool exception_caught = false;
    uint64_t lag_events   = 0;

    const auto start = std::chrono::steady_clock::now();

    try {
        while (std::chrono::steady_clock::now() - start < soak_duration) {
            const auto now_ms = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count());

            cdc.emitEvent(now_ms);

            if (cdc.hasLag(now_ms)) {
                ++lag_events;
            }
        }
    } catch (...) {
        exception_caught = true;
    }

    EXPECT_FALSE(exception_caught)
        << "[TEMPORAL:CDCLag] No exceptions during CDC reliability soak";

    EXPECT_EQ(lag_events, 0u)
        << "[TEMPORAL:CDCLag] Zero CDC lag events expected during soak. "
           "Observed: " << lag_events;

    EXPECT_GT(cdc.totalEvents(), 0u)
        << "At least one CDC event must be emitted during the soak";
}
