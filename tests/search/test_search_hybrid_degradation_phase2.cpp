/**
 * @file test_search_hybrid_degradation_phase2.cpp
 * @brief Phase 2: HybridSearch Degradation Flags Tests
 * @version 1.0.0
 * @date 2026-08-06
 *
 * Tests for Phase 2 HybridSearch SearchStats enhancements:
 * - primary_error_code tracking via search_error_codes.h
 * - Degradation flags: fusion_failed, rerank_fallback
 * - Backend health visibility: bm25_unavailable, vector_unavailable
 *
 * Test IDs: P2H-01 through P2H-04
 * CTest labels: search, hybrid, phase2, degradation
 */

#include <gtest/gtest.h>
#include "search/hybrid_search.h"
#include "search/search_error_codes.h"

namespace themis::search {
namespace testing {

/**
 * P2H-01: SearchStats structure contains degradation flags
 * Verifies that SearchStats has all Phase 2 fields.
 */
TEST(HybridSearchDegradationPhase2, P2H_01_SearchStatsStructure) {
  HybridSearch::SearchStats stats;
  
  // Verify all Phase 1 fields
  EXPECT_EQ(stats.bm25_ok, false);
  EXPECT_EQ(stats.vector_ok, false);
  EXPECT_EQ(stats.partial_result, false);
  EXPECT_EQ(stats.bm25_count, 0);
  EXPECT_EQ(stats.vector_count, 0);
  
  // Verify all Phase 2 fields
  EXPECT_EQ(stats.primary_error_code, 0x0000);  // SUCCESS
  EXPECT_EQ(stats.fusion_failed, false);
  EXPECT_EQ(stats.rerank_fallback, false);
}

/**
 * P2H-02: Error code constants from search_error_codes.h
 * Verifies that common error codes are defined and accessible.
 */
TEST(HybridSearchDegradationPhase2, P2H_02_ErrorCodeConstants) {
  // Verify error code ranges are defined
  static_assert(SEARCH_ERR_BM25_NO_RESULTS == 0x0001);
  static_assert(SEARCH_ERR_VECTOR_BACKEND_UNAVAILABLE == 0x0002);
  static_assert(SEARCH_ERR_RRF_FUSION_FAILED == 0x1000);
  static_assert(SEARCH_ERR_SHARD_TIMEOUT == 0x2000);
  static_assert(SEARCH_ERR_EXPANSION_LIMIT_EXCEEDED == 0x3000);
  
  // Verify error code conversion function exists
  const char* msg = searchErrorCodeToString(SEARCH_ERR_BM25_NO_RESULTS);
  EXPECT_NE(msg, nullptr);
  EXPECT_STRNE(msg, "");
}

/**
 * P2H-03: Primary error code tracking
 * Verifies that primary_error_code field can hold meaningful error codes.
 */
TEST(HybridSearchDegradationPhase2, P2H_03_PrimaryErrorCodeTracking) {
  HybridSearch::SearchStats stats;
  
  // Simulate BM25 failure scenario
  stats.primary_error_code = SEARCH_ERR_BM25_NO_RESULTS;
  stats.bm25_ok = false;
  stats.partial_result = true;
  
  EXPECT_EQ(stats.primary_error_code, 0x0001);
  EXPECT_FALSE(stats.bm25_ok);
  EXPECT_TRUE(stats.partial_result);
  
  // Simulate vector backend failure
  stats.primary_error_code = SEARCH_ERR_VECTOR_BACKEND_UNAVAILABLE;
  stats.vector_ok = false;
  
  EXPECT_EQ(stats.primary_error_code, 0x0002);
  EXPECT_FALSE(stats.vector_ok);
}

/**
 * P2H-04: Degradation flag scenarios
 * Verifies distinct degradation flag combinations.
 */
TEST(HybridSearchDegradationPhase2, P2H_04_DegradationScenarios) {
  // Scenario 1: Fusion failure
  {
    HybridSearch::SearchStats stats;
    stats.fusion_failed = true;
    stats.primary_error_code = SEARCH_ERR_RRF_FUSION_FAILED;
    stats.partial_result = true;
    
    EXPECT_TRUE(stats.fusion_failed);
    EXPECT_EQ(stats.primary_error_code, 0x1000);
  }
  
  // Scenario 2: Reranker fallback
  {
    HybridSearch::SearchStats stats;
    stats.rerank_fallback = true;
    stats.partial_result = false;  // Fallback is transparent
    
    EXPECT_TRUE(stats.rerank_fallback);
  }
  
  // Scenario 3: Multiple degradations
  {
    HybridSearch::SearchStats stats;
    stats.fusion_failed = true;
    stats.rerank_fallback = true;
    stats.partial_result = true;
    stats.bm25_count = 5;
    stats.vector_count = 0;  // Vector unavailable
    
    EXPECT_TRUE(stats.fusion_failed);
    EXPECT_TRUE(stats.rerank_fallback);
    EXPECT_TRUE(stats.partial_result);
    EXPECT_EQ(stats.bm25_count, 5);
    EXPECT_EQ(stats.vector_count, 0);
  }
}

}  // namespace testing
}  // namespace themis::search

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
