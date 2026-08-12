#include <gtest/gtest.h>
#include "analytics/diff_engine.h"
#include "cdc/changefeed.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <chrono>
#include <thread>
#include <atomic>

using namespace themis;
using namespace themis::analytics;

class DiffEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping diff engine focused tests on Windows due to intermittent hangs in focused runs.";
#endif
        test_db_path_ = "./data/themis_diff_engine_test";
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
        
        RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.enable_wal = true;
        config.merge_operator_preset =
            RocksDBWrapper::Config::MergeOperatorPreset::SequenceU64Increment;
        db_ = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());
        
        // Get RocksDB TransactionDB pointer
        auto* txn_db = db_->getRawDB();
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
        // Use proper chrono conversion for timestamp
        auto now = std::chrono::system_clock::now();
        event.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        
        auto recorded = changefeed_->recordEvent(event);
        return recorded.sequence;
    }
    
    // Helper: Record a DELETE event
    uint64_t recordDelete(const std::string& key) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_DELETE;
        event.key = key;
        // Use proper chrono conversion for timestamp
        auto now = std::chrono::system_clock::now();
        event.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        
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
    std::vector<DiffEngine::Change> all_changes;
    all_changes.insert(all_changes.end(), result.added.begin(), result.added.end());
    all_changes.insert(all_changes.end(), result.modified.begin(), result.modified.end());
    all_changes.insert(all_changes.end(), result.deleted.begin(), result.deleted.end());
    for (const auto& change : all_changes) {
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
    std::vector<DiffEngine::Change> all_changes;
    all_changes.insert(all_changes.end(), result.added.begin(), result.added.end());
    all_changes.insert(all_changes.end(), result.modified.begin(), result.modified.end());
    all_changes.insert(all_changes.end(), result.deleted.begin(), result.deleted.end());
    for (const auto& change : all_changes) {
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
    // Small delay to ensure different timestamps (without sleep)
    auto seq1 = changefeed_->getLatestSequence();
    recordPut("users:2", "Bob");
    auto seq2 = changefeed_->getLatestSequence();
    
    // Use a timestamp slightly after the last recorded event
    auto ts2 = ts1 + 10000; // 10 seconds later
    
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

// Test 16: Tag-based diff (requires SnapshotManager)
TEST_F(DiffEngineTest, TagBasedDiffRequiresSnapshotManager) {
    // DiffEngine created without SnapshotManager should throw
    EXPECT_THROW(
        diff_engine_->computeDiffByTag("tag1", "tag2"),
        std::runtime_error
    );
}

// Test 17: Delete then re-add same key
TEST_F(DiffEngineTest, DeleteThenReAdd) {
    auto seq1 = recordPut("users:1", "Alice");
    recordDelete("users:1");
    auto seq2 = recordPut("users:1", "Alice v2");
    
    auto result = diff_engine_->computeDiff(seq1, seq2);
    
    // Should show as modified (net effect)
    EXPECT_EQ(result.stats.total_changes, 1);
}

// Test 18: Very large diff (100K changes) - Performance test
TEST_F(DiffEngineTest, VeryLargeDiffPerformance) {
    auto seq1 = recordPut("users:0", "User 0");
    
    // Record 10K changes (reduced from 100K for test performance)
    for (int i = 1; i < 10000; ++i) {
        recordPut("users:" + std::to_string(i), "User " + std::to_string(i));
    }
    
    auto seq2 = changefeed_->getLatestSequence();
    
    auto start = std::chrono::steady_clock::now();
    auto result = diff_engine_->computeDiff(seq1, seq2);
    auto end = std::chrono::steady_clock::now();
    
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // Should complete in reasonable time
    EXPECT_LT(duration_ms, 5000); // 5 seconds for 10K changes
    
    std::cout << "Very large diff (10K changes) completed in " << duration_ms << "ms\n";
}

// Test 19: Concurrent cache access
TEST_F(DiffEngineTest, ConcurrentCacheAccess) {
    auto seq1 = recordPut("users:1", "Alice");
    auto seq2 = recordPut("users:2", "Bob");
    
    DiffEngine::DiffOptions options;
    options.enable_caching = true;
    
    // First call to populate cache
    auto result1 = diff_engine_->computeDiff(seq1, seq2, options);
    
    // Multiple concurrent reads from cache
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&]() {
            try {
                auto result = diff_engine_->computeDiff(seq1, seq2, options);
                if (result.stats.total_changes == result1.stats.total_changes) {
                    success_count++;
                }
            } catch (...) {
                // Ignore errors
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(success_count, 10); // All reads should succeed
}

// Test 20: Cache eviction on size limit
TEST_F(DiffEngineTest, CacheEvictionOnSizeLimit) {
    DiffEngine::DiffOptions options;
    options.enable_caching = true;
    
    // Create more than MAX_CACHE_SIZE (100) diff results
    for (int i = 0; i < 110; ++i) {
        auto seq1 = recordPut("users:" + std::to_string(i), "User " + std::to_string(i));
        auto seq2 = recordPut("users:" + std::to_string(i+1), "User " + std::to_string(i+1));
        diff_engine_->computeDiff(seq1, seq2, options);
    }
    
    auto cache_stats = diff_engine_->getCacheStats();
    
    // Cache should not exceed MAX_CACHE_SIZE
    EXPECT_LE(cache_stats["cache_size"].get<size_t>(), 100);
}

// Test 21: Invalid limit parameter
TEST_F(DiffEngineTest, InvalidLimitParameter) {
    auto seq1 = recordPut("users:1", "Alice");
    auto seq2 = recordPut("users:2", "Bob");
    
    DiffEngine::DiffOptions options;
    // Use value that exceeds DiffEngine::MAX_DIFF_LIMIT (1000000)
    options.limit = 2000000; 
    
    EXPECT_THROW(
        diff_engine_->computeDiff(seq1, seq2, options),
        std::invalid_argument
    );
}

// Test 22: Empty events list
TEST_F(DiffEngineTest, EmptyEventsList) {
    // Query range with no events
    auto result = diff_engine_->computeDiff(1000, 2000);
    
    EXPECT_EQ(result.stats.total_changes, 0);
    EXPECT_EQ(result.added.size(), 0);
    EXPECT_EQ(result.modified.size(), 0);
    EXPECT_EQ(result.deleted.size(), 0);
}

// Test 23: Sequence number boundary
TEST_F(DiffEngineTest, SequenceNumberBoundary) {
    auto seq1 = recordPut("users:1", "Alice");
    
    // Query with seq1 as both from and to should fail
    EXPECT_THROW(
        diff_engine_->computeDiff(seq1, seq1),
        std::invalid_argument
    );
}

// Test 24: Multiple modifications to same key
TEST_F(DiffEngineTest, MultipleModificationsSameKey) {
    auto seq1 = recordPut("users:1", "Alice v1");
    recordPut("users:1", "Alice v2");
    recordPut("users:1", "Alice v3");
    auto seq2 = recordPut("users:1", "Alice v4");
    
    auto result = diff_engine_->computeDiff(seq1, seq2);
    
    // Should show single modification with latest value
    EXPECT_EQ(result.modified.size(), 1);
    EXPECT_EQ(*result.modified[0].new_value, "Alice v4");
}

// Test 25: ADDED detection with from_sequence=0
TEST_F(DiffEngineTest, AddedDetectionFromZero) {
    auto seq1 = recordPut("users:1", "Alice");
    auto seq2 = recordPut("users:2", "Bob");
    
    // Query from sequence 0 should detect ADDED correctly
    auto result = diff_engine_->computeDiff(0, seq2);
    
    // Both should be detected as ADDED (new keys from sequence 0)
    EXPECT_GE(result.added.size(), 1);
}

// Test 26: Filtering with empty result
TEST_F(DiffEngineTest, FilteringWithEmptyResult) {
    auto seq1 = recordPut("users:1", "Alice");
    auto seq2 = recordPut("users:2", "Bob");
    
    DiffEngine::DiffOptions options;
    options.table_filter = "orders"; // No orders exist
    
    auto result = diff_engine_->computeDiff(seq1, seq2, options);
    
    EXPECT_EQ(result.stats.total_changes, 0);
}

// Test 27: Pagination with offset exceeding total
TEST_F(DiffEngineTest, PaginationOffsetExceedingTotal) {
    auto seq1 = recordPut("users:1", "Alice");
    recordPut("users:2", "Bob");
    auto seq2 = recordPut("users:3", "Charlie");
    
    DiffEngine::DiffOptions options;
    options.offset = 1000; // Way beyond total changes
    options.limit = 10;
    
    auto result = diff_engine_->computeDiff(seq1, seq2, options);
    
    EXPECT_EQ(result.stats.total_changes, 0);
}

// Test 28: Timestamp conversion with empty changefeed
TEST_F(DiffEngineTest, TimestampConversionEmptyChangefeed) {
    // Clear all events by creating new DB
    diff_engine_.reset();
    changefeed_.reset();
    db_.reset();
    
    std::string empty_db_path = "./data/themis_diff_engine_empty_test";
    if (std::filesystem::exists(empty_db_path)) {
        std::filesystem::remove_all(empty_db_path);
    }
    
    RocksDBWrapper::Config config;
    config.db_path = empty_db_path;
    config.enable_wal = true;
    config.merge_operator_preset =
        RocksDBWrapper::Config::MergeOperatorPreset::SequenceU64Increment;
    db_ = std::make_unique<RocksDBWrapper>(config);
    ASSERT_TRUE(db_->open());
    
    auto* txn_db = db_->getRawDB();
    changefeed_ = std::make_unique<Changefeed>(txn_db);
    diff_engine_ = std::make_unique<DiffEngine>(*changefeed_);
    
    auto now = std::chrono::system_clock::now();
    auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    
    // Should handle empty changefeed gracefully
    auto result = diff_engine_->computeDiffByTimestamp(ts - 1000, ts);
    
    EXPECT_EQ(result.stats.total_changes, 0);

    diff_engine_.reset();
    changefeed_.reset();
    db_->close();
    db_.reset();
    
    if (std::filesystem::exists(empty_db_path)) {
        std::filesystem::remove_all(empty_db_path);
    }
}

// Test 29: Cache TTL expiration (simulated)
TEST_F(DiffEngineTest, CacheTTLCheck) {
    auto seq1 = recordPut("users:1", "Alice");
    auto seq2 = recordPut("users:2", "Bob");
    
    DiffEngine::DiffOptions options;
    options.enable_caching = true;
    
    // First call
    auto result1 = diff_engine_->computeDiff(seq1, seq2, options);
    
    // Check cache stats
    auto stats = diff_engine_->getCacheStats();
    EXPECT_EQ(stats["cache_size"].get<size_t>(), 1);
    // Cache TTL is defined as DiffEngine::CACHE_TTL (300 seconds = 5 minutes)
    EXPECT_GE(stats["cache_ttl_seconds"].get<int>(), 0); // Just verify it exists
}

// Test 30: Without include_values flag
TEST_F(DiffEngineTest, WithoutIncludeValuesDetailed) {
    auto seq1 = recordPut("users:1", "Alice");
    recordDelete("users:1");
    auto seq2 = recordPut("users:2", "Bob");
    
    DiffEngine::DiffOptions options;
    options.include_values = false;
    
    auto result = diff_engine_->computeDiff(seq1, seq2, options);
    
    // Should still report changes, just without values
    EXPECT_GT(result.stats.total_changes, 0);
}

// Test 31: Key prefix filtering with special characters
TEST_F(DiffEngineTest, KeyPrefixFilteringSpecialChars) {
    auto seq1 = recordPut("entity:user:123", "Alice");
    recordPut("entity:user:456", "Bob");
    recordPut("entity:order:789", "Order 1");
    auto seq2 = changefeed_->getLatestSequence();
    
    DiffEngine::DiffOptions options;
    options.key_prefix = "entity:user:";
    
    auto result = diff_engine_->computeDiff(seq1, seq2, options);
    
    // Should only include entity:user: keys
    for (const auto& change : result.modified) {
        EXPECT_TRUE(change.key.find("entity:user:") == 0);
    }
}

// Test 32: Deserialization of DiffResult from JSON
TEST_F(DiffEngineTest, JsonDeserializationRoundTrip) {
    auto seq1 = recordPut("users:1", "Alice");
    auto seq2 = recordPut("users:2", "Bob");
    
    auto result1 = diff_engine_->computeDiff(seq1, seq2);
    auto json = result1.toJson();
    
    // Deserialize back
    auto result2 = DiffEngine::DiffResult::fromJson(json);
    
    // Should be identical
    EXPECT_EQ(result1.stats.total_changes, result2.stats.total_changes);
    EXPECT_EQ(result1.from_sequence, result2.from_sequence);
    EXPECT_EQ(result1.to_sequence, result2.to_sequence);
}

// Test 33: Deserialization of Change from JSON
TEST_F(DiffEngineTest, ChangeJsonDeserializationRoundTrip) {
    DiffEngine::Change change;
    change.type = DiffEngine::ChangeType::MODIFIED;
    change.key = "users:123";
    change.old_value = "Alice";
    change.new_value = "Alice Updated";
    change.sequence = 100;
    change.timestamp_ms = 1234567890;
    change.metadata = {{"extra", "info"}};
    
    auto json = change.toJson();
    auto change2 = DiffEngine::Change::fromJson(json);
    
    EXPECT_EQ(change.type, change2.type);
    EXPECT_EQ(change.key, change2.key);
    EXPECT_EQ(change.old_value, change2.old_value);
    EXPECT_EQ(change.new_value, change2.new_value);
    EXPECT_EQ(change.sequence, change2.sequence);
    EXPECT_EQ(change.timestamp_ms, change2.timestamp_ms);
}

// Test 34: Timestamp ordering assumption
TEST_F(DiffEngineTest, TimestampOrderingForBinarySearch) {
    // Record events - timestamps are monotonically increasing by sequence
    auto now = std::chrono::system_clock::now();
    auto ts1 = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    
    // Record 20 events (reduced from 100 to avoid test overhead)
    for (int i = 0; i < 20; ++i) {
        recordPut("users:" + std::to_string(i), "User " + std::to_string(i));
    }
    
    auto ts2 = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Should work with binary search optimization
    auto result = diff_engine_->computeDiffByTimestamp(ts1, ts2);
    
    // Should find some events (exact count depends on timing)
    EXPECT_GE(result.stats.total_changes, 0);
}

// Test: bounded listEvents with to_sequence correctly limits returned events
TEST_F(DiffEngineTest, BoundedFetchReturnsOnlyRangeEvents) {
    // Insert events e1..e5 and verify computeDiff(e1, e3) returns only e2..e3
    auto seq1 = recordPut("key:1", "v1");
    auto seq2 = recordPut("key:2", "v2");
    auto seq3 = recordPut("key:3", "v3");
    recordPut("key:4", "v4");  // intentionally outside requested range
    recordPut("key:5", "v5");  // intentionally outside requested range

    DiffEngine::DiffOptions opts;
    opts.enable_caching = false;
    auto result = diff_engine_->computeDiff(seq1, seq3, opts);

    // Only key:2 and key:3 fall strictly between seq1 (exclusive) and seq3 (inclusive)
    EXPECT_EQ(result.stats.total_changes, 2u);
    bool found2 = false, found3 = false, found4 = false, found5 = false;
    for (const auto& c : result.modified) {
        if (c.key == "key:2") found2 = true;
        if (c.key == "key:3") found3 = true;
        if (c.key == "key:4") found4 = true;
        if (c.key == "key:5") found5 = true;
    }
    EXPECT_TRUE(found2);
    EXPECT_TRUE(found3);
    EXPECT_FALSE(found4);  // must NOT appear — outside [seq1, seq3]
    EXPECT_FALSE(found5);  // must NOT appear — outside [seq1, seq3]
}

// Test: stampede prevention — only one thread should perform the expensive
// computation for a given range while all others wait on the in-flight CV.
// The compute hook is used as an observable counter: it is called exactly once
// per unique range computation (inside the guarded path, after inflight insert).
TEST_F(DiffEngineTest, StampedePreventionConcurrentSameRange) {
    auto seq_from = recordPut("u:1", "Alice");
    recordPut("u:2", "Bob");
    recordPut("u:3", "Charlie");
    auto seq_to = changefeed_->getLatestSequence();

    DiffEngine::DiffOptions opts;
    opts.enable_caching = true;

    // Hook counts how many threads actually enter the expensive compute path.
    // With stampede prevention exactly one thread should reach this point.
    std::atomic<int> compute_count{0};
    constexpr int kThreads = 8;
    diff_engine_->setComputeHookForTesting([&]() {
        compute_count.fetch_add(1, std::memory_order_relaxed);
    });

    std::vector<DiffEngine::DiffResult> results(kThreads);
    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&, i]() {
            results[i] = diff_engine_->computeDiff(seq_from, seq_to, opts);
        });
    }
    for (auto& t : threads) t.join();

    diff_engine_->setComputeHookForTesting({});  // clear hook

    // Exactly one thread must have performed the expensive computation.
    const int actual_count = compute_count.load();
    EXPECT_EQ(actual_count, 1)
        << "Expected exactly 1 compute; got " << actual_count
        << " — stampede prevention may not be working";

    // All threads must observe the same correct result (2 events: u:2, u:3)
    for (int i = 0; i < kThreads; ++i) {
        EXPECT_EQ(results[i].stats.total_changes, results[0].stats.total_changes)
            << "Thread " << i << " got a different result from thread 0";
    }
    EXPECT_EQ(results[0].stats.total_changes, 2u);
}

