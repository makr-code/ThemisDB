/**
 * @file session_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
 *
 * ### Bounded runtime contract (auth_principal_contract.h §2, §4)
 *
 * - Session IDs are prefixed "sess_" and are cryptographically random
 *   (RAND_bytes from OpenSSL).  A construction failure causes createSession()
 *   to throw std::runtime_error; the caller MUST NOT fall back to a
 *   deterministic or user-supplied identifier.
 * - validateSession() returns ValidationResult::valid = false for ANY of:
 *   expired (absolute or idle), unknown session ID, or internally inconsistent
 *   state.  It never throws for a "session not found" condition; use the
 *   returned ValidationResult::reason for diagnostic logging.
 * - Session-ID comparison uses CRYPTO_memcmp to prevent timing-oracle attacks.
 *   Callers MUST NOT compare session IDs with == or std::string::compare.
 * - The maximum absolute lifetime is bounded by kMaxSessionLifetime (30 days)
 *   and the maximum idle timeout by kMaxSessionIdleTimeout (8 hours) as defined
 *   in auth_principal_contract.h.  Values exceeding these bounds are silently
 *   clamped at construction.
 * - revokeSession() and revokeAllUserSessions() are idempotent; revoking an
 *   already-expired or non-existent session ID is a no-op (no error thrown).
 *
 * @see include/auth/auth_principal_contract.h — §2 Temporal contract, §4 Fail-closed
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
    SessionManager(SessionManager&&) noexcept = default;
    SessionManager& operator=(SessionManager&&) noexcept = default;

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
