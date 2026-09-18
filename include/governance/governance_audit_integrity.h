/**
 * @file governance_audit_integrity.h
 * @brief Audit trail cryptographic signing, integrity verification, and chain-of-custody
 * @version 0.0.48
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Critical Path 3: Audit Trail Integrity & Immutability
 * 
 * This module provides:
 * - Cryptographic signing of audit entries (SHA-256 with RSA/HMAC)
 * - Chain-of-custody: each entry includes hash of previous entry
 * - Key rotation support for signature verification
 * - Tamper detection: verify signatures and detect changes
 * - Integrity verification across time ranges
 * - Configurable retention policies with archival and legal hold
 * 
 * Latency targets:
 * - Signing: ≤1ms per entry
 * - Verification: ≤10ms per entry
 * - Tamper detection accuracy: >99%
 */

#pragma once

#ifndef THEMIS_GOVERNANCE_AUDIT_INTEGRITY_H
#define THEMIS_GOVERNANCE_AUDIT_INTEGRITY_H

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <nlohmann/json.hpp>

namespace themis {
namespace governance {

// ============================================================================
// Audit Entry with Integrity Information
// ============================================================================

struct SignatureInfo {
    std::string signature;                    // Base64-encoded signature
    std::string algorithm;                    // "HMAC-SHA256", "RSA-SHA256", etc.
    int64_t signed_at_ms = 0;                // Timestamp when signed
    std::string key_id;                       // ID of signing key (for rotation)
    std::string previous_entry_hash;          // Chain-of-custody: hash of previous entry
    std::string entry_hash;                   // SHA-256 hash of this entry content
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static SignatureInfo fromJson(const nlohmann::json& j);
};

struct ImmutableAuditEntry {
    // Core audit information
    std::string entry_id;                     // Unique entry identifier
    std::string rule_id;                      // Rule being operated on
    std::string operation;                    // "create", "update", "delete", "rollback", "verify"
    std::string user;                         // User performing the operation
    int64_t timestamp_ms = 0;                // When operation occurred
    nlohmann::json details;                   // Operation details
    
    // Integrity information
    SignatureInfo signature_info;             // Cryptographic signature details
    int64_t entry_sequence_number = 0;       // Sequential entry number (for ordering)
    bool is_archived = false;                 // Whether archived
    int64_t archive_timestamp_ms = 0;        // When archived
    std::string archive_hash;                 // Hash of archive (if archived)
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static ImmutableAuditEntry fromJson(const nlohmann::json& j);
    
    /**
     * @brief Verify Integrity.
     * @return True when the operation succeeds.
     */
    bool verifyIntegrity() const;
};

// ============================================================================
// Audit Signer - Cryptographic Signing and Verification
// ============================================================================

class AuditSigner {
public:
    enum class SignatureAlgorithm {
        HMAC_SHA256,      // Fast, symmetric (use for performance)
        RSA_SHA256        // Asymmetric (use for compliance/non-repudiation)
    };
    
    AuditSigner(
        SignatureAlgorithm algorithm,
        const std::string& key_id,
        const std::string& secret_key
    );
    
    SignatureInfo signEntry(
        const ImmutableAuditEntry& entry,
        const std::string& previous_entry_hash = ""
    );
    
    /**
     * @brief Verify Signature.
     * @param[in] entry Input parameter.
     * @param[in] signature_info Input parameter.
     * @return True when the operation succeeds.
     */
    bool verifySignature(
        const ImmutableAuditEntry& entry,
        const SignatureInfo& signature_info
    ) const;
    
    const std::string& getKeyId() const { return key_id_; }
    
    /**
     * @brief Get Algorithm Name.
     * @return Return value.
     */
    std::string getAlgorithmName() const;
    
private:
    SignatureAlgorithm algorithm_;
    std::string key_id_;
    std::string secret_key_;
    
    /**
     * @brief Compute Sha256 Hash.
     * @param[in] content Input parameter.
     * @return Return value.
     */
    std::string computeSha256Hash(const std::string& content) const;
    
