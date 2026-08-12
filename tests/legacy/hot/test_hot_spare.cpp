/**
 * ThemisDB Hot Spare Management Tests
 * 
 * Comprehensive tests for hot spare functionality including:
 * - Spare pool management
 * - Automatic failover
 * - Background rebuild
 * - Progress tracking
 * - Metrics
 */

#include <gtest/gtest.h>
#include "sharding/hot_spare_manager.h"
#include "sharding/redundancy_strategy.h"
#include "sharding/consistent_hash.h"
#include "sharding/shard_topology.h"
#include <thread>
#include <chrono>

using namespace themisdb::sharding;

// ═══════════════════════════════════════════════════════════
// Mock Storage
// ═══════════════════════════════════════════════════════════

class MockShardStorage {
public:
    std::map<std::string, std::map<std::string, std::vector<uint8_t>>> shard_data;
    std::map<std::string, bool> shard_healthy;
    
    bool write(const std::string& shard_id, const std::string& doc_id, 
               const std::vector<uint8_t>& data) {
        if (!shard_healthy[shard_id]) return false;
        shard_data[shard_id][doc_id] = data;
        return true;
    }
    
    std::optional<std::vector<uint8_t>> read(const std::string& shard_id, 
                                              const std::string& doc_id) {
        if (!shard_healthy[shard_id]) return std::nullopt;
        if (shard_data.count(shard_id) && shard_data[shard_id].count(doc_id)) {
            return shard_data[shard_id][doc_id];
        }
        return std::nullopt;
    }
    
    std::vector<std::string> listDocuments(const std::string& shard_id) {
        std::vector<std::string> docs;
        if (shard_data.count(shard_id)) {
            for (const auto& [doc_id, _] : shard_data[shard_id]) {
                docs.push_back(doc_id);
            }
        }
        return docs;
    }
    
    void setShardHealth(const std::string& shard_id, bool healthy) {
        shard_healthy[shard_id] = healthy;
    }
};

// ═══════════════════════════════════════════════════════════
// Test Fixtures
// ═══════════════════════════════════════════════════════════

class HotSpareManagerTest : public ::testing::Test {
protected:
    std::unique_ptr<ConsistentHashRing> ring;
    std::unique_ptr<ShardTopology> topology;
    std::unique_ptr<MockShardStorage> storage;
    std::unique_ptr<RedundancyStrategy> strategy;
    
    void SetUp() override {
        // Create hash ring with multiple shards
        ring = std::make_unique<ConsistentHashRing>(100);
        
        // Add primary shards
        for (int i = 0; i < 3; ++i) {
            ring->addNode("shard-" + std::to_string(i));
        }
        
        // Create topology
        topology = std::make_unique<ShardTopology>();
        
        // Create storage
        storage = std::make_unique<MockShardStorage>();
        
        // Mark all shards as healthy initially
        for (int i = 0; i < 3; ++i) {
            storage->setShardHealth("shard-" + std::to_string(i), true);
        }
        for (int i = 0; i < 3; ++i) {
            storage->setShardHealth("spare-" + std::to_string(i), true);
        }
        
        // Create redundancy strategy
        RedundancyConfig config;
        config.mode = RedundancyMode::MIRROR;
        config.replication_factor = 3;
        strategy = std::make_unique<RedundancyStrategy>(config);
    }
    
    void TearDown() override {
        strategy.reset();
        storage.reset();
        topology.reset();
        ring.reset();
    }
    
    HotSpareManager::WriteHandler createWriteHandler() {
        return [this](const std::string& shard_id, const std::string& doc_id,
                     const std::vector<uint8_t>& data) {
            return storage->write(shard_id, doc_id, data);
        };
    }
    
    HotSpareManager::ReadHandler createReadHandler() {
        return [this](const std::string& shard_id, const std::string& doc_id) {
            return storage->read(shard_id, doc_id);
        };
    }
    
    HotSpareManager::DocumentIterator createDocIterator() {
        return [this](const std::string& shard_id) {
            return storage->listDocuments(shard_id);
        };
    }
};

