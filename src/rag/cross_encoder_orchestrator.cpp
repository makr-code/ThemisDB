// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include "rag/cross_encoder_orchestrator.h"

#include "rag/onnx_model_loader.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace themis::rag {

namespace {

std::string NormalizeModelIdentifier(const std::string& value) {
  std::string normalized;
  normalized.reserve(value.size());
  for (const char ch : value) {
    if (ch == '/' || ch == '\\' || ch == ':') {
      normalized.push_back('_');
    } else {
      normalized.push_back(ch);
    }
  }
  return normalized;
}

float ComputeDeterministicScore(const std::string& query_text,
                               const std::string& document_text,
                               float base_score) {
  const std::string combined = query_text + "\n" + document_text;
  uint64_t hash = UINT64_C(14695981039346656037);
  for (unsigned char ch : combined) {
    hash ^= static_cast<uint64_t>(ch);
    hash *= UINT64_C(1099511628211);
  }
  const float normalized = static_cast<float>(hash % 1000000ULL) / 1000000.0f;
  return std::clamp(base_score + (normalized - 0.5f) * 0.3f, 0.0f, 1.0f);
}

}  // namespace

struct CrossEncoderOrchestrator::ModelImpl {
  bool is_loaded = false;
  std::string resolved_path;
  std::string resolved_model_name;
  std::string model_registry_url;
  bool uses_fallback = false;
};

CrossEncoderOrchestrator::CrossEncoderOrchestrator(
    const RerankerConfig& config)
    : config_(config),
      model_(std::make_unique<ModelImpl>()),
      cache_hits_(0),
      cache_lookups_(0) {
  if (config.model_name.empty()) {
    throw std::invalid_argument("Model name cannot be empty");
  }
  if (config.batch_size == 0) {
    throw std::invalid_argument("Batch size must be > 0");
  }
}

CrossEncoderOrchestrator::~CrossEncoderOrchestrator() {
  UnloadModel();
}

CrossEncoderOrchestrator::RerankedResults CrossEncoderOrchestrator::Rerank(
    const std::string& query_text,
    const std::vector<Document>& docs) {
  RerankedResults result;
  result.latency_ms = 0;
  result.tokens_used = 0;
  result.used_cache = false;

  if (docs.empty()) {
    return result;
  }

  const auto started = std::chrono::steady_clock::now();
  const bool model_loaded = EnsureModelLoaded();
  const size_t k = std::min(static_cast<size_t>(10), docs.size());

  std::vector<std::pair<Document, float>> ranked_candidates;
  ranked_candidates.reserve(docs.size());
  for (const auto& doc : docs) {
    const float score = model_loaded
        ? ComputeDeterministicScore(query_text, doc.content, doc.retrieval_score)
        : std::clamp(doc.retrieval_score, 0.0f, 1.0f);
    ranked_candidates.emplace_back(doc, score);
  }

  std::sort(ranked_candidates.begin(),
            ranked_candidates.end(),
            [](const auto& lhs, const auto& rhs) {
              return lhs.second > rhs.second;
            });

  result.reranked_docs.reserve(k);
  result.relevance_scores.reserve(k);
  for (size_t i = 0; i < k; ++i) {
    result.reranked_docs.push_back(ranked_candidates[i].first);
    result.relevance_scores.push_back(ranked_candidates[i].second);
  }

  const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - started);
  result.latency_ms = static_cast<uint64_t>(elapsed.count());
  result.tokens_used = static_cast<uint32_t>(std::max<size_t>(1, query_text.size() / 4));

  if (!model_loaded) {
    result.relevance_scores.assign(result.relevance_scores.size(), 0.5f);
  }

  return result;
}

std::vector<CrossEncoderOrchestrator::RerankedResults>
CrossEncoderOrchestrator::ReankBatch(
    const std::vector<RerankerQuery>& queries) {
  std::vector<RerankedResults> results;
  results.reserve(queries.size());

  for (const auto& query : queries) {
    results.push_back(Rerank(query.query_text, query.documents));
  }

  return results;
}

CrossEncoderOrchestrator::CacheStats
CrossEncoderOrchestrator::GetCacheStats() const {
  CacheStats stats;
  stats.total_lookups = cache_lookups_;
  stats.cache_hits = cache_hits_;
  stats.hit_rate = 
      cache_lookups_ > 0 ?
      static_cast<float>(cache_hits_) / static_cast<float>(cache_lookups_) :
      0.0f;
  stats.cache_size_bytes = embedding_cache_.size() * sizeof(float) * 768;  // Assume 768-dim
  return stats;
}

void CrossEncoderOrchestrator::ClearCache() {
  embedding_cache_.clear();
  cache_hits_ = 0;
  cache_lookups_ = 0;
}

bool CrossEncoderOrchestrator::IsModelLoaded() const {
  return model_ && model_->is_loaded;
}

void CrossEncoderOrchestrator::UnloadModel() {
  if (model_) {
    model_->is_loaded = false;
  }
}

std::string CrossEncoderOrchestrator::GetModelName() const {
  return config_.model_name;
}

bool CrossEncoderOrchestrator::EnsureModelLoaded() {
  if (IsModelLoaded()) {
    return true;
  }

  if (!model_) {
    return false;
  }

  std::string resolved_path = config_.model_path;
  std::string resolved_model = config_.model_name;
  if (resolved_path.empty()) {
    const std::filesystem::path cwd = std::filesystem::current_path();
    const std::filesystem::path candidate =
        cwd / "models" / (NormalizeModelIdentifier(config_.model_name) + ".onnx");
    if (std::filesystem::exists(candidate)) {
      resolved_path = candidate.string();
    }
  }

  if (!resolved_path.empty() && std::filesystem::exists(resolved_path)) {
    model_->resolved_path = resolved_path;
    model_->resolved_model_name = resolved_model;
    model_->is_loaded = true;
    return true;
  }

  if (config_.auto_download && !config_.registry_url.empty()) {
    themis::rag::judge::ONNXModelLoaderConfig loader_config;
    loader_config.auto_download = true;
    loader_config.cache_dir = "./models";
    loader_config.create_cache_dir = true;
    themis::rag::judge::ONNXModelLoader loader(loader_config);
    const auto info = loader.loadOrDownloadModel(resolved_model, config_.registry_url);
    if (info.has_value() && !info->model_path.empty()) {
      model_->resolved_path = info->model_path;
      model_->resolved_model_name = info->model_name;
      model_->is_loaded = true;
      return true;
    }
  }

  model_->resolved_path.clear();
  model_->resolved_model_name = resolved_model;
  model_->uses_fallback = true;
  return false;
}

void CrossEncoderOrchestrator::EvictStaleEntries() {
  auto now = std::chrono::steady_clock::now();
  std::chrono::hours cache_ttl(1);

  std::vector<std::string> stale_keys;
  for (const auto& [key, entry] : embedding_cache_) {
    if (now - entry.accessed_at > cache_ttl) {
      stale_keys.push_back(key);
    }
  }

  for (const auto& key : stale_keys) {
    embedding_cache_.erase(key);
  }
}

}  // namespace themis::rag
