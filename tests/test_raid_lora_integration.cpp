/**
 * ThemisDB RAID Mode LoRA Integration Tests
 * 
 * Comprehensive tests for RAID modes with LoRA storage including:
 * - RAID 0 (STRIPE) - Data striping
 * - RAID 1 (MIRROR) - Data mirroring
 * - RAID 5 (PARITY) - Striping with parity
 * - RAID 10 (HYBRID) - Striping + mirroring
 * - Failure scenarios and recovery
 */

#include <gtest/gtest.h>
#include "fixtures/mock_shard_cluster.h"
#include "utils/raid_simulator.h"
#include "utils/shard_failure_injector.h"
#include "llm/lora_framework/lora_config.h"
#include <algorithm>

using namespace themis::test;
using namespace themis::llm::lora;

// ═══════════════════════════════════════════════════════════
// Test Fixtures
// ═══════════════════════════════════════════════════════════

class RAIDLoRAIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create 6-shard cluster for RAID tests
        cluster_ = std::make_unique<MockShardCluster>(6);
        cluster_->setLatency(0, 0);  // Disable latency for faster tests
    }
    
    void TearDown() override {
        cluster_.reset();
    }
    
    // Helper to create test LoRA data
    std::vector<uint8_t> createTestLoRAData(size_t size, uint8_t fill_value = 0) {
        std::vector<uint8_t> data(size);
        for (size_t i = 0; i < size; ++i) {
            data[i] = fill_value != 0 ? fill_value : static_cast<uint8_t>(i % 256);
        }
        return data;
    }
    
    // Helper to save striped data
    bool saveStriped(const std::string& key, const std::vector<uint8_t>& data,
                     RAIDSimulator& raid) {
        auto chunks = raid.stripe(data);
        for (size_t i = 0; i < chunks.size() && i < static_cast<size_t>(cluster_->getShardCount()); ++i) {
            std::string chunk_key = key + ":chunk:" + std::to_string(i);
            if (!cluster_->saveToShard(static_cast<int>(i), chunk_key, chunks[i])) {
                return false;
            }
        }
        return true;
    }
    
    // Helper to load striped data
    std::optional<std::vector<uint8_t>> loadStriped(const std::string& key, RAIDSimulator& raid) {
        std::vector<std::optional<std::vector<uint8_t>>> chunks;
        chunks.resize(raid.getShardCount());
        
        for (int i = 0; i < raid.getShardCount(); ++i) {
            std::string chunk_key = key + ":chunk:" + std::to_string(i);
            chunks[i] = cluster_->loadFromShard(i, chunk_key);
        }
        
        return raid.reconstruct(chunks);
    }
    
    std::unique_ptr<MockShardCluster> cluster_;
};

// ═══════════════════════════════════════════════════════════
// RAID 0 (STRIPE) Tests
// ═══════════════════════════════════════════════════════════

TEST_F(RAIDLoRAIntegrationTest, RAID0_StripeLargeLoRA) {
    RAIDSimulator raid(RAIDMode::STRIPE, 4);
    
    // Create large LoRA (16KB)
    auto lora_data = createTestLoRAData(16384);
    std::string key = "lora:raid0-large";
    
    // Stripe across 4 shards
    ASSERT_TRUE(saveStriped(key, lora_data, raid));
    
    // Verify chunks exist on each shard
    for (int i = 0; i < 4; ++i) {
        std::string chunk_key = key + ":chunk:" + std::to_string(i);
        EXPECT_TRUE(cluster_->existsInShard(i, chunk_key))
            << "Chunk " << i << " not found";
    }
    
    // Load and verify complete data
    auto loaded = loadStriped(key, raid);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(*loaded, lora_data);
}

TEST_F(RAIDLoRAIntegrationTest, RAID0_SingleShardFailureLosesData) {
    RAIDSimulator raid(RAIDMode::STRIPE, 4);
    
    auto lora_data = createTestLoRAData(8192);
    std::string key = "lora:raid0-failure";
    
    // Save striped data
    ASSERT_TRUE(saveStriped(key, lora_data, raid));
    
    // Fail one shard
    cluster_->failShard(1);
    
    // Should not be able to reconstruct
    auto loaded = loadStriped(key, raid);
    EXPECT_FALSE(loaded.has_value()) << "RAID 0 should fail with any shard loss";
}

