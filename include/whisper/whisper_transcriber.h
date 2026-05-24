/*
 * ThemisDB | File: whisper_transcriber.h | Version: 0.0.10
 * Maturity: 🟢 PRODUCTION-READY | Score: 94/100
 * Gap Summary: total=8; TODO=1, Stub=6, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "whisper/whisper_config.h"
#include "plugins/audio_backend_interface.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>

namespace themis {
namespace whisper {

/**
 * @brief Interface for the core transcription engine.
 *
 * Separates model-loading / inference from the plugin lifecycle so that
 * test doubles can be injected without linking whisper.cpp.
 */
class IWhisperTranscriber {
public:
    virtual ~IWhisperTranscriber() = default;

    [[nodiscard]] virtual bool initialize(const WhisperConfig& cfg) = 0;
    [[nodiscard]] virtual bool isInitialized() const = 0;

    [[nodiscard]] virtual audio::TranscriptionResult    transcribe(const std::vector<float>& pcm,
                                                     float sample_rate) = 0;
    [[nodiscard]] virtual audio::LanguageDetectionResult detectLanguage(const std::vector<float>& pcm,
                                                          float sample_rate) = 0;

    /**
     * @brief Transcribe with incremental token streaming.
     *
     * Default implementation calls transcribe() and emits the full text
     * as one token.  Implementations backed by a real model should call
     * the callback for every word or segment.
     */
    virtual audio::TranscriptionResult transcribeStream(
            const std::vector<float>& pcm,
            float sample_rate,
            audio::StreamCallback callback) {
        auto result = transcribe(pcm, sample_rate);
        if (result.success && callback) {
            audio::TranscriptionToken tok;
            tok.text        = result.text;
            tok.confidence  = result.confidence;
            tok.token_index = 0;
            callback(tok);
        }
        return result;
    }

    [[nodiscard]] virtual std::string getModelId() const = 0;
};

// ---------------------------------------------------------------------------
// Real implementation – only compiled when whisper.cpp is available
// ---------------------------------------------------------------------------

#ifdef THEMIS_ENABLE_WHISPER
/**
 * @brief Transcriber backed by whisper.cpp.
 *
 * Requires linking against libwhisper.  When THEMIS_ENABLE_WHISPER is not
 * defined the WhisperPlugin falls back to WhisperStubTranscriber.
 */
class WhisperCppTranscriber : public IWhisperTranscriber {
public:
    WhisperCppTranscriber();
    ~WhisperCppTranscriber() override;

    bool initialize(const WhisperConfig& cfg) override;
    bool isInitialized() const override { return initialized_; }

    audio::TranscriptionResult    transcribe(const std::vector<float>& pcm,
                                             float sample_rate) override;
    audio::LanguageDetectionResult detectLanguage(const std::vector<float>& pcm,
                                                  float sample_rate) override;
    std::string getModelId() const override { return model_id_; }

private:
    bool        initialized_ = false;
    std::string model_id_;
    void*       ctx_ = nullptr;  // whisper_context* (opaque to avoid header dep)
    WhisperConfig cfg_;
};
#endif // THEMIS_ENABLE_WHISPER

// ---------------------------------------------------------------------------
// Stub transcriber – used when whisper.cpp is not linked
// ---------------------------------------------------------------------------

/**
 * @brief Stub transcriber that returns silent/empty results without any model.
 *
 * Used in CI builds that do not have a whisper.cpp model file available.
 */
class WhisperStubTranscriber : public IWhisperTranscriber {
public:
    /// Callback type for injecting a real transcription implementation into
    /// WhisperStubTranscriber without full whisper.cpp integration.
    using TranscribeFn = std::function<
        audio::TranscriptionResult(const std::vector<float>& /*pcm*/,
                                   float                    /*sample_rate*/)>;

