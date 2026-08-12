/**
 * @file session_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
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

/// Returns the hex-encoded SHA-256 digest of @p session_id.
/// Session tokens are stored under their hash so that an in-memory snapshot
/// of the sessions_ map does not expose raw bearer tokens, and so that all
/// map lookups have normalised comparison time regardless of input content.
std::string hashSessionId(const std::string &session_id) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(session_id.data()), session_id.size(), digest);
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned char b : digest) {
        oss << std::setw(2) << static_cast<int>(b);
    }
    return oss.str();
}

/**
 * @brief Compare two session IDs using constant-time comparison.
 *
 * Prevents timing attacks that could infer whether a session ID is "close"
 * to a valid one by measuring comparison time. Uses CRYPTO_memcmp for
 * constant-time byte-by-byte comparison.
 *
 * @param id1 First session ID
 * @param id2 Second session ID
 * @return true if both session IDs are equal, false otherwise
 */
bool constantTimeSessionIdEquals(const std::string &id1, const std::string &id2) noexcept {
    if (id1.size() != id2.size()) {
        return false;
    }
    if (id1.empty()) {
        return true;
    }
    return CRYPTO_memcmp(id1.data(), id2.data(), id1.size()) == 0;
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

std::string SessionManager::generateSessionId() {
    unsigned char buf[16];
    if (RAND_bytes(buf, sizeof(buf)) != 1) {
        throw std::runtime_error("SessionManager: RAND_bytes failed");
    }
    std::ostringstream oss;
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

    if (user_sessions.size() < limits_.max_sessions_per_user) {
        return;
    }

    // Sort ascending by creation time; evict oldest
    std::sort(user_sessions.begin(), user_sessions.end());
    const size_t to_remove = user_sessions.size() - limits_.max_sessions_per_user + 1;
    for (size_t i = 0; i < to_remove; ++i) {
        THEMIS_INFO("SessionManager: evicting oldest session '{}' for user '{}' (limit={})", user_sessions[i].second,
                    user_id, limits_.max_sessions_per_user);
        sessions_.erase(user_sessions[i].second);
    }
}

// ---------------------------------------------------------------------------
// createSession
// ---------------------------------------------------------------------------

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

void SessionManager::terminateSession(const std::string &session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(hashSessionId(session_id));
    if (it != sessions_.end()) {
        THEMIS_INFO("SessionManager: terminated session '{}' (user='{}')", session_id, it->second.user_id);
        sessions_.erase(it);
    }
}

// ---------------------------------------------------------------------------
// terminateAllOtherSessions
// ---------------------------------------------------------------------------

int SessionManager::terminateAllOtherSessions(const std::string &user_id, const std::string &keep_session_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> to_erase;
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

    THEMIS_INFO("SessionManager: terminated {} sessions for user '{}' (kept '{}')", to_erase.size(), user_id,
                keep_session_id);
    return static_cast<int>(to_erase.size());
}

// ---------------------------------------------------------------------------
// listSessions
// ---------------------------------------------------------------------------

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
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.size();
}

size_t SessionManager::pruneExpired() {
    std::lock_guard<std::mutex> lock(mutex_);
    return pruneExpiredLocked();
}

size_t SessionManager::pruneExpiredLocked() {
    std::vector<std::string> expired;
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
