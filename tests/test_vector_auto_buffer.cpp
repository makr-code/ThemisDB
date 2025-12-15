/**
 * @file test_vector_auto_buffer.cpp
 * @brief Unit tests for VectorAutoBuffer
 */

#include <gtest/gtest.h>
#include "index/vector_auto_buffer.h"
#include "index/vector_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include <filesystem>
#include <thread>
#include <atomic>

namespace fs = std::filesystem;
using namespace themis;

class VectorAutoBufferTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./data/test_vector_auto_buffer";
        fs::remove_all(test_db_path_);
        
        RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.memtable_size_mb = 64;
        config.block_cache_size_mb = 128;
        
        db_ = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());
        
        vector_index_ = std::make_unique<VectorIndexManager>(*db_);
        
        // Initialize vector index
        auto status = vector_index_->init("test_vectors", 128, VectorIndexManager::Metric::COSINE);
        ASSERT_TRUE(status.ok) << status.message;
    }
    
    void TearDown() override {
        vector_index_.reset();
        db_.reset();
        fs::remove_all(test_db_path_);
    }
    
    BaseEntity createVectorEntity(const std::string& pk, const std::vector<float>& embedding) {
        BaseEntity entity;
        entity.setPrimaryKey(pk);
        
        nlohmann::json data;
        data["embedding"] = embedding;
        data["text"] = "Sample document " + pk;
        entity.setData(data);
        
        return entity;
    }
    
    std::vector<float> createRandomVector(size_t dim) {
        std::vector<float> vec(dim);
        for (size_t i = 0; i < dim; i++) {
            vec[i] = static_cast<float>(rand()) / RAND_MAX;
        }
        return vec;
    }
    
    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<VectorIndexManager> vector_index_;
};

// ===== Basic Functionality Tests =====

TEST_F(VectorAutoBufferTest, AddVector_Success) {
    VectorAutoBufferConfig config;
    config.max_vectors_per_buffer = 1000;
    config.async_flush = false;
    
    VectorAutoBuffer buffer(vector_index_.get(), config);
    
    auto embedding = createRandomVector(128);
    auto entity = createVectorEntity("doc1", embedding);
    
    auto status = buffer.add(entity);
    ASSERT_TRUE(status.ok) << status.message;
    
    auto stats = buffer.getStats();
    EXPECT_EQ(1, stats.vectors_buffered);
    EXPECT_EQ(1, stats.current_buffer_size);
}

TEST_F(VectorAutoBufferTest, UpdateVector_Success) {
    VectorAutoBufferConfig config;
    config.max_vectors_per_buffer = 1000;
    config.async_flush = false;
    
    VectorAutoBuffer buffer(vector_index_.get(), config);
    
    auto embedding = createRandomVector(128);
    auto entity = createVectorEntity("doc1", embedding);
    
    auto status = buffer.update(entity);
    ASSERT_TRUE(status.ok) << status.message;
    
    auto stats = buffer.getStats();
    EXPECT_EQ(1, stats.vectors_buffered);
}

TEST_F(VectorAutoBufferTest, RemoveVector_Success) {
    VectorAutoBufferConfig config;
    config.max_vectors_per_buffer = 1000;
    config.async_flush = false;
    
    VectorAutoBuffer buffer(vector_index_.get(), config);
    
    auto status = buffer.remove("doc1");
    ASSERT_TRUE(status.ok) << status.message;
    
    auto stats = buffer.getStats();
    EXPECT_EQ(1, stats.vectors_buffered);
}

TEST_F(VectorAutoBufferTest, SizeThresholdFlush) {
    VectorAutoBufferConfig config;
    config.max_vectors_per_buffer = 10;  // Small threshold
    config.async_flush = false;
    
    VectorAutoBuffer buffer(vector_index_.get(), config);
    
    // Add 10 vectors - should trigger flush
    for (int i = 0; i < 10; i++) {
        auto embedding = createRandomVector(128);
        auto entity = createVectorEntity("doc" + std::to_string(i), embedding);
        buffer.add(entity);
    }
    
    auto stats = buffer.getStats();
    EXPECT_EQ(10, stats.vectors_buffered);
    EXPECT_EQ(10, stats.vectors_flushed);
    EXPECT_EQ(1, stats.flush_count);
    EXPECT_EQ(1, stats.size_triggered_flush);
    EXPECT_EQ(0, stats.current_buffer_size);
}

TEST_F(VectorAutoBufferTest, TimeThresholdFlush) {
    VectorAutoBufferConfig config;
    config.max_vectors_per_buffer = 1000;
    config.flush_interval = std::chrono::milliseconds(100);
    config.async_flush = true;
    
    VectorAutoBuffer buffer(vector_index_.get(), config);
    buffer.start();
    
    // Add some vectors
    for (int i = 0; i < 5; i++) {
        auto embedding = createRandomVector(128);
        auto entity = createVectorEntity("doc" + std::to_string(i), embedding);
        buffer.add(entity);
    }
    
    // Wait for time-based flush
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    auto stats = buffer.getStats();
    EXPECT_EQ(5, stats.vectors_buffered);
    EXPECT_EQ(5, stats.vectors_flushed);
    EXPECT_GT(stats.auto_flush_count, 0);
    
    buffer.stop();
}

