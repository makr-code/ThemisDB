/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_advanced_vector_integration.cpp               ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:47:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     311                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include <filesystem>
#include "storage/rocksdb_wrapper.h"
#include "index/vector_index.h"
#include "index/advanced_vector_index.h"
#include "storage/base_entity.h"

class AdvancedVectorIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = "./data/themis_advanced_vector_test";
        std::filesystem::remove_all(db_path_);

        themis::RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        cfg.memtable_size_mb = 64;
        cfg.block_cache_size_mb = 128;
        storage_ = std::make_unique<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open());

        vector_index_ = std::make_unique<themis::VectorIndexManager>(*storage_);
    }

    void TearDown() override {
        vector_index_.reset();
        storage_->close();
        std::filesystem::remove_all(db_path_);
    }

    std::string db_path_;
    std::unique_ptr<themis::RocksDBWrapper> storage_;
    std::unique_ptr<themis::VectorIndexManager> vector_index_;
};

// Test AdvancedVectorIndex basic functionality
TEST_F(AdvancedVectorIntegrationTest, BasicCreation) {
    themis::AdvancedVectorIndex::Config config;
    config.nlist = 16;  // Small for testing
    config.nprobe = 4;
    config.use_pq = true;
    config.pq_m = 4;
    config.index_type = themis::AdvancedVectorIndex::Config::Type::IVF_PQ;
    config.use_gpu = false;  // CPU only for CI
    
    themis::AdvancedVectorIndex index(128, config);
    
    // Verify configuration
    EXPECT_EQ(index.getConfig().nlist, 16);
    EXPECT_EQ(index.getConfig().nprobe, 4);
    EXPECT_EQ(index.getConfig().pq_m, 4);
}

// Test AdvancedVectorIndex training
TEST_F(AdvancedVectorIntegrationTest, TrainingAndSearch) {
    themis::AdvancedVectorIndex::Config config;
    config.nlist = 16;
    config.nprobe = 4;
    config.use_pq = false;  // Use IVF_FLAT for simpler testing
    config.index_type = themis::AdvancedVectorIndex::Config::Type::IVF_FLAT;
    config.use_gpu = false;
    config.train_size = 500;  // Small training set
    
    themis::AdvancedVectorIndex index(128, config);
    
    // Generate training data
    std::vector<float> training_data(500 * 128);
    for (size_t i = 0; i < training_data.size(); ++i) {
        training_data[i] = static_cast<float>(i % 128) / 128.0f;
    }
    
    #ifdef THEMIS_GPU_ENABLED
    // Only test if FAISS is available
    bool trained = index.train(training_data.data(), 500);
    EXPECT_TRUE(trained);
    
    // Add vectors
    std::vector<float> vectors(100 * 128);
    std::vector<int64_t> ids(100);
    for (size_t i = 0; i < 100; ++i) {
        ids[i] = static_cast<int64_t>(i);
        for (size_t j = 0; j < 128; ++j) {
            vectors[i * 128 + j] = static_cast<float>(i + j) / 128.0f;
        }
    }
    
    bool added = index.addWithIds(vectors.data(), ids.data(), 100);
    EXPECT_TRUE(added);
    
    // Search
    std::vector<float> query(128);
    for (size_t i = 0; i < 128; ++i) {
        query[i] = static_cast<float>(i) / 128.0f;
    }
    
    auto result = index.search(query.data(), 5);
    EXPECT_EQ(result.ids.size(), 5);
    EXPECT_EQ(result.distances.size(), 5);
    
    // First result should be nearest
    EXPECT_GE(result.ids[0], 0);
    #else
    // FAISS not available - skip test
    GTEST_SKIP() << "FAISS not available in this build";
    #endif
}

// Test AdvancedVectorIndex statistics
TEST_F(AdvancedVectorIntegrationTest, Statistics) {
    themis::AdvancedVectorIndex::Config config;
    config.nlist = 16;
    config.use_pq = true;
    config.pq_m = 4;
    config.index_type = themis::AdvancedVectorIndex::Config::Type::IVF_PQ;
    config.use_gpu = false;
    
    themis::AdvancedVectorIndex index(128, config);
    
    auto stats = index.getStats();
    
    // Initial stats
    EXPECT_EQ(stats.total_vectors, 0);
    EXPECT_FALSE(stats.is_trained);
    
    #ifdef THEMIS_GPU_ENABLED
    // Train and add vectors
    std::vector<float> data(500 * 128, 0.5f);
    index.train(data.data(), 500);
    
    std::vector<int64_t> ids(100);
    for (size_t i = 0; i < 100; ++i) ids[i] = i;
    index.addWithIds(data.data(), ids.data(), 100);
    
    stats = index.getStats();
    EXPECT_EQ(stats.total_vectors, 100);
    EXPECT_TRUE(stats.is_trained);
    EXPECT_GT(stats.memory_usage_bytes, 0);
    #else
    GTEST_SKIP() << "FAISS not available in this build";
    #endif
}

