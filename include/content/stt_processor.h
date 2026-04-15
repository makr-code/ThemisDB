/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            stt_processor.h                                    ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:02:22                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     218                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file stt_processor.h
 * @brief Speech-to-Text (STT) Processor Plugin
 * 
 * Provides speech recognition capabilities using Whisper.cpp for audio transcription.
 * Integrates with existing audio processor for phone calls, meetings, and voice commands.
 * 
 * @author ThemisDB Team
 * @date December 2025
 */

#pragma once

#include "content_plugin_interface.h"
#include <mutex>
#include <atomic>
#include <memory>

namespace themis {
namespace content {

/**
 * @brief Transcription segment with timestamp
 */
struct TranscriptionSegment {
    std::string text;
    int64_t start_ms;
    int64_t end_ms;
    float confidence;
    int speaker_id = -1;  // For speaker diarization
    json metadata;
};

/**
 * @brief Transcription result
 */
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

/**
 * @brief Speech-to-Text Processor using Whisper.cpp
 * 
 * Features:
 * - Multi-language transcription with automatic detection
 * - Timestamp generation for segments
 * - Speaker diarization support
 * - Real-time streaming transcription
 * - Meeting protocol generation
 * - Phone call transcription with metadata
 */
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
    
    /**
     * @brief Transcribe audio with detailed result
     * 
     * @param audio_blob Audio data in supported format
     * @param options Transcription options (language, timestamps, etc.)
     * @return Detailed transcription result
     */
    TranscriptionResult transcribe(
        const std::vector<uint8_t>& audio_blob,
        const json& options = {}
    );
    
    /**
     * @brief Stream transcription in real-time
     * 
     * @param audio_stream Audio data stream
     * @param callback Callback for each transcribed segment
     * @return true if streaming successful
     */
    bool streamTranscribe(
        const std::vector<uint8_t>& audio_stream,
        std::function<void(const TranscriptionSegment&)> callback
    );

    /**
     * @brief Assign speaker IDs to transcription segments via acoustic clustering.
     *
     * Extracts sub-band RMS + zero-crossing-rate feature vectors for each
     * segment's audio window and clusters them with k-means (cosine distance,
     * k-means++ seeding).  The number of speakers used is @p max_speakers when
     * > 0, otherwise min(4, segment_count).
     *
     * This method is also called automatically by transcribe() when the
     * "speaker_diarization" option is set to true.
     *
     * @param segments    Transcription segments with timestamp information.
     * @param pcm_data    Full-audio PCM samples (float, 16 kHz mono).
     * @param max_speakers Maximum number of speakers to cluster (0 = auto).
     * @return Segments with speaker_id filled in (0-based cluster index).
     *         Returns @p segments unchanged when fewer than 2 segments are
     *         provided or @p pcm_data is empty.
     */
    static std::vector<TranscriptionSegment> diarizeSegments(
        const std::vector<TranscriptionSegment>& segments,
        const std::vector<float>& pcm_data,
        int max_speakers = 0
    );
    
    /**
     * @brief Generate meeting protocol from audio
     * 
     * @param audio_blob Audio recording of meeting
     * @param options Protocol options (format, include speakers, etc.)
     * @return Structured meeting protocol
     */
    json generateMeetingProtocol(
        const std::vector<uint8_t>& audio_blob,
        const json& options = {}
    );

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
    
    // Internal methods
    bool loadWhisperModel();
    void unloadWhisperModel();
    
    std::vector<uint8_t> convertToWav16kHz(const std::vector<uint8_t>& audio_blob);
    std::vector<float> extractPCMData(const std::vector<uint8_t>& wav_data);
    
    TranscriptionResult transcribeInternal(
        const std::vector<float>& pcm_data,
        const json& options
    );
    
    std::vector<TranscriptionSegment> performSpeakerDiarization(
        const std::vector<TranscriptionSegment>& segments,
        const std::vector<float>& pcm_data
    );
    
    json formatAsProtocol(
        const TranscriptionResult& result,
        const json& options
    );
    
    std::string formatTimestamp(int64_t ms);
};

} // namespace content
} // namespace themis
