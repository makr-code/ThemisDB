/**
 * Integration tests for WAL Shipper and Applier
 * 
 * Tests the complete replication pipeline
 */

#include <gtest/gtest.h>
#include "sharding/wal_manager.h"
#include "sharding/wal_shipper.h"
#include "sharding/wal_applier.h"
#include <filesystem>
#include <thread>

using namespace themis::sharding;

class WALReplicationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temp directories
        primary_dir_ = "/tmp/themis_wal_primary_test";
        replica_dir_ = "/tmp/themis_wal_replica_test";
        
        std::filesystem::remove_all(primary_dir_);
        std::filesystem::remove_all(replica_dir_);
        
        std::filesystem::create_directories(primary_dir_);
        std::filesystem::create_directories(replica_dir_);
        
        // Configure primary WAL
        WALManagerConfig primary_config;
        primary_config.wal_directory = primary_dir_;
        primary_config.segment_size = 1024;
        primary_config.sync_on_write = true;
        
        primary_wal_ = std::make_shared<WALManager>(primary_config);
        
        // Configure replica WAL
        WALManagerConfig replica_config;
        replica_config.wal_directory = replica_dir_;
        replica_config.segment_size = 1024;
        replica_config.sync_on_write = true;
        
        replica_wal_ = std::make_shared<WALManager>(replica_config);
    }
    
    void TearDown() override {
        primary_wal_.reset();
        replica_wal_.reset();

        std::error_code ec;
        std::filesystem::remove_all(primary_dir_, ec);
        ec.clear();
        std::filesystem::remove_all(replica_dir_, ec);
    }
    
    std::string primary_dir_;
    std::string replica_dir_;
    std::shared_ptr<WALManager> primary_wal_;
    std::shared_ptr<WALManager> replica_wal_;
};

// ============================================================================
// WAL Applier Tests
// ============================================================================

TEST_F(WALReplicationTest, ApplierBasicApply) {
    WALApplierConfig config;
    config.replica_id = "replica_1";
    config.strict_mode = false;
    
    WALApplier applier(config);
    
    std::vector<WALEntry> applied_entries;
    
    // Set apply handler
    applier.setApplyHandler([&applied_entries](const WALEntry& entry) {
        applied_entries.push_back(entry);
        return true;
    });
    
    // Create entries to apply
    std::vector<WALEntry> entries = {};

    for (int i = 0; i < 5; ++i) {
        WALEntry entry;
        entry.lsn = LSN(0, i * 100);
        entry.type = WALEntryType::INSERT;
        entry.data = {{"index", i}};
        entries.push_back(entry);
    }
    
    // Apply batch
    auto result = applier.applyBatch(entries);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.entries_applied, 5);
    EXPECT_EQ(applied_entries.size(), 5);
}

TEST_F(WALReplicationTest, ApplierLSNTracking) {
    WALApplierConfig config;
    config.replica_id = "replica_1";
    config.strict_mode = false;
    
    WALApplier applier(config);
    
    applier.setApplyHandler([](const WALEntry&) { return true; });
    
    // Initial LSN should be 0/0
    EXPECT_EQ(applier.getCurrentLSN().segment, 0);
    EXPECT_EQ(applier.getCurrentLSN().offset, 0);
    
    // Apply entry
    WALEntry entry;
    entry.lsn = LSN(0, 100);
    entry.type = WALEntryType::INSERT;
    entry.data = {{"test", "data"}};
    
    applier.applyBatch({entry});
    
    // LSN should be updated
    EXPECT_EQ(applier.getCurrentLSN().segment, 0);
    EXPECT_EQ(applier.getCurrentLSN().offset, 100);
}

TEST_F(WALReplicationTest, ApplierStrictMode) {
    WALApplierConfig config;
    config.replica_id = "replica_1";
    config.strict_mode = true;
    
    WALApplier applier(config);
    applier.setApplyHandler([](const WALEntry&) { return true; });
    
    // Set current LSN
    applier.setCurrentLSN(LSN(0, 100));
    
    // Try to apply entry with correct LSN
    WALEntry correct_entry;
    correct_entry.lsn = LSN(0, 101);
    correct_entry.type = WALEntryType::INSERT;
    
    auto result1 = applier.applyBatch({correct_entry});
    EXPECT_TRUE(result1.success);
    
    // Try to apply entry with wrong LSN (gap)
    WALEntry wrong_entry;
    wrong_entry.lsn = LSN(0, 200);  // Gap!
    wrong_entry.type = WALEntryType::INSERT;
    
    auto result2 = applier.applyBatch({wrong_entry});
    EXPECT_FALSE(result2.success);
    EXPECT_GT(result2.errors.size(), 0);
}