// ═══════════════════════════════════════════════════════════
// Configuration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(HotSpareManagerTest, ConfigValidation) {
    HotSpareConfig config;
    
    // Invalid: enabled but no spares
    config.enable = true;
    config.spare_shards = {};
    EXPECT_FALSE(config.validate());
    
    // Invalid: zero throttle
    config.spare_shards = {"spare-1"};
    config.rebuild_throttle_mbps = 0;
    EXPECT_FALSE(config.validate());
    
    // Valid configuration
    config.rebuild_throttle_mbps = 100;
    config.max_concurrent_rebuilds = 2;
    config.rebuild_chunk_size_mb = 64;
    EXPECT_TRUE(config.validate());
}

TEST_F(HotSpareManagerTest, ManagerInitialization) {
    HotSpareConfig config;
    config.enable = true;
    config.spare_shards = {"spare-1", "spare-2", "spare-3"};
    config.auto_rebuild = true;
    
    EXPECT_NO_THROW({
        HotSpareManager manager(config, *strategy, *topology);
        EXPECT_FALSE(manager.isRunning());
        
        auto spares = manager.getAllSpares();
        EXPECT_EQ(spares.size(), 3);
    });
}

// ═══════════════════════════════════════════════════════════
// Spare Pool Management Tests
// ═══════════════════════════════════════════════════════════

TEST_F(HotSpareManagerTest, AddRemoveSpares) {
    HotSpareConfig config;
    config.enable = true;
    config.spare_shards = {"spare-1"};
    
    HotSpareManager manager(config, *strategy, *topology);
    
    // Add spare
    manager.addSpare("spare-2");
    auto spares = manager.getAvailableSpares();
    EXPECT_EQ(spares.size(), 2);
    
    // Remove spare
    manager.removeSpare("spare-2");
    spares = manager.getAvailableSpares();
    EXPECT_EQ(spares.size(), 1);
}

TEST_F(HotSpareManagerTest, SpareStateTracking) {
    HotSpareConfig config;
    config.enable = true;
    config.spare_shards = {"spare-1", "spare-2"};
    
    HotSpareManager manager(config, *strategy, *topology);
    
    auto info = manager.getSpareInfo("spare-1");
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->state, SpareState::AVAILABLE);
    EXPECT_EQ(info->shard_id, "spare-1");
}

// ═══════════════════════════════════════════════════════════
// Failover Tests
// ═══════════════════════════════════════════════════════════

TEST_F(HotSpareManagerTest, BasicFailover) {
    HotSpareConfig config;
    config.enable = true;
    config.spare_shards = {"spare-1", "spare-2"};
    config.auto_rebuild = false;  // Disable auto rebuild for this test
    
    HotSpareManager manager(config, *strategy, *topology);
    
    // Simulate shard failure
    std::string failed_shard = "shard-0";
    storage->setShardHealth(failed_shard, false);
    
    // Activate spare
    bool success = manager.activateSpare(
        failed_shard,
        *ring,
        createReadHandler(),
        createWriteHandler(),
        createDocIterator()
    );
    
    EXPECT_TRUE(success);
    
    // Verify spare was activated
    auto info = manager.getSpareInfo("spare-1");
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->state, SpareState::ACTIVE);
    
    // Verify stats
    auto stats = manager.getStats();
    EXPECT_EQ(stats.total_failovers, 1);
    EXPECT_EQ(stats.successful_failovers, 1);
}

TEST_F(HotSpareManagerTest, FailoverWithNoSpares) {
    HotSpareConfig config;
    config.enable = true;
    config.spare_shards = {};  // No spares
    
    HotSpareManager manager(config, *strategy, *topology);
    
    // Try to activate spare with no spares available
    bool success = manager.activateSpare(
        "shard-0",
        *ring,
        createReadHandler(),
        createWriteHandler(),
        createDocIterator()
    );
    
    EXPECT_FALSE(success);
    
    auto stats = manager.getStats();
    EXPECT_EQ(stats.failed_failovers, 1);
}

