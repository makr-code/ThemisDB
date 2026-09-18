/**
 * @file updates_diagnostics.h
 * @brief Unified error taxonomy and diagnostic primitives for the Updates module
 * @version 1.0.0
 * @since 1.8.1 (Q3 2026)
 * 
 * This header defines:
 *  - Error codes in the [7400-7499] range
 *  - Root cause classification
 *  - Severity levels and structured error context
 *  - Machine-parseable error taxonomy
 *
 * @note These error codes are reserved for Updates module diagnostics only.
 *       Other modules must use their own ranges.
 *
 * Doxygen maturity: 🟢 PRODUCTION-READY
 */

#pragma once

#include <cstdint>
#include <string>
#include <chrono>
#include "updates/update_state_machine.h"
#include <nlohmann/json.hpp>

namespace themis {
namespace updates {

using json = nlohmann::json;

// ============================================================================
// Error Code Range: [7400-7499] (100 codes reserved)
// ============================================================================

/**
 * @brief Unified error codes for the Updates module
 * 
 * Range: 7400–7499 (100 codes)
 *  - 7400–7419: State machine errors
 *  - 7420–7439: Rollback and checkpoint errors
 *  - 7440–7459: Patch application errors
 *  - 7460–7479: Network/coordination errors
 *  - 7480–7499: Miscellaneous/cascade errors
 */
enum class DiagnosticErrorCode : uint16_t {
    // State machine errors (7400–7419)
    STATE_INVALID_TRANSITION = 7400,     ///< Attempted invalid state transition
    STATE_ALREADY_IN_PROGRESS = 7401,    ///< Another update already in progress
    STATE_FAILED_LOCKED = 7402,          ///< State locked in FAILED; requires reset
    STATE_TRANSITION_TIMEOUT = 7403,     ///< State transition did not complete in time
    STATE_HISTORY_CORRUPT = 7404,        ///< Persisted state history corrupted
    
    // Rollback and checkpoint errors (7420–7439)
    ROLLBACK_CHECKPOINT_NOT_FOUND = 7420,    ///< Target checkpoint does not exist
    ROLLBACK_NO_CHECKPOINTS = 7421,          ///< No checkpoints available for rollback
    ROLLBACK_FAILED = 7422,                  ///< Rollback operation failed
    ROLLBACK_CASCADE_DETECTED = 7423,        ///< Rollback cascade prevented
    ROLLBACK_PARTIAL_SUCCESS = 7424,         ///< Partial rollback succeeded; some nodes failed
    ROLLBACK_DEFERRED = 7425,                ///< Rollback queued for retry
    ROLLBACK_ISOLATION_ACTIVE = 7426,        ///< Node isolation in place from prior rollback
    
    // Patch application errors (7440–7459)
    PATCH_APPLY_FAILED = 7440,               ///< Failed to apply delta patch
    PATCH_CHECKSUM_MISMATCH = 7441,          ///< File checksum does not match manifest
    PATCH_INCOMPATIBLE_BASE = 7442,          ///< Patch incompatible with current version
    PATCH_INCOMPLETE = 7443,                 ///< Patch file incomplete or truncated
    PATCH_DECODE_ERROR = 7444,               ///< Failed to decode patch format
    
    // Network/coordination errors (7460–7479)
    NETWORK_PARTITION = 7460,                ///< Network partition detected
    COORDINATION_TIMEOUT = 7461,             ///< Coordination timeout waiting for peer
    COORDINATION_PEER_FAILED = 7462,         ///< Peer node failed during coordination
    COORDINATION_QUORUM_LOST = 7463,         ///< Lost quorum during coordinated update
    COORDINATION_ORDERING_VIOLATION = 7464,  ///< Update sequence order violated
    
    // Cascade and miscellaneous errors (7480–7499)
    CASCADE_DETECTED = 7480,                 ///< Cascading failure detected and stopped
    DATA_LOSS_RISK = 7481,                  ///< Operation carries data loss risk
    UNSUPPORTED_OPERATION = 7482,            ///< Operation not supported in current state
    RESOURCE_EXHAUSTED = 7483,               ///< System resources exhausted (e.g., disk, memory)
    UNKNOWN_ERROR = 7499                     ///< Unknown/unclassified error
};

/**
 * @brief Severity level for diagnostic events
 * 
 * Used to prioritize operator attention and automation response.
 */
enum class DiagnosticSeverity {
    INFO,       ///< Normal operation progress; no action required
    WARN,       ///< Degraded path or retry in progress; monitor
    ERROR,      ///< Operation failed but isolated to single node/operation
    CRITICAL    ///< Unrecoverable failure or data loss risk; immediate action required
};

/**
 * @brief Root cause classification for an error
 * 
 * Helps operators and automated systems diagnose and respond appropriately.
 */
enum class RootCauseClass {
    ARTIFACT,           ///< Corrupted or invalid update artifact
    CHECKSUM,           ///< Integrity check failure (hash, signature, etc.)
    INCOMPATIBILITY,    ///< Version/schema/format incompatibility
    NETWORK,            ///< Network connectivity or partition
    CASCADE,            ///< Cascading failure from upstream operation
    STATE,              ///< Invalid or unexpected state machine state
    RESOURCE,           ///< Resource exhaustion (disk, memory, etc.)
    UNKNOWN             ///< Root cause unknown
};

/**
 * @brief Structured error context for diagnostic events
 * 
 * Provides machine-parseable context for errors, enabling:
 *  - Automated routing and remediation
 *  - Historical analysis and pattern detection
 *  - Operator dashboards and alerting
 */
struct ErrorContext {
    /// Timestamp when the error occurred
    std::chrono::system_clock::time_point timestamp;
    