TEST_F(VectorAutoBufferTest, MixedOperations_Success) {
    VectorAutoBufferConfig config;
    config.max_vectors_per_buffer = 100;
    config.async_flush = false;
    
    VectorAutoBuffer buffer(vector_index_.get(), config);
    
    // Add operations
    for (int i = 0; i < 10; i++) {
        auto embedding = createRandomVector(128);
        auto entity = createVectorEntity("doc" + std::to_string(i), embedding);
        buffer.add(entity);
    }
    
    // Update operations
    for (int i = 0; i < 5; i++) {
        auto embedding = createRandomVector(128);
        auto entity = createVectorEntity("doc" + std::to_string(i), embedding);
        buffer.update(entity);
    }
    
    // Remove operations
    for (int i = 5; i < 10; i++) {
        buffer.remove("doc" + std::to_string(i));
    }
    
    auto stats = buffer.getStats();
    EXPECT_EQ(20, stats.vectors_buffered);  // 10 adds + 5 updates + 5 removes
}

// ===== Thread Safety Tests =====

TEST_F(VectorAutoBufferTest, ConcurrentInserts_ThreadSafe) {
    VectorAutoBufferConfig config;
    config.max_vectors_per_buffer = 10000;
    config.async_flush = true;
    config.flush_interval = std::chrono::seconds(10);
    
    VectorAutoBuffer buffer(vector_index_.get(), config);
    buffer.start();
    
    const int num_threads = 4;
    const int vectors_per_thread = 50;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < vectors_per_thread; i++) {
                auto embedding = createRandomVector(128);
                auto entity = createVectorEntity(
                    "thread" + std::to_string(t) + "_doc" + std::to_string(i), 
                    embedding
                );
                auto status = buffer.add(entity);
                if (status.ok) {
                    success_count++;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    buffer.stop();
    
    EXPECT_EQ(num_threads * vectors_per_thread, success_count);
    auto stats = buffer.getStats();
    EXPECT_EQ(num_threads * vectors_per_thread, stats.vectors_buffered);
}

// ===== Error Handling Tests =====

TEST_F(VectorAutoBufferTest, EmptyPrimaryKey_Error) {
    VectorAutoBufferConfig config;
    VectorAutoBuffer buffer(vector_index_.get(), config);
    
    BaseEntity entity;
    entity.setPrimaryKey("");  // Empty PK
    
    auto status = buffer.add(entity);
    EXPECT_FALSE(status.ok);
}

TEST_F(VectorAutoBufferTest, RemoveEmptyPK_Error) {
    VectorAutoBufferConfig config;
    VectorAutoBuffer buffer(vector_index_.get(), config);
    
    auto status = buffer.remove("");
    EXPECT_FALSE(status.ok);
}

// ===== Manual Flush Tests =====

TEST_F(VectorAutoBufferTest, ManualFlush_Success) {
    VectorAutoBufferConfig config;
    config.max_vectors_per_buffer = 1000;
    config.async_flush = false;
    
    VectorAutoBuffer buffer(vector_index_.get(), config);
    
    // Add vectors
    for (int i = 0; i < 25; i++) {
        auto embedding = createRandomVector(128);
        auto entity = createVectorEntity("doc" + std::to_string(i), embedding);
        buffer.add(entity);
    }
    
    EXPECT_EQ(25, buffer.getStats().current_buffer_size);
    
    // Manual flush
    size_t flushed = buffer.flush();
    EXPECT_EQ(25, flushed);
    EXPECT_EQ(0, buffer.getStats().current_buffer_size);
    EXPECT_EQ(25, buffer.getStats().vectors_flushed);
}

// ===== Statistics Tests =====

TEST_F(VectorAutoBufferTest, Statistics_Accurate) {
    VectorAutoBufferConfig config;
    config.max_vectors_per_buffer = 20;
    config.async_flush = false;
    
    VectorAutoBuffer buffer(vector_index_.get(), config);
    
    // Add vectors and trigger flushes
    for (int batch = 0; batch < 3; batch++) {
        for (int i = 0; i < 20; i++) {
            auto embedding = createRandomVector(128);
            auto entity = createVectorEntity(
                "batch" + std::to_string(batch) + "_doc" + std::to_string(i), 
                embedding
            );
            buffer.add(entity);
        }
    }
    
    auto stats = buffer.getStats();
    EXPECT_EQ(60, stats.vectors_buffered);
    EXPECT_EQ(60, stats.vectors_flushed);
    EXPECT_EQ(3, stats.flush_count);
    EXPECT_EQ(3, stats.size_triggered_flush);
}

// ===== Shutdown Tests =====

TEST_F(VectorAutoBufferTest, StopFlushesRemainingVectors) {
    VectorAutoBufferConfig config;
    config.max_vectors_per_buffer = 1000;
    config.async_flush = true;
    config.flush_interval = std::chrono::hours(1);
    
    VectorAutoBuffer buffer(vector_index_.get(), config);
    buffer.start();
    
    // Add vectors
    for (int i = 0; i < 30; i++) {
        auto embedding = createRandomVector(128);
        auto entity = createVectorEntity("doc" + std::to_string(i), embedding);
        buffer.add(entity);
    }
    
    EXPECT_EQ(30, buffer.getStats().current_buffer_size);
    
    // Stop should flush
    buffer.stop();
    
    auto stats = buffer.getStats();
    EXPECT_EQ(30, stats.vectors_flushed);
    EXPECT_EQ(0, stats.current_buffer_size);
}
