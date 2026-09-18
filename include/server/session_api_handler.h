/**
 * @file session_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class SessionApiHandler {
public:
    explicit SessionApiHandler(
        std::shared_ptr<AuthMiddleware> auth,
        std::shared_ptr<auth::SessionManager> manager,
        std::shared_ptr<utils::AuditLogger> audit_logger = nullptr
    );

    nlohmann::json createSession(
        const std::string& bearer_token,
        const nlohmann::json& body,
        const std::string& client_ip = {}
    );

    nlohmann::json listSessions(
        const std::string& bearer_token,
        const std::string& current_session = {}
    );

    /**
     * @brief Revoke Session.
     * @param[in] bearer_token Input parameter.
     * @param[in] session_id Identifier of the session.
     * @return Return value.
     */
    nlohmann::json revokeSession(
        const std::string& bearer_token,
        const std::string& session_id
    );

    nlohmann::json revokeAllOtherSessions(
        const std::string& bearer_token,
        const std::string& current_session = {}
    );

private:
    /**
     * @brief Audit Authorization Decision.
     * @param[in] scope Input parameter.
     * @param[in] endpoint Input parameter.
     * @param[in] auth_result Input parameter.
     */
    void auditAuthorizationDecision(
        const std::string& scope,
        const std::string& endpoint,
        const AuthMiddleware::AuthResult& auth_result
    );

    /**
     * @brief Make Error.
     * @param[in] status_code Input parameter.
     * @param[in] message Input parameter.
     * @return Return value.
     */
    static nlohmann::json makeError(int status_code, const std::string& message);

    /**
     * @brief Session To Json.
     * @param[in] s Input parameter.
     * @return Return value.
     */
    static nlohmann::json sessionToJson(const auth::SessionManager::SessionInfo& s);

    std::shared_ptr<AuthMiddleware> auth_;
    std::shared_ptr<auth::SessionManager> manager_;
    std::shared_ptr<utils::AuditLogger> audit_logger_;
};

} // namespace server
} // namespace themis
