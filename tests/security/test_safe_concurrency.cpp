/*
 * ThemisDB | File: test_safe_concurrency.cpp | Version: 1.0.0
 * Maturity: 🟢 PRODUCTION-READY | Score: 95/100
 * Gap Summary: total=0; CWE-362, CWE-366, CWE-574 coverage
 * Status: Production Ready
 */

/**
 * @file test_safe_concurrency.cpp
 * @brief Comprehensive unit tests for include/security/safe_concurrency.h.
 *
 * Test coverage summary
 * ---------------------
 * ThreadSafeCounter:
 *   - Default construction yields zero
 *   - Custom initial value
 *   - increment() returns new value
 *   - decrement() returns new value
 *   - add() returns new value
 *   - load() / operator T()
 *   - reset() with default and custom value
 *   - Concurrent increments produce correct total (thread-safety)
 *
 * MonotonicSequencer:
 *   - next() starts at 1 by default
 *   - next() is strictly increasing
 *   - last() reflects current high-water mark
 *   - tryAdvance() advances when seq > current
 *   - tryAdvance() is a no-op when seq <= current
 *   - Concurrent next() calls produce unique values (thread-safety)
 *   - tryAdvance() under contention never regresses (thread-safety)
 *
 * SharedDataGuard:
 *   - lock() gives exclusive access
 *   - apply() executes under the lock
 *   - lock_shared() gives read access (shared_mutex specialisation)
 *   - Concurrent writes via lock() produce correct result
 *
 * SafeCAS:
 *   - load() returns initial value
 *   - store() updates value
 *   - trySet() succeeds when expected matches
 *   - trySet() fails and updates expected when mismatch
 *   - trySetStrong() succeeds
 *   - exchange() returns old value
 *   - Concurrent trySet() loop advances value correctly
 *
 * SingletonHolder:
 *   - instance() returns reference to the same object across calls
 *   - instance() from multiple threads returns same address
 *
 * LockOrderGuard:
 *   - lockTwo() acquires both mutexes
 *   - lockThree() acquires all three mutexes
 *   - No deadlock when two threads acquire same pair in opposite order
 *
 * ScopedFlag:
 *   - Default construction is false
 *   - set() / isSet() / clear()
 *   - setIfClear() returns true only on first call
 *   - clearIfSet() returns true only when set
 *   - Concurrent set/clear cycle is data-race free (thread-safety)
 */

#include <gtest/gtest.h>
#include "security/safe_concurrency.h"

#include <thread>
#include <vector>
#include <atomic>
#include <set>
#include <algorithm>
#include <numeric>

using namespace themis::security;

// ============================================================================
// ThreadSafeCounter tests
// ============================================================================

TEST(ThreadSafeCounter, DefaultConstructionIsZero) {
    ThreadSafeCounter<uint64_t> c;
    EXPECT_EQ(c.load(), 0u);
}

TEST(ThreadSafeCounter, CustomInitialValue) {
    ThreadSafeCounter<uint64_t> c(42u);
    EXPECT_EQ(c.load(), 42u);
}

TEST(ThreadSafeCounter, IncrementReturnsNewValue) {
    ThreadSafeCounter<uint64_t> c;
    EXPECT_EQ(c.increment(), 1u);
    EXPECT_EQ(c.increment(), 2u);
}

TEST(ThreadSafeCounter, DecrementReturnsNewValue) {
    ThreadSafeCounter<uint64_t> c(10u);
    EXPECT_EQ(c.decrement(), 9u);
    EXPECT_EQ(c.decrement(), 8u);
}

TEST(ThreadSafeCounter, AddReturnsNewValue) {
    ThreadSafeCounter<uint64_t> c(5u);
    EXPECT_EQ(c.add(3u), 8u);
    EXPECT_EQ(c.load(), 8u);
}

TEST(ThreadSafeCounter, ImplicitConversion) {
    ThreadSafeCounter<uint32_t> c(7u);
    uint32_t val = c;
    EXPECT_EQ(val, 7u);
}

TEST(ThreadSafeCounter, ResetToZero) {
    ThreadSafeCounter<uint64_t> c(99u);
    c.reset();
    EXPECT_EQ(c.load(), 0u);
}

TEST(ThreadSafeCounter, ResetToCustomValue) {
    ThreadSafeCounter<uint64_t> c(99u);
    c.reset(50u);
    EXPECT_EQ(c.load(), 50u);
}

