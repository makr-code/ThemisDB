// Unit tests for Phase 2 Performance Optimizations
// Based on scientific research from PR #156/#157

#include <gtest/gtest.h>
#include "performance/phase2_feature_flags.h"
#include "performance/wisckey.h"
#include "performance/dostoevsky.h"
#include "performance/cicada.h"
#include "performance/ligra.h"
#include "performance/rabitq.h"
#include <filesystem>
#include <fstream>

using namespace themis::performance;

// Test fixture for Phase 2
class Phase2Test : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = std::filesystem::temp_directory_path() / "themis_phase2_test";
        std::filesystem::create_directories(test_dir_);
    }

    void TearDown() override {
        std::filesystem::remove_all(test_dir_);
    }

    std::filesystem::path test_dir_;
};

// ==================== Feature Flags Tests ====================

TEST(Phase2FeatureFlagsTest, DefaultsAreFalse) {
    auto& flags = Phase2FeatureFlags::instance();
    EXPECT_FALSE(flags.wisckey_enabled());
    EXPECT_FALSE(flags.dostoevsky_enabled());
    EXPECT_FALSE(flags.cicada_enabled());
    EXPECT_FALSE(flags.ligra_enabled());
    EXPECT_FALSE(flags.rabitq_enabled());
}

TEST(Phase2FeatureFlagsTest, CanToggleFlags) {
    auto& flags = Phase2FeatureFlags::instance();
    
    flags.set_wisckey_enabled(true);
    EXPECT_TRUE(flags.wisckey_enabled());
    
    flags.set_wisckey_enabled(false);
    EXPECT_FALSE(flags.wisckey_enabled());
}

// ==================== WiscKey Tests ====================

TEST_F(Phase2Test, WiscKeySmallValuesInline) {
    auto log_path = test_dir_ / "value.log";
    WiscKeyStorage storage(log_path.string());
    
    std::string key = "test_key";
    std::string small_value = "small";  // < 1KB
    
    std::string encoded = storage.put(key, small_value);
    
    // Small value should be stored inline (not separated)
    EXPECT_FALSE(WiscKeyStorage::is_separated(encoded));
    EXPECT_EQ(encoded, small_value);
    
    auto retrieved = storage.get(key, encoded);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(*retrieved, small_value);
}

TEST_F(Phase2Test, WiscKeyLargeValuesSeparated) {
    auto log_path = test_dir_ / "value.log";
    WiscKeyStorage storage(log_path.string());
    
    std::string key = "test_key";
    std::string large_value(2000, 'x');  // > 1KB
    
    std::string encoded = storage.put(key, large_value);
    
    // Large value should be separated (stored in value log)
    EXPECT_TRUE(WiscKeyStorage::is_separated(encoded));
    EXPECT_EQ(encoded.size(), 12u);  // Value address is 12 bytes
    
    auto retrieved = storage.get(key, encoded);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(*retrieved, large_value);
}

TEST_F(Phase2Test, WiscKeyStatistics) {
    auto log_path = test_dir_ / "value.log";
    WiscKeyStorage storage(log_path.string());
    
    storage.put("k1", "small1");  // Inline
    storage.put("k2", std::string(2000, 'x'));  // Separated
    storage.put("k3", "small2");  // Inline
    storage.put("k4", std::string(3000, 'y'));  // Separated
    
    auto stats = storage.get_stats();
    EXPECT_EQ(stats.inline_values, 2u);
    EXPECT_EQ(stats.separated_values, 2u);
    EXPECT_GT(stats.value_log_size, 0u);
}

// ==================== Dostoevsky Tests ====================

TEST(DostoevskeyTest, MergePolicySelection) {
    DostoevskeyLSM lsm(5);  // 5 levels
    WorkloadStats stats;
    
    // Read-heavy workload (>70% reads)
    for (int i = 0; i < 80; i++) stats.record_read();
    for (int i = 0; i < 20; i++) stats.record_write();
    
    MergePolicy policy = lsm.compute_optimal_policy(0, stats);
    EXPECT_EQ(policy, MergePolicy::LEVELING);  // Best for reads
}

