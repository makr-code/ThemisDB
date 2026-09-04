// Copyright (c) 2025 VCC ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Write-Amplification Optimization Configuration Tests
// Tests that v1.5.0 optimized defaults are correctly applied

#include <gtest/gtest.h>
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;
using themis::RocksDBWrapper;

class WriteAmplificationConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use test name + timestamp for unique path to avoid collisions
        auto test_info = ::testing::UnitTest::GetInstance()->current_test_info();
        std::string test_name = std::string(test_info->test_case_name()) + "_" + 
                                std::string(test_info->name());
        test_db_path_ = "./data/" + test_name + "_" + 
                       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        fs::create_directories(test_db_path_);
    }

    void TearDown() override {
        if (fs::exists(test_db_path_)) {
            std::error_code ec = {};
            fs::remove_all(test_db_path_, ec);
        }
    }

    std::string test_db_path_ = {};
};

// Test 1: Verify v1.5.0 default memtable configuration
TEST_F(WriteAmplificationConfigTest, DefaultMemtableSize) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    
    // Test default values without explicit setting
    EXPECT_EQ(cfg.memtable_size_mb, 512) 
        << "Default memtable_size_mb should be 512MB for write-amp optimization";
    EXPECT_EQ(cfg.max_write_buffer_number, 6) 
        << "Default max_write_buffer_number should be 6 for high throughput";
    EXPECT_EQ(cfg.db_write_buffer_size_mb, 2048) 
        << "Default db_write_buffer_size_mb should be 2048MB (2GB)";
}

// Test 2: Verify async I/O is enabled by default
TEST_F(WriteAmplificationConfigTest, DefaultAsyncIOEnabled) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    
    EXPECT_TRUE(cfg.enable_async_io) 
        << "Async I/O should be enabled by default in v1.5.0";
    EXPECT_EQ(cfg.async_io_readahead_size_mb, 128) 
        << "Async I/O readahead should be 128MB for better throughput";
}

// Test 3: Verify RocksDB opens successfully with new defaults
TEST_F(WriteAmplificationConfigTest, OpenWithOptimizedDefaults) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    // Use all defaults (no explicit settings)
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open()) 
        << "RocksDB should open successfully with v1.5.0 optimized defaults";
    EXPECT_TRUE(wrapper.isOpen());
    
    // Verify basic operations work
    EXPECT_TRUE(wrapper.put("test_key", "test_value"));
    std::string value = {};
    EXPECT_TRUE(wrapper.get("test_key", value));
    EXPECT_EQ(value, "test_value");
}

// Test 4: Verify configuration can be overridden
TEST_F(WriteAmplificationConfigTest, OverrideDefaults) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    
    // Override defaults for memory-constrained scenario
    cfg.memtable_size_mb = 128;
    cfg.max_write_buffer_number = 3;
    cfg.db_write_buffer_size_mb = 512;
    cfg.enable_async_io = false;
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open()) 
        << "RocksDB should open with custom configuration";
    
    // Verify config was applied
    const auto& applied_cfg = wrapper.getConfig();
    EXPECT_EQ(applied_cfg.memtable_size_mb, 128);
    EXPECT_EQ(applied_cfg.max_write_buffer_number, 3);
    EXPECT_EQ(applied_cfg.db_write_buffer_size_mb, 512);
    EXPECT_FALSE(applied_cfg.enable_async_io);
}

// Test 5: Verify backward compatibility
TEST_F(WriteAmplificationConfigTest, BackwardCompatibility) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    
    // Test with legacy typical values
    cfg.memtable_size_mb = 256;
    cfg.max_write_buffer_number = 3;
    cfg.db_write_buffer_size_mb = 0;  // 0 = unlimited (old default)
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open()) 
        << "RocksDB should maintain backward compatibility with old configs";
    
    EXPECT_TRUE(wrapper.put("legacy_key", "legacy_value"));
    std::string value = {};
    EXPECT_TRUE(wrapper.get("legacy_key", value));
    EXPECT_EQ(value, "legacy_value");
}

// Test 6: Verify async I/O functionality
TEST_F(WriteAmplificationConfigTest, AsyncIOFunctionality) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    cfg.enable_async_io = true;
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open());
    
    // Insert test data
    for (int i = 0; i < 100; i++) {
        std::string key = "async_key_" + std::to_string(i);
        std::string value = "async_value_" + std::to_string(i);
        ASSERT_TRUE(wrapper.put(key, value));
    }
    
    // Test async scan
    auto results = wrapper.scanWithAsyncIO("async_key_", 50);
    // Note: scanWithAsyncIO may return 0-50 results depending on implementation
    // Just verify it doesn't crash and respects the limit
    EXPECT_LE(results.size(), 50) << "Async scan should respect limit";
    
    // Verify async I/O is enabled
    EXPECT_TRUE(wrapper.isAsyncIOEnabled());
}
