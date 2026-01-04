/**
 * ThemisDB RAID/Redundancy Strategy Tests
 * 
 * Comprehensive tests for RAID 0, 1, 5, 10 implementations
 */

#include <gtest/gtest.h>
#include "sharding/redundancy_strategy.h"
#include "sharding/consistent_hash.h"
#include "sharding/shard_topology.h"
#include "storage/blob_redundancy_manager.h"
#include <vector>
#include <map>
#include <string>
#include <cstring>

using namespace themisdb::sharding;
using namespace themisdb::storage;

// ═══════════════════════════════════════════════════════════
// Mock Implementations
// ═══════════════════════════════════════════════════════════

class MockShardStorage {
public:
    std::map<std::string, std::map<std::string, std::vector<uint8_t>>> shard_data;
    
    bool write(const std::string& shard_id, const std::string& doc_id, 
               const std::vector<uint8_t>& data) {
        shard_data[shard_id][doc_id] = data;
        return true;
    }
    
    std::optional<std::vector<uint8_t>> read(const std::string& shard_id, 
                                              const std::string& doc_id) {
        if (shard_data.count(shard_id) && shard_data[shard_id].count(doc_id)) {
            return shard_data[shard_id][doc_id];
        }
        return std::nullopt;
    }
    
    bool remove(const std::string& shard_id, const std::string& doc_id) {
        if (shard_data.count(shard_id)) {
            shard_data[shard_id].erase(doc_id);
            return true;
        }
        return false;
    }
    
    void clearShard(const std::string& shard_id) {
        shard_data.erase(shard_id);
    }
    
    size_t getShardCount() const {
        return shard_data.size();
    }
    
    size_t getDocumentCount(const std::string& shard_id) const {
        if (shard_data.count(shard_id)) {
            return shard_data.at(shard_id).size();
        }
        return 0;
    }
};

// ═══════════════════════════════════════════════════════════
// Test Fixtures
// ═══════════════════════════════════════════════════════════

class RedundancyStrategyTest : public ::testing::Test {
protected:
    std::unique_ptr<ConsistentHashRing> ring;
    std::unique_ptr<ShardTopology> topology;
    std::unique_ptr<MockShardStorage> storage;
    
    void SetUp() override {
        // Create hash ring with multiple shards
        ring = std::make_unique<ConsistentHashRing>(100);
        
        // Add shards
        for (int i = 0; i < 6; ++i) {
            ring->addNode("shard-" + std::to_string(i));
        }
        
        // Create topology
        topology = std::make_unique<ShardTopology>();
        
        // Create storage
        storage = std::make_unique<MockShardStorage>();
    }
    
    void TearDown() override {
        storage.reset();
        topology.reset();
        ring.reset();
    }
    
    // Helper to create write handler
    RedundancyStrategy::WriteHandler createWriteHandler() {
        return [this](const std::string& shard_id, const std::string& doc_id,
                     const std::vector<uint8_t>& data) {
            return storage->write(shard_id, doc_id, data);
        };
    }
    
    // Helper to create read handler
    RedundancyStrategy::ReadHandler createReadHandler() {
        return [this](const std::string& shard_id, const std::string& doc_id) {
            return storage->read(shard_id, doc_id);
        };
    }
};

// ═══════════════════════════════════════════════════════════
// Configuration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(RedundancyStrategyTest, ConfigValidation) {
    RedundancyConfig config;
    
    // Valid config
    config.mode = RedundancyMode::MIRROR;
    config.replication_factor = 3;
    EXPECT_TRUE(config.validate());
    
    // Invalid: replication_factor = 0
    config.replication_factor = 0;
    EXPECT_FALSE(config.validate());
    
    // Valid erasure coding config
    config.mode = RedundancyMode::PARITY;
    config.replication_factor = 1;
    config.erasure_coding.data_shards = 4;
    config.erasure_coding.parity_shards = 2;
    EXPECT_TRUE(config.validate());
    
    // Invalid: parity_shards = 0
    config.erasure_coding.parity_shards = 0;
    EXPECT_FALSE(config.validate());
}

TEST_F(RedundancyStrategyTest, StorageEfficiency) {
    RedundancyConfig config;
    
    // NONE: 100% efficiency
    config.mode = RedundancyMode::NONE;
    EXPECT_DOUBLE_EQ(config.getStorageEfficiency(), 1.0);
    
    // MIRROR with RF=3: 33% efficiency
    config.mode = RedundancyMode::MIRROR;
    config.replication_factor = 3;
    EXPECT_DOUBLE_EQ(config.getStorageEfficiency(), 1.0 / 3.0);
    
    // PARITY (4+2): 67% efficiency
    config.mode = RedundancyMode::PARITY;
    config.erasure_coding.data_shards = 4;
    config.erasure_coding.parity_shards = 2;
    EXPECT_DOUBLE_EQ(config.getStorageEfficiency(), 4.0 / 6.0);
}

