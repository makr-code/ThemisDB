/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            whisper_plugin.h                                   ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-04-13 04:22:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     73                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • fdeed10753  2026-04-12  feat(whisper): v2.1.0 thread-safety, FfmpegAudioChunkRead... ║
    • 938636d98f  2026-04-07  feat(plugins): add audio/imggen interfaces, THEMIS_LLM_PL... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "plugins/audio_backend_interface.h"
#include "whisper/whisper_config.h"
#include "whisper/whisper_transcriber.h"
#include "whisper/audio_chunk_reader.h"
#include <memory>
#include <atomic>
#include <mutex>
#include <nlohmann/json.hpp>

namespace themis {
namespace whisper {

/**
 * @brief Top-level audio-transcription plugin that implements IAudioBackend.
 *
 * Wires together:
 *  - IWhisperTranscriber  (model inference, injected or created from type)
 *  - IAudioChunkReader    (file I/O, injected or defaults to CompositeAudioChunkReader)
 *
 * The default constructor selects WhisperCppTranscriber when
 * THEMIS_ENABLE_WHISPER is defined, otherwise WhisperStubTranscriber.
 *
 * Thread-safety: transcribe(), transcribeFile(), and detectLanguage() are
 * individually thread-safe.  Concurrent calls serialize through
 * transcriber_mutex_ so that the underlying whisper_context* is never
 * accessed from two threads simultaneously.  Counters are std::atomic.
 */
class WhisperPlugin : public audio::IAudioBackend {
public:
    /** Default constructor – builds production or stub backend automatically. */
    WhisperPlugin();

    /** Injection constructor for tests. */
    WhisperPlugin(std::unique_ptr<IWhisperTranscriber> transcriber,
                  std::unique_ptr<IAudioChunkReader>   reader);

    ~WhisperPlugin() override = default;

    // ── IAudioBackend ──────────────────────────────────────────────────────
    bool initialize(const std::string& model_path,
                    const nlohmann::json& config) override;

    bool isInitialized() const override { return initialized_.load(std::memory_order_acquire); }

    audio::TranscriptionResult transcribe(const std::vector<float>& pcm_samples,
                                          float sample_rate) override;

    audio::TranscriptionResult transcribeFile(const std::string& path) override;

    audio::LanguageDetectionResult detectLanguage(const std::vector<float>& pcm_samples,
                                                  float sample_rate) override;

    std::string getModelId() const override;
    std::string getPluginVersion() const override { return "2.0.0"; }
    nlohmann::json getStatistics() const override;

private:
    std::unique_ptr<IWhisperTranscriber> transcriber_;
    std::unique_ptr<IAudioChunkReader>   reader_;
    std::atomic<bool>     initialized_{false};
    std::atomic<uint64_t> transcription_count_{0};
    std::atomic<uint64_t> error_count_{0};
    std::string model_path_;
    mutable std::mutex transcriber_mutex_;  ///< serializes transcriber calls
};

} // namespace whisper
} // namespace themis

// Export symbols for dynamic loading
THEMIS_AUDIO_PLUGIN();
