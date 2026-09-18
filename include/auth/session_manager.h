/**
 * @file session_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class SessionManager {
public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    struct SessionLimits {
        uint32_t max_sessions_per_user = 10;
        std::chrono::milliseconds idle_timeout{std::chrono::hours(8)};
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

        explicit operator bool() const noexcept {
            return valid;
        }
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    SessionManager();
    /**
     * @brief Session Manager.
     * @param[in] limits Input parameter.
     * @return Return value.
     */
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

    std::string createSession(
        const std::string& user_id,
        const std::string& device_fingerprint = {},
        const std::string& ip_address = {},
        const std::string& user_agent = {}
    );

    ValidationResult validateSession(
        const std::string& session_id,
        const std::string& current_ip = {}
    );

    /**
     * @brief Terminate Session.
     * @param[in] session_id Identifier of the session.
     */
    void terminateSession(const std::string& session_id);

    /**
     * @brief Invalidate a session token.
     * @param[in] session_id Identifier of the session.
     * @details Calls: terminateSession().
     */
    void invalidateSession(const std::string& session_id) {
        terminateSession(session_id);
    }

    int terminateAllOtherSessions(
        const std::string& user_id,
        const std::string& keep_session_id = {}
    );

    // -----------------------------------------------------------------------
    // Queries
    // -----------------------------------------------------------------------

    /**
     * @brief List Sessions.
     * @param[in] user_id Identifier of the user.
     * @return Return value.
     */
    std::vector<SessionInfo> listSessions(const std::string& user_id);

    /**
     * @brief Size.
     * @return Return value.
     */
    size_t size() const;

    /**
     * @brief Prune Expired.
     * @return Return value.
     */
    size_t pruneExpired();

    // -----------------------------------------------------------------------
    // Static helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Generate Session Id.
     * @return Return value.
     */
    static std::string generateSessionId();

private:
    SessionLimits limits_;
    std::unordered_map<std::string, SessionInfo> sessions_; ///< id -> info
    mutable std::mutex mutex_;

    /**
     * @brief Is Expired.
     * @param[in] s Input parameter.
     * @return True when the operation succeeds.
     */
    bool isExpired(const SessionInfo& s) const;

    /**
     * @brief Enforce Session Limits.
     * @param[in] user_id Identifier of the user.
     */
    void enforceSessionLimits(const std::string& user_id);

    /**
     * @brief Prune Expired Locked.
     * @return Return value.
     */
    size_t pruneExpiredLocked();
};

} // namespace auth
} // namespace themis
