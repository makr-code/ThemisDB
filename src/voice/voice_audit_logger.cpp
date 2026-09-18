/**
 * @file voice_audit_logger.cpp
 * @brief VoiceAuditLogger implementation
 */

#include "voice/voice_audit_logger.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <ctime>

namespace themis {
namespace voice {

VoiceAuditLogger::VoiceAuditLogger(const Config& config)
    : config_(config) {
}

/**
 * @brief Log Authentication Attempt.
 * @param[in] user_id Identifier of the user.
 * @param[in] method Input parameter.
 * @param[in] success Input parameter.
 * @param[in] reason Input parameter.
 * @param[in] duration_ms Input parameter.
 * @param[in] session_id Identifier of the session.
 * @details Calls: getTimestamp(), empty(), writeEvent().
 */
void VoiceAuditLogger::logAuthenticationAttempt(
    const std::string& user_id,
    const std::string& method,
    bool success,
    const std::string& reason,
    int64_t duration_ms,
    const std::string& session_id) {
    
    if (!config_.enable_logging) {
        return;
    }

    json event;
    event["timestamp"] = getTimestamp();
    event["event_type"] = "VOICE_AUTH_ATTEMPT";
    event["user_id"] = user_id;
    event["method"] = method;
    event["result"] = success ? "PASS" : "FAIL";
    event["duration_ms"] = duration_ms;
    if (!session_id.empty()) {
        event["session_id"] = session_id;
    }
    event["reason"] = reason;

    writeEvent(event);
}

/**
 * @brief Log Session Lifecycle.
 * @param[in] session_id Identifier of the session.
 * @param[in] user_id Identifier of the user.
 * @param[in] event_type Input parameter.
 * @param[in] duration_ms Input parameter.
 * @param[in] bytes_transferred Input parameter.
 * @details Calls: getTimestamp(), writeEvent().
 */
void VoiceAuditLogger::logSessionLifecycle(
    const std::string& session_id,
    const std::string& user_id,
    const std::string& event_type,
    int64_t duration_ms,
    size_t bytes_transferred) {
    
    if (!config_.enable_logging) {
        return;
    }

    json event;
    event["timestamp"] = getTimestamp();
    event["event_type"] = "VOICE_SESSION_LIFECYCLE";
    event["session_id"] = session_id;
    event["user_id"] = user_id;
    event["event"] = event_type;
    event["duration_ms"] = duration_ms;
    event["bytes_transferred"] = static_cast<uint64_t>(bytes_transferred);

    writeEvent(event);
}

/**
 * @brief Log Liveness Challenge.
 * @param[in] user_id Identifier of the user.
 * @param[in] challenge_id Identifier of the challenge.
 * @param[in] event_type Input parameter.
 * @param[in] passed Input parameter.
 * @param[in] reason Input parameter.
 * @details Calls: getTimestamp(), empty(), writeEvent().
 */
void VoiceAuditLogger::logLivenessChallenge(
    const std::string& user_id,
    const std::string& challenge_id,
    const std::string& event_type,
    bool passed,
    const std::string& reason) {
    
    if (!config_.enable_logging) {
        return;
    }

    json event;
    event["timestamp"] = getTimestamp();
    event["event_type"] = "VOICE_LIVENESS_CHALLENGE";
    event["user_id"] = user_id;
    event["challenge_id"] = challenge_id;
    event["event"] = event_type;
    event["passed"] = passed;
    if (!reason.empty()) {
        event["reason"] = reason;
    }

    writeEvent(event);
}

/**
 * @brief Log Spoof Detection.
 * @param[in] user_id Identifier of the user.
 * @param[in] spoof_score Input parameter.
 * @param[in] verdict Input parameter.
 * @param[in] freshness_score Input parameter.
 * @param[in] speaker_match_score Input parameter.
 * @param[in] noise_consistency_score Input parameter.
 * @param[in] reason Input parameter.
 * @details Calls: getTimestamp(), empty(), writeEvent().
 */
void VoiceAuditLogger::logSpoofDetection(
    const std::string& user_id,
    double spoof_score,
    const std::string& verdict,
    double freshness_score,
    double speaker_match_score,
    double noise_consistency_score,
    const std::string& reason) {
    
    if (!config_.enable_logging) {
        return;
    }

    json event;
    event["timestamp"] = getTimestamp();
    event["event_type"] = "VOICE_SPOOF_DETECTION";
    event["user_id"] = user_id;
    event["spoof_score"] = spoof_score;
    event["verdict"] = verdict;
    event["freshness_score"] = freshness_score;
    event["speaker_match_score"] = speaker_match_score;
    event["noise_consistency_score"] = noise_consistency_score;
    if (!reason.empty()) {
        event["reason"] = reason;
    }

    writeEvent(event);
}

std::vector<json> VoiceAuditLogger::getEventLog() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    return event_log_;
}

std::vector<json> VoiceAuditLogger::getEventsForUser(const std::string& user_id) const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<json> user_events = {};

    for (const auto& event : event_log_) {
        if (event.contains("user_id") && event["user_id"].get<std::string>() == user_id) {
            user_events.push_back(event);
        }
    }
    return user_events;
}

/**
 * @brief Clear Event Log.
 * @details Calls: lock(), clear().
 */
void VoiceAuditLogger::clearEventLog() {
    std::lock_guard<std::mutex> lock(mutex_);
    event_log_.clear();
}

size_t VoiceAuditLogger::getEventCount() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    return event_log_.size();
}

void VoiceAuditLogger::setEventCallback(std::function<void(const json&)> callback) {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    event_callback_ = callback;
}

std::string VoiceAuditLogger::getTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::tm utc_time{};
#if defined(_WIN32)
    gmtime_s(&utc_time, &time_t_now);
#else
    gmtime_r(&time_t_now, &utc_time);
#endif

    std::stringstream ss = {};
    ss << std::put_time(&utc_time, "%Y-%m-%dT%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    return ss.str();
}

/**
 * @brief Write Event.
 * @param[in] event Input parameter.
 * @details Calls: void(), lock(), push_back(), callback(), serializeEvent(), empty(), log_file(), is_open().
 */
void VoiceAuditLogger::writeEvent(const json& event) {
    std::function<void(const json&)> callback;
    Config config_snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        event_log_.push_back(event);
        callback = event_callback_;
        config_snapshot = config_;
    }

    if (callback) {
        try {
            callback(event);
        } catch (...) {
            // Audit callbacks must never break event capture.
        }
    }

    const std::string serialized = serializeEvent(event);
    if (config_snapshot.log_to_console) {
        std::cerr << serialized << std::endl;
    }

    if (!config_snapshot.log_file_path.empty()) {
        std::ofstream log_file(config_snapshot.log_file_path, std::ios::app);
        if (log_file.is_open()) {
            log_file << serialized << "\n";
        }
    }
}

std::string VoiceAuditLogger::serializeEvent(const json& event) const {
    return event.dump();
}

}} // namespace themis::voice
