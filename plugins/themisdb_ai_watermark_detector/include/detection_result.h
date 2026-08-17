/**
 * @file detection_result.h
 * @brief Detection result structure for AI-generated content watermark detection.
 *
 * This file defines the output structure returned by WatermarkDetector,
 * containing confidence scores, metadata, and diagnostic information.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <chrono>

namespace themisdb::watermark {

/**
 * @enum DetectionStatus
 * @brief Status codes for watermark detection operations.
 */
enum class DetectionStatus : uint32_t {
  Success = 0,              /**< Detection completed successfully */
  TokenizationFailed = 8101, /**< Unable to tokenize input text */
  InvalidConfiguration = 8102, /**< Detection threshold out of range */
  TimeoutExceeded = 8103,   /**< Detection exceeded time limit */
  UnsupportedLanguage = 8104, /**< Language not in supported set */
  MemoryExhausted = 8105,   /**< Pattern cache unable to allocate */
  InvalidInput = 8106,      /**< Input validation failed */
  Unknown = 8199            /**< Unknown error */
};

/**
 * @enum AIModelFamily
 * @brief Detected or suspected AI model family.
 */
enum class AIModelFamily : uint32_t {
  Claude = 0,      /**< Anthropic Claude */
  GPT = 1,         /**< OpenAI GPT family */
  Gemini = 2,      /**< Google Gemini */
  Llama = 3,       /**< Meta Llama */
  Unknown = 255    /**< Unknown or no watermark detected */
};

/**
 * @struct DetectionResult
 * @brief Complete detection result with confidence scores and metadata.
 *
 * Contains the primary confidence score, per-model-family confidence estimates,
 * and diagnostic information about the detection process.
 */
struct DetectionResult {
  /**
   * @brief Overall confidence score [0.0, 1.0] that text is AI-generated.
   * 
   * Score interpretation:
   * - [0.0, 0.33): Human-written text (low confidence AI-generated)
   * - [0.33, 0.66): Ambiguous text (moderate confidence)
   * - [0.66, 1.0]: AI-generated text (high confidence)
   */
  float confidence_score = 0.5f;

  /**
   * @brief Primary detected AI model family (if confidence >= 0.7).
   */
  AIModelFamily detected_model_family = AIModelFamily::Unknown;

  /**
   * @brief Per-model-family confidence estimates.
   *
   * Maps from AIModelFamily enum to confidence score for fine-grained
   * identification across Claude, GPT, Gemini, Llama, etc.
   */
  std::unordered_map<uint32_t, float> model_family_scores;

  /**
   * @brief Detection status code.
   *
   * Indicates success or failure reason. Non-zero = error condition.
   */
  DetectionStatus status = DetectionStatus::Success;

  /**
   * @brief Human-readable error message (if status != Success).
   */
  std::string error_message;

  /**
   * @brief Number of tokens analyzed (if tokenization succeeded).
   */
  uint64_t tokens_analyzed = 0;

  /**
   * @brief Detected language code (ISO 639-1, e.g., "en", "de", "fr").
   */
  std::string detected_language = "unknown";

  /**
   * @brief Per-heuristic confidence contributions (diagnostic).
   *
   * Maps from heuristic name (e.g., "token_entropy", "green_red_list", "n_gram_entrenchment")
   * to confidence contribution [0.0, 1.0]. Sum may exceed 1.0 (aggregated via averaging).
   */
  std::unordered_map<std::string, float> heuristic_scores;

  /**
   * @brief Text length in characters.
   */
  uint64_t text_length_chars = 0;

  /**
   * @brief Detection duration in milliseconds.
   */
  uint64_t detection_duration_ms = 0;

  /**
   * @brief Whether the result came from cache (True) or fresh computation (False).
   */
  bool from_cache = false;

  /**
   * @brief Optional warning flags (e.g., "text_too_short", "high_uncertainty").
   */
  std::vector<std::string> warnings;

  /**
   * @brief Timestamp when detection was performed (UTC).
   */
  std::chrono::system_clock::time_point detection_timestamp = std::chrono::system_clock::now();

  /**
   * @brief Optional source identifier for audit trails.
   * 
   * Can store LLM model ID, document ID, or other provenance info.
   */
  std::string source_id;

  /**
   * @brief Serialize result to JSON string (diagnostic/logging).
   * 
   * @return JSON representation of detection result
   */
  std::string to_json() const;

  /**
   * @brief Check if result is reliable based on heuristics.
   * 
   * Returns true if: confidence not near 0.5 (ambiguous), text length adequate,
   * no resource/language warnings, detection succeeded.
   * 
   * @return true if result is considered reliable
   */
  bool is_reliable() const;
};

} // namespace themisdb::watermark
