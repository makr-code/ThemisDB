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

/**
 * @struct SignatureInfo
 * @brief Cryptographic signature information for an audit entry
 */
struct SignatureInfo {
    std::string signature;                    // Base64-encoded signature
    std::string algorithm;                    // "HMAC-SHA256", "RSA-SHA256", etc.
    int64_t signed_at_ms = 0;                // Timestamp when signed
    std::string key_id;                       // ID of signing key (for rotation)
    std::string previous_entry_hash;          // Chain-of-custody: hash of previous entry
    std::string entry_hash;                   // SHA-256 hash of this entry content
    
    nlohmann::json toJson() const;
    static SignatureInfo fromJson(const nlohmann::json& j);
};

/**
 * @struct ImmutableAuditEntry
 * @brief Audit entry with cryptographic integrity guarantees
 */
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
    
    nlohmann::json toJson() const;
    static ImmutableAuditEntry fromJson(const nlohmann::json& j);
    
    /**
     * @brief Verify integrity of this entry
     * @return true if signature and hash are valid
     */
    bool verifyIntegrity() const;
};

// ============================================================================
// Audit Signer - Cryptographic Signing and Verification
// ============================================================================

/**
 * @class AuditSigner
 * @brief Handles cryptographic signing and verification of audit entries
 * 
 * Supports:
 * - HMAC-SHA256: Fast, symmetric signing
 * - RSA-SHA256: Asymmetric signing for legal/regulatory compliance
 * - Key rotation: Verify entries signed with previous keys
 */
class AuditSigner {
public:
    enum class SignatureAlgorithm {
        HMAC_SHA256,      // Fast, symmetric (use for performance)
        RSA_SHA256        // Asymmetric (use for compliance/non-repudiation)
    };
    
    /**
     * @brief Create a signer with given algorithm and key
     * @param algorithm Signature algorithm to use
     * @param key_id Unique identifier for this key
     * @param secret_key Secret key (for HMAC) or private key (for RSA)
     */
    AuditSigner(
        SignatureAlgorithm algorithm,
        const std::string& key_id,
        const std::string& secret_key
    );
    
    /**
     * @brief Sign an audit entry
     * @param entry Audit entry to sign
     * @param previous_entry_hash Hash of previous entry (for chain-of-custody)
     * @return Populated SignatureInfo
     */
    SignatureInfo signEntry(
        const ImmutableAuditEntry& entry,
        const std::string& previous_entry_hash = ""
    );
    
    /**
     * @brief Verify a signature
     * @param entry Audit entry
     * @param signature_info Signature to verify
     * @return true if signature is valid
     */
    bool verifySignature(
        const ImmutableAuditEntry& entry,
        const SignatureInfo& signature_info
    ) const;
    
    /**
     * @brief Get the key ID
     */
    const std::string& getKeyId() const { return key_id_; }
    
    /**
     * @brief Get the algorithm name
     */
    std::string getAlgorithmName() const;
    
private:
    SignatureAlgorithm algorithm_;
    std::string key_id_;
    std::string secret_key_;
    
    /**
     * @brief Compute SHA-256 hash of content
     */
    std::string computeSha256Hash(const std::string& content) const;
    
    /**
     * @brief Compute HMAC-SHA256
     */
    std::string computeHmacSha256(const std::string& content) const;
    
    /**
     * @brief Compute RSA-SHA256 signature
     */
    std::string computeRsaSha256(const std::string& content) const;
    
    /**
     * @brief Verify HMAC-SHA256 signature
     */
    bool verifyHmacSha256(
        const std::string& content,
        const std::string& signature
    ) const;
    
    /**
     * @brief Verify RSA-SHA256 signature
     */
    bool verifyRsaSha256(
        const std::string& content,
        const std::string& signature
    ) const;
};

// ============================================================================
// Audit Tamper Detector - Detect Alterations and Integrity Violations
// ============================================================================

/**
 * @struct TamperIncident
 * @brief Evidence of audit trail tampering
 */
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
    
    nlohmann::json toJson() const;
    static TamperIncident fromJson(const nlohmann::json& j);
};

/**
 * @class AuditTamperDetector
 * @brief Detects tampering in audit trails
 * 
 * Performs verification of:
 * - Cryptographic signatures
 * - Chain-of-custody (previous entry hashes)
 * - Sequence ordering
 * - Entry completeness
 * - Timestamp consistency
 */
