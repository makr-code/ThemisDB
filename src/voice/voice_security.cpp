/**
 * @file voice_security.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.42
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 96/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "voice/voice_security.h"
#include <regex>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <spdlog/spdlog.h>

namespace themis { namespace voice {

// ---- Free functions ----

std::string piiTypeToString(PIIType type) {
    switch (type) {
        case PIIType::PHONE_NUMBER:  return "PHONE_NUMBER";
        case PIIType::EMAIL_ADDRESS: return "EMAIL_ADDRESS";
        case PIIType::CREDIT_CARD:   return "CREDIT_CARD";
        case PIIType::SSN:           return "SSN";
        case PIIType::IP_ADDRESS:    return "IP_ADDRESS";
        case PIIType::PERSON_NAME:   return "PERSON_NAME";
        case PIIType::MEDICAL_INFO:  return "MEDICAL_INFO";
        case PIIType::CUSTOM:        return "CUSTOM";
        default:                     return "UNKNOWN";
    }
}

// ---- VoiceSecurityManager ----

VoiceSecurityManager::VoiceSecurityManager(const VoiceSecurityConfig& config)
    : config_(config) {}

std::string VoiceSecurityManager::maskValue(const std::string& /*value*/, PIIType type) const {
    switch (type) {
        case PIIType::PHONE_NUMBER:  return "[PHONE_REDACTED]";
        case PIIType::EMAIL_ADDRESS: return "[EMAIL_REDACTED]";
        case PIIType::CREDIT_CARD:   return "[CC_REDACTED]";
        case PIIType::SSN:           return "[SSN_REDACTED]";
        case PIIType::IP_ADDRESS:    return "[IP_REDACTED]";
        case PIIType::PERSON_NAME:   return "[NAME_REDACTED]";
        case PIIType::MEDICAL_INFO:  return "[MEDICAL_REDACTED]";
        default:                     return "[REDACTED]";
    }
}

RedactionResult VoiceSecurityManager::applyPattern(const std::string& text, PIIType type) const {
    RedactionResult result;
    result.redacted_text = text;

    struct PatternDef {
        PIIType type;
        std::string regex_str;
    };

    static const PatternDef patterns[] = {
        // Phone: +1-234-567-8901, 1234567890, etc.
        {PIIType::PHONE_NUMBER,  R"(\+?[\d][\d\s\-\.\(\)]{8,14}\d)"},
        // Email
        {PIIType::EMAIL_ADDRESS, R"([A-Za-z0-9._%+\-]+@[A-Za-z0-9.\-]+\.[A-Za-z]{2,})"},
        // Credit card: 16 consecutive digits (possibly separated by spaces/dashes)
        {PIIType::CREDIT_CARD,   R"(\b(?:\d[ \-]?){15}\d\b)"},
        // SSN: 123-45-6789
        {PIIType::SSN,           R"(\b\d{3}-\d{2}-\d{4}\b)"},
        // IPv4
        {PIIType::IP_ADDRESS,    R"(\b(?:\d{1,3}\.){3}\d{1,3}\b)"},
    };

    for (const auto& p : patterns) {
        if (p.type != type) continue;
        try {
            std::regex re(p.regex_str);
            std::string output;
            std::sregex_iterator it(result.redacted_text.begin(), result.redacted_text.end(), re);
            std::sregex_iterator end;
            std::string::const_iterator last_pos = result.redacted_text.cbegin();

            for (; it != end; ++it) {
                output.append(last_pos, result.redacted_text.cbegin() + it->position());
                std::string matched = it->str();
                output += maskValue(matched, type);
                result.found_pii.emplace_back(type, matched);
                ++result.redaction_count;
                result.has_pii = true;
                last_pos = result.redacted_text.cbegin() + it->position() + it->length();
            }
            output.append(last_pos, result.redacted_text.cend());
            result.redacted_text = output;
        } catch (const std::regex_error&) {
            // Pattern error; skip this type
        }
        break;
    }
    return result;
}

RedactionResult VoiceSecurityManager::redactPIITypes(
    const std::string& text, const std::vector<PIIType>& types)
{
    RedactionResult cumulative;
    cumulative.redacted_text = text;
    cumulative.has_pii = false;
    cumulative.redaction_count = 0;

    for (PIIType t : types) {
        auto partial = applyPattern(cumulative.redacted_text, t);
        cumulative.redacted_text = partial.redacted_text;
        cumulative.redaction_count += partial.redaction_count;
        for (auto& p : partial.found_pii) {
            cumulative.found_pii.push_back(p);
        }
        if (partial.has_pii) cumulative.has_pii = true;
    }
    return cumulative;
}

