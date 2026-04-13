/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            session_manager.h                                  ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-13 04:14:01                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     223                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 5a16800952  2026-02-24  feat(auth): implement session management and revocation e... ║
    • 4994e3c4b0  2026-02-24  fix(auth): audit fixes - wire http server, fix enforceSes... ║
    • 125b23d98f  2026-02-24  feat(auth): implement session management and revocation e... ║
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
#include <cstdint>

namespace themis {
namespace auth {

/**
 * @brief In-process session manager for authentication sessions.
 *
 * Manages the full lifecycle of user authentication sessions:
 *   - Creation with cryptographically random IDs.
 *   - Validation (existence, expiry, optional IP binding).
 *   - Per-session and bulk termination.
 *   - Configurable idle and absolute timeouts.
 *   - Enforcement of per-user maximum concurrent session limits
 *     (oldest session evicted when the limit is exceeded).
 *
 * Thread-safety: all public methods are thread-safe.
 *
 * Distributed deployments should back this with a shared cache (Redis,
 * Valkey, …) and replicate revocations across nodes.  This implementation
 * provides the single-node baseline.
 */
class SessionManager {
public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    struct SessionLimits {
        /// Maximum number of concurrent sessions per user.  0 = unlimited.
        uint32_t max_sessions_per_user = 10;
        /// Idle timeout: session is invalid if not accessed within this
        /// window.  Zero means no idle timeout.
        std::chrono::milliseconds idle_timeout{std::chrono::hours(8)};
        /// Absolute session lifetime.  Zero means no absolute timeout.
        std::chrono::milliseconds absolute_timeout{std::chrono::hours(24 * 30)};
    };

    // -----------------------------------------------------------------------
    // Session metadata
    // -----------------------------------------------------------------------

    struct SessionInfo {
        std::string session_id;
        std::string user_id;
        std::string device_fingerprint;
        std::string ip_address;
        std::string user_agent;
        std::chrono::system_clock::time_point created_at;
        std::chrono::system_clock::time_point last_accessed_at;
        std::chrono::system_clock::time_point expires_at; ///< Absolute deadline
    };

    // -----------------------------------------------------------------------
    // Validation result
    // -----------------------------------------------------------------------

    struct ValidationResult {
        bool valid = false;
        std::optional<SessionInfo> session;
        std::string reason; ///< Human-readable reason if !valid
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    SessionManager();
    explicit SessionManager(const SessionLimits& limits);
    ~SessionManager() = default;

    // Non-copyable, movable
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;
    SessionManager(SessionManager&&) = default;
    SessionManager& operator=(SessionManager&&) = default;

    // -----------------------------------------------------------------------
    // Lifecycle operations
    // -----------------------------------------------------------------------

    /**
     * @brief Create a new session for the given user.
     *
     * If the user already has max_sessions_per_user active sessions the
     * oldest session is terminated before the new one is created.
     *
     * @param user_id            Subject identifier.
     * @param device_fingerprint Optional device fingerprint (for display).
     * @param ip_address         Client IP (stored for display / analytics).
     * @param user_agent         HTTP User-Agent string.
     * @return Newly created session ID (prefixed "sess_").
     */
    std::string createSession(
        const std::string& user_id,
        const std::string& device_fingerprint = {},
        const std::string& ip_address = {},
        const std::string& user_agent = {}
    );

    /**
     * @brief Validate a session and update its last-accessed timestamp.
     *
     * @param session_id Session to validate.
     * @param current_ip Optional: if non-empty and IP-binding is strict,
     *                   the IP is checked against the stored value.
     *                   Currently informational only (stored, not enforced).
     * @return ValidationResult with valid=true and populated session on
     *         success, or valid=false with a reason string on failure.
     */
    ValidationResult validateSession(
        const std::string& session_id,
        const std::string& current_ip = {}
    );

    /**
     * @brief Terminate (revoke) a single session.
     *
     * @param session_id Session to remove.  No-op if not found.
     */
    void terminateSession(const std::string& session_id);

    /**
     * @brief Terminate all sessions for a user except the specified one.
     *
     * @param user_id           Owner of the sessions.
     * @param keep_session_id   Session to preserve (may be empty to
     *                          terminate all sessions).
     * @return Number of sessions terminated.
     */
    int terminateAllOtherSessions(
        const std::string& user_id,
        const std::string& keep_session_id = {}
    );

    // -----------------------------------------------------------------------
    // Queries
    // -----------------------------------------------------------------------

    /**
     * @brief List all active (non-expired) sessions for a user.
     *
     * @param user_id Subject identifier.
     * @return Vector of SessionInfo ordered by creation time (oldest first).
     */
    std::vector<SessionInfo> listSessions(const std::string& user_id);

    /**
     * @brief Current total number of stored sessions (including expired).
     */
    size_t size() const;

    /**
     * @brief Remove all sessions whose absolute_timeout has passed.
     *
     * Called automatically inside createSession() / validateSession(),
     * but can also be invoked manually for maintenance.
     *
     * @return Number of entries removed.
     */
    size_t pruneExpired();

    // -----------------------------------------------------------------------
    // Static helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Generate a cryptographically random session identifier.
     *
     * Format: "sess_" + 32 hex characters (128 random bits).
     */
    static std::string generateSessionId();

private:
    SessionLimits limits_;
    std::unordered_map<std::string, SessionInfo> sessions_; ///< id -> info
    mutable std::mutex mutex_;

    /// Returns true if the session's absolute or idle deadline has passed.
    bool isExpired(const SessionInfo& s) const;

    /// Evict the oldest session for user_id if the per-user limit is exceeded.
    void enforceSessionLimits(const std::string& user_id);

    /// Prune expired sessions; caller MUST hold mutex_.
    size_t pruneExpiredLocked();
};

} // namespace auth
} // namespace themis
