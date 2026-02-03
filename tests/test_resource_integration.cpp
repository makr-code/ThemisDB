/**
 * ThemisDB Resource Management Integration Tests
 * 
 * Tests integration of resource managers with actual components:
 * - CrossShardTransactionCoordinator with TransactionLifecycleManager
 * - HealthMonitor with ThreadPoolManager
 * - WALShipper with ThreadPoolManager
 * - WALManager with WALRetentionManager
 */

#include <gtest/gtest.h>
#include "sharding/cross_shard_transaction.h"
#include "sharding/health_monitor.h"
#include "sharding/wal_shipper.h"
#include "sharding/wal_manager.h"
#include "sharding/thread_pool_manager.h"
#include "sharding/consensus_module.h"
#include "sharding/multi_primary_coordinator.h"
#include "sharding/replica_topology.h"
#include <thread>
#include <chrono>
#include <filesystem>

using namespace themisdb::sharding;
using namespace themis::sharding;

// Mock consensus module
class MockConsensusModule : public themisdb::sharding::ConsensusModule {
public:
    bool propose(const std::string&, const nlohmann::json&) override { return true; }
    bool isLeader() const override { return true; }
};

// ============================================================================
// Transaction Lifecycle Integration Tests
// ============================================================================

TEST(ResourceIntegrationTest, TransactionLimitsEnforced) {
    CrossShardTransactionConfig config;
    config.transaction_timeout = std::chrono::milliseconds(100);  // Short timeout for testing
    
    auto consensus = std::make_shared<MockConsensusModule>();
    CrossShardTransactionCoordinator coordinator(config, consensus);
    
    ASSERT_TRUE(coordinator.initialize());
    ASSERT_TRUE(coordinator.start());
    
    // Create transactions up to limit
    const size_t limit = 10000;
    size_t created = 0;
    
    for (size_t i = 0; i < limit + 100; ++i) {
        std::string txn_id = "txn_" + std::to_string(i);
        if (coordinator.beginTransaction(txn_id)) {
            ++created;
        } else {
            break;  // Hit limit
        }
    }
    
    // Should have created transactions up to limit
    EXPECT_LE(created, limit);
    EXPECT_GE(created, limit - 10);  // Allow some margin
    
    coordinator.stop();
}

TEST(ResourceIntegrationTest, TransactionTimeoutWorks) {
    CrossShardTransactionConfig config;
    config.transaction_timeout = std::chrono::milliseconds(200);
    
    auto consensus = std::make_shared<MockConsensusModule>();
    CrossShardTransactionCoordinator coordinator(config, consensus);
    
    ASSERT_TRUE(coordinator.initialize());
    ASSERT_TRUE(coordinator.start());
    
    // Create a transaction
    ASSERT_TRUE(coordinator.beginTransaction("timeout_test"));
    
    auto state_before = coordinator.getTransactionState("timeout_test");
    ASSERT_TRUE(state_before.has_value());
    EXPECT_EQ(state_before.value(), TransactionState::ACTIVE);
    
    // Wait for timeout + cleanup cycle
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Transaction should be aborted
    auto state_after = coordinator.getTransactionState("timeout_test");
    if (state_after.has_value()) {
        EXPECT_EQ(state_after.value(), TransactionState::ABORTED);
    }
    
    coordinator.stop();
}

// ============================================================================
// Thread Pool Integration Tests
// ============================================================================

TEST(ResourceIntegrationTest, HealthMonitorUsesThreadPool) {
    // Create shared thread pool
    ThreadPoolManager::Config pool_config{
        .core_threads = 4,
        .max_threads = 8,
        .queue_size = 100
    };
    auto thread_pool = std::make_shared<ThreadPoolManager>(pool_config);
    
    // Create health monitor with thread pool
    HealthMonitorConfig health_config;
    health_config.heartbeat_interval = std::chrono::milliseconds(100);
    
    auto primary_coord = std::make_shared<MultiPrimaryCoordinator>(
        MultiPrimaryCoordinatorConfig{}
    );
    auto topology = std::make_shared<ReplicaTopology>();
    
    auto health_monitor = std::make_shared<HealthMonitor>(
        health_config, primary_coord, topology, nullptr, thread_pool
    );
    
    // Start monitoring
    health_monitor->start();
    
    // Give it time to use the thread pool
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Check thread pool stats
    auto stats = thread_pool->getStats();
    EXPECT_GT(stats.completed_tasks, 0);  // Should have executed some tasks
    
    health_monitor->stop();
}