TEST_F(RAIDLoRAIntegrationTest, RAID0_UniformDistribution) {
    RAIDSimulator raid(RAIDMode::STRIPE, 4);
    
    auto lora_data = createTestLoRAData(4096);
    std::string key = "lora:raid0-distribution";
    
    ASSERT_TRUE(saveStriped(key, lora_data, raid));
    
    // Check chunk sizes are relatively uniform
    std::vector<size_t> chunk_sizes = {};

    for (int i = 0; i < 4; ++i) {
        std::string chunk_key = key + ":chunk:" + std::to_string(i);
        auto chunk = cluster_->loadFromShard(i, chunk_key);
        ASSERT_TRUE(chunk.has_value());
        chunk_sizes.push_back(chunk->size());
    }
    
    // Chunks should be similar in size (within 20% of average)
    size_t total_size = 0;
    for (size_t size : chunk_sizes) {
        total_size += size;
    }
    size_t avg_size = total_size / chunk_sizes.size();
    
    for (size_t size : chunk_sizes) {
        EXPECT_NEAR(size, avg_size, avg_size * 0.2);
    }
}

TEST_F(RAIDLoRAIntegrationTest, RAID0_TolerateZeroFailures) {
    RAIDSimulator raid(RAIDMode::STRIPE, 4);
    EXPECT_EQ(raid.getMaxTolerableFailures(), 0);
}

// ═══════════════════════════════════════════════════════════
// RAID 1 (MIRROR) Tests
// ═══════════════════════════════════════════════════════════

TEST_F(RAIDLoRAIntegrationTest, RAID1_ThreeWayReplication) {
    RAIDSimulator raid(RAIDMode::MIRROR, 3);
    
    auto lora_data = createTestLoRAData(2048, 0xCC);
    std::string key = "lora:raid1-replicated";
    
    // Mirror to 3 shards
    auto replicas = raid.mirror(lora_data, 3);
    EXPECT_EQ(replicas.size(), 3);
    
    for (size_t i = 0; i < replicas.size(); ++i) {
        bool saved = cluster_->saveToShard(static_cast<int>(i), key, replicas[i]);
        EXPECT_TRUE(saved);
    }
    
    // Verify all replicas are identical
    for (int i = 0; i < 3; ++i) {
        auto loaded = cluster_->loadFromShard(i, key);
        ASSERT_TRUE(loaded.has_value());
        EXPECT_EQ(*loaded, lora_data);
    }
}

TEST_F(RAIDLoRAIntegrationTest, RAID1_SingleShardFailureStillReadable) {
    RAIDSimulator raid(RAIDMode::MIRROR, 3);
    
    auto lora_data = createTestLoRAData(2048);
    std::string key = "lora:raid1-fault-tolerant";
    
    // Replicate to 3 shards
    auto replicas = raid.mirror(lora_data, 3);
    for (size_t i = 0; i < replicas.size(); ++i) {
        cluster_->saveToShard(static_cast<int>(i), key, replicas[i]);
    }
    
    // Fail shard 0
    cluster_->failShard(0);
    
    // Should still be readable from shards 1 and 2
    auto from_shard_1 = cluster_->loadFromShard(1, key);
    auto from_shard_2 = cluster_->loadFromShard(2, key);
    
    ASSERT_TRUE(from_shard_1.has_value());
    ASSERT_TRUE(from_shard_2.has_value());
    EXPECT_EQ(*from_shard_1, lora_data);
    EXPECT_EQ(*from_shard_2, lora_data);
}

TEST_F(RAIDLoRAIntegrationTest, RAID1_TwoShardFailuresStillReadable) {
    RAIDSimulator raid(RAIDMode::MIRROR, 3);
    
    auto lora_data = createTestLoRAData(2048);
    std::string key = "lora:raid1-two-failures";
    
    // Replicate to 3 shards
    auto replicas = raid.mirror(lora_data, 3);
    for (size_t i = 0; i < replicas.size(); ++i) {
        cluster_->saveToShard(static_cast<int>(i), key, replicas[i]);
    }
    
    // Fail 2 out of 3 shards
    cluster_->failShard(0);
    cluster_->failShard(1);
    
    // Should still be readable from shard 2
    auto loaded = cluster_->loadFromShard(2, key);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(*loaded, lora_data);
}