    /// Error code (see DiagnosticErrorCode)
    DiagnosticErrorCode error_code = DiagnosticErrorCode::UNKNOWN_ERROR;
    
    /// Severity level
    DiagnosticSeverity severity = DiagnosticSeverity::ERROR;
    
    /// Root cause classification
    RootCauseClass root_cause = RootCauseClass::UNKNOWN;
    
    /// Human-readable error message
    std::string message;
    
    /// Operation in progress when error occurred (e.g., "apply_update", "rollback")
    std::string operation;
    
    /// Update phase (e.g., "downloading", "verifying", "applying", "rolling_back")
    std::string phase;
    
    /// Node ID where error occurred (empty if local/global)
    std::string node_id;
    
    /// Version being processed at time of error
    std::string version;
    
    /// Additional context (e.g., affected file path, network peer)
    json extra_context;
    
    /**
     * @brief Convert to JSON for structured logging
     */
    json toJson() const;
    
    /**
     * @brief Construct from JSON
     */
    static std::optional<ErrorContext> fromJson(const json& j);
};

/**
 * @brief Map diagnostic error code to severity
 */
inline DiagnosticSeverity severityForErrorCode(DiagnosticErrorCode code) {
    uint16_t c = static_cast<uint16_t>(code);
    
    if (c >= 7480 && c <= 7499) return DiagnosticSeverity::CRITICAL;  // Cascade/cascade
    if (c >= 7460 && c <= 7479) return DiagnosticSeverity::ERROR;     // Network/coordination
    if (c >= 7440 && c <= 7459) return DiagnosticSeverity::WARN;      // Patch
    if (c >= 7420 && c <= 7439) return DiagnosticSeverity::WARN;      // Rollback
    if (c >= 7400 && c <= 7419) return DiagnosticSeverity::ERROR;     // State machine
    
    return DiagnosticSeverity::ERROR;  // Default
}

/**
 * @brief Map error code to root cause
 */
inline RootCauseClass rootCauseForErrorCode(DiagnosticErrorCode code) {
    switch (code) {
        case DiagnosticErrorCode::PATCH_CHECKSUM_MISMATCH:
        case DiagnosticErrorCode::PATCH_INCOMPATIBLE_BASE:
        case DiagnosticErrorCode::PATCH_DECODE_ERROR:
            return RootCauseClass::CHECKSUM;
            
        case DiagnosticErrorCode::PATCH_APPLY_FAILED:
        case DiagnosticErrorCode::PATCH_INCOMPLETE:
            return RootCauseClass::ARTIFACT;
            
        case DiagnosticErrorCode::NETWORK_PARTITION:
        case DiagnosticErrorCode::COORDINATION_TIMEOUT:
        case DiagnosticErrorCode::COORDINATION_PEER_FAILED:
            return RootCauseClass::NETWORK;
            
        case DiagnosticErrorCode::CASCADE_DETECTED:
        case DiagnosticErrorCode::ROLLBACK_CASCADE_DETECTED:
            return RootCauseClass::CASCADE;
            
        case DiagnosticErrorCode::STATE_INVALID_TRANSITION:
        case DiagnosticErrorCode::STATE_FAILED_LOCKED:
            return RootCauseClass::STATE;
            
        case DiagnosticErrorCode::RESOURCE_EXHAUSTED:
            return RootCauseClass::RESOURCE;
            
        default:
            return RootCauseClass::UNKNOWN;
    }
}

/**
 * @brief Human-readable name for a diagnostic severity
 */
inline std::string severityName(DiagnosticSeverity s) {
    switch (s) {
        case DiagnosticSeverity::INFO:     return "INFO";
        case DiagnosticSeverity::WARN:     return "WARN";
        case DiagnosticSeverity::ERROR:    return "ERROR";
        case DiagnosticSeverity::CRITICAL: return "CRITICAL";
    }
    return "UNKNOWN";
}

/**
 * @brief Human-readable name for an error code
 */
inline std::string errorCodeName(DiagnosticErrorCode code) {
    switch (code) {
        case DiagnosticErrorCode::STATE_INVALID_TRANSITION:       return "STATE_INVALID_TRANSITION";
        case DiagnosticErrorCode::STATE_ALREADY_IN_PROGRESS:      return "STATE_ALREADY_IN_PROGRESS";
        case DiagnosticErrorCode::STATE_FAILED_LOCKED:            return "STATE_FAILED_LOCKED";
        case DiagnosticErrorCode::STATE_TRANSITION_TIMEOUT:       return "STATE_TRANSITION_TIMEOUT";
        case DiagnosticErrorCode::STATE_HISTORY_CORRUPT:          return "STATE_HISTORY_CORRUPT";
        case DiagnosticErrorCode::ROLLBACK_CHECKPOINT_NOT_FOUND:  return "ROLLBACK_CHECKPOINT_NOT_FOUND";
        case DiagnosticErrorCode::ROLLBACK_NO_CHECKPOINTS:        return "ROLLBACK_NO_CHECKPOINTS";
        case DiagnosticErrorCode::ROLLBACK_FAILED:                return "ROLLBACK_FAILED";
        case DiagnosticErrorCode::ROLLBACK_CASCADE_DETECTED:       return "ROLLBACK_CASCADE_DETECTED";
        case DiagnosticErrorCode::ROLLBACK_PARTIAL_SUCCESS:       return "ROLLBACK_PARTIAL_SUCCESS";
        case DiagnosticErrorCode::ROLLBACK_DEFERRED:              return "ROLLBACK_DEFERRED";
        case DiagnosticErrorCode::ROLLBACK_ISOLATION_ACTIVE:      return "ROLLBACK_ISOLATION_ACTIVE";
        case DiagnosticErrorCode::PATCH_APPLY_FAILED:             return "PATCH_APPLY_FAILED";
        case DiagnosticErrorCode::PATCH_CHECKSUM_MISMATCH:        return "PATCH_CHECKSUM_MISMATCH";
        case DiagnosticErrorCode::PATCH_INCOMPATIBLE_BASE:        return "PATCH_INCOMPATIBLE_BASE";
        case DiagnosticErrorCode::PATCH_INCOMPLETE:               return "PATCH_INCOMPLETE";
        case DiagnosticErrorCode::PATCH_DECODE_ERROR:             return "PATCH_DECODE_ERROR";
        case DiagnosticErrorCode::NETWORK_PARTITION:              return "NETWORK_PARTITION";
        case DiagnosticErrorCode::COORDINATION_TIMEOUT:           return "COORDINATION_TIMEOUT";
        case DiagnosticErrorCode::COORDINATION_PEER_FAILED:       return "COORDINATION_PEER_FAILED";
        case DiagnosticErrorCode::COORDINATION_QUORUM_LOST:       return "COORDINATION_QUORUM_LOST";
        case DiagnosticErrorCode::COORDINATION_ORDERING_VIOLATION: return "COORDINATION_ORDERING_VIOLATION";
        case DiagnosticErrorCode::CASCADE_DETECTED:                return "CASCADE_DETECTED";
        case DiagnosticErrorCode::DATA_LOSS_RISK:                 return "DATA_LOSS_RISK";
        case DiagnosticErrorCode::UNSUPPORTED_OPERATION:          return "UNSUPPORTED_OPERATION";
        case DiagnosticErrorCode::RESOURCE_EXHAUSTED:             return "RESOURCE_EXHAUSTED";
        case DiagnosticErrorCode::UNKNOWN_ERROR:                  return "UNKNOWN_ERROR";
    }
    return "UNKNOWN_ERROR";
}

// ============================================================================
// Unified Error Taxonomy (Phase 3 Standardization)
// ============================================================================

/**
 * @brief Base class for unified error taxonomy
 * 
 * Defines the interface for standardized error reporting across:
 *  - State transitions
 *  - Patch application
 *  - Migration operations
 *  - Rollout coordination
 */
class UpdateIncident {
public:
    virtual ~UpdateIncident() = default;
    
