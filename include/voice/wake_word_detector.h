/**
 * @file wake_word_detector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.16
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis {
namespace voice {

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Data structures
// ---------------------------------------------------------------------------

/** Unique identifier for a registered wake word. */
using WakeWordID = std::string;

/**
 * @brief Result returned by processAudioChunk().
 */
struct WakeWordDetectionResult {
    bool        detected              = false; ///< True when wake word is confirmed
    WakeWordID  wake_word_id;                  ///< Which wake word fired (empty if none)
    float       confidence            = 0.0f;  ///< Score in [0, 1]
    int64_t     detection_timestamp_ms = 0;    ///< Wall-clock ms at detection
};

/**
 * @brief Configuration for a detection session.
 */
struct WakeWordConfig {
    float sensitivity        = 0.5f;   ///< Detection threshold [0=permissive .. 1=strict]
    int   buffer_length_ms   = 1500;   ///< Rolling audio buffer size in ms
    bool  continuous_listen  = true;   ///< Keep listening after a detection event
    int   cooldown_ms        = 1000;   ///< Minimum ms between successive detections
    int   sample_rate        = 16000;  ///< Expected PCM sample rate (Hz)
    float vad_min_energy     = 0.005f; ///< Minimum RMS energy for VAD gate
    
    // Phase 3: Confidence thresholds and safe defaults
    float confidence_threshold = 0.6f; ///< Minimum confidence to accept detection
    bool  reject_low_confidence = true;///< Reject detections below threshold
    bool  use_timeout_protection = true; ///< Enforce timeout with safe defaults
    int64_t detection_timeout_ms = 5000; ///< Max time for detection (5 seconds)
};

// ---------------------------------------------------------------------------
// WakeWordDetector
// ---------------------------------------------------------------------------

/**
 * @brief Lightweight wake-word detector for hands-free voice activation.
 *
 * Usage:
 * @code
 *   WakeWordConfig cfg;
 *   WakeWordDetector detector(cfg);
 *
 *   // Register built-in or custom wake words
 *   detector.addWakeWord("hey-themis", "hey themis");
 *   detector.addWakeWord("themis",     "themis");
 *
 *   // Feed 16-bit PCM chunks as they arrive from the microphone
 *   auto result = detector.processAudioChunk(pcm_bytes);
 *   if (result.detected) { ... }
 * @endcode
 */
class WakeWordDetector {
public:
    /**
     * @brief Callback invoked on each detection event (optional alternative to polling).
     */
    using DetectionCallback =
        std::function<void(const WakeWordDetectionResult&)>;

    /**
     * @brief Construct detector with given configuration.
     * @param config  Detection parameters (sensitivity, cooldown, …).
     */
    explicit WakeWordDetector(const WakeWordConfig& config = {});
    ~WakeWordDetector() = default;

    // Non-copyable, movable
    WakeWordDetector(const WakeWordDetector&)            = delete;
    WakeWordDetector& operator=(const WakeWordDetector&) = delete;
    WakeWordDetector(WakeWordDetector&&)                 noexcept = default;
    WakeWordDetector& operator=(WakeWordDetector&&)      noexcept = default;

    // -----------------------------------------------------------------------
    // Wake-word registration
    // -----------------------------------------------------------------------

    /**
     * @brief Register a wake word.
     *
     * @param id    Unique identifier (returned in DetectionResult::wake_word_id).
     * @param phrase  The phrase to listen for (case-insensitive).
     * @return true if registered successfully; false if @p id already exists.
     */
    bool addWakeWord(const WakeWordID& id, const std::string& phrase);

    /**
     * @brief Remove a previously registered wake word.
     * @return true if removed; false if not found.
     * @param[in] id Input parameter.
     */
    bool removeWakeWord(const WakeWordID& id);

     * @return Return value.
    /** @brief List all registered wake word IDs. */
    std::vector<WakeWordID> listWakeWords() const;

    // -----------------------------------------------------------------------
    // Audio processing
    // -----------------------------------------------------------------------

