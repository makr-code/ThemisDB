/**
 * @file emotion_analyzer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis {
namespace voice {

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Enumerations
// ---------------------------------------------------------------------------

/**
 * @brief Emotion categories detected from voice characteristics.
 */
enum class Emotion {
    NEUTRAL,    ///< Calm, no strong emotional signal
    HAPPY,      ///< Elevated pitch, fast tempo, high energy
    SAD,        ///< Low pitch, slow tempo, low energy
    ANGRY,      ///< High energy, fast tempo, spectral tension
    SURPRISED,  ///< Sudden pitch spike, high energy burst
    FEARFUL,    ///< High pitch, irregular tempo, elevated ZCR
    DISGUSTED   ///< Low-mid pitch, low clarity, moderate energy
};

/**
 * @brief Sentiment polarity derived from voice tone.
 */
enum class Sentiment {
    POSITIVE,  ///< Generally pleasant / affirmative tone
    NEUTRAL,   ///< No strong polarity signal
    NEGATIVE   ///< Distressed, frustrated, or sad tone
};

// ---------------------------------------------------------------------------
// Helper: string conversion
// ---------------------------------------------------------------------------

/** @brief Return the name of an Emotion value as a lowercase string. */
std::string to_string(Emotion e);

/** @brief Return the name of a Sentiment value as a lowercase string. */
std::string to_string(Sentiment s);

// ---------------------------------------------------------------------------
// Data structures
// ---------------------------------------------------------------------------

/**
 * @brief Acoustic quality characteristics extracted from the audio.
 */
struct VoiceQuality {
    float pitch_hz        = 0.0f; ///< Estimated fundamental frequency (Hz)
    float pitch_variation = 0.0f; ///< Normalised pitch variability [0, 1]
    float tempo           = 0.0f; ///< Estimated syllable rate proxy (0–1)
    float volume_db       = -96.f; ///< Approximate loudness in dBFS
    float energy          = 0.0f; ///< Normalised RMS energy [0, 1]
    float clarity         = 0.0f; ///< Spectral clarity / harmonic ratio [0, 1]
};

/**
 * @brief Full emotion analysis result for a single audio buffer.
 */
struct EmotionAnalysis {
    std::map<Emotion, float> emotion_probabilities; ///< Score for every emotion [0, 1]
    Emotion primary_emotion   = Emotion::NEUTRAL;   ///< Highest-scoring emotion
    float   emotion_confidence = 0.0f;              ///< Confidence in primary emotion

    Sentiment sentiment       = Sentiment::NEUTRAL;  ///< Overall polarity
    float     sentiment_score = 0.0f;                ///< Continuous score [-1, +1]

    float stress_level     = 0.0f; ///< Stress indicator [0 = calm, 1 = stressed]
    float engagement_score = 0.0f; ///< Engagement level [0 = disengaged, 1 = engaged]

    VoiceQuality quality; ///< Underlying acoustic features
};

/**
 * @brief Single timed emotion data point within a conversation.
 */
struct TimedEmotion {
    int64_t   timestamp_ms   = 0;               ///< Start of segment (ms)
    Emotion   emotion        = Emotion::NEUTRAL; ///< Detected emotion
    float     confidence     = 0.0f;            ///< Detection confidence
    Sentiment sentiment      = Sentiment::NEUTRAL;
    float     sentiment_score = 0.0f;
};

/**
 * @brief Aggregated statistics across an emotion timeline.
 */
struct EmotionStatistics {
    Emotion dominant_emotion    = Emotion::NEUTRAL; ///< Most frequent emotion
    float   emotion_stability   = 1.0f; ///< 0–1; higher = more stable
    int     emotion_switches    = 0;    ///< Number of emotion transitions

    float average_sentiment  = 0.0f; ///< Mean sentiment score
    float sentiment_trend    = 0.0f; ///< Positive = improving, negative = declining

    float average_stress     = 0.0f; ///< Mean stress level
    float average_engagement = 0.0f; ///< Mean engagement score
};

/**
 * @brief Chronological emotion timeline for a multi-segment conversation.
 */
struct EmotionTimeline {
    std::vector<TimedEmotion> timeline;  ///< One entry per analysed segment
    EmotionStatistics         statistics;
    int64_t total_duration_ms = 0;       ///< Combined duration of all segments
};

/**
 * @brief Audio segment with position metadata, used for timeline analysis.
 */
struct AudioSegment {
    std::vector<uint8_t> audio_data; ///< Raw PCM bytes
    int64_t start_ms    = 0;         ///< Start offset in the conversation (ms)
    int64_t end_ms      = 0;         ///< End offset in the conversation (ms)
    std::string speaker_id;          ///< Optional speaker label
};

/**
 * @brief Configuration for emotion analysis.
 */
struct EmotionConfig {
    int   analysis_window_ms   = 1000; ///< Segment window for analysis (ms)
    float confidence_threshold = 0.6f; ///< Minimum confidence to report primary emotion
    bool  track_sentiment      = true; ///< Compute sentiment score
    bool  track_stress         = true; ///< Compute stress level
    bool  track_engagement     = true; ///< Compute engagement score
    
    // Phase 3: Edge case handling with safe defaults
    bool  skip_on_unavailable  = true;  ///< Skip emotion step if analyzer unavailable
    float fallback_confidence  = 0.0f;  ///< Default confidence when skipping
    bool  use_timeout_protection = true; ///< Enforce timeout with safe defaults
    int64_t timeout_ms = 5000;          ///< Max time for emotion analysis (5 seconds)
};

// ---------------------------------------------------------------------------
// EmotionAnalyzer
// ---------------------------------------------------------------------------

