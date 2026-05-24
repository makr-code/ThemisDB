/*
 * ThemisDB | File: voice_session_manager.cpp | Version: 0.0.42 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 303
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=110 | delta=107 | status=divergent
 * External Severity (v3): C=16, H=82, M=12
 * PR: none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file voice_session_manager.cpp
 * @brief Session management with persistence (Phase 6 production readiness)
 */

#include "voice/voice_session_manager.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <random>
#include <stdexcept>

namespace themis { namespace voice {

// ---- Free functions ----

std::string sessionStateToString(SessionState state) {
    switch (state) {
        case SessionState::ACTIVE:     return "ACTIVE";
        case SessionState::IDLE:       return "IDLE";
        case SessionState::EXPIRED:    return "EXPIRED";
        case SessionState::TERMINATED: return "TERMINATED";
        default:                       return "UNKNOWN";
    }
}

// ---- InMemorySessionBackend ----

bool InMemorySessionBackend::save(const VoiceSessionData& session) {
    std::lock_guard<std::mutex> lock(mutex_);
    store_[session.session_id] = session;
    return true;
}

std::optional<VoiceSessionData> InMemorySessionBackend::load(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = store_.find(session_id);
    if (it == store_.end()) return std::nullopt;
    return it->second;
}

bool InMemorySessionBackend::remove(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return store_.erase(session_id) > 0;
}

std::vector<std::string> InMemorySessionBackend::listActiveSessions() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> ids;
    ids.reserve(store_.size());
    for (const auto& [id, _] : store_) {
        ids.push_back(id);
    }
    return ids;
}

size_t InMemorySessionBackend::count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return store_.size();
}

// ---- VoiceSessionManager ----

VoiceSessionManager::VoiceSessionManager(
    const SessionTimeoutConfig& timeout_config,
    std::unique_ptr<ISessionPersistenceBackend> backend)
    : timeout_config_(timeout_config)
    , backend_(backend ? std::move(backend) : std::make_unique<InMemorySessionBackend>())
{}

int64_t VoiceSessionManager::nowMs() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

bool VoiceSessionManager::isExpired(const VoiceSessionData& session) const {
    if (!timeout_config_.auto_expire) return false;
    int64_t now = nowMs();

    // Check max session duration
    if (now - session.created_at_ms > timeout_config_.max_session_duration_ms) return true;

    // Check idle timeout
    if (now - session.last_activity_ms > timeout_config_.idle_timeout_ms) return true;

    return false;
}

std::string VoiceSessionManager::generateSessionId() {
    int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    std::mt19937 rng(static_cast<unsigned>(now_ms));
    std::uniform_int_distribution<uint16_t> dist(0, 0xFFFF);
    uint16_t rnd = dist(rng);

    std::ostringstream oss;
    oss << "sess_" << std::hex << std::setw(12) << std::setfill('0') << now_ms
        << std::setw(4) << std::setfill('0') << rnd;
    return oss.str();
}

VoiceSessionData VoiceSessionManager::createSession(
    const std::string& user_id, const std::string& device_id)
{
    VoiceSessionData session;
    session.session_id = generateSessionId();
    session.user_id = user_id;
    session.device_id = device_id;
    session.state = SessionState::ACTIVE;

    int64_t now = nowMs();
    session.created_at_ms = now;
    session.last_activity_ms = now;
    session.expires_at_ms = now + timeout_config_.max_session_duration_ms;

    {
        std::lock_guard<std::mutex> lock(manager_mutex_);
        active_cache_[session.session_id] = session;
    }
    const bool saved = backend_->save(session);
    static_cast<void>(saved);
    return session;
}

std::optional<VoiceSessionData> VoiceSessionManager::getSession(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    auto it = active_cache_.find(session_id);
    if (it != active_cache_.end()) {
        if (isExpired(it->second)) {
            it->second.state = SessionState::EXPIRED;
            const bool saved = backend_->save(it->second);
            static_cast<void>(saved);
            return std::nullopt;
        }
        return it->second;
    }

    // Try persistent backend
    auto loaded = backend_->load(session_id);
    if (!loaded) return std::nullopt;
    if (isExpired(*loaded)) {
        loaded->state = SessionState::EXPIRED;
        const bool saved = backend_->save(*loaded);
        static_cast<void>(saved);
        return std::nullopt;
    }
    active_cache_[session_id] = *loaded;
    return loaded;
}

