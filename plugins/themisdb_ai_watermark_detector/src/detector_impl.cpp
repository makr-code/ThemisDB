/**
 * @file detector_impl.cpp
 * @brief Implementation of WatermarkDetector interface.
 *
 * This is the main detector implementation combining token analysis,
 * heuristic scoring, and caching.
 */

#include "../include/detector_interface.h"
#include "../include/token_distribution_analyzer.h"
#include <iostream>
#include <mutex>
#include <sstream>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace themisdb::watermark {

/**
 * @class WatermarkDetectorImpl
 * @brief Concrete implementation of WatermarkDetector.
 *
 * Combines token analysis, multiple heuristics, and caching.
 */
class WatermarkDetectorImpl : public WatermarkDetector {
private:
  DetectionConfig config_;
  std::unique_ptr<TokenDistributionAnalyzer> token_analyzer_;
  bool initialized_ = false;

  // Protects cache_, cache_hits_, cache_misses_, config_ for concurrent
  // detect_text() calls as required by the thread-safety contract.
  mutable std::mutex mutex_;

  // Cache: text_hash -> DetectionResult
  std::unordered_map<size_t, DetectionResult> cache_;
  size_t cache_hits_ = 0;
  size_t cache_misses_ = 0;

public:
  WatermarkDetectorImpl();
  ~WatermarkDetectorImpl() override = default;

  void configure(const DetectionConfig& config) override;
  DetectionResult detect_text(const std::string& text,
                              const std::string& source_id = "") override;
  void clear_cache() override;
  std::string get_cache_stats() const override;
  std::string get_version() const override;
  std::vector<AIModelFamily> get_supported_models() const override;
  bool is_initialized() const override { return initialized_; }
  void reset() override;

private:
  /**
   * @brief Compute hash of text for caching.
   */
  size_t hash_text(const std::string& text) const;

  /**
   * @brief Tokenize text (stub: returns empty vector).
   *
   * Phase 2: Replace with actual tokenizer (from LLM module or standalone).
   */
  std::vector<uint32_t> tokenize(const std::string& text);

  /**
   * @brief Detect language via heuristics (stub).
   *
   * Phase 2: Replace with actual language detection (e.g., fasttext).
   */
  std::string detect_language(const std::string& text);

  /**
   * @brief Aggregate heuristic scores into final confidence.
   */
  float aggregate_scores(const std::unordered_map<std::string, float>& heuristics);
};

// ============================================================================
// Implementation
// ============================================================================

WatermarkDetectorImpl::WatermarkDetectorImpl()
    : token_analyzer_(std::make_unique<TokenDistributionAnalyzer>()) {
  initialized_ = true;
}

void WatermarkDetectorImpl::configure(const DetectionConfig& config) {
  config.validate();
  std::lock_guard<std::mutex> lock(mutex_);
  config_ = config;
}

DetectionResult WatermarkDetectorImpl::detect_text(const std::string& text,
                                                  const std::string& source_id) {
  auto start_time = std::chrono::high_resolution_clock::now();
  DetectionResult result;
  result.source_id = source_id;
  result.text_length_chars = text.size();
  result.detection_timestamp = std::chrono::system_clock::now();

  // Phase 1: Validate input (read config_ under lock)
  DetectionConfig cfg;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    cfg = config_;
  }

  if (text.empty()) {
    result.status = DetectionStatus::InvalidInput;
    result.error_message = "Input text is empty";
    result.warnings.push_back("text_too_short");
    return result;
  }

  if (text.size() > static_cast<size_t>(cfg.max_input_length)) {
    result.status = DetectionStatus::InvalidInput;
    result.error_message = "Input text exceeds maximum length";
    return result;
  }

  // Phase 2: Check cache
  if (cfg.enable_caching) {
    size_t text_hash = hash_text(text);
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cache_.find(text_hash);
    if (it != cache_.end()) {
      result = it->second;
      result.from_cache = true;
      cache_hits_++;
      return result;
    }
    cache_misses_++;
  }

  // Phase 3: Check trusted sources
  if (!cfg.trusted_sources.empty() &&
      cfg.trusted_sources.count(source_id) > 0) {
    result.confidence_score = 0.0f;
    result.status = DetectionStatus::Success;
    result.detected_language = detect_language(text);
    return result;
  }

  // Phase 4: Tokenize (Phase 2: implement actual tokenization)
  auto tokens = tokenize(text);
  result.tokens_analyzed = tokens.size();

  if (tokens.empty()) {
    result.status = DetectionStatus::TokenizationFailed;
    result.error_message = "Failed to tokenize input text";
    result.confidence_score = 0.5f;
    result.warnings.push_back("tokenization_failed");
    return result;
  }

  // Phase 5: Detect language
  result.detected_language = detect_language(text);

  // Phase 6: Run heuristics
  auto heuristic_scores = token_analyzer_->get_all_heuristic_scores(
      tokens, result.text_length_chars);
  result.heuristic_scores = heuristic_scores;

  // Phase 7: Aggregate scores
  result.confidence_score = aggregate_scores(heuristic_scores);

  // Phase 8: Determine primary model family (stub: default Claude)
  if (result.confidence_score >= 0.7f) {
    result.detected_model_family = AIModelFamily::Claude;
  } else {
    result.detected_model_family = AIModelFamily::Unknown;
  }

  // Phase 9: Apply confidence threshold
  if (result.confidence_score < cfg.confidence_threshold) {
    result.confidence_score = 0.0f;
  }

  // Phase 10: Check for warnings
  if (result.tokens_analyzed < static_cast<uint64_t>(cfg.min_tokens_for_reliable_score)) {
    result.warnings.push_back("text_too_short");
  }

  if (cfg.strict_mode &&
      std::abs(result.confidence_score - cfg.confidence_threshold) < 0.1f) {
    result.warnings.push_back("high_uncertainty");
  }

  result.status = DetectionStatus::Success;

  // Phase 11: Cache result
  if (cfg.enable_caching) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (cache_.size() < static_cast<size_t>(cfg.cache_max_entries)) {
      cache_[hash_text(text)] = result;
    }
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  result.detection_duration_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time)
          .count();

  return result;
}

