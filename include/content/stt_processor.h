/**
 * @file stt_processor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "content_plugin_interface.h"
#include <functional>
#include <mutex>
#include <atomic>
#include <memory>

namespace themis {
namespace content {

struct TranscriptionSegment {
    std::string text;
    int64_t start_ms;
    int64_t end_ms;
    float confidence;
    int speaker_id = -1;  // For speaker diarization
    json metadata;
};

struct TranscriptionResult {
    bool success = false;
    std::string error_message;
    
    std::string full_text;
    std::vector<TranscriptionSegment> segments;
    std::string detected_language;
    float average_confidence;
    
    int64_t processing_time_ms = 0;
    int64_t audio_duration_ms = 0;
};

using STTTranscribeFn = std::function<
    TranscriptionResult(const std::vector<float>& pcm_data, const json& options)>;

class STTProcessor : public IContentProcessorPlugin {
public:
    STTProcessor();
    ~STTProcessor() override;
    
    // IContentProcessorPlugin interface
    PluginInfo getInfo() const override;
    bool initialize(const PluginConfig& config) override;
    void shutdown() override;
    bool canProcess(const std::string& mime_type) const override;
    
    ContentExtractionResult extract(
        const std::vector<uint8_t>& blob,
        const std::string& mime_type,
        const ExtractionOptions& options = {}
    ) override;
    
    std::vector<ContentChunk> chunk(
        const ContentExtractionResult& result,
        int max_tokens,
        int overlap
    ) override;
    
    bool healthCheck() const override;
    json getStatistics() const override;
    
    TranscriptionResult transcribe(
        const std::vector<uint8_t>& audio_blob,
        const json& options = {}
    );
    
    bool streamTranscribe(
        const std::vector<uint8_t>& audio_stream,
        std::function<void(const TranscriptionSegment&)> callback
    );

    static std::vector<TranscriptionSegment> diarizeSegments(
        const std::vector<TranscriptionSegment>& segments,
        const std::vector<float>& pcm_data,
        int max_speakers = 0
    );
    
    json generateMeetingProtocol(
        const std::vector<uint8_t>& audio_blob,
        const json& options = {}
    );

    /**
     * @brief Set Transcribe Fn.
     * @param[in] fn Input parameter.
     */
    void setTranscribeFn(STTTranscribeFn fn);

private:
    // Configuration
    std::string model_path_;
    std::string model_size_ = "base";  // tiny, base, small, medium, large
    std::string default_language_ = "auto";
    bool enable_timestamps_ = true;
    bool enable_speaker_diarization_ = false;
    int max_speakers_ = 0;
    bool enable_word_confidence_ = false;
    float vad_threshold_ = 0.5f;  // Voice activity detection
    
    // Whisper context (opaque pointer to avoid exposing whisper.cpp headers)
    void* whisper_ctx_ = nullptr;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    std::atomic<uint64_t> transcriptions_completed_{0};
    std::atomic<uint64_t> total_audio_duration_ms_{0};
    std::atomic<uint64_t> total_processing_time_ms_{0};
    std::atomic<uint64_t> errors_{0};
    
    bool initialized_ = false;
    
    // Injected transcription backend (non-Whisper builds).
    // Protected by stats_mutex_ for thread-safe set/clear.
    STTTranscribeFn transcribe_fn_;

    // Internal methods
    /**
     * @brief Load Whisper Model.
     * @return True when the operation succeeds.
     */
    bool loadWhisperModel();
    /**
     * @brief Unload Whisper Model.
     */
    void unloadWhisperModel();
    
    /**
     * @brief Convert To Wav16k Hz.
     * @param[in] audio_blob Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> convertToWav16kHz(const std::vector<uint8_t>& audio_blob);
    /**
     * @brief Extract PCMData.
     * @param[in] wav_data Input parameter.
     * @return Return value.
     */
    std::vector<float> extractPCMData(const std::vector<uint8_t>& wav_data);
    
    /**
     * @brief Transcribe Internal.
     * @param[in] pcm_data Input parameter.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    TranscriptionResult transcribeInternal(
        const std::vector<float>& pcm_data,
        const json& options
    );
    
    /**
     * @brief Perform Speaker Diarization.
     * @param[in] segments Input parameter.
     * @param[in] pcm_data Input parameter.
     * @return Return value.
     */
    std::vector<TranscriptionSegment> performSpeakerDiarization(
        const std::vector<TranscriptionSegment>& segments,
        const std::vector<float>& pcm_data
    );
    
    /**
     * @brief Format As Protocol.
     * @param[in] result Input parameter.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    json formatAsProtocol(
        const TranscriptionResult& result,
        const json& options
    );
    
    /**
     * @brief Format Timestamp.
     * @param[in] ms Input parameter.
     * @return Return value.
     */
    std::string formatTimestamp(int64_t ms);
};

} // namespace content
} // namespace themis