class AuditTamperDetector {
public:
    /**
     * @brief Verify integrity of a single entry
     * @param entry Entry to verify
     * @param signer Signer for verification
     * @param previous_entry Previous entry (for chain validation)
     * @return Empty if valid, TamperIncident if tampering detected
     */
    std::optional<TamperIncident> verifyEntry(
        const ImmutableAuditEntry& entry,
        const AuditSigner& signer,
        const std::optional<ImmutableAuditEntry>& previous_entry = std::nullopt
    );
    
    /**
     * @brief Verify integrity of entire audit trail
     * @param entries Audit entries to verify
     * @param signer Signer for verification
     * @return Vector of TamperIncident objects (empty if no tampering detected)
     */
    std::vector<TamperIncident> verifyAuditTrail(
        const std::vector<ImmutableAuditEntry>& entries,
        const AuditSigner& signer
    );
    
    /**
     * @brief Verify integrity across a time range
     * @param entries All entries
     * @param signer Signer for verification
     * @param start_time_ms Start of time range
     * @param end_time_ms End of time range
     * @return Vector of TamperIncident objects
     */
    std::vector<TamperIncident> verifyTimeRange(
        const std::vector<ImmutableAuditEntry>& entries,
        const AuditSigner& signer,
        int64_t start_time_ms,
        int64_t end_time_ms
    );
    
    /**
     * @brief Generate tamper report
     * @param incidents All detected incidents
     * @return JSON report
     */
    static nlohmann::json generateTamperReport(
        const std::vector<TamperIncident>& incidents
    );
    
private:
    /**
     * @brief Check if signature is valid
     */
    std::optional<TamperIncident> checkSignatureValidity(
        const ImmutableAuditEntry& entry,
        const AuditSigner& signer
    );
    
    /**
     * @brief Check if chain-of-custody is maintained
     */
    std::optional<TamperIncident> checkChainOfCustody(
        const ImmutableAuditEntry& entry,
        const ImmutableAuditEntry& previous_entry
    );
    
    /**
     * @brief Check if sequence is valid
     */
    std::optional<TamperIncident> checkSequenceValidity(
        const ImmutableAuditEntry& entry,
        const ImmutableAuditEntry& previous_entry
    );
    
    /**
     * @brief Check timestamp consistency
     */
    std::optional<TamperIncident> checkTimestampValidity(
        const ImmutableAuditEntry& entry,
        const ImmutableAuditEntry& previous_entry
    );
};

// ============================================================================
// Audit Retention Policy - Enforce Retention and Archival Rules
// ============================================================================

/**
 * @struct AuditRetentionPolicy
 * @brief Policy for retaining and archiving audit entries
 */
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
    
    nlohmann::json toJson() const;
    static AuditRetentionPolicy fromJson(const nlohmann::json& j);
};

/**
 * @struct LegalHold
 * @brief Legal hold on audit entries (overrides retention policy)
 */
struct LegalHold {
    std::string hold_id;                      // Unique hold identifier
    std::string rule_id;                      // Rule ID (optional, for targeted holds)
    std::string initiated_by;                 // Who initiated the hold
    int64_t initiated_at_ms = 0;             // When hold was initiated
    int64_t expire_at_ms = 0;                // When hold expires (0 = indefinite)
    std::string reason;                       // Reason for legal hold
    std::string status;                       // "active", "released", "expired"
    
    nlohmann::json toJson() const;
    static LegalHold fromJson(const nlohmann::json& j);
};

/**
 * @class AuditRetentionManager
 * @brief Manages audit entry retention and archival
 */
class AuditRetentionManager {
public:
    /**
     * @brief Create retention manager
     * @param default_policy Default retention policy
     */
    explicit AuditRetentionManager(const AuditRetentionPolicy& default_policy);
    
    /**
     * @brief Check if entry should be archived
     * @param entry Audit entry
     * @param current_time_ms Current time
     * @return true if entry meets archival criteria
     */
    bool shouldArchive(
        const ImmutableAuditEntry& entry,
        int64_t current_time_ms
    ) const;
    
    /**
     * @brief Check if entry should be deleted
     * @param entry Audit entry
     * @param current_time_ms Current time
     * @return true if entry meets deletion criteria (and not on legal hold)
     */
    bool shouldDelete(
        const ImmutableAuditEntry& entry,
        int64_t current_time_ms
    ) const;
    
    /**
     * @brief Check if entry is on legal hold
     * @param rule_id Rule ID
     * @return true if any active legal hold covers this entry
     */
    bool isOnLegalHold(const std::string& rule_id) const;
    
