/*
 * ThemisDB | Test: test_gossip_config_manager_focused.cpp | Version: 0.0.47
 * Focused Unit Tests for W2-S05: Gossip Config Manager Hardening
 */

#include <gtest/gtest.h>

#include "sharding/gossip_config_manager.h"
#include "sharding/shard_topology.h"

#include <chrono>
#include <memory>

namespace themis { namespace sharding { 

class GossipConfigManagerFocusedTest : public ::testing::Test {
protected:
    void SetUp() override {
        topology_ = std::make_shared<ShardTopology>();

        ShardInfo local;
        local.shard_id = "shard-0";
        local.primary_endpoint = "localhost:8000";
        local.is_healthy = true;
        topology_->addShard(local);

        ShardInfo peer;
        peer.shard_id = "shard-1";
        peer.primary_endpoint = "localhost:8001";
        peer.is_healthy = true;
        topology_->addShard(peer);

        config_.enabled = false;
        config_.local_shard_id = "shard-0";
        config_.local_endpoint = "localhost:8000";
        config_.gossip_interval_ms = 100;
        config_.fanout = 1;
        config_.update_ttl = 10;
    }

    GossipConfigManager createManager() const {
        return GossipConfigManager(config_, topology_);
    }

    static uint64_t nowNs() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    }

    std::shared_ptr<ShardTopology> topology_;
    GossipConfigManagerConfig config_;
};

TEST_F(GossipConfigManagerFocusedTest, PublishConfigUpdateRejectsEmptyKey) {
    auto manager = createManager();
    EXPECT_TRUE(manager.publishConfigUpdate("", "value").empty());
}

TEST_F(GossipConfigManagerFocusedTest, PublishConfigUpdateRejectsEmptyValue) {
    auto manager = createManager();
    EXPECT_TRUE(manager.publishConfigUpdate("feature.flag", "").empty());
}

TEST_F(GossipConfigManagerFocusedTest, PublishConfigUpdateAcceptsValidInput) {
    auto manager = createManager();
    const std::string update_id = manager.publishConfigUpdate("feature.flag", "enabled");
    EXPECT_FALSE(update_id.empty());
    EXPECT_EQ(manager.getConfig("feature.flag"), "enabled");
}

TEST_F(GossipConfigManagerFocusedTest, PublishResourceSnapshotRejectsEmptyShard) {
    auto manager = createManager();

    ResourceSnapshot invalid;
    invalid.shard_id = "";
    invalid.timestamp_ns = nowNs();
    invalid.available_memory_bytes = 1;
    invalid.total_memory_bytes = 1;

    manager.publishResourceSnapshot(invalid);
    EXPECT_TRUE(manager.getResourceSnapshot("shard-0").shard_id.empty());
}

TEST_F(GossipConfigManagerFocusedTest, PublishResourceSnapshotRejectsInvalidMetrics) {
    auto manager = createManager();

    ResourceSnapshot invalid;
    invalid.shard_id = "shard-1";
    invalid.timestamp_ns = nowNs();
    invalid.available_memory_bytes = 200;
    invalid.total_memory_bytes = 100;
    invalid.available_disk_bytes = 300;
    invalid.total_disk_bytes = 100;
    invalid.available_cpu_cores = 8;
    invalid.total_cpu_cores = 4;

    manager.publishResourceSnapshot(invalid);
    EXPECT_TRUE(manager.getResourceSnapshot("shard-1").shard_id.empty());
}

TEST_F(GossipConfigManagerFocusedTest, PublishResourceSnapshotAcceptsValidSnapshot) {
    auto manager = createManager();

    ResourceSnapshot valid;
    valid.shard_id = "shard-1";
    valid.timestamp_ns = nowNs();
    valid.available_memory_bytes = 64;
    valid.total_memory_bytes = 128;
    valid.available_disk_bytes = 256;
    valid.total_disk_bytes = 512;
    valid.available_cpu_cores = 2;
    valid.total_cpu_cores = 4;
    valid.is_healthy = true;

    manager.publishResourceSnapshot(valid);

    const ResourceSnapshot stored = manager.getResourceSnapshot("shard-1");
    EXPECT_EQ(stored.shard_id, "shard-1");
    EXPECT_EQ(stored.total_memory_bytes, 128ULL);
}
} } // namespace themis::sharding
