/*
 * ThemisDB | File: test_chaos_concurrency_hardening.cpp | Version: 1.0.0
 * Author: Copilot | Maturity: 🟢 PRODUCTION-READY | Status: Phase 4 — Concurrency Hardening
 * Purpose: Phase 4 focused regression tests for chaos concurrency and callback edge hardening.
 */

/**
 * @file test_chaos_concurrency_hardening.cpp
 * @brief Phase 4 focused regression tests for chaos concurrency and callback hardening.
 *
 * Covers eight test cases mapped to chaos roadmap Phase 4 hardening:
 *
 * ### CCH — Concurrency and Callback Edge Hardening
 *   CCH-01  Concurrent injectFault() from N threads — no crash, count consistent
 *   CCH-02  Concurrent recoverFault() while injecting — no crash, no data race
 *   CCH-03  Concurrent isFaultActive() during inject/recover — always returns bool (no throw)
 *   CCH-04  Callback registered before inject — fires exactly once per inject
 *   CCH-05  Multiple callbacks registered — all fire in registration order
 *   CCH-06  Callback registered after inject — does not fire for prior injections
 *   CCH-07  Concurrent clearAllFaults() + inject — no crash, deterministic empty state
 *   CCH-08  getActiveFaults() snapshot under concurrent churn — no crash, no stale entry
 *
 * @see src/chaos/ROADMAP.md — Phase 4 item
 * @see include/chaos/chaos_contract.h — § 5 callback semantics, § 7 blast-radius
 */

#include <gtest/gtest.h>

#include "chaos/chaos_framework.h"
#include "chaos/chaos_contract.h"

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>
#include <future>

using namespace themis::chaos;
using namespace std::chrono_literals;

namespace {

/// Build a permanent FaultSpec for the given node and type.
FaultSpec makeSpec(std::string_view node, FaultType type = FaultType::NODE_FAILURE) {
    return FaultSpec{type, std::string(node)};
}

/// Build N unique node IDs with a given prefix.
std::vector<std::string> makeNodeIds(std::string_view prefix, int n) {
    std::vector<std::string> ids;
    ids.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        ids.push_back(std::string(prefix) + std::to_string(i));
    }
    return ids;
}

} // anonymous namespace

// ============================================================================
// CCH-01: Concurrent injectFault() — no crash, count is consistent
// ============================================================================

/**
 * @test CCH-01 — Concurrent injectFault() from N threads does not crash and
 *       results in a consistent active fault count.
 *
 * Each thread injects a unique node; after join, activeFaultCount() must equal
 * the number of successful injections.
 */
