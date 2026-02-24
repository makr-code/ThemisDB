/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            session_api_handler.cpp                            ║
  Version:         0.0.32                                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "server/session_api_handler.h"
#include "server/auth_middleware.h"
#include "auth/session_manager.h"
#include "utils/logger.h"

#include <chrono>
#include <iomanip>
#include <sstream>

namespace themis {
namespace server {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

SessionApiHandler::SessionApiHandler(
    std::shared_ptr<AuthMiddleware> auth,
    std::shared_ptr<auth::SessionManager> manager
)
    : auth_(std::move(auth))
    , manager_(std::move(manager))
{}

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

nlohmann::json SessionApiHandler::makeError(int status_code, const std::string& message) {
    return {
        {"error",       message},
        {"status_code", status_code}
    };
}

static std::string formatTimePoint(std::chrono::system_clock::time_point tp) {
    auto t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm_buf{};
#ifdef _WIN32
    gmtime_s(&tm_buf, &t);
#else
    gmtime_r(&t, &tm_buf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

nlohmann::json SessionApiHandler::sessionToJson(
    const auth::SessionManager::SessionInfo& s
) {
    nlohmann::json obj = {
        {"session_id",          s.session_id},
        {"user_id",             s.user_id},
        {"device_fingerprint",  s.device_fingerprint},
        {"ip_address",          s.ip_address},
        {"user_agent",          s.user_agent},
        {"created_at",          formatTimePoint(s.created_at)},
        {"last_activity",       formatTimePoint(s.last_activity)},
        {"is_current",          s.is_current}
    };
    return obj;
}

// ---------------------------------------------------------------------------
// createSession
// ---------------------------------------------------------------------------

nlohmann::json SessionApiHandler::createSession(
    const std::string& bearer_token,
    const nlohmann::json& body,
    const std::string& client_ip
) {
    // Validate caller identity (any authenticated user may create a session)
    auto auth_result = auth_->authorize(bearer_token, "auth:sessions");
    if (!auth_result.authorized) {
        THEMIS_WARN("SessionApiHandler::createSession – unauthorized: {}", auth_result.reason);
        return makeError(401, "Unauthorized: " + auth_result.reason);
    }

    const std::string& user_id = auth_result.user_id;

    std::string device_fp;
    std::string user_agent;

    if (body.is_object()) {
        if (body.contains("device_fingerprint") && body["device_fingerprint"].is_string()) {
            device_fp = body["device_fingerprint"].get<std::string>();
        }
        if (body.contains("user_agent") && body["user_agent"].is_string()) {
            user_agent = body["user_agent"].get<std::string>();
        }
    }

    try {
        std::string session_id = manager_->createSession(user_id, device_fp, client_ip, user_agent);

        THEMIS_INFO("SessionApiHandler: created session '{}' for user '{}'", session_id, user_id);

        return {
            {"session_id", session_id},
            {"user_id",    user_id},
            {"created_at", formatTimePoint(std::chrono::system_clock::now())}
        };
    } catch (const std::exception& e) {
        THEMIS_ERROR("SessionApiHandler::createSession error: {}", e.what());
        return makeError(500, std::string("Internal error: ") + e.what());
    }
}

// ---------------------------------------------------------------------------
// listSessions
// ---------------------------------------------------------------------------

nlohmann::json SessionApiHandler::listSessions(
    const std::string& bearer_token,
    const std::string& current_session
) {
    auto auth_result = auth_->authorize(bearer_token, "auth:sessions");
    if (!auth_result.authorized) {
        THEMIS_WARN("SessionApiHandler::listSessions – unauthorized: {}", auth_result.reason);
        return makeError(401, "Unauthorized: " + auth_result.reason);
    }

    const std::string& user_id = auth_result.user_id;

    auto sessions = manager_->listSessions(user_id, current_session);

    nlohmann::json arr = nlohmann::json::array();
    for (const auto& s : sessions) {
        arr.push_back(sessionToJson(s));
    }

    return {
        {"sessions", arr},
        {"total",    static_cast<int>(sessions.size())}
    };
}

// ---------------------------------------------------------------------------
// revokeSession
// ---------------------------------------------------------------------------

nlohmann::json SessionApiHandler::revokeSession(
    const std::string& bearer_token,
    const std::string& session_id
) {
    if (session_id.empty()) {
        return makeError(400, "session_id must not be empty");
    }

    auto auth_result = auth_->authorize(bearer_token, "auth:sessions");
    if (!auth_result.authorized) {
        THEMIS_WARN("SessionApiHandler::revokeSession – unauthorized: {}", auth_result.reason);
        return makeError(401, "Unauthorized: " + auth_result.reason);
    }

    const std::string& caller_id = auth_result.user_id;

    // Verify ownership: the session must belong to the caller unless the caller
    // has admin privileges.
    bool is_admin = auth_->authorize(bearer_token, "admin:all").authorized;

    // Check that the session exists and belongs to the caller
    auto result = manager_->validateSession(session_id);
    if (!result.valid || !result.session.has_value()) {
        // Treat not-found and expired the same way to avoid oracle attacks
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
