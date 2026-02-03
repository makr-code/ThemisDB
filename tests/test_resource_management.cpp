/**
 * ThemisDB Resource Management Tests
 * 
 * Tests for bounded LRU cache, connection pool, thread pool,
 * transaction lifecycle, WAL retention, and resource monitoring.
 */

#include <gtest/gtest.h>
#include "sharding/bounded_lru_cache.h"
#include "sharding/connection_pool.h"
#include "sharding/thread_pool_manager.h"
#include "sharding/transaction_lifecycle_manager.h"
#include "sharding/wal_retention_manager.h"
#include "sharding/resource_monitor.h"
#include <thread>
#include <chrono>

using namespace themisdb::sharding;

// ============================================================================
// Bounded LRU Cache Tests
// ============================================================================

TEST(BoundedLRUCacheTest, BasicPutGet) {
    BoundedLRUCache<std::string, int>::Config config{
        .max_entries = 100,
        .max_bytes = 1000,
        .ttl = std::chrono::minutes(10),
        .size_estimator = [](const int&) { return sizeof(int); }
    };
    
    BoundedLRUCache<std::string, int> cache(config);
    
    cache.put("key1", 42);
    auto value = cache.get("key1");
    
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), 42);
}

TEST(BoundedLRUCacheTest, LRUEviction) {
    BoundedLRUCache<std::string, int>::Config config{
        .max_entries = 3,
        .max_bytes = 1000,
        .ttl = std::chrono::minutes(10),
        .size_estimator = [](const int&) { return sizeof(int); }
    };
    
    BoundedLRUCache<std::string, int> cache(config);
    
    // Fill cache
    cache.put("key1", 1);
    cache.put("key2", 2);
    cache.put("key3", 3);
    
    // Access key1 to make it most recently used
    cache.get("key1");
    
    // Add new entry - should evict key2 (least recently used)
    cache.put("key4", 4);
    
    EXPECT_TRUE(cache.get("key1").has_value());
    EXPECT_FALSE(cache.get("key2").has_value());
    EXPECT_TRUE(cache.get("key3").has_value());
    EXPECT_TRUE(cache.get("key4").has_value());
}

TEST(BoundedLRUCacheTest, TTLExpiration) {
    BoundedLRUCache<std::string, int>::Config config{
        .max_entries = 100,
        .max_bytes = 1000,
        .ttl = std::chrono::milliseconds(100),
        .size_estimator = [](const int&) { return sizeof(int); }
    };
    
    BoundedLRUCache<std::string, int> cache(config);
    
    cache.put("key1", 42);
    EXPECT_TRUE(cache.get("key1").has_value());
    
    // Wait for TTL to expire
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    
    EXPECT_FALSE(cache.get("key1").has_value());
}

TEST(BoundedLRUCacheTest, Stats) {
    BoundedLRUCache<std::string, int>::Config config{
        .max_entries = 100,
        .max_bytes = 1000,
        .ttl = std::chrono::minutes(10),
        .size_estimator = [](const int&) { return sizeof(int); }
    };
    
    BoundedLRUCache<std::string, int> cache(config);
    
    cache.put("key1", 1);
    cache.put("key2", 2);
    cache.get("key1");  // hit
    cache.get("key3");  // miss
    
    auto stats = cache.getStats();
    EXPECT_EQ(stats.entries, 2);
    EXPECT_EQ(stats.hits, 1);
    EXPECT_EQ(stats.misses, 1);
}

// ============================================================================
// Connection Pool Tests
// ============================================================================

TEST(ConnectionPoolTest, AcquireRelease) {
    ConnectionPool::Config config{
        .initial_size = 2,
        .max_size = 5,
        .idle_timeout = std::chrono::minutes(5),
        .connection_timeout = std::chrono::seconds(1),
        .create_connection = []() {
            Connection conn;
            conn.handle = reinterpret_cast<void*>(1);
            conn.created_at = std::chrono::system_clock::now();
            conn.last_used = std::chrono::system_clock::now();
            return conn;
        },
        .destroy_connection = [](Connection) {}
    };
    
    ConnectionPool pool(config);
    
    auto conn = pool.acquire(std::chrono::seconds(1));
    ASSERT_TRUE(conn.has_value());
    
    pool.release(conn.value());
    
    auto stats = pool.getStats();
    EXPECT_GE(stats.total_size, 1);
}

TEST(ConnectionPoolTest, DynamicScaling) {
    ConnectionPool::Config config{
        .initial_size = 1,
        .max_size = 3,
        .idle_timeout = std::chrono::minutes(5),
        .connection_timeout = std::chrono::seconds(1),
        .create_connection = []() {
            static int counter = 0;
            Connection conn;
            conn.handle = reinterpret_cast<void*>(++counter);
            conn.created_at = std::chrono::system_clock::now();
            conn.last_used = std::chrono::system_clock::now();
            return conn;
        },
        .destroy_connection = [](Connection) {}
    };
    
    ConnectionPool pool(config);
    
    // Acquire all connections
    auto conn1 = pool.acquire(std::chrono::seconds(1));
    auto conn2 = pool.acquire(std::chrono::seconds(1));
    auto conn3 = pool.acquire(std::chrono::seconds(1));
    
    EXPECT_TRUE(conn1.has_value());
    EXPECT_TRUE(conn2.has_value());
    EXPECT_TRUE(conn3.has_value());
    
    auto stats = pool.getStats();
    EXPECT_EQ(stats.in_use, 3);
}

