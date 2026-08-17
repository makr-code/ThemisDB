/**
 * @file voice_audit_logger.h
 * @brief Voice Authentication Audit Logging — Compliance & Forensics
 *
 * @version v1.0
 * @note Maturity: 🟡 BETA
 * @note Status: Wave A Batch A-8 Implementation
 *
 * Provides comprehensive audit logging for voice authentication, session lifecycle,
 * and spoofing detection events. All events include:
 * - Timestamp (ISO 8601 format)
 * - User identifier
 * - Event type (AUTH_ATTEMPT, SESSION_CREATED, LIVENESS_CHALLENGE, SPOOF_DETECTED)
 * - Result (PASS/FAIL)
 * - Detailed context
 *
 * ## Logged Events
 *
 * ### VOICE_AUTH_ATTEMPT
 * ```json
 * {
 *     "timestamp": "2026-08-28T14:22:31.456Z",
 *     "event_type": "VOICE_AUTH_ATTEMPT",
 *     "user_id": "user-12345",
 *     "method": "liveness|password|2fa",
 *     "result": "PASS|FAIL",
 *     "duration_ms": 2314,
 *     "session_id": "sess-abc123",
 *     "reason": "Additional context"
 * }
 * ```
 *
 * ### VOICE_SESSION_LIFECYCLE
 * ```json
 * {
 *     "timestamp": "2026-08-28T14:22:31.456Z",
 *     "event_type": "VOICE_SESSION_LIFECYCLE",
 *     "session_id": "sess-abc123",
 *     "user_id": "user-12345",
 *     "event": "created|closed|timeout",
 *     "duration_ms": 123000,
 *     "bytes_transferred": 50000
 * }
 * ```
 *
 * ### VOICE_LIVENESS_CHALLENGE
 * ```json
 * {
 *     "timestamp": "2026-08-28T14:22:31.456Z",
 *     "event_type": "VOICE_LIVENESS_CHALLENGE",
 *     "user_id": "user-12345",
 *     "challenge_id": "ch-xyz789",
 *     "event": "issued|verified|expired",
 *     "passed": true,
 *     "reason": "Additional context"
 * }
 * ```
 *
 * ### VOICE_SPOOF_DETECTION
 * ```json
 * {
 *     "timestamp": "2026-08-28T14:22:31.456Z",
 *     "event_type": "VOICE_SPOOF_DETECTION",
 *     "user_id": "user-12345",
 *     "spoof_score": 0.92,
 *     "verdict": "spoofed|clean",
 *     "freshness_score": 0.3,
 *     "speaker_match_score": 0.8,
 *     "noise_consistency_score": 0.7,
 *     "reason": "Audio freshness check failed (likely synthetic)"
 * }
 * ```
 *
 * @error 7400: Audit logging failed
 */

#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>
#include <functional>
#include <nlohmann/json.hpp>

namespace themis {
namespace voice {

using json = nlohmann::json;

/**
 * @class VoiceAuditLogger
 * @brief Comprehensive audit logging for voice authentication events
 *
 * Provides structured logging of all security-relevant events:
 * - Authentication attempts (success/failure)
 * - Session lifecycle (create/close/timeout)
 * - Liveness challenges (issued/verified/expired)
 * - Spoofing detection (passed/failed)
 *
 * Thread-safe: all methods acquire internal mutex.
 * Logs are formatted as JSON for easy parsing and compliance reporting.
 */
class VoiceAuditLogger {
public:
    /// @brief Configuration for audit logging
    struct Config {
        bool enable_logging = true;               ///< Global on/off switch
        bool log_to_console = false;              ///< Also print to stderr
        std::string log_file_path;                ///< File path for log output (empty = no file)
        bool include_audio_metadata = false;      ///< Include audio characteristics in logs
    };

    /// @brief Construct logger with default config
    VoiceAuditLogger() = default;

    /// @brief Construct logger with custom config
    /// @param config Audit logging configuration
    explicit VoiceAuditLogger(const Config& config);

