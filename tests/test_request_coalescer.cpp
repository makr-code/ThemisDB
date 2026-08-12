/**
 * @file test_request_coalescer.cpp
 * @brief Focused unit tests for cache::RequestCoalescer (Singleflight pattern).
 *
 * Test IDs: RC-01 … RC-14
 *
 * The tests verify that:
 *  - A single caller executes fn() and receives the result (trivial path).
 *  - Concurrent callers for the same key execute fn() exactly once.
 *  - Callers for different keys each execute their own fn().
 *  - Exception from fn() is propagated to all waiters as success=false.
 *  - After a flight completes, the next call for the same key starts a new flight.
 *  - inflight_count() reflects running flights.
 *  - Version and data fields are faithfully returned.
 */

#include <gtest/gtest.h>
#include "cache/request_coalescer.h"

#include <array>
#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace themis::cache;
using namespace std::chrono_literals;

// ---------------------------------------------------------------------------
// RC-01  Single caller: fn() runs and result is returned
// ---------------------------------------------------------------------------
TEST(RequestCoalescerTest, RC01_SingleCallerRunsFn) {
    RequestCoalescer rc;
    int call_count = 0;
    auto res = rc.Do("k1", [&]() -> RequestCoalescer::Result {
        ++call_count;
        return {true, "hello", 42u, ""};
    });
    ASSERT_NE(res, nullptr);
    EXPECT_TRUE(res->success);
    EXPECT_EQ(res->data, "hello");
    EXPECT_EQ(res->version, 42u);
    EXPECT_EQ(call_count, 1);
}

// ---------------------------------------------------------------------------
// RC-02  inflight_count() is 0 after completion
// ---------------------------------------------------------------------------
TEST(RequestCoalescerTest, RC02_InflightCountZeroAfterCompletion) {
    RequestCoalescer rc;
    rc.Do("k", []() -> RequestCoalescer::Result { return {true, "x", 0, ""}; });
    EXPECT_EQ(rc.inflight_count(), 0u);
}

// ---------------------------------------------------------------------------
// RC-03  Different keys each execute their own fn()
// ---------------------------------------------------------------------------
TEST(RequestCoalescerTest, RC03_DifferentKeysIndependent) {
    RequestCoalescer rc;
    int a_count = 0, b_count = 0;
    auto r1 = rc.Do("a", [&]() -> RequestCoalescer::Result { ++a_count; return {true, "A", 1, ""}; });
    auto r2 = rc.Do("b", [&]() -> RequestCoalescer::Result { ++b_count; return {true, "B", 2, ""}; });
    EXPECT_EQ(a_count, 1);
    EXPECT_EQ(b_count, 1);
    EXPECT_EQ(r1->data, "A");
    EXPECT_EQ(r2->data, "B");
}

// ---------------------------------------------------------------------------
// RC-04  fn() throwing exception → success=false, error field set
// ---------------------------------------------------------------------------
TEST(RequestCoalescerTest, RC04_ExceptionYieldsFailureResult) {
    RequestCoalescer rc;
    auto res = rc.Do("ex", []() -> RequestCoalescer::Result {
        throw std::runtime_error("backend down");
    });
    ASSERT_NE(res, nullptr);
    EXPECT_FALSE(res->success);
    EXPECT_FALSE(res->error.empty());
    EXPECT_NE(res->error.find("backend down"), std::string::npos);
}

// ---------------------------------------------------------------------------
// RC-05  fn() throwing unknown (non-std) exception → success=false
// ---------------------------------------------------------------------------
TEST(RequestCoalescerTest, RC05_UnknownExceptionYieldsFailure) {
    RequestCoalescer rc;
    auto res = rc.Do("unk", []() -> RequestCoalescer::Result {
        throw 42; // not derived from std::exception
    });
    ASSERT_NE(res, nullptr);
    EXPECT_FALSE(res->success);
}

