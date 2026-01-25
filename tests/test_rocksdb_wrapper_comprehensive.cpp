// Copyright (c) 2025 VCC ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0
//
// RocksDB Wrapper Comprehensive Tests
// Tests move operations and thread-safety guarantees

#include <gtest/gtest.h>
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;
using themis::RocksDBWrapper;

class RocksDBWrapperMoveTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./data/test_move_" + 
                       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        fs::create_directories(test_db_path_);
    }

    void TearDown() override {
        if (fs::exists(test_db_path_)) {
            std::error_code ec;
            fs::remove_all(test_db_path_, ec);
        }
    }

    std::string test_db_path_;
};

// Test 1: Basic move construction
TEST_F(RocksDBWrapperMoveTest, MoveConstruction) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    cfg.memtable_size_mb = 64;
    cfg.block_cache_size_mb = 128;
    
    RocksDBWrapper wrapper1(cfg);
    ASSERT_TRUE(wrapper1.open());
    EXPECT_TRUE(wrapper1.isOpen());
    
    // Put some data
    EXPECT_TRUE(wrapper1.put("key1", "value1"));
    
    // Move construct
    RocksDBWrapper wrapper2(std::move(wrapper1));
    
    // New wrapper should work
    EXPECT_TRUE(wrapper2.isOpen());
    std::string value;
    EXPECT_TRUE(wrapper2.get("key1", value));
    EXPECT_EQ(value, "value1");
    
    // Can still use wrapper2
    EXPECT_TRUE(wrapper2.put("key2", "value2"));
    EXPECT_TRUE(wrapper2.get("key2", value));
    EXPECT_EQ(value, "value2");
}

// Test 2: Move assignment operator
TEST_F(RocksDBWrapperMoveTest, MoveAssignment) {
    RocksDBWrapper::Config cfg1;
    cfg1.db_path = test_db_path_ + "_src";
    fs::create_directories(cfg1.db_path);
    
    RocksDBWrapper::Config cfg2;
    cfg2.db_path = test_db_path_ + "_dst";
    fs::create_directories(cfg2.db_path);
    
    RocksDBWrapper wrapper1(cfg1);
    ASSERT_TRUE(wrapper1.open());
    EXPECT_TRUE(wrapper1.put("key1", "value1"));
    
    RocksDBWrapper wrapper2(cfg2);
    ASSERT_TRUE(wrapper2.open());
    EXPECT_TRUE(wrapper2.put("key2", "value2"));
    
    // Move assign
    wrapper2 = std::move(wrapper1);
    
    // wrapper2 should now have wrapper1's data
    EXPECT_TRUE(wrapper2.isOpen());
    std::string value;
    EXPECT_TRUE(wrapper2.get("key1", value));
    EXPECT_EQ(value, "value1");
    
    // Can continue using wrapper2
    EXPECT_TRUE(wrapper2.put("key3", "value3"));
    EXPECT_TRUE(wrapper2.get("key3", value));
    EXPECT_EQ(value, "value3");
    
    // Cleanup
    std::error_code ec;
    fs::remove_all(cfg1.db_path, ec);
    fs::remove_all(cfg2.db_path, ec);
}

// Test 3: Self-assignment (should be a no-op)
TEST_F(RocksDBWrapperMoveTest, SelfAssignment) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    
    RocksDBWrapper wrapper(cfg);
    ASSERT_TRUE(wrapper.open());
    EXPECT_TRUE(wrapper.put("key1", "value1"));
    
    // Self-assign via reference
    RocksDBWrapper& ref = wrapper;
    wrapper = std::move(ref);
    
    // Should still work
    EXPECT_TRUE(wrapper.isOpen());
    std::string value;
    EXPECT_TRUE(wrapper.get("key1", value));
    EXPECT_EQ(value, "value1");
}

// Test 4: Move from closed database
TEST_F(RocksDBWrapperMoveTest, MoveFromClosed) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    
    RocksDBWrapper wrapper1(cfg);
    ASSERT_TRUE(wrapper1.open());
    EXPECT_TRUE(wrapper1.put("key1", "value1"));
    wrapper1.close();
    EXPECT_FALSE(wrapper1.isOpen());
    
    // Move construct from closed database
    RocksDBWrapper wrapper2(std::move(wrapper1));
    
    // Should not be open
    EXPECT_FALSE(wrapper2.isOpen());
    
    // But can open and use
    EXPECT_TRUE(wrapper2.open());
    std::string value;
    EXPECT_TRUE(wrapper2.get("key1", value));
    EXPECT_EQ(value, "value1");
}

// Test 5: Source object properly nullified after move
TEST_F(RocksDBWrapperMoveTest, SourceObjectNullified) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = test_db_path_;
    
    RocksDBWrapper wrapper1(cfg);
    ASSERT_TRUE(wrapper1.open());
    
    // Move construct
    RocksDBWrapper wrapper2(std::move(wrapper1));
    
    // Source should not be open (db_ was reset)
    EXPECT_FALSE(wrapper1.isOpen());
    
    // Closing source should be safe (no double-free)
    wrapper1.close();  // Should be a no-op
    
    // Destination should still work
    EXPECT_TRUE(wrapper2.isOpen());
    EXPECT_TRUE(wrapper2.put("key", "value"));
}
