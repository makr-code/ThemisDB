/**
 * Unit tests for HardwareMigrationManager.
 *
 * Core guarantees tested:
 *  1. Node identity round-trips through JSON and through disk.
 *  2. Endpoint replacement succeeds and updates only the endpoint field.
 *  3. Hash-ring positions are UNCHANGED after an endpoint replacement.
 *  4. Missing shard_id in topology is rejected gracefully.
 *  5. Empty shard_id / empty endpoint are rejected gracefully.
 *  6. createAndSaveIdentity refuses to overwrite an existing file.
 */

#include <gtest/gtest.h>
#include "sharding/hardware_migration_manager.h"
#include "sharding/consistent_hash.h"
#include "sharding/shard_topology.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <memory>

using namespace themis::sharding;
namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
// ─────────────────────────────────────────────────────────────────────────────

class HardwareMigrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        tmp_dir_ = fs::temp_directory_path() / "themis_hw_migration_test";
        fs::create_directories(tmp_dir_);
        identity_path_ = (tmp_dir_ / "node_identity.json").string();

        topology_ = std::make_shared<ShardTopology>();
        ring_     = std::make_shared<ConsistentHashRing>(150);

        HardwareMigrationConfig cfg;
        cfg.identity_file_path  = identity_path_;
        cfg.drain_period        = std::chrono::seconds{0};
        cfg.verify_ring_stability = true;

        manager_ = std::make_unique<HardwareMigrationManager>(cfg, topology_, ring_);
    }

    void TearDown() override {
        fs::remove_all(tmp_dir_);
    }

    /** Add a shard to both the topology and the hash ring. */
    void addShard(const std::string& shard_id,
                  const std::string& endpoint,
                  size_t virtual_nodes = 150) {
        ShardInfo info{};
        info.shard_id        = shard_id;
        info.primary_endpoint = endpoint;
        info.is_healthy      = true;
        topology_->addShard(info);
        ring_->addShard(shard_id, virtual_nodes);
    }

    fs::path                                tmp_dir_;
    std::string                             identity_path_;
    std::shared_ptr<ShardTopology>          topology_;
    std::shared_ptr<ConsistentHashRing>     ring_;
    std::unique_ptr<HardwareMigrationManager> manager_;
};

// ─────────────────────────────────────────────────────────────────────────────
// NodeIdentity serialisation / persistence
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(HardwareMigrationTest, NodeIdentityJsonRoundTrip) {
    NodeIdentity id;
    id.shard_id         = "shard_001";
    id.cluster_name     = "cluster_a";
    id.token_start      = 100;
    id.token_end        = 200;
    id.created_at       = "2026-03-01T12:00:00Z";
    id.identity_version = "1";

    auto json = id.toJson();
    auto maybe = NodeIdentity::fromJson(json);

    ASSERT_TRUE(maybe.has_value());
    EXPECT_EQ(maybe->shard_id,         "shard_001");
    EXPECT_EQ(maybe->cluster_name,     "cluster_a");
    EXPECT_EQ(maybe->token_start,      100ULL);
    EXPECT_EQ(maybe->token_end,        200ULL);
    EXPECT_EQ(maybe->created_at,       "2026-03-01T12:00:00Z");
    EXPECT_EQ(maybe->identity_version, "1");
}

TEST_F(HardwareMigrationTest, NodeIdentityFromInvalidJsonReturnsNullopt) {
    EXPECT_FALSE(NodeIdentity::fromJson("not json").has_value());
    EXPECT_FALSE(NodeIdentity::fromJson("{}").has_value());          // missing shard_id
    EXPECT_FALSE(NodeIdentity::fromJson("{\"shard_id\":\"\"}").has_value()); // empty shard_id
}

TEST_F(HardwareMigrationTest, NodeIdentityDiskRoundTrip) {
    NodeIdentity id;
    id.shard_id         = "shard_002";
    id.cluster_name     = "prod_cluster";
    id.token_start      = 500;
    id.token_end        = 1000;
    id.created_at       = "2026-03-01T00:00:00Z";
    id.identity_version = "1";

    std::string path = (tmp_dir_ / "roundtrip_identity.json").string();
    ASSERT_TRUE(id.saveTo(path));

    auto loaded = NodeIdentity::loadFrom(path);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->shard_id,     "shard_002");
    EXPECT_EQ(loaded->token_start,  500ULL);
    EXPECT_EQ(loaded->token_end,    1000ULL);
}

TEST_F(HardwareMigrationTest, LoadIdentityReturnsNulloptIfFileAbsent) {
    EXPECT_FALSE(manager_->loadIdentity().has_value());
}

TEST_F(HardwareMigrationTest, CreateAndSaveIdentityWritesToDisk) {
    auto maybe = manager_->createAndSaveIdentity("shard_003", "mycluster", 0, 9999);
    ASSERT_TRUE(maybe.has_value());
    EXPECT_EQ(maybe->shard_id, "shard_003");
    EXPECT_TRUE(fs::exists(identity_path_));

    auto loaded = manager_->loadIdentity();
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->shard_id, "shard_003");
}

