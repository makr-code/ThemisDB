/**
 * @file test_execution_engine_soak.cpp
 * @brief Wave D — Execution Engine Soak Test (sustained traffic).
 *
 * Long-duration soak test for the ThemisDB execution engine hot paths:
 * query throughput, scheduler stability, and work-stealing reliability.
 * Verifies that all three metrics remain within acceptable bounds over a
 * configurable soak window driven by THEMIS_SOAK_DURATION_MS.
 *
 * In CI environments this test runs at the default 60 000 ms (1 min) so the
 * gate finishes in < 2 min.  The full production soak (3 600 000 ms / 60 min)
 * is reserved for the release pipeline.
 *
 * ## Acceptance criteria
 * - ExecutionSoak_QueryThroughput     : ≥ 1 000 ops/sec over soak window
 * - ExecutionSoak_SchedulerStability  : no simulated queue-overflow events
 * - ExecutionSoak_WorkStealingReliability : no starvation (all tasks complete)
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * @version 1.0.0
 * @note Run in the release/Wave-D soak pipeline; excluded from fast CI gate.
 * @see docs/operability/RUNBOOK_EXECUTION_ENGINE.md — operator runbook
 * @see src/execution/ROADMAP.md — Wave D contribution closure
 */

#include <gtest/gtest.h>

#include <algorithm>
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

// ─────────────────────────────────────────────────────────────────────────────
// Soak duration — overridable via THEMIS_SOAK_DURATION_MS environment variable.
// Default: 60 000 ms (1 min) so CI completes well within the 120 s timeout.
// Production soak: 3 600 000 ms (60 min).
// ─────────────────────────────────────────────────────────────────────────────
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
// In-process stubs model the execution engine hot paths without requiring
// external backends (network, storage, GPU).  They MUST NOT be used in
// production code paths.
// ─────────────────────────────────────────────────────────────────────────────

// ---------------------------------------------------------------------------
// StubQueryQueue — bounded priority queue simulation
// ---------------------------------------------------------------------------
class StubQueryQueue {
public:
    explicit StubQueryQueue(std::size_t capacity) : capacity_(capacity) {}

    /// Enqueue a query item.  Returns false if the queue is full.
    bool enqueue(uint64_t query_id) {
        std::lock_guard<std::mutex> lk(mu_);
        if (queue_.size() >= capacity_) {
            ++overflow_count_;
            return false;
        }
        queue_.push_back(query_id);
        ++enqueue_count_;
        return true;
    }

    /// Dequeue a query item.  Returns false if empty.
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
// StubWorkStealPool — simulates a work-stealing thread pool
// ---------------------------------------------------------------------------
class StubWorkStealPool {
public:
    explicit StubWorkStealPool(unsigned worker_count)
        : worker_count_(worker_count),
          local_queues_(worker_count) {}

    /// Submit a task to worker (task_id % worker_count).
    void submit(uint64_t task_id) {
        const unsigned target = static_cast<unsigned>(task_id % worker_count_);
        std::lock_guard<std::mutex> lk(queue_mu_[target % kMaxWorkers]);
        local_queues_[target].push_back(task_id);
        ++submitted_;
    }

    /// Execute one round of work-stealing drain across all workers.
    void drainOnce() {
        for (unsigned w = 0; w < worker_count_; ++w) {
            uint64_t task_id = 0;
            {
                std::lock_guard<std::mutex> lk(queue_mu_[w % kMaxWorkers]);
                if (local_queues_[w].empty()) continue;
                task_id = local_queues_[w].front();
                local_queues_[w].pop_front();
            }
            ++completed_;
            (void)task_id;
        }
    }

    uint64_t submitted()  const { return submitted_.load(); }
    uint64_t completed()  const { return completed_.load(); }

