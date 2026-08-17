/**
 * @file voice_session_manager.h
 * @brief Voice Session Lifecycle Manager — Frozen API Contract for Phase 1.
 *
 * @version v1.0 frozen as of 2026-08-08
 *
 * Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Design/API Contract Frozen (Phase 1)
 *
 * ## Session Lifecycle State Machine
 * ```
 *   [CREATE] → [ACTIVE] → [IDLE] → [EXPIRED] → [TERMINATED]
 *                ↓          ↑
 *           [EXPIRED] ←──────┴──────→ [TERMINATED]
 * ```
 *
 * - **ACTIVE**: Session created, user is interacting
 * - **IDLE**: No activity for idle_timeout_ms
 * - **EXPIRED**: Session exceeded idle or absolute timeout and is no longer usable
 * - **TERMINATED**: Explicit termination or cleanup
 *
 * ## Error Codes (Voice Module — Session)
 * - 6600: Session creation failed
 * - 6601: Session not found
 * - 6602: Session timeout/expiration
 * - 6603: Session state transition invalid
 * - 6604: Resource limit exceeded (e.g., max concurrent sessions)
 * - 6605: User ID validation failed
 * - 6606-6699: Reserved for future session-related errors
 *
 * ## Thread Safety
 * All public methods are thread-safe (internal mutex protection).
 * Persistence backend is assumed thread-safe.
 * Expired or terminated sessions are removed from active storage fail-closed;
 * only state-change timestamps remain for audit/teardown diagnostics.
 */