TEST_F(WALReplicationTest, ApplierStrictModeRejectsDuplicateLSNFailClosed) {
    WALApplierConfig config;
    config.replica_id = "replica_1";
    config.strict_mode = true;

    WALApplier applier(config);
    applier.setApplyHandler([](const WALEntry&) { return true; });
    applier.setCurrentLSN(LSN(0, 10));

    WALEntry next_entry;
    next_entry.lsn = LSN(0, 11);
    next_entry.type = WALEntryType::INSERT;
    ASSERT_TRUE(applier.applyBatch({next_entry}).success);

    WALEntry duplicate_entry;
    duplicate_entry.lsn = LSN(0, 11);
    duplicate_entry.type = WALEntryType::INSERT;

    const auto duplicate_result = applier.applyBatch({duplicate_entry});
    EXPECT_FALSE(duplicate_result.success);
    ASSERT_FALSE(duplicate_result.errors.empty());
    EXPECT_NE(duplicate_result.errors.front().find("stale or duplicate"), std::string::npos);
}

TEST_F(WALReplicationTest, ApplierStatistics) {
    WALApplierConfig config;
    config.replica_id = "replica_1";
    
    WALApplier applier(config);
    applier.setApplyHandler([](const WALEntry&) { return true; });
    
    // Apply multiple entries
    for (int i = 0; i < 10; ++i) {
        WALEntry entry;
        entry.lsn = LSN(0, i * 100);
        entry.type = WALEntryType::INSERT;
        entry.data = {{"index", i}};
        
        applier.applyBatch({entry});
    }
    
    auto stats = applier.getStatistics();
    EXPECT_EQ(stats.total_entries_applied, 10);
    EXPECT_GT(stats.total_bytes_applied, 0);
}

TEST_F(WALReplicationTest, ApplierRetryOnFailure) {
    WALApplierConfig config;
    config.replica_id = "replica_1";
    config.max_apply_retries = 3;
    
    WALApplier applier(config);
    
    std::atomic<int> attempt_count{0};
    
    // Handler that fails first 2 times, succeeds on 3rd
    applier.setApplyHandler([&attempt_count](const WALEntry&) {
        attempt_count++;
        return attempt_count >= 3;
    });
    
    WALEntry entry;
    entry.lsn = LSN(0, 0);
    entry.type = WALEntryType::INSERT;
    
    auto result = applier.applyBatch({entry});
    EXPECT_TRUE(result.success);
    EXPECT_EQ(attempt_count, 3);
}

// ============================================================================
// WAL Shipper Tests (Mock)
// ============================================================================

TEST_F(WALReplicationTest, ShipperBasicConfiguration) {
    WALShipperConfig config;
    config.primary_id = "primary_1";
    config.batch_size = 10;
    config.ship_interval_ms = 100;
    
    WALShipper shipper(primary_wal_, config);
    
    EXPECT_FALSE(shipper.isRunning());
    
    shipper.addReplica("replica_1", "https://replica1:8080");
    
    auto replicas = shipper.getReplicaInfo();
    EXPECT_EQ(replicas.size(), 1);
    EXPECT_EQ(replicas[0].replica_id, "replica_1");
}

TEST_F(WALReplicationTest, ShipperReplicaManagement) {
    WALShipperConfig config;
    config.primary_id = "primary_1";
    
    WALShipper shipper(primary_wal_, config);
    
    // Add replicas
    shipper.addReplica("replica_1", "https://replica1:8080");
    shipper.addReplica("replica_2", "https://replica2:8080");
    shipper.addReplica("replica_3", "https://replica3:8080");
    
    auto replicas = shipper.getReplicaInfo();
    EXPECT_EQ(replicas.size(), 3);
    
    // Remove replica
    shipper.removeReplica("replica_2");
    
    replicas = shipper.getReplicaInfo();
    EXPECT_EQ(replicas.size(), 2);
}

TEST_F(WALReplicationTest, ShipperStatistics) {
    WALShipperConfig config;
    config.primary_id = "primary_1";
    
    WALShipper shipper(primary_wal_, config);
    
    auto stats = shipper.getStatistics();
    EXPECT_EQ(stats.total_entries_shipped, 0);
    EXPECT_EQ(stats.total_batches, 0);
}

// ============================================================================
// End-to-End Replication Test (Simulated)
// ============================================================================

