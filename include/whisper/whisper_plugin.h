/**
 * @file whisper_plugin.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.9.0-beta
 * @note Maturity: PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "plugins/audio_backend_interface.h"
#include "whisper/whisper_config.h"
#include "whisper/whisper_transcriber.h"
#include "whisper/audio_chunk_reader.h"
#include "whisper/voice_activity_detector.h"
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
    using StubTranscriberFactoryFn = std::function<std::unique_ptr<IWhisperTranscriber>()>;

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

    /**
     * @brief Transcribe with incremental token streaming.
     *
     * If a VAD is installed, silent segments are skipped before the PCM is
     * forwarded to the transcriber.  The @p callback is invoked once per
     * token emitted by the underlying transcriber.  Any exception thrown by
     * the callback aborts the stream and the returned result has
     * success=false.
     */
    audio::TranscriptionResult transcribeStream(
            const std::vector<float>& pcm_samples,
            float sample_rate,
            audio::StreamCallback callback) override;

    audio::LanguageDetectionResult detectLanguage(const std::vector<float>& pcm_samples,
                                                  float sample_rate) override;

    /**
     * @brief Transcribe and optionally attach speaker diarisation segments.
     *
     * Uses the transcriber's optional diarize() capability and always applies
     * plugin-side provenance fields on the returned result.
     */
    DiarisationResult transcribeWithDiarisation(const std::vector<float>& pcm_samples,
                                                float sample_rate,
                                                const DiarisationConfig& cfg);

    std::string getModelId() const override;
    std::string getPluginVersion() const override { return "2.3.0"; }
    nlohmann::json getStatistics() const override;

    static void setStubTranscriberFactoryFn(StubTranscriberFactoryFn fn);

    // ── VAD injection ──────────────────────────────────────────────────────
    /**
     * @brief Inject a custom Voice Activity Detector.
     *
     * If set, transcribeStream() (and transcribe() when vad_config is non-
     * default) uses the VAD to skip silent segments before inference.
     * Passing nullptr disables VAD.
     */
    void setVoiceActivityDetector(std::unique_ptr<IVoiceActivityDetector> vad,
                                  const VadConfig& cfg = {});

private:
    // Applies VAD: returns only speech samples if a VAD is installed, otherwise pcm unchanged.
    std::vector<float> applyVad(const std::vector<float>& pcm, float sample_rate) const;

    std::unique_ptr<IWhisperTranscriber> transcriber_;
    std::unique_ptr<IAudioChunkReader>   reader_;
    std::unique_ptr<IVoiceActivityDetector> vad_;
    VadConfig vad_cfg_;
    std::atomic<bool>     initialized_{false};
    std::atomic<uint64_t> transcription_count_{0};
    std::atomic<uint64_t> error_count_{0};
    std::string model_path_;
    WhisperConfig cfg_;                         ///< config snapshot from initialize()
    std::string last_error_message_;
    mutable std::mutex transcriber_mutex_;      ///< serializes transcriber calls
    mutable std::mutex vad_mutex_;              ///< guards vad_ and vad_cfg_ for thread-safe swap
    mutable std::mutex error_mutex_;            ///< guards last_error_message_
};

} // namespace whisper
} // namespace themis

// Export symbols for dynamic loading
THEMIS_AUDIO_PLUGIN();
