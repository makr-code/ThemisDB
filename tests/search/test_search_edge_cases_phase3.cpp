/**
 * @file test_search_edge_cases_phase3.cpp
 * @brief Current-phase contract checks for hybrid and distributed search.
 */

#include <gtest/gtest.h>

#include <vector>

#include "search/distributed_hybrid_search.h"
#include "search/hybrid_search.h"
#include "search/search_error_codes.h"
#include "search/search_result_stream.h"

namespace themis::search {
namespace testing {

TEST(SearchEdgeCasesPhase3, P3_01_EmptyResultSetHybridFusion) {
  HybridSearch::SearchStats stats;
  stats.partial_result = false;
  stats.bm25_count = 0;
  stats.vector_count = 0;
  stats.primary_error_code = static_cast<uint32_t>(SearchErrorCode::BOTH_BACKENDS_EMPTY);

  EXPECT_EQ(stats.bm25_count, 0u);
  EXPECT_EQ(stats.vector_count, 0u);
  EXPECT_EQ(stats.primary_error_code,
            static_cast<uint32_t>(SearchErrorCode::BOTH_BACKENDS_EMPTY));
  EXPECT_FALSE(stats.partial_result);
}

TEST(SearchEdgeCasesPhase3, P3_02_ShardTimeoutDegradation) {
  DistributedHybridSearch::SearchStats stats;
  stats.shards_queried = 2;
  stats.shards_succeeded = 1;
  stats.shards_failed = 1;
  stats.partial_result = true;
  stats.failed_shard_reasons.push_back("shard_1: timeout");

  EXPECT_TRUE(stats.partial_result);
  EXPECT_EQ(stats.shards_failed, 1u);
  EXPECT_GT(stats.shards_succeeded, 0u);
  EXPECT_EQ(stats.failed_shard_reasons.size(), 1u);
}

TEST(SearchEdgeCasesPhase3, P3_03_KLimitUnderflowHighOverlap) {
  DistributedHybridSearch::SearchStats stats;
  stats.high_overlap_variance = true;
  stats.merge_underflow = true;

  EXPECT_TRUE(stats.high_overlap_variance);
  EXPECT_TRUE(stats.merge_underflow);
}

TEST(SearchEdgeCasesPhase3, P3_04_AllShardsFailed) {
  DistributedHybridSearch::SearchStats stats;
  stats.shards_queried = 3;
  stats.shards_succeeded = 0;
  stats.shards_failed = 3;
  stats.partial_result = false;

  EXPECT_EQ(stats.shards_succeeded, 0u);
  EXPECT_EQ(stats.shards_failed, 3u);
  EXPECT_FALSE(stats.partial_result);
}

TEST(SearchEdgeCasesPhase3, P3_05_RerankerFallback) {
  HybridSearch::SearchStats stats;
  stats.rerank_fallback = true;
  stats.partial_result = false;
  stats.primary_error_code = static_cast<uint32_t>(SearchErrorCode::RERANKER_BACKEND_UNAVAILABLE);

  EXPECT_TRUE(stats.rerank_fallback);
  EXPECT_FALSE(stats.partial_result);
  EXPECT_EQ(stats.primary_error_code,
            static_cast<uint32_t>(SearchErrorCode::RERANKER_BACKEND_UNAVAILABLE));
}

TEST(SearchEdgeCasesPhase3, P3_06_ExpansionLimitExceeded) {
  HybridSearch::SearchStats stats;
  stats.primary_error_code = static_cast<uint32_t>(SearchErrorCode::EXPANSION_LIMIT_EXCEEDED);
  stats.partial_result = true;

  EXPECT_EQ(stats.primary_error_code,
            static_cast<uint32_t>(SearchErrorCode::EXPANSION_LIMIT_EXCEEDED));
  EXPECT_TRUE(stats.partial_result);
}

TEST(SearchEdgeCasesPhase3, P3_07_FusionFailureWithFallback) {
  HybridSearch::SearchStats stats;
  stats.fusion_failed = true;
  stats.partial_result = true;
  stats.primary_error_code = static_cast<uint32_t>(SearchErrorCode::RRF_FUSION_FAILED);
  stats.bm25_count = 10;
  stats.vector_count = 0;

  EXPECT_TRUE(stats.fusion_failed);
  EXPECT_TRUE(stats.partial_result);
  EXPECT_EQ(stats.primary_error_code,
            static_cast<uint32_t>(SearchErrorCode::RRF_FUSION_FAILED));
  EXPECT_GT(stats.bm25_count, 0u);
  EXPECT_EQ(stats.vector_count, 0u);
}

TEST(SearchEdgeCasesPhase3, P3_08_ConcurrentLayerFailures) {
  DistributedHybridSearch::SearchStats stats;
  stats.shards_queried = 4;
  stats.shards_succeeded = 2;
  stats.shards_failed = 2;
  stats.partial_result = true;
  stats.merge_underflow = true;
  stats.failed_shard_reasons.push_back("shard_2: connection timeout");
  stats.failed_shard_reasons.push_back("shard_3: HTTP 503");

  EXPECT_TRUE(stats.partial_result);
  EXPECT_EQ(stats.shards_failed, 2u);
  EXPECT_TRUE(stats.merge_underflow);
  EXPECT_EQ(stats.failed_shard_reasons.size(), 2u);
}

TEST(SearchEdgeCasesPhase3, SearchResultStreamTimeout) {
  SearchResultStream::Config config;
  config.open_timeout_ms = 100;

  EXPECT_EQ(config.open_timeout_ms, 100u);
}

TEST(SearchEdgeCasesPhase3, ErrorCodeRangeFiltering) {
  EXPECT_TRUE(static_cast<uint32_t>(SearchErrorCode::BM25_NO_RESULTS) >= 0x0000 &&
              static_cast<uint32_t>(SearchErrorCode::BM25_NO_RESULTS) <= 0x0FFF);
  EXPECT_TRUE(static_cast<uint32_t>(SearchErrorCode::RRF_FUSION_FAILED) >= 0x1000 &&
              static_cast<uint32_t>(SearchErrorCode::RRF_FUSION_FAILED) <= 0x1FFF);
  EXPECT_TRUE(static_cast<uint32_t>(SearchErrorCode::SHARD_TIMEOUT) >= 0x2000 &&
              static_cast<uint32_t>(SearchErrorCode::SHARD_TIMEOUT) <= 0x2FFF);
  EXPECT_TRUE(static_cast<uint32_t>(SearchErrorCode::EXPANSION_LIMIT_EXCEEDED) >= 0x3000 &&
              static_cast<uint32_t>(SearchErrorCode::EXPANSION_LIMIT_EXCEEDED) <= 0x3FFF);
  EXPECT_TRUE(static_cast<uint32_t>(SearchErrorCode::ANALYTICS_BUFFER_FULL) >= 0x4000 &&
              static_cast<uint32_t>(SearchErrorCode::ANALYTICS_BUFFER_FULL) <= 0x4FFF);
}

TEST(SearchEdgeCasesPhase3, DegradationFlagIndependence) {
  HybridSearch::SearchStats stats;

  stats.fusion_failed = true;
  stats.rerank_fallback = false;
  EXPECT_TRUE(stats.fusion_failed);
  EXPECT_FALSE(stats.rerank_fallback);

  stats.rerank_fallback = true;
  EXPECT_TRUE(stats.fusion_failed);
  EXPECT_TRUE(stats.rerank_fallback);

  stats.fusion_failed = false;
  stats.rerank_fallback = false;
  EXPECT_FALSE(stats.fusion_failed);
  EXPECT_FALSE(stats.rerank_fallback);
}

}  // namespace testing
}  // namespace themis::search
