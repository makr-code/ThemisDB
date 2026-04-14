/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            whisper_transcriber.h                              ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-04-14 06:58:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   88.0/100                                       ║
    • Total Lines:     170                                            ║
    • Open Issues:     TODOs: 0, Stubs: 4                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 938636d98f  2026-04-07  feat(plugins): add audio/imggen interfaces, THEMIS_LLM_PL... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "whisper/whisper_config.h"
#include "plugins/audio_backend_interface.h"
#include <string>
#include <vector>
#include <memory>

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

    virtual bool initialize(const WhisperConfig& cfg) = 0;
    virtual bool isInitialized() const = 0;

    virtual audio::TranscriptionResult    transcribe(const std::vector<float>& pcm,
                                                     float sample_rate) = 0;
    virtual audio::LanguageDetectionResult detectLanguage(const std::vector<float>& pcm,
                                                          float sample_rate) = 0;
    virtual std::string getModelId() const = 0;
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
    bool initialize(const WhisperConfig& cfg) override {
        model_id_ = cfg.model_path.empty() ? "stub" : cfg.model_path;
        initialized_ = true;
        return true;
    }
    bool isInitialized() const override { return initialized_; }

    audio::TranscriptionResult transcribe(const std::vector<float>&, float) override {
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
    std::string getModelId() const override { return model_id_; }

private:
    bool        initialized_ = false;
    std::string model_id_ = "inmemory";
    audio::TranscriptionResult     next_result_;
    audio::LanguageDetectionResult next_lang_;
};

} // namespace whisper
} // namespace themis
