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

// ============================================================================
// NoiseSuppressor (RNNoise-backed — Frozen Feature)
// ============================================================================

/// @class NoiseSuppressor
/// @brief Deep-learning noise suppression via RNNoise or fallback.
///
/// When compiled with THEMIS_ENABLE_RNNOISE, drives the real rnnoise C library.
/// Without the flag, a spectral-gate fallback provides basic suppression.
///
/// **Usage:**
/// ```cpp
/// NoiseSuppressor ns;
/// AudioFrame clean = ns.suppress(noisy_frame, vad_threshold);
/// float vad_prob = ns.lastVadProbability();  // mean VAD from last call
/// ```
///
/// **Thread Safety:** NOT thread-safe. Each stream should have its own instance.
/// Move-only to enforce single ownership and prevent use-after-move bugs.
///
/// **Implementation Note:**
/// When enable_rnnoise_suppression is true in PreprocessingOptions,
/// AudioPreprocessingPipeline automatically instantiates NoiseSuppressor
/// and calls suppress() on each frame.
class NoiseSuppressor {
public:
    /// @brief Callback function type for frame-by-frame processing (testing).
    /// Receives mutable sample buffer and VAD threshold; returns mean VAD probability.
    using ProcessFramesFn = std::function<float(std::vector<float>&, float)>;

    /// @brief Construct a new noise suppressor instance.
    NoiseSuppressor();
    ~NoiseSuppressor();

    // Non-copyable (RNNoise state is a unique resource)
    NoiseSuppressor(const NoiseSuppressor&) = delete;
    NoiseSuppressor& operator=(const NoiseSuppressor&) = delete;

    // Move-only to enforce single ownership
    NoiseSuppressor(NoiseSuppressor&&) noexcept;
    NoiseSuppressor& operator=(NoiseSuppressor&&) noexcept;

    /// @brief Process a full audio frame through noise suppression.
    ///
    /// @param frame Input AudioFrame (mono preferred; multi-channel downmixed internally).
    /// @param vad_threshold VAD probability gate [0.0, 1.0].
    ///   Frames below this threshold are attenuated to silence.
    ///   Default: 0.5f (50% confidence = voice).
    ///
    /// @pre frame.channels should be 1 (mono)
    /// @post Output frame resampled back to input sample rate
    /// @post last_vad_prob_ updated with mean VAD probability
    /// @post frames_processed_ counter incremented
    ///
    /// @return Processed AudioFrame with noise attenuated via RNNoise or fallback.
    /// @error 6705 Processing pipeline error (internal RNNoise failure)
    AudioFrame suppress(const AudioFrame& frame, float vad_threshold = 0.5f);

    /// @brief Mean VAD probability from last suppress() call.
    ///
    /// Reflects how confident RNNoise is that the frame contains voice.
    /// Use alongside voice_activity_ratio for robust voice detection.
    ///
    /// @return Range [0.0, 1.0]. Returns 0 if no frame processed yet.
    float lastVadProbability() const { return last_vad_prob_; }

    /// @brief Check if real RNNoise library is linked.
    ///
    /// @return true when compiled with THEMIS_ENABLE_RNNOISE and library available;
    ///         false when fallback spectral gate is active.
    /// @note Useful for logging/debugging which suppression backend is active.
    static bool isRNNoiseEnabled();

    /// @brief Inject custom frame processor (testing only).
    ///
    /// @param fn Callable(samples, vad_threshold) → mean VAD probability.
    /// @note Reserved for unit tests; production code uses the default implementation.
    static void setProcessFramesFn(ProcessFramesFn fn);

    /// @brief Get diagnostic frame counter.
    /// @return Number of frames processed so far (monotonically increasing).
    /// @note Never decrements; useful for monitoring suppressor health.
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

// ============================================================================
// AudioPreprocessingPipeline (Main API — Frozen)
// ============================================================================

/// @class AudioPreprocessingPipeline
/// @brief Audio preprocessing chain orchestrator (frozen Phase 1 API).
///
/// Chains together resampling, noise suppression, echo cancellation, VAD,
/// and normalization in a fixed order (see file header for chain diagram).
///
/// **Thread Safety:** NOT thread-safe. Each stream should have its own instance.
/// Use one instance per streaming session.
///
/// **Example:**
/// ```cpp
/// PreprocessingOptions opts;
/// opts.enable_rnnoise_suppression = true;
/// opts.enable_vad = true;
/// AudioPreprocessingPipeline pipeline(opts);
///
/// std::vector<uint8_t> raw_audio = ...; // incoming WebSocket frame
/// auto result = pipeline.process(raw_audio, 16000);
/// if (result.success) {
///   float vad_ratio = result.voice_activity_ratio;
///   audio_model.transcribe(result.processed_audio);
/// } else {
///   spdlog::error("Preprocessing failed: {}", result.error_message);
/// }
/// ```
///
/// **Error Handling:** All methods use fail-closed semantics:
/// - process() returns success=false and error_message on any pipeline error
/// - Methods return sensible defaults or empty results on failure
/// - No exceptions are thrown
class AudioPreprocessingPipeline {
public:
    /// @brief Construct with preprocessing options (frozen).
    ///
    /// @param opts Configuration (defaults to sensible safe values).
    ///   All options are applied to subsequent process() calls.
    explicit AudioPreprocessingPipeline(const PreprocessingOptions& opts = {});
    ~AudioPreprocessingPipeline() = default;