    /**
     * @brief Compute Hmac Sha256.
     * @param[in] content Input parameter.
     * @return Return value.
     */
    std::string computeHmacSha256(const std::string& content) const;
    
    /**
     * @brief Compute Rsa Sha256.
     * @param[in] content Input parameter.
     * @return Return value.
     */
    std::string computeRsaSha256(const std::string& content) const;
    
    /**
     * @brief Verify Hmac Sha256.
     * @param[in] content Input parameter.
     * @param[in] signature Input parameter.
     * @return True when the operation succeeds.
     */
    bool verifyHmacSha256(
        const std::string& content,
        const std::string& signature
    ) const;
    
    /**
     * @brief Verify Rsa Sha256.
     * @param[in] content Input parameter.
     * @param[in] signature Input parameter.
     * @return True when the operation succeeds.
     */
    bool verifyRsaSha256(
        const std::string& content,
        const std::string& signature
    ) const;
};

// ============================================================================
// Audit Tamper Detector - Detect Alterations and Integrity Violations
// ============================================================================

struct TamperIncident {
    enum class TamperType {
        INVALID_SIGNATURE,      // Entry signature doesn't match
        BROKEN_CHAIN,           // Previous entry hash doesn't match
        MISSING_ENTRY,          // Gap in sequence numbers
        REORDERED_ENTRY,        // Entries out of order
        ALTERED_ENTRY,          // Entry content changed (hash mismatch)
        CLOCK_SKEW,             // Impossible timestamps (timestamp < previous)
        KEY_ROTATION_ERROR      // Key rotation verification failed
    };
    
    std::string incident_id;                  // Unique incident identifier
    TamperType type;                          // Type of tampering detected
    int64_t detected_at_ms = 0;              // When tampering was detected
    int64_t tamper_entry_sequence = 0;       // Sequence number of tampered entry
    std::string tamper_entry_id;              // ID of tampered entry
    std::string evidence;                     // Detailed evidence
    int64_t affected_entry_count = 0;        // Number of affected entries
    bool is_critical = false;                 // True if core audit trail corrupted
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static TamperIncident fromJson(const nlohmann::json& j);
};

class AuditTamperDetector {
public:
    std::optional<TamperIncident> verifyEntry(
        const ImmutableAuditEntry& entry,
        const AuditSigner& signer,
        const std::optional<ImmutableAuditEntry>& previous_entry = std::nullopt
    );
    
    /**
     * @brief Verify Audit Trail.
     * @param[in] entries Input parameter.
     * @param[in] signer Input parameter.
     * @return Return value.
     */
    std::vector<TamperIncident> verifyAuditTrail(
        const std::vector<ImmutableAuditEntry>& entries,
        const AuditSigner& signer
    );
    
    /**
     * @brief Verify Time Range.
     * @param[in] entries Input parameter.
     * @param[in] signer Input parameter.
     * @param[in] start_time_ms Input parameter.
     * @param[in] end_time_ms Input parameter.
     * @return Return value.
     */
    std::vector<TamperIncident> verifyTimeRange(
        const std::vector<ImmutableAuditEntry>& entries,
        const AuditSigner& signer,
        int64_t start_time_ms,
        int64_t end_time_ms
    );
    
    /**
     * @brief Generate Tamper Report.
     * @param[in] incidents Input parameter.
     * @return Return value.
     */
    static nlohmann::json generateTamperReport(
        const std::vector<TamperIncident>& incidents
    );
    
private:
    /**
     * @brief Check Signature Validity.
     * @param[in] entry Input parameter.
     * @param[in] signer Input parameter.
     * @return Return value.
     */
    std::optional<TamperIncident> checkSignatureValidity(
        const ImmutableAuditEntry& entry,
        const AuditSigner& signer
    );
    
    /**
     * @brief Check Chain Of Custody.
     * @param[in] entry Input parameter.
     * @param[in] previous_entry Input parameter.
     * @return Return value.
     */
    std::optional<TamperIncident> checkChainOfCustody(
        const ImmutableAuditEntry& entry,
        const ImmutableAuditEntry& previous_entry
    );
    
