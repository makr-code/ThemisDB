/*
 * ThemisDB | File: whisper_plugin.cpp | Version: 0.0.10 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 87/100 | Lines: 272
 * Open Issues: TODOs=1, Stubs=3, Gaps=7, Unimpl=0, Mock=1, Sim=2, Debt=0
 * Gap Correlation: internal=7 | external_v3=52 | delta=45 | status=divergent
 * External Severity (v3): C=5, H=42, M=5
 * PR: none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "whisper/whisper_plugin.h"
#include <chrono>
#include <stdexcept>
#include <numeric>

namespace themis {
namespace whisper {

namespace {
std::mutex                            s_stub_transcriber_factory_mutex;
WhisperPlugin::StubTranscriberFactoryFn s_stub_transcriber_factory_fn;
}

void WhisperPlugin::setStubTranscriberFactoryFn(StubTranscriberFactoryFn fn) {
    std::lock_guard<std::mutex> lk(s_stub_transcriber_factory_mutex);
    s_stub_transcriber_factory_fn = std::move(fn);
}

// ── constructors ─────────────────────────────────────────────────────────────

WhisperPlugin::WhisperPlugin() {
#ifdef THEMIS_ENABLE_WHISPER
    transcriber_ = std::make_unique<WhisperCppTranscriber>();
#else
    // STUB/SIMULATION NOTE:
    // Purpose: Keep the plugin loadable when Whisper support is not compiled in.
    // Activation: Compiled when THEMIS_ENABLE_WHISPER is not defined.
    // Production Delta: Transcription uses WhisperStubTranscriber behavior instead of whisper.cpp.
    // Roadmap ref: src/whisper/ROADMAP.md § "Planned Features"
    // Removal Plan: Remove once Whisper becomes a mandatory dependency in all build targets.
    // Roadmap ref: src/whisper/FUTURE_ENHANCEMENTS.md § "Stub/Simulation Lifecycle"
    StubTranscriberFactoryFn factory;
    {
        std::lock_guard<std::mutex> lk(s_stub_transcriber_factory_mutex);
        factory = s_stub_transcriber_factory_fn;
    }
    if (factory) {
        try {
            transcriber_ = factory();
        } catch (const std::exception&) {
            transcriber_.reset();
        }
    }
    if (!transcriber_) {
        transcriber_ = std::make_unique<WhisperStubTranscriber>();
    }
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

// ── transcribeStream ─────────────────────────────────────────────────────────

audio::TranscriptionResult WhisperPlugin::transcribeStream(
        const std::vector<float>& pcm_samples,
        float sample_rate,
        audio::StreamCallback callback) {
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
        // Apply VAD to skip silent frames if a detector is installed.
        // applyVad() holds vad_mutex_ internally, avoiding a data race.
        const auto effective_pcm = applyVad(pcm_samples, sample_rate);

        audio::TranscriptionResult result;
        {
            std::lock_guard<std::mutex> lock(transcriber_mutex_);
            result = transcriber_->transcribeStream(effective_pcm, sample_rate, std::move(callback));
        }
        // Mandatory provenance override
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

// ── VAD ──────────────────────────────────────────────────────────────────────

void WhisperPlugin::setVoiceActivityDetector(std::unique_ptr<IVoiceActivityDetector> vad,
                                              const VadConfig& cfg) {
    std::lock_guard<std::mutex> lk(vad_mutex_);
    vad_     = std::move(vad);
    vad_cfg_ = cfg;
}

std::vector<float> WhisperPlugin::applyVad(const std::vector<float>& pcm,
                                            float sample_rate) const {
    std::lock_guard<std::mutex> lk(vad_mutex_);
    if (!vad_ || pcm.empty()) return pcm;
    const auto segments = vad_->detect(pcm, sample_rate, vad_cfg_);
    if (segments.empty()) return {};

    // Concatenate all speech segments into a single buffer
    std::size_t total = 0;
    for (const auto& seg : segments) {
        total += seg.end_sample - seg.start_sample;
    }
    std::vector<float> speech;
    speech.reserve(total);
    for (const auto& seg : segments) {
        speech.insert(speech.end(),
                      pcm.begin() + static_cast<std::ptrdiff_t>(seg.start_sample),
                      pcm.begin() + static_cast<std::ptrdiff_t>(seg.end_sample));
    }
    return speech;
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

#ifndef THEMIS_TEST_BUILD
extern "C" THEMIS_PLUGIN_EXPORT
themis::audio::IAudioBackend* themis_audio_create() {
    return new themis::whisper::WhisperPlugin();
}

extern "C" THEMIS_PLUGIN_EXPORT
void themis_audio_destroy(themis::audio::IAudioBackend* p) {
    delete p;  // delete nullptr is well-defined; ownership transferred to this function
}
#endif
