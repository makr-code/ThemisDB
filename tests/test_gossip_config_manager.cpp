#include <gtest/gtest.h>
#include "sharding/gossip_config_manager.h"
#include "sharding/shard_topology.h"
#include <memory>
#include <thread>
#include <chrono>

using namespace themis::sharding;

// ============================================================================
// VectorClock Tests
// ============================================================================

TEST(VectorClockTest, InitialState) {
    VectorClock clock;
    EXPECT_EQ(clock.get("shard-1"), 0ULL);
    EXPECT_EQ(clock.get("shard-2"), 0ULL);
}

TEST(VectorClockTest, Increment) {
    VectorClock clock;
    clock.increment("shard-1");
    EXPECT_EQ(clock.get("shard-1"), 1ULL);
    
    clock.increment("shard-1");
    EXPECT_EQ(clock.get("shard-1"), 2ULL);
    
    clock.increment("shard-2");
    EXPECT_EQ(clock.get("shard-2"), 1ULL);
}

TEST(VectorClockTest, Merge) {
    VectorClock clock1;
    clock1.set("shard-1", 5);
    clock1.set("shard-2", 3);
    
    VectorClock clock2;
    clock2.set("shard-1", 3);
    clock2.set("shard-2", 7);
    clock2.set("shard-3", 2);
    
    clock1.merge(clock2);
    
    EXPECT_EQ(clock1.get("shard-1"), 5ULL);  // max(5, 3)
    EXPECT_EQ(clock1.get("shard-2"), 7ULL);  // max(3, 7)
    EXPECT_EQ(clock1.get("shard-3"), 2ULL);  // max(0, 2)
}

TEST(VectorClockTest, CompareEqual) {
    VectorClock clock1;
    clock1.set("shard-1", 5);
    clock1.set("shard-2", 3);
    
    VectorClock clock2;
    clock2.set("shard-1", 5);
    clock2.set("shard-2", 3);
    
    EXPECT_EQ(clock1.compare(clock2), VectorClock::Ordering::EQUAL);
}

TEST(VectorClockTest, CompareBefore) {
    VectorClock clock1;
    clock1.set("shard-1", 3);
    clock1.set("shard-2", 2);
    
    VectorClock clock2;
    clock2.set("shard-1", 5);
    clock2.set("shard-2", 4);
    
    EXPECT_EQ(clock1.compare(clock2), VectorClock::Ordering::BEFORE);
}

TEST(VectorClockTest, CompareAfter) {
    VectorClock clock1;
    clock1.set("shard-1", 7);
    clock1.set("shard-2", 5);
    
    VectorClock clock2;
    clock2.set("shard-1", 3);
    clock2.set("shard-2", 2);
    
    EXPECT_EQ(clock1.compare(clock2), VectorClock::Ordering::AFTER);
}

TEST(VectorClockTest, CompareConcurrent) {
    VectorClock clock1;
    clock1.set("shard-1", 5);
    clock1.set("shard-2", 2);
    
    VectorClock clock2;
    clock2.set("shard-1", 3);
    clock2.set("shard-2", 7);
    
    EXPECT_EQ(clock1.compare(clock2), VectorClock::Ordering::CONCURRENT);
}

TEST(VectorClockTest, CloneAndMergeBehavior) {
    VectorClock clock;
    clock.set("shard-1", 42);
    clock.set("shard-2", 17);

    VectorClock copy = clock;
    EXPECT_EQ(copy.get("shard-1"), 42ULL);
    EXPECT_EQ(copy.get("shard-2"), 17ULL);

    VectorClock incoming;
    incoming.set("shard-1", 50);
    incoming.set("shard-3", 3);
    copy.merge(incoming);

    EXPECT_EQ(copy.get("shard-1"), 50ULL);
    EXPECT_EQ(copy.get("shard-2"), 17ULL);
    EXPECT_EQ(copy.get("shard-3"), 3ULL);
}

// ============================================================================
// ConfigUpdate Tests
// ============================================================================

