/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            whisper_plugin.cpp                                 ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-15 07:15:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   92.0/100                                       ║
    • Total Lines:     187                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9db3a4a848  2026-04-15  feat(whisper): add language_confidence_threshold config +... ║
    • d275653619  2026-04-14  update after codefindings               ║
    • a2d7c07202  2026-04-14  update after codefindings               ║
    • fdeed10753  2026-04-12  feat(whisper): v2.1.0 thread-safety, FfmpegAudioChunkRead... ║
    • 9919fc97a2  2026-04-07  feat(plugins): add whisper src impls (audio_chunk_reader,... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "whisper/whisper_plugin.h"
#include <chrono>
#include <stdexcept>

namespace themis {
namespace whisper {

// ── constructors ─────────────────────────────────────────────────────────────

WhisperPlugin::WhisperPlugin() {
#ifdef THEMIS_ENABLE_WHISPER
    transcriber_ = std::make_unique<WhisperCppTranscriber>();
#else
    // STUB/SIMULATION NOTE:
    // Purpose: Keep the plugin loadable when Whisper support is not compiled in.
    // Activation: Compiled when THEMIS_ENABLE_WHISPER is not defined.
    // Production Delta: Transcription uses WhisperStubTranscriber behavior instead of whisper.cpp.
    // Removal Plan: Remove once Whisper becomes a mandatory dependency in all build targets.
    transcriber_ = std::make_unique<WhisperStubTranscriber>();
#endif
    auto composite = std::make_unique<CompositeAudioChunkReader>();
    composite->addReader(std::make_unique<WavAudioChunkReader>());
    composite->addReader(std::make_unique<FfmpegAudioChunkReader>());
    reader_ = std::move(composite);
}

WhisperPlugin::WhisperPlugin(std::unique_ptr<IWhisperTranscriber> transcriber,
                             std::unique_ptr<IAudioChunkReader>   reader)
    : transcriber_(std::move(transcriber))
    , reader_(std::move(reader)) {}

// ── initialize ───────────────────────────────────────────────────────────────

bool WhisperPlugin::initialize(const std::string& model_path,
                               const nlohmann::json& config) {
    model_path_ = model_path;
    WhisperConfig cfg = WhisperConfig::fromJson(config);
    cfg.model_path = model_path;
    cfg_ = cfg;

    const bool ok = transcriber_->initialize(cfg);
    initialized_.store(ok, std::memory_order_release);
    return ok;
}

// ── transcribe ───────────────────────────────────────────────────────────────

audio::TranscriptionResult WhisperPlugin::transcribe(const std::vector<float>& pcm,
                                                      float sample_rate) {
    if (!initialized_.load(std::memory_order_acquire)) {
        error_count_.fetch_add(1, std::memory_order_relaxed);
        audio::TranscriptionResult err;
        err.success = false;
        err.error_message = "WhisperPlugin not initialized";
        err.plugin_version = getPluginVersion();
        err.ingestion_source_type = "WHISPER";
        return err;
    }
    try {
        audio::TranscriptionResult result;
        {
            std::lock_guard<std::mutex> lock(transcriber_mutex_);
            result = transcriber_->transcribe(pcm, sample_rate);
        }
        // Mandatory provenance override – always from this plugin
        result.ingestion_source_type = "WHISPER";
        result.plugin_version        = getPluginVersion();
        result.model_id              = getModelId();
        if (result.generation_timestamp == 0) {
            result.generation_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        }
        transcription_count_.fetch_add(1, std::memory_order_relaxed);
        return result;
    } catch (const std::exception& ex) {
        error_count_.fetch_add(1, std::memory_order_relaxed);
        audio::TranscriptionResult err;
        err.success               = false;
        err.error_message         = ex.what();
        err.ingestion_source_type = "WHISPER";
        err.plugin_version        = getPluginVersion();
        return err;
    }
}

// ── transcribeFile ───────────────────────────────────────────────────────────

audio::TranscriptionResult WhisperPlugin::transcribeFile(const std::string& path) {
    if (!initialized_.load(std::memory_order_acquire)) {
        error_count_.fetch_add(1, std::memory_order_relaxed);
        audio::TranscriptionResult err;
        err.success               = false;
        err.error_message         = "WhisperPlugin not initialized";
        err.ingestion_source_type = "WHISPER";
        err.plugin_version        = getPluginVersion();
        return err;
    }
    try {
        float sample_rate = 16000.0f;
        auto pcm = reader_->readFile(path, sample_rate);
        return transcribe(pcm, sample_rate);
    } catch (const std::exception& ex) {
        error_count_.fetch_add(1, std::memory_order_relaxed);
        audio::TranscriptionResult err;
        err.success               = false;
        err.error_message         = std::string("transcribeFile: ") + ex.what();
        err.ingestion_source_type = "WHISPER";
        err.plugin_version        = getPluginVersion();
        return err;
    }
}

// ── detectLanguage ───────────────────────────────────────────────────────────

audio::LanguageDetectionResult WhisperPlugin::detectLanguage(
        const std::vector<float>& pcm, float sample_rate) {
    if (!initialized_.load(std::memory_order_acquire)) return {};
    audio::LanguageDetectionResult result;
    {
        std::lock_guard<std::mutex> lock(transcriber_mutex_);
        result = transcriber_->detectLanguage(pcm, sample_rate);
    }
    // Apply language-confidence threshold: return "unknown" when below threshold.
    if (cfg_.language_confidence_threshold > 0.0f &&
        result.confidence < cfg_.language_confidence_threshold) {
        return {"unknown", result.confidence};
    }
    return result;
}

// ── getModelId / getStatistics ───────────────────────────────────────────────

std::string WhisperPlugin::getModelId() const {
    return transcriber_ ? transcriber_->getModelId() : model_path_;
}

nlohmann::json WhisperPlugin::getStatistics() const {
    return {
        {"plugin",             "whisper"},
        {"plugin_version",     getPluginVersion()},
        {"model_id",           getModelId()},
        {"initialized",        initialized_.load(std::memory_order_acquire)},
        {"transcription_count", transcription_count_.load(std::memory_order_relaxed)},
        {"error_count",        error_count_.load(std::memory_order_relaxed)}
    };
}

} // namespace whisper
} // namespace themis

// ── dynamic-loading entry points ─────────────────────────────────────────────

extern "C" THEMIS_PLUGIN_EXPORT
themis::audio::IAudioBackend* themis_audio_create() {
    return new themis::whisper::WhisperPlugin();
}

extern "C" THEMIS_PLUGIN_EXPORT
void themis_audio_destroy(themis::audio::IAudioBackend* p) {
    delete p;
}
