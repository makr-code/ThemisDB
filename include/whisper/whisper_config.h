/*
 * ThemisDB | File: whisper_config.h | Version: 0.0.10
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
