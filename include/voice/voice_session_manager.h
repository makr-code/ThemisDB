/**
 * @file voice_session_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.42
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: voice_session_manager.h | Version: 0.0.42
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// Session management with persistence for Phase 6 production readiness
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
    int64_t max_session_duration_ms = 60 * 60 * 1000;  // 1 hour
    int64_t cleanup_interval_ms = 30 * 1000;            // 30 seconds
    bool auto_expire = true;
};

// Persistence interface (for pluggable backends)
class ISessionPersistenceBackend {
public:
    virtual ~ISessionPersistenceBackend() = default;
    [[nodiscard]] virtual bool save(const VoiceSessionData& session) = 0;
    [[nodiscard]] virtual std::optional<VoiceSessionData> load(const std::string& session_id) = 0;
    [[nodiscard]] virtual bool remove(const std::string& session_id) = 0;
    [[nodiscard]] virtual std::vector<std::string> listActiveSessions() = 0;
};

// In-memory persistence backend (default)
class InMemorySessionBackend : public ISessionPersistenceBackend {
public:
    bool save(const VoiceSessionData& session) override;
    std::optional<VoiceSessionData> load(const std::string& session_id) override;
    bool remove(const std::string& session_id) override;
    std::vector<std::string> listActiveSessions() override;

    size_t count() const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, VoiceSessionData> store_;
};

// VoiceSessionManager: Phase 6 production component
class VoiceSessionManager {
public:
    explicit VoiceSessionManager(
        const SessionTimeoutConfig& timeout_config = {},
        std::unique_ptr<ISessionPersistenceBackend> backend = nullptr
    );
    ~VoiceSessionManager() = default;

    /**
     * Create a new voice session.
     * @param user_id User identifier (non-empty required)
     * @param device_id Device identifier (optional, defaults to empty string)
     * @return VoiceSessionData with valid session_id if created; empty session_id if fail-closed (invalid user_id)
     * @note Rejects empty user_id fail-closed to prevent ghost sessions with missing owner context;
     *       returns VoiceSessionData with empty session_id on validation failure
     */
    VoiceSessionData createSession(const std::string& user_id, const std::string& device_id = "");

    // Get existing session (returns nullopt if not found or expired)
    std::optional<VoiceSessionData> getSession(const std::string& session_id);

    // Update session (activity, history, context)
    bool updateSession(const std::string& session_id, const json& context_update);
    
    /**
     * Add a conversation turn to the session history.
     * @param session_id Session identifier
     * @param user_msg User message (non-empty required)
     * @param assistant_msg Assistant response message (non-empty required)
     * @return true if turn was added successfully; false if session not found or message is empty (fail-closed)
     * @note Rejects empty user_msg or assistant_msg fail-closed to prevent silent history corruption
     */
    bool addConversationTurn(const std::string& session_id, const std::string& user_msg, const std::string& assistant_msg);
    
    bool touchSession(const std::string& session_id);  // Update last_activity

    // Update the preferred language for a session (used by auto-locale switching)
    bool updatePreferredLanguage(const std::string& session_id, const std::string& language_code);

    // Terminate session
    bool terminateSession(const std::string& session_id);

    // Expire old sessions
    size_t expireOldSessions();

    // Multi-device sync: get all sessions for a user
    std::vector<VoiceSessionData> getSessionsForUser(const std::string& user_id);

    // Analytics
    SessionAnalytics getAnalytics() const;

    // Session state check
    bool isSessionActive(const std::string& session_id);
    SessionState getSessionState(const std::string& session_id);

    // Generate a unique session ID
    static std::string generateSessionId();

private:
    SessionTimeoutConfig timeout_config_;
    std::unique_ptr<ISessionPersistenceBackend> backend_;
    mutable std::mutex manager_mutex_;

    std::unordered_map<std::string, VoiceSessionData> active_cache_;

    bool isExpired(const VoiceSessionData& session) const;
    int64_t nowMs() const;
};

}} // namespace themis::voice
