// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "rag/common_types.h"

namespace themis::rag {

/// @brief Benchmark suite for RAG evaluation.
///
/// Manages benchmark datasets, scenarios, and execution workflows. Supports
/// multiple dataset types (TREC, MS MARCO, custom) with configurable relevance
/// assessments and evaluation metrics.
///
/// @details
/// Dataset types:
/// - TREC: Query + relevance judgments
/// - MS MARCO: Query + BM25 baseline + relevance
/// - Custom: User-provided queries with ground truth
///
/// Benchmark scenarios:
/// - "baseline": BM25 retrieval only
/// - "simple_rag": Retrieval + LLM reranking
/// - "adaptive_rag": Adaptive routing + reranking
/// - "production": Live configuration
///
/// Execution modes:
/// - "offline": Run on historical dataset
/// - "canary": Run on 1% of live traffic
/// - "shadow": Run in parallel with production
///
/// Evaluation workflow:
/// 1. Load dataset or stream (query, ground truth relevance)
/// 2. Execute RAG system (retrieve, rerank)
/// 3. Compute metrics (NDCG@@K, MRR@@K, MAP@@K)
/// 4. Compare vs baseline
/// 5. Report regressions and wins
///
/// @code
/// auto suite = std::make_unique<BenchmarkSuite>("trec_covid");
/// suite->LoadDataset("datasets/trec_covid.tsv");
/// auto results = suite->Run("adaptive_rag", "offline");
/// auto summary = suite->CompareToBass("baseline");
/// suite->ExportResults("evaluation_report.json");
/// @endcode
class BenchmarkSuite {
 public:
  /// @brief Benchmark query + ground truth.
  struct BenchmarkQuery {
    std::string query_id;
    std::string query_text;
    std::vector<std::string> ground_truth_doc_ids;  ///< Relevant docs
    std::map<std::string, uint32_t> relevance_judgments;  ///< doc_id → grade
    std::string query_domain;                            ///< Domain hint
  };

  /// @brief Benchmark execution result.
  struct ExecutionResult {
    std::string query_id;
    std::vector<Document> retrieved_docs;    ///< Ranked results
    std::vector<float> retrieval_scores;     ///< BM25 scores
    std::vector<float> rerank_scores;        ///< Reranker scores
    uint64_t query_latency_ms;
    float query_cost_usd;
    std::string execution_scenario;
    int64_t executed_at_us;
  };

  /// @brief Dataset configuration.
  struct DatasetConfig {
    std::string dataset_name;
    std::string dataset_type;  ///< "trec" | "marco" | "custom"
    uint32_t num_queries;
    uint32_t num_documents;
    std::string source_path;
    std::map<std::string, std::string> metadata;
  };

  /// @brief Constructor.
  /// @param dataset_name Dataset identifier.
  explicit BenchmarkSuite(const std::string& dataset_name);

  /// @brief Load dataset from file.
  /// @param file_path Path to dataset (TSV or JSON).
  /// @return true if loaded successfully.
  bool LoadDataset(const std::string& file_path);

  /// @brief Load custom queries.
  /// @param queries Vector of benchmark queries.
  void LoadCustomQueries(const std::vector<BenchmarkQuery>& queries);

  /// @brief Get dataset info.
  /// @return Dataset configuration.
  DatasetConfig GetDatasetConfig();

  /// @brief Run benchmark scenario.
  /// @param scenario Scenario name ("baseline", "simple_rag", "adaptive_rag", etc).
  /// @param mode Execution mode ("offline", "canary", "shadow").
  /// @return List of execution results.
  std::vector<ExecutionResult> Run(
      const std::string& scenario,
      const std::string& mode = "offline");

  /// @brief Run single query.
  /// @param query_id Query identifier.
  /// @param scenario Scenario name.
  /// @return Execution result.
  std::optional<ExecutionResult> RunQuery(
      const std::string& query_id,
      const std::string& scenario);

  /// @brief Get all queries.
  /// @return Vector of benchmark queries.
  std::vector<BenchmarkQuery> GetQueries();

  /// @brief Get sample of queries.
  /// @param size Number of queries to sample.
  /// @return Random sample of queries.
  std::vector<BenchmarkQuery> GetSample(uint32_t size);

  /// @brief Store results for scenario.
  /// @param scenario Scenario name.
  /// @param results Execution results.
  void StoreResults(
      const std::string& scenario,
      const std::vector<ExecutionResult>& results);

  /// @brief Get stored results.
  /// @param scenario Scenario name.
  /// @return Stored results (empty if not run).
  std::vector<ExecutionResult> GetResults(const std::string& scenario);

  /// @brief Export results to file.
  /// @param output_path Output file path (JSON or CSV).
  /// @return true if exported successfully.
  bool ExportResults(const std::string& output_path);

  /// @brief Get query count.
  /// @return Number of queries in dataset.
  uint32_t GetQueryCount();

  /// @brief Get query by ID.
  /// @param query_id Query identifier.
  /// @return Query or nullopt if not found.
  std::optional<BenchmarkQuery> GetQuery(const std::string& query_id);

 private:
  std::string dataset_name_;
  DatasetConfig config_;
  std::vector<BenchmarkQuery> queries_;
  std::map<std::string, std::vector<ExecutionResult>> scenario_results_;
};

}  // namespace themis::rag
