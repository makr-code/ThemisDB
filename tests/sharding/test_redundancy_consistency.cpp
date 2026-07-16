// Copyright 2025 ThemisDB
// Licensed under MIT License

/**
 * @file test_redundancy_consistency.cpp
 * @brief API-level consistency tests for RedundancyStrategy.
 */

#include "sharding/redundancy_strategy.h"
#include "gtest/gtest.h"

#include <memory>
#include <vector>

namespace themis {
namespace sharding {

class RedundancyConsistencyTest : public ::testing::Test {
protected:
    void SetUp() override {
        RedundancyConfig config;
        config.mode = RedundancyMode::MIRROR;
        config.replication_factor = 3;
        strategy_ = std::make_unique<RedundancyStrategy>(config);
    }

    std::unique_ptr<RedundancyStrategy> strategy_;
};

TEST_F(RedundancyConsistencyTest, VersionedReadResultDefaults) {
    RedundancyStrategy::VersionedReadResult result;
    EXPECT_FALSE(result.data.has_value());
    EXPECT_EQ(result.version_token, 0u);
    EXPECT_TRUE(result.shard_id.empty());
}

TEST_F(RedundancyConsistencyTest, VersionedChunkDefaults) {
    RedundancyStrategy::VersionedChunk chunk;
    EXPECT_TRUE(chunk.data.empty());
    EXPECT_EQ(chunk.version_token, 0u);
    EXPECT_TRUE(chunk.shard_id.empty());
}

TEST_F(RedundancyConsistencyTest, VersionedChunkAssignmentRoundTrip) {
    RedundancyStrategy::VersionedChunk chunk;
    chunk.data = {1, 2, 3, 4};
    chunk.version_token = 42;
    chunk.shard_id = "shard-1";

    EXPECT_EQ(chunk.data.size(), 4u);
    EXPECT_EQ(chunk.version_token, 42u);
    EXPECT_EQ(chunk.shard_id, "shard-1");
}

TEST_F(RedundancyConsistencyTest, ConfigRoundTripThroughStrategy) {
    const auto& cfg = strategy_->getConfig();
    EXPECT_EQ(cfg.mode, RedundancyMode::MIRROR);
    EXPECT_EQ(cfg.replication_factor, 3u);
}

}  // namespace sharding
}  // namespace themis
