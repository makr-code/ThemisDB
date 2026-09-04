/**
 * @file voice_liveness_checker.h
 * @brief Liveness detection and anti-spoof verification for voice sessions.
 * @version 1.0.0
 * @date 2026-08-16
 * 
 * Wave A-8 Voice hardening: implements fail-closed liveness detection
 * and anti-spoof mechanisms to verify audio authenticity and session validity.
 * 
 * Detects:
 * - Replayed audio (voice patterns)
 * - Synthetic/generated speech
 * - Spoofing attempts
 * - Invalid session states
 * 
 * @see src/voice/ROADMAP.md § Wave A Scope for voice
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <stdexcept>

namespace themis {
namespace voice {

// =============================================================================
// Liveness Error Types
// =============================================================================

/**
 * @brief Exception thrown when liveness check fails.
 * 
 * Indicates potential spoofing, replay, or authenticity issue.
 */
class LivenessCheckFailedError : public std::runtime_error {
public:
    /**
     * @brief Construct liveness check error.
     * 
     * @param reason Description of liveness failure
     * @param confidence Confidence level (0-100) of the detection
     */
    LivenessCheckFailedError(const std::string& reason, uint8_t confidence)
        : std::runtime_error("Liveness check failed: " + reason + 
                           " (confidence=" + std::to_string(confidence) + "%)"),
          confidence_(confidence) {}

    [[nodiscard]] uint8_t confidence() const noexcept { return confidence_; }

private:
    uint8_t confidence_;
};

// =============================================================================
// Liveness Detection Policies
// =============================================================================

/**
 * @brief Configuration for liveness detection thresholds and behavior.
 */
struct LivenessPolicy {
    /// Confidence threshold for liveness rejection (0-100).
    /// If detected liveness confidence is below this, audio is accepted.
    static constexpr uint8_t MIN_LIVENESS_CONFIDENCE_THRESHOLD = 60;
    
    /// Confidence threshold for spoof detection (0-100).
    /// If spoof confidence is above this, audio is rejected.
    static constexpr uint8_t MAX_SPOOF_CONFIDENCE_THRESHOLD = 40;
    
    /// Maximum time between consecutive audio chunks (5 seconds).
    static constexpr uint32_t MAX_SILENCE_MS = 5000;
    
    /// Minimum audio level (dB SPL) to be considered live speech.
    static constexpr int8_t MIN_AUDIO_LEVEL_DBSPL = -80;
    
    /// Maximum audio level (dB SPL) before clipping/saturation suspected.
    static constexpr int8_t MAX_AUDIO_LEVEL_DBSPL = 90;
    
    /// Minimum frequency variation in legitimate speech (Hz).
    static constexpr uint16_t MIN_FREQUENCY_VARIATION = 100;
    
    /// Maximum sustained frequency (indicates synthetic speech).
    static constexpr uint16_t MAX_SUSTAINED_FREQUENCY_MS = 500;
    
    /// Number of historical sessions to check for replay patterns.
    static constexpr uint32_t MAX_SESSION_HISTORY_FOR_REPLAY_CHECK = 10;
};

// =============================================================================
// Liveness Detection Results
// =============================================================================

/**
 * @brief Result of a liveness check on audio chunk.
 */
struct LivenessCheckResult {
    /// True if audio appears to be genuine live speech.
    bool is_live = false;
    
    /// Confidence level for liveness (0-100, higher = more confident it's live).
    uint8_t liveness_confidence = 0;
    
    /// Confidence level for spoof detection (0-100, higher = more likely spoof).
    uint8_t spoof_confidence = 0;
    
    /// Reason for pass/fail.
    std::string reason;
    
    /// Diagnostic details (for logging).
    std::string diagnostic_details;
};

// =============================================================================
// Liveness Checker
// =============================================================================

/**
 * @brief Performs liveness detection and anti-spoof verification.
 * 
 * Validates that audio is:
 * 1. Live speech (not pre-recorded or synthetic)
 * 2. Not a replay of previous audio
 * 3. From the expected speaker
 * 4. Within expected acoustic characteristics
 */