// Session management with persistence for Phase 6 production readiness
// ============================================================================
// PHASE 1 CONTRACT FREEZE: This file documents the immutable session lifecycle
// and provides canonical error codes for all session-related operations.
// ============================================================================
#pragma once
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <optional>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis { namespace voice {
using json = nlohmann::json;

// Session states
enum class SessionState {
    ACTIVE,
    IDLE,
    EXPIRED,
    TERMINATED
};

std::string sessionStateToString(SessionState state);

// Voice session with full metadata
struct VoiceSessionData {
    std::string session_id;
    std::string user_id;
    std::string device_id;
    SessionState state = SessionState::ACTIVE;

    int64_t created_at_ms = 0;
    int64_t last_activity_ms = 0;
    int64_t expires_at_ms = 0;

    std::vector<std::string> conversation_history;
    json context;
    json metadata;

    // Analytics
    uint32_t total_turns = 0;
    uint32_t error_count = 0;
    int64_t total_audio_ms = 0;
    std::string preferred_language = "en";
};

// Session analytics report
struct SessionAnalytics {
    size_t total_sessions = 0;
    size_t active_sessions = 0;
    size_t expired_sessions = 0;
    double avg_session_duration_ms = 0.0;
    double avg_turns_per_session = 0.0;
    std::map<std::string, size_t> sessions_by_device;
    std::map<std::string, size_t> sessions_by_language;
};

// Session timeout configuration
struct SessionTimeoutConfig {
    int64_t idle_timeout_ms = 5 * 60 * 1000;          // 5 minutes
    int64_t max_session_duration_ms = 30 * 60 * 1000;  // 30 minutes
    int64_t cleanup_interval_ms = 30 * 1000;            // 30 seconds
    bool auto_expire = true;
};

// Persistence interface (for pluggable backends)
/// @class ISessionPersistenceBackend
/// @brief Abstract interface for session persistence (pluggable backend).
///
/// Implementers provide durable storage for voice sessions. The default
/// InMemorySessionBackend is suitable for testing; production deployments
/// may inject a database-backed or Redis-backed implementation.
///
/// @note Thread safety is the responsibility of the implementation.
class ISessionPersistenceBackend {
public:
    virtual ~ISessionPersistenceBackend() = default;

    /// @brief Save session to durable storage.
    ///
    /// @param session VoiceSessionData to persist
    /// @return true if save succeeded; false otherwise
    [[nodiscard]] virtual bool save(const VoiceSessionData& session) = 0;

    /// @brief Load session from durable storage.
    ///
    /// @param session_id Session identifier
    /// @return std::optional<VoiceSessionData> containing the session if found;
    ///         std::nullopt if not found or load failed
    [[nodiscard]] virtual std::optional<VoiceSessionData> load(const std::string& session_id) = 0;

    /// @brief Remove session from durable storage.
    ///
    /// @param session_id Session identifier
    /// @return true if session existed and was removed; false if not found
    [[nodiscard]] virtual bool remove(const std::string& session_id) = 0;

    /// @brief List all active session IDs.
    ///
    /// @return Vector of session IDs currently in durable storage
    [[nodiscard]] virtual std::vector<std::string> listActiveSessions() = 0;
};

// In-memory persistence backend (default)
/// @class InMemorySessionBackend
/// @brief Thread-safe in-memory session storage (testing/development).
///
/// Stores all sessions in a std::unordered_map protected by mutex.
/// Sessions are lost on process termination; suitable for testing only.
class InMemorySessionBackend : public ISessionPersistenceBackend {
public:
    /// @brief Save session to in-memory map.
    /// @return true (always succeeds)
    bool save(const VoiceSessionData& session) override;

    /// @brief Load session from in-memory map.
    /// @return Session data if found; nullopt otherwise
    std::optional<VoiceSessionData> load(const std::string& session_id) override;

    /// @brief Remove session from in-memory map.
    /// @return true if session existed; false otherwise
    bool remove(const std::string& session_id) override;

    /// @brief List all sessions in in-memory map.
    /// @return Vector of all session IDs currently stored
    std::vector<std::string> listActiveSessions() override;

    /// @brief Get current session count.
    /// @return Number of sessions currently in memory
    size_t count() const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, VoiceSessionData> store_;
};

// VoiceSessionManager: Phase 1 Frozen API Contract
/// @class VoiceSessionManager
/// @brief Manages voice session lifecycle with immutable state machine.
///
/// ## Lifecycle Overview
/// VoiceSessionManager orchestrates the complete session lifecycle:
/// 1. Creation: new sessions transition to ACTIVE state
/// 2. Activity tracking: last_activity_ms updated on touchSession()
/// 3. Expiration: automated cleanup via expireOldSessions()
/// 4. Termination: explicit or implicit state transitions remove the session
///    from active storage fail-closed
///
/// ## State Transitions (Frozen)
/// ```
/// ACTIVE ──idle_timeout──> IDLE
/// ACTIVE ──max_duration──> EXPIRED
/// ACTIVE ──terminate()───> TERMINATED
/// IDLE ───touchSession──> ACTIVE
/// IDLE ────cleanup──────> TERMINATED
/// EXPIRED ──cleanup─────> TERMINATED
/// ```
///
/// ## Thread Safety Guarantees
/// - All public methods acquire internal mutex_
/// - Backend persistence calls are serialized
/// - Session data snapshots are returned by value (copy-safe)
///
/// ## Resource Limits (Frozen)
/// - Maximum concurrent sessions: 1000 per manager instance
/// - Session timeout: idle_timeout_ms (default 5 min)
/// - Maximum duration: max_session_duration_ms (default 30 min)
/// - Cleanup interval: cleanup_interval_ms (default 30 sec)
class VoiceSessionManager {
public:
    explicit VoiceSessionManager(
        const SessionTimeoutConfig& timeout_config = {},
        std::unique_ptr<ISessionPersistenceBackend> backend = nullptr
    );
    ~VoiceSessionManager() = default;

    /// @brief Create a new voice session.
    ///
    /// @param user_id User identifier (non-empty required)
    /// @param device_id Device identifier (optional, defaults to empty string)
    ///
    /// @pre user_id must not be empty; empty user_id is rejected fail-closed
    /// @post On success: VoiceSessionData with valid session_id in ACTIVE state
    /// @post On failure: VoiceSessionData with empty session_id returned
    ///
    /// @return VoiceSessionData with valid session_id if created; empty
    ///         session_id if validation failed (fail-closed)
    /// @note Rejects empty user_id fail-closed to prevent ghost sessions;
    ///       returns VoiceSessionData with empty session_id on validation failure
    /// @error 6600 Session creation failed (backend error)
    /// @error 6605 User ID validation failed (empty or invalid)
    VoiceSessionData createSession(const std::string& user_id, const std::string& device_id = "");

    /// @brief Get existing session by ID.
    ///
    /// @param session_id Session identifier
    ///
    /// @return std::optional<VoiceSessionData> containing the session if found
    ///         and not expired; std::nullopt if not found or expired
    /// @error 6601 Session not found
    /// @error 6602 Session expired
    std::optional<VoiceSessionData> getSession(const std::string& session_id);

    /// @brief Update session context and metadata.
    ///
    /// @param session_id Session identifier
    /// @param context_update JSON context data to merge into session.context
    ///
    /// @pre Session must exist and not be in TERMINATED state
    /// @post Session context is updated; last_activity_ms is refreshed
    ///
    /// @return true if update succeeded; false if session not found or expired
    /// @error 6601 Session not found
    /// @error 6603 Session state transition invalid
    bool updateSession(const std::string& session_id, const json& context_update);
    
    /// @brief Add a conversation turn to session history.
    ///
    /// @param session_id Session identifier
    /// @param user_msg User message (non-empty required)
    /// @param assistant_msg Assistant response (non-empty required)
    ///
    /// @pre Session must exist and be in ACTIVE or IDLE state
    /// @pre user_msg and assistant_msg must both be non-empty
    /// @post Conversation turn is appended to session.conversation_history
    /// @post total_turns counter is incremented
    ///
    /// @return true if turn was added; false if session not found or messages empty
    /// @note Rejects empty messages fail-closed to prevent silent history corruption
    /// @error 6601 Session not found
    /// @error 6603 Invalid message content
    bool addConversationTurn(const std::string& session_id, const std::string& user_msg, const std::string& assistant_msg);
    
    /// @brief Touch session (update last activity).
    ///
    /// @param session_id Session identifier
    ///
    /// @pre Session must exist
    /// @post last_activity_ms is updated to current time
    /// @post If session state is IDLE, transitions back to ACTIVE
    ///
    /// @return true if touched; false if session not found
    /// @error 6601 Session not found
    bool touchSession(const std::string& session_id);

    /// @brief Update the preferred language for a session.
    ///
    /// @param session_id Session identifier
    /// @param language_code BCP-47 language code (e.g., "en", "fr", "es")
    ///
    /// @return true if language updated; false if session not found
    /// @error 6601 Session not found
    bool updatePreferredLanguage(const std::string& session_id, const std::string& language_code);

    /// @brief Terminate session explicitly.
    ///
    /// @param session_id Session identifier
    ///
    /// @pre Session must exist
    /// @post Session transitions to TERMINATED state
    ///
    /// @return true if terminated; false if session not found
    /// @error 6601 Session not found
    bool terminateSession(const std::string& session_id);

    /// @brief Expire old sessions (cleanup).
    ///
    /// Scans all sessions and transitions IDLE/EXPIRED sessions to TERMINATED.
    /// Intended to be called periodically (e.g., every cleanup_interval_ms).
    ///
    /// @post All expired sessions removed from cache
    /// @return Number of sessions expired/cleaned up
    size_t expireOldSessions();

    /// @brief Get all sessions for a user (multi-device sync).
    ///
    /// @param user_id User identifier
    ///
    /// @return Vector of VoiceSessionData for all non-terminated sessions owned by user_id
    /// @note Returns empty vector if user has no active sessions
    std::vector<VoiceSessionData> getSessionsForUser(const std::string& user_id);

    /// @brief Get analytics summary.
    ///
    /// @return SessionAnalytics with aggregate statistics
    SessionAnalytics getAnalytics() const;

    /// @brief Check if session is active.
    ///
    /// @param session_id Session identifier
    ///
    /// @return true if session exists and state is ACTIVE; false otherwise
    bool isSessionActive(const std::string& session_id);

    /// @brief Get session state.
    ///
    /// @param session_id Session identifier
    ///
    /// @return SessionState if session found; SessionState::TERMINATED if not found
    SessionState getSessionState(const std::string& session_id);

    /// @brief Generate a unique session ID.
    ///
    /// @return UUID-style session identifier (e.g., "session_12345678abcdef")
    /// @note Format is stable and suitable for logging/debugging
    static std::string generateSessionId();
    
    // Phase 3: Session State Guard Violations
    
    /// @brief Validate state transition (fail-closed)
    /// @param session_id Session identifier
    /// @param new_state Target state
    /// @return true if transition is valid; false if violates state machine
    /// @error 6603 Invalid state transition
    bool validateStateTransition(const std::string& session_id, SessionState new_state);
    
    /// @brief Detect double-close attempt (fail-closed)
    /// @param session_id Session identifier
    /// @return true if session already in TERMINATED state; false otherwise
    bool isDoubleCloseAttempt(const std::string& session_id);
    
    /// @brief Detect use-after-free (fail-closed)
    /// @param session_id Session identifier
    /// @return true if session has expired; false if active
    bool isUseAfterFreeAttempt(const std::string& session_id);
    
    /// @brief Detect session collision (prevent duplicate session_id)
    /// @param session_id Session identifier
    /// @return true if session_id already exists; false otherwise
    bool sessionIdExists(const std::string& session_id);
    
    /// @brief Get session state change timestamp (for audit trail)
    /// @param session_id Session identifier
    /// @return Timestamp in milliseconds; 0 if not found
    int64_t getStateChangeTimestamp(const std::string& session_id);

private:
    SessionTimeoutConfig timeout_config_;
    std::unique_ptr<ISessionPersistenceBackend> backend_;
    mutable std::mutex manager_mutex_;

    std::unordered_map<std::string, VoiceSessionData> active_cache_;
    
    // Phase 3: Session guard tracking
    std::map<std::string, int64_t> state_change_timestamps_;

    bool isExpired(const VoiceSessionData& session) const;
    int64_t nowMs() const;
};;

}} // namespace themis::voice