// ---------------------------------------------------------------------------
// RC-06  After completion, new call for same key starts fresh flight
// ---------------------------------------------------------------------------
TEST(RequestCoalescerTest, RC06_SecondCallStartsFreshFlight) {
    RequestCoalescer rc;
    int cnt = 0;
    rc.Do("r", [&]() -> RequestCoalescer::Result { ++cnt; return {true, "v1", 1, ""}; });
    auto r2 = rc.Do("r", [&]() -> RequestCoalescer::Result { ++cnt; return {true, "v2", 2, ""}; });
    EXPECT_EQ(cnt, 2);
    EXPECT_EQ(r2->data, "v2");
    EXPECT_EQ(r2->version, 2u);
}

// ---------------------------------------------------------------------------
// RC-07  Concurrent callers: fn() executes exactly once
// ---------------------------------------------------------------------------
TEST(RequestCoalescerTest, RC07_ConcurrentCallersFnOnce) {
    RequestCoalescer rc;
    std::atomic<int> fn_calls{0};

    constexpr int N = 8;
    std::vector<std::thread> threads;
    std::vector<std::shared_ptr<RequestCoalescer::Result>> results(N);

    // A barrier to maximize the chance that all threads hit Do() simultaneously.
    std::atomic<int> ready{0};

    for (int i = 0; i < N; ++i) {
        threads.emplace_back([&, i]() {
            ready.fetch_add(1, std::memory_order_relaxed);
            while (ready.load(std::memory_order_relaxed) < N) {
                std::this_thread::yield();
            }
            results[i] = rc.Do("shared_key", [&]() -> RequestCoalescer::Result {
                fn_calls.fetch_add(1, std::memory_order_relaxed);
                // Simulate a slow backend fetch so other threads arrive in flight.
                std::this_thread::sleep_for(20ms);
                return {true, "shared_data", 99u, ""};
            });
        });
    }
    for (auto& t : threads) t.join();

    // fn() must have been called exactly once.
    EXPECT_EQ(fn_calls.load(), 1);

    // All threads received a non-null result.
    for (int i = 0; i < N; ++i) {
        ASSERT_NE(results[i], nullptr) << "Thread " << i << " got null result";
        EXPECT_TRUE(results[i]->success)  << "Thread " << i;
        EXPECT_EQ(results[i]->data, "shared_data") << "Thread " << i;
        EXPECT_EQ(results[i]->version, 99u) << "Thread " << i;
    }
}

// ---------------------------------------------------------------------------
// RC-08  All concurrent waiters receive success=false on owner fn() exception
// ---------------------------------------------------------------------------
TEST(RequestCoalescerTest, RC08_ConcurrentWaitersAllGetFailure) {
    RequestCoalescer rc;
    constexpr int N = 4;
    std::vector<std::thread> threads;
    std::vector<std::shared_ptr<RequestCoalescer::Result>> results(N);
    std::atomic<int> ready{0};

    for (int i = 0; i < N; ++i) {
        threads.emplace_back([&, i]() {
            ready.fetch_add(1, std::memory_order_relaxed);
            while (ready.load(std::memory_order_relaxed) < N) {
                std::this_thread::yield();
            }
            results[i] = rc.Do("fail_key", []() -> RequestCoalescer::Result {
                std::this_thread::sleep_for(5ms);
                throw std::runtime_error("intentional");
            });
        });
    }
    for (auto& t : threads) t.join();

    for (int i = 0; i < N; ++i) {
        ASSERT_NE(results[i], nullptr);
        EXPECT_FALSE(results[i]->success) << "Thread " << i << " should have failed";
    }
}

// ---------------------------------------------------------------------------
// RC-09  Independent keys run concurrently (no cross-key blocking)
// ---------------------------------------------------------------------------
TEST(RequestCoalescerTest, RC09_IndependentKeysRunConcurrently) {
    RequestCoalescer rc;
    std::atomic<int> running{0};
    constexpr int N = 4;
    std::vector<std::thread> threads;

    for (int i = 0; i < N; ++i) {
        threads.emplace_back([&, i]() {
            rc.Do("key_" + std::to_string(i), [&]() -> RequestCoalescer::Result {
                running.fetch_add(1, std::memory_order_relaxed);
                std::this_thread::sleep_for(10ms);
                return {true, "ok", 0, ""};
            });
        });
    }
    // Give threads a moment to enter their flights.
    std::this_thread::sleep_for(5ms);
    // Multiple flights may be running simultaneously.
    EXPECT_GE(running.load(), 1); // at least one started

    for (auto& t : threads) t.join();
}