    /**
     * @brief Check Sequence Validity.
     * @param[in] entry Input parameter.
     * @param[in] previous_entry Input parameter.
     * @return Return value.
     */
    std::optional<TamperIncident> checkSequenceValidity(
        const ImmutableAuditEntry& entry,
        const ImmutableAuditEntry& previous_entry
    );
    
    /**
     * @brief Check Timestamp Validity.
     * @param[in] entry Input parameter.
     * @param[in] previous_entry Input parameter.
     * @return Return value.
     */
    std::optional<TamperIncident> checkTimestampValidity(
        const ImmutableAuditEntry& entry,
        const ImmutableAuditEntry& previous_entry
    );
};

// ============================================================================
// Audit Retention Policy - Enforce Retention and Archival Rules
// ============================================================================

struct AuditRetentionPolicy {
    std::string policy_id;                    // Unique policy identifier
    int64_t retention_period_days = 2555;    // Default: 7 years (2555 days)
    int64_t archive_after_days = 365;        // Archive after 1 year
    bool enable_legal_hold = true;            // Support legal hold override
    bool compress_on_archive = true;          // Compress archived entries
    std::string archive_destination;          // Where to store archived entries
    int64_t created_at_ms = 0;               // Policy creation time
    int64_t modified_at_ms = 0;              // Last modification time
    nlohmann::json metadata;                  // Additional metadata
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static AuditRetentionPolicy fromJson(const nlohmann::json& j);
};

struct LegalHold {
    std::string hold_id;                      // Unique hold identifier
    std::string rule_id;                      // Rule ID (optional, for targeted holds)
    std::string initiated_by;                 // Who initiated the hold
    int64_t initiated_at_ms = 0;             // When hold was initiated
    int64_t expire_at_ms = 0;                // When hold expires (0 = indefinite)
    std::string reason;                       // Reason for legal hold
    std::string status;                       // "active", "released", "expired"
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static LegalHold fromJson(const nlohmann::json& j);
};

class AuditRetentionManager {
public:
    /**
     * @brief Audit Retention Manager.
     * @param[in] default_policy Input parameter.
     * @return Return value.
     */
    explicit AuditRetentionManager(const AuditRetentionPolicy& default_policy);
    
    /**
     * @brief Check whether an entity has reached the archive threshold.
     * @param[in] entry Input parameter.
     * @param[in] current_time_ms Input parameter.
     * @return True when the entity should be archived.
     */
    bool shouldArchive(
        const ImmutableAuditEntry& entry,
        int64_t current_time_ms
    ) const;
    
    /**
     * @brief Should Delete.
     * @param[in] entry Input parameter.
     * @param[in] current_time_ms Input parameter.
     * @return True when the operation succeeds.
     */
    bool shouldDelete(
        const ImmutableAuditEntry& entry,
        int64_t current_time_ms
    ) const;
    
    /**
     * @brief Is On Legal Hold.
     * @param[in] rule_id Identifier of the rule.
     * @return True when the operation succeeds.
     */
    bool isOnLegalHold(const std::string& rule_id) const;
    
    /**
     * @brief Add Legal Hold.
     * @param[in] hold Input parameter.
     */
    void addLegalHold(const LegalHold& hold);
    
    /**
     * @brief Release Legal Hold.
     * @param[in] hold_id Identifier of the hold.
     */
    void releaseLegalHold(const std::string& hold_id);
    
    const AuditRetentionPolicy& getPolicy() const { return policy_; }
    
    /**
     * @brief Set Policy.
     * @param[in] new_policy Input parameter.
     * @param[in] changed_by Input parameter.
     */
    void setPolicy(
        const AuditRetentionPolicy& new_policy,
        const std::string& changed_by
    );
    