// Test AdvancedVectorIndex persistence
TEST_F(AdvancedVectorIntegrationTest, SaveAndLoad) {
    themis::AdvancedVectorIndex::Config config;
    config.nlist = 16;
    config.use_pq = false;
    config.index_type = themis::AdvancedVectorIndex::Config::Type::IVF_FLAT;
    config.use_gpu = false;
    
    std::string save_path = "./data/themis_advanced_vector_test/index_save";
    std::filesystem::create_directories(save_path);
    
    #ifdef THEMIS_GPU_ENABLED
    // Create and train index
    themis::AdvancedVectorIndex index1(128, config);
    
    std::vector<float> data(500 * 128);
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<float>(i % 100) / 100.0f;
    }
    
    ASSERT_TRUE(index1.train(data.data(), 500));
    
    std::vector<int64_t> ids(100);
    for (size_t i = 0; i < 100; ++i) ids[i] = i;
    ASSERT_TRUE(index1.addWithIds(data.data(), ids.data(), 100));
    
    // Save
    ASSERT_TRUE(index1.save(save_path));
    
    // Load into new index
    themis::AdvancedVectorIndex index2(128, config);
    ASSERT_TRUE(index2.load(save_path));
    
    auto stats = index2.getStats();
    EXPECT_EQ(stats.total_vectors, 100);
    EXPECT_TRUE(stats.is_trained);
    
    // Test search works after load
    std::vector<float> query(128, 0.5f);
    auto result = index2.search(query.data(), 5);
    EXPECT_EQ(result.ids.size(), 5);
    #else
    GTEST_SKIP() << "FAISS not available in this build";
    #endif
    
    std::filesystem::remove_all(save_path);
}

// Test batch search
TEST_F(AdvancedVectorIntegrationTest, BatchSearch) {
    themis::AdvancedVectorIndex::Config config;
    config.nlist = 16;
    config.use_pq = false;
    config.index_type = themis::AdvancedVectorIndex::Config::Type::IVF_FLAT;
    config.use_gpu = false;
    
    #ifdef THEMIS_GPU_ENABLED
    themis::AdvancedVectorIndex index(128, config);
    
    // Train
    std::vector<float> data(500 * 128);
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<float>(i % 100) / 100.0f;
    }
    ASSERT_TRUE(index.train(data.data(), 500));
    
    // Add vectors
    std::vector<int64_t> ids(100);
    for (size_t i = 0; i < 100; ++i) ids[i] = i;
    ASSERT_TRUE(index.addWithIds(data.data(), ids.data(), 100));
    
    // Batch search with 3 queries
    std::vector<float> queries(3 * 128);
    for (size_t i = 0; i < queries.size(); ++i) {
        queries[i] = static_cast<float>((i / 128) + i % 128) / 128.0f;
    }
    
    auto results = index.searchBatch(queries.data(), 3, 5);
    
    EXPECT_EQ(results.size(), 3);
    for (const auto& result : results) {
        EXPECT_EQ(result.ids.size(), 5);
        EXPECT_EQ(result.distances.size(), 5);
    }
    #else
    GTEST_SKIP() << "FAISS not available in this build";
    #endif
}

// Test different index types
TEST_F(AdvancedVectorIntegrationTest, DifferentIndexTypes) {
    #ifdef THEMIS_GPU_ENABLED
    std::vector<themis::AdvancedVectorIndex::Config::Type> types = {
        themis::AdvancedVectorIndex::Config::Type::IVF_FLAT,
        themis::AdvancedVectorIndex::Config::Type::IVF_PQ,
        themis::AdvancedVectorIndex::Config::Type::HNSW_FLAT
    };
    
    for (auto type : types) {
        themis::AdvancedVectorIndex::Config config;
        config.nlist = 16;
        config.nprobe = 4;
        config.use_pq = (type == themis::AdvancedVectorIndex::Config::Type::IVF_PQ);
        config.pq_m = 4;
        config.index_type = type;
        config.use_gpu = false;
        
        themis::AdvancedVectorIndex index(128, config);
        
        // Train if needed
        std::vector<float> data(500 * 128, 0.5f);
        if (type != themis::AdvancedVectorIndex::Config::Type::HNSW_FLAT) {
            ASSERT_TRUE(index.train(data.data(), 500));
        }
        
        // Add vectors
        std::vector<int64_t> ids(50);
        for (size_t i = 0; i < 50; ++i) ids[i] = i;
        ASSERT_TRUE(index.add(data.data(), 50));
        
        auto stats = index.getStats();
        EXPECT_EQ(stats.total_vectors, 50);
    }
    #else
    GTEST_SKIP() << "FAISS not available in this build";
    #endif
}

// Test error handling
TEST_F(AdvancedVectorIntegrationTest, ErrorHandling) {
    themis::AdvancedVectorIndex::Config config;
    config.nlist = 16;
    config.use_pq = false;
    config.index_type = themis::AdvancedVectorIndex::Config::Type::IVF_FLAT;
    
    themis::AdvancedVectorIndex index(128, config);
    
    #ifndef THEMIS_GPU_ENABLED
    // Without FAISS, all operations should return false
    std::vector<float> data(100 * 128, 0.5f);
    EXPECT_FALSE(index.train(data.data(), 100));
    EXPECT_FALSE(index.add(data.data(), 100));
    
    auto result = index.search(data.data(), 5);
    EXPECT_TRUE(result.ids.empty());
    EXPECT_TRUE(result.distances.empty());
    #endif
}