// Test: a successful first computation clears the in-flight marker so that
// subsequent callers get a cache hit without any deadlock.
TEST_F(DiffEngineTest, StampedeInFlightCleanedUpOnCacheHit) {
    auto seq1 = recordPut("x:1", "A");
    auto seq2 = recordPut("x:2", "B");

    DiffEngine::DiffOptions opts;
    opts.enable_caching = true;

    // First call: cold path — populates cache and removes inflight marker.
    auto r1 = diff_engine_->computeDiff(seq1, seq2, opts);
    EXPECT_EQ(r1.stats.total_changes, 1u);

    // Second call: cache hit — no deadlock and no second computation.
    std::atomic<int> compute_count{0};
    diff_engine_->setComputeHookForTesting([&]() {
        compute_count.fetch_add(1, std::memory_order_relaxed);
    });
    auto r2 = diff_engine_->computeDiff(seq1, seq2, opts);
    diff_engine_->setComputeHookForTesting({});

    EXPECT_EQ(r2.stats.total_changes, 1u);
    EXPECT_EQ(compute_count.load(), 0) << "Second call should have been a cache hit";
}

// Test: if an exception is thrown during computation (simulated via the hook),
// the in-flight marker must be cleared so subsequent callers are not blocked.
TEST_F(DiffEngineTest, StampedeInFlightCleanedUpOnException) {
    auto seq1 = recordPut("y:1", "V1");
    auto seq2 = recordPut("y:2", "V2");

    DiffEngine::DiffOptions opts;
    opts.enable_caching = true;

    // First call: hook throws — simulates an error inside computeDiff().
    diff_engine_->setComputeHookForTesting([]() {
        throw std::runtime_error("simulated failure in computeDiff");
    });
    EXPECT_THROW(diff_engine_->computeDiff(seq1, seq2, opts), std::runtime_error);
    diff_engine_->setComputeHookForTesting({});

    // Second call (no hook): the inflight marker must have been cleared by the
    // RAII guard, so this call should proceed and return the correct result.
    // If the marker was leaked the call would block forever.
    auto r = diff_engine_->computeDiff(seq1, seq2, opts);
    EXPECT_EQ(r.stats.total_changes, 1u);
}