TEST_F(RAIDLoRAIntegrationTest, RAID1_AllShardsFailDataLost) {
    RAIDSimulator raid(RAIDMode::MIRROR, 3);
    
    auto lora_data = createTestLoRAData(2048);
    std::string key = "lora:raid1-total-failure";
    
    // Replicate to 3 shards
    auto replicas = raid.mirror(lora_data, 3);
    for (size_t i = 0; i < replicas.size(); ++i) {
        cluster_->saveToShard(static_cast<int>(i), key, replicas[i]);
    }
    
    // Fail all shards
    cluster_->failShard(0);
    cluster_->failShard(1);
    cluster_->failShard(2);
    
    // Data should be inaccessible
    for (int i = 0; i < 3; ++i) {
        EXPECT_FALSE(cluster_->loadFromShard(i, key).has_value());
    }
}

// ═══════════════════════════════════════════════════════════
// RAID 5 (PARITY) Tests
// ═══════════════════════════════════════════════════════════

TEST_F(RAIDLoRAIntegrationTest, RAID5_EncodeWithParity) {
    RAIDSimulator raid(RAIDMode::PARITY, 4);  // 3 data + 1 parity
    
    auto lora_data = createTestLoRAData(3072);
    std::string key = "lora:raid5-encoded";
    
    // Encode with parity
    auto chunks = raid.encodeWithParity(lora_data);
    EXPECT_EQ(chunks.size(), 4);  // 3 data + 1 parity
    
    // Save all chunks
    for (size_t i = 0; i < chunks.size(); ++i) {
        std::string chunk_key = key + ":chunk:" + std::to_string(i);
        ASSERT_TRUE(cluster_->saveToShard(static_cast<int>(i), chunk_key, chunks[i]));
    }
}

TEST_F(RAIDLoRAIntegrationTest, RAID5_RecoverFromSingleFailure) {
    RAIDSimulator raid(RAIDMode::PARITY, 4);
    
    auto lora_data = createTestLoRAData(4096);
    std::string key = "lora:raid5-recovery";
    
    // Encode and save
    auto chunks = raid.encodeWithParity(lora_data);
    for (size_t i = 0; i < chunks.size(); ++i) {
        std::string chunk_key = key + ":chunk:" + std::to_string(i);
        cluster_->saveToShard(static_cast<int>(i), chunk_key, chunks[i]);
    }
    
    // Fail one shard
    cluster_->failShard(1);
    
    // Load chunks (with one missing)
    std::vector<std::optional<std::vector<uint8_t>>> loaded_chunks;
    loaded_chunks.resize(4);
    for (int i = 0; i < 4; ++i) {
        std::string chunk_key = key + ":chunk:" + std::to_string(i);
        loaded_chunks[i] = cluster_->loadFromShard(i, chunk_key);
    }
    
    // Should be able to reconstruct
    auto reconstructed = raid.reconstruct(loaded_chunks);
    ASSERT_TRUE(reconstructed.has_value());
    
    // Verify data matches (may have some padding)
    EXPECT_GE(reconstructed->size(), lora_data.size());
    for (size_t i = 0; i < lora_data.size(); ++i) {
        EXPECT_EQ((*reconstructed)[i], lora_data[i]) << "Mismatch at byte " << i;
    }
}

TEST_F(RAIDLoRAIntegrationTest, RAID5_CannotRecoverFromTwoFailures) {
    RAIDSimulator raid(RAIDMode::PARITY, 4);
    
    auto lora_data = createTestLoRAData(3072);
    std::string key = "lora:raid5-two-failures";
    
    // Encode and save
    auto chunks = raid.encodeWithParity(lora_data);
    for (size_t i = 0; i < chunks.size(); ++i) {
        std::string chunk_key = key + ":chunk:" + std::to_string(i);
        cluster_->saveToShard(static_cast<int>(i), chunk_key, chunks[i]);
    }
    
    // Fail two shards
    cluster_->failShard(1);
    cluster_->failShard(2);
    
    // Load chunks
    std::vector<std::optional<std::vector<uint8_t>>> loaded_chunks;
    loaded_chunks.resize(4);
    for (int i = 0; i < 4; ++i) {
        std::string chunk_key = key + ":chunk:" + std::to_string(i);
        loaded_chunks[i] = cluster_->loadFromShard(i, chunk_key);
    }
    
    // Should NOT be able to reconstruct with 2 missing chunks
    auto reconstructed = raid.reconstruct(loaded_chunks);
    EXPECT_FALSE(reconstructed.has_value());
}

