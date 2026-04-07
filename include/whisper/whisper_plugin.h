#pragma once

#include "plugins/audio_backend_interface.h"
#include "whisper/whisper_config.h"
#include "whisper/whisper_transcriber.h"
#include "whisper/audio_chunk_reader.h"
#include <memory>
#include <atomic>
#include <nlohmann/json.hpp>

namespace themis {
namespace whisper {

/**
 * @brief Top-level audio-transcription plugin that implements IAudioBackend.
 *
 * Wires together:
 *  - IWhisperTranscriber  (model inference, injected or created from type)
 *  - IAudioChunkReader    (file I/O, injected or defaults to WavAudioChunkReader)
 *
 * The default constructor selects WhisperCppTranscriber when
 * THEMIS_ENABLE_WHISPER is defined, otherwise WhisperStubTranscriber.
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

    bool isInitialized() const override { return initialized_; }

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
    bool     initialized_ = false;
    uint64_t transcription_count_ = 0;
    uint64_t error_count_         = 0;
    std::string model_path_;
};

} // namespace whisper
} // namespace themis

// Export symbols for dynamic loading
THEMIS_AUDIO_PLUGIN();