TEST(ConfigUpdateTest, StoresExpectedFields) {
    ConfigUpdate update;
    update.update_id = "test-update-123";
    update.config_key = "shard.replication_factor";
    update.config_value = "3";
    update.timestamp_ns = 1234567890ULL;
    update.originator_shard_id = "shard-1";
    update.ttl = 10;
    update.vector_clock.set("shard-1", 5);

    EXPECT_EQ(update.update_id, "test-update-123");
    EXPECT_EQ(update.config_key, "shard.replication_factor");
    EXPECT_EQ(update.config_value, "3");
    EXPECT_EQ(update.timestamp_ns, 1234567890ULL);
    EXPECT_EQ(update.originator_shard_id, "shard-1");
    EXPECT_EQ(update.ttl, 10U);
    EXPECT_EQ(update.vector_clock.get("shard-1"), 5ULL);
}

// ============================================================================
// ResourceSnapshot Tests
// ============================================================================

TEST(ResourceSnapshotTest, StoresExpectedFields) {
    ResourceSnapshot snapshot;
    snapshot.shard_id = "shard-1";
    snapshot.timestamp_ns = 9876543210ULL;
    snapshot.available_memory_bytes = 1024 * 1024 * 1024;
    snapshot.total_memory_bytes = 4ULL * 1024 * 1024 * 1024;
    snapshot.available_cpu_cores = 4;
    snapshot.total_cpu_cores = 8;
    snapshot.cpu_usage_percent = 45.5;
    snapshot.memory_usage_percent = 75.0;
    snapshot.is_healthy = true;
    snapshot.status = "healthy";
    snapshot.warnings.push_back("High CPU usage");

    EXPECT_EQ(snapshot.shard_id, "shard-1");
    EXPECT_EQ(snapshot.timestamp_ns, 9876543210ULL);
    EXPECT_EQ(snapshot.available_memory_bytes, 1024ULL * 1024 * 1024);
    EXPECT_EQ(snapshot.total_memory_bytes, 4ULL * 1024 * 1024 * 1024);
    EXPECT_EQ(snapshot.available_cpu_cores, 4U);
    EXPECT_EQ(snapshot.total_cpu_cores, 8U);
    EXPECT_DOUBLE_EQ(snapshot.cpu_usage_percent, 45.5);
    EXPECT_DOUBLE_EQ(snapshot.memory_usage_percent, 75.0);
    EXPECT_TRUE(snapshot.is_healthy);
    EXPECT_EQ(snapshot.status, "healthy");
    EXPECT_EQ(snapshot.warnings.size(), 1ULL);
    EXPECT_EQ(snapshot.warnings[0], "High CPU usage");
}

// ============================================================================
// GossipConfigManager Tests
// ============================================================================

class GossipConfigManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a simple topology with test shards
        topology_ = std::make_shared<ShardTopology>();
        
        // Add multiple shards for testing
        static constexpr int NUM_TEST_SHARDS = 10;
        for (int i = 0; i < NUM_TEST_SHARDS; ++i) {
            ShardInfo shard;
            shard.shard_id = "shard-" + std::to_string(i);
            shard.primary_endpoint = "localhost:" + std::to_string(8000 + i);
            shard.is_healthy = true;
            topology_->addShard(shard);
        }
    }
    
    std::shared_ptr<ShardTopology> topology_;
};

TEST_F(GossipConfigManagerTest, ManagerInitialization) {
    GossipConfigManagerConfig config;
    config.enabled = false;  // Don't start threads for this test
    config.local_shard_id = "shard-0";
    config.local_endpoint = "localhost:8000";
    
    GossipConfigManager manager(config, topology_);
    
    EXPECT_FALSE(manager.isRunning());
}

TEST_F(GossipConfigManagerTest, StartStop) {
    GossipConfigManagerConfig config;
    config.enabled = true;
    config.local_shard_id = "shard-0";
    config.local_endpoint = "localhost:8000";
    config.gossip_interval_ms = 100;
    config.anti_entropy_interval_ms = 200;
    
    GossipConfigManager manager(config, topology_);
    
    manager.start();
    EXPECT_TRUE(manager.isRunning());
    
    // Let it run for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    
    manager.stop();
    EXPECT_FALSE(manager.isRunning());
}

