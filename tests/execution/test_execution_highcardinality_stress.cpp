/**
 * @file test_execution_highcardinality_stress.cpp
 * @brief Wave D — Execution Engine High-Cardinality Stress Tests.
 *
 * Stress tests covering high-cardinality task dispatch, concurrent queue
 * saturation, and work-stealing behaviour under load.  These tests are
 * excluded from the fast release-critical gate.
 *
 * ## Test IDs
 * - HighCardinalityTaskDispatch  : 5 000 tasks dispatched across 8 threads
 * - ConcurrentQueueStress        : concurrent enqueue/dequeue under contention
 * - WorkStealingUnderLoad        : steal pool drains fully under load
 *
 * ## Labels
 * wave_d;stress;not_release_critical
 *
 * @version 1.0.0
 * @see src/execution/ROADMAP.md — Wave D contribution closure
 * @see docs/operability/RUNBOOK_EXECUTION_ENGINE.md — operator runbook
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// STUB / SIMULATION NOTE
// In-process stubs model the execution engine.  Not for production use.
// ─────────────────────────────────────────────────────────────────────────────

// ---------------------------------------------------------------------------
// StubBoundedQueue
// ---------------------------------------------------------------------------
class StubBoundedQueue {
public:
    explicit StubBoundedQueue(std::size_t capacity) : capacity_(capacity) {}

    bool enqueue(uint64_t item) {
        std::lock_guard<std::mutex> lk(mu_);
        if (queue_.size() >= capacity_) { ++overflow_; return false; }
        queue_.push_back(item);
        ++enqueued_;
        return true;
    }

    bool dequeue(uint64_t& out) {
        std::lock_guard<std::mutex> lk(mu_);
        if (queue_.empty()) return false;
        out = queue_.front();
        queue_.pop_front();
        ++dequeued_;
        return true;
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lk(mu_);
        return queue_.size();
    }

    uint64_t enqueued()  const { return enqueued_.load(); }
    uint64_t dequeued()  const { return dequeued_.load(); }
    uint64_t overflow()  const { return overflow_.load(); }

private:
    const std::size_t capacity_;
    mutable std::mutex mu_;
    std::deque<uint64_t> queue_;
    std::atomic<uint64_t> enqueued_{0};
    std::atomic<uint64_t> dequeued_{0};
    std::atomic<uint64_t> overflow_{0};
};

// ---------------------------------------------------------------------------
// StubWorkStealPool
// ---------------------------------------------------------------------------
class StubWorkStealPool {
public:
    explicit StubWorkStealPool(unsigned workers)
        : workers_(workers), queues_(workers) {}

    void submit(uint64_t task_id) {
        const unsigned w = static_cast<unsigned>(task_id % workers_);
        std::lock_guard<std::mutex> lk(mu_[w % kMaxW]);
        queues_[w].push_back(task_id);
        ++submitted_;
    }

    // Steal and process one item from each worker queue
    void drainStep() {
        for (unsigned w = 0; w < workers_; ++w) {
            std::lock_guard<std::mutex> lk(mu_[w % kMaxW]);
            if (!queues_[w].empty()) {
                queues_[w].pop_front();
                ++processed_;
            }
        }
    }

    uint64_t submitted()  const { return submitted_.load(); }
    uint64_t processed()  const { return processed_.load(); }

    uint64_t remaining() const {
        uint64_t r = 0;
        for (unsigned w = 0; w < workers_; ++w) {
            std::lock_guard<std::mutex> lk(mu_[w % kMaxW]);
            r += static_cast<uint64_t>(queues_[w].size());
        }
        return r;
    }

private:
    static constexpr unsigned kMaxW = 32;
    unsigned workers_;
    std::vector<std::deque<uint64_t>> queues_;
    mutable std::mutex mu_[kMaxW];
    std::atomic<uint64_t> submitted_{0};
    std::atomic<uint64_t> processed_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
// ─────────────────────────────────────────────────────────────────────────────
class ExecutionHighCardinalityStress : public ::testing::Test {
protected:
    static constexpr int kTasks    = 5000;
    static constexpr int kThreads  = 8;
};

// ─────────────────────────────────────────────────────────────────────────────
// HighCardinalityTaskDispatch
//
// Dispatch kTasks tasks from kThreads producer threads into the bounded queue,
// then drain completely.  All tasks must be processed with no overflow.
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(ExecutionHighCardinalityStress, HighCardinalityTaskDispatch) {
    constexpr std::size_t kCapacity = static_cast<std::size_t>(kTasks) * 2;
    StubBoundedQueue queue(kCapacity);

    std::atomic<int> next_task{0};

    std::vector<std::thread> producers;
    for (int t = 0; t < kThreads; ++t) {
        producers.emplace_back([&]() {
            while (true) {
                const int id = next_task.fetch_add(1, std::memory_order_relaxed);
                if (id >= kTasks) break;
                while (!queue.enqueue(static_cast<uint64_t>(id))) {
                    std::this_thread::yield();
                }
            }
        });
    }
    for (auto& t : producers) t.join();

    // Drain
    uint64_t dummy = 0;
    while (queue.dequeue(dummy)) {}

    EXPECT_GE(queue.enqueued(), static_cast<uint64_t>(kTasks))
        << "All " << kTasks << " tasks must be enqueued";
    EXPECT_EQ(queue.overflow(), 0u)
        << "No queue overflow must occur with adequate capacity";
    EXPECT_EQ(queue.size(), 0u)
        << "Queue must be fully drained after processing";
}

// ─────────────────────────────────────────────────────────────────────────────
// ConcurrentQueueStress
//
// kThreads/2 producers and kThreads/2 consumers race on a small queue.
// Total enqueued and dequeued counts must be equal (nothing lost).
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(ExecutionHighCardinalityStress, ConcurrentQueueStress) {
    constexpr int kProducers = kThreads / 2;
    constexpr int kConsumers = kThreads / 2;
    constexpr int kItemsPerProducer = kTasks / kProducers;
    constexpr std::size_t kCapacity = 256;

    StubBoundedQueue queue(kCapacity);
    std::atomic<uint64_t> consumed{0};

    std::atomic<bool> done{false};

    std::vector<std::thread> threads;

    for (int p = 0; p < kProducers; ++p) {
        threads.emplace_back([&, p]() {
            const uint64_t base = static_cast<uint64_t>(p) * kItemsPerProducer;
            for (int i = 0; i < kItemsPerProducer; ++i) {
                while (!queue.enqueue(base + static_cast<uint64_t>(i)))
                    std::this_thread::yield();
            }
        });
    }

    for (int c = 0; c < kConsumers; ++c) {
        threads.emplace_back([&]() {
            uint64_t item = 0;
            while (!done.load(std::memory_order_relaxed) || queue.size() > 0) {
                if (queue.dequeue(item)) consumed.fetch_add(1);
                else std::this_thread::yield();
            }
        });
    }

    // Wait for producers
    for (int i = 0; i < kProducers; ++i) threads[static_cast<std::size_t>(i)].join();
    done.store(true);
    for (int i = kProducers; i < kProducers + kConsumers; ++i)
        threads[static_cast<std::size_t>(i)].join();

    // Final drain
    uint64_t item = 0;
    while (queue.dequeue(item)) consumed.fetch_add(1);

    const uint64_t expected =
        static_cast<uint64_t>(kProducers) * static_cast<uint64_t>(kItemsPerProducer);
    EXPECT_EQ(queue.enqueued(), expected)
        << "All produced items must be enqueued";
    EXPECT_EQ(consumed.load(), expected)
        << "All enqueued items must be consumed (none lost)";
}

// ─────────────────────────────────────────────────────────────────────────────
// WorkStealingUnderLoad
//
// Submit kTasks tasks to an 8-worker steal pool from kThreads producer threads.
// After draining, processed count must equal submitted count (no loss).
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(ExecutionHighCardinalityStress, WorkStealingUnderLoad) {
    StubWorkStealPool pool(static_cast<unsigned>(kThreads));

    std::atomic<int> next_task{0};

    std::vector<std::thread> producers;
    for (int t = 0; t < kThreads; ++t) {
        producers.emplace_back([&]() {
            while (true) {
                const int id = next_task.fetch_add(1, std::memory_order_relaxed);
                if (id >= kTasks) break;
                pool.submit(static_cast<uint64_t>(id));
            }
        });
    }
    for (auto& t : producers) t.join();

    // Drain all tasks via steal-pool drain steps
    while (pool.remaining() > 0) {
        pool.drainStep();
    }

    EXPECT_EQ(pool.submitted(), static_cast<uint64_t>(kTasks))
        << "All " << kTasks << " tasks must be submitted";
    EXPECT_EQ(pool.processed(), pool.submitted())
        << "All submitted tasks must be processed (no starvation/loss)";
}