TEST_F(WALReplicationTest, SimulatedReplication) {
    // Setup applier on "replica"
    WALApplierConfig applier_config;
    applier_config.replica_id = "replica_1";
    applier_config.strict_mode = false;
    
    WALApplier applier(applier_config);
    
    // Apply handler writes to replica WAL
    applier.setApplyHandler([this](const WALEntry& entry) {
        WALEntry replica_entry = entry;
        replica_wal_->append(replica_entry);
        return true;
    });
    
    // Write entries to primary
    std::vector<LSN> primary_lsns = {};

    for (int i = 0; i < 10; ++i) {
        WALEntry entry;
        entry.type = WALEntryType::INSERT;
        entry.data = {{"index", i}, {"value", "test_" + std::to_string(i)}};
        
        LSN lsn = primary_wal_->append(entry);
        primary_lsns.push_back(lsn);
    }
    
    primary_wal_->flush();
    
    // Read from primary and apply to replica
    auto primary_entries = primary_wal_->readRange(LSN(0, 0));
    EXPECT_EQ(primary_entries.size(), 10);
    
    auto result = applier.applyBatch(primary_entries);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.entries_applied, 10);
    
    // Verify replica has all entries
    replica_wal_->flush();
    auto replica_entries = replica_wal_->readRange(LSN(0, 0));
    EXPECT_EQ(replica_entries.size(), 10);
    
    // Verify content matches
    for (size_t i = 0; i < primary_entries.size(); ++i) {
        EXPECT_EQ(primary_entries[i].data["index"], replica_entries[i].data["index"]);
        EXPECT_EQ(primary_entries[i].data["value"], replica_entries[i].data["value"]);
    }
}

TEST_F(WALReplicationTest, ReplicationWithTransactions) {
    WALApplierConfig applier_config;
    applier_config.replica_id = "replica_1";
    
    WALApplier applier(applier_config);
    applier.setApplyHandler([this](const WALEntry& entry) {
        replica_wal_->append(entry);
        return true;
    });
    
    // Write transaction to primary
    std::string tx_id = "tx_12345";
    
    // BEGIN
    WALEntry begin_entry;
    begin_entry.type = WALEntryType::BEGIN_TX;
    begin_entry.transaction_id = tx_id;
    primary_wal_->append(begin_entry);
    
    // INSERT
    WALEntry insert_entry;
    insert_entry.type = WALEntryType::INSERT;
    insert_entry.transaction_id = tx_id;
    insert_entry.data = {{"table", "users"}, {"id", 1}};
    primary_wal_->append(insert_entry);
    
    // UPDATE
    WALEntry update_entry;
    update_entry.type = WALEntryType::UPDATE;
    update_entry.transaction_id = tx_id;
    update_entry.data = {{"table", "users"}, {"id", 1}, {"status", "active"}};
    primary_wal_->append(update_entry);
    
    // COMMIT
    WALEntry commit_entry;
    commit_entry.type = WALEntryType::COMMIT_TX;
    commit_entry.transaction_id = tx_id;
    primary_wal_->append(commit_entry);
    
    primary_wal_->flush();
    
    // Replicate to replica
    auto entries = primary_wal_->readRange(LSN(0, 0));
    auto result = applier.applyBatch(entries);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.entries_applied, 4);
    
    // Verify transaction on replica
    replica_wal_->flush();
    auto replica_entries = replica_wal_->readRange(LSN(0, 0));
    EXPECT_EQ(replica_entries.size(), 4);
    
    EXPECT_EQ(replica_entries[0].type, WALEntryType::BEGIN_TX);
    EXPECT_EQ(replica_entries[1].type, WALEntryType::INSERT);
    EXPECT_EQ(replica_entries[2].type, WALEntryType::UPDATE);
    EXPECT_EQ(replica_entries[3].type, WALEntryType::COMMIT_TX);
    
    for (const auto& entry : replica_entries) {
        EXPECT_EQ(entry.transaction_id, tx_id);
    }
}

// ============================================================================
// Catchup Scenario
// ============================================================================

TEST_F(WALReplicationTest, ReplicaCatchup) {
    WALApplierConfig applier_config;
    applier_config.replica_id = "replica_1";
    applier_config.strict_mode = false;
    
    WALApplier applier(applier_config);
    applier.setApplyHandler([this](const WALEntry& entry) {
        replica_wal_->append(entry);
        return true;
    });
    
    // Write 100 entries to primary
    for (int i = 0; i < 100; ++i) {
        WALEntry entry;
        entry.type = WALEntryType::INSERT;
        entry.data = {{"index", i}};
        primary_wal_->append(entry);
    }
    primary_wal_->flush();
    
    // Replica starts from LSN 0
    applier.setCurrentLSN(LSN(0, 0));
    
    // Catchup in batches of 20
    LSN current_replica_lsn = applier.getCurrentLSN();
    
    while (true) {
        LSN next_lsn = current_replica_lsn;
        next_lsn.offset++;
        
        auto entries = primary_wal_->readRange(next_lsn);
        if (entries.empty()) {
            break;
        }
        
        // Take first 20
        std::vector<WALEntry> batch(entries.begin(), 
                                    entries.begin() + std::min(size_t(20), entries.size()));
        
        auto result = applier.applyBatch(batch);
        EXPECT_TRUE(result.success);
        
        current_replica_lsn = applier.getCurrentLSN();
    }
    
    // Verify replica is caught up
    auto stats = applier.getStatistics();
    EXPECT_EQ(stats.total_entries_applied, 100);
}

