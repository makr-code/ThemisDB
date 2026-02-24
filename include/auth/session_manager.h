/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            session_manager.h                                  ║
  Version:         0.0.32                                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <optional>

namespace themis {
namespace auth {

/**
 * @brief In-process session manager for authentication sessions
 *
 * Manages server-side sessions linked to authenticated users.  A session is
 * created after a successful login and is identified by an opaque session_id.
 * Sessions can be individually revoked (logout from one device) or bulk-revoked
 * (logout everywhere).
 *
 * Key features:
 * - Concurrent-session limit per user (configurable)
 * - Idle-timeout and absolute-timeout enforcement
 * - Optional IP / device-fingerprint pinning
 * - Thread-safe (all public methods)
 *
 * Distributed deployments: back the in-memory store with a shared cache (e.g.
 * Redis) and synchronise changes across nodes.  This implementation provides
 * the single-node baseline that can be extended.
 */
class SessionManager {
public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    struct SessionLimits {
        /// Maximum number of simultaneous active sessions per user.
        int max_concurrent_sessions = 5;
        /// Seconds of inactivity after which a session is considered idle.
        std::chrono::seconds idle_timeout{86400};       // 24 h
        /// Absolute maximum session lifetime from creation.
        std::chrono::seconds absolute_timeout{2592000}; // 30 days
        /// Reject sessions whose IP changed since creation (hijacking guard).
        bool pin_to_ip = false;
        /// Reject sessions whose device fingerprint changed.
        bool pin_to_device = false;
    };

    // -----------------------------------------------------------------------
    // Session data
    // -----------------------------------------------------------------------

    struct SessionInfo {
        std::string session_id;
        std::string user_id;
        std::string device_fingerprint;
        std::string ip_address;
        std::string user_agent;
        std::chrono::system_clock::time_point created_at;
        std::chrono::system_clock::time_point last_activity;
        bool is_current = false; ///< Set by listSessions() for the caller's own session.
    };

    // -----------------------------------------------------------------------
    // Anomaly detection
    // -----------------------------------------------------------------------

    enum class AnomalyType {
        IPAddressChange,
        DeviceChange,
        ConcurrentSessionLimitExceeded,
        IdleTimeoutExceeded,
        AbsoluteTimeoutExceeded
    };

    struct Anomaly {
        AnomalyType type;
        int severity;  ///< 0 (informational) – 100 (critical).
        std::string description;
        std::chrono::system_clock::time_point detected_at;
    };

    // -----------------------------------------------------------------------
    // Validation result
    // -----------------------------------------------------------------------

    struct ValidationResult {
        bool valid = false;
        std::optional<SessionInfo> session;
        std::string error_message;
    };

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /// Construct with default session limits.
    SessionManager();
    /// Construct with custom session limits.
    explicit SessionManager(const SessionLimits& limits);
    ~SessionManager() = default;

    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;
    SessionManager(SessionManager&&) = default;
    SessionManager& operator=(SessionManager&&) = default;

    // -----------------------------------------------------------------------
    // Core operations
    // -----------------------------------------------------------------------

    /**
     * @brief Create a new session for a user.
     *
     * If the concurrent-session limit is reached, the oldest session for that
     * user is evicted automatically.
     *
     * @param user_id           Authenticated user identifier.
     * @param device_fingerprint Opaque device string (e.g. hashed UA+IP).
     * @param ip_address        Client IP address at login time.
     * @param user_agent        HTTP User-Agent string.
     * @return Newly created session_id (URL-safe opaque token).
     */
    std::string createSession(
        const std::string& user_id,
        const std::string& device_fingerprint,
        const std::string& ip_address,
        const std::string& user_agent
    );

    /**
     * @brief Validate an existing session and refresh its last-activity stamp.
     *
     * Returns invalid if the session does not exist, has expired (idle or
     * absolute timeout), or fails pinning checks when enabled.
     *
     * @param session_id              Session identifier.
     * @param current_ip              Current client IP (checked when pin_to_ip).
     * @param current_device_fingerprint Current device (checked when pin_to_device).
     */
    ValidationResult validateSession(
        const std::string& session_id,
        const std::string& current_ip = {},
        const std::string& current_device_fingerprint = {}
    );

    /**
     * @brief List all active sessions for a user.
     *
     * Expired sessions are pruned inline.
     *
     * @param user_id         User identifier.
     * @param current_session Optional: marks the matching session as is_current.
     */
    std::vector<SessionInfo> listSessions(
        const std::string& user_id,
        const std::string& current_session = {}
    );

    /**
     * @brief Terminate (revoke) a specific session.
     *
     * No-op if the session does not exist.
     *
     * @param session_id Session to terminate.
     */
    void terminateSession(const std::string& session_id);

    /**
     * @brief Terminate all sessions for a user except the one given.
     *
     * Useful for "logout everywhere else" functionality.
     *
     * @param user_id            User whose sessions to terminate.
     * @param keep_session_id    Session that should be preserved.
     * @return Number of sessions terminated.
     */
    int terminateAllOtherSessions(
        const std::string& user_id,
        const std::string& keep_session_id
    );

    /**
     * @brief Terminate ALL sessions for a user (full logout).
     *
     * @param user_id User whose sessions to terminate.
     * @return Number of sessions terminated.
     */
    int terminateAllSessions(const std::string& user_id);

    /**
     * @brief Detect anomalies associated with a session.
     *
     * @param session_id Session to inspect.
     * @return List of detected anomalies (empty if none).
     */
    std::vector<Anomaly> detectAnomalies(const std::string& session_id);

    /**
     * @brief Remove all expired sessions from the store.
     *
     * Called automatically during createSession() and validateSession(), but
     * can also be invoked periodically by a maintenance task.
     */
    void pruneExpired();

    /// Total number of sessions in the store (includes expired entries
    /// not yet pruned; call pruneExpired() for an exact live count).
    size_t size() const;

private:
    // -----------------------------------------------------------------------
    // Internal helpers
    // -----------------------------------------------------------------------

    /// Generate a cryptographically-secure URL-safe session ID.
    static std::string generateSessionId();

    /// Return true if the session has exceeded its idle or absolute timeout.
    bool isExpired(const SessionInfo& s) const;

    /// Evict the oldest session for a user when the concurrent limit is reached.
    void enforceSessionLimits(const std::string& user_id);

    // -----------------------------------------------------------------------
    // Storage
    // -----------------------------------------------------------------------

    SessionLimits limits_;

    // session_id -> SessionInfo
    std::unordered_map<std::string, SessionInfo> sessions_;
    // user_id -> set of session_ids (for fast per-user lookup)
    std::unordered_map<std::string, std::vector<std::string>> user_sessions_;

    mutable std::mutex mutex_;
};

} // namespace auth
} // namespace themis
