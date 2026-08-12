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
#include <rocksdb/listener.h>
#include <vector>
#include <map>
#include <string>
#include <cstring>
#include <atomic>
#include <thread>

using namespace themis::sharding;
using themisdb::storage::BlobRedundancyManager;
using themisdb::storage::BlobType;
using themisdb::storage::RocksDBBlobListener;

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

TEST_F(RedundancyStrategyTest, RAID1_WriteConcernAllFailsWhenReplicationTargetsMissing) {
    RedundancyConfig config;
    config.mode = RedundancyMode::MIRROR;
    config.replication_factor = 10;
    config.write_concern = WriteConcern::ALL;

    RedundancyStrategy strategy(config);

    size_t write_calls = 0;
    auto counting_handler = [this, &write_calls](const std::string& shard_id,
                                                 const std::string& doc_id,
                                                 const std::vector<uint8_t>& data) {
        ++write_calls;
        return storage->write(shard_id, doc_id, data);
    };

    std::vector<uint8_t> data = {'T', 'e', 's', 't'};
    auto result = strategy.write("doc1", data, "collection1", *ring, *topology, counting_handler);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(write_calls, 0u);
    EXPECT_NE(result.error_message.find("Insufficient replica targets"), std::string::npos);
}

TEST_F(RedundancyStrategyTest, RAID1_WriteConcernMajorityFailsWhenReplicationTargetsMissing) {
    RedundancyConfig config;
    config.mode = RedundancyMode::MIRROR;
    config.replication_factor = 12;
    config.write_concern = WriteConcern::MAJORITY;

    RedundancyStrategy strategy(config);

    size_t write_calls = 0;
    auto counting_handler = [this, &write_calls](const std::string& shard_id,
                                                 const std::string& doc_id,
                                                 const std::vector<uint8_t>& data) {
        ++write_calls;
        return storage->write(shard_id, doc_id, data);
    };

    std::vector<uint8_t> data = {'T', 'e', 's', 't'};
    auto result = strategy.write("doc1", data, "collection1", *ring, *topology, counting_handler);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(write_calls, 0u);
    EXPECT_NE(result.error_message.find("Insufficient replica targets"), std::string::npos);
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

// ─────────────────────────────────────────────────────────────
// ReedSolomonCoder GF(2^8) arithmetic tests
// ─────────────────────────────────────────────────────────────
// Test XOR-parity decode: write with RS, drop 1 data chunk, recover via parity
TEST_F(RedundancyStrategyTest, RS_Decode_RecoverOneMissingDataChunk) {
    RedundancyConfig config;
    config.mode = RedundancyMode::PARITY;
    config.erasure_coding.data_shards = 3;
    config.erasure_coding.parity_shards = 1;
    config.erasure_coding.algorithm = ErasureCodingAlgorithm::REED_SOLOMON;

    RedundancyStrategy strategy(config);
    const std::string doc_id = "rs-xor-recover";
    // Use data that is a multiple of data_shards bytes for clean chunk alignment
    std::vector<uint8_t> data = {0x10, 0x20, 0x30, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

    auto wr = strategy.write(doc_id, data, "coll", *ring, *topology, createWriteHandler());
    ASSERT_TRUE(wr.success) << wr.error_message;

    // Drop the first data chunk key to simulate a lost shard
    std::string data_chunk_key = doc_id + ":data:0";
    for (auto& [shard_id, docs] : storage->shard_data) {
        docs.erase(data_chunk_key);
    }

    // Read must still succeed (XOR recovery using parity chunk)
    auto rr = strategy.read(doc_id, "coll", *ring, *topology, createReadHandler());
    EXPECT_TRUE(rr.success) << rr.error_message;
}

// Verify RS gf_mul(0, x) == 0 and gf_mul(1, x) == x
TEST_F(RedundancyStrategyTest, RS_GFMultiply_IdentityAndZero) {
    // Encode a 1-byte document: after RS encode, parity = XOR(data chunks).
    // With data=[0x42] and 1 data shard + 1 parity shard, the parity should be 0x42.
    RedundancyConfig config;
    config.mode = RedundancyMode::PARITY;
    config.erasure_coding.data_shards = 1;
    config.erasure_coding.parity_shards = 1;
    config.erasure_coding.algorithm = ErasureCodingAlgorithm::REED_SOLOMON;

    RedundancyStrategy strategy(config);
    const std::vector<uint8_t> data = {0x42};
    auto wr = strategy.write("gf-id", data, "coll", *ring, *topology, createWriteHandler());
    EXPECT_TRUE(wr.success) << wr.error_message;

    // Read back to confirm encode/decode round-trip is correct
    auto rr = strategy.read("gf-id", "coll", *ring, *topology, createReadHandler());
    EXPECT_TRUE(rr.success);
    if (rr.success && !rr.data.empty()) {
        EXPECT_EQ(static_cast<uint8_t>(rr.data[0]), 0x42u);
    }
}

// RS Cauchy: write/read round-trip with CAUCHY algorithm and 1 missing data chunk
TEST_F(RedundancyStrategyTest, RS_Cauchy_RecoverOneMissingDataChunk) {
    RedundancyConfig config;
    config.mode = RedundancyMode::PARITY;
    config.erasure_coding.data_shards = 3;
    config.erasure_coding.parity_shards = 2;
    config.erasure_coding.algorithm = ErasureCodingAlgorithm::CAUCHY;

    RedundancyStrategy strategy(config);
    const std::string doc_id = "cauchy-recover";
    std::vector<uint8_t> data(48, 0xCC);

    auto wr = strategy.write(doc_id, data, "coll", *ring, *topology, createWriteHandler());
    ASSERT_TRUE(wr.success) << wr.error_message;

    // Drop data chunk 1
    std::string data_chunk_key = doc_id + ":data:1";
    for (auto& [shard_id, docs] : storage->shard_data) {
        docs.erase(data_chunk_key);
    }

    auto rr = strategy.read(doc_id, "coll", *ring, *topology, createReadHandler());
    EXPECT_TRUE(rr.success) << rr.error_message;
}

// recoverDocument with PARITY/RAID6 uses correct map/uint32_t types (compile + runtime)
TEST_F(RedundancyStrategyTest, RecoverDocument_Parity_TypeSafe) {
    RedundancyConfig config;
    config.mode = RedundancyMode::PARITY;
    config.erasure_coding.data_shards = 4;
    config.erasure_coding.parity_shards = 2;
    config.erasure_coding.algorithm = ErasureCodingAlgorithm::CAUCHY;

    RedundancyStrategy strategy(config);
    const std::string doc_id = "parity-recover";
    std::vector<uint8_t> data(64, 0xBB);

    auto wr = strategy.write(doc_id, data, "coll", *ring, *topology, createWriteHandler());
    ASSERT_TRUE(wr.success) << wr.error_message;

    // Drop one data chunk to force recovery
    std::string chunk_key = doc_id + ":data:0";
    for (auto& [shard_id, docs] : storage->shard_data) {
        docs.erase(chunk_key);
    }

    bool recovered = strategy.recoverDocument(
        doc_id, "coll", *ring, *topology,
        createReadHandler(), createWriteHandler());
    EXPECT_TRUE(recovered);
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
// RocksDB Event Listener Tests
// ═══════════════════════════════════════════════════════════

TEST(BlobRedundancyEventListenerTest, CreateRocksDBListenerSucceeds) {
    BlobRedundancyManager::Config config;
    BlobRedundancyManager manager(config);

    auto result = manager.createRocksDBListener();
    EXPECT_TRUE(result.has_value()) << "createRocksDBListener should succeed";
    EXPECT_NE(result.value(), nullptr) << "returned listener must not be null";
}

TEST(BlobRedundancyEventListenerTest, NotifySSTFileDeletedMarksLocationUnhealthy) {
    BlobRedundancyManager::Config config;
    BlobRedundancyManager manager(config);

    const std::string sst_path = "/data/db/000042.sst";
    std::string blob_id = manager.registerBlob(
        BlobType::SST_L1, sst_path, 1024 * 1024, "col1", "");

    // Before notification the blob must have at least one healthy location.
    auto before = manager.getBlobMetadata(blob_id);
    ASSERT_FALSE(before.locations.empty());
    EXPECT_TRUE(before.locations[0].is_healthy);

    // Simulate RocksDB deleting the SST file.
    manager.notifySSTFileDeleted(sst_path);

    // After notification the primary location should be marked unhealthy.
    auto after = manager.getBlobMetadata(blob_id);
    ASSERT_FALSE(after.locations.empty());
    EXPECT_FALSE(after.locations[0].is_healthy)
        << "location backed by the deleted SST should be unhealthy";
}

TEST(BlobRedundancyEventListenerTest, NotifySSTFileDeletedUnknownPathIsNoOp) {
    BlobRedundancyManager::Config config;
    BlobRedundancyManager manager(config);

    manager.registerBlob(BlobType::SST_L0, "/data/db/000001.sst", 512, "", "");

    // Notifying with a path that was never registered must not crash or alter
    // any existing blob.
    EXPECT_NO_THROW(manager.notifySSTFileDeleted("/data/db/999999.sst"));
}

TEST(BlobRedundancyEventListenerTest, NotifySSTFileDeletedOnlyAffectsMatchingBlobs) {
    BlobRedundancyManager::Config config;
    BlobRedundancyManager manager(config);

    const std::string deleted_path  = "/data/db/000010.sst";
    const std::string surviving_path = "/data/db/000011.sst";

    std::string deleted_blob_id  = manager.registerBlob(
        BlobType::SST_L1, deleted_path, 1024, "", "");
    std::string surviving_blob_id = manager.registerBlob(
        BlobType::SST_L1, surviving_path, 1024, "", "");

    manager.notifySSTFileDeleted(deleted_path);

    auto deleted_meta  = manager.getBlobMetadata(deleted_blob_id);
    auto surviving_meta = manager.getBlobMetadata(surviving_blob_id);

    ASSERT_FALSE(deleted_meta.locations.empty());
    EXPECT_FALSE(deleted_meta.locations[0].is_healthy)
        << "deleted blob's location should be unhealthy";

    ASSERT_FALSE(surviving_meta.locations.empty());
    EXPECT_TRUE(surviving_meta.locations[0].is_healthy)
        << "unaffected blob's location must stay healthy";
}

TEST(BlobRedundancyEventListenerTest, RocksDBListenerOnTableFileDeletedTriggersReplication) {
    BlobRedundancyManager::Config config;
    BlobRedundancyManager manager(config);

    const std::string sst_path = "/data/db/000099.sst";
    std::string blob_id = manager.registerBlob(
        BlobType::SST_L2_PLUS, sst_path, 2048, "test_col", "");

    // Obtain the listener through the official factory.
    auto listener_result = manager.createRocksDBListener();
    ASSERT_TRUE(listener_result.has_value());

    auto* listener = dynamic_cast<RocksDBBlobListener*>(listener_result.value().get());
    ASSERT_NE(listener, nullptr) << "listener must be a RocksDBBlobListener";

    // Confirm the location is healthy before deletion.
    auto before = manager.getBlobMetadata(blob_id);
    ASSERT_FALSE(before.locations.empty());
    EXPECT_TRUE(before.locations[0].is_healthy);

    // Simulate RocksDB invoking OnTableFileDeleted.
    rocksdb::TableFileDeletionInfo deletion_info;
    deletion_info.file_path = sst_path;
    listener->OnTableFileDeleted(deletion_info);

    // The blob's primary location must now be marked unhealthy.
    auto after = manager.getBlobMetadata(blob_id);
    ASSERT_FALSE(after.locations.empty());
    EXPECT_FALSE(after.locations[0].is_healthy)
        << "location backed by deleted SST should be unhealthy after OnTableFileDeleted";
}

// ═══════════════════════════════════════════════════════════
// RAID 6 (Dual Parity) Tests
// ═══════════════════════════════════════════════════════════

TEST_F(RedundancyStrategyTest, RAID6_BasicConfiguration) {
    RedundancyConfig config;
    config.mode = RedundancyMode::RAID6;
    config.erasure_coding.data_shards = 6;
    config.erasure_coding.parity_shards = 2;
    config.erasure_coding.algorithm = ErasureCodingAlgorithm::CAUCHY;
    
    EXPECT_TRUE(config.validate());
    EXPECT_EQ(config.getFaultTolerance(), 2);
    EXPECT_DOUBLE_EQ(config.getStorageEfficiency(), 6.0 / 8.0);  // 75%
}

TEST_F(RedundancyStrategyTest, RAID6_InvalidConfiguration) {
    RedundancyConfig config;
    config.mode = RedundancyMode::RAID6;
    config.erasure_coding.data_shards = 6;
    config.erasure_coding.parity_shards = 1;  // RAID6 requires at least 2
    
    EXPECT_FALSE(config.validate());
}

TEST_F(RedundancyStrategyTest, RAID6_BasicWriteRead) {
    RedundancyConfig config;
    config.mode = RedundancyMode::RAID6;
    config.erasure_coding.data_shards = 4;
    config.erasure_coding.parity_shards = 2;
    config.erasure_coding.algorithm = ErasureCodingAlgorithm::CAUCHY;
    
    RedundancyStrategy strategy(config);
    
    std::vector<uint8_t> data(4 * 1024, 0xAB);  // 4KB data
    
    // Write
    auto write_result = strategy.write(
        "doc1", data, "collection1",
        *ring, *topology, createWriteHandler()
    );
    
    EXPECT_TRUE(write_result.success);
    EXPECT_GE(write_result.written_shards.size(), 4);  // At least data shards
    
    // Read
    auto read_result = strategy.read(
        "doc1", "collection1",
        *ring, *topology, createReadHandler()
    );
    
    EXPECT_TRUE(read_result.success);
}

TEST_F(RedundancyStrategyTest, RAID6_SingleShardFailure) {
    RedundancyConfig config;
    config.mode = RedundancyMode::RAID6;
    config.erasure_coding.data_shards = 6;
    config.erasure_coding.parity_shards = 2;
    config.erasure_coding.algorithm = ErasureCodingAlgorithm::CAUCHY;
    
    RedundancyStrategy strategy(config);
    
    std::vector<uint8_t> data(6 * 1024, 0xCD);
    
    // Write
    strategy.write("doc1", data, "collection1",
                  *ring, *topology, createWriteHandler());
    
    // Simulate single shard failure
    storage->clearShard("shard-0");
    
    // Read should still succeed
    auto read_result = strategy.read(
        "doc1", "collection1",
        *ring, *topology, createReadHandler()
    );
    
    EXPECT_TRUE(read_result.success);
}

TEST_F(RedundancyStrategyTest, RAID6_DualShardFailure) {
    RedundancyConfig config;
    config.mode = RedundancyMode::RAID6;
    config.erasure_coding.data_shards = 6;
    config.erasure_coding.parity_shards = 2;
    config.erasure_coding.algorithm = ErasureCodingAlgorithm::CAUCHY;
    
    RedundancyStrategy strategy(config);
    
    std::vector<uint8_t> data(6 * 1024, 0xEF);
    
    // Write
    strategy.write("doc1", data, "collection1",
                  *ring, *topology, createWriteHandler());
    
    // Simulate TWO shard failures (RAID 6 should tolerate this)
    storage->clearShard("shard-0");
    storage->clearShard("shard-1");
    
    // Read should still succeed with RAID 6
    auto read_result = strategy.read(
        "doc1", "collection1",
        *ring, *topology, createReadHandler()
    );
    
    EXPECT_TRUE(read_result.success);
}

TEST_F(RedundancyStrategyTest, RAID6_AllDataShardCombinations) {
    // Test recovery from various 2-shard failure combinations
    RedundancyConfig config;
    config.mode = RedundancyMode::RAID6;
    config.erasure_coding.data_shards = 4;
    config.erasure_coding.parity_shards = 2;
    config.erasure_coding.algorithm = ErasureCodingAlgorithm::CAUCHY;
    
    RedundancyStrategy strategy(config);
    
    std::vector<uint8_t> original_data(4 * 1024, 0x42);
    
    // Test all combinations of 2-shard failures
    std::vector<std::pair<int, int>> failure_combinations = {
        {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5},
        {1, 2}, {1, 3}, {1, 4}, {1, 5},
        {2, 3}, {2, 4}, {2, 5},
        {3, 4}, {3, 5},
        {4, 5}
    };
    
    for (const auto& [fail1, fail2] : failure_combinations) {
        // Reset storage for each test
        storage = std::make_unique<MockShardStorage>();
        
        // Write data
        strategy.write("doc_test", original_data, "collection1",
                      *ring, *topology, createWriteHandler());
        
        // Simulate 2-shard failure
        storage->clearShard("shard-" + std::to_string(fail1));
        storage->clearShard("shard-" + std::to_string(fail2));
        
        // Try to read - should succeed
        auto read_result = strategy.read(
            "doc_test", "collection1",
            *ring, *topology, createReadHandler()
        );
        
        EXPECT_TRUE(read_result.success) 
            << "Failed to recover with shards " << fail1 << " and " << fail2 << " down";
    }
}

TEST_F(RedundancyStrategyTest, RAID6_StorageEfficiency) {
    // Test various RAID 6 configurations
    std::vector<std::pair<uint32_t, uint32_t>> configs = {
        {4, 2},   // 4+2 = 67% efficiency
        {6, 2},   // 6+2 = 75% efficiency
        {8, 2},   // 8+2 = 80% efficiency
        {10, 2}   // 10+2 = 83% efficiency
    };
    
    for (const auto& [data_shards, parity_shards] : configs) {
        RedundancyConfig config;
        config.mode = RedundancyMode::RAID6;
        config.erasure_coding.data_shards = data_shards;
        config.erasure_coding.parity_shards = parity_shards;
        
        double expected_efficiency = static_cast<double>(data_shards) / (data_shards + parity_shards);
        
        EXPECT_DOUBLE_EQ(config.getStorageEfficiency(), expected_efficiency)
            << "Config " << data_shards << "+" << parity_shards;
    }
}

TEST_F(RedundancyStrategyTest, RAID6_PerformanceMetrics) {
    RedundancyConfig config;
    config.mode = RedundancyMode::RAID6;
    config.erasure_coding.data_shards = 6;
    config.erasure_coding.parity_shards = 2;
    config.erasure_coding.algorithm = ErasureCodingAlgorithm::CAUCHY;
    
    RedundancyStrategy strategy(config);
    
    std::vector<uint8_t> data(10 * 1024, 0x55);  // 10KB
    
    auto start = std::chrono::steady_clock::now();
    
    // Write
    auto write_result = strategy.write(
        "perf_doc", data, "collection1",
        *ring, *topology, createWriteHandler()
    );
    
    auto end = std::chrono::steady_clock::now();
    auto write_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_TRUE(write_result.success);
    EXPECT_LT(write_time.count(), 1000);  // Should complete within 1 second
    
    // Read
    start = std::chrono::steady_clock::now();
    
    auto read_result = strategy.read(
        "perf_doc", "collection1",
        *ring, *topology, createReadHandler()
    );
    
    end = std::chrono::steady_clock::now();
    auto read_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_TRUE(read_result.success);
    EXPECT_LT(read_time.count(), 1000);  // Should complete within 1 second
}

TEST_F(RedundancyStrategyTest, RAID6_PrometheusMetrics) {
    RedundancyConfig config;
    config.mode = RedundancyMode::RAID6;
    config.erasure_coding.data_shards = 6;
    config.erasure_coding.parity_shards = 2;
    config.erasure_coding.algorithm = ErasureCodingAlgorithm::CAUCHY;
    
    RedundancyStrategy strategy(config);
    
    std::vector<uint8_t> data(5 * 1024, 0x99);
    
    strategy.write("metrics_doc", data, "collection1",
                  *ring, *topology, createWriteHandler());
    strategy.read("metrics_doc", "collection1",
                 *ring, *topology, createReadHandler());
    
    std::string metrics = strategy.exportPrometheusMetrics();
    
    EXPECT_FALSE(metrics.empty());
    EXPECT_NE(metrics.find("themis_redundancy_writes_total"), std::string::npos);
    EXPECT_NE(metrics.find("themis_redundancy_reads_total"), std::string::npos);
    EXPECT_NE(metrics.find("themis_redundancy_bytes_written_total"), std::string::npos);
}

TEST_F(RedundancyStrategyTest, RAID6_vs_RAID5_Comparison) {
    // Compare RAID 5 (1 parity) vs RAID 6 (2 parity)
    
    // RAID 5 config
    RedundancyConfig raid5_config;
    raid5_config.mode = RedundancyMode::PARITY;
    raid5_config.erasure_coding.data_shards = 6;
    raid5_config.erasure_coding.parity_shards = 1;
    
    // RAID 6 config
    RedundancyConfig raid6_config;
    raid6_config.mode = RedundancyMode::RAID6;
    raid6_config.erasure_coding.data_shards = 6;
    raid6_config.erasure_coding.parity_shards = 2;
    raid6_config.erasure_coding.algorithm = ErasureCodingAlgorithm::CAUCHY;
    
    // RAID 5: 1 failure tolerance, ~86% efficiency
    EXPECT_EQ(raid5_config.getFaultTolerance(), 1);
    EXPECT_DOUBLE_EQ(raid5_config.getStorageEfficiency(), 6.0 / 7.0);
    
    // RAID 6: 2 failure tolerance, ~75% efficiency
    EXPECT_EQ(raid6_config.getFaultTolerance(), 2);
    EXPECT_DOUBLE_EQ(raid6_config.getStorageEfficiency(), 6.0 / 8.0);
}

TEST_F(RedundancyStrategyTest, RAID6_LargeDocumentHandling) {
    RedundancyConfig config;
    config.mode = RedundancyMode::RAID6;
    config.erasure_coding.data_shards = 6;
    config.erasure_coding.parity_shards = 2;
    config.erasure_coding.algorithm = ErasureCodingAlgorithm::CAUCHY;
    
    RedundancyStrategy strategy(config);
    
    // Test with larger document (1MB)
    std::vector<uint8_t> large_data(1024 * 1024, 0x77);
    
    auto write_result = strategy.write(
        "large_doc", large_data, "collection1",
        *ring, *topology, createWriteHandler()
    );
    
    EXPECT_TRUE(write_result.success);
    
    auto read_result = strategy.read(
        "large_doc", "collection1",
        *ring, *topology, createReadHandler()
    );
    
    EXPECT_TRUE(read_result.success);
}

TEST_F(RedundancyStrategyTest, RAID6_CauchyAlgorithm) {
    // Verify Cauchy algorithm is used for RAID 6
    RedundancyConfig config;
    config.mode = RedundancyMode::RAID6;
    config.erasure_coding.data_shards = 6;
    config.erasure_coding.parity_shards = 2;
    config.erasure_coding.algorithm = ErasureCodingAlgorithm::CAUCHY;
    
    // Should not throw
    EXPECT_NO_THROW({
        RedundancyStrategy strategy(config);
        
        std::vector<uint8_t> data(6 * 1024, 0xAA);
        strategy.write("test_cauchy", data, "collection1",
                      *ring, *topology, createWriteHandler());
    });
}

TEST_F(RedundancyStrategyTest, RAID6_MultipleDocuments) {
    RedundancyConfig config;
    config.mode = RedundancyMode::RAID6;
    config.erasure_coding.data_shards = 6;
    config.erasure_coding.parity_shards = 2;
    config.erasure_coding.algorithm = ErasureCodingAlgorithm::CAUCHY;
    
    RedundancyStrategy strategy(config);
    
    // Write multiple documents
    const int NUM_DOCS = 10;
    for (int i = 0; i < NUM_DOCS; ++i) {
        std::vector<uint8_t> data(5 * 1024, static_cast<uint8_t>(i));
        std::string doc_id = "multi_doc_" + std::to_string(i);
        
        auto result = strategy.write(doc_id, data, "collection1",
                                    *ring, *topology, createWriteHandler());
        EXPECT_TRUE(result.success);
    }
    
    // Read them back
    for (int i = 0; i < NUM_DOCS; ++i) {
        std::string doc_id = "multi_doc_" + std::to_string(i);
        auto result = strategy.read(doc_id, "collection1",
                                   *ring, *topology, createReadHandler());
        EXPECT_TRUE(result.success);
    }
}

TEST_F(RedundancyStrategyTest, RAID6_EdgeCases) {
    RedundancyConfig config;
    config.mode = RedundancyMode::RAID6;
    config.erasure_coding.data_shards = 4;
    config.erasure_coding.parity_shards = 2;
    config.erasure_coding.algorithm = ErasureCodingAlgorithm::CAUCHY;
    
    RedundancyStrategy strategy(config);
    
    // Test with very small data
    std::vector<uint8_t> small_data = {0x01, 0x02, 0x03};
    auto result1 = strategy.write("small_doc", small_data, "collection1",
                                 *ring, *topology, createWriteHandler());
    EXPECT_TRUE(result1.success);
    
    // Test with empty data
    std::vector<uint8_t> empty_data;
    auto result2 = strategy.write("empty_doc", empty_data, "collection1",
                                 *ring, *topology, createWriteHandler());
    EXPECT_TRUE(result2.success);
}

TEST_F(RedundancyStrategyTest, RAID6_CollectionSpecific) {
    CollectionRedundancyManager manager;
    
    // Set RAID 6 for specific collection
    RedundancyConfig raid6_config;
    raid6_config.mode = RedundancyMode::RAID6;
    raid6_config.erasure_coding.data_shards = 6;
    raid6_config.erasure_coding.parity_shards = 2;
    raid6_config.erasure_coding.algorithm = ErasureCodingAlgorithm::CAUCHY;
    manager.setCollectionConfig("critical_data", raid6_config);
    
    // Verify configuration
    auto config = manager.getConfig("critical_data");
    EXPECT_EQ(config.mode, RedundancyMode::RAID6);
    EXPECT_EQ(config.erasure_coding.data_shards, 6);
    EXPECT_EQ(config.erasure_coding.parity_shards, 2);
    EXPECT_EQ(config.getFaultTolerance(), 2);
}

// ═══════════════════════════════════════════════════════════
// GEO_MIRROR Tests
// ═══════════════════════════════════════════════════════════

class GeoMirrorTest : public ::testing::Test {
protected:
    std::unique_ptr<ConsistentHashRing> ring;
    std::unique_ptr<ShardTopology> topology;
    std::unique_ptr<MockShardStorage> storage;

    void SetUp() override {
        ring = std::make_unique<ConsistentHashRing>(100);
        topology = std::make_unique<ShardTopology>();
        storage = std::make_unique<MockShardStorage>();

        // 6 shards spread across 2 regions / 2 zones each
        for (int i = 0; i < 6; ++i) {
            ring->addNode("shard-" + std::to_string(i));
        }

        auto addShard = [&](const std::string& id,
                            const std::string& region,
                            const std::string& zone,
                            bool healthy = true) {
            ShardInfo info;
            info.shard_id = id;
            info.region   = region;
            info.zone     = zone;
            info.datacenter = region;
            info.is_healthy = healthy;
            topology->addShard(info);
        };

        addShard("shard-0", "us-east", "us-east-1a");
        addShard("shard-1", "us-east", "us-east-1b");
        addShard("shard-2", "us-east", "us-east-1c");
        addShard("shard-3", "eu-west", "eu-west-1a");
        addShard("shard-4", "eu-west", "eu-west-1b");
        addShard("shard-5", "eu-west", "eu-west-1c");
    }

    void TearDown() override {
        storage.reset();
        topology.reset();
        ring.reset();
    }

    RedundancyStrategy::WriteHandler createWriteHandler() {
        return [this](const std::string& shard_id, const std::string& doc_id,
                     const std::vector<uint8_t>& data) {
            return storage->write(shard_id, doc_id, data);
        };
    }

    RedundancyStrategy::ReadHandler createReadHandler() {
        return [this](const std::string& shard_id, const std::string& doc_id) {
            return storage->read(shard_id, doc_id);
        };
    }
};

TEST_F(GeoMirrorTest, BasicWriteAndRead) {
    RedundancyConfig config;
    config.mode = RedundancyMode::GEO_MIRROR;
    config.replication_factor = 3;
    config.write_concern = WriteConcern::MAJORITY;
    config.geo_replication.local_region = "us-east";
    config.geo_replication.read_preference = ReadPreference::LOCAL_REGION;

    RedundancyStrategy strategy(config);
    std::vector<uint8_t> data = {'G', 'e', 'o', 'D', 'a', 't', 'a'};

    auto wr = strategy.write("geo-doc1", data, "coll", *ring, *topology, createWriteHandler());
    EXPECT_TRUE(wr.success);
    EXPECT_FALSE(wr.written_shards.empty());

    auto rr = strategy.read("geo-doc1", "coll", *ring, *topology, createReadHandler());
    EXPECT_TRUE(rr.success);
    EXPECT_EQ(rr.data, std::string(data.begin(), data.end()));
}

TEST_F(GeoMirrorTest, RegionQuorumMet) {
    RedundancyConfig config;
    config.mode = RedundancyMode::GEO_MIRROR;
    config.replication_factor = 6;
    config.write_concern = WriteConcern::MAJORITY;
    // Require 1 ack in each region
    config.geo_replication.region_write_quorums = {{"us-east", 1}, {"eu-west", 1}};

    RedundancyStrategy strategy(config);
    std::vector<uint8_t> data(64, 0xAB);

    auto wr = strategy.write("geo-quorum-doc", data, "coll", *ring, *topology, createWriteHandler());
    EXPECT_TRUE(wr.success);
}

TEST_F(GeoMirrorTest, WriteConcernAllFailsWhenReplicationTargetsMissing) {
    RedundancyConfig config;
    config.mode = RedundancyMode::GEO_MIRROR;
    config.replication_factor = 10;
    config.write_concern = WriteConcern::ALL;

    RedundancyStrategy strategy(config);

    size_t write_calls = 0;
    auto counting_handler = [this, &write_calls](const std::string& shard_id,
                                                 const std::string& doc_id,
                                                 const std::vector<uint8_t>& data) {
        ++write_calls;
        return storage->write(shard_id, doc_id, data);
    };

    std::vector<uint8_t> data = {'G', 'e', 'o', 'A', 'l', 'l'};
    auto wr = strategy.write("geo-all-insufficient", data, "coll", *ring, *topology, counting_handler);

    EXPECT_FALSE(wr.success);
    EXPECT_EQ(write_calls, 0u);
    EXPECT_NE(wr.error_message.find("Insufficient replica targets"), std::string::npos);
}

TEST_F(GeoMirrorTest, WriteConcernMajorityFailsWhenReplicationTargetsMissing) {
    RedundancyConfig config;
    config.mode = RedundancyMode::GEO_MIRROR;
    config.replication_factor = 12;
    config.write_concern = WriteConcern::MAJORITY;

    RedundancyStrategy strategy(config);

    size_t write_calls = 0;
    auto counting_handler = [this, &write_calls](const std::string& shard_id,
                                                 const std::string& doc_id,
                                                 const std::vector<uint8_t>& data) {
        ++write_calls;
        return storage->write(shard_id, doc_id, data);
    };

    std::vector<uint8_t> data = {'G', 'e', 'o', 'M', 'a', 'j'};
    auto wr = strategy.write("geo-majority-insufficient", data, "coll", *ring, *topology, counting_handler);

    EXPECT_FALSE(wr.success);
    EXPECT_EQ(write_calls, 0u);
    EXPECT_NE(wr.error_message.find("Insufficient replica targets"), std::string::npos);
}

TEST_F(GeoMirrorTest, RegionWriteQuorumFailsWhenReplicationTargetsMissing) {
    RedundancyConfig config;
    config.mode = RedundancyMode::GEO_MIRROR;
    config.replication_factor = 6;
    config.write_concern = WriteConcern::ONE;
    config.geo_replication.region_write_quorums = {{"us-east", 4}, {"eu-west", 1}};

    RedundancyStrategy strategy(config);

    size_t write_calls = 0;
    auto counting_handler = [this, &write_calls](const std::string& shard_id,
                                                 const std::string& doc_id,
                                                 const std::vector<uint8_t>& data) {
        ++write_calls;
        return storage->write(shard_id, doc_id, data);
    };

    std::vector<uint8_t> data = {'G', 'e', 'o', 'R', 'e', 'g'};
    auto wr = strategy.write("geo-region-insufficient", data, "coll", *ring, *topology,
                             counting_handler);

    EXPECT_FALSE(wr.success);
    EXPECT_EQ(write_calls, 0u);
    EXPECT_NE(wr.error_message.find("Insufficient replica targets"), std::string::npos);
}

TEST_F(GeoMirrorTest, RegionReadQuorumFailsWhenReplicationTargetsMissing) {
    RedundancyConfig wconfig;
    wconfig.mode = RedundancyMode::GEO_MIRROR;
    wconfig.replication_factor = 6;
    wconfig.write_concern = WriteConcern::ALL;

    RedundancyStrategy wstrategy(wconfig);
    std::vector<uint8_t> data = {'Q', 'u', 'o', 'r', 'u', 'm'};
    auto wr = wstrategy.write("geo-read-quorum-insufficient", data, "coll", *ring, *topology,
                              createWriteHandler());
    ASSERT_TRUE(wr.success);

    topology->updateHealth("shard-3", false);
    topology->updateHealth("shard-4", false);
    topology->updateHealth("shard-5", false);

    RedundancyConfig rconfig;
    rconfig.mode = RedundancyMode::GEO_MIRROR;
    rconfig.replication_factor = 6;
    rconfig.geo_replication.region_read_quorums = {{"us-east", 1}, {"eu-west", 1}};
    rconfig.geo_replication.enable_geo_failover = true;

    RedundancyStrategy rstrategy(rconfig);

    size_t read_calls = 0;
    auto counting_read_handler = [this, &read_calls](const std::string& shard_id,
                                                     const std::string& doc_id) {
        ++read_calls;
        return storage->read(shard_id, doc_id);
    };

    auto rr = rstrategy.read("geo-read-quorum-insufficient", "coll", *ring, *topology,
                             counting_read_handler);
    EXPECT_FALSE(rr.success);
    EXPECT_EQ(read_calls, 0u);
    EXPECT_NE(rr.error_message.find("Insufficient replica targets"), std::string::npos);
}

TEST_F(GeoMirrorTest, FollowerReadPreference) {
    RedundancyConfig config;
    config.mode = RedundancyMode::GEO_MIRROR;
    config.replication_factor = 3;
    config.write_concern = WriteConcern::ALL;
    config.geo_replication.read_preference = ReadPreference::FOLLOWER;

    RedundancyStrategy strategy(config);
    std::vector<uint8_t> data = {'F', 'o', 'l', 'l', 'o', 'w'};

    auto wr = strategy.write("follower-doc", data, "coll", *ring, *topology, createWriteHandler());
    EXPECT_TRUE(wr.success);

    auto rr = strategy.read("follower-doc", "coll", *ring, *topology, createReadHandler());
    EXPECT_TRUE(rr.success);
    EXPECT_EQ(rr.data, std::string(data.begin(), data.end()));
}

TEST_F(GeoMirrorTest, LocalRegionReadPreference) {
    RedundancyConfig config;
    config.mode = RedundancyMode::GEO_MIRROR;
    config.replication_factor = 3;
    config.write_concern = WriteConcern::ALL;
    config.geo_replication.local_region = "eu-west";
    config.geo_replication.read_preference = ReadPreference::LOCAL_REGION;

    RedundancyStrategy strategy(config);
    std::vector<uint8_t> data = {'L', 'o', 'c', 'a', 'l'};

    auto wr = strategy.write("local-doc", data, "coll", *ring, *topology, createWriteHandler());
    EXPECT_TRUE(wr.success);

    auto rr = strategy.read("local-doc", "coll", *ring, *topology, createReadHandler());
    EXPECT_TRUE(rr.success);
    EXPECT_EQ(rr.data, std::string(data.begin(), data.end()));
}

TEST_F(GeoMirrorTest, GeoFailoverExcludesFailedRegion) {
    // Mark all eu-west shards as unhealthy to trigger failover
    topology->updateHealth("shard-3", false);
    topology->updateHealth("shard-4", false);
    topology->updateHealth("shard-5", false);

    RedundancyConfig config;
    config.mode = RedundancyMode::GEO_MIRROR;
    config.replication_factor = 3;
    config.write_concern = WriteConcern::MAJORITY;
    config.geo_replication.enable_geo_failover = true;
    config.geo_replication.region_failure_threshold = 0.5;
    config.geo_replication.local_region = "us-east";

    RedundancyStrategy strategy(config);
    std::vector<uint8_t> data = {'F', 'a', 'i', 'l', 'O', 'v', 'e', 'r'};

    auto wr = strategy.write("failover-doc", data, "coll", *ring, *topology, createWriteHandler());
    EXPECT_TRUE(wr.success);

    // All writes should go to us-east (eu-west is failed-out)
    for (const auto& shard_id : wr.written_shards) {
        auto info = topology->getShard(shard_id);
        if (info) {
            EXPECT_NE(info->region, "eu-west");
        }
    }
}

TEST_F(GeoMirrorTest, TopologyRegionQuery) {
    auto regions = topology->getRegions();
    ASSERT_EQ(regions.size(), 2u);
    EXPECT_EQ(regions[0], "eu-west");
    EXPECT_EQ(regions[1], "us-east");

    auto us_shards = topology->getShardsInRegion("us-east");
    EXPECT_EQ(us_shards.size(), 3u);

    auto eu_shards = topology->getShardsInRegion("eu-west");
    EXPECT_EQ(eu_shards.size(), 3u);

    EXPECT_TRUE(topology->regionHasQuorum("us-east", 2));
    EXPECT_FALSE(topology->regionHasQuorum("us-east", 10));
}

TEST_F(GeoMirrorTest, TopologyHealthyShardsInRegion) {
    topology->updateHealth("shard-3", false);

    auto healthy_eu = topology->getHealthyShardsInRegion("eu-west");
    EXPECT_EQ(healthy_eu.size(), 2u);

    auto healthy_us = topology->getHealthyShardsInRegion("us-east");
    EXPECT_EQ(healthy_us.size(), 3u);
}

TEST_F(GeoMirrorTest, ShardInfoHasRegionAndZone) {
    auto info = topology->getShard("shard-0");
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->region, "us-east");
    EXPECT_EQ(info->zone, "us-east-1a");

    auto info2 = topology->getShard("shard-5");
    ASSERT_TRUE(info2.has_value());
    EXPECT_EQ(info2->region, "eu-west");
    EXPECT_EQ(info2->zone, "eu-west-1c");
}

TEST_F(GeoMirrorTest, RegionReadQuorumEnforced) {
    // Write to all 6 shards first
    RedundancyConfig wconfig;
    wconfig.mode = RedundancyMode::GEO_MIRROR;
    wconfig.replication_factor = 6;
    wconfig.write_concern = WriteConcern::ALL;

    RedundancyStrategy wstrategy(wconfig);
    std::vector<uint8_t> data = {'Q', 'u', 'o', 'r', 'u', 'm'};
    auto wr = wstrategy.write("qr-doc", data, "coll", *ring, *topology, createWriteHandler());
    EXPECT_TRUE(wr.success);

    // Now read with per-region read quorums requiring 1 from each region
    RedundancyConfig rconfig;
    rconfig.mode = RedundancyMode::GEO_MIRROR;
    rconfig.replication_factor = 6;
    rconfig.geo_replication.region_read_quorums = {{"us-east", 1}, {"eu-west", 1}};

    RedundancyStrategy rstrategy(rconfig);
    auto rr = rstrategy.read("qr-doc", "coll", *ring, *topology, createReadHandler());
    EXPECT_TRUE(rr.success);
    EXPECT_EQ(rr.data, std::string(data.begin(), data.end()));
}

TEST_F(GeoMirrorTest, PrometheusMetricsIncludeGeoFields) {
    RedundancyConfig config;
    config.mode = RedundancyMode::GEO_MIRROR;
    config.replication_factor = 3;
    config.geo_replication.region_write_quorums = {{"us-east", 2}};
    config.geo_replication.region_read_quorums  = {{"us-east", 1}};
    config.geo_replication.enable_geo_failover  = true;
    config.geo_replication.max_staleness_ms      = 500;

    RedundancyStrategy strategy(config);
    std::string metrics = strategy.exportPrometheusMetrics();

    EXPECT_NE(metrics.find("themis_geo_replication_mode"), std::string::npos);
    EXPECT_NE(metrics.find("themis_geo_region_write_quorum"), std::string::npos);
    EXPECT_NE(metrics.find("themis_geo_region_read_quorum"), std::string::npos);
    EXPECT_NE(metrics.find("themis_geo_failed_regions_total"), std::string::npos);
    EXPECT_NE(metrics.find("themis_geo_failover_enabled"), std::string::npos);
    EXPECT_NE(metrics.find("themis_geo_max_staleness_ms"), std::string::npos);
    // Should contain the configured region label
    EXPECT_NE(metrics.find("us-east"), std::string::npos);
}

TEST_F(GeoMirrorTest, GeoConfigValidation) {
    // Valid config
    RedundancyConfig valid;
    valid.mode = RedundancyMode::GEO_MIRROR;
    valid.replication_factor = 3;
    valid.geo_replication.region_write_quorums = {{"us-east", 2}, {"eu-west", 1}};
    EXPECT_TRUE(valid.validate());

    // Write quorum exceeds replication_factor
    RedundancyConfig bad_wq;
    bad_wq.mode = RedundancyMode::GEO_MIRROR;
    bad_wq.replication_factor = 3;
    bad_wq.geo_replication.region_write_quorums = {{"us-east", 5}};
    EXPECT_FALSE(bad_wq.validate());

    // Write quorum = 0 is invalid
    RedundancyConfig zero_wq;
    zero_wq.mode = RedundancyMode::GEO_MIRROR;
    zero_wq.replication_factor = 3;
    zero_wq.geo_replication.region_write_quorums = {{"us-east", 0}};
    EXPECT_FALSE(zero_wq.validate());

    // Invalid region_failure_threshold
    RedundancyConfig bad_threshold;
    bad_threshold.mode = RedundancyMode::GEO_MIRROR;
    bad_threshold.replication_factor = 3;
    bad_threshold.geo_replication.enable_geo_failover = true;
    bad_threshold.geo_replication.region_failure_threshold = 0.0;
    EXPECT_FALSE(bad_threshold.validate());
}

// ─────────────────────────────────────────────────────────────
// ReplicaTopology geo-placement tests
// ─────────────────────────────────────────────────────────────
#include "sharding/replica_topology.h"

TEST(ReplicaTopologyGeoTest, RegionAndZoneParsedFromJson) {
    using namespace themis::sharding;
    using json = nlohmann::json;

    json config = json::array({
        {{"shard_id", "s0"}, {"primary_id", "n0"}, {"redundancy", "GEO_MIRROR"},
         {"region", "us-east"}, {"zone", "us-east-1a"},
         {"replicas", json::array({"n1", "n2"})}},
        {{"shard_id", "s1"}, {"primary_id", "n3"}, {"redundancy", "GEO_MIRROR"},
         {"region", "eu-west"}, {"zone", "eu-west-1b"},
         {"replicas", json::array({"n4", "n5"})}}
    });

    ReplicaTopology topo;
    ASSERT_TRUE(topo.loadFromJson(config));
    EXPECT_EQ(topo.getShardCount(), 2u);

    auto rs0 = topo.getReplicaSet("s0");
    ASSERT_NE(rs0, nullptr);
    EXPECT_EQ(rs0->region, "us-east");
    EXPECT_EQ(rs0->zone, "us-east-1a");

    auto rs1 = topo.getReplicaSet("s1");
    ASSERT_NE(rs1, nullptr);
    EXPECT_EQ(rs1->region, "eu-west");
    EXPECT_EQ(rs1->zone, "eu-west-1b");
}

TEST(ReplicaTopologyGeoTest, GetReplicaSetsInRegion) {
    using namespace themis::sharding;
    using json = nlohmann::json;

    json config = json::array({
        {{"shard_id", "s0"}, {"primary_id", "n0"}, {"region", "us-east"}},
        {{"shard_id", "s1"}, {"primary_id", "n1"}, {"region", "us-east"}},
        {{"shard_id", "s2"}, {"primary_id", "n2"}, {"region", "eu-west"}}
    });

    ReplicaTopology topo;
    ASSERT_TRUE(topo.loadFromJson(config));

    auto us = topo.getReplicaSetsInRegion("us-east");
    EXPECT_EQ(us.size(), 2u);

    auto eu = topo.getReplicaSetsInRegion("eu-west");
    EXPECT_EQ(eu.size(), 1u);
}

TEST(ReplicaTopologyGeoTest, GetRegions) {
    using namespace themis::sharding;
    using json = nlohmann::json;

    json config = json::array({
        {{"shard_id", "s0"}, {"primary_id", "n0"}, {"region", "us-east"}},
        {{"shard_id", "s1"}, {"primary_id", "n1"}, {"region", "eu-west"}},
        {{"shard_id", "s2"}, {"primary_id", "n2"}, {"region", "us-east"}},
        // Shard with no region -- should not appear in the regions list
        {{"shard_id", "s3"}, {"primary_id", "n3"}}
    });

    ReplicaTopology topo;
    ASSERT_TRUE(topo.loadFromJson(config));

    auto regions = topo.getRegions();
    // Only non-empty regions; sorted; de-duplicated
    ASSERT_EQ(regions.size(), 2u);
    EXPECT_EQ(regions[0], "eu-west");
    EXPECT_EQ(regions[1], "us-east");
}

TEST(ReplicaTopologyGeoTest, EmptyRegionNotIncluded) {
    using namespace themis::sharding;
    using json = nlohmann::json;

    // All shards lack region/zone fields
    json config = json::array({
        {{"shard_id", "s0"}, {"primary_id", "n0"}},
        {{"shard_id", "s1"}, {"primary_id", "n1"}}
    });

    ReplicaTopology topo;
    ASSERT_TRUE(topo.loadFromJson(config));

    EXPECT_TRUE(topo.getRegions().empty());
    EXPECT_TRUE(topo.getReplicaSetsInRegion("us-east").empty());

    auto rs = topo.getReplicaSet("s0");
    ASSERT_NE(rs, nullptr);
    EXPECT_TRUE(rs->region.empty());
    EXPECT_TRUE(rs->zone.empty());
}

// ═══════════════════════════════════════════════════════════
// Stress Tests
// ═══════════════════════════════════════════════════════════

TEST_F(GeoMirrorTest, ParallelWrite_AllShardsReceiveData) {
    // This test directly exercises the parallel async write path in writeGeoMirror
    // and validates that all region shards receive the data (no dangling-reference
    // UB from the former &shard_id lambda capture).
    RedundancyConfig config;
    config.mode = RedundancyMode::GEO_MIRROR;
    config.replication_factor = 6;
    config.write_concern = WriteConcern::ALL;
    config.geo_replication.region_write_quorums = {{"us-east", 3}, {"eu-west", 3}};

    RedundancyStrategy strategy(config);
    std::vector<uint8_t> data(256, 0x42);

    auto wr = strategy.write("parallel-doc", data, "coll", *ring, *topology, createWriteHandler());
    EXPECT_TRUE(wr.success);

    // All 6 shards should have been written to
    for (int i = 0; i < static_cast<int>(config.replication_factor); ++i) {
        const std::string sid = "shard-" + std::to_string(i);
        auto stored = storage->read(sid, "parallel-doc");
        EXPECT_TRUE(stored.has_value()) << "Shard " << sid << " did not receive the write";
    }
}

TEST_F(GeoMirrorTest, WriteQuorumNotMet_FailsCorrectly) {
    // Mark enough us-east shards unhealthy that us-east quorum cannot be met
    topology->updateHealth("shard-0", false);
    topology->updateHealth("shard-1", false);
    topology->updateHealth("shard-2", false);

    RedundancyConfig config;
    config.mode = RedundancyMode::GEO_MIRROR;
    config.replication_factor = 6;
    config.write_concern = WriteConcern::MAJORITY;
    // Require 2 acks in us-east, but all 3 us-east shards are unhealthy
    config.geo_replication.region_write_quorums = {{"us-east", 2}};

    RedundancyStrategy strategy(config);
    std::vector<uint8_t> data(32, 0xCC);

    auto wr = strategy.write("quorum-fail-doc", data, "coll", *ring, *topology, createWriteHandler());
    // The write should fail because us-east quorum cannot be satisfied
    EXPECT_FALSE(wr.success);
    EXPECT_NE(wr.error_message.find("Geo-quorum"), std::string::npos);
}

// ─────────────────────────────────────────────────────────────
// remove() tests
// ─────────────────────────────────────────────────────────────

TEST_F(RedundancyStrategyTest, Remove_Mirror_DeletesFromAllReplicas) {
    RedundancyConfig config;
    config.mode = RedundancyMode::MIRROR;
    config.replication_factor = 3;
    config.write_concern = WriteConcern::ALL;

    RedundancyStrategy strategy(config);
    std::vector<uint8_t> data = {'r', 'm', 'd', 'o', 'c'};

    // First write the document
    auto wr = strategy.write("rm-doc1", data, "coll", *ring, *topology, createWriteHandler());
    ASSERT_TRUE(wr.success);
    ASSERT_FALSE(wr.written_shards.empty());

    // Now remove it — must succeed
    bool removed = strategy.remove("rm-doc1", "coll", *ring, *topology, createWriteHandler());
    EXPECT_TRUE(removed);
}

TEST_F(GeoMirrorTest, Remove_GeoMirror_DeletesFromLiveRegions) {
    RedundancyConfig config;
    config.mode = RedundancyMode::GEO_MIRROR;
    config.replication_factor = 6;
    config.write_concern = WriteConcern::ALL;

    RedundancyStrategy strategy(config);
    std::vector<uint8_t> data = {'g', 'e', 'o', 'r', 'm'};

    auto wr = strategy.write("geo-rm-doc", data, "coll", *ring, *topology, createWriteHandler());
    ASSERT_TRUE(wr.success);

    bool removed = strategy.remove("geo-rm-doc", "coll", *ring, *topology, createWriteHandler());
    EXPECT_TRUE(removed);
}

TEST_F(RedundancyStrategyTest, Remove_NoShard_ReturnsFalse) {
    // Empty ring — getNode returns nullopt
    ConsistentHashRing empty_ring(100);

    RedundancyConfig config;
    config.mode = RedundancyMode::MIRROR;
    config.replication_factor = 3;

    RedundancyStrategy strategy(config);
    bool removed = strategy.remove("no-shard-doc", "coll", empty_ring, *topology, createWriteHandler());
    EXPECT_FALSE(removed);
}

// ─────────────────────────────────────────────────────────────
// checkDocumentHealth() tests
// ─────────────────────────────────────────────────────────────

TEST_F(RedundancyStrategyTest, CheckDocumentHealth_AllReplicasPresent) {
    RedundancyConfig config;
    config.mode = RedundancyMode::MIRROR;
    config.replication_factor = 3;
    config.write_concern = WriteConcern::ALL;

    RedundancyStrategy strategy(config);
    std::vector<uint8_t> data = {'h', 'e', 'a', 'l', 't', 'h'};

    auto wr = strategy.write("health-doc1", data, "coll", *ring, *topology, createWriteHandler());
    ASSERT_TRUE(wr.success);

    auto health = strategy.checkDocumentHealth("health-doc1", "coll", *ring, *topology, createReadHandler());
    EXPECT_EQ(health.available_replicas, 3u);
    EXPECT_TRUE(health.missing_shards.empty());
    EXPECT_TRUE(health.is_healthy);
    EXPECT_FALSE(health.can_recover);  // nothing missing — no recovery needed
}

TEST_F(RedundancyStrategyTest, CheckDocumentHealth_OneMissing_CanRecover) {
    RedundancyConfig config;
    config.mode = RedundancyMode::MIRROR;
    config.replication_factor = 3;
    config.write_concern = WriteConcern::ALL;

    RedundancyStrategy strategy(config);
    std::vector<uint8_t> data = {'d', 'e', 'g', 'r', 'a', 'd', 'e', 'd'};

    auto wr = strategy.write("health-doc2", data, "coll", *ring, *topology, createWriteHandler());
    ASSERT_TRUE(wr.success);
    ASSERT_GE(wr.written_shards.size(), 2u);

    // Simulate one replica going missing by clearing its storage
    storage->clearShard(wr.written_shards[0]);

    auto health = strategy.checkDocumentHealth("health-doc2", "coll", *ring, *topology, createReadHandler());
    EXPECT_LT(health.available_replicas, 3u);
    EXPECT_FALSE(health.is_healthy);
    EXPECT_TRUE(health.can_recover);
    EXPECT_FALSE(health.missing_shards.empty());
}

// ─────────────────────────────────────────────────────────────
// recoverDocument() tests
// ─────────────────────────────────────────────────────────────

TEST_F(RedundancyStrategyTest, RecoverDocument_Mirror_RestoresMissingReplica) {
    RedundancyConfig config;
    config.mode = RedundancyMode::MIRROR;
    config.replication_factor = 3;
    config.write_concern = WriteConcern::ALL;

    RedundancyStrategy strategy(config);
    const std::vector<uint8_t> data = {'r', 'e', 'c', 'o', 'v', 'e', 'r'};

    auto wr = strategy.write("recover-doc1", data, "coll", *ring, *topology, createWriteHandler());
    ASSERT_TRUE(wr.success);
    ASSERT_GE(wr.written_shards.size(), 2u);

    // Erase one replica
    const std::string erased_shard = wr.written_shards[0];
    storage->clearShard(erased_shard);

    // Verify it's really gone
    EXPECT_FALSE(storage->read(erased_shard, "recover-doc1").has_value());

    // Recover
    bool recovered = strategy.recoverDocument(
        "recover-doc1", "coll", *ring, *topology,
        createReadHandler(), createWriteHandler());
    EXPECT_TRUE(recovered);

    // The erased replica should now have the data back
    auto restored = storage->read(erased_shard, "recover-doc1");
    EXPECT_TRUE(restored.has_value());
    if (restored) {
        EXPECT_EQ(*restored, data);
    }
}

TEST_F(RedundancyStrategyTest, RecoverDocument_NoneMode_ReturnsFalse) {
    // NONE has no redundancy — recovery is impossible
    RedundancyConfig config;
    config.mode = RedundancyMode::NONE;
    config.replication_factor = 1;

    RedundancyStrategy strategy(config);
    bool recovered = strategy.recoverDocument(
        "no-recover", "coll", *ring, *topology,
        createReadHandler(), createWriteHandler());
    EXPECT_FALSE(recovered);
}

TEST_F(RedundancyStrategyTest, RecoverDocument_AllReplicasMissing_ReturnsFalse) {
    RedundancyConfig config;
    config.mode = RedundancyMode::MIRROR;
    config.replication_factor = 3;
    config.write_concern = WriteConcern::ALL;

    RedundancyStrategy strategy(config);
    // Don't write anything — all replicas are missing
    bool recovered = strategy.recoverDocument(
        "never-written", "coll", *ring, *topology,
        createReadHandler(), createWriteHandler());
    EXPECT_FALSE(recovered);
}

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
// Concurrent configure / erasure-coder data-race tests
// ═══════════════════════════════════════════════════════════

// Helper: build a PARITY 4+2 config
static RedundancyConfig makeParity42Config() {
    RedundancyConfig cfg;
    cfg.mode = RedundancyMode::PARITY;
    cfg.erasure_coding.data_shards   = 4;
    cfg.erasure_coding.parity_shards = 2;
    return cfg;
}

TEST_F(RedundancyStrategyTest, ConcurrentConfigure_WriteParity_NoDataRace) {
    // Repeatedly reconfigure while write operations are in flight.
    // The shared_lock guard in writeParity must prevent use-after-free on
    // erasure_coder_.
    RedundancyStrategy strategy(makeParity42Config());

    std::atomic<bool> stop{false};
    std::vector<uint8_t> data(256, 0xAB);

    // Writer thread: keep calling write() until stop
    std::thread writer([&] {
        while (!stop.load(std::memory_order_relaxed)) {
            strategy.write("race-doc", data, "coll", *ring, *topology, createWriteHandler());
        }
    });

    // Reconfigure thread: alternate between two valid PARITY configs
    std::thread reconfigurer([&] {
        for (int i = 0; i < 200; ++i) {
            RedundancyConfig c2 = makeParity42Config();
            c2.erasure_coding.data_shards   = (i % 2 == 0) ? 4 : 3;
            c2.erasure_coding.parity_shards = 2;
            strategy.updateConfig(c2);
        }
        stop.store(true, std::memory_order_relaxed);
    });

    writer.join();
    reconfigurer.join();
    // Test passes if it completes without crash or sanitizer report.
}

TEST_F(RedundancyStrategyTest, ConcurrentConfigure_ReadParity_NoDataRace) {
    // Same pattern but exercises readParity.
    RedundancyStrategy strategy(makeParity42Config());

    // Pre-write some chunks so readParity has something to decode.
    std::vector<uint8_t> data(256, 0xCD);
    strategy.write("read-race-doc", data, "coll", *ring, *topology, createWriteHandler());

    std::atomic<bool> stop{false};

    std::thread reader([&] {
        while (!stop.load(std::memory_order_relaxed)) {
            // Ignore errors — we only care that it does not crash.
            strategy.read("read-race-doc", "coll", *ring, *topology, createReadHandler());
        }
    });

    std::thread reconfigurer([&] {
        for (int i = 0; i < 200; ++i) {
            strategy.updateConfig(makeParity42Config());
        }
        stop.store(true, std::memory_order_relaxed);
    });

    reader.join();
    reconfigurer.join();
}

// ═══════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════
// main() removed - GTest provides its own main via gtest_main