TEST(ResourceIntegrationTest, WALShipperUsesThreadPool) {
    // Create shared thread pool
    ThreadPoolManager::Config pool_config{
        .core_threads = 4,
        .max_threads = 8,
        .queue_size = 100
    };
    auto thread_pool = std::make_shared<ThreadPoolManager>(pool_config);
    
    // Create temporary WAL directory with high-resolution timestamp and random suffix
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::string wal_dir = "/tmp/test_wal_" + std::to_string(now) + "_" + std::to_string(rand());
    std::filesystem::create_directories(wal_dir);
    
    // Create WAL manager
    WALManagerConfig wal_config;
    wal_config.wal_directory = wal_dir;
    wal_config.segment_size = 1024 * 1024;  // 1 MB
    auto wal_manager = std::make_shared<WALManager>(wal_config);
    
    // Create WAL shipper with thread pool
    WALShipperConfig shipper_config;
    shipper_config.primary_id = "test_primary";
    auto wal_shipper = std::make_shared<WALShipper>(
        wal_manager, shipper_config, thread_pool
    );
    
    // Add a replica
    wal_shipper->addReplica("replica_1", "localhost:8080");
    
    // Start shipping
    wal_shipper->start();
    
    // Give it time to use the thread pool
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Check thread pool stats
    auto stats = thread_pool->getStats();
    EXPECT_GT(stats.completed_tasks, 0);  // Should have executed some tasks
    
    wal_shipper->stop();
    
    // Cleanup
    std::filesystem::remove_all(wal_dir);
}

// ============================================================================
// WAL Retention Integration Tests
// ============================================================================

TEST(ResourceIntegrationTest, WALRetentionEnforced) {
    // Create temporary WAL directory with high-resolution timestamp and random suffix
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::string wal_dir = "/tmp/test_wal_retention_" + std::to_string(now) + "_" + std::to_string(rand());
    std::filesystem::create_directories(wal_dir);
    
    // Create WAL manager with small segment size
    WALManagerConfig config;
    config.wal_directory = wal_dir;
    config.segment_size = 1024;  // 1 KB for quick rotation
    config.max_segments = 3;  // Only keep 3 segments
    config.sync_on_write = false;  // Faster for testing
    
    WALManager wal_manager(config);
    
    // Write enough entries to create multiple segments
    for (int i = 0; i < 100; ++i) {
        WALEntry entry;
        entry.type = WALEntryType::INSERT;
        entry.transaction_id = "txn_" + std::to_string(i);
        entry.data = {{"value", std::string(100, 'x')}};  // 100 bytes
        
        wal_manager.append(entry);
    }
    
    // Force flush
    wal_manager.flush();
    
    // Count WAL segment files
    size_t segment_count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(wal_dir)) {
        if (entry.path().extension() == ".wal") {
            ++segment_count;
        }
    }
    
    // Should have limited number of segments
    EXPECT_LE(segment_count, config.max_segments + 2);  // Allow some margin
    
    // Cleanup
    std::filesystem::remove_all(wal_dir);
}

// ============================================================================
// Resource Exhaustion Tests
// ============================================================================

TEST(ResourceIntegrationTest, ThreadPoolHandlesOverload) {
    ThreadPoolManager::Config config{
        .core_threads = 2,
        .max_threads = 4,
        .queue_size = 10
    };
    
    ThreadPoolManager pool(config);
    
    std::atomic<int> completed{0};
    std::atomic<int> rejected{0};
    
    // Submit more tasks than queue can hold
    for (int i = 0; i < 50; ++i) {
        bool submitted = pool.submit([&completed]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            ++completed;
        }, std::chrono::milliseconds(10));
        
        if (!submitted) {
            ++rejected;
        }
    }
    
    // Wait for tasks to complete
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Should have rejected some tasks
    EXPECT_GT(rejected, 0);
    
    // Should have completed some tasks
    EXPECT_GT(completed, 0);
    
    auto stats = pool.getStats();
    EXPECT_EQ(stats.rejected_tasks, rejected);
    EXPECT_GE(stats.completed_tasks, completed);
}

TEST(ResourceIntegrationTest, ConcurrentResourceAccess) {
    // Test that multiple components can use resource managers concurrently
    
    // Shared thread pool
    auto thread_pool = std::make_shared<ThreadPoolManager>(
        ThreadPoolManager::Config{.core_threads = 8, .max_threads = 16}
    );
    
    std::atomic<bool> running{true};
    std::vector<std::thread> threads;
    
    // Multiple threads submitting tasks
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&thread_pool, &running]() {
            while (running) {
                thread_pool->submit([]() {
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                });
                std::this_thread::sleep_for(std::chrono::microseconds(50));
            }
        });
    }
    
    // Let them run for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    running = false;
    for (auto& t : threads) {
        t.join();
    }
    
    // Check that thread pool handled concurrent access correctly
    auto stats = thread_pool->getStats();
    EXPECT_GT(stats.completed_tasks, 100);
    EXPECT_LT(stats.active_threads, 20);  // Should stay within limits
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
