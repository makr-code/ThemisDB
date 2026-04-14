/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            session_api_handler.h                              ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-14 06:56:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     139                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 5a16800952  2026-02-24  feat(auth): implement session management and revocation e... ║
    • 125b23d98f  2026-02-24  feat(auth): implement session management and revocation e... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "auth/session_manager.h"
#include "server/auth_middleware.h"

#include <nlohmann/json.hpp>
#include <memory>
#include <string>

namespace themis {
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
        std::shared_ptr<auth::SessionManager> manager
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
    /// Build a standardised error JSON object with an HTTP status code hint.
    static nlohmann::json makeError(int status_code, const std::string& message);

    /// Serialise a SessionInfo to a public-facing JSON object.
    static nlohmann::json sessionToJson(const auth::SessionManager::SessionInfo& s);

    std::shared_ptr<AuthMiddleware> auth_;
    std::shared_ptr<auth::SessionManager> manager_;
};

} // namespace server
} // namespace themis
