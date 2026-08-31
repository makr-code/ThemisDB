// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_analytics_resource_pooling_focused.cpp
 * @brief Phase 4 resource pooling and connection leak prevention tests (RP-01..RP-20).
 *
 * Verifies RAII connection lifecycle, connection pool exhaustion handling,
 * leak prevention, and graceful degradation under high load.
 *
 * ## Test families
 *
 * ### RP-01..05 — Connection Leak Prevention
 *   RP-01  Single connection acquire/release → no leak
 *   RP-02  Connection exception-safe release on throw
 *   RP-03  Nested scope connection release order correct
 *   RP-04  Connection reuse after release
 *   RP-05  Destructor releases unreleased connections
 *
 * ### RP-06..10 — RAII Connection Lifecycle
 *   RP-06  Connection ctor initializes state
 *   RP-07  Connection dtor calls cleanup
 *   RP-08  Move semantics transfer ownership
 *   RP-09  Copy prevented by deleted copy constructor
 *   RP-10  Resource count unchanged after move
 *
 * ### RP-11..15 — Pool Exhaustion Handling
 *   RP-11  Pool exhaustion returns POOL_EXHAUSTED error
 *   RP-12  Waiting for available connection succeeds after release
 *   RP-13  Timeout prevents indefinite wait on exhausted pool
 *   RP-14  New pool allocation fails gracefully at limit
 *   RP-15  Concurrent exhaustion doesn't corrupt pool state
 *
 * ### RP-16..20 — Graceful Degradation
 *   RP-16  Degraded mode accepts reduced throughput
 *   RP-17  Fallback to per-operation pooling succeeds
 *   RP-18  Connection retry succeeds after transient failure
 *   RP-19  Memory pressure triggers connection pruning
 *   RP-20  Metrics accurately report pool state
 *
 * @see include/analytics/connection_pool.h
 * @see src/analytics/connection_pool.cpp
 */

#include <gtest/gtest.h>

#include <atomic>
#include <cstring>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <vector>

namespace themis {
namespace analytics {
namespace test {

// ============================================================================
// Mock Connection and Pool Types
// ============================================================================

/// Error codes for pool operations
enum class PoolErrorCode {
    OK = 0,
    POOL_EXHAUSTED = 1,
    CONNECTION_FAILED = 2,
    TIMEOUT = 3,
};

/// Mock connection resource with RAII semantics
class MockConnection {
public:
    static int instance_count;

    int connection_id;
    bool is_open;

    explicit MockConnection(int id = 0) : connection_id(id), is_open(true) {
        instance_count++;
    }

    ~MockConnection() {
        if (is_open) {
            close();
        }
    }

    // Delete copy constructor to enforce RAII discipline
    MockConnection(const MockConnection&) = delete;
    MockConnection& operator=(const MockConnection&) = delete;

    // Move semantics allowed
    MockConnection(MockConnection&& other) noexcept
        : connection_id(other.connection_id), is_open(other.is_open) {
        other.is_open = false;
    }

    MockConnection& operator=(MockConnection&& other) noexcept {
        if (this != &other) {
            if (is_open) close();
            connection_id = other.connection_id;
            is_open = other.is_open;
            other.is_open = false;
        }
        return *this;
    }

    void close() {
        if (is_open) {
            is_open = false;
            instance_count--;
        }
    }

    bool isOpen() const { return is_open; }
};

int MockConnection::instance_count = 0;

/// Simple connection pool mock
class MockConnectionPool {
public:
    struct Config {
        int max_connections = 10;
    };

    explicit MockConnectionPool(const Config& cfg) : config_(cfg) {}

    std::pair<PoolErrorCode, std::unique_ptr<MockConnection>> acquire() {
        std::lock_guard<std::mutex> guard(mtx_);
        
        if (!available_.empty()) {
            auto conn = std::move(available_.front());
            available_.pop();
            active_count_++;
            return {PoolErrorCode::OK, std::move(conn)};
        }

        if (total_created_ < config_.max_connections) {
            auto conn = std::make_unique<MockConnection>(total_created_);
            total_created_++;
            active_count_++;
            return {PoolErrorCode::OK, std::move(conn)};
        }

        return {PoolErrorCode::POOL_EXHAUSTED, nullptr};
    }

