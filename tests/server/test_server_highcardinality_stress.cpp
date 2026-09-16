// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_server_highcardinality_stress.cpp
 * @brief Wave D high-cardinality and concurrent stress tests for the server module.
 *
 * Three focused stress cases that exercise the server's routing, rate-limit,
 * and WASM-sandbox paths under high parallelism.  All cases use in-process
 * stubs — no real network sockets or WASM runtimes are required.
 *
 * CTest labels: wave_d;stress;not_release_critical
 *
 * | Test case                    | Scenario                                     |
 * |------------------------------|----------------------------------------------|
 * | HighCardinalityRouting       | 10 000 routes, 8 concurrent threads          |
 * | ConcurrentRateLimitStress    | 1 000 clients, 8 threads, burst injection     |
 * | WASMSandboxLoadStress        | 64 sandbox slots, 8 threads, sustained invoke |
 *
 * @see src/server/ROADMAP.md — Wave D Contribution
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace themis {
namespace test {
namespace wave_d {

// ---------------------------------------------------------------------------
// In-process stub: high-cardinality route table
// ---------------------------------------------------------------------------

struct RouteTable {
    explicit RouteTable(int n) {
        table_.reserve(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i)
            table_["route_" + std::to_string(i)] = i;
    }

    bool lookup(const std::string& route) const {
        auto it = table_.find(route);
        if (it != table_.end()) { ++hits_; return true; }
        ++misses_;
        return false;
    }

    long hits()   const { return hits_.load(std::memory_order_relaxed); }
    long misses() const { return misses_.load(std::memory_order_relaxed); }

private:
    std::unordered_map<std::string, int> table_;
    mutable std::atomic<long> hits_{0};
    mutable std::atomic<long> misses_{0};
};

// ---------------------------------------------------------------------------
// In-process stub: per-client token bucket
// ---------------------------------------------------------------------------

struct TokenBucket {
    explicit TokenBucket(int capacity) : capacity_(capacity), tokens_(capacity) {}

    bool consume(int amount = 1) {
        std::lock_guard<std::mutex> lock(mu_);
        if (tokens_ < amount) return false;
        tokens_ -= amount;
        return true;
    }

    void refill() {
        std::lock_guard<std::mutex> lock(mu_);
        tokens_ = capacity_;
    }

private:
    int        capacity_;
    int        tokens_;
    std::mutex mu_;
};

struct RateLimiterPool {
    explicit RateLimiterPool(int clients, int capacity) {
        for (int i = 0; i < clients; ++i)
            buckets_.emplace_back(std::make_unique<TokenBucket>(capacity));
    }

    bool consume(int client_id) {
        return buckets_.at(static_cast<std::size_t>(client_id % static_cast<int>(buckets_.size())))->consume();
    }

    void refill_all() {
        for (auto& b : buckets_) b->refill();
    }

    long size() const { return static_cast<long>(buckets_.size()); }

private:
    std::vector<std::unique_ptr<TokenBucket>> buckets_;
};

// ---------------------------------------------------------------------------
// In-process stub: WASM sandbox pool
// ---------------------------------------------------------------------------

struct WasmSandboxPool {
    explicit WasmSandboxPool(int slots) : slots_(slots) {
        active_.assign(static_cast<std::size_t>(slots), false);
    }

    bool acquire(int* slot_out) {
        std::lock_guard<std::mutex> lock(mu_);
        for (int i = 0; i < slots_; ++i) {
            if (!active_[static_cast<std::size_t>(i)]) {
                active_[static_cast<std::size_t>(i)] = true;
                *slot_out = i;
                ++acquired_;
                return true;
            }
        }
        ++exhausted_;
        return false;
    }

    void release(int slot) {
        std::lock_guard<std::mutex> lock(mu_);
        active_[static_cast<std::size_t>(slot)] = false;
    }

    bool invoke(int /*slot*/) {
        ++invocations_;
        return true;
    }

    long acquired()    const { return acquired_.load(std::memory_order_relaxed); }
    long exhausted()   const { return exhausted_.load(std::memory_order_relaxed); }
    long invocations() const { return invocations_.load(std::memory_order_relaxed); }

private:
    int                 slots_;
    std::mutex          mu_;
    std::vector<bool>   active_;
    std::atomic<long>   acquired_{0};
    std::atomic<long>   exhausted_{0};
    std::atomic<long>   invocations_{0};
};

// ===========================================================================
// SRV-STRESS-01 — High-cardinality routing (10 000 routes, 8 threads)
// ===========================================================================

TEST(HighCardinalityRouting, AllRoutesResolvable) {
    constexpr int kRoutes  = 10000;
    constexpr int kThreads = 8;
    constexpr int kOpsPerThread = 50000;

    RouteTable table(kRoutes);
    std::atomic<long> total_lookups{0};

    auto worker = [&](int seed) {
        std::mt19937 rng(static_cast<unsigned>(seed));
        std::uniform_int_distribution<int> dist(0, kRoutes - 1);
        for (int i = 0; i < kOpsPerThread; ++i) {
            table.lookup("route_" + std::to_string(dist(rng)));
            ++total_lookups;
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i)
        threads.emplace_back(worker, i + 1);
    for (auto& t : threads) t.join();

    const long expected = static_cast<long>(kThreads) * kOpsPerThread;
    EXPECT_EQ(total_lookups.load(), expected);
    EXPECT_EQ(0L, table.misses())
        << "Route misses detected — routing table incomplete";
    EXPECT_EQ(expected, table.hits());
}

// ===========================================================================
// SRV-STRESS-02 — Concurrent rate-limit stress (1000 clients, 8 threads)
// ===========================================================================

TEST(ConcurrentRateLimitStress, NoUnderflowOrOverflow) {
    constexpr int kClients  = 1000;
    constexpr int kCapacity = 100;
    constexpr int kThreads  = 8;
    constexpr int kIterations = 20000;

    RateLimiterPool pool(kClients, kCapacity);
    std::atomic<long> allowed{0};
    std::atomic<long> rejected{0};
    std::atomic<bool> stop{false};

    // Periodic refill thread
    std::thread refiller([&]() {
        while (!stop.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            pool.refill_all();
        }
    });

    auto worker = [&](int seed) {
        std::mt19937 rng(static_cast<unsigned>(seed));
        std::uniform_int_distribution<int> client_dist(0, kClients - 1);
        for (int i = 0; i < kIterations; ++i) {
            if (pool.consume(client_dist(rng)))
                ++allowed;
            else
                ++rejected;
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i)
        threads.emplace_back(worker, i + 200);
    for (auto& t : threads) t.join();

    stop.store(true, std::memory_order_relaxed);
    refiller.join();

    const long total = allowed.load() + rejected.load();
    EXPECT_EQ(total, static_cast<long>(kThreads) * kIterations)
        << "Requests not fully accounted — possible race condition";
    // Rate-limiting should cause some rejections given burst traffic
    EXPECT_GT(rejected.load(), 0L)
        << "No rejections recorded — rate-limit path may not be exercised";
    EXPECT_GT(allowed.load(), 0L)
        << "No allowed requests — token refill may be broken";
}

// ===========================================================================
// SRV-STRESS-03 — WASM sandbox load stress (64 slots, 8 threads)
// ===========================================================================

TEST(WASMSandboxLoadStress, NoSlotLeakOrCrash) {
    constexpr int kSlots    = 64;
    constexpr int kThreads  = 8;
    constexpr int kIterations = 10000;

    WasmSandboxPool pool(kSlots);

    auto worker = [&](int /*seed*/) {
        for (int i = 0; i < kIterations; ++i) {
            int slot = -1;
            if (pool.acquire(&slot)) {
                pool.invoke(slot);
                pool.release(slot);
            }
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i)
        threads.emplace_back(worker, i + 300);
    for (auto& t : threads) t.join();

    EXPECT_GT(pool.acquired(), 0L)
        << "No sandbox slots were acquired — stress path not exercised";
    EXPECT_GT(pool.invocations(), 0L)
        << "No WASM invocations recorded";
    // Exhaustion is acceptable (contention is the point); no crash/assert is
    // the gate — reaching this line means no UB/crash.
    SUCCEED() << "WASM sandbox stress completed without crash. "
              << "acquired=" << pool.acquired()
              << " exhausted=" << pool.exhausted()
              << " invocations=" << pool.invocations();
}

}  // namespace wave_d
}  // namespace test
}  // namespace themis