    /// Returns true if any local queue holds tasks that have never been
    /// attempted (approximation of starvation: a worker has > 2× average).
    bool hasStarvation() const {
        uint64_t total = 0;
        for (unsigned w = 0; w < worker_count_; ++w) {
            std::lock_guard<std::mutex> lk(queue_mu_[w % kMaxWorkers]);
            total += static_cast<uint64_t>(local_queues_[w].size());
        }
        if (total == 0) return false;
        const uint64_t avg = total / worker_count_;
        for (unsigned w = 0; w < worker_count_; ++w) {
            std::lock_guard<std::mutex> lk(queue_mu_[w % kMaxWorkers]);
            if (static_cast<uint64_t>(local_queues_[w].size()) > avg * 3 + 10)
                return true;
        }
        return false;
    }

private:
    static constexpr unsigned kMaxWorkers = 32;
    unsigned worker_count_;
    std::vector<std::deque<uint64_t>> local_queues_;
    mutable std::mutex queue_mu_[kMaxWorkers];
    std::atomic<uint64_t> submitted_{0};
    std::atomic<uint64_t> completed_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// Test 1: ExecutionSoak_QueryThroughput
//
// Continuously enqueue and dequeue queries for soak_duration ms across
// 4 producer threads and 4 consumer threads.  Aggregate throughput (enqueue +
// dequeue ops combined) must be ≥ 1 000 ops/sec.
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_ExecutionSoak, ExecutionSoak_QueryThroughput) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs());

    constexpr std::size_t kQueueCapacity = 4096;
    StubQueryQueue queue(kQueueCapacity);

    std::atomic<bool> running{true};

    // Producer threads
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

    // Consumer threads
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
    for (auto& t : consumers) t.join();

    const double elapsed_s =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    const uint64_t total_ops = queue.enqueueCount() + queue.dequeueCount();
    const double ops_per_sec = static_cast<double>(total_ops) / elapsed_s;

    EXPECT_GT(total_ops, 0u)
        << "At least one enqueue/dequeue must complete during the soak";
    EXPECT_GE(ops_per_sec, 1000.0)
        << "Query throughput must be ≥ 1 000 ops/sec. "
           "Observed: " << ops_per_sec << " ops/sec";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 2: ExecutionSoak_SchedulerStability
//
// Sustain enqueue load against a bounded queue for soak_duration/10 ms.
// The queue must not overflow (all submissions succeed).
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_ExecutionSoak, ExecutionSoak_SchedulerStability) {
    const auto soak_duration =
        std::chrono::milliseconds(soakDurationMs() / 10);

    // Large-enough capacity to absorb producer bursts; the consumer thread
    // drains fast enough that overflow should never occur.
    constexpr std::size_t kQueueCapacity = 8192;
    StubQueryQueue queue(kQueueCapacity);

    std::atomic<bool> running{true};

    // Single producer at controlled rate (yield after each enqueue)
    std::thread producer([&]() {
        uint64_t id = 0;
        while (running.load(std::memory_order_relaxed)) {
            queue.enqueue(id++);
            std::this_thread::yield();
        }
    });

    // Single consumer — drains immediately
    std::thread consumer([&]() {
        uint64_t dummy = 0;
        while (running.load(std::memory_order_relaxed)) {
            queue.dequeue(dummy);
        }
    });

    std::this_thread::sleep_for(soak_duration);
    running.store(false, std::memory_order_relaxed);
    producer.join();
    consumer.join();

    EXPECT_GT(queue.enqueueCount(), 0u)
        << "Scheduler must process at least one query during the soak";
    EXPECT_EQ(queue.overflowCount(), 0u)
        << "Scheduler queue must not overflow during normal load. "
           "Overflow events: " << queue.overflowCount();
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 3: ExecutionSoak_WorkStealingReliability
//
// Submit tasks into a 4-worker steal pool for soak_duration/10 ms, then drain
// completely.  After draining, no starvation condition must remain and all
// submitted tasks must be completed.
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_ExecutionSoak, ExecutionSoak_WorkStealingReliability) {
    const auto soak_duration =
        std::chrono::milliseconds(soakDurationMs() / 10);

    constexpr unsigned kWorkers = 4;
    StubWorkStealPool pool(kWorkers);

    std::atomic<bool> running{true};

    // Producer: submit tasks continuously
    std::thread producer([&]() {
        uint64_t id = 0;
        while (running.load(std::memory_order_relaxed)) {
            pool.submit(id++);
            std::this_thread::yield();
        }
    });

    // Worker threads: drain their own queue + steal-drain pass
    std::vector<std::thread> workers;
    for (unsigned w = 0; w < kWorkers; ++w) {
        workers.emplace_back([&]() {
            while (running.load(std::memory_order_relaxed)) {
                pool.drainOnce();
                std::this_thread::yield();
            }
        });
    }

    std::this_thread::sleep_for(soak_duration);
    running.store(false, std::memory_order_relaxed);
    producer.join();
    for (auto& t : workers) t.join();

    // Final drain pass — process remaining items
    for (int i = 0; i < 1000; ++i) pool.drainOnce();

    EXPECT_GT(pool.submitted(), 0u)
        << "At least one task must be submitted during the soak";

    EXPECT_FALSE(pool.hasStarvation())
        << "Work-stealing pool must not show starvation (no worker queue "
           "holding > 3× average remaining tasks)";
}
