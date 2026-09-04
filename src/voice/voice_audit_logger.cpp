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

    writeEvent([[maybe_unused]] event);
}

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
    event["bytes_transferred"] = static_cast<uint64_t>([[maybe_unused]] bytes_transferred);

    writeEvent([[maybe_unused]] event);
}

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

    writeEvent([[maybe_unused]] event);
}

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

    writeEvent([[maybe_unused]] event);
}

std::vector<json> VoiceAuditLogger::getEventLog() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return event_log_;
}

std::vector<json> VoiceAuditLogger::getEventsForUser([[maybe_unused]] const std::string& user_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<json> user_events = {};

    for ([[maybe_unused]] const auto& event : event_log_) {
        if ([[maybe_unused]] event.contains("user_id") && event["user_id"].get<std::string>() == user_id) {
            user_events.push_back([[maybe_unused]] event);
        }
    }
    return user_events;
}

void VoiceAuditLogger::clearEventLog() {
    std::lock_guard<std::mutex> lock(mutex_);
    event_log_.clear();
}

size_t VoiceAuditLogger::getEventCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(event_log_.size());
}

void VoiceAuditLogger::setEventCallback(std::function<void(const json&)> callback) {
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

void VoiceAuditLogger::writeEvent([[maybe_unused]] const json& event) {
    std::function<void(const json&)> callback;
    Config config_snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        event_log_.push_back([[maybe_unused]] event);
        callback = event_callback_;
        config_snapshot = config_;
    }

    if ([[maybe_unused]] callback) {
        try {
            callback([[maybe_unused]] event);
        } catch (...) {
            // Audit callbacks must never break event capture.
        }
    }

    const std::string serialized = serializeEvent([[maybe_unused]] event);
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

std::string VoiceAuditLogger::serializeEvent([[maybe_unused]] const json& event) const {
    return event.dump();
}

}} // namespace themis::voice