TEST_F(HotSpareManagerTest, FailoverHistory) {
    HotSpareConfig config;
    config.enable = true;
    config.spare_shards = {"spare-1", "spare-2"};
    config.auto_rebuild = false;
    
    HotSpareManager manager(config, *strategy, *topology);
    
    // Perform multiple failovers
    manager.activateSpare("shard-0", *ring, createReadHandler(), 
                         createWriteHandler(), createDocIterator());
    manager.activateSpare("shard-1", *ring, createReadHandler(), 
                         createWriteHandler(), createDocIterator());
    
    auto history = manager.getFailoverHistory(10);
    EXPECT_EQ(history.size(), 2);
    EXPECT_EQ(history[0].failed_shard_id, "shard-0");
    EXPECT_EQ(history[1].failed_shard_id, "shard-1");
}

TEST_F(HotSpareManagerTest, FailoverTimingUnder5Seconds) {
    HotSpareConfig config;
    config.enable = true;
    config.spare_shards = {"spare-1"};
    config.auto_rebuild = false;
    
    HotSpareManager manager(config, *strategy, *topology);
    
    auto start = std::chrono::steady_clock::now();
    
    bool success = manager.activateSpare(
        "shard-0",
        *ring,
        createReadHandler(),
        createWriteHandler(),
        createDocIterator()
    );
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_TRUE(success);
    EXPECT_LT(duration.count(), 5000);  // Should be under 5 seconds
}

// ═══════════════════════════════════════════════════════════
// Rebuild Tests
// ═══════════════════════════════════════════════════════════

TEST_F(HotSpareManagerTest, RebuildStatusTracking) {
    HotSpareConfig config;
    config.enable = true;
    config.spare_shards = {"spare-1"};
    config.auto_rebuild = true;
    
    HotSpareManager manager(config, *strategy, *topology);
    
    // Initial status
    auto status = manager.getRebuildStatus();
    EXPECT_FALSE(status.is_rebuilding);
    EXPECT_EQ(status.active_rebuilds, 0);
}

TEST_F(HotSpareManagerTest, RebuildProgressCalculation) {
    SpareShardInfo spare;
    spare.state = SpareState::REBUILDING;
    spare.bytes_rebuilt = 50 * 1024 * 1024;  // 50 MB
    spare.total_bytes = 100 * 1024 * 1024;   // 100 MB
    spare.rebuild_started = std::chrono::system_clock::now() - std::chrono::seconds(10);
    
    EXPECT_DOUBLE_EQ(spare.getProgressPercentage(), 50.0);
    EXPECT_GT(spare.getRebuildThroughputMBps(), 0.0);
    EXPECT_GT(spare.getEstimatedTimeRemaining().count(), 0);
}

TEST_F(HotSpareManagerTest, RebuildPauseResume) {
    HotSpareConfig config;
    config.enable = true;
    config.spare_shards = {"spare-1"};
    config.auto_rebuild = true;
    
    HotSpareManager manager(config, *strategy, *topology);
    
    // Pause rebuild
    EXPECT_NO_THROW(manager.pauseRebuild("spare-1"));
    
    // Resume rebuild
    EXPECT_NO_THROW(manager.resumeRebuild("spare-1"));
}

TEST_F(HotSpareManagerTest, RebuildCancel) {
    HotSpareConfig config;
    config.enable = true;
    config.spare_shards = {"spare-1"};
    config.auto_rebuild = true;
    
    HotSpareManager manager(config, *strategy, *topology);
    
    EXPECT_NO_THROW(manager.cancelRebuild("spare-1"));
}

// ═══════════════════════════════════════════════════════════
// Lifecycle Tests
// ═══════════════════════════════════════════════════════════

TEST_F(HotSpareManagerTest, StartStop) {
    HotSpareConfig config;
    config.enable = true;
    config.spare_shards = {"spare-1"};
    
    HotSpareManager manager(config, *strategy, *topology);
    
    EXPECT_FALSE(manager.isRunning());
    
    manager.start();
    EXPECT_TRUE(manager.isRunning());
    
    manager.stop();
    EXPECT_FALSE(manager.isRunning());
}

