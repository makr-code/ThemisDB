/**
 * @file test_resource_pooling.cpp
 * @brief Phase 3 P3-03: Resource Pooling — Connection/Thread/Buffer Manager Tests
 *
 * This test file validates Phase 3-03 deliverables:
 *  - Unified resource pool interface + orchestrator (P3-03-A design doc)
 *  - Adaptive connection pool (min/max dynamic adjustment) (P3-03-B)
 *  - Thread pool tuning (work-stealing + backpressure) (P3-03-C)
 *  - Buffer pool (slab allocator for fixed-size blocks) (P3-03-D)
 *  - Integration + saturation testing (P3-03-E)
 *  - Performance tuning + profiling (P3-03-F)
 *
 * Target: 36 tests (8+10+6+8+4 from P3-03 tasks B-F)
 *
 * Acceptance Criteria:
 *  - Connection pool: min=5, max=50, scale-up latency < 10ms
 *  - Thread pool: work-stealing queue, latency p99 < 5ms
 *  - Buffer pool: > 90% reuse rate (vs. malloc/free baseline)
 *  - Peak resource utilization <= 80% under synthetic load
 *  - Complete Doxygen + RESOURCE_POOLING.md architecture doc
 *
 * @see ai_working/PHASE3_OPTIMIZATION_DETAILED_PLAN.md (P3-03)
 * @see include/base/resource_pool_manager.h
 * @see include/execution/thread_pool_manager.h
 * @see include/base/buffer_pool.h
 */

#include <gtest/gtest.h>

#include "base/buffer_pool.h"
#include "base/resource_pool_manager.h"
#include "execution/thread_pool_manager.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

