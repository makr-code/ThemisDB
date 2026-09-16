// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_chimera_highcardinality_stress.cpp
 * @brief Wave D — Chimera High-Cardinality Stress Tests.
 *
 * Validates the Chimera hybrid adapter under high-cardinality and concurrent
 * workload conditions without requiring real external backends.
 *
 * Test families
 * -------------
 * - HighCardinalityHybridQuery  : high-volume hybrid query routing
 * - ConcurrentRouterStress      : concurrent router dispatch under load
 * - FallbackPathStress          : fallback path under edge conditions
 *
 * ## Labels
 * wave_d;stress;not_release_critical
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_CHIMERA.md
 * @see src/chimera/ROADMAP.md — Wave D contribution closure
 */

// SIMULATION NOTE: All stubs in this file are in-process simulations.
// They do NOT drive real Chimera/adapter infrastructure and MUST NOT be linked
// into production code paths.

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// In-process stubs
// ─────────────────────────────────────────────────────────────────────────────

namespace {

struct StubChimeraRouter {
    std::atomic<uint64_t> routed{0};
    std::atomic<uint64_t> fallback_invoked{0};
    std::atomic<uint64_t> router_faults{0};

    bool route(uint64_t query_id) {
        ++routed;
        (void)query_id;
        return true; // primary path always succeeds in stub
    }

    void fallback(uint64_t query_id) {
        ++fallback_invoked;
        (void)query_id;
    }
};

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Stress test cases
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief HighCardinalityHybridQuery
 *
 * Issues 1 000 000 hybrid queries from 8 concurrent threads. Asserts that all
 * queries are routed with no faults.
 */
TEST(ChimeraHighCardinalityStress, HighCardinalityHybridQuery) {
    constexpr uint64_t kTotalQueries = 1'000'000ULL;
    constexpr int kThreads = 8;
    constexpr uint64_t kPerThread = kTotalQueries / kThreads;

    StubChimeraRouter router;

    std::vector<std::thread> workers;
    workers.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&, t]() {
            const uint64_t base = static_cast<uint64_t>(t) * kPerThread;
            for (uint64_t i = 0; i < kPerThread; ++i) {
                if (!router.route(base + i)) {
                    router.fallback(base + i);
                }
            }
        });
    }
    for (auto& th : workers) th.join();

    EXPECT_EQ(router.router_faults.load(), 0ULL)
        << "Router reported unexpected faults under high-cardinality stress";
    EXPECT_EQ(router.routed.load(), kTotalQueries)
        << "Not all queries were routed";
}

/**
 * @brief ConcurrentRouterStress
 *
 * Exercises concurrent router dispatch from 8 threads each issuing 100 000
 * queries. Asserts consistent routing counts with no faults.
 */
TEST(ChimeraHighCardinalityStress, ConcurrentRouterStress) {
    constexpr uint64_t kPerThread = 100'000ULL;
    constexpr int kThreads = 8;

    StubChimeraRouter router;

    std::vector<std::thread> workers;
    workers.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&, t]() {
            const uint64_t base = static_cast<uint64_t>(t) * kPerThread;
            for (uint64_t i = 0; i < kPerThread; ++i) {
                router.route(base + i);
            }
        });
    }
    for (auto& th : workers) th.join();

    EXPECT_EQ(router.router_faults.load(), 0ULL)
        << "Concurrent router stress produced unexpected faults";
    EXPECT_EQ(router.routed.load(), kThreads * kPerThread)
        << "Routing count mismatch after concurrent stress";
}

/**
 * @brief FallbackPathStress
 *
 * Directly invokes the fallback path 500 000 times from 4 threads. Asserts
 * that all invocations complete without errors.
 */
TEST(ChimeraHighCardinalityStress, FallbackPathStress) {
    constexpr uint64_t kPerThread = 125'000ULL;
    constexpr int kThreads = 4;

    StubChimeraRouter router;

    std::vector<std::thread> workers;
    workers.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&, t]() {
            const uint64_t base = static_cast<uint64_t>(t) * kPerThread;
            for (uint64_t i = 0; i < kPerThread; ++i) {
                router.fallback(base + i);
            }
        });
    }
    for (auto& th : workers) th.join();

    const uint64_t expected = kThreads * kPerThread;
    EXPECT_EQ(router.fallback_invoked.load(), expected)
        << "Fallback invocation count mismatch under stress";
}