class VoiceLivenessChecker {
public:
    /**
     * @brief Create a liveness checker for a voice session.
     * 
     * @param session_id Session identifier
     * @param speaker_profile Reference profile of expected speaker (optional)
     */
    explicit VoiceLivenessChecker(const std::string& session_id,
                                  const std::string& speaker_profile = "");

    /**
     * @brief Check if audio chunk is live and not spoofed.
     * 
     * Analyzes acoustic properties to detect:
     * - Replay attacks (same audio repeated)
     * - Synthetic/TTS speech
     * - Extreme audio conditions
     * - Silence or noise
     * 
     * @param audio_data Raw audio samples
     * @param audio_size Size of audio in bytes
     * @param sample_rate Sample rate in Hz
     * 
     * @return LivenessCheckResult with detection outcome
     * 
     * @throws std::invalid_argument if parameters are invalid
     */
    [[nodiscard]] LivenessCheckResult check_audio_chunk(
        const uint8_t* audio_data,
        size_t audio_size,
        uint32_t sample_rate);

    /**
     * @brief Check for replay attacks by comparing to session history.
     * 
     * Analyzes spectral and temporal features to detect if audio
     * matches previously seen patterns (replay detection).
     * 
     * @param audio_hash Computed hash of current audio chunk
     * @return true if replay is suspected; false if appears novel
     */
    [[nodiscard]] bool is_replay_detected(const std::string& audio_hash) noexcept;

    /**
     * @brief Check if audio is pure silence or noise.
     * 
     * @param audio_data Audio samples
     * @param audio_size Size in bytes
     * @param sample_rate Sample rate
     * 
     * @return true if silence/noise detected; false if has speech
     */
    [[nodiscard]] bool is_silence_or_noise_only(
        const uint8_t* audio_data,
        size_t audio_size,
        uint32_t sample_rate) const noexcept;

    /**
     * @brief Compute acoustic hash of audio chunk for replay detection.
     * 
     * @param audio_data Audio samples
     * @param audio_size Size in bytes
     * 
     * @return Hex string hash of audio features
     */
    [[nodiscard]] std::string compute_audio_hash(
        const uint8_t* audio_data,
        size_t audio_size) const noexcept;

    /**
     * @brief Reset liveness checker state (for new stream).
     */
    void reset() noexcept;

    /**
     * @brief Get session ID.
     */
    [[nodiscard]] const std::string& session_id() const noexcept { return session_id_; }

    /**
     * @brief Get number of chunks checked so far.
     */
    [[nodiscard]] uint32_t chunks_checked() const noexcept { return chunks_checked_; }

    /**
     * @brief Get number of chunks rejected due to liveness failure.
     */
    [[nodiscard]] uint32_t chunks_rejected() const noexcept { return chunks_rejected_; }

private:
    std::string session_id_;
    std::string speaker_profile_;
    uint32_t chunks_checked_;
    uint32_t chunks_rejected_;
    std::vector<std::string> audio_hash_history_;
    std::chrono::high_resolution_clock::time_point last_chunk_time_;
    
    // Helpers
    [[nodiscard]] uint8_t detect_spoof_indicators(
        const uint8_t* audio_data,
        size_t audio_size,
        uint32_t sample_rate) const noexcept;
    
    [[nodiscard]] uint8_t estimate_liveness_confidence(
        const uint8_t* audio_data,
        size_t audio_size,
        uint32_t sample_rate) const noexcept;
};

/**
 * @brief Helper to convert audio level (linear) to dB SPL.
 * 
 * @param linear_level Linear amplitude
 * @return Decibels SPL
 */
[[nodiscard]] inline int8_t level_to_dbspl(double linear_level) noexcept {
    if (linear_level <= 0) {
      return -120;
    }
    return static_cast<int8_t>(20.0 * std::log10(linear_level));
}

/**
 * @brief Helper to check if audio level is within expected range.
 * 
 * @param level_dbspl Level in dB SPL
 * @return true if level is valid; false if clipped/saturated
 */
[[nodiscard]] inline bool is_valid_audio_level(int8_t level_dbspl) noexcept {
    return level_dbspl >= LivenessPolicy::MIN_AUDIO_LEVEL_DBSPL &&
           level_dbspl <= LivenessPolicy::MAX_AUDIO_LEVEL_DBSPL;
}

}  // namespace voice
}  // namespace themis