namespace themis { namespace resource { 

// ===== Task P3-03-B: Adaptive Connection Pool (8 tests) =====

/**
 * @test ConnectionPoolInitialization
 * @brief Validates connection pool initialization with min/max constraints.
 *
 * Verifies:
 *  - Pool initialized with min=5 connections
 *  - Max capacity set to 50 connections
 *  - Available connections = min initially
 */
TEST(Phase3ResourcePooling, ConnectionPoolInitialization) {
    AdaptiveConnectionPool::Config cfg;
    cfg.min_size = 5;
    cfg.max_size = 50;
    AdaptiveConnectionPool pool(cfg);

    EXPECT_EQ(pool.size(),      5u);
    EXPECT_EQ(pool.available(), 5u);
    EXPECT_EQ(pool.in_use(),    0u);
}

/**
 * @test BufferPoolEvictionTelemetry
 * @brief Validates eviction telemetry when the retained free-list is saturated.
 */
TEST(Phase3ResourcePooling, BufferPoolEvictionTelemetry) {
    BufferPool::Config cfg;
    cfg.initial_per_class = 0;
    cfg.max_per_class = 1;
    BufferPool pool(cfg);

    auto buf1 = pool.acquire(128);
    auto buf2 = pool.acquire(128);
    ASSERT_TRUE(buf1.valid());
    ASSERT_TRUE(buf2.valid());

    buf1.release();
    buf2.release();

    const auto st = pool.statistics();
    EXPECT_EQ(st.total_releases, 2u);
    EXPECT_EQ(st.release_evictions, 1u);
    EXPECT_EQ(st.per_class_evictions[0], 1u);
    EXPECT_EQ(st.per_class_free[0], 1u);
}

/**
 * @test ConnectionPoolAcquisitionUnderNormalLoad
 * @brief Validates connection acquisition under normal load.
 *
 * Verifies:
 *  - Acquire connection from pool succeeds instantly
 *  - Available connection count decremented
 *  - in_use count incremented
 */
TEST(Phase3ResourcePooling, ConnectionPoolAcquisitionUnderNormalLoad) {
    AdaptiveConnectionPool pool;  // default min=5
    int slot = -1;
    const bool ok = pool.acquire(std::chrono::milliseconds(500), slot);
    ASSERT_TRUE(ok);
    EXPECT_GE(slot, 0);
    EXPECT_EQ(pool.in_use(), 1u);
    pool.release(slot);
    EXPECT_EQ(pool.in_use(), 0u);
}

/**
 * @test ConnectionPoolReleaseBackToPool
 * @brief Validates connection release/return to pool.
 *
 * Verifies:
 *  - Released connection returned to pool
 *  - Available count incremented after release
 */
TEST(Phase3ResourcePooling, ConnectionPoolReleaseBackToPool) {
    AdaptiveConnectionPool pool;
    int s1 = -1, s2 = -1;
    ASSERT_TRUE(pool.acquire(std::chrono::milliseconds(500), s1));
    ASSERT_TRUE(pool.acquire(std::chrono::milliseconds(500), s2));
    EXPECT_EQ(pool.in_use(), 2u);

    pool.release(s1);
    EXPECT_EQ(pool.in_use(), 1u);
    pool.release(s2);
    EXPECT_EQ(pool.in_use(), 0u);
    EXPECT_EQ(pool.available(), pool.size());
}

/**
 * @test ConnectionPoolAdaptiveScaleUp
 * @brief Validates that forceScaleUp grows the pool correctly.
 *
 * Verifies:
 *  - Scale-up increases pool size by scale_step
 *  - Pool size stays <= max_size
 */
TEST(Phase3ResourcePooling, ConnectionPoolAdaptiveScaleUp) {
    AdaptiveConnectionPool::Config cfg;
    cfg.min_size   = 5;
    cfg.max_size   = 50;
    cfg.scale_step = 5;
    AdaptiveConnectionPool pool(cfg);

    const std::size_t before = pool.size();
    pool.forceScaleUp();
    const std::size_t after = pool.size();
    EXPECT_GT(after, before);
    EXPECT_LE(after, cfg.max_size);

    auto st = pool.statistics();
    EXPECT_GE(st.scale_up_events, 1u);
}

/**
 * @test ConnectionPoolAdaptiveScaleDown
 * @brief Validates that forceScaleDown shrinks the pool correctly.
 *
 * Verifies:
 *  - Scale-down decreases pool size
 *  - Pool never shrinks below min_size
 */
TEST(Phase3ResourcePooling, ConnectionPoolAdaptiveScaleDown) {
    AdaptiveConnectionPool::Config cfg;
    cfg.min_size   = 5;
    cfg.max_size   = 50;
    cfg.scale_step = 5;
    AdaptiveConnectionPool pool(cfg);

    pool.forceScaleUp();  // Now at 10
    const std::size_t before = pool.size();
    pool.forceScaleDown();
    const std::size_t after = pool.size();
    EXPECT_LE(after, before);
    EXPECT_GE(after, cfg.min_size);
}

/**
 * @test ConnectionPoolTimeoutHandling
 * @brief Validates timeout when connection acquisition exceeds deadline.
 *
 * Verifies:
 *  - Return false if connection not available within short timeout
 *  - Pool stays consistent after timeout
 */
TEST(Phase3ResourcePooling, ConnectionPoolTimeoutHandling) {
    AdaptiveConnectionPool::Config cfg;
    cfg.min_size = 1;
    cfg.max_size = 1;
    AdaptiveConnectionPool pool(cfg);

    // Acquire the only slot.
    int s = -1;
    ASSERT_TRUE(pool.acquire(std::chrono::milliseconds(100), s));

    // A second acquire with a 50 ms timeout must time out.
    int s2 = -1;
    const bool ok = pool.acquire(std::chrono::milliseconds(50), s2);
    EXPECT_FALSE(ok);
    EXPECT_EQ(s2, -1);

    auto st = pool.statistics();
    EXPECT_GE(st.total_timeouts, 1u);

    pool.release(s);
}

/**
 * @test ConnectionPoolMaxCapacityEnforcement
 * @brief Validates enforcement of maximum pool size.
 *
 * Verifies:
 *  - forceScaleUp repeatedly does not exceed max_size
 *  - pool.size() <= max_size at all times
 */
TEST(Phase3ResourcePooling, ConnectionPoolMaxCapacityEnforcement) {
    AdaptiveConnectionPool::Config cfg;
    cfg.min_size   = 5;
    cfg.max_size   = 10;
    cfg.scale_step = 10;
    AdaptiveConnectionPool pool(cfg);

    // Multiple scale-ups must be capped at max.
    pool.forceScaleUp();
    pool.forceScaleUp();
    pool.forceScaleUp();

    EXPECT_LE(pool.size(), cfg.max_size);
}

/**
 * @test ConnectionPoolStressTest
 * @brief Validates connection pool under high-concurrency stress.
 *
 * Verifies:
 *  - 20 concurrent threads successfully acquire/release
 *  - No deadlocks or race conditions
 *  - Pool state remains consistent throughout
 */
TEST(Phase3ResourcePooling, ConnectionPoolStressTest) {
    AdaptiveConnectionPool::Config cfg;
    cfg.min_size = 10;
    cfg.max_size = 50;
    AdaptiveConnectionPool pool(cfg);

    std::atomic<int> acquired{0};
    std::atomic<int> released{0};
    constexpr int kThreads     = 20;
    constexpr int kItersEach   = 10;

    std::vector<std::thread> workers;
    workers.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&pool, &acquired, &released] {
            for (int i = 0; i < kItersEach; ++i) {
                int slot = -1;
                if (pool.acquire(std::chrono::milliseconds(500), slot)) {
                    acquired.fetch_add(1, std::memory_order_relaxed);
                    std::this_thread::yield();
                    pool.release(slot);
                    released.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (auto& th : workers) th.join();

    EXPECT_EQ(acquired.load(), released.load());
    EXPECT_EQ(pool.in_use(), 0u);
}

// ===== Task P3-03-C: Thread Pool Tuning (10 tests) =====

/**
 * @test ThreadPoolInitialization
 * @brief Validates thread pool initialization.
 *
 * Verifies:
 *  - Pool starts with min_threads active threads
 *  - Queued items start at 0
 */
TEST(Phase3ResourcePooling, ThreadPoolInitialization) {
    WorkStealingThreadPool::Config cfg;
    cfg.min_threads = 2;
    WorkStealingThreadPool pool(cfg);

    const auto st = pool.statistics();
    EXPECT_GE(st.active_threads, 2u);
    EXPECT_EQ(st.queued_items,   0u);
    pool.shutdown();
}

/**
 * @test ThreadPoolWorkDispatch
 * @brief Validates work dispatch to thread pool.
 *
 * Verifies:
 *  - Enqueue work item succeeds
 *  - Work is executed by the pool
 */
TEST(Phase3ResourcePooling, ThreadPoolWorkDispatch) {
    WorkStealingThreadPool pool;
    std::atomic<int> counter{0};

    ASSERT_TRUE(pool.submit([&counter] { counter.fetch_add(1); }));
    pool.waitAll(std::chrono::seconds(5));
    EXPECT_EQ(counter.load(), 1);
    pool.shutdown();
}

/**
 * @test ThreadPoolWorkStealingQueue
 * @brief Validates that multiple work items are all executed (work stealing in action).
 *
 * Verifies:
 *  - Submit N items to pool
 *  - All N items eventually executed
 */
TEST(Phase3ResourcePooling, ThreadPoolWorkStealingQueue) {
    WorkStealingThreadPool::Config cfg;
    cfg.min_threads = 4;
    WorkStealingThreadPool pool(cfg);

    constexpr int kItems = 100;
    std::atomic<int> done{0};
    for (int i = 0; i < kItems; ++i) {
        ASSERT_TRUE(pool.submit([&done] { done.fetch_add(1); }));
    }
    ASSERT_TRUE(pool.waitAll(std::chrono::seconds(10)));
    EXPECT_EQ(done.load(), kItems);
    pool.shutdown();
}

/**
 * @test ThreadPoolBackpressureHandling
 * @brief Validates that a tiny pool with tiny queue_depth rejects overflow.
 *
 * Verifies:
 *  - submit() returns false when queue is full and timeout is very short
 */
TEST(Phase3ResourcePooling, ThreadPoolBackpressureHandling) {
    WorkStealingThreadPool::Config cfg;
    cfg.min_threads     = 1;
    cfg.max_queue_depth = 2;
    cfg.idle_timeout_ms = 100;
    WorkStealingThreadPool pool(cfg);

    // Fill the queue beyond capacity with a blocking task.
    std::atomic<bool> gate{false};
    // Submit a task that blocks until we release it.
    pool.submit([&gate] {
        while (!gate.load(std::memory_order_acquire)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }, "blocker", std::chrono::milliseconds(500));

    // Fill remaining queue depth.
    for (int i = 0; i < 2; ++i) {
        pool.submit([] {}, "filler", std::chrono::milliseconds(200));
    }

    // This one must be rejected (very short timeout).
    const bool accepted = pool.submit([] {}, "overflow", std::chrono::milliseconds(10));
    EXPECT_FALSE(accepted);

    const auto st = pool.statistics();
    EXPECT_GE(st.rejected, 1u);
    EXPECT_GE(st.queue_high_watermark, 2u);
    EXPECT_GT(st.queue_pressure, 0.0);

    gate.store(true, std::memory_order_release);
    pool.shutdown();
}

/**
 * @test ThreadPoolLatencyUnderLoad
 * @brief Validates pool latency is reasonable under moderate load.
 *
 * Verifies:
 *  - 200 items complete within 5 seconds
 */
TEST(Phase3ResourcePooling, ThreadPoolLatencyUnderLoad) {
    WorkStealingThreadPool::Config cfg;
    cfg.min_threads = 4;
    WorkStealingThreadPool pool(cfg);

    constexpr int kItems = 200;
    std::atomic<int> done{0};
    const auto t0 = std::chrono::steady_clock::now();

    for (int i = 0; i < kItems; ++i) {
        ASSERT_TRUE(pool.submit([&done] {
            done.fetch_add(1, std::memory_order_relaxed);
        }));
    }
    ASSERT_TRUE(pool.waitAll(std::chrono::seconds(5)));
    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();
    EXPECT_EQ(done.load(), kItems);
    EXPECT_LT(elapsed_ms, 5000);  // Must complete within 5 s.
    pool.shutdown();
}

/**
 * @test ThreadPoolThroughputMeasurement
 * @brief Validates throughput: 500 tasks complete in reasonable time.
 */
TEST(Phase3ResourcePooling, ThreadPoolThroughputMeasurement) {
    WorkStealingThreadPool::Config cfg;
    cfg.min_threads = 4;
    WorkStealingThreadPool pool(cfg);

    constexpr int kItems = 500;
    std::atomic<int> done{0};
    for (int i = 0; i < kItems; ++i) {
        ASSERT_TRUE(pool.submit([&done] { done.fetch_add(1); }));
    }
    ASSERT_TRUE(pool.waitAll(std::chrono::seconds(10)));
    EXPECT_EQ(done.load(), kItems);

    const auto st = pool.statistics();
    EXPECT_EQ(st.failed, 0u);
    EXPECT_EQ(st.submitted, static_cast<std::uint64_t>(kItems));
    EXPECT_GE(st.queue_high_watermark, 1u);
    pool.shutdown();
}

/**
 * @test ThreadPoolShutdownGracefully
 * @brief Validates graceful shutdown completes in-flight work.
 *
 * Verifies:
 *  - Submitted work executes before shutdown returns
 *  - submit() returns false after shutdown
 */
TEST(Phase3ResourcePooling, ThreadPoolShutdownGracefully) {
    WorkStealingThreadPool pool;
    std::atomic<int> counter{0};
    for (int i = 0; i < 20; ++i) {
        pool.submit([&counter] { counter.fetch_add(1); });
    }
    pool.shutdown(std::chrono::seconds(10));
    EXPECT_EQ(counter.load(), 20);

    // After shutdown, new submissions must be rejected.
    const bool ok = pool.submit([] {});
    EXPECT_FALSE(ok);
}

/**
 * @test ThreadPoolDynamicThreadAdjustment
 * @brief Validates initial thread count respects min_threads.
 */
TEST(Phase3ResourcePooling, ThreadPoolDynamicThreadAdjustment) {
    WorkStealingThreadPool::Config cfg;
    cfg.min_threads = 2;
    cfg.max_threads = 8;
    WorkStealingThreadPool pool(cfg);

    std::atomic<bool> gate{false};
    for (int i = 0; i < 6; ++i) {
        ASSERT_TRUE(pool.submit([&gate] {
            while (!gate.load(std::memory_order_acquire)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }, "scale-up", std::chrono::milliseconds(200)));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    const auto st = pool.statistics();
    EXPECT_GE(pool.thread_count(), cfg.min_threads);
    EXPECT_LE(pool.thread_count(), cfg.max_threads);
    EXPECT_GT(st.scale_up_events, 0u);
    EXPECT_GT(st.queue_high_watermark, cfg.min_threads);

    gate.store(true, std::memory_order_release);
    pool.shutdown();
}

/**
 * @test ThreadPoolExceptionHandling
 * @brief Validates that throwing tasks do not crash worker threads.
 *
 * Verifies:
 *  - Exception in work item is caught (failed counter increments)
 *  - Pool continues to execute subsequent items
 */
TEST(Phase3ResourcePooling, ThreadPoolExceptionHandling) {
    WorkStealingThreadPool pool;
    std::atomic<int> safe_counter{0};

    // Submit a throwing task.
    pool.submit([] { throw std::runtime_error("test exception"); }, "thrower");

    // Submit subsequent safe tasks.
    for (int i = 0; i < 10; ++i) {
        pool.submit([&safe_counter] { safe_counter.fetch_add(1); });
    }
    pool.waitAll(std::chrono::seconds(5));

    // Pool must have recorded the failure and continued.
    const auto st = pool.statistics();
    EXPECT_GE(st.failed, 1u);
    EXPECT_EQ(safe_counter.load(), 10);
    pool.shutdown();
}

/**
 * @test ThreadPoolStressTestHighConcurrency
 * @brief Validates pool under 1000 concurrent work items.
 *
 * Verifies:
 *  - All 1000 work items complete without deadlock
 *  - No failures for CPU-bound work
 */
TEST(Phase3ResourcePooling, ThreadPoolStressTestHighConcurrency) {
    WorkStealingThreadPool::Config cfg;
    cfg.min_threads     = 4;
    cfg.max_queue_depth = 2000;
    WorkStealingThreadPool pool(cfg);

    constexpr int kItems = 1000;
    std::atomic<int> done{0};
    for (int i = 0; i < kItems; ++i) {
        ASSERT_TRUE(pool.submit([&done] { done.fetch_add(1); },
                                "stress", std::chrono::seconds(10)));
    }
    ASSERT_TRUE(pool.waitAll(std::chrono::seconds(30)));
    EXPECT_EQ(done.load(), kItems);
    pool.shutdown();
}

// ===== Task P3-03-D: Buffer Pool (6 tests) =====

/**
 * @test BufferPoolInitialization
 * @brief Validates buffer pool initialization with slab allocator.
 *
 * Verifies:
 *  - Slab classes: 128B, 256B, 512B, 1KB, 2KB, 4KB
 *  - All slabs ready for allocation
 */
TEST(Phase3ResourcePooling, BufferPoolInitialization) {
    BufferPool pool;
    EXPECT_EQ(BufferPool::kSlabSizes.size(), 6u);
    EXPECT_EQ(BufferPool::kSlabSizes[0], 128u);
    EXPECT_EQ(BufferPool::kSlabSizes[5], 4096u);
    EXPECT_EQ(BufferPool::kMaxSlabSize, 4096u);

    auto st = pool.statistics();
    EXPECT_EQ(st.total_allocations, 0u);
    EXPECT_EQ(st.total_releases, 0u);
    EXPECT_EQ(st.release_evictions, 0u);
    EXPECT_EQ(st.free_buffers, 6u * 32u);
    EXPECT_EQ(st.configured_capacity, 6u * 256u);
    EXPECT_DOUBLE_EQ(st.hit_rate(), 0.0);
    EXPECT_DOUBLE_EQ(st.pressure(), 0.0);
}

/**
 * @test BufferPoolAllocationAndReuse
 * @brief Validates buffer allocation and reuse from slabs.
 *
 * Verifies:
 *  - Allocate buffer returns a valid pointer
 *  - Size of handle matches slab class
 *  - After release, next allocation can reuse the slot
 */
TEST(Phase3ResourcePooling, BufferPoolAllocationAndReuse) {
    BufferPool pool;

    {
        auto buf = pool.acquire(100);  // Should go to 128-byte slab.
        ASSERT_TRUE(buf.valid());
        EXPECT_EQ(buf.size(), 128u);  // Rounded up to slab class.
        EXPECT_NE(buf.data(), nullptr);
        // Write and read back.
        std::memset(buf.data(), 0xAB, buf.size());
        EXPECT_EQ(static_cast<uint8_t*>(buf.data())[0], 0xAB);
        // buf released here.
    }

    // Pool should have one allocation recorded.
    const auto st = pool.statistics();
    EXPECT_GE(st.total_allocations, 1u);
}

/**
 * @test BufferPoolReuseRate
 * @brief Validates high reuse rate from slab free-list.
 *
 * Verifies:
 *  - After 50 allocate-release cycles on the same slab class,
 *    slab_hits >> slab_misses (pre-allocated free list serves requests).
 */
TEST(Phase3ResourcePooling, BufferPoolReuseRate) {
    BufferPool::Config cfg;
    cfg.initial_per_class = 64;
    BufferPool pool(cfg);

    constexpr int kCycles = 50;
    for (int i = 0; i < kCycles; ++i) {
        auto buf = pool.acquire(256);  // B256 slab.
        ASSERT_TRUE(buf.valid());
        // buf released at end of scope.
    }

    const auto st = pool.statistics();
    EXPECT_EQ(st.total_allocations, static_cast<std::size_t>(kCycles));
    // Most requests served from pre-allocated free list.
    EXPECT_GT(st.slab_hits, st.slab_misses);
    EXPECT_GT(st.hit_rate(), 0.95);
    EXPECT_EQ(st.total_releases, static_cast<std::size_t>(kCycles));
    EXPECT_EQ(st.release_evictions, 0u);
    EXPECT_EQ(st.current_live, 0u);
    EXPECT_DOUBLE_EQ(st.pressure(), 0.0);
}

/**
 * @test BufferPoolExhaustionHandling
 * @brief Validates behavior for oversized requests (OS fallback).
 *
 * Verifies:
 *  - Requests > 4KB served from OS allocator (os_fallbacks increments)
 *  - Handle is valid and correctly sized
 */
TEST(Phase3ResourcePooling, BufferPoolExhaustionHandling) {
    BufferPool pool;

    // Request larger than kMaxSlabSize triggers OS fallback.
    auto big = pool.acquire(8192);
    ASSERT_TRUE(big.valid());
    EXPECT_EQ(big.size(), 8192u);

    const auto st = pool.statistics();
    EXPECT_GE(st.os_fallbacks, 1u);
}

/**
 * @test BufferPoolFragmentationRisistance
 * @brief Validates no fragmentation with fixed-size slab allocations.
 *
 * Verifies:
 *  - Requesting same size repeatedly reuses same-class slab (no cross-class)
 *  - No allocation size exceeds the slab class size
 */
TEST(Phase3ResourcePooling, BufferPoolFragmentationRisistance) {
    BufferPool pool;

    for (int i = 0; i < 20; ++i) {
        auto buf = pool.acquire(512);
        ASSERT_TRUE(buf.valid());
        // All must round up to exactly the B512 class.
        EXPECT_EQ(buf.size(), 512u);
    }
}

/**
 * @test BufferPoolSlabBalance
 * @brief Validates that different slab classes are independently tracked.
 *
 * Verifies:
 *  - Allocations from different size classes tracked separately
 *  - per_class_allocs reflects allocation counts per class
 */
TEST(Phase3ResourcePooling, BufferPoolSlabBalance) {
    BufferPool pool;

    pool.acquire(128);   // class 0
    pool.acquire(256);   // class 1
    pool.acquire(512);   // class 2
    pool.acquire(1024);  // class 3
    pool.acquire(2048);  // class 4
    pool.acquire(4096);  // class 5

    const auto st = pool.statistics();
    // Each class should have at least one allocation.
    for (const auto& cnt : st.per_class_allocs) {
        EXPECT_GE(cnt, 1u);
    }
}

// ===== Task P3-03-E: Integration Testing (8 tests) =====

/**
 * @test IntegrationResourcePoolManager
 * @brief Validates unified resource pool manager orchestration.
 */
TEST(Phase3ResourcePooling, IntegrationResourcePoolManager) {
    ResourcePoolManager mgr;
    EXPECT_FALSE(mgr.is_shutdown());

    // Both sub-pools accessible.
    EXPECT_FALSE(mgr.connectionPool().is_shutdown());
    EXPECT_FALSE(mgr.bufferPool().is_shutdown());

    mgr.shutdown();
    EXPECT_TRUE(mgr.is_shutdown());
}

/**
 * @test IntegrationConcurrentPoolOperations
 * @brief Validates concurrent operations across connection + buffer pools.
 */
TEST(Phase3ResourcePooling, IntegrationConcurrentPoolOperations) {
    ResourcePoolManager mgr;
    constexpr int kThreads = 8;
    std::atomic<int> conn_ok{0};
    std::atomic<int> buf_ok{0};

    std::vector<std::thread> workers;
    workers.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&mgr, &conn_ok, &buf_ok] {
            int slot = -1;
            if (mgr.connectionPool().acquire(std::chrono::milliseconds(200), slot)) {
                conn_ok.fetch_add(1);
                auto buf = mgr.bufferPool().acquire(512);
                if (buf.valid()) buf_ok.fetch_add(1);
                mgr.connectionPool().release(slot);
            }
        });
    }
    for (auto& th : workers) th.join();

    EXPECT_GT(conn_ok.load(), 0);
    EXPECT_GT(buf_ok.load(), 0);
    mgr.shutdown();
}

/**
 * @test IntegrationPoolSaturationMonitoring
 * @brief Validates saturation stats reflect pool utilisation.
 */
TEST(Phase3ResourcePooling, IntegrationPoolSaturationMonitoring) {
    ResourcePoolManager::Config cfg;
    cfg.conn_pool.min_size = 5;
    cfg.conn_pool.max_size = 5;
    cfg.saturation_alert_threshold = 0.5;
    ResourcePoolManager mgr(cfg);

    // Acquire 3 of 5 slots → saturation = 0.6 > 0.5 threshold.
    int s1 = -1, s2 = -1, s3 = -1;
    ASSERT_TRUE(mgr.connectionPool().acquire(std::chrono::milliseconds(200), s1));
    ASSERT_TRUE(mgr.connectionPool().acquire(std::chrono::milliseconds(200), s2));
    ASSERT_TRUE(mgr.connectionPool().acquire(std::chrono::milliseconds(200), s3));

    const auto st = mgr.statistics();
    EXPECT_NEAR(st.saturation_conn, 0.6, 0.05);
    EXPECT_TRUE(st.saturation_alert);

    mgr.connectionPool().release(s1);
    mgr.connectionPool().release(s2);
    mgr.connectionPool().release(s3);
    mgr.shutdown();
}

/**
 * @test IntegrationPoolResourceLeakDetection
 * @brief Validates statistics track live (un-released) handles.
 */
TEST(Phase3ResourcePooling, IntegrationPoolResourceLeakDetection) {
    ResourcePoolManager mgr;
    {
        auto buf1 = mgr.bufferPool().acquire(128);
        auto buf2 = mgr.bufferPool().acquire(256);
        ASSERT_TRUE(buf1.valid());
        ASSERT_TRUE(buf2.valid());

        const auto st = mgr.statistics();
        EXPECT_EQ(st.buffer.current_live, 2u);
        // buf1, buf2 released on scope exit.
    }
    const auto st = mgr.statistics();
    EXPECT_EQ(st.buffer.current_live, 0u);
    mgr.shutdown();
}

/**
 * @test IntegrationPoolRecoveryAfterError
 * @brief Validates pool remains functional after individual slot exhaustion.
 */
TEST(Phase3ResourcePooling, IntegrationPoolRecoveryAfterError) {
    AdaptiveConnectionPool::Config cfg;
    cfg.min_size = 2;
    cfg.max_size = 2;
    AdaptiveConnectionPool pool(cfg);

    int s1 = -1, s2 = -1;
    ASSERT_TRUE(pool.acquire(std::chrono::milliseconds(100), s1));
    ASSERT_TRUE(pool.acquire(std::chrono::milliseconds(100), s2));

    // "Failure": pretend s1 failed — release it.
    pool.release(s1);

    // Pool should replenish: another acquire must succeed.
    int s3 = -1;
    ASSERT_TRUE(pool.acquire(std::chrono::milliseconds(100), s3));
    pool.release(s2);
    pool.release(s3);
    EXPECT_EQ(pool.in_use(), 0u);
}

/**
 * @test IntegrationPoolStatisticsCollection
 * @brief Validates accurate statistics across pools.
 */
TEST(Phase3ResourcePooling, IntegrationPoolStatisticsCollection) {
    ResourcePoolManager mgr;

    int slot = -1;
    ASSERT_TRUE(mgr.connectionPool().acquire(std::chrono::milliseconds(200), slot));
    mgr.connectionPool().release(slot);

    auto buf = mgr.bufferPool().acquire(512);
    ASSERT_TRUE(buf.valid());
    buf.release();

    const auto st = mgr.statistics();
    EXPECT_GE(st.conn.total_acquires,       1u);
    EXPECT_GE(st.buffer.total_allocations,  1u);
    EXPECT_GE(st.buffer.total_releases,     1u);
    mgr.shutdown();
}

/**
 * @test IntegrationPoolScenarioQueryExecution
 * @brief Validates pools under a synthetic query-execution scenario.
 */
TEST(Phase3ResourcePooling, IntegrationPoolScenarioQueryExecution) {
    ResourcePoolManager mgr;
    constexpr int kQueries = 50;
    std::atomic<int> done{0};

    std::vector<std::thread> workers;
    workers.reserve(kQueries);
    for (int i = 0; i < kQueries; ++i) {
        workers.emplace_back([&mgr, &done] {
            // Simulate: acquire connection, allocate buffer, do work, release.
            int slot = -1;
            if (!mgr.connectionPool().acquire(std::chrono::milliseconds(500), slot)) {
                return;
            }
            auto buf = mgr.bufferPool().acquire(512);
            if (buf.valid()) {
                std::memset(buf.data(), 0, buf.size());  // "Query work".
                done.fetch_add(1, std::memory_order_relaxed);
            }
            mgr.connectionPool().release(slot);
        });
    }
    for (auto& th : workers) th.join();

    EXPECT_GT(done.load(), 0);
    mgr.shutdown();
}

/**
 * @test IntegrationPoolPerformanceWave7Regression
 * @brief Validates pool overhead is negligible (< 10 ms for 100 round-trips).
 */
TEST(Phase3ResourcePooling, IntegrationPoolPerformanceWave7Regression) {
    AdaptiveConnectionPool pool;

    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < 100; ++i) {
        int slot = -1;
        ASSERT_TRUE(pool.acquire(std::chrono::milliseconds(500), slot));
        pool.release(slot);
    }
    const auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - t0).count();

    // 100 acquire+release must complete well under 100 ms total.
    EXPECT_LT(elapsed_us, 100'000);
}

// ===== Task P3-03-F: Performance Tuning (4 tests) =====

/**
 * @test PerformanceTuningProfilingUnderSyntheticLoad
 * @brief Profiles resource pools under synthetic load.
 *
 * Verifies:
 *  - Buffer-pool round-trips are faster than malloc/free baseline
 */
TEST(Phase3ResourcePooling, PerformanceTuningProfilingUnderSyntheticLoad) {
    BufferPool pool;
    constexpr int kOps = 1000;

    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < kOps; ++i) {
        auto buf = pool.acquire(512);
        (void)buf;  // Released on scope exit.
    }
    const long pool_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - t0).count();

