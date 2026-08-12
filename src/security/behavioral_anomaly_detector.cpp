/**
 * @file behavioral_anomaly_detector.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "security/behavioral_anomaly_detector.h"
#include "utils/logger.h"

#include <algorithm>
#include <ctime>
#include <sstream>

namespace themis {
namespace security {

// ============================================================================
// Constructor
// ============================================================================

BehavioralAnomalyDetector::BehavioralAnomalyDetector(const Config& config)
    : config_(config) {}

// ============================================================================
// Public API
// ============================================================================

ThreatScore BehavioralAnomalyDetector::scoreEvent(const AccessEvent& event) {
    // Input validation: basic sanity checks
    if (event.session_id.empty()) {
        THEMIS_WARN("BehavioralAnomalyDetector: scoreEvent called with empty session_id");
        return {ThreatLevel::LOW, 0.0, "Empty session_id"};
    }
    if (event.user_id.empty()) {
        THEMIS_WARN("BehavioralAnomalyDetector: scoreEvent called with empty user_id");
        return {ThreatLevel::LOW, 0.0, "Empty user_id"};
    }
    if (event.action.empty()) {
        THEMIS_WARN("BehavioralAnomalyDetector: scoreEvent called with empty action");
        return {ThreatLevel::LOW, 0.0, "Empty action"};
    }
    
    std::lock_guard<std::mutex> lock(mutex_);

    auto& state = sessions_[event.session_id];

    // Append event; evict oldest if ring buffer is full.
    state.events.push_back(event);
    while (state.events.size() > config_.max_events_per_session) {
        state.events.pop_front();
    }

    // Evaluate all heuristics; take the highest threat.
    ThreatScore result;
    result.level = ThreatLevel::LOW;
    result.score = 0.0;

    result = maxScore(result, checkBurstRate(state, event));
    result = maxScore(result, checkOffHours(event));
    result = maxScore(result, checkPrivilegeEscalation(state, event));
    result = maxScore(result, checkUnusualResource(state, event));

    // Track peak threat level for the session.
    if (static_cast<int>(result.level) > static_cast<int>(state.peak_level)) {
        state.peak_level = result.level;
    }

    if (result.level != ThreatLevel::LOW) {
        THEMIS_WARN("BehavioralAnomalyDetector: {} threat for session='{}' "
                    "user='{}' score={:.3f} reason='{}'",
                    result.level == ThreatLevel::CRITICAL ? "CRITICAL" :
                    result.level == ThreatLevel::HIGH     ? "HIGH"     : "MEDIUM",
                    event.session_id, event.user_id,
                    result.score, result.explanation);
    }

    return result;
}

void BehavioralAnomalyDetector::clearSession(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(session_id);
}

size_t BehavioralAnomalyDetector::sessionEventCount(
    const std::string& session_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(session_id);
    return it != sessions_.end() ? it->second.events.size() : 0u;
}

// ============================================================================
// Heuristics
// ============================================================================

ThreatScore BehavioralAnomalyDetector::checkBurstRate(
    const SessionState& state, const AccessEvent& event) const {
    // Early exit: burst rate check disabled if threshold <= 0 (config-based disable)
    if (config_.burst_rate_threshold <= 0.0) {
        return {};  // Returns empty ThreatScore (LOW level, 0.0 score, no explanation)
    }

    auto window_start = event.timestamp - config_.burst_window;
    size_t count = 0;
    for (const auto& e : state.events) {
        if (e.timestamp >= window_start) ++count;
    }

    double window_s = static_cast<double>(config_.burst_window.count());
    // Sanity check: if window is invalid, skip this heuristic
    if (window_s <= 0.0) {
        return {};  // Returns empty ThreatScore (misconfiguration guard)
    }

    double rate = static_cast<double>(count) / window_s;
    if (rate > config_.burst_rate_threshold) {
        std::ostringstream oss;
        oss << "Burst rate " << rate << " req/s exceeds threshold "
            << config_.burst_rate_threshold << " req/s";
        return {ThreatLevel::HIGH, levelToScore(ThreatLevel::HIGH), oss.str()};
    }
    return {};
}

ThreatScore BehavioralAnomalyDetector::checkOffHours(
    const AccessEvent& event) const {
    // Off-hours detection disabled when start == end (config-based disable)
    if (config_.work_hours_start_utc == config_.work_hours_end_utc) {
        return {};  // Returns empty ThreatScore when feature is disabled
    }

    // Extract UTC hour from event timestamp
    auto tt = std::chrono::system_clock::to_time_t(event.timestamp);
    struct std::tm utc_tm{};
#if defined(_WIN32)
    gmtime_s(&utc_tm, &tt);
#else
    gmtime_r(&tt, &utc_tm);
#endif
    int hour = utc_tm.tm_hour;

    bool in_hours = (config_.work_hours_start_utc <= config_.work_hours_end_utc)
        ? (hour >= config_.work_hours_start_utc && hour < config_.work_hours_end_utc)
        : (hour >= config_.work_hours_start_utc || hour < config_.work_hours_end_utc);

    if (!in_hours) {
        std::ostringstream oss;
        oss << "Off-hours access at UTC hour " << hour
            << " (work hours: " << config_.work_hours_start_utc
            << "-" << config_.work_hours_end_utc << ")";
        return {ThreatLevel::MEDIUM, levelToScore(ThreatLevel::MEDIUM), oss.str()};
    }
    return {};
}

ThreatScore BehavioralAnomalyDetector::checkPrivilegeEscalation(
    const SessionState& state, const AccessEvent& event) const {
    // Check if the current action is privileged
    bool is_privileged = std::find(config_.privileged_actions.begin(),
                                    config_.privileged_actions.end(),
                                    event.action) != config_.privileged_actions.end();
    // Early exit: action is not privileged, no escalation risk
    if (!is_privileged) return {};  // Not a privileged action, skip this check

    // Check if the resource was accessed before in this session
    bool resource_seen = false;
    size_t event_count = state.events.size();
    // The event was already appended; check all but the last entry
    for (size_t i = 0; i + 1 < event_count; ++i) {
        if (state.events[i].resource == event.resource) {
            resource_seen = true;
            break;
        }
    }

    if (!resource_seen) {
        std::ostringstream oss;
        oss << "Privileged action '" << event.action
            << "' on previously-unaccessed resource '" << event.resource << "'";
        return {ThreatLevel::HIGH, levelToScore(ThreatLevel::HIGH), oss.str()};
    }
    return {};  // Resource has been seen before, no escalation threat
}

ThreatScore BehavioralAnomalyDetector::checkUnusualResource(
    const SessionState& state, const AccessEvent& event) const {
    // Only flag as unusual if the session has been previously flagged (elevated threat state)
    if (state.peak_level == ThreatLevel::LOW) {
        return {};  // Session has no history of anomalies, skip this check
    }

    // If the resource has not been seen before in this session, flag it
    size_t event_count = state.events.size();
    bool resource_seen = false;
    for (size_t i = 0; i + 1 < event_count; ++i) {
        if (state.events[i].resource == event.resource) {
            resource_seen = true;
            break;
        }
    }

    if (!resource_seen) {
        std::ostringstream oss;
        oss << "New resource '" << event.resource
            << "' accessed by already-flagged session";
        return {ThreatLevel::MEDIUM, levelToScore(ThreatLevel::MEDIUM), oss.str()};
    }
    return {};
}

// ============================================================================
// Helpers
// ============================================================================

ThreatScore BehavioralAnomalyDetector::maxScore(const ThreatScore& a,
                                                  const ThreatScore& b) {
    if (static_cast<int>(b.level) > static_cast<int>(a.level)) return b;
    if (static_cast<int>(b.level) == static_cast<int>(a.level) && b.score > a.score) return b;
    return a;
}

double BehavioralAnomalyDetector::levelToScore(ThreatLevel lvl) noexcept {
    switch (lvl) {
        case ThreatLevel::CRITICAL: return 1.0;
        case ThreatLevel::HIGH:     return 0.75;
        case ThreatLevel::MEDIUM:   return 0.5;
        case ThreatLevel::LOW:
        default:                    return 0.0;
    }
}

} // namespace security
} // namespace themis