    bool initialize(const WhisperConfig& cfg) override {
        model_id_ = cfg.model_path.empty() ? "stub" : cfg.model_path;
        initialized_ = true;
        return true;
    }
    bool isInitialized() const override { return initialized_; }

    /// Inject (or remove) a real transcription fn.  Pass nullptr to restore
    /// the empty-result stub.  Thread-safe with concurrent transcribe() calls.
    void setTranscribeFn(TranscribeFn fn) {
        std::lock_guard<std::mutex> lk(transcribe_fn_mutex_);
        transcribe_fn_ = std::move(fn);
    }

    audio::TranscriptionResult transcribe(const std::vector<float>& pcm,
                                          float sample_rate) override {
        TranscribeFn fn_copy;
        {
            std::lock_guard<std::mutex> lk(transcribe_fn_mutex_);
            fn_copy = transcribe_fn_;
        }
        if (fn_copy) {
            auto result = fn_copy(pcm, sample_rate);
            result.model_id            = model_id_;
            result.plugin_version      = "2.0.0";
            result.ingestion_source_type = "WHISPER";
            return result;
        }
        audio::TranscriptionResult r;
        r.text = "";
        r.language = "unknown";
        r.confidence = 0.0f;
        r.model_id = model_id_;
        r.plugin_version = "2.0.0";
        r.ingestion_source_type = "WHISPER";
        return r;
    }
    audio::LanguageDetectionResult detectLanguage(const std::vector<float>&, float) override {
        return {"unknown", 0.0f};
    }
    std::string getModelId() const override { return model_id_; }

private:
    bool        initialized_ = false;
    std::string model_id_ = "stub";
    TranscribeFn        transcribe_fn_;
    std::mutex          transcribe_fn_mutex_;
};

// ---------------------------------------------------------------------------
// Test double
// ---------------------------------------------------------------------------

/**
 * @brief In-memory transcriber for unit tests.
 *
 * Callers pre-set the next result via setNextResult() / setNextLanguage().
 */
class InMemoryWhisperTranscriber : public IWhisperTranscriber {
public:
    void setNextResult(audio::TranscriptionResult r) {
        next_result_ = std::move(r);
        initialized_ = true;
    }
    void setNextLanguage(audio::LanguageDetectionResult r) {
        next_lang_ = std::move(r);
    }
    /** Pre-set tokens to emit during transcribeStream() instead of one bulk token. */
    void setStreamTokens(std::vector<audio::TranscriptionToken> tokens) {
        stream_tokens_ = std::move(tokens);
    }

    bool initialize(const WhisperConfig& cfg) override {
        model_id_ = cfg.model_path.empty() ? "inmemory" : cfg.model_path;
        initialized_ = true;
        return true;
    }
    bool isInitialized() const override { return initialized_; }

    audio::TranscriptionResult transcribe(const std::vector<float>&, float) override {
        auto r = next_result_;
        r.ingestion_source_type = "WHISPER";
        r.model_id = model_id_;
        return r;
    }
    audio::LanguageDetectionResult detectLanguage(const std::vector<float>&, float) override {
        return next_lang_;
    }

    audio::TranscriptionResult transcribeStream(
            const std::vector<float>& pcm,
            float sample_rate,
            audio::StreamCallback callback) override {
        if (!stream_tokens_.empty() && callback) {
            for (const auto& tok : stream_tokens_) {
                callback(tok);
            }
            auto r = next_result_;
            r.ingestion_source_type = "WHISPER";
            r.model_id = model_id_;
            return r;
        }
        return IWhisperTranscriber::transcribeStream(pcm, sample_rate, std::move(callback));
    }

    std::string getModelId() const override { return model_id_; }

private:
    bool        initialized_ = false;
    std::string model_id_ = "inmemory";
    audio::TranscriptionResult     next_result_;
    audio::LanguageDetectionResult next_lang_;
    std::vector<audio::TranscriptionToken> stream_tokens_;
};

} // namespace whisper
} // namespace themis
