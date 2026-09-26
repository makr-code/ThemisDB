/**
 * @file test_dual_read_validator.cpp
 * @brief Tests for DualReadValidator query-time metric collection
 *
 * Tests metric computation, regression detection, and batch validation.
 * Test cases: DUALREAD-01..DUALREAD-06
 *
 * @date 2026-09-24
 */

#include <gtest/gtest.h>
#include <vector>
#include <map>

#include "retrieval/dual_read_validator.h"

using namespace themis::retrieval;

class DualReadValidatorTest : public ::testing::Test {
 protected:
  DualReadValidator validator_{"/tmp/current", "/tmp/previous"};

  // Helper to create retrieval results
  std::vector<RetrievalResult> CreateResults(
      const std::vector<std::pair<std::string, double>>& docs) {
    std::vector<RetrievalResult> results;
    for (size_t i = 0; i < docs.size(); ++i) {
      RetrievalResult r;
      r.document_id = docs[i].first;
      r.relevance_score = docs[i].second;
      r.rank = i + 1;
      results.push_back(r);
    }
    return results;
  }
};

/**
 * @test DUALREAD-01: Detect regression (recall drop > 2pp)
 */
TEST_F(DualReadValidatorTest, DUALREAD_01_DetectRegression) {
  RetrievalMetrics current;
  current.recall_at_10 = 0.80;  // 80% recall
  current.ndcg_at_10 = 0.75;
  current.mrr_at_10 = 0.90;

  RetrievalMetrics previous;
  previous.recall_at_10 = 0.85;  // 85% recall (5pp better)
  previous.ndcg_at_10 = 0.78;
  previous.mrr_at_10 = 0.92;

  // Delta is 5pp > 2pp tolerance → regression detected
  EXPECT_TRUE(current.HasRegression(previous, 2.0));
}

/**
 * @test DUALREAD-02: Accept small variance (< 2pp)
 */
TEST_F(DualReadValidatorTest, DUALREAD_02_AcceptSmallVariance) {
  RetrievalMetrics current;
  current.recall_at_10 = 0.84;  // 84% recall
  current.ndcg_at_10 = 0.75;
  current.mrr_at_10 = 0.90;

  RetrievalMetrics previous;
  previous.recall_at_10 = 0.85;  // 85% recall (1pp better)
  previous.ndcg_at_10 = 0.76;
  previous.mrr_at_10 = 0.91;

  // Delta is 1pp < 2pp tolerance → no regression
  EXPECT_FALSE(current.HasRegression(previous, 2.0));
}

/**
 * @test DUALREAD-03: Compute recall@10 correctly
 */
TEST_F(DualReadValidatorTest, DUALREAD_03_ComputeRecall) {
  // Results: doc1, doc2, doc3, ... (in top 10)
  auto results = CreateResults({
      {"doc1", 0.95},
      {"doc2", 0.90},
      {"doc3", 0.85},
      {"doc4", 0.80},
      {"doc5", 0.75},
      {"doc6", 0.70},
      {"doc7", 0.65},
      {"doc8", 0.60},
      {"doc9", 0.55},
      {"doc10", 0.50},
  });

  // Ground truth: doc1, doc3, doc5, doc7 (4 relevant)
  std::vector<std::string> ground_truth = {"doc1", "doc3", "doc5", "doc7"};

  // Recall@10 = 4 relevant found / 4 total relevant = 1.0
  double recall = DualReadValidator::ComputeRecallAtK(results, ground_truth, 10);
  EXPECT_DOUBLE_EQ(recall, 1.0);

  // If only top-5 results considered: 3 relevant found / 4 total = 0.75
  double recall_at_5 = DualReadValidator::ComputeRecallAtK(results, ground_truth, 5);
  EXPECT_DOUBLE_EQ(recall_at_5, 0.75);
}

/**
 * @test DUALREAD-04: Compute nDCG@10 correctly (normalized discount)
 */
TEST_F(DualReadValidatorTest, DUALREAD_04_ComputeNDCG) {
  // Results with perfect ranking (all relevant at top)
  auto perfect_results = CreateResults({
      {"doc1", 0.99},  // relevant
      {"doc2", 0.98},  // relevant
      {"doc3", 0.97},  // relevant
      {"irrelevant1", 0.50},
      {"irrelevant2", 0.40},
  });

  std::vector<std::string> ground_truth = {"doc1", "doc2", "doc3"};

  // Perfect ranking → nDCG should be close to 1.0
  double ndcg_perfect = DualReadValidator::ComputeNdcgAtK(perfect_results, ground_truth, 10);
  EXPECT_GT(ndcg_perfect, 0.95);  // Should be very high for perfect ranking
}

/**
 * @test DUALREAD-05: Compute MRR@10 (reciprocal rank of first relevant)
 */
