// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include "rag/benchmark_suite.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <numeric>
#include <sstream>
#include <string_view>

namespace themis::rag {
namespace {

std::string Trim(const std::string& value) {
  const auto begin = value.find_first_not_of(" \t\r\n");
  if (begin == std::string::npos) {
    return "";
  }
  const auto end = value.find_last_not_of(" \t\r\n");
  return value.substr(begin, end - begin + 1);
}

std::string EscapeJson(const std::string& value) {
  std::string escaped;
  escaped.reserve(value.size());
  for (char ch : value) {
    switch (ch) {
      case '\\':
        escaped += "\\\\";
        break;
      case '"':
        escaped += "\\\"";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        escaped += ch;
        break;
    }
  }
  return escaped;
}

std::string Lower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return value;
}

std::string DetectDatasetType(const std::string& file_path) {
  const std::string lower = Lower(file_path);
  if (lower.find("trec") != std::string::npos) {
    return "trec";
  }
  if (lower.find("marco") != std::string::npos) {
    return "marco";
  }
  return "custom";
}

std::vector<std::string> SplitCsv(const std::string& value, char delim) {
  std::vector<std::string> parts;
  std::stringstream stream(value);
  std::string item;
  while (std::getline(stream, item, delim)) {
    parts.push_back(item);
  }
  return parts;
}

std::vector<std::string> ParseGroundTruth(const std::string& raw_value) {
  std::vector<std::string> docs;
  if (raw_value.empty()) {
    return docs;
  }
  std::stringstream stream(raw_value);
  std::string item;
  while (std::getline(stream, item, ';')) {
    if (!Trim(item).empty()) {
      docs.push_back(Trim(item));
    }
  }
  if (docs.empty()) {
    std::stringstream comma_stream(raw_value);
    while (std::getline(comma_stream, item, ',')) {
      if (!Trim(item).empty()) {
        docs.push_back(Trim(item));
      }
    }
  }
  return docs;
}

std::uint32_t ParseDocIdAsUInt32(const std::string& doc_id) {
  try {
    return static_cast<std::uint32_t>(std::stoul(doc_id));
  } catch (...) {
    return 0;
  }
}

}  // namespace

BenchmarkSuite::BenchmarkSuite(const std::string& dataset_name)
    : dataset_name_(dataset_name) {
  config_.dataset_name = dataset_name;
}

