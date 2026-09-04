#include <gtest/gtest.h>
#include "query/result_stream.h"
#include "utils/error_registry.h"
#include <vector>
#include <string>
#include <algorithm>
#include <limits>

using namespace themis;
using namespace themis::query;

// ============================================================================
// Test Fixtures
// ============================================================================

class ResultStreamTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test data
        test_keys_ = {"key1", "key2", "key3", "key4", "key5", 
                      "key6", "key7", "key8", "key9", "key10"};
        
        large_dataset_.reserve(1000);
        for (int i = 0; i < 1000; ++i) {
            large_dataset_.push_back("item_" + std::to_string(i));
        }
    }
    
    std::vector<std::string> test_keys_;
    std::vector<std::string> large_dataset_;
};

// ============================================================================
// Basic Streaming Tests
// ============================================================================

TEST_F(ResultStreamTest, CreateStreamFromVector) {
    auto stream = createKeyStream(test_keys_);
    
    ASSERT_TRUE(stream->hasNext());
    EXPECT_EQ(stream->position(), 0);
}

TEST_F(ResultStreamTest, IterateThroughAllItems) {
    auto stream = createKeyStream(test_keys_);
    
    std::vector<std::string> results = {};

    while (stream->hasNext()) {
        auto result = stream->next();
        ASSERT_TRUE(result.has_value()) << result.error().message();
        results.push_back(*result);
    }
    
    EXPECT_EQ(results.size(), test_keys_.size());
    EXPECT_EQ(results, test_keys_);
    EXPECT_FALSE(stream->hasNext());
}

TEST_F(ResultStreamTest, NextOnEmptyStreamReturnsError) {
    std::vector<std::string> empty_data;
    auto stream = createKeyStream(empty_data);
    
    EXPECT_FALSE(stream->hasNext());
    
    auto result = stream->next();
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED);
}

TEST_F(ResultStreamTest, NextAfterExhaustionReturnsError) {
    std::vector<std::string> single_item = {"item1"};
    auto stream = createKeyStream(single_item);
    
    // Consume the single item
    auto result1 = stream->next();
    ASSERT_TRUE(result1.has_value());
    
    // Try to get another item
    auto result2 = stream->next();
    EXPECT_FALSE(result2.has_value());
}

// ============================================================================
// Batch Operations Tests
// ============================================================================

TEST_F(ResultStreamTest, FetchBatch) {
    auto stream = createKeyStream(test_keys_);
    
    auto batch_result = stream->nextBatch(3);
    ASSERT_TRUE(batch_result.has_value()) << batch_result.error().message();
    
    auto& batch = *batch_result;
    EXPECT_EQ(batch.items.size(), 3);
    EXPECT_EQ(batch.items[0], "key1");
    EXPECT_EQ(batch.items[1], "key2");
    EXPECT_EQ(batch.items[2], "key3");
    EXPECT_FALSE(batch.is_last_batch);
}

TEST_F(ResultStreamTest, FetchMultipleBatches) {
    auto stream = createKeyStream(test_keys_);
    
    // First batch
    auto batch1 = stream->nextBatch(4);
    ASSERT_TRUE(batch1.has_value());
    EXPECT_EQ(batch1->items.size(), 4);
    
    // Second batch
    auto batch2 = stream->nextBatch(4);
    ASSERT_TRUE(batch2.has_value());
    EXPECT_EQ(batch2->items.size(), 4);
    
    // Third batch (partial)
    auto batch3 = stream->nextBatch(4);
    ASSERT_TRUE(batch3.has_value());
    EXPECT_EQ(batch3->items.size(), 2);
    EXPECT_TRUE(batch3->is_last_batch);
}

TEST_F(ResultStreamTest, BatchSizeLargerThanData) {
    auto stream = createKeyStream(test_keys_);
    
    auto batch_result = stream->nextBatch(100);
    ASSERT_TRUE(batch_result.has_value());
    
    EXPECT_EQ(batch_result->items.size(), test_keys_.size());
    EXPECT_TRUE(batch_result->is_last_batch);
}

TEST_F(ResultStreamTest, EmptyBatchOnExhaustedStream) {
    std::vector<std::string> single_item = {"item1"};
    auto stream = createKeyStream(single_item);
    
    // Consume all items
    auto batch1 = stream->nextBatch(10);
    ASSERT_TRUE(batch1.has_value());
    EXPECT_EQ(batch1->items.size(), 1);
    
    // Try to fetch another batch
    auto batch2 = stream->nextBatch(10);
    EXPECT_FALSE(batch2.has_value());
}