TEST(DostoevskeyTest, WriteHeavyPolicy) {
    DostoevskeyLSM lsm(5);
    WorkloadStats stats;
    
    // Write-heavy workload (<30% reads)
    for (int i = 0; i < 20; i++) stats.record_read();
    for (int i = 0; i < 80; i++) stats.record_write();
    
    MergePolicy policy = lsm.compute_optimal_policy(0, stats);
    EXPECT_EQ(policy, MergePolicy::TIERING);  // Best for writes
}

TEST(DostoevskeyTest, MixedWorkloadPolicy) {
    DostoevskeyLSM lsm(5);
    WorkloadStats stats;
    
    // Mixed workload (30-70% reads)
    for (int i = 0; i < 50; i++) stats.record_read();
    for (int i = 0; i < 50; i++) stats.record_write();
    
    MergePolicy policy = lsm.compute_optimal_policy(0, stats);
    EXPECT_EQ(policy, MergePolicy::LAZY_LEVELING);  // Hybrid
}

TEST(DostoevskeyTest, CostEstimation) {
    DostoevskeyLSM lsm(5);
    
    auto leveling_cost = lsm.estimate_cost(0, MergePolicy::LEVELING);
    EXPECT_LT(leveling_cost.read_amplification, 2.0);  // Low read amplification
    
    auto tiering_cost = lsm.estimate_cost(0, MergePolicy::TIERING);
    EXPECT_LT(tiering_cost.write_amplification, 2.0);  // Low write amplification
    
    auto lazy_cost = lsm.estimate_cost(0, MergePolicy::LAZY_LEVELING);
    // Lazy leveling should be between the two extremes
    EXPECT_LT(lazy_cost.read_amplification, 5.0);
    EXPECT_LT(lazy_cost.write_amplification, 5.0);
}

// ==================== Cicada Tests ====================

TEST(CicadaTest, RecordLocking) {
    CicadaRecord record;
    
    EXPECT_FALSE(record.is_locked());
    EXPECT_EQ(record.get_version(), 0u);
    
    EXPECT_TRUE(record.try_lock());
    EXPECT_TRUE(record.is_locked());
    
    // Second lock attempt should fail
    EXPECT_FALSE(record.try_lock());
    
    record.unlock_and_increment_version();
    EXPECT_FALSE(record.is_locked());
    EXPECT_EQ(record.get_version(), 1u);
}

TEST(CicadaTest, SimpleTransaction) {
    CicadaRecord record;
    CicadaTransaction txn;
    
    // Read operation
    txn.record_read(&record, record.get_version());
    
    // Write operation
    txn.record_write(&record);
    
    // Commit should succeed
    EXPECT_TRUE(txn.commit());
    EXPECT_FALSE(txn.is_aborted());
}

TEST(CicadaTest, ConflictDetection) {
    CicadaRecord record;
    
    // First transaction
    CicadaTransaction txn1;
    txn1.record_read(&record, record.get_version());
    txn1.record_write(&record);
    
    // Acquire lock for txn1
    EXPECT_TRUE(record.try_lock());
    
    // Second transaction tries to write (should fail)
    CicadaTransaction txn2;
    txn2.record_write(&record);
    
    // txn2 commit should fail (record is locked)
    EXPECT_FALSE(txn2.commit());
    EXPECT_TRUE(txn2.is_aborted());
    
    // Release lock
    record.unlock_and_increment_version();
}

TEST(CicadaTest, ContentionManager) {
    ContentionManager mgr;
    
    // Record some commits and aborts
    for (int i = 0; i < 70; i++) mgr.record_commit();
    for (int i = 0; i < 30; i++) mgr.record_abort();
    
    // Abort rate should be 30%
    EXPECT_NEAR(mgr.get_abort_rate(), 0.3, 0.01);
    EXPECT_FALSE(mgr.should_backoff());  // <50% abort rate
    
    // High contention scenario
    mgr.reset_stats();
    for (int i = 0; i < 40; i++) mgr.record_commit();
    for (int i = 0; i < 60; i++) mgr.record_abort();
    
    EXPECT_NEAR(mgr.get_abort_rate(), 0.6, 0.01);
    EXPECT_TRUE(mgr.should_backoff());  // >50% abort rate
}

// ==================== Ligra Tests ====================

TEST(LigraTest, FrontierOperations) {
    Frontier f(100);
    
    f.add(10);
    f.add(20);
    f.add(30);
    
    EXPECT_TRUE(f.contains(10));
    EXPECT_TRUE(f.contains(20));
    EXPECT_FALSE(f.contains(50));
    EXPECT_EQ(f.size(), 3u);
}