// ============================================================================
// Thread Pool Manager Tests
// ============================================================================

TEST(ThreadPoolManagerTest, SubmitTask) {
    ThreadPoolManager::Config config{
        .core_threads = 2,
        .max_threads = 4,
        .queue_size = 100,
        .thread_timeout = std::chrono::seconds(60)
    };
    
    ThreadPoolManager pool(config);
    
    std::atomic<int> counter{0};
    
    bool submitted = pool.submit([&counter]() {
        ++counter;
    });
    
    EXPECT_TRUE(submitted);
    
    // Wait for task to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_EQ(counter, 1);
}

TEST(ThreadPoolManagerTest, MultipleTasksParallel) {
    ThreadPoolManager::Config config{
        .core_threads = 4,
        .max_threads = 8,
        .queue_size = 100,
        .thread_timeout = std::chrono::seconds(60)
    };
    
    ThreadPoolManager pool(config);
    
    std::atomic<int> counter{0};
    const int num_tasks = 10;
    
    for (int i = 0; i < num_tasks; ++i) {
        pool.submit([&counter]() {
            ++counter;
        });
    }
    
    // Wait for all tasks to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    EXPECT_EQ(counter, num_tasks);
}

// ============================================================================
// Transaction Lifecycle Manager Tests
// ============================================================================

TEST(TransactionLifecycleManagerTest, RegisterTransaction) {
    TransactionLifecycleManager::Config config{
        .max_pending = 100,
        .ttl = std::chrono::minutes(10),
        .on_timeout = nullptr
    };
    
    TransactionLifecycleManager manager(config);
    
    bool registered = manager.registerTransaction("txn_001");
    EXPECT_TRUE(registered);
    
    auto meta = manager.getMetadata("txn_001");
    ASSERT_TRUE(meta.has_value());
    EXPECT_EQ(meta->transaction_id, "txn_001");
    EXPECT_EQ(meta->state, TransactionState::ACTIVE);
}

TEST(TransactionLifecycleManagerTest, TransitionState) {
    TransactionLifecycleManager::Config config{
        .max_pending = 100,
        .ttl = std::chrono::minutes(10),
        .on_timeout = nullptr
    };
    
    TransactionLifecycleManager manager(config);
    
    manager.registerTransaction("txn_001");
    bool transitioned = manager.transitionState("txn_001", TransactionState::COMMITTED);
    
    EXPECT_TRUE(transitioned);
    
    auto meta = manager.getMetadata("txn_001");
    ASSERT_TRUE(meta.has_value());
    EXPECT_EQ(meta->state, TransactionState::COMMITTED);
}

TEST(TransactionLifecycleManagerTest, PendingLimit) {
    TransactionLifecycleManager::Config config{
        .max_pending = 2,
        .ttl = std::chrono::minutes(10),
        .on_timeout = nullptr
    };
    
    TransactionLifecycleManager manager(config);
    
    EXPECT_TRUE(manager.registerTransaction("txn_001"));
    EXPECT_TRUE(manager.registerTransaction("txn_002"));
    EXPECT_FALSE(manager.registerTransaction("txn_003"));  // Exceeds limit
}

// ============================================================================
// WAL Retention Manager Tests
// ============================================================================

TEST(WALRetentionManagerTest, RegisterSegment) {
    WALRetentionManager::Config config{
        .max_segment_size = 1024,
        .max_total_size = 10240,
        .retention_time = std::chrono::hours(24),
        .on_segment_ready_for_deletion = nullptr
    };
    
    WALRetentionManager manager(config);
    
    bool registered = manager.registerSegment("segment_001");
    EXPECT_TRUE(registered);
    
    auto stats = manager.getStats();
    EXPECT_EQ(stats.active_segments, 1);
}

TEST(WALRetentionManagerTest, SegmentRotation) {
    WALRetentionManager::Config config{
        .max_segment_size = 1024,
        .max_total_size = 10240,
        .retention_time = std::chrono::hours(24),
        .on_segment_ready_for_deletion = nullptr
    };
    
    WALRetentionManager manager(config);
    
    manager.registerSegment("segment_001");
    manager.updateSegmentSize("segment_001", 1024);
    
    EXPECT_TRUE(manager.needsRotation());
}

// ============================================================================
// Resource Monitor Tests
// ============================================================================

TEST(ResourceMonitorTest, RegisterAlertHandler) {
    ResourceMonitor monitor;
    
    bool alert_triggered = false;
    monitor.registerAlertHandler("memory_usage", [&alert_triggered](const ResourceAlert&) {
        alert_triggered = true;
    });
    
    // Trigger alert
    monitor.updateMetric(
        "test_component",
        "memory_usage",
        100.0,
        50.0,  // threshold
        ResourceAlert::Severity::WARNING
    );
    
    EXPECT_TRUE(alert_triggered);
}

TEST(ResourceMonitorTest, GlobalStats) {
    ResourceMonitor monitor;
    
    monitor.updateMetric("comp1", "memory_bytes", 1000, 2000, ResourceAlert::Severity::WARNING);
    monitor.updateMetric("comp2", "active_threads", 5, 10, ResourceAlert::Severity::WARNING);
    
    auto stats = monitor.getGlobalStats();
    EXPECT_EQ(stats.total_memory_bytes, 1000);
    EXPECT_EQ(stats.active_threads, 5);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
