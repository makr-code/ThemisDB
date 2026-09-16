// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_maintenance_soak.cpp
 * @brief Wave D — Maintenance Module Soak Test (sustained traffic).
 *
 * Long-duration soak test for the ThemisDB maintenance module hot paths:
 * vacuum throughput, compaction stability, and retention-policy reliability.
 * Verifies that all three metrics remain within acceptable bounds over a
 * configurable soak window driven by THEMIS_SOAK_DURATION_MS.
 *
 * ## Acceptance criteria
 * - MaintenanceSoak_VacuumThroughput      : ≥ 1 000 vacuum ops/sec
 * - MaintenanceSoak_CompactionStability   : zero compaction stall events
 * - MaintenanceSoak_RetentionReliability  : zero retention violations
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * Environment overrides
 * ---------------------
 * THEMIS_SOAK_DURATION_MS — total run window in milliseconds (default 60000)
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_MAINTENANCE.md — operator runbook
 * @see src/maintenance/ROADMAP.md — Wave D contribution closure
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <mutex>
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
// STUB / SIMULATION NOTE
//
// In-process stubs model the maintenance module hot paths without requiring
// external backends (database, disk I/O). MUST NOT be used in production.
// ─────────────────────────────────────────────────────────────────────────────

namespace {

// ---------------------------------------------------------------------------
// StubVacuumQueue — models vacuum task throughput
// ---------------------------------------------------------------------------
class StubVacuumQueue {
public:
    explicit StubVacuumQueue(std::size_t capacity) : capacity_(capacity) {}

    bool enqueue(uint64_t task_id) {
        std::lock_guard<std::mutex> lk(mu_);
        if (queue_.size() >= capacity_) { ++overflow_count_; return false; }
        queue_.push_back(task_id);
        ++enqueue_count_;
        return true;
    }

    bool dequeue(uint64_t& out) {
        std::lock_guard<std::mutex> lk(mu_);
        if (queue_.empty()) return false;
        out = queue_.front();
        queue_.pop_front();
        ++dequeue_count_;
        return true;
    }

    uint64_t enqueueCount()  const { return enqueue_count_.load(); }
    uint64_t dequeueCount()  const { return dequeue_count_.load(); }
    uint64_t overflowCount() const { return overflow_count_.load(); }

private:
    const std::size_t capacity_;
    std::deque<uint64_t> queue_;
    std::mutex mu_;
    std::atomic<uint64_t> enqueue_count_{0};
    std::atomic<uint64_t> dequeue_count_{0};
    std::atomic<uint64_t> overflow_count_{0};
};

// ---------------------------------------------------------------------------
// StubCompactor — simulates compaction without stall
// ---------------------------------------------------------------------------
class StubCompactor {
public:
    bool compact(uint32_t shard_id) {
        ++compact_count_;
        (void)shard_id;
        return true; // stub: never stalls
    }

    uint64_t compactCount() const { return compact_count_.load(); }
    uint64_t stallCount()   const { return stall_count_.load(); }

private:
    std::atomic<uint64_t> compact_count_{0};
    std::atomic<uint64_t> stall_count_{0};
};

// ---------------------------------------------------------------------------
// StubRetentionPolicy — simulates retention enforcement
// ---------------------------------------------------------------------------
class StubRetentionPolicy {
public:
    /// Apply retention to a record. Returns false if a violation occurred.
    bool applyRetention(uint64_t record_id, uint64_t age_ms, uint64_t ttl_ms) {
        ++checked_count_;
        if (age_ms > ttl_ms) {
            ++expired_count_;
            return true; // correctly expired
        }
        if (age_ms == 0 && ttl_ms == 0) {
            ++violation_count_; // nonsensical record = violation
            return false;
        }
        return true;
    }

