/**
 * @file test_ts_auto_buffer.cpp
 * @brief Unit tests for TSAutoBuffer
 */

#include <gtest/gtest.h>
#include "timeseries/ts_auto_buffer.h"
#include "timeseries/tsstore.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <thread>
#include <chrono>
#include <atomic>

namespace fs = std::filesystem;
using namespace themis;

class TSAutoBufferTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./data/test_ts_auto_buffer";
        fs::remove_all(test_db_path_);
        
        RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.memtable_size_mb = 64;
        config.block_cache_size_mb = 128;
        
        db_ = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());
        
        TSStore::Config ts_config;
        ts_config.compression = TSStore::CompressionType::Gorilla;
        ts_store_ = std::make_unique<TSStore>(db_->getRawDB(), nullptr, ts_config);
    }
    
    void TearDown() override {
        ts_store_.reset();
        db_.reset();
        fs::remove_all(test_db_path_);
    }
    
    TSStore::DataPoint createPoint(const std::string& metric, const std::string& entity, 
                                   int64_t timestamp_ms, double value) {
        TSStore::DataPoint point;
        point.metric = metric;
        point.entity = entity;
        point.timestamp_ms = timestamp_ms;
        point.value = value;
        point.tags = nlohmann::json::object();
        point.metadata = nlohmann::json::object();
        return point;
    }
    
    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<TSStore> ts_store_;
};

// ===== Basic Functionality Tests =====

TEST_F(TSAutoBufferTest, AddPoint_Success) {
    TSAutoBufferConfig config;
    config.max_points_per_buffer = 1000;
    config.async_flush = false;  // Synchronous for testing
    
    TSAutoBuffer buffer(ts_store_.get(), config);
    
    auto point = createPoint("cpu", "server01", 1000, 75.5);
    auto status = buffer.add(point);
    
    ASSERT_TRUE(status.ok) << status.message;
    
    auto stats = buffer.getStats();
    EXPECT_EQ(1, stats.points_buffered);
    EXPECT_EQ(1, stats.current_buffer_size);
}

TEST_F(TSAutoBufferTest, SizeThresholdFlush) {
    TSAutoBufferConfig config;
    config.max_points_per_buffer = 10;  // Small threshold
    config.async_flush = false;
    
    TSAutoBuffer buffer(ts_store_.get(), config);
    
    // Add 10 points - should trigger flush
    for (int i = 0; i < 10; i++) {
        auto point = createPoint("cpu", "server01", 1000 + i, 50.0 + i);
        buffer.add(point);
    }
    
    auto stats = buffer.getStats();
    EXPECT_EQ(10, stats.points_buffered);
    EXPECT_EQ(10, stats.points_flushed);
    EXPECT_EQ(1, stats.flush_count);
    EXPECT_EQ(1, stats.size_triggered_flush);
    EXPECT_EQ(0, stats.current_buffer_size);  // Buffer should be empty after flush
}

TEST_F(TSAutoBufferTest, TimeThresholdFlush) {
    TSAutoBufferConfig config;
    config.max_points_per_buffer = 1000;
    config.flush_interval = std::chrono::milliseconds(100);  // 100ms
    config.async_flush = true;
    
    TSAutoBuffer buffer(ts_store_.get(), config);
    buffer.start();
    
    // Add some points
    for (int i = 0; i < 5; i++) {
        auto point = createPoint("cpu", "server01", 1000 + i, 50.0 + i);
        buffer.add(point);
    }
    
    // Wait for time-based flush
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    auto stats = buffer.getStats();
    EXPECT_EQ(5, stats.points_buffered);
    EXPECT_EQ(5, stats.points_flushed);
    EXPECT_GT(stats.auto_flush_count, 0);
    
    buffer.stop();
}

TEST_F(TSAutoBufferTest, MemoryThresholdFlush) {
    TSAutoBufferConfig config;
    config.max_points_per_buffer = 10000;
    config.max_memory_bytes = 1024;  // 1KB - very small for testing
    config.async_flush = false;
    
    TSAutoBuffer buffer(ts_store_.get(), config);
    
    // Add points until memory threshold is hit
    for (int i = 0; i < 100; i++) {
        auto point = createPoint("cpu", "server01", 1000 + i, 50.0 + i);
        buffer.add(point);
    }
    
    auto stats = buffer.getStats();
    EXPECT_GT(stats.flush_count, 0);
    EXPECT_GT(stats.buffer_overflow_count, 0);
}