TEST_F(RAIDLoRAIntegrationTest, RAID5_ParityCalculation) {
    RAIDSimulator raid(RAIDMode::PARITY, 4);
    
    // Create simple test data
    std::vector<uint8_t> lora_data = {0xAA, 0xBB, 0xCC, 0xDD};
    
    auto chunks = raid.encodeWithParity(lora_data);
    ASSERT_EQ(chunks.size(), 4);
    
    // Last chunk is parity - verify XOR property
    // (We don't expose calculateParity, so we verify through reconstruction)
    cluster_->failShard(0);
    
    // Simulate missing first chunk
    std::vector<std::optional<std::vector<uint8_t>>> test_chunks;
    test_chunks.push_back(std::nullopt);
    for (size_t i = 1; i < chunks.size(); ++i) {
        test_chunks.push_back(chunks[i]);
    }
    
    auto reconstructed = raid.reconstruct(test_chunks);
    ASSERT_TRUE(reconstructed.has_value());
}

TEST_F(RAIDLoRAIntegrationTest, RAID5_TolerateOneFailure) {
    RAIDSimulator raid(RAIDMode::PARITY, 4);
    EXPECT_EQ(raid.getMaxTolerableFailures(), 1);
    
    EXPECT_TRUE(raid.canRecover({0}));
    EXPECT_TRUE(raid.canRecover({3}));
    EXPECT_FALSE(raid.canRecover({0, 1}));
}

// ═══════════════════════════════════════════════════════════
// RAID 10 (HYBRID) Tests
// ═══════════════════════════════════════════════════════════

TEST_F(RAIDLoRAIntegrationTest, RAID10_StripedMirrors) {
    RAIDSimulator raid(RAIDMode::HYBRID, 4);  // 2 stripe groups, each mirrored
    
    auto lora_data = createTestLoRAData(4096);
    std::string key = "lora:raid10-hybrid";
    
    // For RAID 10, we stripe then mirror each stripe
    // Simulate with 2 stripes, each with 2 replicas
    auto striped = raid.stripe(lora_data);  // 2 chunks
    
    // Save stripe 0 to shards 0 and 1 (mirrors)
    cluster_->saveToShard(0, key + ":stripe0", striped[0]);
    cluster_->saveToShard(1, key + ":stripe0", striped[0]);
    
    // Save stripe 1 to shards 2 and 3 (mirrors)
    cluster_->saveToShard(2, key + ":stripe1", striped[1]);
    cluster_->saveToShard(3, key + ":stripe1", striped[1]);
    
    // Verify all mirrors exist
    EXPECT_TRUE(cluster_->existsInShard(0, key + ":stripe0"));
    EXPECT_TRUE(cluster_->existsInShard(1, key + ":stripe0"));
    EXPECT_TRUE(cluster_->existsInShard(2, key + ":stripe1"));
    EXPECT_TRUE(cluster_->existsInShard(3, key + ":stripe1"));
}

TEST_F(RAIDLoRAIntegrationTest, RAID10_SingleFailureInStripeGroup) {
    RAIDSimulator raid(RAIDMode::HYBRID, 4);
    
    auto lora_data = createTestLoRAData(2048);
    
    // Stripe and save with mirrors
    auto striped = raid.stripe(lora_data);
    
    // Mirror stripe 0
    cluster_->saveToShard(0, "stripe0", striped[0]);
    cluster_->saveToShard(1, "stripe0", striped[0]);
    
    // Mirror stripe 1
    cluster_->saveToShard(2, "stripe1", striped[1]);
    cluster_->saveToShard(3, "stripe1", striped[1]);
    
    // Fail shard 0 (one mirror of stripe 0)
    cluster_->failShard(0);
    
    // Should still be able to read from mirror
    auto stripe0_mirror = cluster_->loadFromShard(1, "stripe0");
    auto stripe1 = cluster_->loadFromShard(2, "stripe1");
    
    ASSERT_TRUE(stripe0_mirror.has_value());
    ASSERT_TRUE(stripe1.has_value());
    
    // Reconstruct
    std::vector<std::optional<std::vector<uint8_t>>> chunks = {
        stripe0_mirror, stripe1
    };
    auto reconstructed = raid.reconstruct(chunks);
    ASSERT_TRUE(reconstructed.has_value());
    EXPECT_EQ(*reconstructed, lora_data);
}

