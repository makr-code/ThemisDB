// Copyright 2025 ThemisDB
// Licensed under MIT License

/**
 * @file test_redundancy_consistency.cpp
 * @brief Tests for version-aware consistency in RedundancyStrategy
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY | Score: 95/100
 * @note Tests GAP fixes: undefined_conflict_resolution, unspecified_consistency, missing_version_tracking
 */

#include "sharding/redundancy_strategy.h"
#include "gtest/gtest.h"
#include <vector>
#include <string>
#include <optional>

namespace themis {
namespace sharding {

class RedundancyConsistencyTest : public ::testing::Test {
protected:
    void SetUp() override {
        RedundancyConfig config;
        config.mode = RedundancyMode::MIRROR;
        config.replication_factor = 3;
        config.conflict_resolution = ConflictResolution::LAST_WRITE_WINS;
        strategy_ = std::make_unique<RedundancyStrategy>(config);
    }
    
    void TearDown() override {
        strategy_.reset();
    }
    
    std::unique_ptr<RedundancyStrategy> strategy_;
};

// ============================================================================
// VersionedReadResult Tests
// ============================================================================

TEST_F(RedundancyConsistencyTest, VersionedReadResultDefaultValues) {
    VersionedReadResult result;
    EXPECT_FALSE(result.data.has_value());
    EXPECT_EQ(result.version_token, 0);
    EXPECT_TRUE(result.shard_id.empty());
}

TEST_F(RedundancyConsistencyTest, VersionedReadResultWithData) {
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    VersionedReadResult result;
    result.data = data;
    result.version_token = 42;
    result.shard_id = "shard-1";
    
    EXPECT_TRUE(result.data.has_value());
    EXPECT_EQ(result.data.value(), data);
    EXPECT_EQ(result.version_token, 42);
    EXPECT_EQ(result.shard_id, "shard-1");
}

// ============================================================================
// VersionedChunk Tests
// ============================================================================

TEST_F(RedundancyConsistencyTest, VersionedChunkDefaultValues) {
    VersionedChunk chunk;
    EXPECT_TRUE(chunk.data.empty());
    EXPECT_EQ(chunk.version_token, 0);
    EXPECT_TRUE(chunk.shard_id.empty());
}

TEST_F(RedundancyConsistencyTest, VersionedChunkWithData) {
    VersionedChunk chunk;
    chunk.data = {1, 2, 3};
    chunk.version_token = 100;
    chunk.shard_id = "shard-2";
    
    EXPECT_EQ(chunk.data.size(), 3);
    EXPECT_EQ(chunk.version_token, 100);
    EXPECT_EQ(chunk.shard_id, "shard-2");
}

// ============================================================================
// mergeChunksWithConsistency Tests - Consistent Case
// ============================================================================

TEST_F(RedundancyConsistencyTest, MergeChunksConsistentAllSameVersion) {
    std::vector<VersionedChunk> chunks = {
        {{1, 2}, 10, "shard-1"},
        {{3, 4}, 10, "shard-2"},
        {{5, 6}, 10, "shard-3"}
    };
    
    uint64_t result_version = 0;
    auto merged = strategy_->mergeChunksWithConsistency(
        chunks, ConflictResolution::LAST_WRITE_WINS, result_version
    );
    
    // Should merge all chunks
    EXPECT_EQ(merged.size(), 6);
    EXPECT_EQ(merged[0], 1);
    EXPECT_EQ(merged[1], 2);
    EXPECT_EQ(merged[2], 3);
    EXPECT_EQ(merged[3], 4);
    EXPECT_EQ(merged[4], 5);
    EXPECT_EQ(merged[5], 6);
    EXPECT_EQ(result_version, 10);
}

TEST_F(RedundancyConsistencyTest, MergeChunksEmptyInput) {
    std::vector<VersionedChunk> chunks;
    
    uint64_t result_version = 99;
    auto merged = strategy_->mergeChunksWithConsistency(
        chunks, ConflictResolution::LAST_WRITE_WINS, result_version
    );
    
    EXPECT_TRUE(merged.empty());
    EXPECT_EQ(result_version, 0);
}

TEST_F(RedundancyConsistencyTest, MergeChunksSingleChunk) {
    std::vector<VersionedChunk> chunks = {
        {{1, 2, 3}, 5, "shard-1"}
    };
    
    uint64_t result_version = 0;
    auto merged = strategy_->mergeChunksWithConsistency(
        chunks, ConflictResolution::LAST_WRITE_WINS, result_version
    );
    
    EXPECT_EQ(merged.size(), 3);
    EXPECT_EQ(merged[0], 1);
    EXPECT_EQ(merged[1], 2);
    EXPECT_EQ(merged[2], 3);
    EXPECT_EQ(result_version, 5);
}

// ============================================================================
// mergeChunksWithConsistency Tests - Conflict Resolution
// ============================================================================

TEST_F(RedundancyConsistencyTest, ConflictResolutionLastWriteWins) {
    std::vector<VersionedChunk> chunks = {
        {{1, 2}, 5, "shard-1"},   // Older
        {{3, 4}, 10, "shard-2"},  // Newer
        {{5, 6}, 7, "shard-3"}    // Middle
    };
    
    uint64_t result_version = 0;
    auto merged = strategy_->mergeChunksWithConsistency(
        chunks, ConflictResolution::LAST_WRITE_WINS, result_version
    );
    
    // Should select highest version (10 from shard-2)
    EXPECT_EQ(merged.size(), 2);
    EXPECT_EQ(merged[0], 3);
    EXPECT_EQ(merged[1], 4);
    EXPECT_EQ(result_version, 10);
}

TEST_F(RedundancyConsistencyTest, ConflictResolutionFirstWriteWins) {
    std::vector<VersionedChunk> chunks = {
        {{1, 2}, 10, "shard-1"},  // Newer
        {{3, 4}, 5, "shard-2"},   // Older
        {{5, 6}, 7, "shard-3"}    // Middle
    };
    
    uint64_t result_version = 0;
    auto merged = strategy_->mergeChunksWithConsistency(
        chunks, ConflictResolution::FIRST_WRITE_WINS, result_version
    );
    
    // Should select lowest version (5 from shard-2)
    EXPECT_EQ(merged.size(), 2);
    EXPECT_EQ(merged[0], 3);
    EXPECT_EQ(merged[1], 4);
    EXPECT_EQ(result_version, 5);
}

TEST_F(RedundancyConsistencyTest, ConflictResolutionHighestNodeId) {
    std::vector<VersionedChunk> chunks = {
        {{1, 2}, 10, "shard-a"},
        {{3, 4}, 10, "shard-z"},  // Highest node ID
        {{5, 6}, 10, "shard-m"}
    };
    
    uint64_t result_version = 0;
    auto merged = strategy_->mergeChunksWithConsistency(
        chunks, ConflictResolution::HIGHEST_NODE_ID, result_version
    );
    
    // Should select highest node ID (shard-z)
    EXPECT_EQ(merged.size(), 2);
    EXPECT_EQ(merged[0], 3);
    EXPECT_EQ(merged[1], 4);
    EXPECT_EQ(result_version, 10);  // Version from shard-z
}

TEST_F(RedundancyConsistencyTest, ConflictResolutionCustomFallback) {
    std::vector<VersionedChunk> chunks = {
        {{1, 2}, 5, "shard-1"},
        {{3, 4}, 10, "shard-2"},
        {{5, 6}, 7, "shard-3"}
    };
    
    uint64_t result_version = 0;
    auto merged = strategy_->mergeChunksWithConsistency(
        chunks, ConflictResolution::CUSTOM, result_version
    );
    
    // CUSTOM falls back to LAST_WRITE_WINS
    EXPECT_EQ(merged.size(), 2);
    EXPECT_EQ(merged[0], 3);
    EXPECT_EQ(merged[1], 4);
    EXPECT_EQ(result_version, 10);
}

// ============================================================================
// mergeChunksWithConsistency Tests - Edge Cases
// ============================================================================

TEST_F(RedundancyConsistencyTest, AllChunksEmptyData) {
    std::vector<VersionedChunk> chunks = {
        {{}, 10, "shard-1"},
        {{}, 10, "shard-2"},
        {{}, 10, "shard-3"}
    };
    
    uint64_t result_version = 0;
    auto merged = strategy_->mergeChunksWithConsistency(
        chunks, ConflictResolution::LAST_WRITE_WINS, result_version
    );
    
    EXPECT_TRUE(merged.empty());
    EXPECT_EQ(result_version, 10);
}

TEST_F(RedundancyConsistencyTest, MixedEmptyAndNonEmptyData) {
    std::vector<VersionedChunk> chunks = {
        {{1, 2}, 10, "shard-1"},
        {{}, 10, "shard-2"},
        {{3, 4}, 10, "shard-3"}
    };
    
    uint64_t result_version = 0;
    auto merged = strategy_->mergeChunksWithConsistency(
        chunks, ConflictResolution::LAST_WRITE_WINS, result_version
    );
    
    // Should merge all non-empty data
    EXPECT_EQ(merged.size(), 4);
    EXPECT_EQ(merged[0], 1);
    EXPECT_EQ(merged[1], 2);
    EXPECT_EQ(merged[2], 3);
    EXPECT_EQ(merged[3], 4);
    EXPECT_EQ(result_version, 10);
}

// ============================================================================
// Conflict Detection Tests
// ============================================================================

TEST_F(RedundancyConsistencyTest, DetectsConsistency) {
    std::vector<VersionedChunk> consistent_chunks = {
        {{1, 2}, 10, "shard-1"},
        {{3, 4}, 10, "shard-2"},
        {{5, 6}, 10, "shard-3"}
    };
    
    uint64_t result_version = 0;
    auto merged = strategy_->mergeChunksWithConsistency(
        consistent_chunks, ConflictResolution::LAST_WRITE_WINS, result_version
    );
    
    // All chunks have same version - should merge all
    EXPECT_EQ(merged.size(), 6);
    EXPECT_EQ(result_version, 10);
}

TEST_F(RedundancyConsistencyTest, DetectsConflict) {
    std::vector<VersionedChunk> conflicting_chunks = {
        {{1, 2}, 10, "shard-1"},
        {{3, 4}, 15, "shard-2"},  // Different version
        {{5, 6}, 10, "shard-3"}
    };
    
    uint64_t result_version = 0;
    auto merged = strategy_->mergeChunksWithConsistency(
        conflicting_chunks, ConflictResolution::LAST_WRITE_WINS, result_version
    );
    
    // Conflict detected - should select single chunk (version 15)
    EXPECT_EQ(merged.size(), 2);
    EXPECT_EQ(result_version, 15);
}

// ============================================================================
// ReadPreference Tests
// ============================================================================

TEST_F(RedundancyConsistencyTest, ReadPreferenceValues) {
    EXPECT_EQ(static_cast<int>(ReadPreference::PRIMARY), 0);
    EXPECT_EQ(static_cast<int>(ReadPreference::NEAREST), 1);
    EXPECT_EQ(static_cast<int>(ReadPreference::ROUND_ROBIN), 2);
    EXPECT_EQ(static_cast<int>(ReadPreference::RANDOM), 3);
    EXPECT_EQ(static_cast<int>(ReadPreference::SECONDARY_ONLY), 4);
    EXPECT_EQ(static_cast<int>(ReadPreference::FOLLOWER), 5);
    EXPECT_EQ(static_cast<int>(ReadPreference::LOCAL_REGION), 6);
}

// ============================================================================
// ConflictResolution Enum Tests
// ============================================================================

TEST_F(RedundancyConsistencyTest, ConflictResolutionValues) {
    EXPECT_EQ(static_cast<int>(ConflictResolution::LAST_WRITE_WINS), 0);
    EXPECT_EQ(static_cast<int>(ConflictResolution::FIRST_WRITE_WINS), 1);
    EXPECT_EQ(static_cast<int>(ConflictResolution::HIGHEST_NODE_ID), 2);
    EXPECT_EQ(static_cast<int>(ConflictResolution::CUSTOM), 3);
}

// ============================================================================
// RedundancyMode Enum Tests
// ============================================================================

TEST_F(RedundancyConsistencyTest, RedundancyModeValues) {
    EXPECT_EQ(static_cast<int>(RedundancyMode::NONE), 0);
    EXPECT_EQ(static_cast<int>(RedundancyMode::MIRROR), 1);
    EXPECT_EQ(static_cast<int>(RedundancyMode::STRIPE), 2);
    EXPECT_EQ(static_cast<int>(RedundancyMode::STRIPE_MIRROR), 3);
    EXPECT_EQ(static_cast<int>(RedundancyMode::PARITY), 4);
    EXPECT_EQ(static_cast<int>(RedundancyMode::RAID6), 5);
    EXPECT_EQ(static_cast<int>(RedundancyMode::GEO_MIRROR), 6);
}

// ============================================================================
// Configuration Tests
// ============================================================================

TEST_F(RedundancyConsistencyTest, ConfigWithConflictResolution) {
    RedundancyConfig config;
    config.mode = RedundancyMode::MIRROR;
    config.replication_factor = 3;
    config.conflict_resolution = ConflictResolution::HIGHEST_NODE_ID;
    
    auto strategy = std::make_unique<RedundancyStrategy>(config);
    EXPECT_NE(strategy, nullptr);
}

TEST_F(RedundancyConsistencyTest, ConfigDefaultConflictResolution) {
    RedundancyConfig config;
    config.mode = RedundancyMode::MIRROR;
    config.replication_factor = 3;
    // conflict_resolution should default to LAST_WRITE_WINS (0)
    
    EXPECT_EQ(static_cast<int>(config.conflict_resolution), 0);
}

} // namespace sharding
} // namespace themis
