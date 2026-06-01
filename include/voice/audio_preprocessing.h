/*
 * ThemisDB | File: audio_preprocessing.h | Version: 0.0.42
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// Audio preprocessing pipeline for Phase 1 production readiness
// Noise reduction, echo cancellation, VAD, speaker diarization support
// RNNoise deep-learning noise suppression (Phase 3)
#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include <functional>
#include <memory>
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
    bool enable_rnnoise_suppression = false;  // RNNoise deep-learning noise suppression
    float vad_threshold = 0.5f;
    float noise_reduction_strength = 0.7f;
    float rnnoise_vad_threshold = 0.5f;  // VAD gate: frames below this are attenuated
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
    float rnnoise_vad_probability = 0.0f;  // RNNoise per-frame mean VAD probability
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

// ---------------------------------------------------------------------------
// NoiseSuppressor: RNNoise-backed deep-learning noise suppression.
//
// When the library is built with THEMIS_ENABLE_RNNOISE the implementation
// drives the real rnnoise C API (rnnoise_create / rnnoise_process_frame /
// rnnoise_destroy).  Without the flag a spectral-gate fallback is used so
// the rest of the pipeline compiles and runs without the external library.
//
// Usage:
//   NoiseSuppressor ns;
//   AudioFrame clean = ns.suppress(noisy_frame);
//   float p = ns.lastVadProbability();   // mean VAD from last suppress() call
// ---------------------------------------------------------------------------
class NoiseSuppressor {
public:
    using ProcessFramesFn = std::function<float(std::vector<float>&, float)>;

    NoiseSuppressor();
    ~NoiseSuppressor();

    // Non-copyable (RNNoise state is a unique resource)
    NoiseSuppressor(const NoiseSuppressor&) = delete;
    NoiseSuppressor& operator=(const NoiseSuppressor&) = delete;
    NoiseSuppressor(NoiseSuppressor&&) noexcept;
    NoiseSuppressor& operator=(NoiseSuppressor&&) noexcept;

    // Process a full audio frame through the noise suppressor.
    // Input/output: mono float samples, any sample rate (resampled internally
    // to 48 kHz as required by RNNoise, then resampled back).
    // vad_threshold: frames whose RNNoise VAD probability is below this value
    //                are attenuated (set to silence) to gate residual noise.
    AudioFrame suppress(const AudioFrame& frame, float vad_threshold = 0.5f);

    // Mean VAD probability reported by RNNoise for the last suppress() call.
    // Range [0, 1].  Returns 0 if no frame has been processed yet.
    float lastVadProbability() const { return last_vad_prob_; }

    // Returns true when the real RNNoise library is linked in.
    static bool isRNNoiseEnabled();

    static void setProcessFramesFn(ProcessFramesFn fn);

    // Diagnostic counters
    uint64_t framesProcessed() const { return frames_processed_; }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    float last_vad_prob_ = 0.0f;
    uint64_t frames_processed_ = 0;

    // Internal helpers
    static std::vector<float> resampleLinear(const std::vector<float>& in,
                                              int src_rate, int dst_rate);
    float processRNNoiseFrames(std::vector<float>& samples_48k,
                                float vad_threshold);
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

    // Deep-learning noise suppression via RNNoise (or spectral-gate fallback).
    // When enable_rnnoise_suppression is set in the options this is called
    // automatically by processFrame().  It can also be invoked directly.
    AudioFrame applyRNNoiseSuppression(const AudioFrame& frame,
                                        float vad_threshold = 0.5f);

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
    NoiseSuppressor noise_suppressor_;
    uint64_t frames_processed_ = 0;
    uint64_t total_processing_time_ms_ = 0;

    float computeRMS(const std::vector<float>& samples) const;
    float computeNoiseFloor(const std::vector<float>& samples) const;
    std::vector<float> applyHighPassFilter(const std::vector<float>& samples, float cutoff_hz, int sample_rate) const;
    std::vector<float> convertRawToFloat(const std::vector<uint8_t>& raw, int bits_per_sample = 16) const;
};

}} // namespace themis::voice