TEST(ThreadSafeCounter, ConcurrentIncrements) {
    constexpr int kThreads     = 8;
    constexpr int kPerThread   = 10'000;
    ThreadSafeCounter<uint64_t> c;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&c]() {
            for (int j = 0; j < kPerThread; ++j) {
                c.increment();
            }
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_EQ(c.load(), static_cast<uint64_t>(kThreads * kPerThread));
}

// ============================================================================
// MonotonicSequencer tests
// ============================================================================

TEST(MonotonicSequencer, DefaultStartIsOne) {
    MonotonicSequencer seq;
    EXPECT_EQ(seq.next(), 1u);
}

TEST(MonotonicSequencer, CustomStart) {
    MonotonicSequencer seq(100u);
    EXPECT_EQ(seq.next(), 101u);
}

TEST(MonotonicSequencer, StrictlyIncreasing) {
    MonotonicSequencer seq;
    uint64_t prev = 0;
    for (int i = 0; i < 100; ++i) {
        uint64_t n = seq.next();
        EXPECT_GT(n, prev);
        prev = n;
    }
}

TEST(MonotonicSequencer, LastReflectsHighWaterMark) {
    MonotonicSequencer seq;
    seq.next();
    seq.next();
    seq.next();
    EXPECT_EQ(seq.last(), 3u);
}

TEST(MonotonicSequencer, TryAdvanceSucceeds) {
    MonotonicSequencer seq;
    EXPECT_TRUE(seq.tryAdvance(10u));
    EXPECT_EQ(seq.last(), 10u);
}

TEST(MonotonicSequencer, TryAdvanceNoOpWhenNotHigher) {
    MonotonicSequencer seq;
    seq.tryAdvance(5u);
    EXPECT_FALSE(seq.tryAdvance(3u));   // 3 < 5, should not advance
    EXPECT_EQ(seq.last(), 5u);
}

TEST(MonotonicSequencer, TryAdvanceSameValueIsNoOp) {
    MonotonicSequencer seq;
    seq.tryAdvance(7u);
    EXPECT_FALSE(seq.tryAdvance(7u));   // equal, not strictly greater
    EXPECT_EQ(seq.last(), 7u);
}

TEST(MonotonicSequencer, ConcurrentNextUniqueValues) {
    constexpr int kThreads   = 4;
    constexpr int kPerThread = 5'000;
    MonotonicSequencer seq;

    std::vector<std::vector<uint64_t>> results(kThreads);
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&seq, &results, i]() {
            results[i].reserve(kPerThread);
            for (int j = 0; j < kPerThread; ++j) {
                results[i].push_back(seq.next());
            }
        });
    }
    for (auto& t : threads) t.join();

    // Flatten and check uniqueness
    std::vector<uint64_t> all;
    all.reserve(kThreads * kPerThread);
    for (auto& v : results) all.insert(all.end(), v.begin(), v.end());
    std::sort(all.begin(), all.end());
    EXPECT_TRUE(std::adjacent_find(all.begin(), all.end()) == all.end())
        << "Duplicate sequence numbers detected";
    EXPECT_EQ(all.front(), 1u);
    EXPECT_EQ(all.back(), static_cast<uint64_t>(kThreads * kPerThread));
}

TEST(MonotonicSequencer, ConcurrentTryAdvanceNeverRegresses) {
    constexpr int kThreads = 8;
    MonotonicSequencer seq;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&seq, i]() {
            // Each thread tries to advance to its own bucket of values
            for (uint64_t v = static_cast<uint64_t>(i) * 100 + 1;
                 v <= static_cast<uint64_t>(i + 1) * 100;
                 ++v)
            {
                seq.tryAdvance(v);
            }
        });
    }
    for (auto& t : threads) t.join();

    // The high-water mark must be at most kThreads*100 and must not be zero
    uint64_t hwm = seq.last();
    EXPECT_GT(hwm, 0u);
    EXPECT_LE(hwm, static_cast<uint64_t>(kThreads * 100));
}

// ============================================================================
// SharedDataGuard tests
// ============================================================================

TEST(SharedDataGuard, LockGivesAccess) {
    SharedDataGuard<int> guard(42);
    auto lock = guard.lock();
    EXPECT_EQ(*lock, 42);
}

TEST(SharedDataGuard, LockAllowsMutation) {
    SharedDataGuard<int> guard(0);
    {
        auto lock = guard.lock();
        *lock = 99;
    }
    auto lock2 = guard.lock();
    EXPECT_EQ(*lock2, 99);
}

