/**
 * @file voice_security.h
 * @brief Voice Security & Privacy — Frozen API Contract for Phase 1.
 *
 * @version v1.0 frozen as of 2026-08-08
 *
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Design/API Contract Frozen (Phase 1)
 *
 * ## Security Guarantees (Frozen)
 *
 * **PII Detection & Redaction (Frozen Types):**
 * - PHONE_NUMBER: Pattern-based detection + masking
 * - EMAIL_ADDRESS: RFC 5322 patterns + masking
 * - CREDIT_CARD: Luhn validation + masking
 * - SSN: 9-digit patterns + masking
 * - IP_ADDRESS: IPv4/IPv6 + masking
 * - PERSON_NAME: NER-based detection
 * - MEDICAL_INFO: Regex patterns
 * - CUSTOM: User-defined patterns
 *
 * **Consent Tracking (Frozen Consent Types):**
 * - recording_consent: May record audio
 * - transcription_consent: May transcribe
 * - data_retention_consent: May retain beyond session
 * - analytics_consent: May use for analytics/training
 *
 * **Audit Logging Contract (Frozen Event Types):**
 * Every access, modification, or deletion is logged with:
 * - event_type: (e.g., "session_created", "transcription_generated")
 * - user_id: Actor performing action
 * - action: (create, read, update, delete)
 * - resource: (session_id, profile_id, recording_id)
 * - success: true/false
 * - timestamp_ms: Wall-clock milliseconds
 *
 * **Data Deletion (GDPR/CCPA) — Frozen Semantics:**
 * deleteUserData() triggers:
 * 1. Recording files deleted
 * 2. Transcripts redacted/deleted
 * 3. Sessions terminated
 * 4. Analytics records cleared (optional)
 * Deletion is irreversible.
 *
 * ## Error Codes (Voice Module — Security)
 * - 7010: PII detection/redaction error
 * - 7011: Consent record not found
 * - 7012: Consent revoked
 * - 7013: Audit logging failed
 * - 7014: Data deletion failed
 * - 7015: Privacy policy violation
 * - 7016-7099: Reserved for privacy-related errors
 *
 * ## Thread Safety
 * VoiceSecurityManager is thread-safe (internal mutex).
 * Consent records and audit logs are protected.
 */


// Security, privacy, and compliance for Phase 7 production readiness
// ============================================================================
// PHASE 1 CONTRACT FREEZE: This file documents immutable security & privacy
// contracts including PII detection, consent management, audit logging, and
// GDPR/CCPA-compliant data deletion.
// ============================================================================
#pragma once
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <optional>
#include <functional>
#include <nlohmann/json.hpp>