    const std::vector<std::pair<int64_t, AuditRetentionPolicy>>& getPolicyHistory() const {
        return policy_history_;
    }
    
private:
    mutable std::mutex mutex_;
    AuditRetentionPolicy policy_;
    std::vector<std::pair<int64_t, AuditRetentionPolicy>> policy_history_;
    std::unordered_map<std::string, LegalHold> legal_holds_;
};

// ============================================================================
// Integrated Audit Integrity Manager
// ============================================================================

class AuditIntegrityManager {
public:
    AuditIntegrityManager(
        const AuditRetentionPolicy& retention_policy,
        const std::shared_ptr<AuditSigner>& signer
    );
    
    /**
     * @brief Add Entry.
     * @param[in] entry Input parameter.
     * @return Return value.
     */
    ImmutableAuditEntry addEntry(const ImmutableAuditEntry& entry);
    
    /**
     * @brief Verify Integrity.
     * @return Return value.
     */
    std::vector<TamperIncident> verifyIntegrity();
    
    /**
     * @brief Verify Time Range.
     * @param[in] start_time_ms Input parameter.
     * @param[in] end_time_ms Input parameter.
     * @return Return value.
     */
    std::vector<TamperIncident> verifyTimeRange(
        int64_t start_time_ms,
        int64_t end_time_ms
    );
    
    /**
     * @brief Get Entry.
     * @param[in] entry_id Identifier of the entry.
     * @return Return value.
     */
    std::optional<ImmutableAuditEntry> getEntry(const std::string& entry_id) const;
    
    std::vector<ImmutableAuditEntry> queryEntries(
        const std::optional<std::string>& rule_id = std::nullopt,
        const std::optional<std::string>& user = std::nullopt,
        const std::optional<int64_t>& start_time_ms = std::nullopt,
        const std::optional<int64_t>& end_time_ms = std::nullopt
    ) const;
    
    /**
     * @brief Archive Expired Entries.
     * @return Return value.
     */
    int64_t archiveExpiredEntries();
    
    /**
     * @brief Perform Cleanup.
     * @return Return value.
     */
    int64_t performCleanup();
    
    const std::vector<TamperIncident>& getLastTamperIncidents() const {
        return last_tamper_incidents_;
    }
    
    /**
     * @brief Get Performance Metrics.
     * @return Return value.
     */
    nlohmann::json getPerformanceMetrics() const;
    
    nlohmann::json exportAuditTrail(bool compress = false) const;
    
    /**
     * @brief Import Audit Trail.
     * @param[in] data Input parameter.
     * @return True when the operation succeeds.
     */
    bool importAuditTrail(const nlohmann::json& data);
    
    /**
     * @brief Rotate Key.
     * @param[in] new_signer Input parameter.
     * @param[in] key_transition_entry Input parameter.
     */
    void rotateKey(
        const std::shared_ptr<AuditSigner>& new_signer,
        const ImmutableAuditEntry& key_transition_entry
    );
    
    const std::vector<std::shared_ptr<AuditSigner>>& getKeyHistory() const {
        return key_history_;
    }
    
private:
    mutable std::mutex mutex_;
    
    std::shared_ptr<AuditSigner> current_signer_;
    std::vector<std::shared_ptr<AuditSigner>> key_history_;
    std::vector<ImmutableAuditEntry> entries_;
    std::unique_ptr<AuditTamperDetector> tamper_detector_;
    std::unique_ptr<AuditRetentionManager> retention_manager_;
    
    std::vector<TamperIncident> last_tamper_incidents_;
    
    // Performance metrics
    struct PerformanceMetrics {
        std::vector<int64_t> signing_times_us;   // Microseconds
        std::vector<int64_t> verification_times_us;
        int64_t total_entries = 0;
        int64_t total_tamper_checks = 0;
    } metrics_;
    
    /**
     * @brief Get Next Sequence Number.
     * @return Return value.
     */
    int64_t getNextSequenceNumber() const;
    
    /**
     * @brief Get Previous Entry Hash.
     * @return Return value.
     */
    std::string getPreviousEntryHash() const;
};

} // namespace governance
} // namespace themis

#endif // THEMIS_GOVERNANCE_AUDIT_INTEGRITY_H