    /// @brief Process raw audio bytes (e.g., from WebSocket frame).
    ///
    /// Decodes, resamples, and applies preprocessing chain to raw audio.
    ///
    /// @param raw_audio Encoded audio bytes (PCM16, OPUS, WEBM_OPUS, etc.).
    /// @param source_sample_rate Sample rate of raw_audio in Hz (default 16 kHz).
    ///
    /// @pre raw_audio.size() must be > 512 and <= kMaxAudioFrameSizeBytes
    /// @pre source_sample_rate must be in [kMinSampleRateHz, kMaxSampleRateHz]
    /// @post Preprocessed audio in result.processed_audio on success
    ///
    /// @return PreprocessingResult with processed_audio (or diagnostics on error).
    /// @error 6700 Audio frame too small
    /// @error 6701 Audio frame too large
    /// @error 6703 Invalid sample rate
    /// @error 6704 Malformed audio data (e.g., corrupt PCM)
    /// @error 6705 Preprocessing pipeline error
    PreprocessingResult process(const std::vector<uint8_t>& raw_audio, int source_sample_rate = 16000);

    /// @brief Process an already-decoded AudioFrame.
    ///
    /// Applies preprocessing chain directly to decoded AudioFrame
    /// (skips codec decoding step).
    ///
    /// @param frame AudioFrame after decoding from transport format.
    ///
    /// @return PreprocessingResult with processed_audio on success.
    /// @error 6705 Preprocessing pipeline error
    PreprocessingResult processFrame(const AudioFrame& frame);

    /// @brief Apply noise reduction to a frame (standalone).
    ///
    /// Can be called directly or invoked automatically by processFrame()
    /// when enable_noise_reduction=true in PreprocessingOptions.
    ///
    /// @param frame Input AudioFrame.
    /// @param strength Reduction strength [0.0, 1.0]. Default 0.7f.
    ///   Higher = more aggressive (may distort speech).
    ///
    /// @return AudioFrame with noise suppressed via spectral gate.
    /// @note Not affected by enable_rnnoise_suppression setting.
    AudioFrame applyNoiseReduction(const AudioFrame& frame, float strength = 0.7f);

    /// @brief Apply RNNoise-based noise suppression (standalone).
    ///
    /// Can be called directly or invoked automatically by processFrame()
    /// when enable_rnnoise_suppression=true.
    ///
    /// @param frame Input AudioFrame (mono preferred).
    /// @param vad_threshold VAD gate [0.0, 1.0]. Default 0.5f.
    ///
    /// @return AudioFrame with RNNoise suppression applied.
    /// @note Falls back to spectral gate if THEMIS_ENABLE_RNNOISE not set.
    /// @note Internally resamples to 48 kHz, processes, then back to original rate.
    AudioFrame applyRNNoiseSuppression(const AudioFrame& frame,
                                        float vad_threshold = 0.5f);

    /// @brief Apply echo cancellation (standalone).
    ///
    /// Removes/attenuates echo from microphone signal using speaker reference.
    /// NOT automatically invoked by processFrame().
    ///
    /// @param input Microphone audio frame (near-end).
    /// @param reference Speaker/reference audio frame (far-end).
    ///
    /// @return AudioFrame with echo attenuated.
    /// @pre input and reference sample rates must match
    /// @note Caller is responsible for feeding reference signal
    ///       (typically from VoIP receive stream).
    AudioFrame applyEchoCancellation(const AudioFrame& input, const AudioFrame& reference);

    /// @brief Compute voice activity ratio (fraction of speech).
    ///
    /// Analyzes energy levels to estimate fraction of audio containing speech.
    /// Uses simple energy thresholding; true speech/silence detection is future work.
    ///
    /// @param frame AudioFrame to analyze.
    ///
    /// @return Ratio [0.0, 1.0]. 0 = pure silence; 1 = pure voice.
    /// @note Part of automatic preprocessing chain when enable_vad=true.
    float detectVoiceActivity(const AudioFrame& frame);

    /// @brief Normalize audio to target RMS energy.
    ///
    /// Scales sample values to achieve desired RMS level (prevents clipping,
    /// normalizes dynamic range).
    ///
    /// @param frame Input AudioFrame.
    /// @param target_rms Target RMS [0.01, 0.5]. Default 0.1f (10% full scale).
    ///
    /// @return Normalized AudioFrame.
    /// @note Part of automatic preprocessing chain when enable_normalization=true.
    AudioFrame normalize(const AudioFrame& frame, float target_rms = 0.1f);