// ============================================================================
// WAL Shipper Compression Tests
// ============================================================================

class WALShipperCompressionTest : public ::testing::Test {
protected:
    void SetUp() override {
        wal_dir_ = "/tmp/themis_shipper_compress_test";
        std::filesystem::remove_all(wal_dir_);
        std::filesystem::create_directories(wal_dir_);

        WALManagerConfig wal_cfg;
        wal_cfg.wal_directory = wal_dir_;
        wal_cfg.segment_size = 4 * 1024 * 1024;
        wal_cfg.sync_on_write = false;
        wal_ = std::make_shared<WALManager>(wal_cfg);
    }
    void TearDown() override {
        std::filesystem::remove_all(wal_dir_);
    }
    std::string wal_dir_;
    std::shared_ptr<WALManager> wal_;
};

TEST_F(WALShipperCompressionTest, DefaultCompressionIsZstd) {
    WALShipperConfig cfg;
    EXPECT_EQ(cfg.compression, WALShipperConfig::CompressionType::Zstd);
}

TEST_F(WALShipperCompressionTest, SelectCompressionSmallPayloadReturnsNone) {
    WALShipperConfig cfg;
    cfg.primary_id = "p1";
    WALShipper shipper(wal_, cfg);

    // Payloads below 4 KB should not be compressed regardless of other factors
    auto type = shipper.selectCompressionType(1024, true, 0.1);
    EXPECT_EQ(type, WALShipperConfig::CompressionType::None);
}

TEST_F(WALShipperCompressionTest, SelectCompressionHighCpuReturnsNone) {
    WALShipperConfig cfg;
    cfg.primary_id = "p1";
    WALShipper shipper(wal_, cfg);

    // CPU utilisation > 0.85 should bypass compression
    auto type = shipper.selectCompressionType(65536, true, 0.90);
    EXPECT_EQ(type, WALShipperConfig::CompressionType::None);
}

TEST_F(WALShipperCompressionTest, SelectCompressionRepetitiveLowCpuReturnsZstd) {
    WALShipperConfig cfg;
    cfg.primary_id = "p1";
    WALShipper shipper(wal_, cfg);

    // Large, repetitive payload under low CPU load → Zstd
    auto type = shipper.selectCompressionType(65536, true, 0.30);
    EXPECT_EQ(type, WALShipperConfig::CompressionType::Zstd);
}

TEST_F(WALShipperCompressionTest, SelectCompressionMediumCpuReturnsLZ4) {
    WALShipperConfig cfg;
    cfg.primary_id = "p1";
    WALShipper shipper(wal_, cfg);

    // Large non-repetitive payload with medium CPU → LZ4
    auto type = shipper.selectCompressionType(65536, false, 0.60);
    EXPECT_EQ(type, WALShipperConfig::CompressionType::LZ4);
}

TEST_F(WALShipperCompressionTest, CalculateOptimalBatchSizeDefaultNonAdaptive) {
    WALShipperConfig cfg;
    cfg.primary_id = "p1";
    cfg.batch_size = 100;
    cfg.adaptive_batch_size = false;
    WALShipper shipper(wal_, cfg);

    // When adaptive is off, always returns config batch_size
    EXPECT_EQ(shipper.calculateOptimalBatchSize(1.0, 0.5, 20000), cfg.batch_size);
    EXPECT_EQ(shipper.calculateOptimalBatchSize(100.0, 0.9, 1000), cfg.batch_size);
}

TEST_F(WALShipperCompressionTest, CalculateOptimalBatchSizeAdaptiveHighLatency) {
    WALShipperConfig cfg;
    cfg.primary_id = "p1";
    cfg.batch_size = 100;
    cfg.adaptive_batch_size = true;
    cfg.min_batch_size = 10;
    cfg.max_batch_size = 1000;
    WALShipper shipper(wal_, cfg);

    // High latency (>50ms) should push batch size up
    size_t high_lat = shipper.calculateOptimalBatchSize(100.0, 0.5, 20000);
    size_t low_lat  = shipper.calculateOptimalBatchSize(0.5,   0.5, 20000);
    EXPECT_GT(high_lat, low_lat);
}

TEST_F(WALShipperCompressionTest, InitialStatsHaveDefaultCompressionRatio) {
    WALShipperConfig cfg;
    cfg.primary_id = "p1";
    WALShipper shipper(wal_, cfg);

    auto stats = shipper.getStatistics();
    EXPECT_EQ(stats.total_batches, 0u);
    EXPECT_DOUBLE_EQ(stats.avg_compression_ratio, 1.0);
    EXPECT_EQ(stats.total_bytes_uncompressed, 0u);
}
