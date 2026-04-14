/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            session_api_handler.cpp                            ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-14 18:52:03                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     255                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a2a0e15fab  2026-03-11  Changes before error encountered        ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 5a16800952  2026-02-24  feat(auth): implement session management and revocation e... ║
    • 125b23d98f  2026-02-24  feat(auth): implement session management and revocation e... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "server/session_api_handler.h"
#include "utils/logger.h"

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
    std::shared_ptr<auth::SessionManager> manager
)
    : auth_(std::move(auth))
    , manager_(std::move(manager))
{
    if (!auth_)    { throw std::invalid_argument("SessionApiHandler: auth must not be null"); }
    if (!manager_) { throw std::invalid_argument("SessionApiHandler: manager must not be null"); }
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

nlohmann::json SessionApiHandler::sessionToJson(const auth::SessionManager::SessionInfo& s) {
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

// ---------------------------------------------------------------------------
// createSession
// ---------------------------------------------------------------------------

nlohmann::json SessionApiHandler::createSession(
    const std::string& bearer_token,
    const nlohmann::json& body,
    const std::string& client_ip
) {
    auto span = Tracer::startSpan("createSession");
    auto auth_result = auth_->authorize(bearer_token, "auth:sessions");
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
        }
        if (body.contains("user_agent") && body["user_agent"].is_string()) {
            user_agent = body["user_agent"].get<std::string>();
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
    auto auth_result = auth_->authorize(bearer_token, "auth:sessions");
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
    if (session_id.empty()) {
        return makeError(400, "session_id must not be empty");
    }

    auto auth_result = auth_->authorize(bearer_token, "auth:sessions");
    if (!auth_result.authorized) {
        THEMIS_WARN("SessionApiHandler::revokeSession – unauthorized: {}", auth_result.reason);
        return makeError(401, "Unauthorized: " + auth_result.reason);
    }

    const std::string& caller_id = auth_result.user_id;

    // Admins may revoke any session; regular users only their own
    bool is_admin = auth_->authorize(bearer_token, "admin:all").authorized;

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
    auto auth_result = auth_->authorize(bearer_token, "auth:sessions");
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