namespace themis { namespace voice {
using json = nlohmann::json;

// PII types for redaction
enum class PIIType {
    PHONE_NUMBER,
    EMAIL_ADDRESS,
    CREDIT_CARD,
    SSN,              // Social Security Number
    IP_ADDRESS,
    PERSON_NAME,
    MEDICAL_INFO,
    CUSTOM
};

/**
 * @brief TBD: Describe piiTypeToString.
 * @param[in] type Input parameter.
 * @return Return value.
 */
std::string piiTypeToString(PIIType type);

// Redaction result
struct RedactionResult {
    std::string redacted_text;
    std::vector<std::pair<PIIType, std::string>> found_pii;  // type + original value
    int redaction_count = 0;
    bool has_pii = false;
};

// Consent record
struct ConsentRecord {
    std::string user_id;
    std::string session_id;
    bool recording_consent = false;
    bool transcription_consent = false;
    bool data_retention_consent = false;
    bool analytics_consent = false;
    int64_t consent_timestamp_ms = 0;
    std::string consent_version = "1.0";
    json custom_consents;
};

// Audit log entry
struct VoiceAuditEntry {
    std::string event_type;
    std::string session_id;
    std::string user_id;
    std::string action;
    std::string resource;
    int64_t timestamp_ms = 0;
    bool success = true;
    std::string details;
    json metadata;
};

// Data deletion request (GDPR/CCPA)
struct DataDeletionRequest {
    std::string user_id;
    std::string reason;
    bool delete_recordings = true;
    bool delete_transcripts = true;
    bool delete_sessions = true;
    bool delete_analytics = false;
    int64_t request_timestamp_ms = 0;
};

// Data deletion result
struct DataDeletionResult {
    bool success = false;
    std::string error_message;
    size_t recordings_deleted = 0;
    size_t sessions_deleted = 0;
    size_t transcripts_deleted = 0;
    int64_t completion_timestamp_ms = 0;
};

// Phase 3: Rate limiting for authentication failures
struct RateLimiterConfig {
    int max_failures = 5;              // Max failed attempts before lockout
    int64_t lockout_duration_ms = 60000; // 60 seconds
    int64_t failure_window_ms = 600000;  // 10 minutes (reset counter after)
};

// Phase 3: Security denial tracking for audit
struct SecurityDenialEntry {
    int64_t timestamp_ms = 0;
    std::string user_id;
    std::string session_id;
    std::string action = {};
    std::string resource;
    std::string denial_reason;         // e.g., "auth_failed", "access_denied", "privilege_escalation"
    std::string denial_code;           // e.g., "SECURITY_VIOLATION"
};

// Security config
struct VoiceSecurityConfig {
    bool enable_pii_redaction = true;
    bool enable_consent_tracking = true;
    bool enable_audit_logging = true;
    bool enable_auto_deletion = false;
    bool enable_rate_limiting = true;  // Phase 3
    int64_t data_retention_days = 90;
    std::vector<PIIType> pii_types_to_redact = {
        PIIType::PHONE_NUMBER, PIIType::EMAIL_ADDRESS,
        PIIType::CREDIT_CARD, PIIType::SSN
    };
    bool redact_in_transcripts = true;
    bool redact_in_summaries = true;
    RateLimiterConfig rate_limiter;    // Phase 3
};

// VoiceSecurityManager: Phase 7 production component (Phase 3 enhancements)
/** @brief VoiceSecurityManager: Phase 7 production component (Phase 3 enhancements). */
class VoiceSecurityManager {
public:
    explicit VoiceSecurityManager(const VoiceSecurityConfig& config = {});
    ~VoiceSecurityManager() = default;

    /**
     * @brief PII Redaction
     * @param[in] text Input parameter.
     * @return Return value.
     */
    RedactionResult redactPII(const std::string& text);
    /**
     * @brief TBD: Describe redactPIITypes.
     * @param[in] text Input parameter.
     * @param[in] types Input parameter.
     * @return Return value.
     */
    RedactionResult redactPIITypes(const std::string& text, const std::vector<PIIType>& types);
    /**
     * @brief TBD: Describe containsPII.
     * @param[in] text Input parameter.
     * @return True on success.
     */
    bool containsPII(const std::string& text) const;

    /**
     * @brief Consent Management
     * @param[in] record Input parameter.
     * @return True on success.
     */
    bool recordConsent(const ConsentRecord& record);
    /**
     * @brief TBD: Describe getConsent.
     * @param[in] user_id Input parameter.
     * @return Return value.
     */
    std::optional<ConsentRecord> getConsent(const std::string& user_id) const;
    /**
     * @brief TBD: Describe hasRecordingConsent.
     * @param[in] user_id Input parameter.
     * @return True on success.
     */
    bool hasRecordingConsent(const std::string& user_id) const;
    /**
     * @brief TBD: Describe hasTranscriptionConsent.
     * @param[in] user_id Input parameter.
     * @return True on success.
     */
    bool hasTranscriptionConsent(const std::string& user_id) const;
    /**
     * @brief TBD: Describe revokeConsent.
     * @param[in] user_id Input parameter.
     * @return True on success.
     */
    bool revokeConsent(const std::string& user_id);

    /**
     * @brief Audit Logging
     * @param[in] entry Input parameter.
     */
    void logEvent(const VoiceAuditEntry& entry);
    /**
     * @brief TBD: Describe logAccess.
     * @param[in] user_id Input parameter.
     * @param[in] session_id Input parameter.
     * @param[in] resource Input parameter.
     */
    void logAccess(const std::string& user_id, const std::string& session_id, const std::string& resource);
    /**
     * @brief TBD: Describe logError.
     * @param[in] user_id Input parameter.
     * @param[in] session_id Input parameter.
     * @param[in] error Input parameter.
     */
    void logError(const std::string& user_id, const std::string& session_id, const std::string& error);
    std::vector<VoiceAuditEntry> getAuditLog(const std::string& user_id = "", size_t limit = 100) const;