    /// Get the primary error code
    virtual DiagnosticErrorCode errorCode() const = 0;
    
    /// Get the root cause
    virtual RootCauseClass rootCause() const = 0;
    
    /// Get the severity
    virtual DiagnosticSeverity severity() const = 0;
    
    /// Get human-readable message
    virtual std::string message() const = 0;
    
    /// Get operation context
    virtual std::string operationContext() const = 0;
    
    /// Convert to structured JSON for logging
    virtual json toJson() const = 0;
};

/**
 * @brief State transition error - state machine contract violations
 * 
 * Covers invalid transitions, concurrent attempts, timeout, and state corruption.
 */
class StateTransitionError : public UpdateIncident {
private:
    DiagnosticErrorCode error_code_;
    std::string operation_;
    UpdateState from_state_;
    UpdateState to_state_;
    std::string reason_ = {};
    
public:
    StateTransitionError(
        DiagnosticErrorCode code,
        UpdateState from,
        UpdateState to,
        std::string reason
    ) : error_code_(code), from_state_(from), to_state_(to), reason_(std::move(reason)) {}
    
    DiagnosticErrorCode errorCode() const override { return error_code_; }
    RootCauseClass rootCause() const override { return RootCauseClass::STATE; }
    DiagnosticSeverity severity() const override { return severityForErrorCode(error_code_); }
    std::string message() const override { return reason_; }
    std::string operationContext() const override { return "state_transition"; }
    json toJson() const override;
};

/**
 * @brief Patch application error - delta/patch processing failures
 * 
 * Covers checksum mismatches, incompatibilities, corruption, and timeouts.
 */
class PatchApplyError : public UpdateIncident {
private:
    DiagnosticErrorCode error_code_;
    std::string patch_id_;
    std::string base_version_;
    std::string target_version_;
    std::string reason_;
    
public:
    PatchApplyError(
        DiagnosticErrorCode code,
        std::string patch_id,
        std::string base_version,
        std::string target_version,
        std::string reason
    ) : error_code_(code), patch_id_(std::move(patch_id)),
        base_version_(std::move(base_version)), target_version_(std::move(target_version)),
        reason_(std::move(reason)) {}
    