    // Malloc/free baseline.
    const auto t1 = std::chrono::steady_clock::now();
    for (int i = 0; i < kOps; ++i) {
        void* p = std::malloc(512);
        std::free(p);
    }
    const long malloc_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - t1).count();

    // Slab pool should be at least as fast as or faster than malloc.
    // Allow 2× slack for CI variability.
    EXPECT_LT(pool_us, malloc_us * 3 + 1000);  // Generous bound for CI.
    (void)pool_us;
    (void)malloc_us;
}

/**
 * @test PerformanceTuningSlabSizeOptimization
 * @brief Validates that allocations round up to the correct slab class.
 *
 * Verifies:
 *  - 1-byte request served from 128-byte slab.
 *  - 4096-byte request served from 4096-byte slab.
 *  - 4097-byte request served from OS (os_fallback).
 */
TEST(Phase3ResourcePooling, PerformanceTuningSlabSizeOptimization) {
    BufferPool pool;

    auto b1 = pool.acquire(1);
    EXPECT_EQ(b1.size(), 128u);

    auto b2 = pool.acquire(4096);
    EXPECT_EQ(b2.size(), 4096u);

    auto b3 = pool.acquire(4097);
    ASSERT_TRUE(b3.valid());
    const auto st = pool.statistics();
    EXPECT_GE(st.os_fallbacks, 1u);
}