// ============================================================================
// Reset and Position Tests
// ============================================================================

TEST_F(ResultStreamTest, ResetStreamToBeginning) {
    auto stream = createKeyStream(test_keys_);
    
    // Consume some items
    for (int i = 0; i < 5; ++i) {
        stream->next();
    }
    EXPECT_EQ(stream->position(), 5);
    
    // Reset
    stream->reset();
    
    // Verify reset
    EXPECT_EQ(stream->position(), 0);
    EXPECT_TRUE(stream->hasNext());
    
    auto result = stream->next();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "key1");
}

TEST_F(ResultStreamTest, PositionTracking) {
    auto stream = createKeyStream(test_keys_);
    
    EXPECT_EQ(stream->position(), 0);
    
    stream->next();
    EXPECT_EQ(stream->position(), 1);
    
    stream->next();
    EXPECT_EQ(stream->position(), 2);
    
    stream->nextBatch(3);
    EXPECT_EQ(stream->position(), 5);
}

// ============================================================================
// Skip Operations Tests
// ============================================================================

TEST_F(ResultStreamTest, SkipItems) {
    auto stream = createKeyStream(test_keys_);
    
    auto skip_result = stream->skip(3);
    ASSERT_TRUE(skip_result.has_value()) << skip_result.error().message();
    
    auto result = stream->next();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "key4");
}

TEST_F(ResultStreamTest, SkipZeroItems) {
    auto stream = createKeyStream(test_keys_);
    
    auto skip_result = stream->skip(0);
    ASSERT_TRUE(skip_result.has_value());
    
    auto result = stream->next();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "key1");
}

TEST_F(ResultStreamTest, SkipBeyondEnd) {
    auto stream = createKeyStream(test_keys_);
    
    auto skip_result = stream->skip(100);
    ASSERT_TRUE(skip_result.has_value());
    
    EXPECT_FALSE(stream->hasNext());
}

TEST_F(ResultStreamTest, SkipPartiallyAvailable) {
    std::vector<std::string> three_items = {"a", "b", "c"};
    auto stream = createKeyStream(three_items);
    
    // Skip 2, consume 1, try to skip 2 more (only 0 available)
    stream->skip(2);
    stream->next();
    
    auto skip_result = stream->skip(2);
    EXPECT_TRUE(skip_result.has_value()); // Partial skip succeeds
    EXPECT_FALSE(stream->hasNext());
}

TEST_F(ResultStreamTest, SkipWithPotentialOverflow) {
    std::vector<std::string> small_dataset = {"a", "b", "c"};
    auto stream = createKeyStream(small_dataset);
    
    // Try to skip an extremely large number
    auto skip_result = stream->skip(std::numeric_limits<size_t>::max());
    ASSERT_TRUE(skip_result.has_value());
    
    // Should be at the end
    EXPECT_FALSE(stream->hasNext());
}

// ============================================================================
// Cursor Tests
// ============================================================================

TEST_F(ResultStreamTest, CursorState) {
    auto stream = createKeyStream(test_keys_);
    
    // Initial cursor
    auto cursor1 = stream->cursor();
    EXPECT_EQ(cursor1.offset, 0);
    EXPECT_TRUE(cursor1.has_more);
    
    // Advance and check cursor
    stream->nextBatch(3);
    auto cursor2 = stream->cursor();
    EXPECT_EQ(cursor2.offset, 3);
}

TEST_F(ResultStreamTest, SetCursor) {
    auto stream = createKeyStream(test_keys_);
    
    // Advance stream
    stream->nextBatch(5);
    
    // Save cursor
    PaginationCursor saved_cursor = stream->cursor();
    
    // Continue consuming
    stream->nextBatch(5);
    EXPECT_FALSE(stream->hasNext());
    
    // Restore cursor
    stream->setCursor(saved_cursor);
    
    // Verify we can continue from saved position
    auto result = stream->next();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "key6"); // First item after the saved cursor position
}

// ============================================================================
// Large Dataset Tests
// ============================================================================