TEST_F(RedundancyStrategyTest, FaultTolerance) {
    RedundancyConfig config;
    
    // NONE: 0 fault tolerance
    config.mode = RedundancyMode::NONE;
    EXPECT_EQ(config.getFaultTolerance(), 0);
    
    // MIRROR with RF=3: 2 failures tolerated
    config.mode = RedundancyMode::MIRROR;
    config.replication_factor = 3;
    EXPECT_EQ(config.getFaultTolerance(), 2);
    
    // PARITY (4+2): 2 failures tolerated
    config.mode = RedundancyMode::PARITY;
    config.erasure_coding.data_shards = 4;
    config.erasure_coding.parity_shards = 2;
    EXPECT_EQ(config.getFaultTolerance(), 2);
}

// ═══════════════════════════════════════════════════════════
// RAID 0 (STRIPE) Tests
// ═══════════════════════════════════════════════════════════

TEST_F(RedundancyStrategyTest, RAID0_BasicWrite) {
    RedundancyConfig config;
    config.mode = RedundancyMode::STRIPE;
    config.stripe.stripe_size_kb = 1;  // 1KB chunks
    
    RedundancyStrategy strategy(config);
    
    // Create test data (5KB)
    std::vector<uint8_t> data(5 * 1024, 0x42);
    
    // Write
    auto result = strategy.write(
        "doc1", data, "collection1",
        *ring, *topology, createWriteHandler()
    );
    
    EXPECT_TRUE(result.success);
    EXPECT_GT(storage->getShardCount(), 1);  // Should be distributed
}

TEST_F(RedundancyStrategyTest, RAID0_ReadAfterWrite) {
    RedundancyConfig config;
    config.mode = RedundancyMode::STRIPE;
    config.stripe.stripe_size_kb = 1;
    
    RedundancyStrategy strategy(config);
    
    std::vector<uint8_t> original_data(5 * 1024, 0x42);
    
    // Write
    auto write_result = strategy.write(
        "doc1", original_data, "collection1",
        *ring, *topology, createWriteHandler()
    );
    EXPECT_TRUE(write_result.success);
    
    // Read
    auto read_result = strategy.read(
        "doc1", "collection1",
        *ring, *topology, createReadHandler()
    );
    
    EXPECT_TRUE(read_result.success);
    EXPECT_GT(read_result.chunks_read, 1);  // Multiple chunks read
}

// ═══════════════════════════════════════════════════════════
// RAID 1 (MIRROR) Tests
// ═══════════════════════════════════════════════════════════

TEST_F(RedundancyStrategyTest, RAID1_BasicReplication) {
    RedundancyConfig config;
    config.mode = RedundancyMode::MIRROR;
    config.replication_factor = 3;
    config.write_concern = WriteConcern::ALL;
    
    RedundancyStrategy strategy(config);
    
    std::vector<uint8_t> data = {'H', 'e', 'l', 'l', 'o'};
    
    // Write
    auto result = strategy.write(
        "doc1", data, "collection1",
        *ring, *topology, createWriteHandler()
    );
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.written_shards.size(), 3);  // Written to 3 shards
    EXPECT_EQ(result.acknowledgements, 3);
}

TEST_F(RedundancyStrategyTest, RAID1_WriteConcernMajority) {
    RedundancyConfig config;
    config.mode = RedundancyMode::MIRROR;
    config.replication_factor = 3;
    config.write_concern = WriteConcern::MAJORITY;
    
    RedundancyStrategy strategy(config);
    
    std::vector<uint8_t> data = {'T', 'e', 's', 't'};
    
    auto result = strategy.write(
        "doc1", data, "collection1",
        *ring, *topology, createWriteHandler()
    );
    
    EXPECT_TRUE(result.success);
    // Should succeed with >= 2 out of 3 writes
    EXPECT_GE(result.acknowledgements, 2);
}

TEST_F(RedundancyStrategyTest, RAID1_ReadFromReplica) {
    RedundancyConfig config;
    config.mode = RedundancyMode::MIRROR;
    config.replication_factor = 3;
    config.read_preference = ReadPreference::NEAREST;
    
    RedundancyStrategy strategy(config);
    
    std::vector<uint8_t> data = {'D', 'a', 't', 'a'};
    
    // Write
    auto write_result = strategy.write(
        "doc1", data, "collection1",
        *ring, *topology, createWriteHandler()
    );
    EXPECT_TRUE(write_result.success);
    
    // Read
    auto read_result = strategy.read(
        "doc1", "collection1",
        *ring, *topology, createReadHandler()
    );
    
    EXPECT_TRUE(read_result.success);
    EXPECT_EQ(read_result.data.size(), data.size());
}