TEST_F(HotSpareManagerTest, MultipleStartStop) {
    HotSpareConfig config;
    config.enable = true;
    config.spare_shards = {"spare-1"};
    
    HotSpareManager manager(config, *strategy, *topology);
    
    // Multiple starts should be safe
    manager.start();
    manager.start();
    EXPECT_TRUE(manager.isRunning());
    
    // Multiple stops should be safe
    manager.stop();
    manager.stop();
    EXPECT_FALSE(manager.isRunning());
}

// ═══════════════════════════════════════════════════════════
// Statistics Tests
// ═══════════════════════════════════════════════════════════

TEST_F(HotSpareManagerTest, StatisticsTracking) {
    HotSpareConfig config;
    config.enable = true;
    config.spare_shards = {"spare-1", "spare-2"};
    config.auto_rebuild = false;
    
    HotSpareManager manager(config, *strategy, *topology);
    
    // Perform operations
    manager.activateSpare("shard-0", *ring, createReadHandler(), 
                         createWriteHandler(), createDocIterator());
    
    auto stats = manager.getStats();
    EXPECT_EQ(stats.total_failovers, 1);
    EXPECT_EQ(stats.successful_failovers, 1);
    EXPECT_GT(stats.spares_available, 0);
}

TEST_F(HotSpareManagerTest, AverageFailoverTime) {
    HotSpareConfig config;
    config.enable = true;
    config.spare_shards = {"spare-1", "spare-2"};
    config.auto_rebuild = false;
    
    HotSpareManager manager(config, *strategy, *topology);
    
    // Perform multiple failovers
    manager.activateSpare("shard-0", *ring, createReadHandler(), 
                         createWriteHandler(), createDocIterator());
    manager.activateSpare("shard-1", *ring, createReadHandler(), 
                         createWriteHandler(), createDocIterator());
    
    auto stats = manager.getStats();
    EXPECT_GT(stats.avg_failover_time.count(), 0);
}

// ═══════════════════════════════════════════════════════════
// Metrics Tests
// ═══════════════════════════════════════════════════════════

TEST_F(HotSpareManagerTest, PrometheusMetrics) {
    HotSpareConfig config;
    config.enable = true;
    config.spare_shards = {"spare-1"};
    
    HotSpareManager manager(config, *strategy, *topology);
    
    std::string metrics = manager.exportPrometheusMetrics();
    
    // Verify metrics contain expected strings
    EXPECT_NE(metrics.find("themis_hot_spare_total_failovers"), std::string::npos);
    EXPECT_NE(metrics.find("themis_hot_spare_spares_available"), std::string::npos);
    EXPECT_NE(metrics.find("themis_hot_spare_avg_failover_time_ms"), std::string::npos);
}

TEST_F(HotSpareManagerTest, MetricsAfterFailover) {
    HotSpareConfig config;
    config.enable = true;
    config.spare_shards = {"spare-1"};
    config.auto_rebuild = false;
    
    HotSpareManager manager(config, *strategy, *topology);
    
    manager.activateSpare("shard-0", *ring, createReadHandler(), 
                         createWriteHandler(), createDocIterator());
    
    std::string metrics = manager.exportPrometheusMetrics();
    
    // Verify metrics show the failover
    EXPECT_NE(metrics.find("themis_hot_spare_successful_failovers 1"), std::string::npos);
}

// ═══════════════════════════════════════════════════════════
// Configuration Update Tests
// ═══════════════════════════════════════════════════════════

TEST_F(HotSpareManagerTest, ConfigUpdate) {
    HotSpareConfig config;
    config.enable = true;
    config.spare_shards = {"spare-1"};
    config.rebuild_throttle_mbps = 50;
    
    HotSpareManager manager(config, *strategy, *topology);
    
    // Update config
    config.rebuild_throttle_mbps = 100;
    EXPECT_NO_THROW(manager.updateConfig(config));
    
    EXPECT_EQ(manager.getConfig().rebuild_throttle_mbps, 100);
}