TEST_F(GossipConfigManagerTest, PublishConfigUpdate) {
    GossipConfigManagerConfig config;
    config.enabled = false;
    config.local_shard_id = "shard-0";
    config.local_endpoint = "localhost:8000";
    
    GossipConfigManager manager(config, topology_);
    
    std::string update_id = manager.publishConfigUpdate(
        "test.config.key",
        "test_value"
    );
    
    EXPECT_FALSE(update_id.empty());
    
    // Verify config was stored
    std::string value = manager.getConfig("test.config.key");
    EXPECT_EQ(value, "test_value");
}

TEST_F(GossipConfigManagerTest, ConfigUpdateCallback) {
    GossipConfigManagerConfig config;
    config.enabled = false;
    config.local_shard_id = "shard-0";
    config.local_endpoint = "localhost:8000";
    
    GossipConfigManager manager(config, topology_);
    
    bool callback_called = false;
    std::string received_key;
    std::string received_value;
    
    manager.onConfigUpdate([&](const ConfigUpdate& update) {
        callback_called = true;
        received_key = update.config_key;
        received_value = update.config_value;
    });
    
    manager.publishConfigUpdate("callback.test", "callback_value");
    
    EXPECT_TRUE(callback_called);
    EXPECT_EQ(received_key, "callback.test");
    EXPECT_EQ(received_value, "callback_value");
}

TEST_F(GossipConfigManagerTest, ConflictResolution) {
    GossipConfigManagerConfig config;
    config.enabled = false;
    config.local_shard_id = "shard-0";
    config.local_endpoint = "localhost:8000";
    
    GossipConfigManager manager(config, topology_);
    
    manager.publishConfigUpdate("conflict.key", "value1");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    manager.publishConfigUpdate("conflict.key", "value2");
    
    // The newer update (with later timestamp) should win due to last-write-wins
    std::string value = manager.getConfig("conflict.key");
    EXPECT_EQ(value, "value2");
}

TEST_F(GossipConfigManagerTest, ResourceSnapshotPublish) {
    GossipConfigManagerConfig config;
    config.enabled = false;
    config.local_shard_id = "shard-0";
    config.local_endpoint = "localhost:8000";
    
    GossipConfigManager manager(config, topology_);
    
    ResourceSnapshot snapshot;
    snapshot.shard_id = "shard-0";
    snapshot.timestamp_ns = 12345ULL;
    snapshot.available_memory_bytes = 1024 * 1024 * 1024;
    snapshot.total_memory_bytes = 4ULL * 1024 * 1024 * 1024;
    snapshot.cpu_usage_percent = 50.0;
    snapshot.is_healthy = true;
    
    manager.publishResourceSnapshot(snapshot);
    
    // Verify snapshot was stored
    auto retrieved = manager.getResourceSnapshot("shard-0");
    EXPECT_EQ(retrieved.shard_id, "shard-0");
    EXPECT_EQ(retrieved.available_memory_bytes, 1024ULL * 1024 * 1024);
    EXPECT_DOUBLE_EQ(retrieved.cpu_usage_percent, 50.0);
}

TEST_F(GossipConfigManagerTest, MultipleShardIntegration) {
    // Test with 10+ shards as per acceptance criteria
    std::vector<std::unique_ptr<GossipConfigManager>> managers;
    
    for (int i = 0; i < 10; ++i) {
        GossipConfigManagerConfig config;
        config.enabled = false;
        config.local_shard_id = "shard-" + std::to_string(i);
        config.local_endpoint = "localhost:800" + std::to_string(i);
        
        managers.push_back(std::make_unique<GossipConfigManager>(config, topology_));
    }
    
    for (int i = 0; i < 10; ++i) {
        const std::string key = "shared.config." + std::to_string(i);
        const std::string value = "shared_value_" + std::to_string(i);
        std::string update_id = managers[i]->publishConfigUpdate(key, value);
        EXPECT_FALSE(update_id.empty());
        EXPECT_EQ(managers[i]->getConfig(key), value);
    }
}

TEST_F(GossipConfigManagerTest, VectorClockProgression) {
    GossipConfigManagerConfig config;
    config.enabled = false;
    config.local_shard_id = "shard-0";
    config.local_endpoint = "localhost:8000";
    
    GossipConfigManager manager(config, topology_);
    
    auto clock_before = manager.getVectorClock();
    uint64_t initial_value = clock_before.get("shard-0");
    
    // Publish an update (should increment clock)
    manager.publishConfigUpdate("test.key", "test_value");
    
    auto clock_after = manager.getVectorClock();
    uint64_t after_value = clock_after.get("shard-0");
    
    EXPECT_GT(after_value, initial_value);
}

