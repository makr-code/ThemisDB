/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            session_manager.cpp                                ║
  Version:         0.0.32                                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "auth/session_manager.h"
#include "utils/logger.h"

#include <openssl/rand.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>

namespace themis {
namespace auth {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

SessionManager::SessionManager()
    : limits_(SessionLimits{})
{}

SessionManager::SessionManager(const SessionLimits& limits)
    : limits_(limits)
{}

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

std::string SessionManager::generateSessionId() {
    unsigned char buf[32];
    if (RAND_bytes(buf, static_cast<int>(sizeof(buf))) != 1) {
        throw std::runtime_error("SessionManager: failed to generate secure random bytes");
    }
    std::ostringstream oss;
    oss << "sess_";
    for (auto b : buf) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    }
    return oss.str();
}

bool SessionManager::isExpired(const SessionInfo& s) const {
    auto now = std::chrono::system_clock::now();

    // Absolute timeout check
    if (now - s.created_at > limits_.absolute_timeout) {
        return true;
    }

    // Idle timeout check
    if (now - s.last_activity > limits_.idle_timeout) {
        return true;
    }

    return false;
}

void SessionManager::enforceSessionLimits(const std::string& user_id) {
    // Called while mutex_ is held.
    auto it = user_sessions_.find(user_id);
    if (it == user_sessions_.end()) {
        return;
    }

    auto& ids = it->second;

    // Remove entries that no longer exist in sessions_
    ids.erase(std::remove_if(ids.begin(), ids.end(),
        [this](const std::string& sid) {
            return sessions_.find(sid) == sessions_.end();
        }), ids.end());

    // While at or over the limit, remove the oldest session.
    // The `!ids.empty()` guard prevents UB when max_concurrent_sessions <= 0:
    // in that degenerate case the new session is added after enforcement, so the
    // first session per user always escapes the cap but subsequent ones evict it.
    while (!ids.empty() && static_cast<int>(ids.size()) >= limits_.max_concurrent_sessions) {
        // Find the session with the oldest created_at
        auto oldest_it = ids.begin();
        auto oldest_time = sessions_[*oldest_it].created_at;

        for (auto jt = ids.begin() + 1; jt != ids.end(); ++jt) {
            auto& candidate = sessions_[*jt];
            if (candidate.created_at < oldest_time) {
                oldest_time = candidate.created_at;
                oldest_it = jt;
            }
        }

        THEMIS_INFO("SessionManager: evicting oldest session '{}' for user '{}' (limit={})",
                    *oldest_it, user_id, limits_.max_concurrent_sessions);
        sessions_.erase(*oldest_it);
        ids.erase(oldest_it);
    }
}

// ---------------------------------------------------------------------------
// createSession
// ---------------------------------------------------------------------------

std::string SessionManager::createSession(
    const std::string& user_id,
    const std::string& device_fingerprint,
    const std::string& ip_address,
    const std::string& user_agent
) {
    if (user_id.empty()) {
        throw std::invalid_argument("SessionManager::createSession: user_id must not be empty");
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // Prune expired sessions first to keep the store tidy
    auto now = std::chrono::system_clock::now();
    for (auto it = sessions_.begin(); it != sessions_.end(); ) {
        if (isExpired(it->second)) {
            auto& ids = user_sessions_[it->second.user_id];
            ids.erase(std::remove(ids.begin(), ids.end(), it->first), ids.end());
            it = sessions_.erase(it);
        } else {
            ++it;
        }
    }

    // Enforce per-user session cap
    enforceSessionLimits(user_id);

    // Create new session
    SessionInfo info;
    info.session_id        = generateSessionId();
    info.user_id           = user_id;
    info.device_fingerprint = device_fingerprint;
    info.ip_address        = ip_address;
    info.user_agent        = user_agent;
    info.created_at        = now;
    info.last_activity     = now;

    sessions_[info.session_id] = info;
    user_sessions_[user_id].push_back(info.session_id);

    THEMIS_INFO("SessionManager: created session '{}' for user '{}' (ip={})",
                info.session_id, user_id, ip_address);

    return info.session_id;
}

// ---------------------------------------------------------------------------
// validateSession
// ---------------------------------------------------------------------------

SessionManager::ValidationResult SessionManager::validateSession(
    const std::string& session_id,
    const std::string& current_ip,
    const std::string& current_device_fingerprint
) {
    if (session_id.empty()) {
        return {false, std::nullopt, "session_id must not be empty"};
    }

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) {
        return {false, std::nullopt, "session not found"};
    }

    SessionInfo& s = it->second;

    if (isExpired(s)) {
        // Clean up the expired entry
        auto& ids = user_sessions_[s.user_id];
        ids.erase(std::remove(ids.begin(), ids.end(), session_id), ids.end());
        sessions_.erase(it);
        return {false, std::nullopt, "session expired"};
    }

    // IP pinning check
    if (limits_.pin_to_ip && !current_ip.empty() && s.ip_address != current_ip) {
        THEMIS_WARN("SessionManager: IP mismatch for session '{}': stored='{}' current='{}'",
                    session_id, s.ip_address, current_ip);
        return {false, std::nullopt, "session IP address mismatch"};
    }

    // Device pinning check
    if (limits_.pin_to_device && !current_device_fingerprint.empty() &&
        s.device_fingerprint != current_device_fingerprint) {
        THEMIS_WARN("SessionManager: device mismatch for session '{}'", session_id);
        return {false, std::nullopt, "session device fingerprint mismatch"};
    }

    // Refresh last-activity timestamp
    s.last_activity = std::chrono::system_clock::now();

    return {true, s, ""};
}

// ---------------------------------------------------------------------------
// listSessions
// ---------------------------------------------------------------------------

std::vector<SessionManager::SessionInfo> SessionManager::listSessions(
    const std::string& user_id,
    const std::string& current_session
) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto uit = user_sessions_.find(user_id);
    if (uit == user_sessions_.end()) {
        return {};
    }

