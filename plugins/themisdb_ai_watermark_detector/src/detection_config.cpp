/**
 * @file detection_config.cpp
 * @brief Implementation of DetectionConfig validation and formatting.
 */

#include "../include/detection_config.h"
#include <stdexcept>
#include <sstream>

namespace themisdb::watermark {

void DetectionConfig::validate() const {
  if (confidence_threshold < 0.0f || confidence_threshold > 1.0f) {
    throw std::invalid_argument(
        "confidence_threshold must be in [0.0, 1.0], got " +
        std::to_string(confidence_threshold));
  }

  if (max_input_length <= 0) {
    throw std::invalid_argument(
        "max_input_length must be > 0, got " + std::to_string(max_input_length));
  }

  if (cache_max_entries < 0) {
    throw std::invalid_argument(
        "cache_max_entries must be >= 0, got " + std::to_string(cache_max_entries));
  }

  if (timeout_ms <= 0) {
    throw std::invalid_argument(
        "timeout_ms must be > 0, got " + std::to_string(timeout_ms));
  }

  if (min_tokens_for_reliable_score < 0) {
    throw std::invalid_argument(
        "min_tokens_for_reliable_score must be >= 0, got " +
        std::to_string(min_tokens_for_reliable_score));
  }
}

std::string DetectionConfig::to_string() const {
  std::ostringstream oss;
  oss << "DetectionConfig{\n"
      << "  confidence_threshold=" << confidence_threshold << "\n"
      << "  strict_mode=" << (strict_mode ? "true" : "false") << "\n"
      << "  max_input_length=" << max_input_length << "\n"
      << "  enable_caching=" << (enable_caching ? "true" : "false") << "\n"
      << "  cache_max_entries=" << cache_max_entries << "\n"
      << "  timeout_ms=" << timeout_ms << "\n"
      << "  enable_multilingual=" << (enable_multilingual ? "true" : "false")
      << "\n"
      << "  min_tokens_for_reliable_score=" << min_tokens_for_reliable_score
      << "\n"
      << "  enable_diagnostics=" << (enable_diagnostics ? "true" : "false")
      << "\n"
      << "}";
  return oss.str();
}

}  // namespace themisdb::watermark