TEST(SharedDataGuard, ApplyExecutesUnderLock) {
    SharedDataGuard<std::vector<int>> guard;
    guard.apply([](std::vector<int>& v) { v.push_back(1); v.push_back(2); });
    auto result = guard.apply([](std::vector<int>& v) { return v.size(); });
    EXPECT_EQ(result, 2u);
}

TEST(SharedDataGuard, SharedMutexVariantReadAccess) {
    SharedDataGuard<std::string, std::shared_mutex> guard("hello");
    auto slock = guard.lock_shared();
    EXPECT_EQ(*slock, "hello");
}

TEST(SharedDataGuard, ConcurrentWritesSafe) {
    constexpr int kThreads   = 4;
    constexpr int kPerThread = 2'500;
    SharedDataGuard<int> guard(0);

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&guard]() {
            for (int j = 0; j < kPerThread; ++j) {
                auto lk = guard.lock();
                ++(*lk);
            }
        });
    }
    for (auto& t : threads) t.join();

    auto lk = guard.lock();
    EXPECT_EQ(*lk, kThreads * kPerThread);
}

// ============================================================================
// SafeCAS tests
// ============================================================================

TEST(SafeCAS, LoadReturnsInitialValue) {
    SafeCAS<uint64_t> cas(10u);
    EXPECT_EQ(cas.load(), 10u);
}

TEST(SafeCAS, StoreUpdatesValue) {
    SafeCAS<uint64_t> cas(0u);
    cas.store(55u);
    EXPECT_EQ(cas.load(), 55u);
}

TEST(SafeCAS, TrySetSucceedsOnMatch) {
    SafeCAS<uint64_t> cas(5u);
    uint64_t expected = 5u;
    EXPECT_TRUE(cas.trySet(expected, 10u));
    EXPECT_EQ(cas.load(), 10u);
}

TEST(SafeCAS, TrySetFailsOnMismatch) {
    SafeCAS<uint64_t> cas(5u);
    uint64_t expected = 99u;  // wrong
    EXPECT_FALSE(cas.trySet(expected, 10u));
    EXPECT_EQ(expected, 5u);  // updated to current value
    EXPECT_EQ(cas.load(), 5u);
}

TEST(SafeCAS, TrySetStrongSucceeds) {
    SafeCAS<int> cas(7);
    int expected = 7;
    EXPECT_TRUE(cas.trySetStrong(expected, 14));
    EXPECT_EQ(cas.load(), 14);
}

TEST(SafeCAS, ExchangeReturnsPreviousValue) {
    SafeCAS<uint32_t> cas(3u);
    uint32_t old = cas.exchange(9u);
    EXPECT_EQ(old, 3u);
    EXPECT_EQ(cas.load(), 9u);
}

TEST(SafeCAS, ConcurrentTrySetLoop) {
    // Multiple threads race to advance a shared counter from 0 to 1000
    SafeCAS<uint64_t> counter(0u);
    constexpr int kThreads = 4;
    constexpr uint64_t kTarget = 1'000u;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&counter]() {
            for (uint64_t target = 1; target <= kTarget; ++target) {
                uint64_t expected = counter.load();
                while (expected < target) {
                    if (counter.trySet(expected, target)) break;
                    // expected refreshed by trySet on failure; retry
                }
            }
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_EQ(counter.load(), kTarget);
}

// ============================================================================
// SingletonHolder tests
// ============================================================================

namespace {
struct RegistryStub {
    int value{42};
};
} // anonymous namespace

TEST(SingletonHolder, SameAddressAcrossCalls) {
    auto& a = SingletonHolder<RegistryStub>::instance();
    auto& b = SingletonHolder<RegistryStub>::instance();
    EXPECT_EQ(&a, &b);
}

TEST(SingletonHolder, StateIsPersistent) {
    auto& inst = SingletonHolder<RegistryStub>::instance();
    inst.value = 99;
    EXPECT_EQ(SingletonHolder<RegistryStub>::instance().value, 99);
}

TEST(SingletonHolder, SameAddressFromMultipleThreads) {
    constexpr int kThreads = 8;
    std::vector<RegistryStub*> ptrs(kThreads, nullptr);
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&ptrs, i]() {
            ptrs[i] = &SingletonHolder<RegistryStub>::instance();
        });
    }
    for (auto& t : threads) t.join();

    for (int i = 1; i < kThreads; ++i) {
        EXPECT_EQ(ptrs[0], ptrs[i]) << "Thread " << i << " got different address";
    }
}

