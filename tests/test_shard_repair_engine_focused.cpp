#include <gtest/gtest.h>
#include "sharding/shard_repair_engine.h"
#include "sharding/shard_topology.h"
#include "sharding/consistent_hash.h"
#include "sharding/redundancy_strategy.h"
#include <memory>
#include <vector>

using namespace themis::sharding;

// No-op read handler: all reads return "not found".
static const RedundancyStrategy::ReadHandler kNullReadHandler =
    [](const std::string&, const std::string&) -> std::optional<std::vector<uint8_t>> {
        return std::nullopt;
    };

// Always-succeed write handler.
static const RedundancyStrategy::WriteHandler kAlwaysSucceedWriteHandler =
    [](const std::string&, const std::string&, const std::vector<uint8_t>&) -> bool {
        return true;
    };

namespace themis { namespace sharding { 

class ShardRepairEngineTest : public ::testing::Test {
protected:
    std::shared_ptr<ShardTopology> topology_;
    std::shared_ptr<ConsistentHashRing> ring_;
    std::unique_ptr<RedundancyStrategy> strategy_;
    std::unique_ptr<ShardRepairEngine> engine_;

    void SetUp() override {
        // Create minimal topology
        ShardTopology::Config topo_cfg;
        topo_cfg.enable_health_checks = false;
        topo_cfg.cluster_name = "repair-test-focused";
        topology_ = std::make_shared<ShardTopology>(topo_cfg);

        // Add three test shards (for replication_factor=3)
        for (int i = 1; i <= 3; ++i) {
            ShardInfo s;
            s.shard_id = "shard_" + std::to_string(i);
            s.primary_endpoint = "localhost:" + std::to_string(9080 + i);
            s.is_healthy = true;
            topology_->addShard(s);
        }

        // Create consistent hash ring with all three shards
        ring_ = std::make_shared<ConsistentHashRing>();
        for (int i = 1; i <= 3; ++i) {
            ring_->addShard("shard_" + std::to_string(i), 150);
        }

        // Create redundancy strategy
        RedundancyConfig cfg;
        cfg.mode = RedundancyMode::MIRROR;
        cfg.replication_factor = 3;
        cfg.write_quorum = 2;  // Must be <= replication_factor
        strategy_ = std::make_unique<RedundancyStrategy>(cfg);

        // Create repair engine with minimal config
        RepairConfig repair_cfg;
        repair_cfg.default_collection = "test_collection";
        repair_cfg.enable_periodic_scan = false;  // Manual control for testing
        repair_cfg.enable_auto_repair = false;     // Disable background threads
        repair_cfg.repair_poll_interval = std::chrono::seconds(1);

        engine_ = std::make_unique<ShardRepairEngine>(
            repair_cfg,
            *strategy_,
            *ring_,
            *topology_,
            kNullReadHandler,
            kAlwaysSucceedWriteHandler
        );
    }

    void TearDown() override {
        if (engine_ && engine_->isRunning()) {
            engine_->stop();
        }
    }
};

// Test 1: Fail-closed guard rejects empty document_id
TEST_F(ShardRepairEngineTest, TriggerDocumentRepairFailsClosedForEmptyDocumentId) {
    // Attempt to trigger repair with empty document_id
    std::string job_id = engine_->triggerDocumentRepair("");
    
    // Verify fail-closed: empty job_id returned
    EXPECT_TRUE(job_id.empty());
}

// Test 2: Valid document_id returns non-empty job_id
TEST_F(ShardRepairEngineTest, TriggerDocumentRepairAcceptsValidDocumentId) {
    // Trigger repair with valid document_id
    std::string job_id = engine_->triggerDocumentRepair("doc_123");
    
    // Verify non-empty job_id returned
    EXPECT_FALSE(job_id.empty());
    
    // Verify job was created with correct document_id
    auto job = engine_->getJobStatus(job_id);
    EXPECT_EQ(job.document_id, "doc_123");
    EXPECT_FALSE(job.error_message.find("not found") != std::string::npos);  // Job should exist
}

// Test 3: Empty collection parameter uses default_collection
TEST_F(ShardRepairEngineTest, TriggerDocumentRepairWithDefaultCollection) {
    // Trigger repair with valid document_id and empty collection
    std::string job_id = engine_->triggerDocumentRepair("doc_456", "");
    
    // Verify job was created
    EXPECT_FALSE(job_id.empty());
    
    // Verify job uses default collection
    auto job = engine_->getJobStatus(job_id);
    EXPECT_EQ(job.collection, "test_collection");
}

// Test 4: Specific collection parameter is preserved
TEST_F(ShardRepairEngineTest, TriggerDocumentRepairWithSpecificCollection) {
    // Trigger repair with specific collection
    std::string job_id = engine_->triggerDocumentRepair("doc_789", "custom_collection");
    
    // Verify job was created with specific collection
    EXPECT_FALSE(job_id.empty());
    auto job = engine_->getJobStatus(job_id);
    EXPECT_EQ(job.collection, "custom_collection");
}

// Test 5: Multiple fail-closed guards (verify no regression)
TEST_F(ShardRepairEngineTest, MultipleFailClosedGsGuardsAreIndependent) {
    // First call with empty document_id should fail-close
    std::string job_id1 = engine_->triggerDocumentRepair("");
    EXPECT_TRUE(job_id1.empty());
    
    // Second call with valid document_id should succeed (not affected by first call)
    std::string job_id2 = engine_->triggerDocumentRepair("valid_doc");
    EXPECT_FALSE(job_id2.empty());
    
    // Third call with empty document_id should also fail-close (consistent)
    std::string job_id3 = engine_->triggerDocumentRepair("");
    EXPECT_TRUE(job_id3.empty());
}
} } // namespace themis::sharding
