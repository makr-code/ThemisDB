#include <gtest/gtest.h>
#include "llm/lora_framework/adapter_consistency_checker.h"
#include "llm/lora_framework/adapter_sync_manager.h"
#include "llm/lora_framework/lora_storage_service.h"
#include "sharding/shard_topology.h"
#include <vector>
#include <chrono>
#include <thread>

using namespace themis::llm::lora;
using namespace themis::sharding;

/**
 * Test suite for Adapter Consistency Checker
 */
class AdapterConsistencyCheckerTest : public ::testing::Test {
protected:
    void SetUp() override {
        AdapterConsistencyChecker::Config config;
        config.enable_checksums = true;
        config.enable_signatures = true;
        checker_ = std::make_unique<AdapterConsistencyChecker>(config);
    }
    
    std::unique_ptr<AdapterConsistencyChecker> checker_;
};

TEST_F(AdapterConsistencyCheckerTest, CalculateChecksum) {
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    
    std::string checksum = checker_->calculateChecksum(data);
    
    EXPECT_FALSE(checksum.empty());
    EXPECT_EQ(checksum.length(), 64);  // SHA-256 hex is 64 chars
}

TEST_F(AdapterConsistencyCheckerTest, VerifyChecksum) {
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    
    std::string checksum = checker_->calculateChecksum(data);
    
    EXPECT_TRUE(checker_->verifyChecksum(data, checksum));
    EXPECT_FALSE(checker_->verifyChecksum(data, "invalid_checksum"));
}

TEST_F(AdapterConsistencyCheckerTest, ChecksumConsistency) {
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    
    std::string checksum1 = checker_->calculateChecksum(data);
    std::string checksum2 = checker_->calculateChecksum(data);
    
    EXPECT_EQ(checksum1, checksum2);
}

TEST_F(AdapterConsistencyCheckerTest, GenerateSignature) {
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    
    std::string signature = checker_->generateSignature(data);
    
    EXPECT_FALSE(signature.empty());
}

TEST_F(AdapterConsistencyCheckerTest, VerifySignature) {
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    
    std::string signature = checker_->generateSignature(data);
    
    EXPECT_TRUE(checker_->verifySignature(data, signature));
}

TEST_F(AdapterConsistencyCheckerTest, CheckAdapter) {
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    
    AdapterMetadata metadata;
    metadata.adapter_id = "test_adapter";
    metadata.base_model = "llama-2-7b";
    metadata.version = "1";
    metadata.updated_at = std::chrono::system_clock::now();
    metadata.checksum = checker_->calculateChecksum(data);
    metadata.signature = checker_->generateSignature(data);
    
    auto result = checker_->checkAdapter("test_adapter", data, metadata);
    
    EXPECT_TRUE(result.is_valid);
    EXPECT_TRUE(result.signature_valid);
    EXPECT_EQ(result.version, "1");
    EXPECT_FALSE(result.checksum.empty());
}

TEST_F(AdapterConsistencyCheckerTest, CheckAdapterInvalidChecksum) {
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    
    AdapterMetadata metadata;
    metadata.adapter_id = "test_adapter";
    metadata.version = "1";
    metadata.checksum = "invalid_checksum";
    
    auto result = checker_->checkAdapter("test_adapter", data, metadata);
    
    EXPECT_FALSE(result.is_valid);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(AdapterConsistencyCheckerTest, CompareVersions) {
    ConsistencyCheckResult local;
    local.version = "1";
    local.timestamp = 1000;
    
    ConsistencyCheckResult remote;
    remote.version = "2";
    remote.timestamp = 2000;
    
    EXPECT_EQ(checker_->compareVersions(local, remote), -1);  // local < remote
    EXPECT_EQ(checker_->compareVersions(remote, local), 1);   // remote > local
    EXPECT_EQ(checker_->compareVersions(local, local), 0);    // equal
}

TEST_F(AdapterConsistencyCheckerTest, CompareVersionsSameVersionDifferentTimestamp) {
    ConsistencyCheckResult local;
    local.version = "1";
    local.timestamp = 1000;
    
    ConsistencyCheckResult remote;
    remote.version = "1";
    remote.timestamp = 2000;
    
    EXPECT_EQ(checker_->compareVersions(local, remote), -1);  // local < remote (by timestamp)
}

TEST_F(AdapterConsistencyCheckerTest, ResolveConflictNewerWins) {
    ConsistencyCheckResult local;
    local.version = "1";
    local.timestamp = 1000;
    
    ConsistencyCheckResult remote;
    remote.version = "2";
    remote.timestamp = 2000;
    
    auto winner = checker_->resolveConflict(local, remote);
    
    EXPECT_EQ(winner.version, "2");  // Remote wins (newer)
    EXPECT_EQ(winner.timestamp, 2000);
}

/**
 * Test suite for Adapter Sync Manager
 */
class AdapterSyncManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create storage service
        LoRAStorageService::Config storage_config;
        storage_config.backend = LoRAStorageService::Backend::FileSystem;
        storage_config.filesystem_path = "/tmp/test_lora_sync";
        storage_service_ = std::make_shared<LoRAStorageService>(storage_config);
        
        // Create topology
        ShardTopology::Config topo_config;
        topo_config.cluster_name = "test_cluster";
        topology_ = std::make_shared<ShardTopology>(topo_config);
        
        // Add test shards
        ShardInfo shard1;
        shard1.shard_id = "shard_001";
        shard1.primary_endpoint = "localhost:8081";
        shard1.is_healthy = true;
        topology_->addShard(shard1);
        
        ShardInfo shard2;
        shard2.shard_id = "shard_002";
        shard2.primary_endpoint = "localhost:8082";
        shard2.is_healthy = true;
        topology_->addShard(shard2);
        
        ShardInfo shard3;
        shard3.shard_id = "shard_003";
        shard3.primary_endpoint = "localhost:8083";
        shard3.is_healthy = true;
        topology_->addShard(shard3);
        
        // Create consistency checker
        AdapterConsistencyChecker::Config checker_config;
        consistency_checker_ = std::make_shared<AdapterConsistencyChecker>(checker_config);
        
        // Create sync manager
        AdapterSyncManager::Config sync_config;
        sync_config.sync_interval = std::chrono::seconds(1);  // Short interval for testing
        sync_config.enable_auto_sync = false;  // Manual control for tests
        sync_config.replication_factor = 2;
        sync_manager_ = std::make_unique<AdapterSyncManager>(
            sync_config, storage_service_, topology_, consistency_checker_
        );
    }
    
    void TearDown() override {
        sync_manager_->stop();
    }
    
    std::shared_ptr<LoRAStorageService> storage_service_;
    std::shared_ptr<ShardTopology> topology_;
    std::shared_ptr<AdapterConsistencyChecker> consistency_checker_;
    std::unique_ptr<AdapterSyncManager> sync_manager_;
};