    /// @brief Resample audio to target sample rate.
    ///
    /// Uses linear interpolation for resampling (simple but sufficient for speech).
    ///
    /// @param frame Input AudioFrame.
    /// @param target_sample_rate Target Hz (frozen nominal: 16000).
    ///
    /// @return Resampled AudioFrame.
    /// @error 6703 Invalid target sample rate
    /// @note Part of automatic preprocessing chain (initial step).
    AudioFrame resample(const AudioFrame& frame, int target_sample_rate);

    /// @brief Score transcription confidence from audio quality.
    ///
    /// Estimates STT confidence by analyzing preprocessed audio quality
    /// (SNR, spectral stability, etc.). Should be called on processed_audio
    /// for best accuracy.
    ///
    /// @param frame AudioFrame to analyze.
    ///
    /// @return ConfidenceScore combining acoustic and language model scores.
    /// @note Intended for post-processing result (not real-time feedback).
    ConfidenceScore scoreConfidence(const AudioFrame& frame);

    /// @brief Detect language from audio features.
    ///
    /// Analyzes spectral and prosodic features to identify language.
    /// Not guaranteed to be accurate; consider using STT model's language detection.
    ///
    /// @param frame AudioFrame to analyze.
    /// @param hint Hint language code (e.g., "en", "fr", "auto" for no hint).
    ///
    /// @return LanguageDetectionResult with detected language and alternatives.
    LanguageDetectionResult detectLanguage(const AudioFrame& frame, const std::string& hint = "auto");
    
    /// @brief Validate audio payload (Phase 3 exhaustive validation).
    ///
    /// @param raw_audio Raw audio bytes.
    /// @param declared_sample_rate Claimed sample rate.
    /// @param declared_channels Claimed channel count.
    /// @param declared_bits_per_sample Claimed bits per sample.
    ///
    /// @return AudioValidationResult with details of validation outcome.
    AudioValidationResult validateAudioPayload(const std::vector<uint8_t>& raw_audio, 
                                               int declared_sample_rate,
                                               int declared_channels,
                                               int declared_bits_per_sample);
    
    /// @brief Check if codec is supported (whitelist-based).
    ///
    /// @param codec AudioCodec to check.
    /// @return true if codec is in the frozen supported list; false otherwise.
    bool isCodecSupported(AudioCodec codec) const;

    /// @brief Detect codec from audio header (heuristic).
    ///
    /// @param raw_audio Raw audio bytes.
    /// @return Detected AudioCodec, or UNKNOWN if detection failed.
    AudioCodec detectCodecFromHeader(const std::vector<uint8_t>& raw_audio) const;
    /// @brief Validate frame header (detect malformed/truncated data).
    ///
    /// Checks for consistency in audio frame header fields (sample rate, channels, etc.).
    ///
    /// @param raw_audio Raw audio bytes to inspect.
    /// @return true if frame header appears valid; false if malformed/truncated.
    /// @error 6704 Malformed audio data
    bool validateFrameHeader(const std::vector<uint8_t>& raw_audio) const;
    
    /// @brief Detect overflow attempts (fuzzing-aware).
    ///
    /// Checks for suspicious patterns that might indicate buffer overflow attempts
    /// or malicious fuzzing payloads.
    ///
    /// @param raw_audio Raw audio bytes.
    /// @return true if potential overflow/injection detected; false if benign.
    /// @error 6704 Malformed audio data
    bool detectOverflowAttempt(const std::vector<uint8_t>& raw_audio) const;

    /// @brief Get pipeline statistics (JSON).
    ///
    /// Returns aggregate statistics: frames processed, total time, error counts, etc.
    /// Useful for monitoring and performance tuning.
    ///
    /// @return JSON object with diagnostic statistics.
    json getStatistics() const;

    /// @brief Reset statistics counters.
    /// Clears all diagnostic counters (useful before starting a new session).
    void resetStatistics();

private:
    PreprocessingOptions opts_;
    NoiseSuppressor noise_suppressor_;
    uint64_t frames_processed_ = 0;
    uint64_t total_processing_time_ms_ = 0;
    
    // Phase 3: Validation statistics (for monitoring)
    uint64_t validation_errors_ = 0;
    uint64_t malformed_frames_ = 0;
    uint64_t truncation_attempts_ = 0;
    uint64_t overflow_attempts_ = 0;

    // Internal helpers (private implementation)
    float computeRMS(const std::vector<float>& samples) const;
    float computeNoiseFloor(const std::vector<float>& samples) const;
    std::vector<float> applyHighPassFilter(const std::vector<float>& samples, float cutoff_hz, int sample_rate) const;
    std::vector<float> convertRawToFloat(const std::vector<uint8_t>& raw, int bits_per_sample = 16) const;
};

}} // namespace themis::voice
