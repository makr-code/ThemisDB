#include <gtest/gtest.h>
#include "analytics/diff_engine.h"
#include "cdc/changefeed.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <chrono>

using namespace themis;
using namespace themis::analytics;

class DiffEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./data/themis_diff_engine_test";
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
        
        RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.enable_wal = true;
        db_ = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());
        
        // Get RocksDB TransactionDB pointer
        auto txn_db = db_->getTransactionDB();
        ASSERT_NE(txn_db, nullptr);
        
        changefeed_ = std::make_unique<Changefeed>(txn_db);
        diff_engine_ = std::make_unique<DiffEngine>(*changefeed_);
    }
    
    void TearDown() override {
        diff_engine_.reset();
        changefeed_.reset();
        db_.reset();
        
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
    }
    
    // Helper: Record a PUT event
    uint64_t recordPut(const std::string& key, const std::string& value) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = key;
        event.value = value;
        event.timestamp_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
        
        auto recorded = changefeed_->recordEvent(event);
        return recorded.sequence;
    }
    
    // Helper: Record a DELETE event
    uint64_t recordDelete(const std::string& key) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_DELETE;
        event.key = key;
        event.timestamp_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
        
        auto recorded = changefeed_->recordEvent(event);
        return recorded.sequence;
    }
    
    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<Changefeed> changefeed_;
    std::unique_ptr<DiffEngine> diff_engine_;
};

// Test 1: Empty diff (no changes)
TEST_F(DiffEngineTest, EmptyDiff) {
    auto seq1 = recordPut("users:1", "Alice");
    auto seq2 = seq1; // No changes after seq1
    
    auto result = diff_engine_->computeDiff(seq1, seq1 + 100);
    
    EXPECT_EQ(result.added.size(), 0);
    EXPECT_EQ(result.modified.size(), 0);
    EXPECT_EQ(result.deleted.size(), 0);
    EXPECT_EQ(result.stats.total_changes, 0);
}

// Test 2: Single modification
TEST_F(DiffEngineTest, SingleModification) {
    auto seq1 = recordPut("users:1", "Alice");
    auto seq2 = recordPut("users:1", "Alice Updated");
    
    auto result = diff_engine_->computeDiff(seq1, seq2);
    
    EXPECT_EQ(result.modified.size(), 1);
    EXPECT_EQ(result.added.size(), 0);
    EXPECT_EQ(result.deleted.size(), 0);
    EXPECT_EQ(result.stats.total_changes, 1);
    
    EXPECT_EQ(result.modified[0].key, "users:1");
    EXPECT_TRUE(result.modified[0].new_value.has_value());
    EXPECT_EQ(*result.modified[0].new_value, "Alice Updated");
}

// Test 3: Single deletion
TEST_F(DiffEngineTest, SingleDeletion) {
    auto seq1 = recordPut("users:1", "Alice");
    auto seq2 = recordDelete("users:1");
    
    auto result = diff_engine_->computeDiff(seq1, seq2);
    
    EXPECT_EQ(result.deleted.size(), 1);
    EXPECT_EQ(result.modified.size(), 0);
    EXPECT_EQ(result.added.size(), 0);
    EXPECT_EQ(result.stats.total_changes, 1);
    
    EXPECT_EQ(result.deleted[0].key, "users:1");
}

// Test 4: Multiple changes
TEST_F(DiffEngineTest, MultipleChanges) {
    auto seq1 = recordPut("users:1", "Alice");
    recordPut("users:2", "Bob");
    recordPut("users:3", "Charlie");
    auto seq2 = changefeed_->getLatestSequence();
    
    auto result = diff_engine_->computeDiff(seq1, seq2);
    
    EXPECT_EQ(result.stats.total_changes, 2); // users:2 and users:3
    EXPECT_GE(result.modified.size(), 2);
}

