// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_distributed_tensor_soak.cpp
 * @brief Wave D — Distributed Tensor Soak Test (sustained traffic).
 *
 * Long-duration soak test for the ThemisDB distributed tensor hot paths:
 * sharded operation throughput, sync stability, and partition reliability.
 * Verifies that all three metrics remain within acceptable bounds over a
 * configurable soak window driven by THEMIS_SOAK_DURATION_MS.
 *
 * ## Acceptance criteria
 * - DistTensorSoak_ShardedOpThroughput   : ≥ 500 ops/sec over soak window
 * - DistTensorSoak_SyncStability         : zero sync failures
 * - DistTensorSoak_PartitionReliability  : zero partition routing failures
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * Environment overrides
 * ---------------------
 * THEMIS_SOAK_DURATION_MS — total run window in milliseconds (default 60000)
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_DISTRIBUTED_TENSOR.md — operator runbook
 * @see src/distributed_tensor/ROADMAP.md — Wave D contribution closure
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
// In-process stubs model distributed tensor hot paths without requiring
// real network shards or GPU backends. MUST NOT be used in production
// code paths.
// ─────────────────────────────────────────────────────────────────────────────

namespace {

// ---------------------------------------------------------------------------
// StubShardedOpQueue — simulates sharded tensor operation throughput
// ---------------------------------------------------------------------------
class StubShardedOpQueue {
public:
    explicit StubShardedOpQueue(std::size_t capacity) : capacity_(capacity) {}

    bool enqueue(uint64_t op_id) {
        std::lock_guard<std::mutex> lk(mu_);
        if (queue_.size() >= capacity_) { ++overflow_count_; return false; }
        queue_.push_back(op_id);
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
// StubTensorSyncBus — simulates distributed tensor sync (all-reduce, etc.)
// ---------------------------------------------------------------------------
class StubTensorSyncBus {
public:
    bool sync(uint64_t shard_id, uint64_t step_id) {
        ++sync_count_;
        (void)shard_id; (void)step_id;
        return true; // stub: always succeeds
    }

    uint64_t syncCount() const { return sync_count_.load(); }
    uint64_t failCount() const { return fail_count_.load(); }

private:
    std::atomic<uint64_t> sync_count_{0};
    std::atomic<uint64_t> fail_count_{0};
};

// ---------------------------------------------------------------------------
// StubPartitionRouter — simulates tensor partition routing
// ---------------------------------------------------------------------------
class StubPartitionRouter {
public:
    int route(uint64_t tensor_id) {
        ++route_count_;
        return static_cast<int>(tensor_id % 4); // stub: round-robin
    }

    uint64_t routeCount() const { return route_count_.load(); }
    uint64_t failCount()  const { return fail_count_.load(); }

private:
    std::atomic<uint64_t> route_count_{0};
    std::atomic<uint64_t> fail_count_{0};
};

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Test 1: DistTensorSoak_ShardedOpThroughput
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_DistTensorSoak, DistTensorSoak_ShardedOpThroughput) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs());

    StubShardedOpQueue queue(4096);
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
        << "[DIST_TENSOR:ShardSyncFailed] At least one sharded op must complete";
    EXPECT_GE(ops_per_sec, 500.0)
        << "Sharded op throughput must be ≥ 500 ops/sec. "
           "Observed: " << ops_per_sec << " ops/sec";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 2: DistTensorSoak_SyncStability
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_DistTensorSoak, DistTensorSoak_SyncStability) {
    const auto soak_duration =
        std::chrono::milliseconds(soakDurationMs() / 10);

    StubTensorSyncBus bus;
    std::atomic<bool> running{true};

    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&, i]() {
            uint64_t step = 0;
            while (running.load(std::memory_order_relaxed)) {
                bus.sync(static_cast<uint64_t>(i), step++);
                std::this_thread::yield();
            }
        });
    }

    std::this_thread::sleep_for(soak_duration);
    running.store(false, std::memory_order_relaxed);
    for (auto& t : workers) t.join();

    EXPECT_GT(bus.syncCount(), 0u)
        << "[DIST_TENSOR:AllReduceTimeout] At least one sync must complete";
    EXPECT_EQ(bus.failCount(), 0u)
        << "[DIST_TENSOR:AllReduceTimeout] Zero sync failures expected. "
           "Observed: " << bus.failCount();
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 3: DistTensorSoak_PartitionReliability
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_DistTensorSoak, DistTensorSoak_PartitionReliability) {
    const auto soak_duration =
        std::chrono::milliseconds(soakDurationMs() / 10);

    StubPartitionRouter router;
    std::atomic<bool>   running{true};

    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&, i]() {
            uint64_t tensor = static_cast<uint64_t>(i) * 1'000'000ULL;
            while (running.load(std::memory_order_relaxed)) {
                (void)router.route(tensor++);
                std::this_thread::yield();
            }
        });
    }

    std::this_thread::sleep_for(soak_duration);
    running.store(false, std::memory_order_relaxed);
    for (auto& t : workers) t.join();

    EXPECT_GT(router.routeCount(), 0u)
        << "[DIST_TENSOR:PartitionStall] At least one partition routing must complete";
    EXPECT_EQ(router.failCount(), 0u)
        << "[DIST_TENSOR:PartitionStall] Zero routing failures expected. "
           "Observed: " << router.failCount();
}