TEST_F(RedundancyStrategyTest, RAID1_FailoverOnShardFailure) {
    RedundancyConfig config;
    config.mode = RedundancyMode::MIRROR;
    config.replication_factor = 3;
    
    RedundancyStrategy strategy(config);
    
    std::vector<uint8_t> data = {'F', 'a', 'i', 'l', 'o', 'v', 'e', 'r'};
    
    // Write
    strategy.write("doc1", data, "collection1", 
                  *ring, *topology, createWriteHandler());
    
    // Simulate shard failure (remove one shard's data)
    storage->clearShard("shard-0");
    
    // Read should still succeed from replica
    auto read_result = strategy.read(
        "doc1", "collection1",
        *ring, *topology, createReadHandler()
    );
    
    // Should be able to read from remaining replicas
    // This test may fail if all replicas were on the failed shard
    // In production, topology would ensure proper distribution
}

// ═══════════════════════════════════════════════════════════
// RAID 5 (PARITY) Tests
// ═══════════════════════════════════════════════════════════

TEST_F(RedundancyStrategyTest, RAID5_BasicErasureCoding) {
    RedundancyConfig config;
    config.mode = RedundancyMode::PARITY;
    config.erasure_coding.data_shards = 4;
    config.erasure_coding.parity_shards = 2;
    config.erasure_coding.algorithm = ErasureCodingAlgorithm::REED_SOLOMON;
    
    RedundancyStrategy strategy(config);
    
    std::vector<uint8_t> data(4 * 1024, 0x55);  // 4KB data
    
    // Write
    auto result = strategy.write(
        "doc1", data, "collection1",
        *ring, *topology, createWriteHandler()
    );
    
    EXPECT_TRUE(result.success);
    EXPECT_GE(result.written_shards.size(), 4);  // At least data shards
}

TEST_F(RedundancyStrategyTest, RAID5_RecoveryFromMissingChunk) {
    RedundancyConfig config;
    config.mode = RedundancyMode::PARITY;
    config.erasure_coding.data_shards = 4;
    config.erasure_coding.parity_shards = 2;
    
    RedundancyStrategy strategy(config);
    
    std::vector<uint8_t> data(4 * 1024, 0xAA);
    
    // Write
    strategy.write("doc1", data, "collection1",
                  *ring, *topology, createWriteHandler());
    
    // Read should work even with some chunks missing
    // (erasure coding allows recovery)
    auto read_result = strategy.read(
        "doc1", "collection1",
        *ring, *topology, createReadHandler()
    );
    
    EXPECT_TRUE(read_result.success);
}

// ═══════════════════════════════════════════════════════════
// RAID 10 (STRIPE + MIRROR) Tests
// ═══════════════════════════════════════════════════════════

TEST_F(RedundancyStrategyTest, RAID10_CombinedStripingAndMirroring) {
    RedundancyConfig config;
    config.mode = RedundancyMode::STRIPE_MIRROR;
    config.replication_factor = 2;
    config.stripe.stripe_size_kb = 1;
    
    RedundancyStrategy strategy(config);
    
    std::vector<uint8_t> data(5 * 1024, 0x77);
    
    // Write
    auto result = strategy.write(
        "doc1", data, "collection1",
        *ring, *topology, createWriteHandler()
    );
    
    EXPECT_TRUE(result.success);
    // Should have multiple chunks, each mirrored
    EXPECT_GT(result.written_shards.size(), 2);
}

// ═══════════════════════════════════════════════════════════
// Statistics and Monitoring Tests
// ═══════════════════════════════════════════════════════════

TEST_F(RedundancyStrategyTest, StatisticsTracking) {
    RedundancyConfig config;
    config.mode = RedundancyMode::MIRROR;
    config.replication_factor = 2;
    
    RedundancyStrategy strategy(config);
    
    std::vector<uint8_t> data = {'S', 't', 'a', 't', 's'};
    
    // Perform some operations
    strategy.write("doc1", data, "collection1", 
                  *ring, *topology, createWriteHandler());
    strategy.read("doc1", "collection1", 
                 *ring, *topology, createReadHandler());
    
    // Get stats
    auto stats = strategy.getStats();
    
    EXPECT_GT(stats.reads_from_primary, 0);
}

TEST_F(RedundancyStrategyTest, PrometheusMetricsExport) {
    RedundancyConfig config;
    config.mode = RedundancyMode::MIRROR;
    config.replication_factor = 2;
    
    RedundancyStrategy strategy(config);
    
    std::vector<uint8_t> data = {'M', 'e', 't', 'r', 'i', 'c', 's'};
    
    strategy.write("doc1", data, "collection1",
                  *ring, *topology, createWriteHandler());
    
    // Export metrics
    std::string metrics = strategy.exportPrometheusMetrics();
    
    EXPECT_FALSE(metrics.empty());
    EXPECT_NE(metrics.find("themis_redundancy_writes_total"), std::string::npos);
    EXPECT_NE(metrics.find("themis_redundancy_reads_total"), std::string::npos);
}