TEST_F(RAIDLoRAIntegrationTest, RAID10_MultipleFailuresAcrossStripeGroups) {
    RAIDSimulator raid(RAIDMode::HYBRID, 4);
    
    auto lora_data = createTestLoRAData(2048);
    auto striped = raid.stripe(lora_data);
    
    // Setup mirrors
    cluster_->saveToShard(0, "s0", striped[0]);
    cluster_->saveToShard(1, "s0", striped[0]);
    cluster_->saveToShard(2, "s1", striped[1]);
    cluster_->saveToShard(3, "s1", striped[1]);
    
    // Fail one shard from each stripe group
    cluster_->failShard(0);  // Stripe 0
    cluster_->failShard(2);  // Stripe 1
    
    // Should still work using remaining mirrors
    auto s0 = cluster_->loadFromShard(1, "s0");
    auto s1 = cluster_->loadFromShard(3, "s1");
    
    ASSERT_TRUE(s0.has_value());
    ASSERT_TRUE(s1.has_value());
}

TEST_F(RAIDLoRAIntegrationTest, RAID10_BothMirrorsFailDataLost) {
    RAIDSimulator raid(RAIDMode::HYBRID, 4);
    
    auto lora_data = createTestLoRAData(2048);
    auto striped = raid.stripe(lora_data);
    
    // Setup mirrors
    cluster_->saveToShard(0, "s0", striped[0]);
    cluster_->saveToShard(1, "s0", striped[0]);
    cluster_->saveToShard(2, "s1", striped[1]);
    cluster_->saveToShard(3, "s1", striped[1]);
    
    // Fail both mirrors of stripe 0
    cluster_->failShard(0);
    cluster_->failShard(1);
    
    // Cannot reconstruct without stripe 0
    auto s1 = cluster_->loadFromShard(2, "s1");
    ASSERT_TRUE(s1.has_value());
    
    std::vector<std::optional<std::vector<uint8_t>>> chunks = {
        std::nullopt, s1
    };
    auto reconstructed = raid.reconstruct(chunks);
    EXPECT_FALSE(reconstructed.has_value());
}

// ═══════════════════════════════════════════════════════════
// Failure Recovery Tests
// ═══════════════════════════════════════════════════════════

TEST_F(RAIDLoRAIntegrationTest, FailoverScenarioWithRecovery) {
    RAIDSimulator raid(RAIDMode::MIRROR, 3);
    ShardFailureInjector injector;
    
    auto lora_data = createTestLoRAData(2048);
    std::string key = "lora:failover-test";
    
    // Setup replication
    auto replicas = raid.mirror(lora_data, 3);
    for (size_t i = 0; i < replicas.size(); ++i) {
        cluster_->saveToShard(static_cast<int>(i), key, replicas[i]);
    }
    
    // Inject transient failure
    injector.injectFailure(1, ShardFailureInjector::FailureType::TRANSIENT,
                          std::chrono::seconds(1));
    cluster_->failShard(1);
    
    // Read from healthy shards
    auto from_0 = cluster_->loadFromShard(0, key);
    auto from_2 = cluster_->loadFromShard(2, key);
    
    ASSERT_TRUE(from_0.has_value());
    ASSERT_TRUE(from_2.has_value());
    
    // Recover shard
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    injector.update();
    cluster_->recoverShard(1);
    
    // Should now be accessible
    auto recovered = cluster_->loadFromShard(1, key);
    ASSERT_TRUE(recovered.has_value());
    EXPECT_EQ(*recovered, lora_data);
}

TEST_F(RAIDLoRAIntegrationTest, CascadingFailureDetection) {
    ShardFailureInjector injector;
    
    // Inject cascading failures
    auto scenarios = injector.injectCascadingFailures(0, 2, std::chrono::milliseconds(100));
    
    EXPECT_EQ(scenarios.size(), 3);  // Initial + 2 cascading
    
    // Verify failures
    EXPECT_TRUE(injector.isShardFailed(0));
}