RedactionResult VoiceSecurityManager::redactPII(const std::string& text) {
    return redactPIITypes(text, config_.pii_types_to_redact);
}

bool VoiceSecurityManager::containsPII(const std::string& text) const {
    // Create a temporary manager to check without modifying state
    VoiceSecurityManager tmp(config_);
    auto result = tmp.redactPIITypes(text, config_.pii_types_to_redact);
    return result.has_pii;
}

// ---- Consent Management ----

bool VoiceSecurityManager::recordConsent(const ConsentRecord& record) {
    std::lock_guard<std::mutex> lock(mutex_);
    consents_[record.user_id] = record;
    return true;
}

std::optional<ConsentRecord> VoiceSecurityManager::getConsent(const std::string& user_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = consents_.find(user_id);
    if (it == consents_.end()) return std::nullopt;
    return it->second;
}

bool VoiceSecurityManager::hasRecordingConsent(const std::string& user_id) const {
    auto consent = getConsent(user_id);
    return consent.has_value() && consent->recording_consent;
}

bool VoiceSecurityManager::hasTranscriptionConsent(const std::string& user_id) const {
    auto consent = getConsent(user_id);
    return consent.has_value() && consent->transcription_consent;
}

bool VoiceSecurityManager::revokeConsent(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return consents_.erase(user_id) > 0;
}

// ---- Audit Logging ----

void VoiceSecurityManager::logEvent(const VoiceAuditEntry& entry) {
    if (!config_.enable_audit_logging) return;
    std::lock_guard<std::mutex> lock(mutex_);
    audit_log_.push_back(entry);
}

void VoiceSecurityManager::logAccess(
    const std::string& user_id, const std::string& session_id, const std::string& resource)
{
    VoiceAuditEntry entry;
    entry.event_type  = "ACCESS";
    entry.user_id     = user_id;
    entry.session_id  = session_id;
    entry.resource    = resource;
    entry.action      = "READ";
    entry.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    entry.success     = true;
    logEvent(entry);
}

void VoiceSecurityManager::logError(
    const std::string& user_id, const std::string& session_id, const std::string& error)
{
    VoiceAuditEntry entry;
    entry.event_type  = "ERROR";
    entry.user_id     = user_id;
    entry.session_id  = session_id;
    entry.action      = "ERROR";
    entry.details     = error;
    entry.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    entry.success     = false;
    logEvent(entry);
}

std::vector<VoiceAuditEntry> VoiceSecurityManager::getAuditLog(
    const std::string& user_id, size_t limit) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<VoiceAuditEntry> result;
    for (auto it = audit_log_.rbegin(); it != audit_log_.rend(); ++it) {
        if (!user_id.empty() && it->user_id != user_id) continue;
        result.push_back(*it);
        if (result.size() >= limit) break;
    }
    return result;
}

// ---- GDPR / CCPA ----

DataDeletionResult VoiceSecurityManager::deleteUserData(const DataDeletionRequest& request) {
    DataDeletionResult result;
    result.success = true;

    std::lock_guard<std::mutex> lock(mutex_);

    if (request.delete_sessions) {
        result.sessions_deleted = consents_.erase(request.user_id);
    }

    if (request.delete_transcripts || request.delete_recordings) {
        size_t removed = 0;
        audit_log_.erase(
            std::remove_if(audit_log_.begin(), audit_log_.end(),
                [&]([[maybe_unused]] const VoiceAuditEntry& e) {
                    if (e.user_id == request.user_id) { ++removed; return true; }
                    return false;
                }),
            audit_log_.end());
        result.transcripts_deleted = removed;
        result.recordings_deleted  = removed;
    }

    result.completion_timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return result;
}

bool VoiceSecurityManager::scheduleAutoDelete(const std::string& user_id, int64_t delete_after_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    auto_delete_schedule_[user_id] = now + delete_after_ms;
    return true;
}

json VoiceSecurityManager::exportUserData(const std::string& user_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    json data;
    data["user_id"] = user_id;

    auto it = consents_.find(user_id);
    if (it != consents_.end()) {
        data["consent"]["recording"]    = it->second.recording_consent;
        data["consent"]["transcription"]= it->second.transcription_consent;
        data["consent"]["version"]      = it->second.consent_version;
    }

    json audit_entries = json::array();
    for (const auto& entry : audit_log_) {
        if (entry.user_id != user_id) continue;
        json e;
        e["event_type"]   = entry.event_type;
        e["action"]       = entry.action;
        e["resource"]     = entry.resource;
        e["timestamp_ms"] = entry.timestamp_ms;
        audit_entries.push_back(e);
    }
    data["audit_log"] = audit_entries;
    return data;
}