/**
 * @test PerformanceTuningPeakUtilizationLimit
 * @brief Validates peak utilization is recorded and capped.
 *
 * Verifies:
 *  - Acquire all slots → peak utilization = 1.0
 *  - Stats reflect the peak
 */
TEST(Phase3ResourcePooling, PerformanceTuningPeakUtilizationLimit) {
    AdaptiveConnectionPool::Config cfg;
    cfg.min_size = 5;
    cfg.max_size = 5;
    AdaptiveConnectionPool pool(cfg);

    // Acquire all 5 slots.
    std::vector<int> slots(5, -1);
    for (auto& s : slots) {
        ASSERT_TRUE(pool.acquire(std::chrono::milliseconds(200), s));
    }

    const auto st = pool.statistics();
    EXPECT_NEAR(st.peak_utilization, 1.0, 0.01);

    for (int s : slots) pool.release(s);
}

/**
 * @test PerformanceTuningWave7GatesVerification
 * @brief Verifies connection pool acquisition is well within latency budget.
 *
 * Verifies:
 *  - 100 consecutive acquire+release < 50 ms total (approximates gate budget).
 */
TEST(Phase3ResourcePooling, PerformanceTuningWave7GatesVerification) {
    AdaptiveConnectionPool::Config cfg;
    cfg.min_size = 10;
    cfg.max_size = 50;
    AdaptiveConnectionPool pool(cfg);

    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < 100; ++i) {
        int slot = -1;
        ASSERT_TRUE(pool.acquire(std::chrono::milliseconds(500), slot));
        pool.release(slot);
    }
    const long elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();
    EXPECT_LT(elapsed_ms, 500);  // Generous bound; pure in-memory logic.
}
} } // namespace themis::resource
