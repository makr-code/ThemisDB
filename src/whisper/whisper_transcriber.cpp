/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            whisper_transcriber.cpp                            ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-15 18:51:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     123                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9919fc97a2  2026-04-07  feat(plugins): add whisper src impls (audio_chunk_reader,... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "whisper/whisper_transcriber.h"
#include <chrono>

#ifdef THEMIS_ENABLE_WHISPER
// Real whisper.cpp integration
// Requires: #include "whisper.h" (from whisper.cpp)
#include <whisper.h>
#endif

namespace themis {
namespace whisper {

#ifdef THEMIS_ENABLE_WHISPER

// ── WhisperCppTranscriber ──────────────────────────────────────────────────

WhisperCppTranscriber::WhisperCppTranscriber() = default;

WhisperCppTranscriber::~WhisperCppTranscriber() {
    if (ctx_) {
        whisper_free(static_cast<whisper_context*>(ctx_));
        ctx_ = nullptr;
    }
}

bool WhisperCppTranscriber::initialize(const WhisperConfig& cfg) {
    if (initialized_) return true;
    cfg_ = cfg;

    whisper_context_params cparams = whisper_context_default_params();
    auto* ctx = whisper_init_from_file_with_params(cfg.model_path.c_str(), cparams);
    if (!ctx) return false;

    ctx_ = ctx;
    model_id_ = cfg.model_path;
    initialized_ = true;
    return true;
}

audio::TranscriptionResult WhisperCppTranscriber::transcribe(
        const std::vector<float>& pcm, float sample_rate) {
    audio::TranscriptionResult result;
    result.ingestion_source_type = "WHISPER";
    result.plugin_version = "2.0.0";
    result.model_id = model_id_;
    result.duration_seconds = pcm.empty() ? 0.0 : pcm.size() / static_cast<double>(sample_rate);

    const int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    result.generation_timestamp = now_ms;

    if (!initialized_ || !ctx_) {
        result.success = false;
        result.error_message = "not initialized";
        return result;
    }

    whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    wparams.n_threads   = cfg_.n_threads;
    wparams.translate   = cfg_.translate;
    wparams.language    = cfg_.language == "auto" ? nullptr : cfg_.language.c_str();
    wparams.beam_search.beam_size = cfg_.beam_size;
    wparams.print_progress = cfg_.print_progress;

    auto* ctx = static_cast<whisper_context*>(ctx_);
    if (whisper_full(ctx, wparams, pcm.data(), static_cast<int>(pcm.size())) != 0) {
        result.success = false;
        result.error_message = "whisper_full() failed";
        return result;
    }

    const int n_seg = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_seg; ++i) {
        result.text += whisper_full_get_segment_text(ctx, i);
    }
    result.language   = whisper_lang_str(whisper_full_lang_id(ctx));
    result.confidence = 1.0f;  // whisper.cpp does not expose a single confidence score
    result.success    = true;
    return result;
}

audio::LanguageDetectionResult WhisperCppTranscriber::detectLanguage(
        const std::vector<float>& pcm, float /*sample_rate*/) {
    audio::LanguageDetectionResult res;
    if (!initialized_ || !ctx_ || pcm.empty()) return res;

    float lang_probs[WHISPER_N_LANGS];
    auto* ctx = static_cast<whisper_context*>(ctx_);
    const int lang_id = whisper_full_lang_id(ctx);
    whisper_lang_auto_detect(ctx, 0, cfg_.n_threads, lang_probs);
    if (lang_id >= 0) {
        res.language   = whisper_lang_str(lang_id);
        res.confidence = lang_probs[lang_id];
    }
    return res;
}

#endif // THEMIS_ENABLE_WHISPER

} // namespace whisper
} // namespace themis

