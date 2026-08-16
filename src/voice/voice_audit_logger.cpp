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

    std::lock_guard<std::mutex> lock(mutex_);

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

void VoiceAuditLogger::logSessionLifecycle(
    const std::string& session_id,
    const std::string& user_id,
    const std::string& event_type,
    int64_t duration_ms,
    size_t bytes_transferred) {
    
    if (!config_.enable_logging) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

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

void VoiceAuditLogger::logLivenessChallenge(
    const std::string& user_id,
    const std::string& challenge_id,
    const std::string& event_type,
    bool passed,
    const std::string& reason) {
    
    if (!config_.enable_logging) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

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

    std::lock_guard<std::mutex> lock(mutex_);

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
    std::lock_guard<std::mutex> lock(mutex_);
    return event_log_;
}

std::vector<json> VoiceAuditLogger::getEventsForUser(const std::string& user_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<json> user_events;
    for (const auto& event : event_log_) {
        if (event.contains("user_id") && event["user_id"].get<std::string>() == user_id) {
            user_events.push_back(event);
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
    return event_log_.size();
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

    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t_now), "%Y-%m-%dT%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    return ss.str();
}

void VoiceAuditLogger::writeEvent(const json& event) {
    // Add to in-memory log
    event_log_.push_back(event);

    // Call callback if registered
    if (event_callback_) {
        event_callback_(event);
    }

    // Write to console if enabled
    if (config_.log_to_console) {
        std::cerr << serializeEvent(event) << std::endl;
    }

    // Write to file if configured
    if (!config_.log_file_path.empty()) {
        std::ofstream log_file(config_.log_file_path, std::ios::app);
        if (log_file.is_open()) {
            log_file << serializeEvent(event) << "\n";
            log_file.close();
        }
    }
}

std::string VoiceAuditLogger::serializeEvent(const json& event) const {
    return event.dump();
}

}} // namespace themis::voice