TEST_F(TSAutoBufferTest, ManualFlush) {
    TSAutoBufferConfig config;
    config.max_points_per_buffer = 1000;
    config.async_flush = false;
    
    TSAutoBuffer buffer(ts_store_.get(), config);
    
    // Add some points
    for (int i = 0; i < 50; i++) {
        auto point = createPoint("cpu", "server01", 1000 + i, 50.0 + i);
        buffer.add(point);
    }
    
    EXPECT_EQ(50, buffer.getStats().current_buffer_size);
    
    // Manual flush
    size_t flushed = buffer.flush();
    EXPECT_EQ(50, flushed);
    EXPECT_EQ(0, buffer.getStats().current_buffer_size);
    EXPECT_EQ(50, buffer.getStats().points_flushed);
}

// ===== Thread Safety Tests =====

TEST_F(TSAutoBufferTest, ConcurrentInserts_ThreadSafe) {
    TSAutoBufferConfig config;
    config.max_points_per_buffer = 10000;
    config.async_flush = true;
    config.flush_interval = std::chrono::seconds(10);  // Long interval
    
    TSAutoBuffer buffer(ts_store_.get(), config);
    buffer.start();
    
    const int num_threads = 4;
    const int points_per_thread = 100;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < points_per_thread; i++) {
                auto point = createPoint("cpu", "server0" + std::to_string(t), 
                                        1000 + i, 50.0 + i);
                auto status = buffer.add(point);
                if (status.ok) {
                    success_count++;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    buffer.stop();  // This will flush remaining points
    
    EXPECT_EQ(num_threads * points_per_thread, success_count);
    auto stats = buffer.getStats();
    EXPECT_EQ(num_threads * points_per_thread, stats.points_buffered);
}

// ===== Error Handling Tests =====

TEST_F(TSAutoBufferTest, EmptyMetric_Error) {
    TSAutoBufferConfig config;
    TSAutoBuffer buffer(ts_store_.get(), config);
    
    TSStore::DataPoint point;
    point.metric = "";  // Empty metric
    point.entity = "server01";
    point.timestamp_ms = 1000;
    point.value = 50.0;
    
    auto status = buffer.add(point);
    EXPECT_FALSE(status.ok);
    EXPECT_NE(status.message.find("empty"), std::string::npos);
}

TEST_F(TSAutoBufferTest, EmptyEntity_Error) {
    TSAutoBufferConfig config;
    TSAutoBuffer buffer(ts_store_.get(), config);
    
    TSStore::DataPoint point;
    point.metric = "cpu";
    point.entity = "";  // Empty entity
    point.timestamp_ms = 1000;
    point.value = 50.0;
    
    auto status = buffer.add(point);
    EXPECT_FALSE(status.ok);
    EXPECT_NE(status.message.find("empty"), std::string::npos);
}

// ===== Configuration Tests =====

TEST_F(TSAutoBufferTest, UpdateConfig_Applied) {
    TSAutoBufferConfig config;
    config.max_points_per_buffer = 100;
    
    TSAutoBuffer buffer(ts_store_.get(), config);
    
    EXPECT_EQ(100, buffer.getConfig().max_points_per_buffer);
    
    // Update config
    TSAutoBufferConfig new_config;
    new_config.max_points_per_buffer = 500;
    buffer.setConfig(new_config);
    
    EXPECT_EQ(500, buffer.getConfig().max_points_per_buffer);
}

// ===== Statistics Tests =====

TEST_F(TSAutoBufferTest, Statistics_Accurate) {
    TSAutoBufferConfig config;
    config.max_points_per_buffer = 100;
    config.async_flush = false;
    
    TSAutoBuffer buffer(ts_store_.get(), config);
    
    // Add points and trigger flushes
    for (int batch = 0; batch < 3; batch++) {
        for (int i = 0; i < 100; i++) {
            auto point = createPoint("cpu", "server01", 1000 + batch * 100 + i, 50.0 + i);
            buffer.add(point);
        }
    }
    
    auto stats = buffer.getStats();
    EXPECT_EQ(300, stats.points_buffered);
    EXPECT_EQ(300, stats.points_flushed);
    EXPECT_EQ(3, stats.flush_count);
    EXPECT_EQ(3, stats.size_triggered_flush);
    EXPECT_EQ(0, stats.time_triggered_flush);
}

// ===== Shutdown Tests =====

TEST_F(TSAutoBufferTest, StopFlushesRemainingPoints) {
    TSAutoBufferConfig config;
    config.max_points_per_buffer = 1000;
    config.async_flush = true;
    config.flush_interval = std::chrono::hours(1);  // Never flush automatically
    
    TSAutoBuffer buffer(ts_store_.get(), config);
    buffer.start();
    
    // Add points
    for (int i = 0; i < 50; i++) {
        auto point = createPoint("cpu", "server01", 1000 + i, 50.0 + i);
        buffer.add(point);
    }
    
    EXPECT_EQ(50, buffer.getStats().current_buffer_size);
    
    // Stop should flush remaining points
    buffer.stop();
    
    auto stats = buffer.getStats();
    EXPECT_EQ(50, stats.points_flushed);
    EXPECT_EQ(0, stats.current_buffer_size);
}