TEST_F(HardwareMigrationTest, CreateAndSaveIdentityRefusesOverwrite) {
    // Create once — should succeed.
    ASSERT_TRUE(manager_->createAndSaveIdentity("shard_004", "cluster_x", 0, 100).has_value());

    // Create again — should fail because the file already exists.
    EXPECT_FALSE(manager_->createAndSaveIdentity("shard_004", "cluster_x", 0, 100).has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// Endpoint replacement
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(HardwareMigrationTest, ReplaceEndpointSucceeds) {
    addShard("shard_001", "old.host:8080");

    auto result = manager_->replaceEndpoint("shard_001", "new.host:8080");

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.shard_id,     "shard_001");
    EXPECT_EQ(result.old_endpoint, "old.host:8080");
    EXPECT_EQ(result.new_endpoint, "new.host:8080");

    // Verify topology was updated.
    auto info = topology_->getShard("shard_001");
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->primary_endpoint, "new.host:8080");
}

TEST_F(HardwareMigrationTest, ReplaceEndpointPreservesOtherFields) {
    ShardInfo info{};
    info.shard_id          = "shard_005";
    info.primary_endpoint  = "host-a:9000";
    info.datacenter        = "dc1";
    info.rack              = "rack-02";
    info.token_start       = 123;
    info.token_end         = 456;
    info.is_healthy        = true;
    topology_->addShard(info);
    ring_->addShard("shard_005", 150);

    auto result = manager_->replaceEndpoint("shard_005", "host-b:9000");
    ASSERT_TRUE(result.success);

    auto updated = topology_->getShard("shard_005");
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->primary_endpoint, "host-b:9000");
    // All other fields unchanged:
    EXPECT_EQ(updated->datacenter,   "dc1");
    EXPECT_EQ(updated->rack,         "rack-02");
    EXPECT_EQ(updated->token_start,  123ULL);
    EXPECT_EQ(updated->token_end,    456ULL);
    EXPECT_TRUE(updated->is_healthy);
}

TEST_F(HardwareMigrationTest, ReplaceEndpointFailsForUnknownShard) {
    auto result = manager_->replaceEndpoint("nonexistent_shard", "new.host:8080");
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.message, "");
}

TEST_F(HardwareMigrationTest, ReplaceEndpointFailsForEmptyShardId) {
    auto result = manager_->replaceEndpoint("", "new.host:8080");
    EXPECT_FALSE(result.success);
}

TEST_F(HardwareMigrationTest, ReplaceEndpointFailsForEmptyNewEndpoint) {
    addShard("shard_010", "old.host:8080");
    auto result = manager_->replaceEndpoint("shard_010", "");
    EXPECT_FALSE(result.success);

    // Topology should be unchanged.
    auto info = topology_->getShard("shard_010");
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->primary_endpoint, "old.host:8080");
}

// ─────────────────────────────────────────────────────────────────────────────
// Ring stability
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(HardwareMigrationTest, RingPositionsUnchangedAfterEndpointReplacement) {
    addShard("shard_a", "host-a:8080");
    addShard("shard_b", "host-b:8080");

    // Capture ring state before migration.
    auto before = manager_->captureRingSnapshot();
    size_t before_total = ring_->getVirtualNodeCount();

    manager_->replaceEndpoint("shard_a", "host-a-new:8080");

    // Ring state must be identical.
    auto after       = manager_->captureRingSnapshot();
    size_t after_total = ring_->getVirtualNodeCount();

    EXPECT_EQ(before_total, after_total)
        << "Total virtual node count changed after endpoint replacement";

    EXPECT_EQ(before.size(), after.size())
        << "Number of shards in ring changed after endpoint replacement";

    // validateRingStability with the shards that were there before.
    std::vector<std::string> shards = {"shard_a", "shard_b"};
    EXPECT_TRUE(manager_->validateRingStability(shards, before));
}

TEST_F(HardwareMigrationTest, RingStabilityVerifiedFlagSetOnSuccess) {
    addShard("shard_x", "old.host:7000");

    auto result = manager_->replaceEndpoint("shard_x", "new.host:7000");
    ASSERT_TRUE(result.success);
    EXPECT_TRUE(result.ring_stability_verified);
}

TEST_F(HardwareMigrationTest, CaptureRingSnapshotIsConsistentForKnownShards) {
    const int N_SHARDS = 5;
    for (int i = 0; i < N_SHARDS; ++i) {
        addShard("shard_" + std::to_string(i), "host" + std::to_string(i) + ":8080");
    }

    auto snapshot = manager_->captureRingSnapshot();
    EXPECT_EQ(snapshot.size(), static_cast<size_t>(N_SHARDS));

    for (int i = 0; i < N_SHARDS; ++i) {
        EXPECT_TRUE(snapshot.count("shard_" + std::to_string(i)) > 0);
    }
}

TEST_F(HardwareMigrationTest, MultipleEndpointReplacementsKeepRingStable) {
    addShard("node_1", "10.0.0.1:8080");
    addShard("node_2", "10.0.0.2:8080");
    addShard("node_3", "10.0.0.3:8080");

    auto before = manager_->captureRingSnapshot();

    // Migrate all three nodes to new hardware.
    ASSERT_TRUE(manager_->replaceEndpoint("node_1", "10.1.0.1:8080").success);
    ASSERT_TRUE(manager_->replaceEndpoint("node_2", "10.1.0.2:8080").success);
    ASSERT_TRUE(manager_->replaceEndpoint("node_3", "10.1.0.3:8080").success);

    std::vector<std::string> shards = {"node_1", "node_2", "node_3"};
    EXPECT_TRUE(manager_->validateRingStability(shards, before));
}
