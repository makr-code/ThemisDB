// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include "rag/benchmark_suite.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>

namespace themis::rag {

BenchmarkSuite::BenchmarkSuite(const std::string& dataset_name)
    : dataset_name_(dataset_name) {
  config_.dataset_name = dataset_name;
}

bool BenchmarkSuite::LoadDataset(const std::string& file_path) {
  // TODO: Parse TREC/MARCO format TSV or JSON
  config_.source_path = file_path;
  config_.num_queries = 100;   // Placeholder
  config_.num_documents = 1000;  // Placeholder
  return true;
}

void BenchmarkSuite::LoadCustomQueries(const std::vector<BenchmarkQuery>& queries) {
  queries_ = queries;
  config_.num_queries = queries.size();
}

BenchmarkSuite::DatasetConfig BenchmarkSuite::GetDatasetConfig() {
  return config_;
}

std::vector<BenchmarkSuite::ExecutionResult> BenchmarkSuite::Run(
    const std::string& scenario,
    const std::string& mode) {
  std::vector<ExecutionResult> results;

  for (const auto& query : queries_) {
    auto result = RunQuery(query.query_id, scenario);
    if (result) {
      results.push_back(result.value());
    }
  }

  return results;
}

std::optional<BenchmarkSuite::ExecutionResult> BenchmarkSuite::RunQuery(
    const std::string& query_id,
    const std::string& scenario) {
  // TODO: Execute RAG system with scenario config, retrieve and rerank
  ExecutionResult result;
  result.query_id = query_id;
  result.execution_scenario = scenario;
  result.query_latency_ms = 100;
  result.query_cost_usd = 0.005f;
  result.executed_at_us = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch())
      .count();

  // Placeholder: return empty results
  return result;
}

std::vector<BenchmarkSuite::BenchmarkQuery> BenchmarkSuite::GetQueries() {
  return queries_;
}

std::vector<BenchmarkSuite::BenchmarkQuery> BenchmarkSuite::GetSample(uint32_t size) {
  std::vector<BenchmarkQuery> sample;
  if (queries_.size() <= size) {
    return queries_;
  }

  // Random sampling
  size_t step = queries_.size() / size;
  for (size_t i = 0; i < queries_.size(); i += step) {
    if (sample.size() < size) {
      sample.push_back(queries_[i]);
    }
  }

  return sample;
}

void BenchmarkSuite::StoreResults(
    const std::string& scenario,
    const std::vector<ExecutionResult>& results) {
  scenario_results_[scenario] = results;
}

std::vector<BenchmarkSuite::ExecutionResult> BenchmarkSuite::GetResults(
    const std::string& scenario) {
  auto it = scenario_results_.find(scenario);
  if (it != scenario_results_.end()) {
    return it->second;
  }
  return {};
}

bool BenchmarkSuite::ExportResults(const std::string& output_path) {
  // TODO: Export to JSON or CSV
  return true;
}

uint32_t BenchmarkSuite::GetQueryCount() {
  return queries_.size();
}

std::optional<BenchmarkSuite::BenchmarkQuery> BenchmarkSuite::GetQuery(
    const std::string& query_id) {
  for (const auto& query : queries_) {
    if (query.query_id == query_id) {
      return query;
    }
  }
  return std::nullopt;
}

}  // namespace themis::rag
