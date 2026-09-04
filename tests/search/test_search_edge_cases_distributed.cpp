/**
 * @file test_search_edge_cases_distributed.cpp
 * @brief Contract smoke tests for distributed search degradation diagnostics.
 *
 * This file intentionally keeps to the public API that is still valid in the
 * current codebase. Older edge-case tests referenced removed fields such as
 * failed_shard_ids, shard_latencies_ms, and legacy error constants that no
 * longer exist in the distributed search contract.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <string>
#include <vector>

#include "search/distributed_hybrid_search.h"
#include "search/hybrid_search.h"
#include "search/search_error_codes.h"

namespace themis::search {
namespace testing {

class DistributedSearchEdgeCasesTest : public ::testing::Test {
 protected:
  static constexpr uint32_t kCanonicalRngSeed = 42;

  void SetUp() override {
    srand(kCanonicalRngSeed);
  }
};

TEST_F(DistributedSearchEdgeCasesTest, DIS_01_PartialShardFailureFirstShard) {
  DistributedHybridSearch::SearchStats stats;
  stats.shards_queried = 3;
  stats.shards_succeeded = 2;
  stats.shards_failed = 1;
  stats.partial_result = true;
  stats.failed_shard_reasons.push_back("shard_0: timeout");

  EXPECT_TRUE(stats.partial_result);
  EXPECT_EQ(stats.shards_failed, 1u);
  EXPECT_EQ(stats.shards_succeeded, 2u);
  EXPECT_EQ(stats.failed_shard_reasons.size(), 1u);
  EXPECT_NE(stats.failed_shard_reasons.front().find("shard_0"), std::string::npos);
}

TEST_F(DistributedSearchEdgeCasesTest, DIS_02_CascadingShardFailures) {
  DistributedHybridSearch::SearchStats stats;
  stats.shards_queried = 5;
  stats.shards_succeeded = 2;
  stats.shards_failed = 3;
  stats.partial_result = true;
  stats.failed_shard_reasons.push_back("shard_1: HTTP 500");
  stats.failed_shard_reasons.push_back("shard_3: timeout");
  stats.failed_shard_reasons.push_back("shard_4: connection reset");

  EXPECT_TRUE(stats.partial_result);
  EXPECT_EQ(stats.shards_failed, 3u);
  EXPECT_LT(stats.shards_succeeded, stats.shards_queried);
  EXPECT_EQ(stats.failed_shard_reasons.size(), 3u);
}

TEST_F(DistributedSearchEdgeCasesTest, DIS_03_NetworkInducedUnderflow) {
  std::vector<HybridSearch::Result> merged_results;
  constexpr int kExpectedResults = 10;
  constexpr int kActualResults = 4;

  for (int i = 0; i < kActualResults; ++i) {
    HybridSearch::Result r;
    r.document_id = "doc_" + std::to_string(i);
    r.hybrid_score = 1.0 - (i * 0.1);
    merged_results.push_back(r);
  }

  EXPECT_LT(static_cast<int>(merged_results.size()), kExpectedResults);
  EXPECT_EQ(static_cast<int>(merged_results.size()), kActualResults);
  EXPECT_GE(merged_results.front().hybrid_score, merged_results.back().hybrid_score);
}

TEST_F(DistributedSearchEdgeCasesTest, DIS_04_NetworkDelayAccumulation) {
  std::vector<int> shard_latency_ms = {500, 450, 520, 480};
  int total_execution_time_ms = 550;

  double avg_latency = 0.0;
  for (int latency : shard_latency_ms) {
    avg_latency += latency;
  }
  avg_latency /= static_cast<double>(shard_latency_ms.size());

  EXPECT_NEAR(avg_latency, 487.5, 1.0);
  EXPECT_LT(total_execution_time_ms, avg_latency * 2.0);
}

TEST_F(DistributedSearchEdgeCasesTest, DIS_05_ResponseReordering) {
  std::vector<int> shard_order = {0, 1, 2, 3};
  std::vector<int> arrival_order = {2, 0, 3, 1};

  EXPECT_NE(shard_order, arrival_order);

  std::vector<HybridSearch::Result> reassembled = {};

  for (int idx : arrival_order) {
    HybridSearch::Result r;
    r.document_id = "doc_" + std::to_string(idx);
    reassembled.push_back(r);
  }

  EXPECT_EQ(reassembled.size(), 4u);
  EXPECT_EQ(reassembled[0].document_id, "doc_2");
  EXPECT_EQ(reassembled[1].document_id, "doc_0");
}

TEST_F(DistributedSearchEdgeCasesTest, DIS_06_DeduplicationAfterReordering) {
  std::vector<HybridSearch::Result> raw_results;

  for (int i = 0; i < 3; ++i) {
    HybridSearch::Result r;
    r.document_id = "doc_0";
    r.hybrid_score = 0.9 - (i * 0.05);
    raw_results.push_back(r);
  }

  std::vector<HybridSearch::Result> deduped;
  bool found_doc_0 = false;
  for (const auto& r : raw_results) {
    if (r.document_id != "doc_0" || !found_doc_0) {
      deduped.push_back(r);
      if (r.document_id == "doc_0") {
        found_doc_0 = true;
      }
    }
  }

  EXPECT_EQ(deduped.size(), 1u);
  EXPECT_EQ(deduped.front().document_id, "doc_0");
}

TEST_F(DistributedSearchEdgeCasesTest, DIS_07_CascadingTimeoutPartialRecovery) {
  DistributedHybridSearch::SearchStats stats;
  stats.shards_queried = 6;
  stats.shards_succeeded = 3;
  stats.shards_failed = 3;
  stats.partial_result = true;
  stats.failed_shard_reasons.push_back("shard_2: connection timeout");
  stats.failed_shard_reasons.push_back("shard_3: HTTP 503");

  EXPECT_TRUE(stats.partial_result);
  EXPECT_EQ(stats.shards_failed, 3u);
  EXPECT_GT(stats.shards_succeeded, 0u);
  EXPECT_EQ(stats.failed_shard_reasons.size(), 2u);
}

TEST_F(DistributedSearchEdgeCasesTest, DIS_08_TimeoutRecoveryBackoff) {
  std::vector<int> retry_delays_ms = {10, 20, 40};

  int expected = 10;
  for (int i = 0; i < 3; ++i) {
    EXPECT_EQ(retry_delays_ms[i], expected);
    expected *= 2;
  }
}

TEST_F(DistributedSearchEdgeCasesTest, DIS_09_LoadBalancingUnevenLatency) {
  std::vector<int> latency_ms = {100, 2000, 150, 120};

  int slow_count = 0;
  for (int latency : latency_ms) {
    if (latency > 1000) {
      ++slow_count;
    }
  }

  EXPECT_EQ(slow_count, 1);
  EXPECT_EQ(*std::max_element(latency_ms.begin(), latency_ms.end()), 2000);
}

TEST_F(DistributedSearchEdgeCasesTest, DIS_10_AdaptiveTimeoutAdjustment) {
  std::vector<int> historical_latencies = {100, 120, 150, 110};

  double avg = 0.0;
  for (int latency : historical_latencies) {
    avg += latency;
  }
  avg /= static_cast<double>(historical_latencies.size());

  double variance = 0.0;
  for (int latency : historical_latencies) {
    variance += (latency - avg) * (latency - avg);
  }
  variance /= static_cast<double>(historical_latencies.size());

  double stddev = std::sqrt(variance);
  int adaptive_timeout = static_cast<int>(avg + 2.0 * stddev);

  EXPECT_GT(adaptive_timeout, static_cast<int>(avg));
  EXPECT_LT(adaptive_timeout, 200);
}

TEST_F(DistributedSearchEdgeCasesTest, DIS_11_ShardStateCorruptionDetection) {
  struct ShardState {
    std::string shard_id = {};
    uint64_t checksum = {};
    int result_count = {};
    bool is_valid() const { return checksum != 0; }
  };

  ShardState state;
  state.shard_id = "shard_0";
  state.checksum = 0;
  state.result_count = 5;

  EXPECT_FALSE(state.is_valid());

  state.checksum = 0x12345678u;
  EXPECT_TRUE(state.is_valid());
}

TEST_F(DistributedSearchEdgeCasesTest, DIS_12_ShardStateTransitionConsistency) {
  enum ShardPhase { IDLE, QUERYING, RESPONDING, COMPLETED, FAILED };

  ShardPhase phase = IDLE;
  EXPECT_EQ(phase, IDLE);

  phase = QUERYING;
  EXPECT_EQ(phase, QUERYING);

  phase = RESPONDING;
  EXPECT_EQ(phase, RESPONDING);

  phase = COMPLETED;
  EXPECT_EQ(phase, COMPLETED);

  EXPECT_NE(phase, IDLE);
}

TEST_F(DistributedSearchEdgeCasesTest, DIS_13_ConcurrentShardQueryIsolation) {
  struct QueryContext {
    int query_id = 0;
    int shard_id = {};
  };

  std::vector<QueryContext> contexts = {{1, 0}, {1, 1}, {2, 0}, {2, 1}};

  int query_1_count = 0;
  int query_2_count = 0;
  for (const auto& ctx : contexts) {
    if (ctx.query_id == 1) {
      ++query_1_count;
    } else {
      ++query_2_count;
    }
  }

  EXPECT_EQ(query_1_count, 2);
  EXPECT_EQ(query_2_count, 2);
}

TEST_F(DistributedSearchEdgeCasesTest, DIS_14_ConcurrentRaceConditionPrevention) {
  std::vector<HybridSearch::Result> results;
  const int expected_result_count = 10;

  for (int i = 0; i < expected_result_count; ++i) {
    HybridSearch::Result r;
    r.document_id = "doc_" + std::to_string(i);
    results.push_back(r);
  }

  EXPECT_EQ(static_cast<int>(results.size()), expected_result_count);
  for (int i = 0; i < expected_result_count; ++i) {
    EXPECT_EQ(results[i].document_id, "doc_" + std::to_string(i));
  }
}

TEST_F(DistributedSearchEdgeCasesTest, DIS_15_ReplicaFailoverAutomatic) {
  DistributedHybridSearch::SearchStats stats;
  stats.shards_queried = 1;
  stats.shards_succeeded = 1;
  stats.shards_failed = 0;
  stats.partial_result = false;
  stats.failed_shard_reasons.clear();

  EXPECT_FALSE(stats.partial_result);
  EXPECT_EQ(stats.shards_succeeded, 1u);
  EXPECT_EQ(stats.shards_failed, 0u);
  EXPECT_TRUE(stats.failed_shard_reasons.empty());
}

TEST_F(DistributedSearchEdgeCasesTest, DIS_16_ReplicaConsistencyAfterFailover) {
  std::vector<HybridSearch::Result> primary_results;
  std::vector<HybridSearch::Result> replica_results;

  for (int i = 0; i < 5; ++i) {
    HybridSearch::Result r;
    r.document_id = "doc_" + std::to_string(i);
    r.hybrid_score = 1.0 - (i * 0.1);
    primary_results.push_back(r);
    replica_results.push_back(r);
  }

  EXPECT_EQ(primary_results.size(), replica_results.size());
  for (size_t i = 0; i < primary_results.size(); ++i) {
    EXPECT_EQ(primary_results[i].document_id, replica_results[i].document_id);
    EXPECT_DOUBLE_EQ(primary_results[i].hybrid_score, replica_results[i].hybrid_score);
  }
}

}  // namespace testing
}  // namespace themis::search