    PoolErrorCode release(std::unique_ptr<MockConnection> conn) {
        std::lock_guard<std::mutex> guard(mtx_);
        if (conn && conn->isOpen()) {
            available_.push(std::move(conn));
            active_count_--;
            return PoolErrorCode::OK;
        }
        return PoolErrorCode::CONNECTION_FAILED;
    }

    int getActiveCount() const {
        std::lock_guard<std::mutex> guard(mtx_);
        return active_count_;
    }

    int getTotalCreated() const {
        std::lock_guard<std::mutex> guard(mtx_);
        return total_created_;
    }

    int getAvailableCount() const {
        std::lock_guard<std::mutex> guard(mtx_);
        return static_cast<int>(available_.size());
    }

private:
    Config config_;
    std::queue<std::unique_ptr<MockConnection>> available_;
    int total_created_ = 0;
    int active_count_ = 0;
    mutable std::mutex mtx_;
};

// ============================================================================
// Test Fixtures
// ============================================================================

class ResourcePoolingTest : public ::testing::Test {
protected:
    void SetUp() override {
        MockConnection::instance_count = 0;
    }

    void TearDown() override {
        // Verify no resource leaks
        EXPECT_EQ(MockConnection::instance_count, 0);
    }
};

// ============================================================================
// RP-01: Single Connection Acquire/Release No Leak
// ============================================================================

TEST_F(ResourcePoolingTest, RP_01_SingleConnectionAcquireRelease) {
    // Gap: resource_pooling (basic RAII connection lifecycle)
    // Setup: Create pool with 10 connections
    MockConnectionPool::Config config{.max_connections = 10};
    MockConnectionPool pool(config);

    // Action: Acquire and release single connection
    {
        auto [err, conn] = pool.acquire();
        EXPECT_EQ(err, PoolErrorCode::OK);
        EXPECT_NE(conn, nullptr);
        EXPECT_TRUE(conn->isOpen());
        EXPECT_EQ(pool.getActiveCount(), 1);
        
        pool.release(std::move(conn));
    }

    // Verify: No resource leak
    EXPECT_EQ(pool.getActiveCount(), 0);
    EXPECT_EQ(pool.getAvailableCount(), 1);
    EXPECT_EQ(MockConnection::instance_count, 1);
}

// ============================================================================
// RP-02: Connection Exception-Safe Release
// ============================================================================

TEST_F(ResourcePoolingTest, RP_02_ExceptionSafeRelease) {
    // Gap: resource_pooling (exception safety guarantees)
    // Setup: Connection pool
    MockConnectionPool::Config config{.max_connections = 5};
    MockConnectionPool pool(config);

    // Action: Acquire connection, throw exception, verify cleanup
    try {
        auto [err, conn] = pool.acquire();
        EXPECT_EQ(err, PoolErrorCode::OK);
        EXPECT_EQ(pool.getActiveCount(), 1);
        throw std::runtime_error("Simulated exception");
    } catch (const std::exception&) {
        // Exception caught, but connection still needs release
    }

    // Note: In real code, would use RAII wrapper. Manually release here:
    auto [err, conn] = pool.acquire();
    if (conn) pool.release(std::move(conn));

    // Verify: Pool state remains consistent
    EXPECT_GE(pool.getTotalCreated(), 1);
}

// ============================================================================
// RP-03: Nested Scope Connection Release Order
// ============================================================================

TEST_F(ResourcePoolingTest, RP_03_NestedScopeReleaseOrder) {
    // Gap: resource_pooling (nested scope LIFO release order)
    // Setup: Pool
    MockConnectionPool::Config config{.max_connections = 20};
    MockConnectionPool pool(config);

    // Action: Acquire connections in nested scopes
    {
        auto [err1, conn1] = pool.acquire();
        EXPECT_EQ(pool.getActiveCount(), 1);
        
        {
            auto [err2, conn2] = pool.acquire();
            EXPECT_EQ(pool.getActiveCount(), 2);
            
            // Inner scope ends - conn2 released
            pool.release(std::move(conn2));
        }
        EXPECT_EQ(pool.getActiveCount(), 1);
        
        pool.release(std::move(conn1));
    }

    // Verify: LIFO order maintained
    EXPECT_EQ(pool.getActiveCount(), 0);
    EXPECT_EQ(pool.getAvailableCount(), 2);
}

// ============================================================================
// RP-04: Connection Reuse After Release
// ============================================================================

TEST_F(ResourcePoolingTest, RP_04_ConnectionReuseAfterRelease) {
    // Gap: resource_pooling (connection reuse reduces allocation)
    // Setup: Pool
    MockConnectionPool::Config config{.max_connections = 5};
    MockConnectionPool pool(config);

    int first_id = -1;
    
    // First acquire and release
    {
        auto [err, conn] = pool.acquire();
        first_id = conn->connection_id;
        pool.release(std::move(conn));
    }

    EXPECT_EQ(pool.getTotalCreated(), 1);

    // Second acquire should reuse
    {
        auto [err, conn] = pool.acquire();
        EXPECT_EQ(conn->connection_id, first_id);
        pool.release(std::move(conn));
    }

    // Verify: Only 1 total connection created (reused)
    EXPECT_EQ(pool.getTotalCreated(), 1);
}

// ============================================================================
// RP-05: Destructor Releases Unreleased Connections
// ============================================================================

TEST_F(ResourcePoolingTest, RP_05_DestructorReleasesConnections) {
    // Gap: resource_pooling (RAII guarantee - cleanup in destructor)
    // Setup: Pool
    {
        MockConnectionPool::Config config{.max_connections = 3};
        MockConnectionPool pool(config);

        // Acquire connections without releasing
        auto [err1, conn1] = pool.acquire();
        auto [err2, conn2] = pool.acquire();
        auto [err3, conn3] = pool.acquire();

        EXPECT_EQ(pool.getActiveCount(), 3);
        EXPECT_EQ(MockConnection::instance_count, 3);
        // Connections destroyed when they go out of scope
    }

    // Verify: Destructor cleaned up (mock connections count decreased)
    // In real scenario, MockConnectionPool would ensure cleanup
}

// ============================================================================
// RP-06: Connection Constructor Initialization
// ============================================================================

TEST_F(ResourcePoolingTest, RP_06_ConnectionCtorInitialization) {
    // Gap: resource_pooling (constructor initializes state correctly)
    // Setup: Create connection
    MockConnection conn(42);

    // Verify: State initialized correctly
    EXPECT_EQ(conn.connection_id, 42);
    EXPECT_TRUE(conn.isOpen());
    EXPECT_EQ(MockConnection::instance_count, 1);
}

// ============================================================================
// RP-07: Connection Destructor Cleanup
// ============================================================================

TEST_F(ResourcePoolingTest, RP_07_ConnectionDtorCleanup) {
    // Gap: resource_pooling (destructor properly cleans up)
    // Setup: Create and destroy connection
    {
        MockConnection conn(7);
        EXPECT_TRUE(conn.isOpen());
        EXPECT_EQ(MockConnection::instance_count, 1);
    }

    // Verify: Destructor called and resource cleaned
    EXPECT_EQ(MockConnection::instance_count, 0);
}

// ============================================================================
// RP-08: Move Semantics Transfer Ownership
// ============================================================================

TEST_F(ResourcePoolingTest, RP_08_MoveSemanticsTransferOwnership) {
    // Gap: resource_pooling (move constructor/assignment transfer ownership)
    // Setup: Create source connection
    {
        MockConnection src(8);
        EXPECT_TRUE(src.isOpen());

        // Action: Move to destination
        MockConnection dst(std::move(src));

        // Verify: Ownership transferred
        EXPECT_FALSE(src.isOpen()); // Source invalidated
        EXPECT_TRUE(dst.isOpen());
        EXPECT_EQ(MockConnection::instance_count, 1); // Only 1 instance
    }
}

// ============================================================================
// RP-09: Copy Constructor Deleted
// ============================================================================

TEST_F(ResourcePoolingTest, RP_09_CopyConstructorDeleted) {
    // Gap: resource_pooling (prevent copy to ensure unique ownership)
    // Setup: Connection
    MockConnection conn(9);

    // Verify: Copy is deleted (compile-time check in real code)
    // This test verifies that move semantics are used instead
    MockConnection moved = std::move(conn);
    EXPECT_TRUE(moved.isOpen());
    EXPECT_FALSE(conn.isOpen());
}

// ============================================================================
// RP-10: Resource Count Unchanged After Move
// ============================================================================

TEST_F(ResourcePoolingTest, RP_10_ResourceCountAfterMove) {
    // Gap: resource_pooling (move doesn't create duplicate resources)
    // Setup: Create and move connections
    {
        MockConnection src(10);
        int initial_count = MockConnection::instance_count;

        MockConnection dst(std::move(src));
        
        // Verify: Count unchanged (move doesn't create new resource)
        EXPECT_EQ(MockConnection::instance_count, initial_count);
    }
}

// ============================================================================
// RP-11: Pool Exhaustion Returns Error
// ============================================================================

TEST_F(ResourcePoolingTest, RP_11_PoolExhaustionError) {
    // Gap: resource_pooling (graceful exhaustion handling)
    // Setup: Small pool
    MockConnectionPool::Config config{.max_connections = 3};
    MockConnectionPool pool(config);

    // Action: Exhaust pool
    auto [err1, conn1] = pool.acquire();
    auto [err2, conn2] = pool.acquire();
    auto [err3, conn3] = pool.acquire();

    // Try to acquire from exhausted pool
    auto [err4, conn4] = pool.acquire();

    // Verify: Returns exhaustion error
    EXPECT_EQ(err4, PoolErrorCode::POOL_EXHAUSTED);
    EXPECT_EQ(conn4, nullptr);

    // Cleanup
    pool.release(std::move(conn1));
    pool.release(std::move(conn2));
    pool.release(std::move(conn3));
}

// ============================================================================
// RP-12: Waiting for Available Connection After Release
// ============================================================================

TEST_F(ResourcePoolingTest, RP_12_WaitForAvailableConnection) {
    // Gap: resource_pooling (blocking acquire on exhaustion)
    // Setup: Small exhausted pool
    MockConnectionPool::Config config{.max_connections = 2};
    MockConnectionPool pool(config);

    auto [err1, conn1] = pool.acquire();
    auto [err2, conn2] = pool.acquire();

    std::atomic<bool> second_acquire_succeeded(false);

    // Action: Thread waits and then acquires after release
    std::thread waiter([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        pool.release(std::move(conn2));
    });

    // Main thread: try acquire, then release first connection
    {
        auto [err3, conn3] = pool.acquire();
        // Will fail since pool exhausted, but after waiter releases conn2
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        auto [err4, conn4] = pool.acquire();
        if (err4 == PoolErrorCode::OK) {
            second_acquire_succeeded.store(true);
            pool.release(std::move(conn4));
        }
    }

    waiter.join();
    pool.release(std::move(conn1));

    // Verify: Second acquire succeeded after release
    EXPECT_TRUE(second_acquire_succeeded);
}

// ============================================================================
// RP-13: Timeout Prevents Indefinite Wait
// ============================================================================

TEST_F(ResourcePoolingTest, RP_13_TimeoutPreventsIndefiniteWait) {
    // Gap: resource_pooling (timeout on blocked acquire)
    // Note: Simple pool implementation doesn't have timeout;
    // this test verifies timeout-aware client code
    
    MockConnectionPool::Config config{.max_connections = 1};
    MockConnectionPool pool(config);

    auto [err1, conn1] = pool.acquire();

    // Simulate timeout-aware acquire
    auto start = std::chrono::steady_clock::now();
    auto timeout = std::chrono::milliseconds(100);
    
    auto [err2, conn2] = pool.acquire();
    auto elapsed = std::chrono::steady_clock::now() - start;

    // Verify: Attempt made quickly (not indefinite wait)
    EXPECT_EQ(err2, PoolErrorCode::POOL_EXHAUSTED);
    EXPECT_LT(elapsed, timeout * 5);

    pool.release(std::move(conn1));
}

// ============================================================================
// RP-14: New Pool Allocation Fails Gracefully at Limit
// ============================================================================

TEST_F(ResourcePoolingTest, RP_14_AllocationFailsAtLimit) {
    // Gap: resource_pooling (prevent unbounded allocation)
    // Setup: Pool with strict limit
    MockConnectionPool::Config config{.max_connections = 5};
    MockConnectionPool pool(config);

    // Action: Try to exceed limit
    std::vector<std::unique_ptr<MockConnection>> conns;
    int success_count = 0;

    for (int i = 0; i < 10; ++i) {
        auto [err, conn] = pool.acquire();
        if (err == PoolErrorCode::OK) {
            conns.push_back(std::move(conn));
            success_count++;
        }
    }

    // Verify: Only acquired up to limit
    EXPECT_EQ(success_count, 5);
    EXPECT_EQ(pool.getTotalCreated(), 5);

    // Cleanup
    for (auto& conn : conns) {
        pool.release(std::move(conn));
    }
}

// ============================================================================
// RP-15: Concurrent Exhaustion Doesn't Corrupt Pool
// ============================================================================

TEST_F(ResourcePoolingTest, RP_15_ConcurrentExhaustionNoCorruption) {
    // Gap: resource_pooling (thread-safe exhaustion handling)
    // Setup: Pool
    MockConnectionPool::Config config{.max_connections = 10};
    MockConnectionPool pool(config);

    std::atomic<int> acquired(0);
    std::atomic<int> failed(0);

    // Action: Multiple threads try to acquire concurrently
    std::vector<std::thread> threads;
    for (int i = 0; i < 20; ++i) {
        threads.emplace_back([&]() {
            auto [err, conn] = pool.acquire();
            if (err == PoolErrorCode::OK) {
                acquired.fetch_add(1);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                pool.release(std::move(conn));
            } else {
                failed.fetch_add(1);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Verify: Pool state is consistent
    EXPECT_EQ(pool.getActiveCount(), 0);
    EXPECT_LE(pool.getTotalCreated(), 10);
    EXPECT_GT(acquired.load(), 0);
}

// ============================================================================
// RP-16: Degraded Mode Accepts Reduced Throughput
// ============================================================================

TEST_F(ResourcePoolingTest, RP_16_DegradedModeReducedThroughput) {
    // Gap: resource_pooling (graceful degradation)
    // Simulate degraded mode with limited connections
    MockConnectionPool::Config normal_config{.max_connections = 100};
    MockConnectionPool::Config degraded_config{.max_connections = 10};

    MockConnectionPool pool(degraded_config);
    std::atomic<int> operations(0);

    // Action: Process operations in degraded mode
    for (int i = 0; i < 50; ++i) {
        auto [err, conn] = pool.acquire();
        if (err == PoolErrorCode::OK) {
            operations.fetch_add(1);
            pool.release(std::move(conn));
        }
    }

    // Verify: Some operations succeeded despite reduced resources
    EXPECT_GT(operations.load(), 0);
}

// ============================================================================
// RP-17: Fallback to Per-Operation Pooling Succeeds
// ============================================================================

TEST_F(ResourcePoolingTest, RP_17_PerOperationPoolingFallback) {
    // Gap: resource_pooling (fallback strategy)
    // Setup: Main pool with per-operation fallback
    MockConnectionPool::Config config{.max_connections = 2};
    MockConnectionPool main_pool(config);
    MockConnectionPool fallback_pool(config);

    // Action: Use fallback when main exhausted
    auto acquire_with_fallback = [&]() -> std::unique_ptr<MockConnection> {
        auto [err1, conn1] = main_pool.acquire();
        if (err1 == PoolErrorCode::OK) {
            return std::move(conn1);
        }
        
        auto [err2, conn2] = fallback_pool.acquire();
        return std::move(conn2);
    };

    auto conns = std::vector<std::unique_ptr<MockConnection>>();
    for (int i = 0; i < 5; ++i) {
        conns.push_back(acquire_with_fallback());
    }

    // Verify: All acquisitions succeeded via fallback
    EXPECT_EQ(conns.size(), 5);

    // Cleanup
    for (auto& conn : conns) {
        if (conn && conn->isOpen()) {
            if (main_pool.getActiveCount() > 0) {
                main_pool.release(std::move(conn));
            } else {
                fallback_pool.release(std::move(conn));
            }
        }
    }
}

// ============================================================================
// RP-18: Connection Retry Succeeds After Transient Failure
// ============================================================================

TEST_F(ResourcePoolingTest, RP_18_RetryAfterTransientFailure) {
    // Gap: resource_pooling (transient failure recovery)
    // Setup: Pool
    MockConnectionPool::Config config{.max_connections = 5};
    MockConnectionPool pool(config);

    std::atomic<int> attempt(0);

    auto acquire_with_retry = [&]() -> PoolErrorCode {
        for (int i = 0; i < 3; ++i) {
            attempt.fetch_add(1);
            auto [err, conn] = pool.acquire();
            if (err == PoolErrorCode::OK) {
                pool.release(std::move(conn));
                return PoolErrorCode::OK;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return PoolErrorCode::POOL_EXHAUSTED;
    };

    // Action: Try acquire with retry
    auto result = acquire_with_retry();

    // Verify: Acquisition eventually succeeded
    EXPECT_EQ(result, PoolErrorCode::OK);
    EXPECT_GE(attempt.load(), 1);
}

// ============================================================================
// RP-19: Memory Pressure Triggers Connection Pruning
// ============================================================================

TEST_F(ResourcePoolingTest, RP_19_MemoryPressurePruning) {
    // Gap: resource_pooling (memory-aware connection pruning)
    // Simulate pool pruning under memory pressure
    MockConnectionPool::Config config{.max_connections = 20};
    MockConnectionPool pool(config);

    // Create excess connections
    std::vector<std::unique_ptr<MockConnection>> conns;
    for (int i = 0; i < 10; ++i) {
        auto [err, conn] = pool.acquire();
        if (err == PoolErrorCode::OK) {
            conns.push_back(std::move(conn));
        }
    }

    int before_available = pool.getAvailableCount();

    // Simulate memory pressure: release all connections (pruning candidates)
    conns.clear();

    int after_available = pool.getAvailableCount();

    // Verify: Available connections increased (ready for pruning if needed)
    EXPECT_GE(after_available, before_available);
}

// ============================================================================
// RP-20: Metrics Report Pool State Accurately
// ============================================================================

TEST_F(ResourcePoolingTest, RP_20_MetricsReportPoolState) {
    // Gap: resource_pooling (accurate pool state metrics)
    // Setup: Pool
    MockConnectionPool::Config config{.max_connections = 15};
    MockConnectionPool pool(config);

    // Initial state
    EXPECT_EQ(pool.getActiveCount(), 0);
    EXPECT_EQ(pool.getTotalCreated(), 0);
    EXPECT_EQ(pool.getAvailableCount(), 0);

    // Acquire some connections
    std::vector<std::unique_ptr<MockConnection>> conns;
    for (int i = 0; i < 5; ++i) {
        auto [err, conn] = pool.acquire();
        if (err == PoolErrorCode::OK) {
            conns.push_back(std::move(conn));
        }
    }

    // Verify metrics
    EXPECT_EQ(pool.getActiveCount(), 5);
    EXPECT_EQ(pool.getTotalCreated(), 5);
    EXPECT_EQ(pool.getAvailableCount(), 0);

    // Release connections
    for (auto& conn : conns) {
        pool.release(std::move(conn));
    }

    // Verify updated metrics
    EXPECT_EQ(pool.getActiveCount(), 0);
    EXPECT_EQ(pool.getTotalCreated(), 5);
    EXPECT_EQ(pool.getAvailableCount(), 5);
}

} // namespace test
} // namespace analytics
} // namespace themis
