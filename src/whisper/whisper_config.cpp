/*
 * ThemisDB | File: whisper_config.cpp | Version: 0.0.10 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 47
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=3 | delta=0 | status=aligned
 * External Severity (v3): C=0, H=2, M=1
 * PR: none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "whisper/whisper_config.h"
#include <stdexcept>

namespace themis {
namespace whisper {

WhisperConfig WhisperConfig::fromJson(const json& j) {
    WhisperConfig cfg;
    if (j.contains("model_path"))       cfg.model_path       = j["model_path"].get<std::string>();
    if (j.contains("language"))         cfg.language         = j["language"].get<std::string>();
    if (j.contains("n_threads"))        cfg.n_threads        = j["n_threads"].get<int>();
    if (j.contains("translate"))        cfg.translate        = j["translate"].get<bool>();
    if (j.contains("beam_size"))        cfg.beam_size        = j["beam_size"].get<int>();
    if (j.contains("print_progress"))   cfg.print_progress   = j["print_progress"].get<bool>();
    if (j.contains("quality_threshold"))             cfg.quality_threshold             = j["quality_threshold"].get<float>();
    if (j.contains("language_confidence_threshold")) cfg.language_confidence_threshold = j["language_confidence_threshold"].get<float>();
    if (cfg.n_threads < 1)  cfg.n_threads = 1;
    if (cfg.beam_size < 1)  cfg.beam_size = 1;
    if (cfg.language_confidence_threshold < 0.0f) cfg.language_confidence_threshold = 0.0f;
    if (cfg.language_confidence_threshold > 1.0f) cfg.language_confidence_threshold = 1.0f;
    return cfg;
}

json WhisperConfig::toJson() const {
    return {
        {"model_path",        model_path},
        {"language",          language},
        {"n_threads",         n_threads},
        {"translate",         translate},
        {"beam_size",         beam_size},
        {"print_progress",              print_progress},
        {"quality_threshold",           quality_threshold},
        {"language_confidence_threshold", language_confidence_threshold}
    };
}

} // namespace whisper
} // namespace themis