TEST_F(AdapterSyncManagerTest, DiscoverPeers) {
    auto peers = sync_manager_->discoverPeers();
    
    EXPECT_EQ(peers.size(), 3);
    EXPECT_TRUE(std::find(peers.begin(), peers.end(), "shard_001") != peers.end());
    EXPECT_TRUE(std::find(peers.begin(), peers.end(), "shard_002") != peers.end());
    EXPECT_TRUE(std::find(peers.begin(), peers.end(), "shard_003") != peers.end());
}

TEST_F(AdapterSyncManagerTest, StartStop) {
    EXPECT_FALSE(sync_manager_->isRunning());
    
    sync_manager_->start();
    EXPECT_FALSE(sync_manager_->isRunning());  // Auto-sync disabled
    
    sync_manager_->stop();
    EXPECT_FALSE(sync_manager_->isRunning());
}

TEST_F(AdapterSyncManagerTest, GetStats) {
    auto stats = sync_manager_->getStats();
    
    EXPECT_TRUE(stats.contains("running"));
    EXPECT_TRUE(stats.contains("total_syncs"));
    EXPECT_TRUE(stats.contains("successful_syncs"));
    EXPECT_TRUE(stats.contains("sync_failures"));
    EXPECT_TRUE(stats.contains("config"));
}

TEST_F(AdapterSyncManagerTest, SyncAdapterNotFound) {
    bool result = sync_manager_->syncAdapter("nonexistent_adapter");
    
    EXPECT_FALSE(result);
}

TEST_F(AdapterSyncManagerTest, GetSyncStatusNotFound) {
    auto status = sync_manager_->getSyncStatus("nonexistent_adapter");
    
    EXPECT_EQ(status.adapter_id, "nonexistent_adapter");
    EXPECT_FALSE(status.is_synced);
}

TEST_F(AdapterSyncManagerTest, SyncCallback) {
    bool callback_invoked = false;
    SyncJobResult captured_result;
    
    sync_manager_->onSyncComplete([&](const SyncJobResult& result) {
        callback_invoked = true;
        captured_result = result;
    });
    
    auto result = sync_manager_->syncAllAdapters();
    
    EXPECT_TRUE(callback_invoked);
    EXPECT_EQ(captured_result.adapters_checked, result.adapters_checked);
}

/**
 * Integration test for full sync workflow
 */
