/**
 * @file session_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "auth/session_manager.h"
#include "server/auth_middleware.h"

#include <nlohmann/json.hpp>
#include <memory>
#include <string>

namespace themis {
namespace utils {
class AuditLogger;
}
namespace server {

/**
 * @brief REST handler for session management and revocation
 *
 * Exposes the following endpoints (all require a valid bearer token):
 *
 *   POST   /auth/sessions
 *     Create a new session for the authenticated user.
 *     Body: { "device_fingerprint": "...", "user_agent": "..." }
 *     Returns: { "session_id": "sess_...", "user_id": "...", "created_at": "..." }
 *
 *   GET    /auth/sessions
 *     List all active sessions for the authenticated user.
 *     Returns: { "sessions": [...], "total": N }
 *
 *   DELETE /auth/sessions/{session_id}
 *     Revoke a specific session.
 *     Returns: { "success": true, "session_id": "..." }
 *
 *   DELETE /auth/sessions
 *     Revoke all sessions for the authenticated user except the current one.
 *     Body (optional): { "current_session_id": "sess_..." }
 *     Returns: { "success": true, "terminated": N }
 *
 * Required scope: "auth:sessions" for all operations.
 * Admins (scope "admin:all") may revoke sessions belonging to other users.
 */
class SessionApiHandler {
public:
    /**
     * @param auth    Shared AuthMiddleware for token validation.
     * @param manager Shared SessionManager instance.
     */
    explicit SessionApiHandler(
        std::shared_ptr<AuthMiddleware> auth,
        std::shared_ptr<auth::SessionManager> manager,
        std::shared_ptr<utils::AuditLogger> audit_logger = nullptr
    );

    /**
     * @brief Create a new session for the authenticated user.
     *
     * @param bearer_token  Raw bearer token from the Authorization header.
     * @param body          JSON request body.
     * @param client_ip     Client IP address (used for session metadata).
     * @return JSON response with session details, or error object.
     */
    nlohmann::json createSession(
        const std::string& bearer_token,
        const nlohmann::json& body,
        const std::string& client_ip = {}
    );

    /**
     * @brief List all active sessions for the authenticated user.
     *
     * @param bearer_token     Raw bearer token.
     * @param current_session  Optional: marks this session as is_current in output.
     * @return JSON response with sessions array, or error object.
     */
    nlohmann::json listSessions(
        const std::string& bearer_token,
        const std::string& current_session = {}
    );

    /**
     * @brief Revoke a specific session.
     *
     * A user may only revoke their own sessions.  Admins (scope "admin:all")
     * may revoke any session.
     *
     * @param bearer_token Raw bearer token.
     * @param session_id   Session to revoke.
     * @return JSON response indicating success or error.
     */
    nlohmann::json revokeSession(
        const std::string& bearer_token,
        const std::string& session_id
    );

    /**
     * @brief Revoke all sessions for the authenticated user except the current one.
     *
     * @param bearer_token     Raw bearer token.
     * @param current_session  Session to preserve; all others are terminated.
     * @return JSON response with the number of sessions terminated.
     */
    nlohmann::json revokeAllOtherSessions(
        const std::string& bearer_token,
        const std::string& current_session = {}
    );

private:
    void auditAuthorizationDecision(
        const std::string& scope,
        const std::string& endpoint,
        const AuthMiddleware::AuthResult& auth_result
    );

    /// Build a standardised error JSON object with an HTTP status code hint.
    static nlohmann::json makeError(int status_code, const std::string& message);

    /// Serialise a SessionInfo to a public-facing JSON object.
    static nlohmann::json sessionToJson(const auth::SessionManager::SessionInfo& s);

    std::shared_ptr<AuthMiddleware> auth_;
    std::shared_ptr<auth::SessionManager> manager_;
    std::shared_ptr<utils::AuditLogger> audit_logger_;
};

} // namespace server
} // namespace themis