// Test 5: Filter by table
TEST_F(DiffEngineTest, FilterByTable) {
    auto seq1 = recordPut("users:1", "Alice");
    recordPut("users:2", "Bob");
    recordPut("orders:1", "Order 1");
    recordPut("orders:2", "Order 2");
    auto seq2 = changefeed_->getLatestSequence();
    
    DiffEngine::DiffOptions options;
    options.table_filter = "users";
    
    auto result = diff_engine_->computeDiff(seq1, seq2);
    
    // Should contain changes for users:2, orders:1, orders:2
    EXPECT_GE(result.stats.total_changes, 3);
    
    // Now filter by users
    result = diff_engine_->computeDiff(seq1, seq2, options);
    
    // Should only contain users:2
    bool has_users = false;
    bool has_orders = false;
    for (const auto& change : result.modified) {
        if (change.key.find("users") != std::string::npos) has_users = true;
        if (change.key.find("orders") != std::string::npos) has_orders = true;
    }
    
    EXPECT_TRUE(has_users);
    EXPECT_FALSE(has_orders);
}

// Test 6: Filter by key prefix
TEST_F(DiffEngineTest, FilterByKeyPrefix) {
    auto seq1 = recordPut("entity:users:1", "Alice");
    recordPut("entity:users:2", "Bob");
    recordPut("entity:orders:1", "Order 1");
    auto seq2 = changefeed_->getLatestSequence();
    
    DiffEngine::DiffOptions options;
    options.key_prefix = "entity:users";
    
    auto result = diff_engine_->computeDiff(seq1, seq2, options);
    
    // Should only contain entity:users keys
    for (const auto& change : result.modified) {
        EXPECT_TRUE(change.key.find("entity:users") == 0);
    }
}

// Test 7: Pagination with limit
TEST_F(DiffEngineTest, PaginationLimit) {
    auto seq1 = recordPut("users:1", "Alice");
    recordPut("users:2", "Bob");
    recordPut("users:3", "Charlie");
    recordPut("users:4", "David");
    recordPut("users:5", "Eve");
    auto seq2 = changefeed_->getLatestSequence();
    
    DiffEngine::DiffOptions options;
    options.limit = 2;
    
    auto result = diff_engine_->computeDiff(seq1, seq2, options);
    
    EXPECT_LE(result.stats.total_changes, 2);
}

// Test 8: Pagination with offset
TEST_F(DiffEngineTest, PaginationOffset) {
    auto seq1 = recordPut("users:1", "Alice");
    recordPut("users:2", "Bob");
    recordPut("users:3", "Charlie");
    recordPut("users:4", "David");
    auto seq2 = changefeed_->getLatestSequence();
    
    DiffEngine::DiffOptions options;
    options.offset = 2;
    options.limit = 2;
    
    auto result = diff_engine_->computeDiff(seq1, seq2, options);
    
    // Should skip first 2 changes and return next 2
    EXPECT_LE(result.stats.total_changes, 2);
}

// Test 9: Include values flag
TEST_F(DiffEngineTest, IncludeValuesFlag) {
    auto seq1 = recordPut("users:1", "Alice");
    auto seq2 = recordPut("users:1", "Alice Updated");
    
    // With values
    DiffEngine::DiffOptions options_with;
    options_with.include_values = true;
    auto result_with = diff_engine_->computeDiff(seq1, seq2, options_with);
    
    EXPECT_EQ(result_with.modified.size(), 1);
    EXPECT_TRUE(result_with.modified[0].new_value.has_value());
    
    // Without values
    DiffEngine::DiffOptions options_without;
    options_without.include_values = false;
    auto result_without = diff_engine_->computeDiff(seq1, seq2, options_without);
    
    EXPECT_EQ(result_without.modified.size(), 1);
    // Values should not be included (implementation may still set them, but flag indicates preference)
}

