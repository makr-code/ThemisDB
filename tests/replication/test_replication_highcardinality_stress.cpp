// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_replication_highcardinality_stress.cpp
 * @brief Wave D — Replication High-Cardinality Stress Tests.
 *
 * Three stress cases for the replication subsystem under high-cardinality
 * and concurrency conditions using in-process stubs and seed-42 determinism.
 *
 * ## Cases
 * - HighCardinalityReplicationSlot : 1 000 slots, 8-thread concurrent access
 * - ConcurrentCDCStress            : concurrent CDC publish under contention
 * - BackpressureEdgeCases          : backpressure queue edge cases
 *
 * ## Labels
 * wave_d;stress;not_release_critical
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_REPLICATION.md
 * @see src/replication/ROADMAP.md
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

static constexpr uint64_t kReplStressSeed = 42;

// ---------------------------------------------------------------------------
// Stubs
// ---------------------------------------------------------------------------

class StressReplicationSlot {
public:
    explicit StressReplicationSlot(uint64_t start_lsn)
        : lsn_(start_lsn), acked_(start_lsn) {}

    uint64_t advance() noexcept {
        return lsn_.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    void ack(uint64_t lsn) noexcept {
        uint64_t prev = acked_.load(std::memory_order_relaxed);
        while (prev < lsn && !acked_.compare_exchange_weak(
                prev, lsn, std::memory_order_relaxed)) {}
    }

    uint64_t lsn()   const noexcept { return lsn_.load(std::memory_order_relaxed); }
    uint64_t acked() const noexcept { return acked_.load(std::memory_order_relaxed); }

private:
    std::atomic<uint64_t> lsn_;
    std::atomic<uint64_t> acked_;
};

class StressCDCPublisher {
public:
    void publish(uint32_t slot_id, uint64_t seq) {
        std::lock_guard<std::mutex> lk(mu_);
        auto& last = last_seq_[slot_id];
        if (seq <= last) {
            out_of_order_.fetch_add(1, std::memory_order_relaxed);
        } else {
            last = seq;
        }
        total_.fetch_add(1, std::memory_order_relaxed);
    }

    uint64_t outOfOrder() const noexcept {
        return out_of_order_.load(std::memory_order_relaxed);
    }
    uint64_t total() const noexcept {
        return total_.load(std::memory_order_relaxed);
    }

private:
    std::mutex                               mu_;
    std::unordered_map<uint32_t, uint64_t>   last_seq_;
    std::atomic<uint64_t>                    out_of_order_{0};
    std::atomic<uint64_t>                    total_{0};
};

class StressBPQueue {
public:
    static constexpr std::size_t kMaxDepth = 512;

