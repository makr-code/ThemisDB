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
    if (j.contains("quality_threshold"))cfg.quality_threshold= j["quality_threshold"].get<float>();
    if (cfg.n_threads < 1)  cfg.n_threads = 1;
    if (cfg.beam_size < 1)  cfg.beam_size = 1;
    return cfg;
}

json WhisperConfig::toJson() const {
    return {
        {"model_path",        model_path},
        {"language",          language},
        {"n_threads",         n_threads},
        {"translate",         translate},
        {"beam_size",         beam_size},
        {"print_progress",    print_progress},
        {"quality_threshold", quality_threshold}
    };
}

} // namespace whisper
} // namespace themis