TEST_F(ResultStreamTest, LargeDatasetStreaming) {
    StreamConfig config;
    config.batch_size = 100;
    
    auto stream = createKeyStream(large_dataset_, config);
    
    size_t total_count = 0;
    while (stream->hasNext()) {
        auto batch = stream->nextBatch(config.batch_size);
        ASSERT_TRUE(batch.has_value());
        total_count += batch->items.size();
    }
    
    EXPECT_EQ(total_count, large_dataset_.size());
}

TEST_F(ResultStreamTest, LargeDatasetWithCustomBatchSize) {
    StreamConfig config;
    config.batch_size = 50;
    config.max_buffer_size = 200;
    
    auto stream = std::make_shared<ResultStream<std::string>>(
        large_dataset_, config
    );
    
    // Fetch multiple batches
    size_t batch_count = 0;
    while (stream->hasNext()) {
        auto batch = stream->nextBatch(50);
        ASSERT_TRUE(batch.has_value());
        batch_count++;
        
        if (!batch->is_last_batch) {
            EXPECT_EQ(batch->items.size(), 50);
        }
    }
    
    EXPECT_EQ(batch_count, 20); // 1000 / 50 = 20 batches
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(ResultStreamTest, StatisticsTracking) {
    auto stream = createKeyStream(test_keys_);
    
    // Initial statistics
    auto stats1 = stream->statistics();
    EXPECT_EQ(stats1.items_read, 0);
    
    // Read some items
    stream->nextBatch(5);
    
    auto stats2 = stream->statistics();
    EXPECT_EQ(stats2.items_read, 5);
    
    // Read more items
    stream->next();
    stream->next();
    
    auto stats3 = stream->statistics();
    EXPECT_EQ(stats3.items_read, 7);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(ResultStreamTest, SingleItemStream) {
    std::vector<std::string> single = {"only_one"};
    auto stream = createKeyStream(single);
    
    EXPECT_TRUE(stream->hasNext());
    
    auto result = stream->next();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "only_one");
    
    EXPECT_FALSE(stream->hasNext());
}

TEST_F(ResultStreamTest, EmptyStream) {
    std::vector<std::string> empty;
    auto stream = createKeyStream(empty);
    
    EXPECT_FALSE(stream->hasNext());
    EXPECT_EQ(stream->position(), 0);
    
    auto stats = stream->statistics();
    EXPECT_EQ(stats.items_read, 0);
}

TEST_F(ResultStreamTest, StreamWithDuplicates) {
    std::vector<std::string> with_dupes = {"a", "b", "a", "c", "b", "a"};
    auto stream = createKeyStream(with_dupes);
    
    std::vector<std::string> results = {};

    while (stream->hasNext()) {
        auto result = stream->next();
        ASSERT_TRUE(result.has_value());
        results.push_back(*result);
    }
    
    EXPECT_EQ(results, with_dupes);
}

// ============================================================================
// Configuration Tests
// ============================================================================

TEST_F(ResultStreamTest, CustomStreamConfig) {
    StreamConfig config;
    config.batch_size = 25;
    config.max_buffer_size = 100;
    config.enable_backpressure = true;
    config.backpressure_threshold = 80;
    
    auto stream = createKeyStream(large_dataset_, config);
    
    // Stream should work with custom configuration
    auto batch = stream->nextBatch(25);
    ASSERT_TRUE(batch.has_value());
    EXPECT_LE(batch->items.size(), 25);
}

// ============================================================================
// Data Source Function Tests
// ============================================================================

TEST_F(ResultStreamTest, CustomDataSource) {
    // Create a data source that generates data on-demand
    int call_count = 0;
    auto data_source = [&call_count](const PaginationCursor& cursor, size_t batch_size) 
        -> Result<std::vector<std::string>> {
        call_count++;
        
        std::vector<std::string> batch;
        size_t start = cursor.offset;
        size_t end = std::min(start + batch_size, size_t(10));
        
        for (size_t i = start; i < end; ++i) {
            batch.push_back("generated_" + std::to_string(i));
        }
        
        return batch;
    };
    
    auto stream = createStream<std::string>(data_source);
    
    // Fetch all items
    std::vector<std::string> all_items = {};

    while (stream->hasNext()) {
        auto result = stream->next();
        if (result) {
            all_items.push_back(*result);
        }
    }
    
    EXPECT_GT(call_count, 0);
    EXPECT_EQ(all_items.size(), 10);
    EXPECT_EQ(all_items[0], "generated_0");
    EXPECT_EQ(all_items[9], "generated_9");
}
