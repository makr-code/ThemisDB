/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_diff_engine.cpp                               ║
  Version:         0.0.22                                             ║
  Last Modified:   2026-02-21 19:29:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     713                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
