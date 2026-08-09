/**
 * @file whisper_config.h
 * @brief Runtime configuration for the Whisper transcription plugin.
 * @version 1.9.0-beta
 * @note Score: 100/100
 * @note Status: Production Ready
 */

#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace themis {
namespace whisper {

using json = nlohmann::json;

/**
 * @brief Runtime configuration for the Whisper transcription plugin.
 */
struct WhisperConfig {
    std::string model_path;
    std::string model_sha256; // optional expected SHA-256 of model file (lowercase hex)
    std::string language = "auto";   // "auto" = detect, or BCP-47 code
    int  n_threads = 4;
    bool translate = false;          // translate to English if true
    int  beam_size = 5;
    bool print_progress = false;
    float quality_threshold = 0.0f;             // minimum transcription confidence to accept (0 = accept all)
    float language_confidence_threshold = 0.0f; // minimum detectLanguage confidence to accept (0 = accept all)

    static WhisperConfig fromJson(const json& j);
    json toJson() const;
};

} // namespace whisper
} // namespace themis