json VoiceSecurityManager::getSecurityStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    json stats;
    stats["total_consents"]    = consents_.size();
    stats["total_audit_events"]= audit_log_.size();
    stats["auto_delete_scheduled"] = auto_delete_schedule_.size();
    return stats;
}

// ============================================================================
// Phase 3: Rate Limiting and Security Denial Audit Trail
// ============================================================================

int64_t VoiceSecurityManager::nowMs() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

void VoiceSecurityManager::cleanupExpiredLockouts() {
    if (!config_.enable_rate_limiting) return;
    
    int64_t now = nowMs();
    std::vector<std::string> expired_users;
    
    for (const auto& [user_id, lockout_time] : lockout_until_ms_) {
        if (now >= lockout_time) {
            expired_users.push_back(user_id);
        }
    }
    
    for (const auto& user_id : expired_users) {
        lockout_until_ms_.erase(user_id);
        failure_counts_[user_id] = 0;
    }
}

bool VoiceSecurityManager::recordAuthFailure(const std::string& user_id) {
    if (!config_.enable_rate_limiting) return true;
    
    std::lock_guard<std::mutex> lock(mutex_);
    cleanupExpiredLockouts();
    
    // Check if user is already locked out
    auto lockout_it = lockout_until_ms_.find(user_id);
    if (lockout_it != lockout_until_ms_.end()) {
        int64_t now = nowMs();
        if (now < lockout_it->second) {
            // Still locked out
            return false;
        }
    }
    
    int64_t now = nowMs();
    auto last_failure_it = last_failure_times_.find(user_id);
    
    // Check if failure window has expired
    if (last_failure_it != last_failure_times_.end()) {
        if (now - last_failure_it->second > config_.rate_limiter.failure_window_ms) {
            // Window expired, reset counter
            failure_counts_[user_id] = 0;
        }
    }
    
    // Increment failure count
    failure_counts_[user_id]++;
    last_failure_times_[user_id] = now;
    
    // Check if threshold exceeded
    if (failure_counts_[user_id] >= config_.rate_limiter.max_failures) {
        // Lockout user
        lockout_until_ms_[user_id] = now + config_.rate_limiter.lockout_duration_ms;
        spdlog::warn("User {} locked out due to {} auth failures", 
            user_id, failure_counts_[user_id]);
        return false;
    }
    
    return true;
}

bool VoiceSecurityManager::isRateLimited(const std::string& user_id) const {
    if (!config_.enable_rate_limiting) return false;
    
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = lockout_until_ms_.find(user_id);
    if (it == lockout_until_ms_.end()) return false;
    
    int64_t now = nowMs();
    return now < it->second;
}

void VoiceSecurityManager::resetRateLimiter(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    failure_counts_.erase(user_id);
    last_failure_times_.erase(user_id);
    lockout_until_ms_.erase(user_id);
}

void VoiceSecurityManager::logSecurityDenial(const SecurityDenialEntry& entry) {
    if (!config_.enable_audit_logging) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    denial_trail_.push_back(entry);
}

std::vector<SecurityDenialEntry> VoiceSecurityManager::getSecurityDenials(
    const std::string& user_id, size_t limit) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<SecurityDenialEntry> result;
    
    size_t count = 0;
    for (auto it = denial_trail_.rbegin(); it != denial_trail_.rend() && count < limit; ++it) {
        if (it->user_id == user_id || user_id.empty()) {
            result.push_back(*it);
            ++count;
        }
    }
    
    return result;
}

bool VoiceSecurityManager::denyOperationWithAudit(
    const std::string& user_id,
    const std::string& session_id,
    const std::string& action,
    const std::string& resource,
    const std::string& reason)
{
    // Phase 3.5: Deny operation with full audit context (always returns false)
    SecurityDenialEntry entry;
    entry.timestamp_ms = nowMs();
    entry.user_id = user_id;
    entry.session_id = session_id;
    entry.action = action;
    entry.resource = resource;
    entry.denial_reason = reason;
    entry.denial_code = "SECURITY_VIOLATION";
    
    logSecurityDenial(entry);
    
    // Log to spdlog for observability
    spdlog::warn("Security denial: user={}, action={}, resource={}, reason={}", 
        user_id, action, resource, reason);
    
    return false;  // Always deny
}

}} // namespace themis::voice