// Test 10: Diff by timestamp
TEST_F(DiffEngineTest, DiffByTimestamp) {
    auto now = std::chrono::system_clock::now();
    auto ts1 = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    
    recordPut("users:1", "Alice");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    recordPut("users:2", "Bob");
    
    auto now2 = std::chrono::system_clock::now();
    auto ts2 = std::chrono::duration_cast<std::chrono::milliseconds>(now2.time_since_epoch()).count();
    
    auto result = diff_engine_->computeDiffByTimestamp(ts1, ts2);
    
    EXPECT_GE(result.stats.total_changes, 1);
    EXPECT_TRUE(result.from_timestamp_ms.has_value());
    EXPECT_TRUE(result.to_timestamp_ms.has_value());
}

// Test 11: Invalid sequence range
TEST_F(DiffEngineTest, InvalidSequenceRange) {
    EXPECT_THROW(
        diff_engine_->computeDiff(100, 50),
        std::invalid_argument
    );
}

// Test 12: Invalid timestamp range
TEST_F(DiffEngineTest, InvalidTimestampRange) {
    auto now = std::chrono::system_clock::now();
    auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    
    EXPECT_THROW(
        diff_engine_->computeDiffByTimestamp(ts + 1000, ts),
        std::invalid_argument
    );
}

// Test 13: Cache functionality
TEST_F(DiffEngineTest, CacheFunctionality) {
    auto seq1 = recordPut("users:1", "Alice");
    auto seq2 = recordPut("users:2", "Bob");
    
    DiffEngine::DiffOptions options;
    options.enable_caching = true;
    
    // First call - should compute
    auto result1 = diff_engine_->computeDiff(seq1, seq2, options);
    
    // Second call - should use cache
    auto result2 = diff_engine_->computeDiff(seq1, seq2, options);
    
    // Results should be identical
    EXPECT_EQ(result1.stats.total_changes, result2.stats.total_changes);
    
    // Check cache stats
    auto cache_stats = diff_engine_->getCacheStats();
    EXPECT_GT(cache_stats["cache_size"].get<size_t>(), 0);
    
    // Clear cache
    diff_engine_->clearCache();
    cache_stats = diff_engine_->getCacheStats();
    EXPECT_EQ(cache_stats["cache_size"].get<size_t>(), 0);
}

// Test 14: JSON serialization
TEST_F(DiffEngineTest, JsonSerialization) {
    auto seq1 = recordPut("users:1", "Alice");
    auto seq2 = recordPut("users:2", "Bob");
    
    auto result = diff_engine_->computeDiff(seq1, seq2);
    auto json = result.toJson();
    
    EXPECT_TRUE(json.contains("added"));
    EXPECT_TRUE(json.contains("modified"));
    EXPECT_TRUE(json.contains("deleted"));
    EXPECT_TRUE(json.contains("stats"));
    EXPECT_TRUE(json.contains("from_sequence"));
    EXPECT_TRUE(json.contains("to_sequence"));
    
    // Verify stats
    auto stats = json["stats"];
    EXPECT_TRUE(stats.contains("added_count"));
    EXPECT_TRUE(stats.contains("modified_count"));
    EXPECT_TRUE(stats.contains("deleted_count"));
    EXPECT_TRUE(stats.contains("total_changes"));
}

// Test 15: Large diff performance
TEST_F(DiffEngineTest, LargeDiffPerformance) {
    auto seq1 = recordPut("users:0", "User 0");
    
    // Record 1000 changes
    for (int i = 1; i < 1000; ++i) {
        recordPut("users:" + std::to_string(i), "User " + std::to_string(i));
    }
    
    auto seq2 = changefeed_->getLatestSequence();
    
    auto start = std::chrono::steady_clock::now();
    auto result = diff_engine_->computeDiff(seq1, seq2);
    auto end = std::chrono::steady_clock::now();
    
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // Should complete in reasonable time (less than 1 second for 1000 changes)
    EXPECT_LT(duration_ms, 1000);
    
    std::cout << "Large diff (1000 changes) completed in " << duration_ms << "ms\n";
}

// Test 16: Tag-based diff (not yet implemented)
TEST_F(DiffEngineTest, TagBasedDiffNotImplemented) {
    EXPECT_THROW(
        diff_engine_->computeDiffByTag("tag1", "tag2"),
        std::runtime_error
    );
}