void WatermarkDetectorImpl::clear_cache() {
  std::lock_guard<std::mutex> lock(mutex_);
  cache_.clear();
  cache_hits_ = 0;
  cache_misses_ = 0;
}

std::string WatermarkDetectorImpl::get_cache_stats() const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::ostringstream oss;
  size_t total = cache_hits_ + cache_misses_;
  float hit_rate = (total > 0) ? (100.0f * cache_hits_ / total) : 0.0f;
  oss << "Cache hits: " << cache_hits_ << ", misses: " << cache_misses_
      << ", hit rate: " << hit_rate << "%, entries: " << cache_.size();
  return oss.str();
}

std::string WatermarkDetectorImpl::get_version() const {
  return "WatermarkDetector/1.0.0-phase1";
}

std::vector<AIModelFamily> WatermarkDetectorImpl::get_supported_models() const {
  return {AIModelFamily::Claude};  // Phase 2+: add GPT, Gemini, Llama
}

void WatermarkDetectorImpl::reset() {
  clear_cache();
  token_analyzer_ = std::make_unique<TokenDistributionAnalyzer>();
}

size_t WatermarkDetectorImpl::hash_text(const std::string& text) const {
  std::hash<std::string> hasher;
  return hasher(text);
}

std::vector<uint32_t> WatermarkDetectorImpl::tokenize(const std::string& text) {
  // STUB/SIMULATION NOTE:
  // Purpose: Phase 1 placeholder; returns deterministic dummy token IDs based
  //          on text length so heuristics can exercise a non-empty token path.
  // Activation: Always active in Phase 1; gated out once Phase 2 LLM-module
  //             tokenizer integration is complete.
  // Production Delta: Does not reflect real vocabulary tokens; distributions
  //                   are not meaningful for watermark detection.
  // Removal Plan: Replace with actual tokenizer in Phase 2
  //               (see plugins/themisdb_ai_watermark_detector/ROADMAP.md Phase 2).
  std::vector<uint32_t> tokens;
  int estimated_tokens = text.size() / 4;  // rough estimate: 4 chars per token
  if (estimated_tokens < 10) estimated_tokens = 10;

  for (int i = 0; i < estimated_tokens && i < 1000; ++i) {
    tokens.push_back(i % 50000);  // dummy token ID
  }
  return tokens;
}

std::string WatermarkDetectorImpl::detect_language(const std::string& text) {
  // STUB: Phase 2 implementation will use language detection library (e.g., fasttext).
  // For now, return "en" as default.
  if (text.empty()) return "unknown";
  return "en";
}

float WatermarkDetectorImpl::aggregate_scores(
    const std::unordered_map<std::string, float>& heuristics) {
  if (heuristics.empty()) return 0.5f;

  float sum = 0.0f;
  for (const auto& [name, score] : heuristics) {
    sum += score;
  }
  return sum / heuristics.size();
}

// ============================================================================
// Factory
// ============================================================================

std::unique_ptr<WatermarkDetector> WatermarkDetectorFactory::create() {
  auto detector = std::make_unique<WatermarkDetectorImpl>();
  if (!detector->is_initialized()) {
    throw std::runtime_error("Failed to initialize WatermarkDetector");
  }
  return detector;
}

std::unique_ptr<WatermarkDetector> WatermarkDetectorFactory::create(
    const std::string& detector_type) {
  if (detector_type == "claude" || detector_type == "default") {
    return create();
  }
  return nullptr;
}

}  // namespace themisdb::watermark
