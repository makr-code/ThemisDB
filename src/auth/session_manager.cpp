/**
 * @file session_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "auth/session_manager.h"

#include <algorithm>
#include <iomanip>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <sstream>
#include <stdexcept>

#include "utils/logger.h"

namespace themis {
namespace auth {

// ---------------------------------------------------------------------------
// File-local helper
// ---------------------------------------------------------------------------

namespace {

/**
 * @brief Hash Session Id.
 * @param[in] session_id Identifier of the session.
 * @return Return value.
 * @details Calls: SHA256(), data(), size(), std::setfill(), std::setw(), str().
 */
std::string hashSessionId(const std::string &session_id) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(session_id.data()),session_id.size(), digest);
    std::ostringstream oss = {};
    oss << std::hex << std::setfill('0');
    for (unsigned char b : digest) {
        oss << std::setw(2) << static_cast<int>(b);
    }
    return oss.str();
}

bool constantTimeSessionIdEquals(const std::string &id1, const std::string &id2) noexcept {
    if (id1.size() != id2.size()) {
        return false;
    }
    if (id1.empty()) {
        return true;
    }
    return CRYPTO_memcmp(id1.data(), id2.data(),id1.size()) == 0;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

SessionManager::SessionManager() : limits_(SessionLimits{}) {}

SessionManager::SessionManager(const SessionLimits &limits) : limits_(limits) {}

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

/**
 * @brief Generate Session Id.
 * @return Return value.
 * @throws std::runtime_error if an error occurs.
 * @details Calls: RAND_bytes(), std::setw(), std::setfill(), str().
 */
std::string SessionManager::generateSessionId() {
    unsigned char buf[16];
    if (RAND_bytes(buf, sizeof(buf)) != 1) {
        throw std::runtime_error("SessionManager: RAND_bytes failed");
    }
    std::ostringstream oss = {};
    oss << "sess_";
    for (unsigned char b : buf) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    }
    return oss.str();
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

bool SessionManager::isExpired(const SessionInfo &s) const {
    const auto now = std::chrono::system_clock::now();

    // Absolute timeout
    if (limits_.absolute_timeout.count() > 0) {
        if (now > s.expires_at) {
            return true;
        }
    }

    // Idle timeout
    if (limits_.idle_timeout.count() > 0) {
        if (now - s.last_accessed_at > limits_.idle_timeout) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Enforce Session Limits.
 * @param[in] user_id Identifier of the user.
 * @details Calls: emplace_back(), size(), std::sort(), begin(), end(), THEMIS_INFO(), erase().
 */
void SessionManager::enforceSessionLimits(const std::string &user_id) {
    if (limits_.max_sessions_per_user == 0) {
        return;
    }

    // Collect all session IDs for this user, ordered by creation time
    std::vector<std::pair<std::chrono::system_clock::time_point, std::string>> user_sessions;
    for (const auto &[id, info] : sessions_) {
        if (info.user_id == user_id) {
            user_sessions.emplace_back(info.created_at, id);
        }
    }

    if (user_sessions.size() < static_cast<size_t>(limits_.max_sessions_per_user)) {
        return;
    }

    // Sort ascending by creation time; evict oldest
    std::sort(user_sessions.begin(), user_sessions.end());
    const size_t max_sessions = static_cast<size_t>(limits_.max_sessions_per_user);
    const size_t to_remove = user_sessions.size() - max_sessions + 1;
    for (size_t i = 0; i < to_remove; ++i) {
        THEMIS_INFO("SessionManager: evicting oldest session '{}' for user '{}' (limit={})", user_sessions[i].second,
                    user_id, limits_.max_sessions_per_user);
        sessions_.erase(user_sessions[i].second);
    }
}

// ---------------------------------------------------------------------------
// createSession
// ---------------------------------------------------------------------------

/**
 * @brief Create a session for an authenticated user.
 * @param[in] user_id User identifier.
 * @param[in] device_fingerprint Input parameter.
 * @param[in] ip_address Input parameter.
 * @param[in] user_agent Input parameter.
 * @return Session token.
 * @throws std::invalid_argument if an error occurs.
 * @details Calls: empty(), lock(), pruneExpiredLocked(), enforceSessionLimits(), std::chrono::system_clock::now(), count(), std::chrono::system_clock::time_point::max(), generateSessionId().
 */
std::string SessionManager::createSession(const std::string &user_id, const std::string &device_fingerprint,
                                          const std::string &ip_address, const std::string &user_agent) {
    if (user_id.empty()) {
        throw std::invalid_argument("SessionManager::createSession: user_id must not be empty");
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // Prune expired sessions first to keep the store bounded
    pruneExpiredLocked();

    // Enforce per-user session limit (evict oldest if needed)
    enforceSessionLimits(user_id);

    const auto now        = std::chrono::system_clock::now();
    const auto expires_at = (limits_.absolute_timeout.count() > 0) ? now + limits_.absolute_timeout
                                                                   : std::chrono::system_clock::time_point::max();

    const std::string session_id = generateSessionId();

    SessionInfo info;
    info.session_id         = session_id;
    info.user_id            = user_id;
    info.device_fingerprint = device_fingerprint;
    info.ip_address         = ip_address;
    info.user_agent         = user_agent;
    info.created_at         = now;
    info.last_accessed_at   = now;
    info.expires_at         = expires_at;

    sessions_.emplace(hashSessionId(session_id), std::move(info));

    THEMIS_INFO("SessionManager: created session '{}' for user '{}'", session_id, user_id);
    return session_id;
}

// ---------------------------------------------------------------------------
// validateSession
// ---------------------------------------------------------------------------

/**
 * @brief Validate a session token.
 * @param[in] session_id Identifier of the session.
 * @param[in] param Input parameter.
 * @return Validated session on success.
 * @details Calls: empty(), lock(), find(), hashSessionId(), end(), isExpired(), erase(), std::chrono::system_clock::now().
 */
SessionManager::ValidationResult SessionManager::validateSession(const std::string &session_id,
                                                                 const std::string & /*current_ip*/
) {
    if (session_id.empty()) {
        return {false, std::nullopt, "session_id must not be empty"};
    }

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = sessions_.find(hashSessionId(session_id));
    if (it == sessions_.end()) {
        return {false, std::nullopt, "session not found"};
    }

    SessionInfo &s = it->second;

    if (isExpired(s)) {
        sessions_.erase(it);
        return {false, std::nullopt, "session expired"};
    }

    // Update last-accessed timestamp
    s.last_accessed_at = std::chrono::system_clock::now();

    return {true, s, {}};
}

// ---------------------------------------------------------------------------
// terminateSession
// ---------------------------------------------------------------------------

/**
 * @brief Terminate Session.
 * @param[in] session_id Identifier of the session.
 * @details Calls: lock(), find(), hashSessionId(), end(), THEMIS_INFO(), erase().
 */
void SessionManager::terminateSession(const std::string &session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(hashSessionId(session_id));
    if (it != sessions_.end()) {
        THEMIS_INFO("SessionManager: terminated session '{}' (user='{}')", session_id, it->second.user_id);
        sessions_.erase(it);
    }
}

// ---------------------------------------------------------------------------
// Backward-compatibility alias
// ---------------------------------------------------------------------------

// invalidateSession() is intentionally provided as a compatibility shim to the
// modern terminateSession() API. The canonical public name remains
// terminateSession() and is the one used by current code paths.

// ---------------------------------------------------------------------------
// terminateAllOtherSessions
// ---------------------------------------------------------------------------

/**
 * @brief Terminate All Other Sessions.
 * @param[in] user_id Identifier of the user.
 * @param[in] keep_session_id Identifier of the keep session.
 * @return Return value.
 * @details Calls: lock(), constantTimeSessionIdEquals(), push_back(), erase(), THEMIS_INFO(), size().
 */
int SessionManager::terminateAllOtherSessions(const std::string &user_id, const std::string &keep_session_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> to_erase = {};

    for (const auto &[id, info] : sessions_) {
        // `id` is the SHA-256 hash of the original token; compare against
        // info.session_id (which holds the original token) so that the raw
        // keep_session_id can be matched correctly.
        // Use constant-time comparison to prevent timing attacks on session IDs.
        if (info.user_id == user_id && !constantTimeSessionIdEquals(info.session_id, keep_session_id)) {
            to_erase.push_back(id);
        }
    }
    for (const auto &id : to_erase) {
        sessions_.erase(id);
    }

    THEMIS_INFO("SessionManager: terminated {} sessions for user '{}' (kept '{}')",to_erase.size(), user_id,
                keep_session_id);
    return to_erase.size();
}

// ---------------------------------------------------------------------------
// listSessions
// ---------------------------------------------------------------------------

/**
 * @brief List Sessions.
 * @param[in] user_id Identifier of the user.
 * @return Return value.
 * @details Calls: lock(), isExpired(), push_back(), erase(), std::sort(), begin(), end().
 */
std::vector<SessionManager::SessionInfo> SessionManager::listSessions(const std::string &user_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Remove expired entries while we iterate
    std::vector<std::string> expired_ids;
    std::vector<SessionInfo> result;

    for (const auto &[id, info] : sessions_) {
        if (info.user_id != user_id) {
            continue;
        }
        if (isExpired(info)) {
            expired_ids.push_back(id);
        } else {
            result.push_back(info);
        }
    }
    for (const auto &id : expired_ids) {
        sessions_.erase(id);
    }

    // Sort by creation time, oldest first
    std::sort(result.begin(), result.end(),
              [](const SessionInfo &a, const SessionInfo &b) { return a.created_at < b.created_at; });
    return result;
}

// ---------------------------------------------------------------------------
// size / pruneExpired
// ---------------------------------------------------------------------------

size_t SessionManager::size() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.size();
}

/**
 * @brief Prune Expired.
 * @return Return value.
 * @details Calls: lock(), pruneExpiredLocked().
 */
size_t SessionManager::pruneExpired() {
    std::lock_guard<std::mutex> lock(mutex_);
    return pruneExpiredLocked();
}

/**
 * @brief Prune Expired Locked.
 * @return Return value.
 * @details Calls: isExpired(), push_back(), erase(), size().
 */
size_t SessionManager::pruneExpiredLocked() {
    std::vector<std::string> expired = {};

    for (const auto &[id, info] : sessions_) {
        if (isExpired(info)) {
            expired.push_back(id);
        }
    }
    for (const auto &id : expired) {
        sessions_.erase(id);
    }
    return expired.size();
}

} // namespace auth
} // namespace themis

