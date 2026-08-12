// Test for WiscKey garbage collection implementation
#include <performance/wisckey.h>
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

using namespace themis::performance;
namespace fs = std::filesystem;

class WiscKeyGCTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = "/tmp/wisckey_gc_test";
        fs::create_directories(test_dir_);
        log_path_ = test_dir_ + "/value.log";
    }

    void TearDown() override {
        fs::remove_all(test_dir_);
    }

    std::string test_dir_;
    std::string log_path_;
};

TEST_F(WiscKeyGCTest, BasicCompaction) {
    // Create a value log and append some values
    ValueLog log(log_path_);
    
    std::string value1 = "value1_data";
    std::string value2 = "value2_data";
    std::string value3 = "value3_data";
    
    ValueAddress addr1 = log.append(value1);
    ValueAddress addr2 = log.append(value2);
    ValueAddress addr3 = log.append(value3);
    
    // Verify initial state
    EXPECT_EQ(addr1.offset, 0);
    EXPECT_EQ(addr1.size, value1.size());
    EXPECT_EQ(addr2.offset, value1.size());
    EXPECT_EQ(addr2.size, value2.size());
    
    uint64_t initial_size = log.size();
    EXPECT_GT(initial_size, 0);
    
    // Compact with only addr1 and addr3 as live (addr2 is garbage)
    std::vector<ValueAddress> live_addresses = {addr1, addr3};
    log.compact(live_addresses);
    
    // After compaction, log should be smaller
    uint64_t compacted_size = log.size();
    EXPECT_LT(compacted_size, initial_size);
    EXPECT_EQ(compacted_size, value1.size() + value3.size());
    
    // Addresses should be updated in-place
    EXPECT_EQ(live_addresses[0].offset, 0);
    EXPECT_EQ(live_addresses[0].size, value1.size());
    EXPECT_EQ(live_addresses[1].offset, value1.size());
    EXPECT_EQ(live_addresses[1].size, value3.size());
}

TEST_F(WiscKeyGCTest, CompactionPreservesValues) {
    ValueLog log(log_path_);
    
    std::string value1 = "first_value";
    std::string value2 = "second_value";
    std::string value3 = "third_value";
    
    ValueAddress addr1 = log.append(value1);
    ValueAddress addr2 = log.append(value2);
    ValueAddress addr3 = log.append(value3);
    
    // Compact keeping only value1 and value3
    std::vector<ValueAddress> live_addresses = {addr1, addr3};
    log.compact(live_addresses);
    
    // After compaction, the addresses should be updated
    // value1 should now be at offset 0
    EXPECT_EQ(live_addresses[0].offset, 0);
    EXPECT_EQ(live_addresses[0].size, value1.size());
    
    auto read_value1 = log.read(live_addresses[0]);
    ASSERT_TRUE(read_value1.has_value());
    EXPECT_EQ(*read_value1, value1);
    
    // value3 should now be at offset value1.size()
    EXPECT_EQ(live_addresses[1].offset, value1.size());
    EXPECT_EQ(live_addresses[1].size, value3.size());
    
    auto read_value3 = log.read(live_addresses[1]);
    ASSERT_TRUE(read_value3.has_value());
    EXPECT_EQ(*read_value3, value3);
}

TEST_F(WiscKeyGCTest, EmptyCompaction) {
    ValueLog log(log_path_);
    
    std::string value = "some_value";
    log.append(value);
    
    uint64_t size_before = log.size();
    
    // Compact with empty live addresses (all garbage)
    std::vector<ValueAddress> live_addresses;
    log.compact(live_addresses);
    
    // Size should remain unchanged for empty compaction
    EXPECT_EQ(log.size(), size_before);
}