    std::vector<SessionInfo> result;
    std::vector<std::string> valid_ids;

    for (const auto& sid : uit->second) {
        auto sit = sessions_.find(sid);
        if (sit == sessions_.end()) {
            continue; // already removed
        }
        if (isExpired(sit->second)) {
            sessions_.erase(sit);
            continue;
        }
        SessionInfo copy = sit->second;
        copy.is_current = (!current_session.empty() && copy.session_id == current_session);
        result.push_back(copy);
        valid_ids.push_back(sid);
    }

    // Update the index to only contain live sessions
    uit->second = std::move(valid_ids);

    return result;
}

// ---------------------------------------------------------------------------
// terminateSession
// ---------------------------------------------------------------------------

void SessionManager::terminateSession(const std::string& session_id) {
    if (session_id.empty()) return;

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) return;

    const std::string& uid = it->second.user_id;
    auto& ids = user_sessions_[uid];
    ids.erase(std::remove(ids.begin(), ids.end(), session_id), ids.end());
    sessions_.erase(it);

    THEMIS_INFO("SessionManager: terminated session '{}'", session_id);
}

// ---------------------------------------------------------------------------
// terminateAllOtherSessions
// ---------------------------------------------------------------------------

int SessionManager::terminateAllOtherSessions(
    const std::string& user_id,
    const std::string& keep_session_id
) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto uit = user_sessions_.find(user_id);
    if (uit == user_sessions_.end()) return 0;

    int count = 0;
    std::vector<std::string> remaining;

    for (const auto& sid : uit->second) {
        if (sid == keep_session_id) {
            remaining.push_back(sid);
            continue;
        }
        sessions_.erase(sid);
        ++count;
    }

    uit->second = std::move(remaining);

    THEMIS_INFO("SessionManager: terminated {} sessions for user '{}' (kept '{}')",
                count, user_id, keep_session_id);
    return count;
}

// ---------------------------------------------------------------------------
// terminateAllSessions
// ---------------------------------------------------------------------------

int SessionManager::terminateAllSessions(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto uit = user_sessions_.find(user_id);
    if (uit == user_sessions_.end()) return 0;

    int count = static_cast<int>(uit->second.size());
    for (const auto& sid : uit->second) {
        sessions_.erase(sid);
    }
    user_sessions_.erase(uit);

    THEMIS_INFO("SessionManager: terminated all {} sessions for user '{}'", count, user_id);
    return count;
}

// ---------------------------------------------------------------------------
// detectAnomalies
// ---------------------------------------------------------------------------

std::vector<SessionManager::Anomaly> SessionManager::detectAnomalies(
    const std::string& session_id
) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<Anomaly> anomalies;
    auto now = std::chrono::system_clock::now();

    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) return anomalies;

    const SessionInfo& s = it->second;

    // Check if idle timeout is about to be exceeded (within 10 % remaining)
    auto idle_elapsed = now - s.last_activity;
    if (idle_elapsed > limits_.idle_timeout * 9 / 10) {
        anomalies.push_back({
            AnomalyType::IdleTimeoutExceeded,
            40,
            "Session approaching idle timeout",
            now
        });
    }

    // Check if absolute timeout is about to be exceeded
    auto abs_elapsed = now - s.created_at;
    if (abs_elapsed > limits_.absolute_timeout * 9 / 10) {
        anomalies.push_back({
            AnomalyType::AbsoluteTimeoutExceeded,
            60,
            "Session approaching absolute lifetime limit",
            now
        });
    }

    // Check concurrent session count for the user
    auto uit = user_sessions_.find(s.user_id);
    if (uit != user_sessions_.end()) {
        int live_count = 0;
        for (const auto& sid : uit->second) {
            auto sit = sessions_.find(sid);
            if (sit != sessions_.end() && !isExpired(sit->second)) {
                ++live_count;
            }
        }
        if (live_count >= limits_.max_concurrent_sessions) {
            anomalies.push_back({
                AnomalyType::ConcurrentSessionLimitExceeded,
                50,
                "User has reached the maximum concurrent session limit",
                now
            });
        }
    }

    return anomalies;
}

// ---------------------------------------------------------------------------
// pruneExpired
// ---------------------------------------------------------------------------

void SessionManager::pruneExpired() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto it = sessions_.begin(); it != sessions_.end(); ) {
        if (isExpired(it->second)) {
            auto& ids = user_sessions_[it->second.user_id];
            ids.erase(std::remove(ids.begin(), ids.end(), it->first), ids.end());
            it = sessions_.erase(it);
        } else {
            ++it;
        }
    }
}

// ---------------------------------------------------------------------------
// size
// ---------------------------------------------------------------------------

size_t SessionManager::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.size();
}

} // namespace auth
} // namespace themis
