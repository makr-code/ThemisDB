// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "rag/evaluation_result_store.h"

namespace themis::rag::testing {

class EvaluationResultStoreTest : public ::testing::Test {
 protected:
  EvaluationResultStore store_;
};

TEST_F(EvaluationResultStoreTest, StoreResult) {
  EvaluationResultStore::Result result;
  result.result_id = "r1";
  result.scenario_name = "benchmark_v1";
  result.retriever_name = "retriever_v1";
  result.ndcg_10 = 0.85f;
  
  bool success = store_.StoreResult(result);
  EXPECT_TRUE(success);
}

TEST_F(EvaluationResultStoreTest, RetrieveResult) {
  EvaluationResultStore::Result result;
  result.result_id = "r1";
  result.scenario_name = "benchmark_v1";
  result.retriever_name = "retriever_v1";
  result.ndcg_10 = 0.85f;
  
  store_.StoreResult(result);
  auto retrieved = store_.GetResult("r1");
  
  EXPECT_TRUE(retrieved.has_value());
  EXPECT_EQ(retrieved->result_id, "r1");
}

TEST_F(EvaluationResultStoreTest, ListResultsForScenario) {
  EvaluationResultStore::Result result1, result2;
  result1.result_id = "r1";
  result1.scenario_name = "benchmark_v1";
  result1.retriever_name = "retriever_v1";
  result1.ndcg_10 = 0.85f;
  
  result2.result_id = "r2";
  result2.scenario_name = "benchmark_v1";
  result2.retriever_name = "retriever_v2";
  result2.ndcg_10 = 0.90f;
  
  store_.StoreResult(result1);
  store_.StoreResult(result2);
  
  auto results = store_.GetResultsForScenario("benchmark_v1");
  EXPECT_EQ(results.size(), 2);
}

TEST_F(EvaluationResultStoreTest, CompareResults) {
  EvaluationResultStore::Result result1, result2;
  result1.result_id = "r1";
  result1.scenario_name = "benchmark_v1";
  result1.retriever_name = "retriever_v1";
  result1.ndcg_10 = 0.85f;
  
  result2.result_id = "r2";
  result2.scenario_name = "benchmark_v1";
  result2.retriever_name = "retriever_v2";
  result2.ndcg_10 = 0.90f;
  
  store_.StoreResult(result1);
  store_.StoreResult(result2);
  
  auto comparison = store_.CompareResults("r1", "r2");
  EXPECT_GT(comparison["r2_ndcg_10_improvement"], 0.0f);
}

TEST_F(EvaluationResultStoreTest, GetTrends) {
  EvaluationResultStore::Result result1, result2, result3;
  result1.result_id = "r1";
  result1.scenario_name = "benchmark_v1";
  result1.retriever_name = "retriever_v1";
  result1.ndcg_10 = 0.80f;
  
  result2.result_id = "r2";
  result2.scenario_name = "benchmark_v1";
  result2.retriever_name = "retriever_v1";
  result2.ndcg_10 = 0.85f;
  
  result3.result_id = "r3";
  result3.scenario_name = "benchmark_v1";
  result3.retriever_name = "retriever_v1";
  result3.ndcg_10 = 0.90f;
  
  store_.StoreResult(result1);
  store_.StoreResult(result2);
  store_.StoreResult(result3);
  
  auto trends = store_.GetTrends("retriever_v1", "benchmark_v1");
  EXPECT_GE(trends.size(), 0);
}

TEST_F(EvaluationResultStoreTest, DeleteResult) {
  EvaluationResultStore::Result result;
  result.result_id = "r1";
  result.scenario_name = "benchmark_v1";
  result.retriever_name = "retriever_v1";
  result.ndcg_10 = 0.85f;
  
  store_.StoreResult(result);
  bool success = store_.DeleteResult("r1");
  EXPECT_TRUE(success);
  
  auto retrieved = store_.GetResult("r1");
  EXPECT_FALSE(retrieved.has_value());
}

TEST_F(EvaluationResultStoreTest, MultipleRetrievers) {
  EvaluationResultStore::Result result1, result2;
  result1.result_id = "r1";
  result1.scenario_name = "benchmark_v1";
  result1.retriever_name = "retriever_v1";
  result1.ndcg_10 = 0.85f;
  
  result2.result_id = "r2";
  result2.scenario_name = "benchmark_v1";
  result2.retriever_name = "retriever_v2";
  result2.ndcg_10 = 0.90f;
  
  store_.StoreResult(result1);
  store_.StoreResult(result2);
  
  auto results = store_.GetResultsForScenario("benchmark_v1");
  EXPECT_EQ(results.size(), 2);
}

TEST_F(EvaluationResultStoreTest, NonexistentResult) {
  auto retrieved = store_.GetResult("nonexistent");
  EXPECT_FALSE(retrieved.has_value());
}

}  // namespace themis::rag::testing
