// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <map>
#include <vector>

#include "rag/benchmark_suite.h"

namespace themis::rag::testing {

class BenchmarkSuiteTest : public ::testing::Test {
 protected:
  BenchmarkSuite suite_{"test_dataset"};

  static BenchmarkSuite::BenchmarkQuery MakeQuery(const std::string& id,
                                                  const std::string& text) {
    BenchmarkSuite::BenchmarkQuery q;
    q.query_id = id;
    q.query_text = text;
    q.ground_truth_doc_ids = {"doc_1"};
    q.relevance_judgments = {{"doc_1", 3}, {"doc_2", 1}};
    q.query_domain = "unit-test";
    return q;
  }
};

TEST_F(BenchmarkSuiteTest, LoadDataset) {
  bool success = suite_.LoadDataset("datasets/test_dataset.tsv");
  EXPECT_TRUE(success);

  auto cfg = suite_.GetDatasetConfig();
  EXPECT_EQ(cfg.dataset_name, "test_dataset");
  EXPECT_EQ(cfg.source_path, "datasets/test_dataset.tsv");
}

TEST_F(BenchmarkSuiteTest, LoadCustomQueriesUpdatesCount) {
  std::vector<BenchmarkSuite::BenchmarkQuery> queries = {
      MakeQuery("q1", "test query 1"), MakeQuery("q2", "test query 2")};

  suite_.LoadCustomQueries(queries);
  EXPECT_EQ(suite_.GetQueryCount(), 2u);
}

TEST_F(BenchmarkSuiteTest, RunQuery) {
  suite_.LoadCustomQueries({MakeQuery("q1", "test query")});

  auto result = suite_.RunQuery("q1", "baseline");
  EXPECT_TRUE(result.has_value());
  EXPECT_GT(result->executed_at_us, 0);
}

TEST_F(BenchmarkSuiteTest, GetQueryById) {
  suite_.LoadCustomQueries({MakeQuery("q1", "test")});
  auto metadata = suite_.GetQuery("q1");
  ASSERT_TRUE(metadata.has_value());
  EXPECT_EQ(metadata->query_id, "q1");
}

TEST_F(BenchmarkSuiteTest, StoreAndGetResultsByScenario) {
  suite_.LoadCustomQueries({MakeQuery("q1", "test")});
  auto result1 = suite_.RunQuery("q1", "baseline");
  auto result2 = suite_.RunQuery("q1", "adaptive_rag");

  EXPECT_TRUE(result1.has_value());
  EXPECT_TRUE(result2.has_value());

  suite_.StoreResults("baseline", {*result1});
  suite_.StoreResults("adaptive_rag", {*result2});

  auto stored = suite_.GetResults("baseline");
  ASSERT_EQ(stored.size(), 1u);
  EXPECT_EQ(stored.front().query_id, "q1");
}

TEST_F(BenchmarkSuiteTest, ExportResults) {
  bool export_ok = suite_.ExportResults("results.json");
  EXPECT_TRUE(export_ok);
}

TEST_F(BenchmarkSuiteTest, EmptyDatasetHandling) {
  bool success = suite_.LoadDataset("datasets/empty.tsv");
  EXPECT_TRUE(success);
}

TEST_F(BenchmarkSuiteTest, QueryExecutionTime) {
  suite_.LoadCustomQueries({MakeQuery("q1", "test")});
  auto result = suite_.RunQuery("q1", "baseline");
  EXPECT_TRUE(result.has_value());
  EXPECT_GT(result->query_latency_ms, 0u);
}

TEST_F(BenchmarkSuiteTest, RetrievedDocuments) {
  suite_.LoadCustomQueries({MakeQuery("q1", "test")});
  auto result = suite_.RunQuery("q1", "baseline");
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->retrieved_docs.empty());
}

}  // namespace themis::rag::testing