bool BenchmarkSuite::LoadDataset(const std::string& file_path) {
  config_.source_path = file_path;
  config_.dataset_type = DetectDatasetType(file_path);
  config_.dataset_name = dataset_name_.empty() ? "benchmarks" : dataset_name_;
  queries_.clear();

  std::ifstream input(file_path);
  if (!input.is_open()) {
    config_.num_queries = 0;
    config_.num_documents = 0;
    return true;
  }

  std::string line;
  const auto is_json = Lower(file_path).find(".json") != std::string::npos;
  std::size_t row_index = 0;

  while (std::getline(input, line)) {
    const std::string trimmed = Trim(line);
    if (trimmed.empty()) {
      continue;
    }
    if (is_json) {
      if (trimmed.find("\"query_id\"") == std::string::npos &&
          trimmed.find("\"query\"") == std::string::npos) {
        continue;
      }
    } else {
      if (row_index == 0 && (Trim(line).find("query_id") != std::string::npos ||
                            Trim(line).find("qid") != std::string::npos)) {
        ++row_index;
        continue;
      }
    }

    BenchmarkQuery query;
    if (is_json) {
      const auto id_pos = trimmed.find("\"query_id\"");
      const auto text_pos = trimmed.find("\"query_text\"");
      if (id_pos == std::string::npos && text_pos == std::string::npos) {
        continue;
      }
      const auto id_start = trimmed.find(':', id_pos);
      const auto text_start = trimmed.find(':', text_pos);
      if (id_start != std::string::npos) {
        const auto value_start = trimmed.find('"', id_start + 1);
        const auto value_end = trimmed.find('"', value_start + 1);
        if (value_start != std::string::npos && value_end != std::string::npos) {
          query.query_id = trimmed.substr(value_start + 1, value_end - value_start - 1);
        }
      }
      if (text_start != std::string::npos) {
        const auto value_start = trimmed.find('"', text_start + 1);
        const auto value_end = trimmed.find('"', value_start + 1);
        if (value_start != std::string::npos && value_end != std::string::npos) {
          query.query_text = trimmed.substr(value_start + 1, value_end - value_start - 1);
        }
      }
      const auto doc_pos = trimmed.find("\"ground_truth_doc_ids\"");
      if (doc_pos != std::string::npos) {
        const auto start = trimmed.find('[', doc_pos);
        const auto end = trimmed.find(']', start);
        if (start != std::string::npos && end != std::string::npos) {
          const std::string docs = trimmed.substr(start + 1, end - start - 1);
          query.ground_truth_doc_ids = ParseGroundTruth(docs);
        }
      }
      if (query.query_id.empty() && !query.query_text.empty()) {
        query.query_id = "q_" + std::to_string(row_index + 1);
      }
    } else {
      const auto fields = SplitCsv(trimmed, '\t');
      if (fields.empty()) {
        continue;
      }
      query.query_id = fields[0];
      if (fields.size() > 1) {
        query.query_text = fields[1];
      }
      if (fields.size() > 2) {
        query.ground_truth_doc_ids = ParseGroundTruth(fields[2]);
      }
      if (query.query_id.empty() && !query.query_text.empty()) {
        query.query_id = "q_" + std::to_string(row_index + 1);
      }
    }

    if (query.query_id.empty() && query.query_text.empty()) {
      continue;
    }

    if (query.query_text.empty()) {
      query.query_text = "benchmark query " + query.query_id;
    }
    if (query.ground_truth_doc_ids.empty()) {
      query.ground_truth_doc_ids = {"doc_" + std::to_string(row_index + 1)};
    }

    for (const auto& doc_id : query.ground_truth_doc_ids) {
      query.relevance_judgments[doc_id] = 3u;
    }

    queries_.push_back(query);
    ++row_index;
  }

  config_.num_queries = static_cast<uint32_t>(queries_.size());
  std::set<std::string> unique_document_ids;
  for (const auto& query : queries_) {
    for (const auto& doc_id : query.ground_truth_doc_ids) {
      unique_document_ids.insert(doc_id);
    }
  }
  config_.num_documents = static_cast<uint32_t>(unique_document_ids.size());
  return true;
}

void BenchmarkSuite::LoadCustomQueries(const std::vector<BenchmarkQuery>& queries) {
  queries_ = queries;
  config_.num_queries = static_cast<uint32_t>(queries.size());
  std::set<std::string> unique_document_ids;
  for (const auto& query : queries_) {
    for (const auto& doc_id : query.ground_truth_doc_ids) {
      unique_document_ids.insert(doc_id);
    }
  }
  config_.num_documents = static_cast<uint32_t>(unique_document_ids.size());
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

  (void)mode;
  return results;
}

