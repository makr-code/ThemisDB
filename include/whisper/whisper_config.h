/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            whisper_config.h                                   ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-15 07:10:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     52                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9db3a4a848  2026-04-15  feat(whisper): add language_confidence_threshold config +... ║
    • 938636d98f  2026-04-07  feat(plugins): add audio/imggen interfaces, THEMIS_LLM_PL... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
