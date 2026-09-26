// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "rag/benchmark_suite.h"

namespace themis::rag::testing {

class BenchmarkSuiteTest : public ::testing::Test {
 protected:
  BenchmarkSuite suite_;
};

TEST_F(BenchmarkSuiteTest, LoadDataset) {
  bool success = suite_.LoadDataset(
      "test_dataset",
      {{"doc_1", "content 1"}, {"doc_2", "content 2"}});
  EXPECT_TRUE(success);
}

TEST_F(BenchmarkSuiteTest, RegisterQuery) {
  suite_.LoadDataset("test_dataset", {{"doc_1", "content 1"}});
  
  bool success = suite_.RegisterQuery("q1", "test query", {"doc_1"});
  EXPECT_TRUE(success);
}

TEST_F(BenchmarkSuiteTest, RunQuery) {
  suite_.LoadDataset("test_dataset", {{"doc_1", "content 1"}, {"doc_2", "content 2"}});
  suite_.RegisterQuery("q1", "test query", {"doc_1"});
  
  auto result = suite_.RunQuery("q1", "retriever_v1");
  EXPECT_TRUE(result.has_value());
  EXPECT_GT(result->executed_at_us, 0);
}

TEST_F(BenchmarkSuiteTest, QueryWithMultipleRelevantDocs) {
  suite_.LoadDataset("test_dataset", {
      {"doc_1", "content 1"},
      {"doc_2", "content 2"},
      {"doc_3", "content 3"}
  });
  
  suite_.RegisterQuery("q1", "test query", {"doc_1", "doc_3"});
  
  auto result = suite_.RunQuery("q1", "retriever_v1");
  EXPECT_TRUE(result.has_value());
}

TEST_F(BenchmarkSuiteTest, GetScenarios) {
  suite_.LoadDataset("test_scenario", {{"doc_1", "content"}});
  
  auto scenarios = suite_.GetScenarios();
  EXPECT_GE(scenarios.size(), 1);
}

TEST_F(BenchmarkSuiteTest, GetQueryMetadata) {
  suite_.LoadDataset("test_dataset", {{"doc_1", "content"}});
  suite_.RegisterQuery("q1", "test", {"doc_1"});
  
  auto metadata = suite_.GetQueryMetadata("q1");
  EXPECT_EQ(metadata.query_id, "q1");
}

TEST_F(BenchmarkSuiteTest, MultipleRetrievers) {
  suite_.LoadDataset("test_dataset", {{"doc_1", "content"}});
  suite_.RegisterQuery("q1", "test", {"doc_1"});
  
  auto result1 = suite_.RunQuery("q1", "retriever_v1");
  auto result2 = suite_.RunQuery("q1", "retriever_v2");
  
  EXPECT_TRUE(result1.has_value());
  EXPECT_TRUE(result2.has_value());
}

TEST_F(BenchmarkSuiteTest, ExportResults) {
  suite_.LoadDataset("test_dataset", {{"doc_1", "content"}});
  suite_.RegisterQuery("q1", "test", {"doc_1"});
  
  auto result = suite_.RunQuery("q1", "retriever_v1");
  bool export_ok = suite_.ExportResults("test_scenario", "/tmp/results.json");
  
  EXPECT_TRUE(export_ok);
}

TEST_F(BenchmarkSuiteTest, EmptyDatasetHandling) {
  bool success = suite_.LoadDataset("empty", {});
  EXPECT_TRUE(success);
}

TEST_F(BenchmarkSuiteTest, QueryExecutionTime) {
  suite_.LoadDataset("test_dataset", {{"doc_1", "content"}});
  suite_.RegisterQuery("q1", "test", {"doc_1"});
  
  auto result = suite_.RunQuery("q1", "retriever_v1");
  EXPECT_TRUE(result.has_value());
  EXPECT_GT(result->executed_at_us, 0);
}

TEST_F(BenchmarkSuiteTest, RetrievedDocuments) {
  suite_.LoadDataset("test_dataset", {{"doc_1", "content"}});
  suite_.RegisterQuery("q1", "test", {"doc_1"});
  
  auto result = suite_.RunQuery("q1", "retriever_v1");
  EXPECT_TRUE(result.has_value());
  EXPECT_GT(result->retrieved_docs.size(), 0);
}

}  // namespace themis::rag::testing