    /**
     * @brief Feed a raw PCM audio chunk for wake-word scanning.
     *
     * Expects 16-bit little-endian PCM at the sample rate configured via
     * WakeWordConfig::sample_rate.
     *
     * @param audio_chunk  Raw PCM bytes.
     * @return Detection result (detected == false if no wake word fired).
     */
    WakeWordDetectionResult processAudioChunk(
        const std::vector<uint8_t>& audio_chunk);

    /**
     * @brief Register a callback that is invoked synchronously on detection.
     *
     * At most one callback is active at a time; calling this again replaces
     * the previous one.  Pass nullptr to remove.
     * @param[in] callback Input parameter.
     */
    void setDetectionCallback(DetectionCallback callback);

    // -----------------------------------------------------------------------
    // Configuration & state
    // -----------------------------------------------------------------------

     * @param[in] config Input parameter.
    /** @brief Update runtime configuration (thread-safe). */
    void setConfig(const WakeWordConfig& config);

     * @return Return value.
    /** @brief Return a copy of the current configuration. */
    WakeWordConfig getConfig() const;

    /** @brief Reset internal audio buffer and cooldown timer. */
    void reset();

     * @return Return value.
    /** @brief Return runtime statistics (detections, false positives, …). */
    json getStatistics() const;
    
    // Phase 3: Confidence Thresholds and Safe Defaults
    
    /**
     * @brief @brief Check if confidence meets threshold (Phase 3) @param confidence Confidence score [0, 1] @return true if confidence >= threshold; false otherwise
     * @param[in] confidence Input parameter.
     * @return True on success.
     * @note Exception safety: noexcept.
     */
    bool meetsConfidenceThreshold(float confidence) const noexcept;
    
    /**
     * @brief @brief Detect timeout during wake-word processing (Phase 3) @return true if detector is experiencing timeout/delays
     * @return True on success.
     * @note Exception safety: noexcept.
     */
    bool isTimeoutDetected() const noexcept;
    
    /**
     * @brief @brief Get safe default result when detection times out (Phase 3) @return WakeWordDetectionResult with detected=false (safe default)
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    WakeWordDetectionResult getTimeoutDefault() const noexcept;

private:
    // Internals
    mutable std::mutex mutex_;
    WakeWordConfig     config_;

    struct WakeWord {
        WakeWordID  id;
        std::string phrase;        ///< lower-case normalized phrase
        std::vector<std::string> tokens; ///< space-split tokens
    };
    std::vector<WakeWord> wake_words_;

    // Rolling PCM sample buffer (float)
    std::vector<float> sample_buffer_;
    int64_t            last_detection_ms_ = 0;

    // Statistics
    uint64_t total_chunks_processed_ = 0;
    uint64_t total_detections_       = 0;
    uint64_t low_confidence_rejects_ = 0;  // Phase 3: confidence threshold rejections
    uint64_t timeout_fallbacks_      = 0;  // Phase 3: timeout fallback count

    // Optional callback
    DetectionCallback detection_callback_;
    
    // Phase 3: Timeout tracking
    int64_t last_processing_start_ms_ = 0;
    bool timeout_detected_ = false;

    /**
     * @brief Helpers
     * @param[in] samples Input parameter.
     * @return Return value.
     */
    float computeRMS(const std::vector<float>& samples) const;
    /**
     * @brief TBD: Describe pcmToFloat.
     * @param[in] raw Input parameter.
     * @return Return value.
     */
    std::vector<float> pcmToFloat(const std::vector<uint8_t>& raw) const;
    /**
     * @brief TBD: Describe scorePhrase.
     * @param[in] phrase Input parameter.
     * @param[in] samples Input parameter.
     * @return Return value.
     */
    float scorePhrase(const std::string& phrase,
                      const std::vector<float>& samples) const;
    /**
     * @brief TBD: Describe nowMs.
     * @return Return value.
     */
    int64_t nowMs() const;
};

} // namespace voice
} // namespace themis