TEST(LigraTest, SparseToDenseConversion) {
    Frontier f(100);
    
    // Add a few vertices (sparse)
    for (int i = 0; i < 5; i++) {
        f.add(i * 10);
    }
    
    EXPECT_FALSE(f.is_dense_mode());
    
    // Convert to dense
    f.switch_to_dense();
    EXPECT_TRUE(f.is_dense_mode());
    EXPECT_EQ(f.size(), 5u);
    
    // Verify vertices are still present
    EXPECT_TRUE(f.contains(0));
    EXPECT_TRUE(f.contains(40));
}

TEST(LigraTest, ParallelBFS) {
    // Create simple graph: 0 -> 1 -> 2 -> 3
    std::vector<std::vector<NodeID>> adj_list = {
        {1},      // 0 -> 1
        {2},      // 1 -> 2
        {3},      // 2 -> 3
        {}        // 3 -> (none)
    };
    
    LigraProcessor processor(4);
    auto distances = processor.parallel_bfs(0, adj_list);
    
    EXPECT_EQ(distances[0], 0);
    EXPECT_EQ(distances[1], 1);
    EXPECT_EQ(distances[2], 2);
    EXPECT_EQ(distances[3], 3);
}

// ==================== RaBitQ Tests ====================

TEST(RaBitQTest, VectorQuantization) {
    RaBitQVector vec(100);  // 100-dimensional vector
    
    // Set some values
    vec.set(0, 0);  // 00
    vec.set(1, 1);  // 01
    vec.set(2, 2);  // 10
    vec.set(3, 3);  // 11
    
    // Verify retrieval
    EXPECT_EQ(vec.get(0), 0u);
    EXPECT_EQ(vec.get(1), 1u);
    EXPECT_EQ(vec.get(2), 2u);
    EXPECT_EQ(vec.get(3), 3u);
    
    // Check compression (100 values * 2 bits = 200 bits = 25 bytes)
    EXPECT_EQ(vec.compressed_size(), 25u);
}

TEST(RaBitQTest, EncoderTraining) {
    RaBitQEncoder encoder(4);  // 4D vectors
    
    // Training data
    std::vector<std::vector<float>> training = {
        {1.0f, 2.0f, 3.0f, 4.0f},
        {2.0f, 3.0f, 4.0f, 5.0f},
        {3.0f, 4.0f, 5.0f, 6.0f}
    };
    
    encoder.train(training);
    
    // Encode a vector
    std::vector<float> vec = {1.5f, 2.5f, 3.5f, 4.5f};
    auto quantized = encoder.encode(vec);
    
    EXPECT_EQ(quantized.dimension(), 4u);
    
    // Decode should give approximate values
    auto decoded = encoder.decode(quantized);
    EXPECT_EQ(decoded.size(), 4u);
}

TEST(RaBitQTest, IndexOperations) {
    RaBitQIndex index(4);  // 4D vectors
    
    // Train with some data
    std::vector<std::vector<float>> training = {
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f}
    };
    index.train(training);
    
    // Add vectors
    index.add(1, {1.0f, 0.0f, 0.0f, 0.0f});
    index.add(2, {0.0f, 1.0f, 0.0f, 0.0f});
    index.add(3, {0.0f, 0.0f, 1.0f, 0.0f});
    
    EXPECT_EQ(index.size(), 3u);
    
    // Search for nearest neighbor
    std::vector<float> query = {0.9f, 0.1f, 0.0f, 0.0f};
    auto results = index.search(query, 2);
    
    EXPECT_GE(results.size(), 1u);
    EXPECT_LE(results.size(), 2u);
}

TEST(RaBitQTest, MemoryCompression) {
    RaBitQIndex index(128);  // 128D vectors
    
    std::vector<std::vector<float>> training(10, std::vector<float>(128, 1.0f));
    index.train(training);
    
    // Add 100 vectors
    for (int i = 0; i < 100; i++) {
        index.add(i, std::vector<float>(128, static_cast<float>(i)));
    }
    
    auto stats = index.get_memory_stats();
    
    // Should have significant compression (close to 16x)
    EXPECT_GT(stats.compression_ratio, 10.0);
    EXPECT_LT(stats.compression_ratio, 20.0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