TEST_F(DualReadValidatorTest, DUALREAD_05_ComputeMRR) {
  // Relevant doc at position 1 → MRR = 1/1 = 1.0
  auto results_rank1 = CreateResults({
      {"relevant_doc", 0.99},
      {"irrelevant1", 0.50},
  });
  std::vector<std::string> ground_truth = {"relevant_doc"};
  double mrr1 = DualReadValidator::ComputeMrrAtK(results_rank1, ground_truth, 10);
  EXPECT_DOUBLE_EQ(mrr1, 1.0);

  // Relevant doc at position 3 → MRR = 1/3 ≈ 0.333
  auto results_rank3 = CreateResults({
      {"irrel1", 0.90},
      {"irrel2", 0.80},
      {"relevant_doc", 0.70},
  });
  double mrr3 = DualReadValidator::ComputeMrrAtK(results_rank3, ground_truth, 10);
  EXPECT_NEAR(mrr3, 1.0 / 3.0, 0.001);

  // No relevant results → MRR = 0.0
  auto results_no_relevant = CreateResults({
      {"irrel1", 0.90},
      {"irrel2", 0.80},
  });
  double mrr_none = DualReadValidator::ComputeMrrAtK(results_no_relevant, ground_truth, 10);
  EXPECT_DOUBLE_EQ(mrr_none, 0.0);
}

/**
 * @test DUALREAD-06: Compute metric deltas correctly
 */
TEST_F(DualReadValidatorTest, DUALREAD_06_ComputeDeltas) {
  RetrievalMetrics current;
  current.recall_at_10 = 0.80;
  current.ndcg_at_10 = 0.75;
  current.mrr_at_10 = 0.90;

  RetrievalMetrics previous;
  previous.recall_at_10 = 0.85;
  previous.ndcg_at_10 = 0.78;
  previous.mrr_at_10 = 0.92;

  auto deltas = current.ComputeDeltas(previous);
  EXPECT_EQ(deltas.size(), 3);
  EXPECT_NEAR(deltas[0], 0.05, 0.001);   // recall delta
  EXPECT_NEAR(deltas[1], 0.03, 0.001);   // ndcg delta
  EXPECT_NEAR(deltas[2], 0.02, 0.001);   // mrr delta
}

/**
 * @test Set and retrieve ground truth
 */
TEST_F(DualReadValidatorTest, GroundTruthCaching) {
  std::map<std::string, std::vector<std::string>> judgments;
  judgments["query1"] = {"doc1", "doc3", "doc5"};
  judgments["query2"] = {"doc2", "doc4"};

  validator_.SetGroundTruth(judgments);

  auto gt1 = validator_.GetGroundTruth("query1");
  EXPECT_EQ(gt1.size(), 3);
  EXPECT_EQ(gt1[0], "doc1");

  auto gt2 = validator_.GetGroundTruth("query2");
  EXPECT_EQ(gt2.size(), 2);
}

/**
 * @test Recall edge case: empty ground truth
 */
TEST_F(DualReadValidatorTest, RecallEmptyGroundTruth) {
  auto results = CreateResults({{"doc1", 0.9}, {"doc2", 0.8}});
  std::vector<std::string> empty_ground_truth;

  double recall = DualReadValidator::ComputeRecallAtK(results, empty_ground_truth, 10);
  EXPECT_DOUBLE_EQ(recall, 0.0);  // No ground truth → no recall possible
}

/**
 * @test nDCG edge case: empty results
 */
TEST_F(DualReadValidatorTest, NDCGEmptyResults) {
  std::vector<RetrievalResult> empty_results;
  std::vector<std::string> ground_truth = {"doc1", "doc2"};

  double ndcg = DualReadValidator::ComputeNdcgAtK(empty_results, ground_truth, 10);
  EXPECT_DOUBLE_EQ(ndcg, 0.0);
}

/**
 * @test Regression threshold at exact 2pp boundary
 */
TEST_F(DualReadValidatorTest, RegressionBoundary) {
  RetrievalMetrics current;
  current.recall_at_10 = 0.82;  // 82%
  current.ndcg_at_10 = 0.75;
  current.mrr_at_10 = 0.90;

  RetrievalMetrics previous;
  previous.recall_at_10 = 0.84;  // 84% (exactly 2pp better)
  previous.ndcg_at_10 = 0.77;
  previous.mrr_at_10 = 0.92;

  // Exactly at boundary: 2pp delta → should NOT be flagged as regression
  // (tolerance is >= not >)
  EXPECT_FALSE(current.HasRegression(previous, 2.0));

  // Slightly more than 2pp
  current.recall_at_10 = 0.819;  // 81.9%
  // Now delta is 84 - 81.9 = 2.1pp > 2pp → regression
  EXPECT_TRUE(current.HasRegression(previous, 2.0));
}