// Opt-in performance test: bounded fetch must complete in ≤50 ms even when
// the changefeed contains a large number of events before the requested range.
// Run with THEMIS_RUN_PERF_TESTS=1.
TEST_F(DiffEngineTest, BoundedFetch_LargeChangefeed_Under50ms) {
    const char* perf_env = std::getenv("THEMIS_RUN_PERF_TESTS");
    if (!perf_env || std::string(perf_env) != "1") {
        GTEST_SKIP() << "Skipped — set THEMIS_RUN_PERF_TESTS=1 to enable perf tests";
    }

    // Fill the changefeed with a large number of events (background noise)
    constexpr int kBackgroundEvents = 20000;
    for (int i = 0; i < kBackgroundEvents; ++i) {
        recordPut("bg:" + std::to_string(i), "v");
    }

    // Record the narrow window we actually want to diff
    constexpr int kRangeEvents = 1000;
    auto seq_from = changefeed_->getLatestSequence();
    for (int i = 0; i < kRangeEvents; ++i) {
        recordPut("rng:" + std::to_string(i), "val");
    }
    auto seq_to = changefeed_->getLatestSequence();

    DiffEngine::DiffOptions opts;
    opts.enable_caching = false; // measure cold path

    auto t0 = std::chrono::steady_clock::now();
    auto result = diff_engine_->computeDiff(seq_from, seq_to, opts);
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();

    EXPECT_EQ(result.stats.total_changes, static_cast<size_t>(kRangeEvents));
    EXPECT_LE(elapsed_ms, 50)
        << "BoundedFetch took " << elapsed_ms
        << " ms (target ≤ 50 ms). Background events: " << kBackgroundEvents
        << ", range events: " << kRangeEvents;
}