bool VoiceSessionManager::updateSession(
    const std::string& session_id, const json& context_update)
{
    std::lock_guard<std::mutex> lock(manager_mutex_);
    auto it = active_cache_.find(session_id);
    if (it == active_cache_.end()) return false;

    // Merge context
    if (it->second.context.is_object() && context_update.is_object()) {
        for (const auto& [k, v] : context_update.items()) {
            it->second.context[k] = v;
        }
    } else {
        it->second.context = context_update;
    }
    it->second.last_activity_ms = nowMs();
    const bool saved = backend_->save(it->second);
    static_cast<void>(saved);
    return true;
}

bool VoiceSessionManager::addConversationTurn(
    const std::string& session_id,
    const std::string& user_msg,
    const std::string& assistant_msg)
{
    std::lock_guard<std::mutex> lock(manager_mutex_);
    auto it = active_cache_.find(session_id);
    if (it == active_cache_.end()) return false;

    it->second.conversation_history.push_back("User: " + user_msg);
    it->second.conversation_history.push_back("Assistant: " + assistant_msg);
    it->second.total_turns++;
    it->second.last_activity_ms = nowMs();
    const bool saved = backend_->save(it->second);
    static_cast<void>(saved);
    return true;
}

bool VoiceSessionManager::touchSession(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    auto it = active_cache_.find(session_id);
    if (it == active_cache_.end()) return false;
    it->second.last_activity_ms = nowMs();
    const bool saved = backend_->save(it->second);
    static_cast<void>(saved);
    return true;
}

bool VoiceSessionManager::updatePreferredLanguage(
    const std::string& session_id, const std::string& language_code)
{
    std::lock_guard<std::mutex> lock(manager_mutex_);
    auto it = active_cache_.find(session_id);
    if (it == active_cache_.end()) return false;
    it->second.preferred_language = language_code;
    it->second.last_activity_ms = nowMs();
    const bool saved = backend_->save(it->second);
    static_cast<void>(saved);
    return true;
}

bool VoiceSessionManager::terminateSession(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    auto it = active_cache_.find(session_id);
    if (it == active_cache_.end()) return false;

    it->second.state = SessionState::TERMINATED;
    const bool saved = backend_->save(it->second);
    static_cast<void>(saved);
    active_cache_.erase(it);
    return true;
}

size_t VoiceSessionManager::expireOldSessions() {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    size_t expired = 0;
    for (auto& [id, session] : active_cache_) {
        if (isExpired(session)) {
            session.state = SessionState::EXPIRED;
            const bool saved = backend_->save(session);
            static_cast<void>(saved);
            ++expired;
        }
    }
    // Remove expired from cache
    for (auto it = active_cache_.begin(); it != active_cache_.end(); ) {
        if (it->second.state == SessionState::EXPIRED ||
            it->second.state == SessionState::TERMINATED) {
            it = active_cache_.erase(it);
        } else {
            ++it;
        }
    }
    return expired;
}

std::vector<VoiceSessionData> VoiceSessionManager::getSessionsForUser(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    std::vector<VoiceSessionData> result;
    for (const auto& [id, session] : active_cache_) {
        if (session.user_id == user_id) result.push_back(session);
    }
    return result;
}

SessionAnalytics VoiceSessionManager::getAnalytics() const {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    SessionAnalytics analytics;
    analytics.total_sessions = active_cache_.size();

    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    double total_duration = 0.0;
    double total_turns = 0.0;

    for (const auto& [id, session] : active_cache_) {
        if (session.state == SessionState::ACTIVE || session.state == SessionState::IDLE) {
            ++analytics.active_sessions;
        } else if (session.state == SessionState::EXPIRED) {
            ++analytics.expired_sessions;
        }
        total_duration += static_cast<double>(now - session.created_at_ms);
        total_turns += session.total_turns;

        if (!session.device_id.empty()) {
            analytics.sessions_by_device[session.device_id]++;
        }
        analytics.sessions_by_language[session.preferred_language]++;
    }

    if (analytics.total_sessions > 0) {
        analytics.avg_session_duration_ms = total_duration / analytics.total_sessions;
        analytics.avg_turns_per_session   = total_turns / analytics.total_sessions;
    }

    return analytics;
}

bool VoiceSessionManager::isSessionActive(const std::string& session_id) {
    auto s = getSession(session_id);
    return s.has_value() && s->state == SessionState::ACTIVE;
}

SessionState VoiceSessionManager::getSessionState(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    auto it = active_cache_.find(session_id);
    if (it == active_cache_.end()) return SessionState::EXPIRED;
    return it->second.state;
}

}} // namespace themis::voice
