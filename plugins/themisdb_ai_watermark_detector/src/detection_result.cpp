/**
 * @file detection_result.cpp
 * @brief Implementation of DetectionResult methods.
 */

#include "../include/detection_result.h"
#include <sstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace themisdb::watermark {

std::string DetectionResult::to_json() const {
  json j;
  j["confidence_score"] = confidence_score;
  j["detected_model_family"] = static_cast<uint32_t>(detected_model_family);
  j["status"] = static_cast<uint32_t>(status);
  j["error_message"] = error_message;
  j["tokens_analyzed"] = tokens_analyzed;
  j["detected_language"] = detected_language;
  j["text_length_chars"] = text_length_chars;
  j["detection_duration_ms"] = detection_duration_ms;
  j["from_cache"] = from_cache;
  j["source_id"] = source_id;

  // Convert model_family_scores
  json model_scores_json = json::object();
  for (const auto& [model, score] : model_family_scores) {
    model_scores_json[std::to_string(model)] = score;
  }
  j["model_family_scores"] = model_scores_json;

  // Convert heuristic_scores
  json heuristic_scores_json = json::object();
  for (const auto& [name, score] : heuristic_scores) {
    heuristic_scores_json[name] = score;
  }
  j["heuristic_scores"] = heuristic_scores_json;

  // Convert warnings
  j["warnings"] = warnings;

  return j.dump(2);
}

bool DetectionResult::is_reliable() const {
  // Check status
  if (status != DetectionStatus::Success) {
    return false;
  }

  // Check confidence is not ambiguous
  if (std::abs(confidence_score - 0.5f) < 0.15f) {
    return false;  // Too close to neutral
  }

  // Check for critical warnings
  for (const auto& warning : warnings) {
    if (warning == "text_too_short" || warning == "high_uncertainty" ||
        warning == "tokenization_failed" || warning == "memory_exhausted") {
      return false;
    }
  }

  // Check text length is adequate
  if (tokens_analyzed < 50) {
    return false;
  }

  return true;
}

}  // namespace themisdb::watermark