    /**
     * @brief GDPR/CCPA Data Deletion
     * @param[in] request Input parameter.
     * @return Return value.
     */
    DataDeletionResult deleteUserData(const DataDeletionRequest& request);
    /**
     * @brief TBD: Describe scheduleAutoDelete.
     * @param[in] user_id Input parameter.
     * @param[in] delete_after_ms Input parameter.
     * @return True on success.
     */
    bool scheduleAutoDelete(const std::string& user_id, int64_t delete_after_ms);

    /**
     * @brief Data export (GDPR right to access)
     * @param[in] user_id Input parameter.
     * @return Return value.
     */
    json exportUserData(const std::string& user_id) const;

    /**
     * @brief Security stats
     * @return Return value.
     */
    json getSecurityStats() const;
    
    // Phase 3: Rate Limiting for Auth Failures
    
    /**
     * @brief @brief Record authentication failure (Phase 3) @param user_id User identifier @return true if user is NOT locked out; false if reached lockout threshold
     * @param[in] user_id Input parameter.
     * @return True on success.
     */
    bool recordAuthFailure(const std::string& user_id);
    
    /**
     * @brief @brief Check if user is locked out due to rate limiting (Phase 3) @param user_id User identifier @return true if user is currently locked out; false otherwise
     * @param[in] user_id Input parameter.
     * @return True on success.
     */
    bool isRateLimited(const std::string& user_id) const;
    
    /**
     * @brief @brief Reset rate limiter for a user (Phase 3) @param user_id User identifier
     * @param[in] user_id Input parameter.
     */
    void resetRateLimiter(const std::string& user_id);
    
    // Phase 3: Security Denial Audit Trail
    
    /**
     * @brief @brief Log security denial for audit trail (Phase 3) @param entry SecurityDenialEntry with user, action, reason
     * @param[in] entry Input parameter.
     */
    void logSecurityDenial(const SecurityDenialEntry& entry);
    
    /// @brief Get all security denials for a user (Phase 3)
    /// @param user_id User identifier
    /// @return Vector of SecurityDenialEntry
    std::vector<SecurityDenialEntry> getSecurityDenials(const std::string& user_id, size_t limit = 100) const;
    
    /**
     * @brief @brief Deny operation with full audit context (Phase 3) @param user_id User identifier @param session_id Session identifier @param action Action being attempted (e.
     * @param[in] user_id Input parameter.
     * @param[in] session_id Input parameter.
     * @param[in] action Input parameter.
     * @param[in] resource Input parameter.
     * @param[in] reason Input parameter.
     * @return True on success.
     * @details g., "escalate_privileges") @param resource Resource being accessed @param reason Denial reason (e.g., "privilege_escalation_attempt") @return Always false (denial)
     */
    bool denyOperationWithAudit(const std::string& user_id,
                                const std::string& session_id,
                                const std::string& action,
                                const std::string& resource,
                                const std::string& reason);

private:
    VoiceSecurityConfig config_;
    mutable std::mutex mutex_;

    std::map<std::string, ConsentRecord> consents_;
    std::vector<VoiceAuditEntry> audit_log_;
    std::map<std::string, int64_t> auto_delete_schedule_;
    
    // Phase 3: Rate limiting
    std::map<std::string, int> failure_counts_;         // user_id -> failure count
    std::map<std::string, int64_t> last_failure_times_; // user_id -> last failure timestamp
    std::map<std::string, int64_t> lockout_until_ms_;   // user_id -> lockout expiry time
    
    // Phase 3: Security denial trail
    std::vector<SecurityDenialEntry> denial_trail_;

    /**
     * @brief TBD: Describe applyPattern.
     * @param[in] text Input parameter.
     * @param[in] type Input parameter.
     * @return Return value.
     */
    RedactionResult applyPattern(const std::string& text, PIIType type) const;
    /**
     * @brief TBD: Describe maskValue.
     * @param[in] value Input parameter.
     * @param[in] type Input parameter.
     * @return Return value.
     */
    std::string maskValue(const std::string& value, PIIType type) const;
    
    /**
     * @brief Phase 3: Rate limiting helpers
     * @return Return value.
     */
    int64_t nowMs() const;
    /**
     * @brief TBD: Describe cleanupExpiredLockouts.
     */
    void cleanupExpiredLockouts();
};

}} // namespace themis::voice