    /// @brief Destructor
    ~VoiceAuditLogger() = default;

    /// Delete copy, allow move
    VoiceAuditLogger(const VoiceAuditLogger&) = delete;
    VoiceAuditLogger& operator=(const VoiceAuditLogger&) = delete;
    VoiceAuditLogger(VoiceAuditLogger&&) noexcept = default;
    VoiceAuditLogger& operator=(VoiceAuditLogger&&) noexcept = default;

    /// @brief Log authentication attempt
    /// @param user_id User identifier
    /// @param method Authentication method ("liveness", "password", "2fa", etc.)
    /// @param success Whether authentication passed
    /// @param reason Detailed reason (on failure) or context
    /// @param duration_ms Time taken for authentication
    /// @param session_id Associated session ID (optional)
    void logAuthenticationAttempt(
        const std::string& user_id,
        const std::string& method,
        bool success,
        const std::string& reason,
        int64_t duration_ms = 0,
        const std::string& session_id = ""
    );

    /// @brief Log session lifecycle event
    /// @param session_id Session identifier
    /// @param user_id User identifier
    /// @param event Event type: "created", "closed", "timeout", "expired"
    /// @param duration_ms Session duration in milliseconds
    /// @param bytes_transferred Total bytes transferred in session
    void logSessionLifecycle(
        const std::string& session_id,
        const std::string& user_id,
        const std::string& event,
        int64_t duration_ms = 0,
        size_t bytes_transferred = 0
    );

    /// @brief Log liveness challenge event
    /// @param user_id User identifier
    /// @param challenge_id Challenge identifier
    /// @param event Event type: "issued", "verified", "expired", "failed"
    /// @param passed Whether challenge passed
    /// @param reason Detailed reason
    void logLivenessChallenge(
        const std::string& user_id,
        const std::string& challenge_id,
        const std::string& event,
        bool passed,
        const std::string& reason = ""
    );

    /// @brief Log spoofing detection result
    /// @param user_id User identifier
    /// @param spoof_score Overall spoof probability (0.0-1.0)
    /// @param verdict "spoofed" or "clean"
    /// @param freshness_score Audio freshness score (0.0-1.0)
    /// @param speaker_match_score Speaker match score (0.0-1.0)
    /// @param noise_consistency_score Noise consistency score (0.0-1.0)
    /// @param reason Detailed reason for verdict
    void logSpoofDetection(
        const std::string& user_id,
        double spoof_score,
        const std::string& verdict,
        double freshness_score = 0.0,
        double speaker_match_score = 0.0,
        double noise_consistency_score = 0.0,
        const std::string& reason = ""
    );

    /// @brief Get all logged events
    /// @return Vector of JSON event objects
    [[nodiscard]] std::vector<json> getEventLog() const;

    /// @brief Get events for a specific user
    /// @param user_id User identifier
    /// @return Vector of JSON events for that user
    [[nodiscard]] std::vector<json> getEventsForUser(const std::string& user_id) const;

    /// @brief Clear all logged events
    void clearEventLog();

    /// @brief Get event count
    /// @return Number of logged events
    [[nodiscard]] size_t getEventCount() const;

    /// @brief Register callback for new events
    /// @param callback Function called when new event is logged
    /// @note Callback receives the JSON event object
    void setEventCallback(std::function<void(const json&)> callback);

private:
    Config config_;
    mutable std::mutex mutex_;
    std::vector<json> event_log_;
    std::function<void(const json&)> event_callback_;

    /// @brief Get current timestamp in ISO 8601 format
    /// @return Timestamp string (e.g., "2026-08-28T14:22:31.456Z")
    [[nodiscard]] std::string getTimestamp() const;

    /// @brief Write event to output (console, file, or callback)
    /// @param event JSON event object
    void writeEvent(const json& event);

    /// @brief Serialize event to JSON string with formatting
    /// @param event JSON event
    /// @return Formatted JSON string
    [[nodiscard]] std::string serializeEvent(const json& event) const;
};

}} // namespace themis::voice
