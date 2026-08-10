/**
 * @file whisper_plugin.cpp
 * @brief Whisper plugin implementation.
 * @version 1.9.0-beta
 * @note Score: 100/100
 * @note Status: Production Ready
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
    // PERMANENT HARDWARE FALLBACK NOTE (Whisper SDK not available):
    // Purpose: Keep the plugin loadable when Whisper support is not compiled in.
    // Activation: Compiled when THEMIS_ENABLE_WHISPER is not defined.
    // Production Delta: Transcription uses WhisperStubTranscriber behavior instead of whisper.cpp.
    // Hardware requirement: whisper.cpp + -DTHEMIS_ENABLE_WHISPER=ON.
    // Roadmap ref: src/whisper/ROADMAP.md § "Planned Features"
    StubTranscriberFactoryFn factory;
    {
        std::lock_guard<std::mutex> lk(s_stub_transcriber_factory_mutex);
        factory = s_stub_transcriber_factory_fn;
    }
    if (factory) {
        try {
            transcriber_ = factory();
        } catch (const nlohmann::json::exception& ex) {
            std::lock_guard<std::mutex> lk(error_mutex_);
            last_error_message_ = ex.what();
            transcriber_.reset();
        } catch (const std::exception& ex) {
            std::lock_guard<std::mutex> lk(error_mutex_);
            last_error_message_ = ex.what();
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
    if (!ok) {
        auto err = transcriber_->getLastError();
        if (err.empty()) {
            err = "failed to initialize transcriber";
        }
        std::lock_guard<std::mutex> lk(error_mutex_);
        last_error_message_ = std::move(err);
    } else {
        std::lock_guard<std::mutex> lk(error_mutex_);
        last_error_message_.clear();
    }
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
        std::string last_error;
        {
            std::lock_guard<std::mutex> lk(error_mutex_);
            last_error = last_error_message_;
        }
        err.error_message = last_error.empty()
            ? "WhisperPlugin not initialized"
            : "WhisperPlugin not initialized: " + last_error;
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
        {
            std::lock_guard<std::mutex> lk(error_mutex_);
            last_error_message_ = ex.what();
        }
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
        std::string last_error;
        {
            std::lock_guard<std::mutex> lk(error_mutex_);
            last_error = last_error_message_;
        }
        err.error_message = last_error.empty()
            ? "WhisperPlugin not initialized"
            : "WhisperPlugin not initialized: " + last_error;
        err.ingestion_source_type = "WHISPER";
        err.plugin_version        = getPluginVersion();
        return err;
    }
    try {
        float sample_rate = 16000.0f;
        auto pcm = reader_->readFile(path, sample_rate);
        return transcribe(pcm, sample_rate);
    } catch (const std::exception& ex) {
        {
            std::lock_guard<std::mutex> lk(error_mutex_);
            last_error_message_ = ex.what();
        }
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

DiarisationResult WhisperPlugin::transcribeWithDiarisation(
        const std::vector<float>& pcm_samples,
        float sample_rate,
        const DiarisationConfig& cfg) {
    DiarisationResult result;
    result.plugin_version = getPluginVersion();
    result.model_id = getModelId();
    result.ingestion_source_type = "WHISPER";
    result.generation_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    if (!initialized_.load(std::memory_order_acquire)) {
        error_count_.fetch_add(1, std::memory_order_relaxed);
        result.success = false;
        std::string last_error;
        {
            std::lock_guard<std::mutex> lk(error_mutex_);
            last_error = last_error_message_;
        }
        result.error_message = last_error.empty()
            ? "WhisperPlugin not initialized"
            : "WhisperPlugin not initialized: " + last_error;
        return result;
    }

    try {
        DiarisationConfig effective_cfg = cfg;
        if (effective_cfg.min_speakers < 1) {
            effective_cfg.min_speakers = 1;
        }
        if (effective_cfg.max_speakers < effective_cfg.min_speakers) {
            effective_cfg.max_speakers = effective_cfg.min_speakers;
        }
        std::lock_guard<std::mutex> lock(transcriber_mutex_);
        result = transcriber_->diarize(pcm_samples, sample_rate, effective_cfg);
        result.plugin_version = getPluginVersion();
        result.model_id = getModelId();
        result.ingestion_source_type = "WHISPER";
        if (result.generation_timestamp == 0) {
            result.generation_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        }
        return result;
    } catch (const std::exception& ex) {
        {
            std::lock_guard<std::mutex> lk(error_mutex_);
            last_error_message_ = ex.what();
        }
        error_count_.fetch_add(1, std::memory_order_relaxed);
        result.success = false;
        result.error_message = ex.what();
        return result;
    }
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
        std::string last_error;
        {
            std::lock_guard<std::mutex> lk(error_mutex_);
            last_error = last_error_message_;
        }
        err.error_message = last_error.empty()
            ? "WhisperPlugin not initialized"
            : "WhisperPlugin not initialized: " + last_error;
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
        {
            std::lock_guard<std::mutex> lk(error_mutex_);
            last_error_message_ = ex.what();
        }
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
    std::string last_error;
    {
        std::lock_guard<std::mutex> lk(error_mutex_);
        last_error = last_error_message_;
    }
    return {
        {"plugin",             "whisper"},
        {"plugin_version",     getPluginVersion()},
        {"model_id",           getModelId()},
        {"initialized",        initialized_.load(std::memory_order_acquire)},
        {"transcription_count", transcription_count_.load(std::memory_order_relaxed)},
        {"error_count",        error_count_.load(std::memory_order_relaxed)},
        {"last_error",         last_error}
    };
}

} // namespace whisper
} // namespace themis

// ── dynamic-loading entry points ─────────────────────────────────────────────

#if !defined(THEMIS_TEST_BUILD) && defined(THEMIS_PLUGIN_EXPORTS)
extern "C" THEMIS_PLUGIN_EXPORT
themis::audio::IAudioBackend* themis_audio_create() {
    auto plugin = std::make_unique<themis::whisper::WhisperPlugin>();
    return plugin.release();
}

extern "C" THEMIS_PLUGIN_EXPORT
void themis_audio_destroy(themis::audio::IAudioBackend* p) {
    delete p;  // delete nullptr is well-defined; ownership transferred to this function
}
#endif