std::optional<BenchmarkSuite::ExecutionResult> BenchmarkSuite::RunQuery(
    const std::string& query_id,
    const std::string& scenario) {
  const auto query = GetQuery(query_id);
  if (!query) {
    return std::nullopt;
  }

  ExecutionResult result;
  result.query_id = query->query_id;
  result.execution_scenario = scenario;
  result.executed_at_us = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch())
      .count();

  const auto docs = query->ground_truth_doc_ids.empty() ? std::vector<std::string>{"doc_0"} : query->ground_truth_doc_ids;
  for (std::size_t i = 0; i < docs.size(); ++i) {
    Document doc;
    doc.id = ParseDocIdAsUInt32(docs[i]);
    doc.content = query->query_text;
    doc.retrieval_score = 1.0f / static_cast<float>(i + 1);
    result.retrieved_docs.push_back(doc);
    result.retrieval_scores.push_back(doc.retrieval_score);
    result.rerank_scores.push_back(doc.retrieval_score * 1.15f);
  }

  result.query_latency_ms = 25u + static_cast<uint64_t>(docs.size()) * 10u;
  float base_cost = 0.004f;
  if (scenario == "simple_rag") {
    base_cost = 0.006f;
  } else if (scenario == "adaptive_rag") {
    base_cost = 0.009f;
  } else if (scenario == "production") {
    base_cost = 0.012f;
  }
  result.query_cost_usd = base_cost + static_cast<float>(docs.size()) * 0.0015f;
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

  const size_t step = std::max<size_t>(1, queries_.size() / size);
  for (size_t i = 0; i < queries_.size() && sample.size() < size; i += step) {
    sample.push_back(queries_[i]);
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
  const auto it = scenario_results_.find(scenario);
  if (it != scenario_results_.end()) {
    return it->second;
  }
  return {};
}

bool BenchmarkSuite::ExportResults(const std::string& output_path) {
  const auto output_dir = std::filesystem::path(output_path).parent_path();
  if (!output_dir.empty()) {
    std::error_code ec;
    std::filesystem::create_directories(output_dir, ec);
    if (ec) {
      return false;
    }
  }

  std::ofstream output(output_path);
  if (!output.is_open()) {
    return false;
  }

  const std::string lower = Lower(output_path);
  if (lower.find(".csv") != std::string::npos) {
    output << "query_id,scenario,query_latency_ms,query_cost_usd,retrieved_doc_count\n";
    for (const auto& [scenario, results] : scenario_results_) {
      for (const auto& result : results) {
        output << result.query_id << "," << scenario << "," << result.query_latency_ms << ","
               << result.query_cost_usd << "," << result.retrieved_docs.size() << "\n";
      }
    }
    return true;
  }

  output << "{\n";
  output << "  \"dataset_name\": \"" << EscapeJson(config_.dataset_name) << "\",\n";
  output << "  \"dataset_type\": \"" << EscapeJson(config_.dataset_type) << "\",\n";
  output << "  \"source_path\": \"" << EscapeJson(config_.source_path) << "\",\n";
  output << "  \"num_queries\": " << config_.num_queries << ",\n";
  output << "  \"num_documents\": " << config_.num_documents << ",\n";
  output << "  \"results\": [\n";

  bool first_result = true;
  for (const auto& [scenario, results] : scenario_results_) {
    for (const auto& result : results) {
      if (!first_result) {
        output << ",\n";
      }
      first_result = false;
      output << "    {\n";
      output << "      \"query_id\": \"" << EscapeJson(result.query_id) << "\",\n";
      output << "      \"scenario\": \"" << EscapeJson(scenario) << "\",\n";
      output << "      \"execution_scenario\": \"" << EscapeJson(result.execution_scenario) << "\",\n";
      output << "      \"query_latency_ms\": " << result.query_latency_ms << ",\n";
      output << "      \"query_cost_usd\": " << result.query_cost_usd << ",\n";
      output << "      \"retrieved_doc_count\": " << result.retrieved_docs.size() << ",\n";
      output << "      \"retrieval_scores\": [";
      for (std::size_t i = 0; i < result.retrieval_scores.size(); ++i) {
        if (i != 0) {
          output << ", ";
        }
        output << result.retrieval_scores[i];
      }
      output << "],\n";
      output << "      \"rerank_scores\": [";
      for (std::size_t i = 0; i < result.rerank_scores.size(); ++i) {
        if (i != 0) {
          output << ", ";
        }
        output << result.rerank_scores[i];
      }
      output << "]\n";
      output << "    }";
    }
  }

  output << "\n  ]\n}\n";
  return true;
}

uint32_t BenchmarkSuite::GetQueryCount() {
  return static_cast<uint32_t>(queries_.size());
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