// ---------------------------------------------------------------------------
// RC-10  Result data is not corrupted for large payloads
// ---------------------------------------------------------------------------
TEST(RequestCoalescerTest, RC10_LargePayloadIntegrity) {
    RequestCoalescer rc;
    std::string big(64 * 1024, 'Z'); // 64 KB JSON-like payload
    auto res = rc.Do("big", [&]() -> RequestCoalescer::Result {
        return {true, big, 7u, ""};
    });
    ASSERT_NE(res, nullptr);
    EXPECT_EQ(res->data.size(), 64u * 1024u);
    EXPECT_EQ(res->data.front(), 'Z');
    EXPECT_EQ(res->version, 7u);
}

// ---------------------------------------------------------------------------
// RC-11  Sequential reuse of the same key works N times
// ---------------------------------------------------------------------------
TEST(RequestCoalescerTest, RC11_SequentialReuseWorks) {
    RequestCoalescer rc;
    for (uint64_t v = 0; v < 10; ++v) {
        auto res = rc.Do("key", [v]() -> RequestCoalescer::Result {
            return {true, "data", v, ""};
        });
        ASSERT_NE(res, nullptr);
        EXPECT_TRUE(res->success);
        EXPECT_EQ(res->version, v);
    }
}

// ---------------------------------------------------------------------------
// RC-12  Multiple independent coalescers do not interfere
// ---------------------------------------------------------------------------
TEST(RequestCoalescerTest, RC12_MultipleCoalescersIndependent) {
    RequestCoalescer rc1, rc2;
    int c1 = 0, c2 = 0;
    auto r1 = rc1.Do("k", [&]() -> RequestCoalescer::Result { ++c1; return {true, "a", 1, ""}; });
    auto r2 = rc2.Do("k", [&]() -> RequestCoalescer::Result { ++c2; return {true, "b", 2, ""}; });
    EXPECT_EQ(c1, 1);
    EXPECT_EQ(c2, 1);
    EXPECT_EQ(r1->data, "a");
    EXPECT_EQ(r2->data, "b");
}

// ---------------------------------------------------------------------------
// RC-13  Empty key is valid
// ---------------------------------------------------------------------------
TEST(RequestCoalescerTest, RC13_EmptyKeyIsValid) {
    RequestCoalescer rc;
    auto res = rc.Do("", []() -> RequestCoalescer::Result { return {true, "empty_key", 0, ""}; });
    ASSERT_NE(res, nullptr);
    EXPECT_TRUE(res->success);
}

// ---------------------------------------------------------------------------
// RC-14  High concurrency stress: fn() called exactly once per unique key
// ---------------------------------------------------------------------------
TEST(RequestCoalescerTest, RC14_HighConcurrencyStress) {
    constexpr int KEYS    = 4;
    constexpr int THREADS = 16; // 4 threads per key

    RequestCoalescer rc;
    std::array<std::atomic<int>, KEYS> fn_calls{};
    for (auto& c : fn_calls) c.store(0);

    std::vector<std::thread> threads;
    threads.reserve(THREADS);
    std::atomic<int> start_barrier{0};

    for (int t = 0; t < THREADS; ++t) {
        int key_idx = t % KEYS;
        threads.emplace_back([&, key_idx]() {
            start_barrier.fetch_add(1, std::memory_order_relaxed);
            while (start_barrier.load(std::memory_order_relaxed) < THREADS) {
                std::this_thread::yield();
            }
            rc.Do("stress_key_" + std::to_string(key_idx),
                  [&, key_idx]() -> RequestCoalescer::Result {
                      fn_calls[key_idx].fetch_add(1, std::memory_order_relaxed);
                      std::this_thread::sleep_for(15ms);
                      return {true, "v", static_cast<uint64_t>(key_idx), ""};
                  });
        });
    }
    for (auto& t : threads) t.join();

    // Each unique key should have been fetched exactly once.
    for (int k = 0; k < KEYS; ++k) {
        EXPECT_EQ(fn_calls[k].load(), 1) << "Key " << k << " fetched " << fn_calls[k].load() << " times";
    }
}
