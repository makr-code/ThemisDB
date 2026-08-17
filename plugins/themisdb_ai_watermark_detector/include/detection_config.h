/**
 * @file detection_config.h
 * @brief Configuration structure for watermark detection.
 *
 * Allows fine-grained control of detection behavior: thresholds, timeouts,
 * caching, language support, and trusted source allowlists.
 */

#pragma once

#include <string>
#include <set>
#include <cstdint>

namespace themisdb::watermark {

/**
 * @struct DetectionConfig
 * @brief Configuration parameters for WatermarkDetector.
 *
 * All parameters have sensible defaults; users can override as needed.
 */
struct DetectionConfig {
  /**
   * @brief Confidence threshold [0.0, 1.0] for reporting AI-generated text.
   *
   * If computed confidence < threshold, result is reported as non-AI
   * (confidence_score = 0.0). Default: 0.85 (high confidence required).
   *
   * Valid range: [0.0, 1.0]
   */
  float confidence_threshold = 0.85f;

  /**
   * @brief Strict mode: reject any ambiguity near threshold.
   *
   * If true, results within ±0.1 of threshold are marked unreliable
   * with warning "high_uncertainty". Default: false.
   */
  bool strict_mode = false;

  /**
   * @brief Maximum input text length in characters.
   *
   * Texts exceeding this limit are truncated or rejected.
   * Default: 1,000,000 chars (~200KB average text).
   */
  int max_input_length = 1'000'000;

  /**
   * @brief Enable result caching for identical inputs.
   *
   * If true, hash(text) -> cached_result lookup is performed.
   * Useful for high-volume or repeated-text scenarios.
   * Default: true.
   */
  bool enable_caching = true;

  /**
   * @brief Maximum number of cached results to retain (LRU eviction).
   *
   * Default: 10,000 entries (~100MB memory estimate).
   */
  int cache_max_entries = 10'000;

  /**
   * @brief Set of trusted source IDs to skip detection.
   *
   * If source_id is in this set, detection is skipped and
   * confidence_score = 0.0 is returned (human-written).
   *
   * Example: {"human_editorial_db", "verified_authors"}
   */
  std::set<std::string> trusted_sources;

  /**
   * @brief Maximum detection time per text in milliseconds.
   *
   * If detection exceeds this, TimeoutExceeded error is returned.
   * Default: 5000ms (5 seconds).
   */
  int timeout_ms = 5000;

  /**
   * @brief Enable multilingual detection (vs. English-only).
   *
   * If true, detection is attempted for any UTF-8 text.
   * If false, only English-language texts are analyzed.
   * Default: true.
   */
  bool enable_multilingual = true;

  /**
   * @brief Supported languages (ISO 639-1 codes, e.g., "en", "de", "fr").
   *
   * If enable_multilingual = true and detected language not in this set,
   * UnsupportedLanguage warning is issued but detection continues.
   * Default: {"en", "de", "fr", "es", "it", "pt", "nl", "ja", "zh"}
   */
  std::set<std::string> supported_languages = {
      "en", "de", "fr", "es", "it", "pt", "nl", "ja", "zh"};

  /**
   * @brief Minimum text length in tokens for reliable detection.
   *
   * Texts shorter than this may have unreliable scores.
   * Default: 100 tokens (~300 chars average).
   */
  int min_tokens_for_reliable_score = 100;

  /**
   * @brief Debug mode: include per-heuristic scores in result.
   *
   * If true, heuristic_scores map is populated. May increase
   * detection latency by ~10%. Default: false.
   */
  bool enable_diagnostics = false;

  /**
   * @brief Validate configuration parameters.
   *
   * Checks that all numeric ranges are valid and raises std::invalid_argument
   * if not.
   *
   * @throws std::invalid_argument if validation fails
   */
  void validate() const;

  /**
   * @brief Return a human-readable configuration summary.
   *
   * @return Configuration as formatted string
   */
  std::string to_string() const;
};

} // namespace themisdb::watermark