TEST_F(WiscKeyGCTest, CompactionWithLargeValues) {
    ValueLog log(log_path_);
    
    // Create large values (>1KB as per WiscKey threshold)
    std::string large_value1(2048, 'A');
    std::string large_value2(2048, 'B');
    std::string large_value3(2048, 'C');
    
    ValueAddress addr1 = log.append(large_value1);
    ValueAddress addr2 = log.append(large_value2);
    ValueAddress addr3 = log.append(large_value3);
    
    uint64_t initial_size = log.size();
    EXPECT_EQ(initial_size, 2048 * 3);
    
    // Keep only first and last values
    std::vector<ValueAddress> live_addresses = {addr1, addr3};
    log.compact(live_addresses);
    
    uint64_t compacted_size = log.size();
    EXPECT_EQ(compacted_size, 2048 * 2);
    
    // Verify addresses were updated
    EXPECT_EQ(live_addresses[0].offset, 0);
    EXPECT_EQ(live_addresses[0].size, large_value1.size());
    EXPECT_EQ(live_addresses[1].offset, large_value1.size());
    EXPECT_EQ(live_addresses[1].size, large_value3.size());
    
    // Verify values are still correct
    auto read1 = log.read(live_addresses[0]);
    ASSERT_TRUE(read1.has_value());
    EXPECT_EQ(*read1, large_value1);
    
    auto read3 = log.read(live_addresses[1]);
    ASSERT_TRUE(read3.has_value());
    EXPECT_EQ(*read3, large_value3);
}

TEST_F(WiscKeyGCTest, AppendAfterCompaction) {
    ValueLog log(log_path_);
    
    std::string value1 = "value1";
    std::string value2 = "value2";
    std::string value3 = "value3";
    
    ValueAddress addr1 = log.append(value1);
    log.append(value2);  // Will be garbage
    ValueAddress addr3 = log.append(value3);
    
    // Compact
    std::vector<ValueAddress> live_addresses = {addr1, addr3};
    log.compact(live_addresses);
    
    // Verify addresses were updated
    EXPECT_EQ(live_addresses[0].offset, 0);
    EXPECT_EQ(live_addresses[1].offset, value1.size());
    
    // Append a new value after compaction
    std::string value4 = "value4";
    ValueAddress addr4 = log.append(value4);
    
    // New value should be appended at the end
    EXPECT_EQ(addr4.offset, value1.size() + value3.size());
    EXPECT_EQ(addr4.size, value4.size());
    
    // Verify the new value is readable
    auto read4 = log.read(addr4);
    ASSERT_TRUE(read4.has_value());
    EXPECT_EQ(*read4, value4);
}

TEST_F(WiscKeyGCTest, WiscKeyStorageIntegration) {
    // Test that WiscKeyStorage works with compaction
    WiscKeyStorage storage(log_path_);
    
    // Store values
    std::string key1 = "key1";
    std::string value1 = std::string(1500, 'A');  // > threshold, will be separated
    std::string encoded1 = storage.put(key1, value1);
    
    std::string key2 = "key2";
    std::string value2 = std::string(500, 'B');  // < threshold, inline
    std::string encoded2 = storage.put(key2, value2);
    
    std::string key3 = "key3";
    std::string value3 = std::string(1500, 'C');  // > threshold, will be separated
    std::string encoded3 = storage.put(key3, value3);
    
    // Check that values are stored correctly
    EXPECT_TRUE(WiscKeyStorage::is_separated(encoded1));
    EXPECT_FALSE(WiscKeyStorage::is_separated(encoded2));
    EXPECT_TRUE(WiscKeyStorage::is_separated(encoded3));
    
    // Retrieve values
    auto retrieved1 = storage.get(key1, encoded1);
    ASSERT_TRUE(retrieved1.has_value());
    EXPECT_EQ(*retrieved1, value1);
    
    auto retrieved2 = storage.get(key2, encoded2);
    ASSERT_TRUE(retrieved2.has_value());
    EXPECT_EQ(*retrieved2, value2);
    
    auto retrieved3 = storage.get(key3, encoded3);
    ASSERT_TRUE(retrieved3.has_value());
    EXPECT_EQ(*retrieved3, value3);
    
    // Check stats
    auto stats = storage.get_stats();
    EXPECT_EQ(stats.separated_values, 2);
    EXPECT_EQ(stats.inline_values, 1);
    EXPECT_GT(stats.value_log_size, 0);
}