TEST_F(GossipConfigManagerTest, GetAllConfigs) {
    GossipConfigManagerConfig config;
    config.enabled = false;
    config.local_shard_id = "shard-0";
    config.local_endpoint = "localhost:8000";
    
    GossipConfigManager manager(config, topology_);
    
    manager.publishConfigUpdate("config.key1", "value1");
    manager.publishConfigUpdate("config.key2", "value2");
    manager.publishConfigUpdate("config.key3", "value3");
    
    auto all_configs = manager.getAllConfigs();
    
    EXPECT_EQ(all_configs.size(), 3ULL);
    EXPECT_EQ(all_configs["config.key1"], "value1");
    EXPECT_EQ(all_configs["config.key2"], "value2");
    EXPECT_EQ(all_configs["config.key3"], "value3");
}

TEST_F(GossipConfigManagerTest, GetStatistics) {
    GossipConfigManagerConfig config;
    config.enabled = false;
    config.local_shard_id = "shard-0";
    config.local_endpoint = "localhost:8000";
    
    GossipConfigManager manager(config, topology_);
    
    manager.publishConfigUpdate("stat.test", "value");
    
    auto stats = manager.getStatistics();
    
    EXPECT_GE(stats.config_updates_sent, 1ULL);
}

TEST_F(GossipConfigManagerTest, ResourceSnapshotCallback) {
    GossipConfigManagerConfig config;
    config.enabled = false;
    config.local_shard_id = "shard-0";
    config.local_endpoint = "localhost:8000";
    
    GossipConfigManager manager(config, topology_);
    
    bool callback_called = false;
    std::string received_shard_id;
    
    manager.onResourceSnapshot([&](const ResourceSnapshot& snapshot) {
        callback_called = true;
        received_shard_id = snapshot.shard_id;
    });
    
    ResourceSnapshot snapshot;
    snapshot.shard_id = "shard-5";
    snapshot.timestamp_ns = 999;
    snapshot.is_healthy = true;
    
    manager.publishResourceSnapshot(snapshot);
    
    EXPECT_TRUE(callback_called);
    EXPECT_EQ(received_shard_id, "shard-5");
}

TEST_F(GossipConfigManagerTest, PublishConfigUpdateWithZeroTtlDoesNotApplyLocally) {
    GossipConfigManagerConfig config;
    config.enabled = false;
    config.local_shard_id = "shard-0";
    config.local_endpoint = "localhost:8000";
    config.update_ttl = 0;

    GossipConfigManager manager(config, topology_);

    manager.publishConfigUpdate("ttl.key", "new-value");
    EXPECT_EQ(manager.getConfig("ttl.key"), "");

    auto stats = manager.getStatistics();
    EXPECT_GE(stats.config_updates_sent, 1ULL);
}

TEST_F(GossipConfigManagerTest, ResourceSnapshotKeepsNewerTimestamp) {
    GossipConfigManagerConfig config;
    config.enabled = false;
    config.local_shard_id = "shard-0";
    config.local_endpoint = "localhost:8000";

    GossipConfigManager manager(config, topology_);

    ResourceSnapshot newer;
    newer.shard_id = "shard-1";
    newer.timestamp_ns = 2000ULL;
    newer.cpu_usage_percent = 42.0;
    newer.is_healthy = true;

    ResourceSnapshot older;
    older.shard_id = "shard-1";
    older.timestamp_ns = 1000ULL;
    older.cpu_usage_percent = 99.0;
    older.is_healthy = false;

    manager.publishResourceSnapshot(newer);
    manager.publishResourceSnapshot(older);

    auto snapshot = manager.getResourceSnapshot("shard-1");
    EXPECT_EQ(snapshot.timestamp_ns, 2000ULL);
    EXPECT_DOUBLE_EQ(snapshot.cpu_usage_percent, 42.0);
    EXPECT_TRUE(snapshot.is_healthy);
}
