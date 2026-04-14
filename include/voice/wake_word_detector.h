/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            wake_word_detector.h                               ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-14 11:30:51                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     219                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 8ae8a4193b  2026-02-22  feat(voice): implement wake-word detection for hands-free... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file wake_word_detector.h
 * @brief Wake-word detection for hands-free activation
 *
 * Provides lightweight, always-on wake-word spotting that triggers the voice
 * pipeline without requiring a button press.  Detection is based on Voice
 * Activity Detection (VAD) energy gating combined with configurable keyword
 * matching.  A future model-based backend can be plugged in by replacing
 * the scoring strategy without changing the public API.
 *
 * Built-in wake words: "hey themis", "themis", "database"
 *
 * @author ThemisDB Team
 * @date February 2026
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
    WakeWordDetector(WakeWordDetector&&)                 = default;
    WakeWordDetector& operator=(WakeWordDetector&&)      = default;

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
     */
    bool removeWakeWord(const WakeWordID& id);

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
     */
    void setDetectionCallback(DetectionCallback callback);

    // -----------------------------------------------------------------------
    // Configuration & state
    // -----------------------------------------------------------------------

    /** @brief Update runtime configuration (thread-safe). */
    void setConfig(const WakeWordConfig& config);

    /** @brief Return a copy of the current configuration. */
    WakeWordConfig getConfig() const;

    /** @brief Reset internal audio buffer and cooldown timer. */
    void reset();

    /** @brief Return runtime statistics (detections, false positives, …). */
    json getStatistics() const;

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

    // Optional callback
    DetectionCallback detection_callback_;

    // Helpers
    float computeRMS(const std::vector<float>& samples) const;
    std::vector<float> pcmToFloat(const std::vector<uint8_t>& raw) const;
    float scorePhrase(const std::string& phrase,
                      const std::vector<float>& samples) const;
    int64_t nowMs() const;
};

} // namespace voice
} // namespace themis