/**
 * @brief Acoustic emotion and sentiment analyser.
 *
 * Performs all analysis from raw PCM audio (16-bit little-endian, any sample
 * rate) using hand-crafted acoustic features:
 *
 * Feature set (per analysis window)
 * ----------------------------------
 * - Global RMS energy & crest factor
 * - Zero-crossing rate (tempo / energy proxy)
 * - 8 sub-band RMS values (spectral shape)
 * - Estimated pitch (Yin-style autocorrelation approximation)
 * - Spectral centroid & flatness
 * - High-frequency energy ratio (arousal indicator)
 *
 * Emotion scoring
 * ---------------
 * Each acoustic feature contributes to emotion scores via a hand-tuned
 * linear feature-to-emotion mapping.  The resulting raw scores are
 * softmax-normalised to produce proper probabilities.  The primary emotion
 * is the argmax; its probability is reported as the confidence.
 *
 * Sentiment mapping
 * -----------------
 * HAPPY / SURPRISED → positive bias
 * ANGRY / SAD / FEARFUL / DISGUSTED → negative bias
 * NEUTRAL → no bias
 * The sentiment score is a weighted sum of emotion probabilities.
 *
 * Thread safety
 * -------------
 * EmotionAnalyzer is stateless with respect to audio data; the same instance
 * can be used concurrently from multiple threads.  The statistics counters are
 * guarded by an internal mutex.
 */
class EmotionAnalyzer {
public:
    /**
     * @brief Construct with optional configuration.
     */
    explicit EmotionAnalyzer(const EmotionConfig& config = {});
    ~EmotionAnalyzer() = default;

    EmotionAnalyzer(const EmotionAnalyzer&)            = delete;
    EmotionAnalyzer& operator=(const EmotionAnalyzer&) = delete;
    EmotionAnalyzer(EmotionAnalyzer&&)                 = default;
    EmotionAnalyzer& operator=(EmotionAnalyzer&&)      = default;

    // -----------------------------------------------------------------------
    // Core analysis
    // -----------------------------------------------------------------------

    /**
     * @brief Analyse emotions in a single raw-PCM audio buffer.
     *
     * @param audio_data  Raw PCM bytes (16-bit LE, 16 kHz recommended).
     * @param config      Per-call configuration override; uses instance config
     *                    when omitted.
     * @return EmotionAnalysis on success; std::nullopt if audio is empty.
     */
    std::optional<EmotionAnalysis> analyze(
        const std::vector<uint8_t>& audio_data,
        const EmotionConfig&        config = {}) const;

    /**
     * @brief Track emotions across multiple time-stamped audio segments.
     *
     * Analyses each segment independently and assembles a timeline with
     * aggregate statistics.
     *
     * @param segments  Ordered list of AudioSegment (need not be contiguous).
     * @param config    Analysis configuration.
     * @return EmotionTimeline (may be empty when no segments are provided).
     */
    EmotionTimeline track(
        const std::vector<AudioSegment>& segments,
        const EmotionConfig&             config = {}) const;

    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    /** @brief Update default configuration. */
    void set_config(const EmotionConfig& config);

    /** @brief Return a copy of the current default configuration. */
    EmotionConfig get_config() const;
    
    // Phase 3: Edge Case Handling
    
    /// @brief Check if analyzer is available (Phase 3)
    /// @return true if ready to analyze; false if unavailable (skip with safe default)
    bool isAvailable() const noexcept;
    
    /// @brief Analyze with timeout protection (Phase 3)
    /// @param audio_data Raw PCM bytes
    /// @param config Analysis configuration
    /// @return EmotionAnalysis if successful; default neutral emotion on timeout
    std::optional<EmotionAnalysis> analyzeWithTimeout(
        const std::vector<uint8_t>& audio_data,
        const EmotionConfig& config = {}) const;

    // -----------------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------------

    /** @brief Return cumulative analysis statistics as JSON. */
    json get_statistics() const;

private:
    EmotionConfig config_;
    mutable uint64_t total_analyses_ = 0;
    mutable uint64_t timeout_fallbacks_ = 0;  // Phase 3: timeout count
    mutable bool is_available_ = true;        // Phase 3: availability flag

    // PCM helpers
    std::vector<float> pcmToFloat(const std::vector<uint8_t>& raw) const;

    // Acoustic feature extraction
    struct AcousticFeatures {
        float global_rms       = 0.0f;
        float crest_factor     = 0.0f;
        float zcr              = 0.0f; ///< Normalised zero-crossing rate
        float spectral_centroid = 0.0f;
        float spectral_flatness = 0.0f;
        float hf_ratio         = 0.0f; ///< High-frequency energy ratio
        float pitch_hz         = 0.0f;
        float pitch_variation  = 0.0f;
        std::vector<float> band_rms; ///< 8 sub-band RMS values
    };

    AcousticFeatures extractFeatures(const std::vector<float>& samples) const;

    // Emotion scoring
    std::map<Emotion, float> scoreEmotions(const AcousticFeatures& f) const;
    static std::map<Emotion, float> softmax(const std::map<Emotion, float>& raw);

    // Derived signals
    float computeSentimentScore(const std::map<Emotion, float>& probs) const;
    float computeStressLevel(const AcousticFeatures& f) const;
    float computeEngagementScore(const AcousticFeatures& f) const;
    VoiceQuality buildVoiceQuality(const AcousticFeatures& f) const;

    // Timeline helpers
    static EmotionStatistics computeStatistics(
        const std::vector<TimedEmotion>& entries,
        const std::vector<float>&        stress_levels,
        const std::vector<float>&        engagement_scores);
};

} // namespace voice
} // namespace themis