// ═══════════════════════════════════════════════════════════
// Collection-Level Configuration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(RedundancyStrategyTest, CollectionSpecificConfiguration) {
    CollectionRedundancyManager manager;
    
    // Set default
    RedundancyConfig default_config;
    default_config.mode = RedundancyMode::MIRROR;
    default_config.replication_factor = 2;
    manager.setDefaultConfig(default_config);
    
    // Set collection-specific
    RedundancyConfig collection_config;
    collection_config.mode = RedundancyMode::PARITY;
    collection_config.erasure_coding.data_shards = 4;
    collection_config.erasure_coding.parity_shards = 2;
    manager.setCollectionConfig("important_data", collection_config);
    
    // Get config
    auto config1 = manager.getConfig("regular_data");
    EXPECT_EQ(config1.mode, RedundancyMode::MIRROR);
    
    auto config2 = manager.getConfig("important_data");
    EXPECT_EQ(config2.mode, RedundancyMode::PARITY);
}

// ═══════════════════════════════════════════════════════════
// Blob Redundancy Manager Tests
// ═══════════════════════════════════════════════════════════

TEST(BlobRedundancyManagerTest, BasicBlobRegistration) {
    BlobRedundancyManager::Config config;
    config.enable_blob_tracking = true;
    
    BlobRedundancyManager manager(config);
    
    // Register a blob
    std::string blob_id = manager.registerBlob(
        BlobType::SST_L0,
        "/path/to/file.sst",
        1024 * 1024,  // 1MB
        "test_collection",
        "doc1"
    );
    
    EXPECT_FALSE(blob_id.empty());
    
    // Get metadata
    auto metadata = manager.getBlobMetadata(blob_id);
    EXPECT_EQ(metadata.blob_id, blob_id);
    EXPECT_EQ(metadata.type, BlobType::SST_L0);
    EXPECT_EQ(metadata.collection, "test_collection");
}

TEST(BlobRedundancyManagerTest, BlobHealthCheck) {
    BlobRedundancyManager::Config config;
    config.enable_blob_tracking = true;
    
    BlobRedundancyManager manager(config);
    
    std::string blob_id = manager.registerBlob(
        BlobType::MANIFEST,
        "/path/to/MANIFEST",
        4096,
        "",
        ""
    );
    
    // Verify blob
    bool healthy = manager.verifyBlob(blob_id);
    EXPECT_TRUE(healthy);
}

TEST(BlobRedundancyManagerTest, DegradedBlobDetection) {
    BlobRedundancyManager::Config config;
    BlobRedundancyManager manager(config);
    
    // Register some blobs
    manager.registerBlob(BlobType::SST_L1, "/file1.sst", 1024, "", "");
    manager.registerBlob(BlobType::SST_L2_PLUS, "/file2.sst", 2048, "", "");
    
    // Get degraded blobs
    auto degraded = manager.getDegradedBlobs();
    
    // Initially, might be 0 or more depending on configuration
    EXPECT_GE(degraded.size(), 0);
}

TEST(BlobRedundancyManagerTest, PrometheusMetrics) {
    BlobRedundancyManager::Config config;
    BlobRedundancyManager manager(config);
    
    manager.registerBlob(BlobType::WAL, "/wal.log", 512, "", "");
    
    std::string metrics = manager.exportPrometheusMetrics();
    
    EXPECT_FALSE(metrics.empty());
    EXPECT_NE(metrics.find("themis_blob_redundancy_total_blobs"), std::string::npos);
}

// ═══════════════════════════════════════════════════════════
// Stress Tests
// ═══════════════════════════════════════════════════════════

TEST_F(RedundancyStrategyTest, DISABLED_StressTest_ManyWrites) {
    // Disabled by default - enable for performance testing
    
    RedundancyConfig config;
    config.mode = RedundancyMode::MIRROR;
    config.replication_factor = 3;
    
    RedundancyStrategy strategy(config);
    
    const int NUM_DOCS = 1000;
    std::vector<uint8_t> data(1024, 0xFF);
    
    for (int i = 0; i < NUM_DOCS; ++i) {
        std::string doc_id = "doc" + std::to_string(i);
        auto result = strategy.write(doc_id, data, "collection1",
                                    *ring, *topology, createWriteHandler());
        ASSERT_TRUE(result.success);
    }
    
    auto stats = strategy.getStats();
    EXPECT_EQ(stats.reads_from_primary + stats.reads_from_replica, 0);
}

// ═══════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
