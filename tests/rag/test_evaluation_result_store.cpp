// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <chrono>

#include "rag/evaluation_result_store.h"

namespace themis::rag::testing {

class EvaluationResultStoreTest : public ::testing::Test {
 protected:
  EvaluationResultStore store_;

  static EvaluationResultStore::BenchmarkRun MakeRun(const std::string& run_id,
                                                     const std::string& version,
                                                     float ndcg10,
                                                     float latency_ms) {
    EvaluationResultStore::BenchmarkRun run;
    run.run_id = run_id;
    run.dataset_name = "trec_covid";
    run.scenario_name = "adaptive_rag";
    run.version = version;
    run.created_at_us = std::chrono::duration_cast<std::chrono::microseconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();
    run.mean_ndcg_10 = ndcg10;
    run.mean_ndcg_100 = ndcg10;
    run.mean_mrr_10 = 0.7f;
    run.mean_map_10 = 0.6f;
    run.mean_latency_ms = latency_ms;
    run.mean_cost_usd = 0.01f;

    EvaluationResultStore::QueryResult qr;
    qr.query_id = "q1";
    qr.scenario = "adaptive_rag";
    qr.ndcg_10 = ndcg10;
    qr.ndcg_100 = ndcg10;
    qr.mrr_10 = 0.7f;
    qr.map_10 = 0.6f;
    qr.precision_10 = 0.5f;
    qr.recall_10 = 0.5f;
    qr.query_latency_ms = static_cast<uint64_t>(latency_ms);
    qr.query_cost_usd = 0.01f;
    qr.computed_at_us = run.created_at_us;
    run.query_results.push_back(qr);
    return run;
  }
};

TEST_F(EvaluationResultStoreTest, StoreAndLoadRun) {
  auto run = MakeRun("run_1", "v1", 0.85f, 100.0f);
  const auto run_id = store_.StoreRun(run);
  EXPECT_EQ(run_id, "run_1");

  auto loaded = store_.LoadResults("trec_covid", "adaptive_rag", "v1");
  ASSERT_TRUE(loaded.has_value());
  EXPECT_EQ(loaded->run_id, "run_1");
}

TEST_F(EvaluationResultStoreTest, GetVersionsNewestFirst) {
  store_.StoreRun(MakeRun("run_1", "v1", 0.80f, 120.0f));
  store_.StoreRun(MakeRun("run_2", "v2", 0.85f, 110.0f));

  auto versions = store_.GetVersions("trec_covid", "adaptive_rag");
  ASSERT_EQ(versions.size(), 2u);
  EXPECT_EQ(versions[0], "v2");
  EXPECT_EQ(versions[1], "v1");
}

TEST_F(EvaluationResultStoreTest, CompareResults) {
  store_.StoreRun(MakeRun("run_1", "v1", 0.80f, 120.0f));
  store_.StoreRun(MakeRun("run_2", "v2", 0.90f, 100.0f));

  auto comparison = store_.Compare("run_1", "run_2");
  EXPECT_GT(comparison.mean_ndcg_10_delta, 0.0f);
}

TEST_F(EvaluationResultStoreTest, GetTrend) {
  store_.StoreRun(MakeRun("run_1", "v1", 0.80f, 120.0f));
  store_.StoreRun(MakeRun("run_2", "v2", 0.85f, 110.0f));

  auto trend = store_.GetTrend("trec_covid", "adaptive_rag", "ndcg_10");
  EXPECT_EQ(trend.metric_name, "ndcg_10");
  EXPECT_GE(trend.values.size(), 2u);
}

TEST_F(EvaluationResultStoreTest, ExportRun) {
  store_.StoreRun(MakeRun("run_1", "v1", 0.85f, 100.0f));
  const std::string path = "/tmp/themis_rag_eval_run.json";
  EXPECT_TRUE(store_.ExportRun("run_1", path));

  std::ifstream input(path);
  ASSERT_TRUE(input.is_open());
  std::string contents((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
  EXPECT_NE(contents.find("\"schema_version\""), std::string::npos);
  EXPECT_NE(contents.find("\"provenance\""), std::string::npos);
  EXPECT_NE(contents.find("\"run_id\""), std::string::npos);
}

TEST_F(EvaluationResultStoreTest, GenerateJsonReportIncludesStructuredPayload) {
  auto baseline = MakeRun("baseline", "v1", 0.80f, 120.0f);
  auto candidate = MakeRun("candidate", "v2", 0.90f, 100.0f);
  store_.StoreRun(baseline);
  store_.StoreRun(candidate);

  const std::string report = store_.GenerateReport("baseline", "candidate", "json");
  EXPECT_NE(report.find("\"baseline_run_id\""), std::string::npos);
  EXPECT_NE(report.find("\"candidate_run_id\""), std::string::npos);
  EXPECT_NE(report.find("\"verdict\""), std::string::npos);
  EXPECT_NE(report.find("\"mean_ndcg_10_delta\""), std::string::npos);
}

TEST_F(EvaluationResultStoreTest, GetRunStats) {
  store_.StoreRun(MakeRun("run_1", "v1", 0.85f, 100.0f));
  auto stats = store_.GetRunStats("run_1");
  EXPECT_GT(stats["mean_ndcg_10"], 0.0f);
  EXPECT_EQ(stats["num_queries"], 1.0f);
}

TEST_F(EvaluationResultStoreTest, PruneOldRunsKeepsRecent) {
  store_.StoreRun(MakeRun("run_1", "v1", 0.85f, 100.0f));
  EXPECT_EQ(store_.PruneOldRuns(3650), 0u);
}

}  // namespace themis::rag::testing
