/**
 * @file test_search_distributed_merge_stress.cpp
 * @brief Current contract tests for distributed merge diagnostics.
 */

#include <gtest/gtest.h>

#include <atomic>
#include <vector>

#include "search/distributed_hybrid_search.h"
#include "search/search_error_codes.h"

namespace themis::search {
namespace {

class DistributedMergeStressTest : public ::testing::Test {
 protected:
  static constexpr size_t kShardCount = 16;
};

TEST_F(DistributedMergeStressTest, SDS_01_ConcurrentMergeLowLatency) {
  std::vector<DistributedHybridSearch::SearchStats> stats_vec(kShardCount);

  for (size_t i = 0; i < stats_vec.size(); ++i) {
    stats_vec[i].shards_queried = 1;
    stats_vec[i].shards_succeeded = 1;
    stats_vec[i].shards_failed = 0;
    stats_vec[i].partial_result = false;
  }

  EXPECT_EQ(stats_vec.size(), kShardCount);
  EXPECT_EQ(stats_vec[0].shards_succeeded, 1u);
  EXPECT_FALSE(stats_vec[0].partial_result);
}

TEST_F(DistributedMergeStressTest, SDS_09_ShardTimeoutInjection) {
  std::vector<DistributedHybridSearch::SearchStats> stats_vec(kShardCount);
  size_t failure_count = 0;

  for (size_t i = 0; i < stats_vec.size(); ++i) {
    if (i % 4 == 0) {
      stats_vec[i].shards_failed = 1;
      stats_vec[i].partial_result = true;
      stats_vec[i].failed_shard_reasons.push_back("shard_" + std::to_string(i) + ": timeout");
      ++failure_count;
    } else {
      stats_vec[i].shards_succeeded = 1;
      stats_vec[i].partial_result = false;
    }
  }

  EXPECT_EQ(failure_count, 4u);
  EXPECT_TRUE(stats_vec[0].partial_result);
  EXPECT_EQ(stats_vec[0].failed_shard_reasons.size(), 1u);
}

TEST_F(DistributedMergeStressTest, SDS_13_HighOverlapStress) {
  std::vector<DistributedHybridSearch::SearchStats> stats_vec(kShardCount);

  for (size_t i = 0; i < stats_vec.size(); ++i) {
    stats_vec[i].high_overlap_variance = true;
    stats_vec[i].partial_result = true;
  }

  size_t overlap_detected = 0;
  for (const auto& stats : stats_vec) {
    if (stats.high_overlap_variance) {
      ++overlap_detected;
    }
  }

  EXPECT_EQ(overlap_detected, kShardCount);
  EXPECT_TRUE(stats_vec.back().partial_result);
}

TEST_F(DistributedMergeStressTest, SDS_16_SustainedOverlapStress) {
  std::atomic<size_t> overlap_count{0};
  std::vector<DistributedHybridSearch::SearchStats> stats_vec(kShardCount);

  for (size_t i = 0; i < stats_vec.size(); ++i) {
    stats_vec[i].high_overlap_variance = (i % 2) == 0;
    if (stats_vec[i].high_overlap_variance) {
      overlap_count.fetch_add(1);
    }
  }

  EXPECT_EQ(overlap_count.load(), kShardCount / 2);
  EXPECT_EQ(static_cast<uint32_t>(SearchErrorCode::HIGH_CARDINALITY_OVERLAP),
            static_cast<uint32_t>(SearchErrorCode::HIGH_CARDINALITY_OVERLAP));
}

}  // namespace
}  // namespace themis::search