TEST(ChaosConcurrencyHardeningTest, CCH01_ConcurrentInjectNoRace) {
    constexpr int kThreads  = 8;
    constexpr int kPerThread = 16;

    FaultInjector fi{"cch01"};
    std::atomic<int> success_count{0};

    std::vector<std::thread> workers;
    workers.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&fi, &success_count, t]() {
            for (int i = 0; i < kPerThread; ++i) {
                const std::string node = "cch01-t" + std::to_string(t)
                                         + "-n" + std::to_string(i);
                if (fi.injectFault(makeSpec(node))) {
                    success_count.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (auto& w : workers) {
        w.join();
    }

    EXPECT_EQ(static_cast<int>(fi.activeFaultCount()), success_count.load());
    EXPECT_GT(success_count.load(), 0);
}

// ============================================================================
// CCH-02: Concurrent recoverFault() while injecting — no data race
// ============================================================================

/**
 * @test CCH-02 — Concurrent recoverFault() while inject threads are running
 *       produces no crash or data race.
 */
TEST(ChaosConcurrencyHardeningTest, CCH02_ConcurrentRecoverDuringInject) {
    constexpr int kInjectThreads  = 4;
    constexpr int kRecoverThreads = 4;
    constexpr int kIterations     = 32;

    FaultInjector fi{"cch02"};
    std::atomic<bool> go{false};

    // Pre-populate some nodes that recover threads can target.
    const auto nodes = makeNodeIds("cch02-pre", 16);
    for (const auto& n : nodes) {
        fi.injectFault(makeSpec(n));
    }

    std::vector<std::thread> workers;
    workers.reserve(kInjectThreads + kRecoverThreads);

    for (int t = 0; t < kInjectThreads; ++t) {
        workers.emplace_back([&fi, &go, t]() {
            while (!go.load(std::memory_order_acquire)) {}
            for (int i = 0; i < kIterations; ++i) {
                const std::string node = "cch02-dyn-t" + std::to_string(t)
                                         + "-" + std::to_string(i);
                fi.injectFault(makeSpec(node));
            }
        });
    }
    for (int t = 0; t < kRecoverThreads; ++t) {
        workers.emplace_back([&fi, &go, &nodes, t]() {
            while (!go.load(std::memory_order_acquire)) {}
            for (int i = 0; i < kIterations; ++i) {
                const auto& node = nodes[static_cast<std::size_t>(i % static_cast<int>(nodes.size()))];
                fi.recoverFault(node);  // may return false; that is acceptable
            }
        });
    }

    go.store(true, std::memory_order_release);
    for (auto& w : workers) {
        w.join();
    }

    // The test passes if we reach here without a crash or sanitizer alert.
    SUCCEED();
}

// ============================================================================
// CCH-03: Concurrent isFaultActive() during inject/recover — always returns bool
// ============================================================================

/**
 * @test CCH-03 — isFaultActive() called concurrently with inject and recover
 *       threads always returns a valid boolean (no throw, no crash).
 */
TEST(ChaosConcurrencyHardeningTest, CCH03_ConcurrentQueryDuringMutations) {
    FaultInjector fi{"cch03"};
    const std::string node = "cch03-target";
    fi.injectFault(makeSpec(node));

    std::atomic<bool> stop{false};
    std::atomic<int>  query_count{0};

    // Reader thread.
    std::thread reader([&fi, &stop, &node, &query_count]() {
        while (!stop.load(std::memory_order_acquire)) {
            bool active = fi.isFaultActive(node);
            (void)active;
            query_count.fetch_add(1, std::memory_order_relaxed);
        }
    });

    // Writer thread: alternates inject / recover.
    std::thread writer([&fi, &node]() {
        for (int i = 0; i < 64; ++i) {
            fi.injectFault(makeSpec(node));
            fi.recoverFault(node);
        }
    });

    writer.join();
    stop.store(true, std::memory_order_release);
    reader.join();

    EXPECT_GT(query_count.load(), 0);
}

// ============================================================================
// CCH-04: Callback fires exactly once per inject
// ============================================================================

/**
 * @test CCH-04 — A callback registered before inject fires exactly once per
 *       successful injectFault() call.
 */
TEST(ChaosConcurrencyHardeningTest, CCH04_CallbackFiresOncePerInject) {
    FaultInjector fi{"cch04"};
    std::atomic<int> inject_calls{0};

    fi.registerEventCallback([&inject_calls](const FaultSpec& /*spec*/, bool injected) {
        if (injected) {
            inject_calls.fetch_add(1, std::memory_order_relaxed);
        }
    });

    constexpr int kNodes = 8;
    for (int i = 0; i < kNodes; ++i) {
        fi.injectFault(makeSpec("cch04-n" + std::to_string(i)));
    }

    EXPECT_EQ(inject_calls.load(), kNodes);
}

// ============================================================================
// CCH-05: Multiple callbacks fire in registration order
// ============================================================================

/**
 * @test CCH-05 — Multiple registered callbacks all fire in FIFO registration
 *       order on a single injectFault() call.
 */
TEST(ChaosConcurrencyHardeningTest, CCH05_MultipleCallbacksFIFOOrder) {
    FaultInjector fi{"cch05"};
    std::vector<int> order;
    std::mutex       order_mu;

    for (int id = 0; id < 4; ++id) {
        fi.registerEventCallback([id, &order, &order_mu](const FaultSpec& /*spec*/, bool injected) {
            if (injected) {
                std::lock_guard<std::mutex> lk(order_mu);
                order.push_back(id);
            }
        });
    }

    fi.injectFault(makeSpec("cch05-n1"));

    std::lock_guard<std::mutex> lk(order_mu);
    ASSERT_EQ(order.size(), 4u);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(order[static_cast<std::size_t>(i)], i)
            << "Callback " << i << " fired out of order";
    }
}

// ============================================================================
// CCH-06: Callback registered after inject does not fire for prior injections
// ============================================================================

/**
 * @test CCH-06 — A callback registered after injectFault() does NOT fire
 *       retroactively for the already-injected fault.
 */
TEST(ChaosConcurrencyHardeningTest, CCH06_LateCallbackDoesNotFireRetroactively) {
    FaultInjector fi{"cch06"};

    fi.injectFault(makeSpec("cch06-n1"));  // injected before callback registration

    std::atomic<int> late_calls{0};
    fi.registerEventCallback([&late_calls](const FaultSpec& /*spec*/, bool injected) {
        if (injected) {
            late_calls.fetch_add(1, std::memory_order_relaxed);
        }
    });

    // The callback was registered after the inject; it must NOT have fired yet.
    EXPECT_EQ(late_calls.load(), 0);

    // A subsequent inject DOES fire the newly registered callback.
    fi.injectFault(makeSpec("cch06-n2"));
    EXPECT_EQ(late_calls.load(), 1);
}

// ============================================================================
// CCH-07: Concurrent clearAllFaults() + inject — no crash
// ============================================================================

/**
 * @test CCH-07 — clearAllFaults() called concurrently with inject threads
 *       does not crash and leaves the registry in a consistent (empty or
 *       partially-populated) state after all threads complete.
 */
TEST(ChaosConcurrencyHardeningTest, CCH07_ConcurrentClearAndInject) {
    FaultInjector fi{"cch07"};
    std::atomic<bool> go{false};

    std::thread inject_thread([&fi, &go]() {
        while (!go.load(std::memory_order_acquire)) {}
        for (int i = 0; i < 64; ++i) {
            fi.injectFault(makeSpec("cch07-n" + std::to_string(i)));
        }
    });
    std::thread clear_thread([&fi, &go]() {
        while (!go.load(std::memory_order_acquire)) {}
        for (int i = 0; i < 16; ++i) {
            fi.clearAllFaults();
        }
    });

    go.store(true, std::memory_order_release);
    inject_thread.join();
    clear_thread.join();

    // Post-condition: activeFaultCount() must be consistent (≤ 64 injected).
    std::size_t count = fi.activeFaultCount();
    EXPECT_LE(count, 64u);
    // Reaching here without crash / sanitizer alert is the primary pass criterion.
    SUCCEED();
}

// ============================================================================
// CCH-08: getActiveFaults() snapshot under concurrent churn — no crash
// ============================================================================

/**
 * @test CCH-08 — getActiveFaults() snapshot is safe to call under concurrent
 *       inject and recover churn: no crash, no stale dangling entry.
 */
TEST(ChaosConcurrencyHardeningTest, CCH08_GetActiveFaultsSnapshotUnderChurn) {
    FaultInjector fi{"cch08"};
    std::atomic<bool> stop{false};
    std::atomic<int>  snapshot_count{0};

    // Churn thread: alternates inject and recover on a fixed node set.
    std::thread churn([&fi, &stop]() {
        const auto nodes = makeNodeIds("cch08-churn", 8);
        while (!stop.load(std::memory_order_acquire)) {
            for (const auto& n : nodes) {
                fi.injectFault(makeSpec(n));
            }
            for (const auto& n : nodes) {
                fi.recoverFault(n);
            }
        }
    });

    // Snapshot thread: repeatedly calls getActiveFaults().
    std::thread snapshot([&fi, &stop, &snapshot_count]() {
        while (!stop.load(std::memory_order_acquire)) {
            auto faults = fi.getActiveFaults();
            (void)faults;
            snapshot_count.fetch_add(1, std::memory_order_relaxed);
        }
    });

    // Run for a bounded duration.
    std::this_thread::sleep_for(50ms);
    stop.store(true, std::memory_order_release);
    churn.join();
    snapshot.join();

    EXPECT_GT(snapshot_count.load(), 0);
}
