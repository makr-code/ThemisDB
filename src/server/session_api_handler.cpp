/**
 * @file session_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=0, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/session_api_handler.h"
#include "utils/logger.h"
#include "utils/input_validator.h"
#include "utils/audit_logger.h"

#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include "utils/tracing.h"

namespace themis {
namespace server {

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

namespace {

constexpr size_t MAX_SESSION_IDENTIFIER_LEN = 256;
constexpr size_t MAX_DEVICE_FINGERPRINT_LEN = 512;
constexpr size_t MAX_USER_AGENT_LEN = 1024;

bool isSafeSessionIdentifier(std::string_view value) {
    themis::utils::InputValidator validator;
    return !value.empty() &&
           validator.validateStringLength(std::string(value), MAX_SESSION_IDENTIFIER_LEN) &&
           validator.validatePathSegment(std::string(value));
}

bool isSafeHeaderLikeValue(std::string_view value, size_t max_len) {
    themis::utils::InputValidator validator;
    return validator.validateStringLength(std::string(value), max_len) &&
           validator.validateHeaderValue(std::string(value));
}

} // namespace

static std::string timePointToISO8601(std::chrono::system_clock::time_point tp) {
    // Guard against max() sentinel used for "no absolute timeout"
    static const auto max_tp = std::chrono::system_clock::time_point::max();
    if (tp == max_tp) {
        return "";
    }
    const std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

SessionApiHandler::SessionApiHandler(
    std::shared_ptr<AuthMiddleware> auth,
    std::shared_ptr<auth::SessionManager> manager,
    std::shared_ptr<utils::AuditLogger> audit_logger
)
    : auth_(std::move(auth))
    , manager_(std::move(manager))
    , audit_logger_(std::move(audit_logger))
{
    if ([[maybe_unused]] !auth_)    { throw std::invalid_argument("SessionApiHandler: auth must not be null"); }
    if ([[maybe_unused]] !manager_) { throw std::invalid_argument("SessionApiHandler: manager must not be null"); }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

nlohmann::json SessionApiHandler::makeError(int status_code, const std::string& message) {
    return {
        {"error",       message},
        {"status_code", status_code}
    };
}

nlohmann::json SessionApiHandler::sessionToJson([[maybe_unused]] const auth::SessionManager::SessionInfo& s) {
    nlohmann::json j;
    j["session_id"]         = s.session_id;
    j["user_id"]            = s.user_id;
    j["device_fingerprint"] = s.device_fingerprint;
    j["ip_address"]         = s.ip_address;
    j["user_agent"]         = s.user_agent;
    j["created_at"]         = timePointToISO8601(s.created_at);
    j["last_accessed_at"]   = timePointToISO8601(s.last_accessed_at);
    const std::string exp   = timePointToISO8601(s.expires_at);
    if (!exp.empty()) {
        j["expires_at"] = exp;
    }
    return j;
}

void SessionApiHandler::auditAuthorizationDecision(
    const std::string& scope,
    const std::string& endpoint,
    const AuthMiddleware::AuthResult& auth_result
) {
    if (!audit_logger_) {
        return;
    }

    const auto event_type = auth_result.authorized
        ? utils::SecurityEventType::CUSTOM_EVENT
        : utils::SecurityEventType::PERMISSION_DENIED;

    nlohmann::json details = {
        {"component", "session_api_handler"},
        {"scope", scope},
        {"endpoint", endpoint},
        {"decision", auth_result.authorized ? "allowed" : "denied"}
    };
    if (!auth_result.reason.empty()) {
        details["reason"] = auth_result.reason;
    }

    const std::string actor = auth_result.user_id.empty() ? "anonymous" : auth_result.user_id;

    try {
        audit_logger_->logSecurityEvent(event_type, actor, endpoint, details);
    } catch (const std::exception& ex) {
        THEMIS_WARN("SessionApiHandler audit logging failed: {}", ex.what());
    }
}

// ---------------------------------------------------------------------------
// createSession
// ---------------------------------------------------------------------------

nlohmann::json SessionApiHandler::createSession(
    const std::string& bearer_token,
    const nlohmann::json& body,
    const std::string& client_ip
) {
    auto span = Tracer::startSpan("createSession");
    if (!isSafeHeaderLikeValue(bearer_token, 8192)) {
        return makeError(400, "Invalid bearer token format");
    }
    if (!client_ip.empty() && !isSafeHeaderLikeValue(client_ip, 128)) {
        return makeError(400, "Invalid client_ip format");
    }

    auto auth_result = auth_->authorize(bearer_token, "auth:sessions");
    auditAuthorizationDecision("auth:sessions", "POST /auth/sessions", auth_result);
    if (!auth_result.authorized) {
        THEMIS_WARN("SessionApiHandler::createSession – unauthorized: {}", auth_result.reason);
        return makeError(401, "Unauthorized: " + auth_result.reason);
    }

    const std::string& user_id = auth_result.user_id;

    std::string device_fingerprint;
    std::string user_agent;

    if (body.is_object()) {
        if (body.contains("device_fingerprint") && body["device_fingerprint"].is_string()) {
            device_fingerprint = body["device_fingerprint"].get<std::string>();
            if (!isSafeHeaderLikeValue(device_fingerprint, MAX_DEVICE_FINGERPRINT_LEN)) {
                return makeError(400, "Invalid device_fingerprint format");
            }
        }
        if (body.contains("user_agent") && body["user_agent"].is_string()) {
            user_agent = body["user_agent"].get<std::string>();
            if (!isSafeHeaderLikeValue(user_agent, MAX_USER_AGENT_LEN)) {
                return makeError(400, "Invalid user_agent format");
            }
        }
    }

    try {
        const std::string session_id = manager_->createSession(
            user_id, device_fingerprint, client_ip, user_agent);

        auto result = manager_->validateSession(session_id);
        if (!result.valid || !result.session.has_value()) {
            return makeError(500, "Failed to retrieve newly created session");
        }

        THEMIS_INFO("SessionApiHandler: created session '{}' for user '{}'",
                    session_id, user_id);

        return sessionToJson(*result.session);

    } catch (const std::exception& e) {
        THEMIS_ERROR("SessionApiHandler::createSession exception: {}", e.what());
        return makeError(500, "Internal error creating session");
    }
}

// ---------------------------------------------------------------------------
// listSessions
// ---------------------------------------------------------------------------

nlohmann::json SessionApiHandler::listSessions(
    const std::string& bearer_token,
    const std::string& current_session
) {
    auto span = Tracer::startSpan("listSessions");
    if (!isSafeHeaderLikeValue(bearer_token, 8192)) {
        return makeError(400, "Invalid bearer token format");
    }
    if (!current_session.empty() && !isSafeSessionIdentifier(current_session)) {
        return makeError(400, "current_session contains invalid characters or length");
    }

    auto auth_result = auth_->authorize(bearer_token, "auth:sessions");
    auditAuthorizationDecision("auth:sessions", "GET /auth/sessions", auth_result);
    if (!auth_result.authorized) {
        THEMIS_WARN("SessionApiHandler::listSessions – unauthorized: {}", auth_result.reason);
        return makeError(401, "Unauthorized: " + auth_result.reason);
    }

    const std::string& user_id = auth_result.user_id;
    const auto sessions = manager_->listSessions(user_id);

    nlohmann::json arr = nlohmann::json::array();
    for (const auto& s : sessions) {
        auto j = sessionToJson(s);
        j["is_current"] = (!current_session.empty() && s.session_id == current_session);
        arr.push_back(std::move(j));
    }

    return {
        {"sessions", arr},
        {"total",    arr.size()}
    };
}

// ---------------------------------------------------------------------------
// revokeSession
// ---------------------------------------------------------------------------

nlohmann::json SessionApiHandler::revokeSession(
    const std::string& bearer_token,
    const std::string& session_id
) {
    auto span = Tracer::startSpan("revokeSession");
    if (!isSafeHeaderLikeValue(bearer_token, 8192)) {
        return makeError(400, "Invalid bearer token format");
    }
    if (session_id.empty()) {
        return makeError(400, "session_id must not be empty");
    }
    if (!isSafeSessionIdentifier(session_id)) {
        return makeError(400, "session_id contains invalid characters or length");
    }

    auto auth_result = auth_->authorize(bearer_token, "auth:sessions");
    auditAuthorizationDecision("auth:sessions", "DELETE /auth/sessions/{session_id}", auth_result);
    if (!auth_result.authorized) {
        THEMIS_WARN("SessionApiHandler::revokeSession – unauthorized: {}", auth_result.reason);
        return makeError(401, "Unauthorized: " + auth_result.reason);
    }

    const std::string& caller_id = auth_result.user_id;

    // Admins may revoke any session; regular users only their own
    auto admin_result = auth_->authorize(bearer_token, "admin:all");
    auditAuthorizationDecision("admin:all", "DELETE /auth/sessions/{session_id}", admin_result);
    bool is_admin = admin_result.authorized;

    auto result = manager_->validateSession(session_id);
    if (!result.valid || !result.session.has_value()) {
        // Treat not-found and expired the same to avoid oracle attacks
        return makeError(404, "Session not found");
    }

    const std::string& owner_id = result.session->user_id;
    if (!is_admin && owner_id != caller_id) {
        THEMIS_WARN("SessionApiHandler::revokeSession – user '{}' attempted to revoke session "
                    "owned by '{}'", caller_id, owner_id);
        return makeError(403, "Forbidden: session belongs to a different user");
    }

    manager_->terminateSession(session_id);

    THEMIS_INFO("SessionApiHandler: revoked session '{}' (owner='{}', caller='{}')",
                session_id, owner_id, caller_id);

    return {
        {"success",    true},
        {"session_id", session_id}
    };
}

// ---------------------------------------------------------------------------
// revokeAllOtherSessions
// ---------------------------------------------------------------------------

nlohmann::json SessionApiHandler::revokeAllOtherSessions(
    const std::string& bearer_token,
    const std::string& current_session
) {
    auto span = Tracer::startSpan("revokeAllOtherSessions");
    if (!isSafeHeaderLikeValue(bearer_token, 8192)) {
        return makeError(400, "Invalid bearer token format");
    }
    if (!current_session.empty() && !isSafeSessionIdentifier(current_session)) {
        return makeError(400, "current_session contains invalid characters or length");
    }

    auto auth_result = auth_->authorize(bearer_token, "auth:sessions");
    auditAuthorizationDecision("auth:sessions", "DELETE /auth/sessions", auth_result);
    if (!auth_result.authorized) {
        THEMIS_WARN("SessionApiHandler::revokeAllOtherSessions – unauthorized: {}",
                    auth_result.reason);
        return makeError(401, "Unauthorized: " + auth_result.reason);
    }

    const std::string& user_id = auth_result.user_id;

    int terminated = manager_->terminateAllOtherSessions(user_id, current_session);

    THEMIS_INFO("SessionApiHandler: revoked {} sessions for user '{}' (kept '{}')",
                terminated, user_id, current_session);

    return {
        {"success",    true},
        {"terminated", terminated}
    };
}

} // namespace server
} // namespace themis