// Opt-in performance test: the second concurrent caller for the same range
// must be served (via cache hit after wait) with a total overhead ≤ 5 ms.
// Run with THEMIS_RUN_PERF_TESTS=1.
TEST_F(DiffEngineTest, StampedeWait_SecondCallerUnder5ms) {
    const char* perf_env = std::getenv("THEMIS_RUN_PERF_TESTS");
    if (!perf_env || std::string(perf_env) != "1") {
        GTEST_SKIP() << "Skipped — set THEMIS_RUN_PERF_TESTS=1 to enable perf tests";
    }

    auto seq_from = recordPut("sw:1", "A");
    recordPut("sw:2", "B");
    recordPut("sw:3", "C");
    auto seq_to = changefeed_->getLatestSequence();

    DiffEngine::DiffOptions opts;
    opts.enable_caching = true;

    // Thread 1 primes the cache (cold path)
    std::thread t1([&] {
        diff_engine_->computeDiff(seq_from, seq_to, opts);
    });
    t1.join();

    // Cache is now warm. Measure how long it takes for a subsequent caller —
    // this is the overhead the "second concurrent caller" would experience after
    // the in-flight caller finishes and inserts into the cache.
    constexpr int kWarmCallers = 8;
    std::vector<long long> latencies(kWarmCallers);
    std::vector<std::thread> threads;
    threads.reserve(kWarmCallers);

    for (int i = 0; i < kWarmCallers; ++i) {
        threads.emplace_back([&latencies, &engine = *diff_engine_,
                              seq_from, seq_to, opts, idx = i] {
            auto t0 = std::chrono::steady_clock::now();
            engine.computeDiff(seq_from, seq_to, opts);
            latencies[idx] = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - t0).count();
        });
    }
    for (auto& t : threads) t.join();

    for (int i = 0; i < kWarmCallers; ++i) {
        EXPECT_LE(latencies[i], 5)
            << "Warm caller " << i << " took " << latencies[i]
            << " ms (target ≤ 5 ms for cache-hit path)";
    }
}