    bool push(uint64_t v) {
        std::lock_guard<std::mutex> lk(mu_);
        if (q_.size() >= kMaxDepth) { return false; }
        q_.push(v);
        return true;
    }
    bool pop(uint64_t& out) {
        std::lock_guard<std::mutex> lk(mu_);
        if (q_.empty()) { return false; }
        out = q_.front(); q_.pop();
        return true;
    }
    std::size_t size() {
        std::lock_guard<std::mutex> lk(mu_);
        return q_.size();
    }

private:
    std::mutex            mu_;
    std::queue<uint64_t>  q_;
};

// ---------------------------------------------------------------------------
// HighCardinalityReplicationSlot — 1 000 slots, 8 threads
// ---------------------------------------------------------------------------
TEST(ReplicationHighCardinalityStress, HighCardinalityReplicationSlot) {
    constexpr uint32_t kSlotCount   = 1'000;
    constexpr uint32_t kThreadCount = 8;
    constexpr uint64_t kOpsPerSlot  = 100;

    std::vector<StressReplicationSlot> slots;
    slots.reserve(kSlotCount);
    for (uint32_t i = 0; i < kSlotCount; ++i) {
        slots.emplace_back(static_cast<uint64_t>(i) * 10'000ULL);
    }

    std::atomic<uint64_t> total_advances{0};
    std::atomic<uint64_t> errors{0};

    const uint32_t slots_per_thread = kSlotCount / kThreadCount;

    auto worker = [&](uint32_t tid) {
        const uint32_t start = tid * slots_per_thread;
        const uint32_t end   = std::min(start + slots_per_thread, kSlotCount);
        for (uint32_t s = start; s < end; ++s) {
            uint64_t prev_lsn = slots[s].lsn();
            for (uint64_t op = 0; op < kOpsPerSlot; ++op) {
                const uint64_t new_lsn = slots[s].advance();
                if (new_lsn <= prev_lsn) {
                    errors.fetch_add(1, std::memory_order_relaxed);
                }
                slots[s].ack(new_lsn);
                prev_lsn = new_lsn;
            }
            total_advances.fetch_add(kOpsPerSlot, std::memory_order_relaxed);
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(kThreadCount);
    for (uint32_t i = 0; i < kThreadCount; ++i) {
        workers.emplace_back(worker, i);
    }
    for (auto& t : workers) t.join();

    EXPECT_EQ(total_advances.load(), static_cast<uint64_t>(kSlotCount) * kOpsPerSlot);
    EXPECT_EQ(errors.load(), 0ULL)
        << "HighCardinalityReplicationSlot: LSN regression detected";
}

// ---------------------------------------------------------------------------
// ConcurrentCDCStress
// ---------------------------------------------------------------------------
TEST(ReplicationHighCardinalityStress, ConcurrentCDCStress) {
    constexpr uint32_t kSlotCount   = 32;
    constexpr uint32_t kThreadCount = 8;
    constexpr uint64_t kEventsPerThread = 5'000;

    StressCDCPublisher publisher;
    std::atomic<bool>  go{false};

    auto worker = [&](uint32_t tid) {
        const uint32_t slot_id = tid % kSlotCount;
        uint64_t seq = static_cast<uint64_t>(tid) * kEventsPerThread;
        while (!go.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        for (uint64_t e = 0; e < kEventsPerThread; ++e) {
            publisher.publish(slot_id, seq++);
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(kThreadCount);
    for (uint32_t i = 0; i < kThreadCount; ++i) {
        workers.emplace_back(worker, i);
    }
    go.store(true, std::memory_order_release);
    for (auto& t : workers) t.join();

    EXPECT_EQ(publisher.total(),
              static_cast<uint64_t>(kThreadCount) * kEventsPerThread);
    // Each thread writes to its own slot ID — no cross-thread ordering issues.
    EXPECT_EQ(publisher.outOfOrder(), 0ULL)
        << "ConcurrentCDCStress: " << publisher.outOfOrder()
        << " out-of-order CDC events";
}

// ---------------------------------------------------------------------------
// BackpressureEdgeCases
// ---------------------------------------------------------------------------
TEST(ReplicationHighCardinalityStress, BackpressureEdgeCases) {
    // Test: fill queue to capacity, then drain, repeat.
    constexpr uint32_t kCycles = 20;

    StressBPQueue queue;

    for (uint32_t cycle = 0; cycle < kCycles; ++cycle) {
        // Fill to capacity.
        uint64_t enqueued = 0;
        for (uint64_t v = 0; v < StressBPQueue::kMaxDepth * 2; ++v) {
            if (queue.push(v)) { ++enqueued; }
        }
        EXPECT_EQ(enqueued, StressBPQueue::kMaxDepth)
            << "Cycle " << cycle << ": queue accepted more than kMaxDepth items";

        // Full drain.
        uint64_t val = 0;
        uint64_t drained = 0;
        while (queue.pop(val)) { ++drained; }
        EXPECT_EQ(drained, StressBPQueue::kMaxDepth)
            << "Cycle " << cycle << ": drained " << drained
            << " but expected " << StressBPQueue::kMaxDepth;

        EXPECT_EQ(queue.size(), 0UL)
            << "Cycle " << cycle << ": queue not empty after drain";
    }
}
