/**
 * @file audio_preprocessing.h
 * @brief Audio Input Pipeline — Frozen API Contract for Phase 1.
 *
 * @version v1.0 frozen as of 2026-08-08
 *
 * Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Design/API Contract Frozen (Phase 1)
 *
 * ## Audio Validation Rules (Frozen)
 *
 * **Frame Size Constraints:**
 * - Minimum: 512 bytes (8ms @ 16kHz, mono, 16-bit)
 * - Maximum: 524288 bytes (512 KB)
 * - Recommended: 32 KB (standard WebRTC frame)
 *
 * **Codec Support:**
 * - PCM16 (16-bit PCM, 8–48 kHz)
 * - OPUS (variable bitrate, 8–48 kHz)
 * - WEBM_OPUS (WebM container with OPUS)
 *
 * **Preprocessing Chain Contract:**
 * ```
 * Raw Audio
 *    ↓
 * [Resampling] → normalize to target_sample_rate
 *    ↓
 * [RNNoise Suppression] (if enabled)
 *    ↓
 * [Noise Reduction] (if enabled)
 *    ↓
 * [Echo Cancellation] (if enabled, requires reference signal)
 *    ↓
 * [Voice Activity Detection] → detect voice/silence ratio
 *    ↓
 * [Normalization] → target RMS energy
 *    ↓
 * Processed Audio (ready for STT)
 * ```
 *
 * ## Error Codes (Voice Module — Audio)
 * - 6700: Audio frame too small
 * - 6701: Audio frame too large (exceeds kMaxAudioFrameSizeBytes)
 * - 6702: Unsupported codec
 * - 6703: Invalid sample rate
 * - 6704: Malformed audio data
 * - 6705: Preprocessing pipeline error
 * - 6706-6799: Reserved for future audio-related errors
 *
 * ## Thread Safety
 * AudioPreprocessingPipeline and NoiseSuppressor are NOT thread-safe.
 * Each stream/session should have its own instance.
 * NoiseSuppressor is move-only to enforce single ownership.
 */