TEST(AdapterSyncIntegrationTest, FullSyncWorkflow) {
    // Create components
    LoRAStorageService::Config storage_config;
    storage_config.backend = LoRAStorageService::Backend::FileSystem;
    storage_config.filesystem_path = "/tmp/test_lora_sync_integration";
    auto storage_service = std::make_shared<LoRAStorageService>(storage_config);
    
    ShardTopology::Config topo_config;
    auto topology = std::make_shared<ShardTopology>(topo_config);
    
    // Add shards
    for (int i = 1; i <= 5; i++) {
        ShardInfo shard;
        shard.shard_id = "shard_" + std::to_string(i);
        shard.primary_endpoint = "localhost:808" + std::to_string(i);
        shard.is_healthy = true;
        topology->addShard(shard);
    }
    
    auto consistency_checker = std::make_shared<AdapterConsistencyChecker>();
    
    AdapterSyncManager::Config sync_config;
    sync_config.replication_factor = 3;
    sync_config.enable_auto_sync = false;
    auto sync_manager = std::make_unique<AdapterSyncManager>(
        sync_config, storage_service, topology, consistency_checker
    );
    
    // Create test adapter
    AdapterWeights weights;
    weights.data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    weights.size_bytes = weights.data.size();
    
    AdapterMetadata metadata;
    metadata.adapter_id = "test_adapter";
    metadata.base_model = "llama-2-7b";
    metadata.version = "1";
    metadata.updated_at = std::chrono::system_clock::now();
    metadata.checksum = consistency_checker->calculateChecksum(weights.data);
    
    // Save adapter
    EXPECT_TRUE(storage_service->saveAdapter("test_adapter", weights, metadata));
    
    // Sync adapter
    EXPECT_TRUE(sync_manager->syncAdapter("test_adapter"));
    
    // Check sync status
    auto status = sync_manager->getSyncStatus("test_adapter");
    EXPECT_TRUE(status.is_synced);
    EXPECT_EQ(status.local_version, "1");
    EXPECT_EQ(status.sync_failure_count, 0);
    
    // Get stats
    auto stats = sync_manager->getStats();
    EXPECT_GT(stats["successful_syncs"].get<uint64_t>(), 0);
}

/**
 * Test RAID integration with adapter sync
 */
TEST(AdapterSyncRAIDTest, RAIDReplication) {
    // Create components
    LoRAStorageService::Config storage_config;
    storage_config.backend = LoRAStorageService::Backend::FileSystem;
    storage_config.filesystem_path = "/tmp/test_lora_sync_raid";
    auto storage_service = std::make_shared<LoRAStorageService>(storage_config);
    
    ShardTopology::Config topo_config;
    auto topology = std::make_shared<ShardTopology>(topo_config);
    
    // Add 8 shards for RAID testing
    for (int i = 1; i <= 8; i++) {
        ShardInfo shard;
        shard.shard_id = "shard_" + std::to_string(i);
        shard.datacenter = (i <= 4) ? "dc1" : "dc2";
        shard.is_healthy = true;
        topology->addShard(shard);
    }
    
    auto consistency_checker = std::make_shared<AdapterConsistencyChecker>();
    
    // Test RAID 1 (MIRROR) - 3-way replication
    AdapterSyncManager::Config mirror_config;
    mirror_config.replication_factor = 3;
    auto mirror_sync = std::make_unique<AdapterSyncManager>(
        mirror_config, storage_service, topology, consistency_checker
    );
    
    EXPECT_EQ(mirror_sync->discoverPeers().size(), 8);
    
    // Test RAID 5 (PARITY) - Different replication strategy
    // In production, this would use erasure coding
    AdapterSyncManager::Config parity_config;
    parity_config.replication_factor = 6;  // 4 data + 2 parity
    auto parity_sync = std::make_unique<AdapterSyncManager>(
        parity_config, storage_service, topology, consistency_checker
    );
    
    EXPECT_EQ(parity_sync->discoverPeers().size(), 8);
}

/**
 * Test failure scenarios
 */
TEST(AdapterSyncFailureTest, ShardFailure) {
    // Create components
    LoRAStorageService::Config storage_config;
    storage_config.backend = LoRAStorageService::Backend::FileSystem;
    storage_config.filesystem_path = "/tmp/test_lora_sync_failure";
    auto storage_service = std::make_shared<LoRAStorageService>(storage_config);
    
    ShardTopology::Config topo_config;
    auto topology = std::make_shared<ShardTopology>(topo_config);
    
    // Add shards
    ShardInfo shard1;
    shard1.shard_id = "shard_001";
    shard1.is_healthy = true;
    topology->addShard(shard1);
    
    ShardInfo shard2;
    shard2.shard_id = "shard_002";
    shard2.is_healthy = false;  // Failed shard
    topology->addShard(shard2);
    
    ShardInfo shard3;
    shard3.shard_id = "shard_003";
    shard3.is_healthy = true;
    topology->addShard(shard3);
    
    auto consistency_checker = std::make_shared<AdapterConsistencyChecker>();
    
    AdapterSyncManager::Config sync_config;
    sync_config.replication_factor = 2;
    auto sync_manager = std::make_unique<AdapterSyncManager>(
        sync_config, storage_service, topology, consistency_checker
    );
    
    // Only healthy shards should be discovered
    auto peers = sync_manager->discoverPeers();
    EXPECT_EQ(peers.size(), 2);
    EXPECT_TRUE(std::find(peers.begin(), peers.end(), "shard_001") != peers.end());
    EXPECT_TRUE(std::find(peers.begin(), peers.end(), "shard_003") != peers.end());
    EXPECT_TRUE(std::find(peers.begin(), peers.end(), "shard_002") == peers.end());
}


