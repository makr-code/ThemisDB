/**
 * @file detector_interface.h
 * @brief Abstract interface for AI-generated content watermark detection.
 *
 * This is the primary API contract for the AI Watermark Detector plugin.
 * Implementations detect watermarks (Claude, GPT, Gemini, Llama) and return
 * confidence scores with comprehensive diagnostics.
 */

#pragma once

#include "detection_result.h"
#include "detection_config.h"
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace themisdb::watermark {

/**
 * @class WatermarkDetector
 * @brief Abstract interface for watermark detection on text.
 *
 * Implementations detect AI-generated content watermarks and return
 * confidence scores. Supports caching, multilingual detection, and
 * configurable thresholds.
 *
 * Thread safety: Implementations MUST be thread-safe for concurrent
 * detect_text() calls (same or different texts).
 *
 * Example usage:
 * @code
 * auto detector = WatermarkDetectorFactory::create();
 * DetectionConfig config;
 * config.confidence_threshold = 0.85f;
 * detector->configure(config);
 *
 * DetectionResult result = detector->detect_text(
 *     "This is a sample text to analyze...",
 *     "doc_12345");  // optional source_id for audit
 *
 * if (result.status == DetectionStatus::Success &&
 *     result.confidence_score >= config.confidence_threshold) {
 *   std::cout << "AI-generated text detected with score "
 *             << result.confidence_score << std::endl;
 * }
 * @endcode
 */
class WatermarkDetector {
public:
  virtual ~WatermarkDetector() = default;

  /**
   * @brief Configure detector with parameters.
   *
   * Must be called before detect_text(). Can be called multiple times
   * to reconfigure.
   *
   * @param config DetectionConfig with desired settings
   * @throws std::invalid_argument if config validation fails
   */
  virtual void configure(const DetectionConfig& config) = 0;

  /**
   * @brief Detect watermark in text and return confidence score.
   *
   * Analyzes the input text for AI-generated content markers
   * (watermarks, token distribution anomalies, entropy patterns, etc.)
   * and returns a confidence score.
   *
   * @param text Input text to analyze (UTF-8 required)
   * @param source_id Optional identifier for audit trails (e.g., LLM model ID, document ID)
   * @return DetectionResult with confidence score and metadata
   *
   * Thread safety: Safe to call concurrently from multiple threads.
   * Latency: ~5-15ms per 1k tokens (depending on text length and cache hit).
   *
   * @note Long texts (>100k tokens) may incur higher latency. Timeout
   *       is enforced via DetectionConfig::timeout_ms.
   *
   * @note Empty or very short texts (<50 tokens) receive "text_too_short"
   *       warning and reduced confidence (near 0.5).
   */
  virtual DetectionResult detect_text(const std::string& text,
                                      const std::string& source_id = "") = 0;

  /**
   * @brief Batch detection on multiple texts.
   *
   * Detects watermarks in multiple texts and returns results in order.
   * May be faster than sequential detect_text() calls if implementation
   * supports vectorized processing.
   *
   * @param texts Vector of texts to analyze
   * @param source_ids Optional vector of source IDs (must match texts.size() if provided)
   * @return Vector of DetectionResult in same order as inputs
   *
   * @throws std::invalid_argument if source_ids.size() != texts.size() and not empty
   *
   * Default implementation calls detect_text() for each text sequentially.
   * Subclasses may override for better performance.
   */
  virtual std::vector<DetectionResult> detect_batch(
      const std::vector<std::string>& texts,
      const std::vector<std::string>& source_ids = {}) {
    if (!source_ids.empty() && source_ids.size() != texts.size()) {
      throw std::invalid_argument(
          "source_ids must be empty or match texts.size()");
    }
    std::vector<DetectionResult> results;
    for (size_t i = 0; i < texts.size(); ++i) {
      std::string sid = (source_ids.size() > i) ? source_ids[i] : "";
      results.push_back(detect_text(texts[i], sid));
    }
    return results;
  }

  /**
   * @brief Clear cache of detected texts.
   *
   * Useful for memory management or to force re-detection after
   * algorithm updates.
   */
  virtual void clear_cache() = 0;

  /**
   * @brief Get cache statistics (hits, misses, size).
   *
   * Returns a diagnostic string with cache performance metrics.
   * Only meaningful if DetectionConfig::enable_caching = true.
   *
   * @return Cache statistics as string
   */
  virtual std::string get_cache_stats() const = 0;

  /**
   * @brief Get detector version and build information.
   *
   * @return Version string (e.g., "WatermarkDetector/1.0.0")
   */
  virtual std::string get_version() const = 0;

  /**
   * @brief Get list of supported AI model families.
   *
   * @return Vector of AIModelFamily enum values supported by this detector
   */
  virtual std::vector<AIModelFamily> get_supported_models() const = 0;

  /**
   * @brief Check if detector is ready for use.
   *
   * Returns false if initialization failed or dependencies are missing.
   *
   * @return true if detector is operational
   */
  virtual bool is_initialized() const = 0;

  /**
   * @brief Reset detector to initial state (clear caches, reload models).
   *
   * Useful for error recovery or cleanup. May be slow.
   *
   * @throws std::runtime_error if reset fails
   */
  virtual void reset() = 0;
};

/**
 * @class WatermarkDetectorFactory
 * @brief Factory for creating WatermarkDetector instances.
 *
 * Provides a single entry point for detector instantiation.
 * Implementations can be registered and selected via factory methods.
 */
class WatermarkDetectorFactory {
public:
  /**
   * @brief Create default watermark detector instance.
   *
   * Returns a fully initialized detector ready for use.
   * Implements Claude watermark detection + baseline heuristics.
   *
   * @return std::unique_ptr<WatermarkDetector> (never nullptr)
   * @throws std::runtime_error if detector initialization fails
   */
  static std::unique_ptr<WatermarkDetector> create();

  /**
   * @brief Create detector with specified implementation type.
   *
   * @param detector_type Implementation type (e.g., "claude", "ensemble", "debug")
   * @return std::unique_ptr<WatermarkDetector> or nullptr if type unknown
   */
  static std::unique_ptr<WatermarkDetector> create(const std::string& detector_type);
};

} // namespace themisdb::watermark