TEST_F(HotSpareManagerTest, InvalidConfigUpdate) {
    HotSpareConfig config;
    config.enable = true;
    config.spare_shards = {"spare-1"};
    
    HotSpareManager manager(config, *strategy, *topology);
    
    // Try to update with invalid config
    config.rebuild_throttle_mbps = 0;  // Invalid
    EXPECT_THROW(manager.updateConfig(config), std::invalid_argument);
}

// ═══════════════════════════════════════════════════════════
// Integration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(HotSpareManagerTest, EndToEndFailoverAndRebuild) {
    HotSpareConfig config;
    config.enable = true;
    config.spare_shards = {"spare-1"};
    config.auto_rebuild = false;  // Manual control for testing
    
    HotSpareManager manager(config, *strategy, *topology);
    
    // Write some data to primary shard
    std::vector<uint8_t> data = {'t', 'e', 's', 't'};
    storage->write("shard-0", "doc-1", data);
    
    // Simulate shard failure
    storage->setShardHealth("shard-0", false);
    
    // Activate spare
    bool success = manager.activateSpare(
        "shard-0",
        *ring,
        createReadHandler(),
        createWriteHandler(),
        createDocIterator()
    );
    
    EXPECT_TRUE(success);
    
    // Verify spare is active
    auto info = manager.getSpareInfo("spare-1");
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->state, SpareState::ACTIVE);
    
    // Verify statistics
    auto stats = manager.getStats();
    EXPECT_EQ(stats.successful_failovers, 1);
}

TEST_F(HotSpareManagerTest, MultipleSimultaneousFailures) {
    HotSpareConfig config;
    config.enable = true;
    config.spare_shards = {"spare-1", "spare-2", "spare-3"};
    config.auto_rebuild = false;
    
    HotSpareManager manager(config, *strategy, *topology);
    
    // Simulate multiple failures
    bool success1 = manager.activateSpare("shard-0", *ring, createReadHandler(), 
                                          createWriteHandler(), createDocIterator());
    bool success2 = manager.activateSpare("shard-1", *ring, createReadHandler(), 
                                          createWriteHandler(), createDocIterator());
    
    EXPECT_TRUE(success1);
    EXPECT_TRUE(success2);
    
    auto stats = manager.getStats();
    EXPECT_EQ(stats.successful_failovers, 2);
    
    // Should still have spares available
    auto available = manager.getAvailableSpares();
    EXPECT_GT(available.size(), 0);
}

// ═══════════════════════════════════════════════════════════
// Edge Cases
// ═══════════════════════════════════════════════════════════

TEST_F(HotSpareManagerTest, FailoverWhenAllSparesActive) {
    HotSpareConfig config;
    config.enable = true;
    config.spare_shards = {"spare-1"};
    config.auto_rebuild = false;
    
    HotSpareManager manager(config, *strategy, *topology);
    
    // Use up all spares
    manager.activateSpare("shard-0", *ring, createReadHandler(), 
                         createWriteHandler(), createDocIterator());
    
    // Try another failover - should fail
    bool success = manager.activateSpare("shard-1", *ring, createReadHandler(), 
                                         createWriteHandler(), createDocIterator());
    
    EXPECT_FALSE(success);
    EXPECT_EQ(manager.getAvailableSpares().size(), 0);
}

TEST_F(HotSpareManagerTest, RemoveActiveSpare) {
    HotSpareConfig config;
    config.enable = true;
    config.spare_shards = {"spare-1"};
    config.auto_rebuild = false;
    
    HotSpareManager manager(config, *strategy, *topology);
    
    // Activate spare
    manager.activateSpare("shard-0", *ring, createReadHandler(), 
                         createWriteHandler(), createDocIterator());
    
    // Try to remove active spare - should fail
    manager.removeSpare("spare-1");
    
    // Verify spare still exists
    auto info = manager.getSpareInfo("spare-1");
    ASSERT_TRUE(info.has_value());
}

// ═══════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════
// main() removed - GTest provides its own main via gtest_main
