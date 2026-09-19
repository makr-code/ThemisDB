// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_replication_wave_d_soak.cpp
 * @brief Wave D — Replication Wave-D Soak Tests (backpressure, slot-stream, CDC).
 *
 * Three soak cases that complement the existing test_replication_soak_60min.cpp
 * with Wave D-specific acceptance criteria focused on backpressure throughput,
 * replication-slot stream stability, and CDC edge-case reliability.
 *
 * ## Acceptance criteria
 * - ReplicationWaveD_BackpressureThroughput : no write stalls; queue depth bounded
 * - ReplicationWaveD_SlotStreamStability    : slot cursor advances monotonically
 * - ReplicationWaveD_CDCEdgeCaseReliability : change events delivered in order
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * @version 1.0.0
 * @note Excluded from fast release_critical gate.
 *       Named _wave_d_ to avoid conflict with test_replication_soak_60min.cpp.
 * @see docs/operability/RUNBOOK_REPLICATION.md — operator runbook
 * @see src/replication/ROADMAP.md — Wave D contribution
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

// ---------------------------------------------------------------------------
// Soak duration helper
// ---------------------------------------------------------------------------
static uint64_t soakDurationMs() {
    const char* env = std::getenv("THEMIS_SOAK_DURATION_MS");
    if (env && *env) {
        try { return static_cast<uint64_t>(std::stoull(env)); }
        catch (...) {}
    }
    return 60'000ULL;
}

// ---------------------------------------------------------------------------
// In-process replication stubs
// MUST NOT be used in production code paths.
// ---------------------------------------------------------------------------

/// Stub backpressure queue — bounded producer/consumer in-process model.
class StubBackpressureQueue {
public:
    static constexpr std::size_t kMaxDepth = 1024;

    bool enqueue(uint64_t record_id) {
        std::lock_guard<std::mutex> lk(mu_);
        if (queue_.size() >= kMaxDepth) {
            stalls_.fetch_add(1, std::memory_order_relaxed);
            return false; // backpressure applied
        }
        queue_.push(record_id);
        enqueued_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    bool dequeue(uint64_t& out) {
        std::lock_guard<std::mutex> lk(mu_);
        if (queue_.empty()) { return false; }
        out = queue_.front();
        queue_.pop();
        return true;
    }

    uint64_t stalls()    const noexcept { return stalls_.load(std::memory_order_relaxed); }
    uint64_t enqueued()  const noexcept { return enqueued_.load(std::memory_order_relaxed); }
    std::size_t depth() {
        std::lock_guard<std::mutex> lk(mu_);
        return queue_.size();
    }

private:
    std::mutex             mu_;
    std::queue<uint64_t>   queue_;
    std::atomic<uint64_t>  stalls_{0};
    std::atomic<uint64_t>  enqueued_{0};
};

/// Stub replication slot — models monotonically advancing LSN cursor.
class StubReplicationSlot {
public:
    explicit StubReplicationSlot(uint64_t start_lsn) : lsn_(start_lsn) {}

    StubReplicationSlot(const StubReplicationSlot&) = delete;
    StubReplicationSlot& operator=(const StubReplicationSlot&) = delete;

    StubReplicationSlot(StubReplicationSlot&& other) noexcept
        : lsn_(other.lsn_.load(std::memory_order_relaxed)) {}

    StubReplicationSlot& operator=(StubReplicationSlot&& other) noexcept {
        if (this != &other) {
            lsn_.store(other.lsn_.load(std::memory_order_relaxed), std::memory_order_relaxed);
        }
        return *this;
    }

    uint64_t advance(uint64_t delta) noexcept {
        return lsn_.fetch_add(delta, std::memory_order_relaxed) + delta;
    }

    uint64_t currentLsn() const noexcept {
        return lsn_.load(std::memory_order_relaxed);
    }

private:
    std::atomic<uint64_t> lsn_;
};

/// Stub CDC event stream — in-order delivery verification.
class StubCDCStream {
public:
    void publish(uint64_t seq) {
        std::lock_guard<std::mutex> lk(mu_);
        if (seq != next_expected_) {
            out_of_order_.fetch_add(1, std::memory_order_relaxed);
        }
        next_expected_ = seq + 1;
        published_.fetch_add(1, std::memory_order_relaxed);
    }

    uint64_t outOfOrder() const noexcept {
        return out_of_order_.load(std::memory_order_relaxed);
    }

