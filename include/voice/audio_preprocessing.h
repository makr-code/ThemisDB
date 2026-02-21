// Audio preprocessing pipeline for Phase 1 production readiness
// Noise reduction, echo cancellation, VAD, speaker diarization support
#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include <functional>
#include <nlohmann/json.hpp>

namespace themis { namespace voice {
using json = nlohmann::json;

// Audio sample format
struct AudioFrame {
    std::vector<float> samples;
    int sample_rate = 16000;
    int channels = 1;
    int64_t timestamp_ms = 0;
};

// Preprocessing options
struct PreprocessingOptions {
    bool enable_noise_reduction = true;
    bool enable_echo_cancellation = false;
    bool enable_vad = true;          // Voice Activity Detection
    bool enable_normalization = true;
    float vad_threshold = 0.5f;
    float noise_reduction_strength = 0.7f;
    float target_rms = 0.1f;         // Target RMS for normalization
    int target_sample_rate = 16000;
};

// Preprocessing result
struct PreprocessingResult {
    bool success = false;
    std::string error_message;
    AudioFrame processed_audio;
    float detected_noise_level = 0.0f;
    float voice_activity_ratio = 0.0f;  // Fraction of audio with voice
    int64_t processing_time_ms = 0;
    json diagnostics;
};

// Confidence scoring result for STT
struct ConfidenceScore {
    float overall = 0.0f;
    float acoustic = 0.0f;
    float language_model = 0.0f;
    std::string quality_level;  // "high", "medium", "low"
};

// Language detection result
struct LanguageDetectionResult {
    std::string detected_language;
    float confidence = 0.0f;
    std::vector<std::pair<std::string, float>> alternatives;
};

// AudioPreprocessingPipeline: Phase 1 production component
class AudioPreprocessingPipeline {
public:
    explicit AudioPreprocessingPipeline(const PreprocessingOptions& opts = {});
    ~AudioPreprocessingPipeline() = default;

    // Core preprocessing
    PreprocessingResult process(const std::vector<uint8_t>& raw_audio, int source_sample_rate = 16000);
    PreprocessingResult processFrame(const AudioFrame& frame);

    // Noise reduction
    AudioFrame applyNoiseReduction(const AudioFrame& frame, float strength = 0.7f);

    // Echo cancellation
    AudioFrame applyEchoCancellation(const AudioFrame& input, const AudioFrame& reference);

    // Voice activity detection (returns fraction of active voice)
    float detectVoiceActivity(const AudioFrame& frame);

    // Audio normalization
    AudioFrame normalize(const AudioFrame& frame, float target_rms = 0.1f);

    // Sample rate conversion
    AudioFrame resample(const AudioFrame& frame, int target_sample_rate);

    // Confidence scoring based on audio quality
    ConfidenceScore scoreConfidence(const AudioFrame& frame);

    // Language detection from audio features
    LanguageDetectionResult detectLanguage(const AudioFrame& frame, const std::string& hint = "auto");

    // Statistics
    json getStatistics() const;
    void resetStatistics();

private:
    PreprocessingOptions opts_;
    uint64_t frames_processed_ = 0;
    uint64_t total_processing_time_ms_ = 0;

    float computeRMS(const std::vector<float>& samples) const;
    float computeNoiseFloor(const std::vector<float>& samples) const;
    std::vector<float> applyHighPassFilter(const std::vector<float>& samples, float cutoff_hz, int sample_rate) const;
    std::vector<float> convertRawToFloat(const std::vector<uint8_t>& raw, int bits_per_sample = 16) const;
};

}} // namespace themis::voice