    /**
     * @brief Add a legal hold
     * @param hold Legal hold to add
     */
    void addLegalHold(const LegalHold& hold);
    
    /**
     * @brief Release a legal hold
     * @param hold_id Hold ID to release
     */
    void releaseLegalHold(const std::string& hold_id);
    
    /**
     * @brief Get retention policy
     */
    const AuditRetentionPolicy& getPolicy() const { return policy_; }
    
    /**
     * @brief Set new retention policy (tracks change in audit log)
     * @param new_policy New policy
     * @param changed_by User making change
     */
    void setPolicy(
        const AuditRetentionPolicy& new_policy,
        const std::string& changed_by
    );
    
    /**
     * @brief Get policy change history
     */
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

/**
 * @class AuditIntegrityManager
 * @brief Comprehensive audit trail integrity management
 * 
 * Orchestrates:
 * - Cryptographic signing of audit entries
 * - Tamper detection and incident reporting
 * - Retention policy enforcement and archival
 * - Chain-of-custody verification
 * - Performance monitoring (latency tracking)
 */
class AuditIntegrityManager {
public:
    /**
     * @brief Create integrity manager
     * @param retention_policy Default retention policy
     * @param signer Audit signer for cryptographic operations
     */
    AuditIntegrityManager(
        const AuditRetentionPolicy& retention_policy,
        const std::shared_ptr<AuditSigner>& signer
    );
    
    /**
     * @brief Add an audit entry (automatically signs it)
     * @param entry Audit entry to add
     * @return Signed entry with integrity info
     */
    ImmutableAuditEntry addEntry(const ImmutableAuditEntry& entry);
    
    /**
     * @brief Verify audit trail integrity
     * @return Vector of TamperIncident objects (empty if valid)
     */
    std::vector<TamperIncident> verifyIntegrity();
    
    /**
     * @brief Verify integrity within time range
     * @param start_time_ms Start time
     * @param end_time_ms End time
     * @return Vector of TamperIncident objects
     */
    std::vector<TamperIncident> verifyTimeRange(
        int64_t start_time_ms,
        int64_t end_time_ms
    );
    
    /**
     * @brief Get entry by ID
     * @param entry_id Entry ID
     * @return Entry if found
     */
    std::optional<ImmutableAuditEntry> getEntry(const std::string& entry_id) const;
    
    /**
     * @brief Query entries
     * @param rule_id Optional rule filter
     * @param user Optional user filter
     * @param start_time_ms Optional start time
     * @param end_time_ms Optional end time
     * @return Matching entries
     */
    std::vector<ImmutableAuditEntry> queryEntries(
        const std::optional<std::string>& rule_id = std::nullopt,
        const std::optional<std::string>& user = std::nullopt,
        const std::optional<int64_t>& start_time_ms = std::nullopt,
        const std::optional<int64_t>& end_time_ms = std::nullopt
    ) const;
    
    /**
     * @brief Archive entries meeting retention policy
     * @return Number of entries archived
     */
    int64_t archiveExpiredEntries();
    
    /**
     * @brief Perform cleanup (delete entries past retention)
     * @return Number of entries deleted
     */
    int64_t performCleanup();
    
    /**
     * @brief Get tamper detection results
     */
    const std::vector<TamperIncident>& getLastTamperIncidents() const {
        return last_tamper_incidents_;
    }
    
    /**
     * @brief Get performance metrics
     * @return JSON with timing information
     */
    nlohmann::json getPerformanceMetrics() const;
    
    /**
     * @brief Export audit trail (all entries with signatures)
     * @param compress Whether to compress output
     * @return JSON export
     */
    nlohmann::json exportAuditTrail(bool compress = false) const;
    
    /**
     * @brief Import audit trail
     * @param data JSON import data
     * @return true if successful
     */
    bool importAuditTrail(const nlohmann::json& data);
    
    /**
     * @brief Rotate signing key
     * @param new_signer New signer for future entries
     * @param key_transition_entry Entry documenting the key rotation
     */
    void rotateKey(
        const std::shared_ptr<AuditSigner>& new_signer,
        const ImmutableAuditEntry& key_transition_entry
    );
    
    /**
     * @brief Get signing key history (for multi-key verification)
     */
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
     * @brief Get next sequence number
     */
    int64_t getNextSequenceNumber() const;
    
    /**
     * @brief Get previous entry hash
     */
    std::string getPreviousEntryHash() const;
};

} // namespace governance
} // namespace themis

#endif // THEMIS_GOVERNANCE_AUDIT_INTEGRITY_H