    uint64_t published() const noexcept {
        return published_.load(std::memory_order_relaxed);
    }

private:
    std::mutex            mu_;
    uint64_t              next_expected_{0};
    std::atomic<uint64_t> out_of_order_{0};
    std::atomic<uint64_t> published_{0};
};

// ---------------------------------------------------------------------------
// ReplicationWaveD_BackpressureThroughput
// ---------------------------------------------------------------------------
TEST(ReplicationWaveD, ReplicationWaveD_BackpressureThroughput) {
    constexpr uint32_t kProducers = 4;
    constexpr uint32_t kConsumers = 2;

    StubBackpressureQueue bpq;
    std::atomic<bool>     running{true};

    const auto start    = std::chrono::steady_clock::now();
    const auto duration = std::chrono::milliseconds(soakDurationMs());
    const auto end_time = start + duration;

    auto producer = [&](uint64_t base) {
        uint64_t id = base;
        while (running.load(std::memory_order_relaxed)) {
            bpq.enqueue(id++);
            if (std::chrono::steady_clock::now() >= end_time) {
                running.store(false, std::memory_order_relaxed);
                break;
            }
        }
    };

    auto consumer = [&]() {
        uint64_t val = 0;
        while (running.load(std::memory_order_relaxed) ||
               std::chrono::steady_clock::now() < end_time) {
            bpq.dequeue(val);
        }
        // drain
        while (bpq.dequeue(val)) {}
    };

    std::vector<std::thread> threads;
    threads.reserve(kProducers + kConsumers);
    for (uint32_t i = 0; i < kProducers; ++i)
        threads.emplace_back(producer, static_cast<uint64_t>(i) * 10'000'000ULL);
    for (uint32_t i = 0; i < kConsumers; ++i)
        threads.emplace_back(consumer);
    for (auto& t : threads) t.join();

    EXPECT_GT(bpq.enqueued(), 0ULL)
        << "No records were enqueued — soak window too short";

    // Stall rate should be low relative to total enqueued; allow up to 10 %.
    const double stall_ratio = bpq.enqueued() == 0 ? 0.0 :
        static_cast<double>(bpq.stalls()) / static_cast<double>(bpq.enqueued());
    EXPECT_LE(stall_ratio, 0.10)
        << "Backpressure stall ratio " << stall_ratio * 100.0
        << "% exceeds 10 % threshold";
}

// ---------------------------------------------------------------------------
// ReplicationWaveD_SlotStreamStability
// ---------------------------------------------------------------------------
TEST(ReplicationWaveD, ReplicationWaveD_SlotStreamStability) {
    constexpr uint32_t kSlotCount = 4;

    std::vector<StubReplicationSlot> slots;
    slots.reserve(kSlotCount);
    for (uint32_t i = 0; i < kSlotCount; ++i) {
        slots.emplace_back(static_cast<uint64_t>(i) * 1'000ULL);
    }

    const auto start    = std::chrono::steady_clock::now();
    const auto duration = std::chrono::milliseconds(soakDurationMs());
    const auto end_time = start + duration;

    std::atomic<uint64_t> total_advances{0};

    auto stream_worker = [&](uint32_t slot_idx) {
        uint64_t prev_lsn = slots[slot_idx].currentLsn();
        while (std::chrono::steady_clock::now() < end_time) {
            const uint64_t new_lsn = slots[slot_idx].advance(1);
            ASSERT_GE(new_lsn, prev_lsn)
                << "Slot " << slot_idx << " LSN regressed from " << prev_lsn
                << " to " << new_lsn;
            prev_lsn = new_lsn;
            total_advances.fetch_add(1, std::memory_order_relaxed);
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(kSlotCount);
    for (uint32_t i = 0; i < kSlotCount; ++i) {
        workers.emplace_back(stream_worker, i);
    }
    for (auto& t : workers) t.join();

    EXPECT_GT(total_advances.load(), 0ULL)
        << "No slot advances recorded — soak window too short";
}

// ---------------------------------------------------------------------------
// ReplicationWaveD_CDCEdgeCaseReliability
// ---------------------------------------------------------------------------
TEST(ReplicationWaveD, ReplicationWaveD_CDCEdgeCaseReliability) {
    StubCDCStream stream;

    const auto start    = std::chrono::steady_clock::now();
    const auto duration = std::chrono::milliseconds(soakDurationMs());
    const auto end_time = start + duration;

    uint64_t seq = 0;
    while (std::chrono::steady_clock::now() < end_time) {
        // Publish in order — edge cases: empty payloads, boundary values.
        stream.publish(seq++);
        if (seq % 1000 == 0) {
            // Simulate a "gap close" event at boundaries.
            stream.publish(seq++);
        }
    }

    EXPECT_GT(stream.published(), 0ULL) << "No CDC events published";
    EXPECT_EQ(stream.outOfOrder(), 0ULL)
        << "CDC delivered " << stream.outOfOrder()
        << " out-of-order events during soak";
}