/*
 * ThemisDB | File: audio_preprocessing.h | Version: v1.0 FROZEN
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Status: Design/API Contract Frozen (Phase 1)
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// Audio preprocessing pipeline for Phase 1 production readiness
// ============================================================================
// PHASE 1 CONTRACT FREEZE: This file documents the immutable audio validation
// and preprocessing chain contract.
// ============================================================================
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

// ============================================================================
// Audio Constants and Limits (Frozen — Phase 1 Contract)
// ============================================================================

/// @brief Maximum audio frame size in bytes (512 KB frozen limit).
/// Prevents denial-of-service via oversized payloads.
/// @note Minimum realistic frame: 512 bytes (~8ms @ 16kHz, mono, 16-bit)
constexpr uint32_t kMaxAudioFrameSizeBytes = 512 * 1024;  // 512 KB

/// @brief Minimum audio frame size for meaningful processing.
/// Frames smaller than this are rejected as malformed.
constexpr uint32_t kMinAudioFrameSizeBytes = 512;  // ~8ms @ 16kHz, mono, 16-bit

/// @brief Minimum sample rate (Hz).
/// Frames with sample_rate < 8 kHz are rejected as malformed.
constexpr int kMinSampleRateHz = 8000;

/// @brief Maximum sample rate (Hz).
/// Frames with sample_rate > 48 kHz are rejected (frozen limit).
/// Standard models train on 8–16 kHz; up to 48 kHz accepted but downsampled.
constexpr int kMaxSampleRateHz = 48000;

/// @brief Canonical/frozen target sample rate (Hz).
/// All preprocessing normalizes to this rate unless explicitly overridden.
constexpr int kNominalSampleRateHz = 16000;

/// @brief Supported audio codecs (frozen enumeration).
/// Additional codecs require Phase 1 amendment with implementation review.
enum class SupportedAudioCodec : uint8_t {
    PCM16,       ///< 16-bit PCM (most common, frozen)
    OPUS,        ///< Opus VBR (frozen)
    WEBM_OPUS,   ///< WebM container with Opus (frozen)
};

// Backward-compat alias for existing code
enum class AudioCodec {
    PCM16,      // Linear PCM, 16-bit
    PCM32,      // Linear PCM, 32-bit
    OPUS,       // Opus codec
    AAC,        // AAC codec
    FLAC,       // FLAC codec
    UNKNOWN
};

// Audio validation result (Phase 3)
struct AudioValidationResult {
    bool valid = false;
    std::string error_message;
    AudioCodec detected_codec = AudioCodec::UNKNOWN;
    int detected_sample_rate = 0;
    int detected_channels = 0;
    int detected_bits_per_sample = 0;
    bool is_malformed_frame_header = false;
    bool is_truncated = false;
    bool is_overflow_attempt = false;
};

// Audio sample format
/// @struct AudioFrame
/// @brief PCM audio with sample rate and timing information.
/// Represents a single audio frame after decoding from transport format.
struct AudioFrame {
    std::vector<float> samples;      ///< Float samples [-1.0, +1.0]
    int sample_rate = 16000;         ///< Hz (frozen nominal: 16 kHz)
    int channels = 1;                ///< Mono (frozen: 1 channel)
    int64_t timestamp_ms = 0;        ///< Wall-clock milliseconds at capture
};

// Preprocessing options
/// @struct PreprocessingOptions
/// @brief Configuration for AudioPreprocessingPipeline.
/// All enabled pipelines are chained in documented order (see file header).
struct PreprocessingOptions {
    /// @brief Enable noise reduction (spectral gate or deep-learning).
    /// Default: true. When false, noise_reduction_strength is ignored.
    bool enable_noise_reduction = true;

    /// @brief Enable echo cancellation (requires reference signal).
    /// Default: false. When true, applyEchoCancellation() must be called with reference.
    bool enable_echo_cancellation = false;

    /// @brief Enable voice activity detection to quantify speech fraction.
    /// Default: true. Provides voice_activity_ratio in PreprocessingResult.
    bool enable_vad = true;

    /// @brief Enable RMS-based normalization to target_rms.
    /// Default: true. Prevents clipping and normalizes dynamic range.
    bool enable_normalization = true;

    /// @brief Enable RNNoise deep-learning suppression (frozen feature).
    /// Default: false. Requires THEMIS_ENABLE_RNNOISE CMake flag.
    /// When enabled, RNNoise replaces spectral-gate noise reduction.
    bool enable_rnnoise_suppression = false;

    /// @brief VAD confidence threshold [0.0, 1.0].
    /// Frames below this threshold are considered silence.
    /// Default: 0.5f (50% confidence = voice).
    float vad_threshold = 0.5f;

    /// @brief Noise reduction strength [0.0, 1.0].
    /// Higher values = more aggressive suppression (may distort speech).
    /// Default 0.7f is a balanced setting for optimal quality.
    float noise_reduction_strength = 0.7f;

    /// @brief RNNoise VAD gate threshold [0.0, 1.0].
    /// Frames below this are attenuated to silence (frozen feature).
    /// Default: 0.5f (50% confidence = voice).
    float rnnoise_vad_threshold = 0.5f;

    /// @brief Target RMS for normalization [0.01, 0.5].
    /// Default: 0.1f (10% of full scale).
    /// Higher values preserve more dynamic range but risk clipping.
    float target_rms = 0.1f;

    /// @brief Target sample rate after resampling (Hz).
    /// Default: 16000 Hz (frozen nominal).
    /// Standard for speech models; changing requires Phase 1 amendment.
    int target_sample_rate = 16000;
};

// Preprocessing result
/// @struct PreprocessingResult
/// @brief Outcome of processFrame() or process().
/// Contains diagnostics to inform caller of pipeline quality metrics.
struct PreprocessingResult {
    /// @brief true if preprocessing succeeded; false if pipeline error occurred.
    bool success = false;

    /// @brief Error description when success=false.
    /// Maps to error codes [6700-6799] (see file header).
    std::string error_message;

    /// @brief Processed audio frame (populated when success=true).
    AudioFrame processed_audio;

    /// @brief Estimated noise floor [0.0, 1.0] after preprocessing.
    /// High values indicate residual noise; consider tuning noise_reduction_strength.
    float detected_noise_level = 0.0f;

    /// @brief Fraction of audio with detected voice [0.0, 1.0].
    /// 0 = pure silence; 1 = pure voice. Helps detect inactive sessions.
    float voice_activity_ratio = 0.0f;

    /// @brief RNNoise mean VAD probability (when RNNoise enabled).
    /// Mean of per-frame VAD outputs [0.0, 1.0]. Complements voice_activity_ratio.
    float rnnoise_vad_probability = 0.0f;

    /// @brief Wall-clock milliseconds spent in preprocessing.
    /// Useful for monitoring real-time performance on target hardware.
    int64_t processing_time_ms = 0;

    /// @brief Diagnostic metadata (sampling rates, filter settings, etc.).
    /// JSON structure varies by pipeline configuration.
    json diagnostics;
};

// Confidence scoring result for STT
/// @struct ConfidenceScore
/// @brief Confidence estimate for STT result quality.
/// Combines acoustic and language-model confidence.
struct ConfidenceScore {
    /// @brief Overall confidence [0.0, 1.0].
    /// Geometric mean of acoustic and language_model confidences.
    float overall = 0.0f;

    /// @brief Acoustic model confidence [0.0, 1.0].
    /// Reflects how well the audio matches known phonemes.
    float acoustic = 0.0f;

    /// @brief Language model confidence [0.0, 1.0].
    /// Reflects how well the transcript matches expected language patterns.
    float language_model = 0.0f;

    /// @brief Categorical quality level.
    /// Values: "high" (>0.8), "medium" (0.5-0.8), "low" (<0.5).
    std::string quality_level;
};

// Language detection result
/// @struct LanguageDetectionResult
/// @brief Detected language(s) from audio features.
struct LanguageDetectionResult {
    /// @brief Detected BCP-47 language code (e.g., "en", "fr", "es").
    /// Primary detection result.
    std::string detected_language;

    /// @brief Confidence [0.0, 1.0] that detected_language is correct.
    /// Used for filtering low-confidence results.
    float confidence = 0.0f;

    /// @brief Ranked alternatives: (language_code, confidence) pairs.
    /// Useful for fallback or presenting options to user.
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
    
    // Phase 3: Exhaustive input validation (fail-closed)
    AudioValidationResult validateAudioPayload(const std::vector<uint8_t>& raw_audio, 
                                               int declared_sample_rate,
                                               int declared_channels,
                                               int declared_bits_per_sample);
    
    // Phase 3: Codec validation (whitelist-based)
    bool isCodecSupported(AudioCodec codec) const;
    AudioCodec detectCodecFromHeader(const std::vector<uint8_t>& raw_audio) const;
    
    // Phase 3: Frame header validation (detect malformed/truncated data)
    bool validateFrameHeader(const std::vector<uint8_t>& raw_audio) const;
    
    // Phase 3: Overflow detection (fuzzing-aware)
    bool detectOverflowAttempt(const std::vector<uint8_t>& raw_audio) const;

    // Statistics
    json getStatistics() const;
    void resetStatistics();

private:
    PreprocessingOptions opts_;
    NoiseSuppressor noise_suppressor_;
    uint64_t frames_processed_ = 0;
    uint64_t total_processing_time_ms_ = 0;
    
    // Phase 3: Validation statistics
    uint64_t validation_errors_ = 0;
    uint64_t malformed_frames_ = 0;
    uint64_t truncation_attempts_ = 0;
    uint64_t overflow_attempts_ = 0;

    float computeRMS(const std::vector<float>& samples) const;
    float computeNoiseFloor(const std::vector<float>& samples) const;
    std::vector<float> applyHighPassFilter(const std::vector<float>& samples, float cutoff_hz, int sample_rate) const;
    std::vector<float> convertRawToFloat(const std::vector<uint8_t>& raw, int bits_per_sample = 16) const;
};

}} // namespace themis::voice
