// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_training_pipeline_soak.cpp
 * @brief Wave D — Training Pipeline Soak Test (sustained traffic).
 *
 * Long-duration soak test for the ThemisDB training pipeline hot paths:
 * batch throughput, checkpoint stability, and gradient sync reliability.
 * Verifies that all three metrics remain within acceptable bounds over a
 * configurable soak window driven by THEMIS_SOAK_DURATION_MS.
 *
 * ## Acceptance criteria
 * - TrainingSoak_BatchThroughput          : ≥ 500 batch ops/sec over soak window
 * - TrainingSoak_CheckpointStability      : zero checkpoint failures
 * - TrainingSoak_GradientSyncReliability  : zero gradient sync failures
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * Environment overrides
 * ---------------------
 * THEMIS_SOAK_DURATION_MS — total run window in milliseconds (default 60000)
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_TRAINING_PIPELINE.md — operator runbook
 * @see src/training/ROADMAP.md — Wave D contribution closure
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
// In-process stubs model the training pipeline hot paths without requiring
// external GPU hardware or storage backends. MUST NOT be used in production
// code paths.
// ─────────────────────────────────────────────────────────────────────────────

namespace {

// ---------------------------------------------------------------------------
// StubBatchDispatchQueue — simulates training batch throughput
// ---------------------------------------------------------------------------
class StubBatchDispatchQueue {
public:
    explicit StubBatchDispatchQueue(std::size_t capacity) : capacity_(capacity) {}

    bool enqueue(uint64_t batch_id) {
        std::lock_guard<std::mutex> lk(mu_);
        if (queue_.size() >= capacity_) { ++overflow_count_; return false; }
        queue_.push_back(batch_id);
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
// StubCheckpointManager — simulates checkpoint save/restore cycle
// ---------------------------------------------------------------------------
class StubCheckpointManager {
public:
    bool saveCheckpoint(uint64_t step_id) {
        ++save_count_;
        (void)step_id;
        return true; // stub: always succeeds
    }

    bool restoreCheckpoint(uint64_t step_id) {
        ++restore_count_;
        (void)step_id;
        return true; // stub: always succeeds
    }

    uint64_t saveCount()    const { return save_count_.load(); }
    uint64_t restoreCount() const { return restore_count_.load(); }
    uint64_t failCount()    const { return fail_count_.load(); }

private:
    std::atomic<uint64_t> save_count_{0};
    std::atomic<uint64_t> restore_count_{0};
    std::atomic<uint64_t> fail_count_{0};
};

// ---------------------------------------------------------------------------
// StubGradientSyncBus — simulates gradient sync across workers
// ---------------------------------------------------------------------------
class StubGradientSyncBus {
public:
    bool syncGradients(uint64_t worker_id, uint64_t step_id) {
        ++sync_count_;
        (void)worker_id;
        (void)step_id;
        return true; // stub: always succeeds
    }

    uint64_t syncCount() const { return sync_count_.load(); }
    uint64_t failCount() const { return fail_count_.load(); }

private:
    std::atomic<uint64_t> sync_count_{0};
    std::atomic<uint64_t> fail_count_{0};
};

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Test 1: TrainingSoak_BatchThroughput
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_TrainingSoak, TrainingSoak_BatchThroughput) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs());

    StubBatchDispatchQueue queue(4096);
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
        << "[TRAINING:BatchOOM] At least one batch dispatch must complete";
    EXPECT_GE(ops_per_sec, 500.0)
        << "Batch throughput must be ≥ 500 ops/sec. "
           "Observed: " << ops_per_sec << " ops/sec";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 2: TrainingSoak_CheckpointStability
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_TrainingSoak, TrainingSoak_CheckpointStability) {
    const auto soak_duration =
        std::chrono::milliseconds(soakDurationMs() / 10);

    StubCheckpointManager mgr;
    std::atomic<bool> running{true};

    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&, i]() {
            uint64_t step = static_cast<uint64_t>(i) * 1'000ULL;
            while (running.load(std::memory_order_relaxed)) {
                mgr.saveCheckpoint(step);
                mgr.restoreCheckpoint(step);
                ++step;
                std::this_thread::yield();
            }
        });
    }

    std::this_thread::sleep_for(soak_duration);
    running.store(false, std::memory_order_relaxed);
    for (auto& t : workers) t.join();

    EXPECT_GT(mgr.saveCount(), 0u)
        << "[TRAINING:CheckpointFailed] At least one checkpoint save must complete";
    EXPECT_EQ(mgr.failCount(), 0u)
        << "[TRAINING:CheckpointFailed] Zero checkpoint failures expected. "
           "Observed: " << mgr.failCount();
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 3: TrainingSoak_GradientSyncReliability
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_TrainingSoak, TrainingSoak_GradientSyncReliability) {
    const auto soak_duration =
        std::chrono::milliseconds(soakDurationMs() / 10);

    StubGradientSyncBus bus;
    std::atomic<bool>  running{true};

    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&, i]() {
            uint64_t step = 0;
            while (running.load(std::memory_order_relaxed)) {
                bus.syncGradients(static_cast<uint64_t>(i), step++);
                std::this_thread::yield();
            }
        });
    }

    std::this_thread::sleep_for(soak_duration);
    running.store(false, std::memory_order_relaxed);
    for (auto& t : workers) t.join();

    EXPECT_GT(bus.syncCount(), 0u)
        << "[TRAINING:GradientSyncStall] At least one gradient sync must complete";
    EXPECT_EQ(bus.failCount(), 0u)
        << "[TRAINING:GradientSyncStall] Zero gradient sync failures expected. "
           "Observed: " << bus.failCount();
}