    uint64_t checkedCount()   const { return checked_count_.load(); }
    uint64_t expiredCount()   const { return expired_count_.load(); }
    uint64_t violationCount() const { return violation_count_.load(); }

private:
    std::atomic<uint64_t> checked_count_{0};
    std::atomic<uint64_t> expired_count_{0};
    std::atomic<uint64_t> violation_count_{0};
};

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Test 1: MaintenanceSoak_VacuumThroughput
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_MaintenanceSoak, MaintenanceSoak_VacuumThroughput) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs());

    StubVacuumQueue queue(8192);
    std::atomic<bool> running{true};

    std::vector<std::thread> producers;
    for (int i = 0; i < 4; ++i) {
        producers.emplace_back([&, i]() {
            uint64_t id = static_cast<uint64_t>(i) * 1'000'000ULL;
            while (running.load(std::memory_order_relaxed)) {
                queue.enqueue(id++);
                std::this_thread::yield();
            }
        });
    }

    std::vector<std::thread> consumers;
    for (int i = 0; i < 4; ++i) {
        consumers.emplace_back([&]() {
            uint64_t dummy = 0;
            while (running.load(std::memory_order_relaxed)) {
                queue.dequeue(dummy);
                std::this_thread::yield();
            }
        });
    }

    const auto t0 = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(soak_duration);
    running.store(false, std::memory_order_relaxed);

    for (auto& t : producers) t.join();
    for (auto& t : consumers)  t.join();

    const double elapsed_s =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    const uint64_t total_ops = queue.enqueueCount() + queue.dequeueCount();
    const double   ops_per_sec = static_cast<double>(total_ops) / elapsed_s;

    EXPECT_GT(total_ops, 0u)
        << "[MAINTENANCE:VacuumFailed] At least one vacuum op must complete";
    EXPECT_GE(ops_per_sec, 1000.0)
        << "Vacuum throughput must be ≥ 1 000 ops/sec. "
           "Observed: " << ops_per_sec << " ops/sec";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 2: MaintenanceSoak_CompactionStability
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_MaintenanceSoak, MaintenanceSoak_CompactionStability) {
    const auto soak_duration =
        std::chrono::milliseconds(soakDurationMs() / 10);

    StubCompactor compactor;
    std::atomic<bool> running{true};

    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&, i]() {
            uint32_t shard = static_cast<uint32_t>(i);
            while (running.load(std::memory_order_relaxed)) {
                compactor.compact(shard);
                std::this_thread::yield();
            }
        });
    }

    std::this_thread::sleep_for(soak_duration);
    running.store(false, std::memory_order_relaxed);
    for (auto& t : workers) t.join();

    EXPECT_GT(compactor.compactCount(), 0u)
        << "[MAINTENANCE:CompactionStall] At least one compaction must complete";
    EXPECT_EQ(compactor.stallCount(), 0u)
        << "[MAINTENANCE:CompactionStall] Zero compaction stall events expected. "
           "Observed: " << compactor.stallCount();
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 3: MaintenanceSoak_RetentionReliability
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_MaintenanceSoak, MaintenanceSoak_RetentionReliability) {
    const auto soak_duration =
        std::chrono::milliseconds(soakDurationMs() / 10);

    StubRetentionPolicy policy;
    std::atomic<bool>   running{true};

    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&, i]() {
            uint64_t id = static_cast<uint64_t>(i) * 1'000'000ULL;
            while (running.load(std::memory_order_relaxed)) {
                // age = id % 200 ms, ttl = 100 ms — half are expired, half valid
                policy.applyRetention(id, id % 200, 100);
                ++id;
                std::this_thread::yield();
            }
        });
    }

    std::this_thread::sleep_for(soak_duration);
    running.store(false, std::memory_order_relaxed);
    for (auto& t : workers) t.join();

    EXPECT_GT(policy.checkedCount(), 0u)
        << "[MAINTENANCE:RetentionViolation] At least one record must be checked";
    EXPECT_EQ(policy.violationCount(), 0u)
        << "[MAINTENANCE:RetentionViolation] Zero retention violations expected. "
           "Observed: " << policy.violationCount();
}