    DiagnosticErrorCode errorCode() const override { return error_code_; }
    RootCauseClass rootCause() const override { return rootCauseForErrorCode(error_code_); }
    DiagnosticSeverity severity() const override { return severityForErrorCode(error_code_); }
    std::string message() const override { return reason_; }
    std::string operationContext() const override { return "patch_apply"; }
    json toJson() const override;
};

/**
 * @brief Migration error - schema migration and transformation failures
 * 
 * Covers partial migrations, timeouts, transaction isolation, and fallbacks.
 */
class MigrationError : public UpdateIncident {
private:
    DiagnosticErrorCode error_code_;
    std::string table_name_;
    std::string operation_;
    std::string reason_;
    bool fallback_applied_;
    
public:
    MigrationError(
        DiagnosticErrorCode code,
        std::string table_name,
        std::string operation,
        std::string reason,
        bool fallback_applied = false
    ) : error_code_(code), table_name_(std::move(table_name)),
        operation_(std::move(operation)), reason_(std::move(reason)),
        fallback_applied_(fallback_applied) {}
    
    DiagnosticErrorCode errorCode() const override { return error_code_; }
    RootCauseClass rootCause() const override { return rootCauseForErrorCode(error_code_); }
    DiagnosticSeverity severity() const override { return severityForErrorCode(error_code_); }
    std::string message() const override { return reason_; }
    std::string operationContext() const override { return "migration"; }
    json toJson() const override;
    
    bool fallbackApplied() const { return fallback_applied_; }
};

/**
 * @brief Rollout error - coordinated deployment and rollback failures
 * 
 * Covers quorum loss, network partitions, cascade detection, and multi-node failures.
 */
class RolloutError : public UpdateIncident {
private:
    DiagnosticErrorCode error_code_;
    std::string update_id_;
    std::vector<std::string> failed_nodes_;
    std::string reason_;
    int healthy_node_count_;
    int total_node_count_;
    
public:
    RolloutError(
        DiagnosticErrorCode code,
        std::string update_id,
        std::vector<std::string> failed_nodes,
        std::string reason,
        int healthy_node_count,
        int total_node_count
    ) : error_code_(code), update_id_(std::move(update_id)),
        failed_nodes_(std::move(failed_nodes)), reason_(std::move(reason)),
        healthy_node_count_(healthy_node_count), total_node_count_(total_node_count) {}
    
    DiagnosticErrorCode errorCode() const override { return error_code_; }
    RootCauseClass rootCause() const override { return rootCauseForErrorCode(error_code_); }
    DiagnosticSeverity severity() const override { return severityForErrorCode(error_code_); }
    std::string message() const override { return reason_; }
    std::string operationContext() const override { return "rollout"; }
    json toJson() const override;
    
    const std::vector<std::string>& failedNodes() const { return failed_nodes_; }
    int healthyNodeCount() const { return healthy_node_count_; }
    int totalNodeCount() const { return total_node_count_; }
};

} // namespace updates
} // namespace themis