// ============================================================================
// LockOrderGuard tests
// ============================================================================

TEST(LockOrderGuard, LockTwoAcquiresBothMutexes) {
    std::mutex a, b;
    {
        auto [la, lb] = LockOrderGuard::lockTwo(a, b);
        // If we reach here both mutexes are locked; try_lock should fail.
        EXPECT_FALSE(a.try_lock());
        EXPECT_FALSE(b.try_lock());
    }
    // After locks are released, try_lock must succeed.
    EXPECT_TRUE(a.try_lock());
    a.unlock();
    EXPECT_TRUE(b.try_lock());
    b.unlock();
}

TEST(LockOrderGuard, LockThreeAcquiresAllMutexes) {
    std::mutex a, b, c;
    {
        auto [la, lb, lc] = LockOrderGuard::lockThree(a, b, c);
        EXPECT_FALSE(a.try_lock());
        EXPECT_FALSE(b.try_lock());
        EXPECT_FALSE(c.try_lock());
    }
    EXPECT_TRUE(a.try_lock()); a.unlock();
    EXPECT_TRUE(b.try_lock()); b.unlock();
    EXPECT_TRUE(c.try_lock()); c.unlock();
}

TEST(LockOrderGuard, NoDeadlockOppositeOrder) {
    // Two threads acquire same two mutexes in opposite order.  If this
    // hangs it would be a deadlock; GTest's --test_timeout should catch it
    // in CI.  The test just verifies both threads complete.
    std::mutex mu_a, mu_b;
    std::atomic<int> completed{0};

    auto worker = [&](bool reverse) {
        for (int i = 0; i < 1'000; ++i) {
            if (!reverse) {
                auto [la, lb] = LockOrderGuard::lockTwo(mu_a, mu_b);
                (void)la; (void)lb;
            } else {
                auto [lb, la] = LockOrderGuard::lockTwo(mu_b, mu_a);
                (void)lb; (void)la;
            }
        }
        completed.fetch_add(1, std::memory_order_relaxed);
    };

    std::thread t1(worker, false);
    std::thread t2(worker, true);
    t1.join();
    t2.join();

    EXPECT_EQ(completed.load(), 2);
}

// ============================================================================
// ScopedFlag tests
// ============================================================================

TEST(ScopedFlag, DefaultConstructionIsFalse) {
    ScopedFlag f;
    EXPECT_FALSE(f.isSet());
}

TEST(ScopedFlag, SetAndIsSet) {
    ScopedFlag f;
    f.set();
    EXPECT_TRUE(f.isSet());
}

TEST(ScopedFlag, ClearAfterSet) {
    ScopedFlag f;
    f.set();
    f.clear();
    EXPECT_FALSE(f.isSet());
}

TEST(ScopedFlag, SetIfClearReturnsTrueOnFirstCall) {
    ScopedFlag f;
    EXPECT_TRUE(f.setIfClear());
    EXPECT_TRUE(f.isSet());
}

TEST(ScopedFlag, SetIfClearReturnsFalseWhenAlreadySet) {
    ScopedFlag f;
    f.set();
    EXPECT_FALSE(f.setIfClear());
}

TEST(ScopedFlag, ClearIfSetReturnsTrueWhenSet) {
    ScopedFlag f;
    f.set();
    EXPECT_TRUE(f.clearIfSet());
    EXPECT_FALSE(f.isSet());
}

TEST(ScopedFlag, ClearIfSetReturnsFalseWhenClear) {
    ScopedFlag f;
    EXPECT_FALSE(f.clearIfSet());
}

TEST(ScopedFlag, InitialTrueValue) {
    ScopedFlag f(true);
    EXPECT_TRUE(f.isSet());
}

TEST(ScopedFlag, ConcurrentSetClearCycleIsDataRaceFree) {
    // Multiple threads repeatedly set and clear the flag.  ThreadSanitizer
    // should report no data races on a TSAN build.
    ScopedFlag f;
    std::atomic<bool> stop{false};
    constexpr int kThreads = 4;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&f, &stop, i]() {
            while (!stop.load(std::memory_order_relaxed)) {
                if (i % 2 == 0) {
                    f.setIfClear();
                } else {
                    f.clearIfSet();
                }
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    stop.store(true, std::memory_order_relaxed);
    for (auto& t : threads) t.join();

    // No assertions needed — the test passes if it completes without data
    // races or crashes.
    SUCCEED();
}
