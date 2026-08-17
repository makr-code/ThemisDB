/**
 * @file replication_failover_diagnostics.h
 * @brief Wave A-8 Enhanced Failover Diagnostics API for ThemisDB Replication.
 *
 * Provides comprehensive diagnostic interfaces for failover operations,
 * enabling operators to understand and debug replication promotion/failover
 * events in production environments.
 *
 * ## Design Principles
 *
 * - **Fail-safe observability**: Diagnostics never block or delay failover logic
 * - **Non-invasive**: Diagnostic queries don't modify replication state
 * - **Real-time**: Snapshots reflect current state at query time
 * - **Root-cause traceability**: Each diagnostic includes decision rationale
 *
 * @see src/replication/ROADMAP.md — §3.1 Stronger failover diagnostics
 * @see include/replication/replication_manager.h — ReplicationManager
 *
 * @version 1.0.0
 * @note Part of Wave A-8 Replication Module Closure
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>
#include <optional>

namespace themisdb {
namespace replication {

// ============================================================================
// § 1 Failover candidate ranking diagnostic
// ============================================================================

/**
 * @brief Diagnostic information about a failover candidate.
 *
 * Provides transparency into the candidate selection process, enabling
 * operators to understand why a particular replica was (or was not)
 * selected during failover.
 */
struct FailoverCandidateDiagnostic {
    /// Replica node ID
    std::string node_id;

    /// Ranking score (higher is better)
    int32_t ranking_score{0};

    /// Reasons why candidate was selected/rejected
    struct EvaluationSteps {
        /// Replica is healthy (passes heartbeat check)
        bool health_check_passed{false};
        std::string health_reason;

        /// Replica has acceptable replication lag
        bool lag_check_passed{false};
        std::string lag_reason;
        int64_t lag_ms{-1};

        /// Replica matches placement constraints
        bool placement_check_passed{false};
        std::string placement_reason;

        /// Replica is a voting member
        bool voter_check_passed{false};
        std::string voter_reason;

        /// Final eligibility decision
        bool is_eligible{false};
        std::string final_decision;
    } steps;

    /// Timestamp when this diagnostic was captured
    std::chrono::system_clock::time_point captured_at;
};

/**
 * @brief Get diagnostic information about all failover candidates.
 *
 * Returns the evaluation steps and ranking for each candidate, enabling
 * operators to understand the failover selection process.
 *
 * @param exclude_node_id Node ID to exclude from candidate list (typically
 *                        the failed leader).
 * @return Vector of diagnostic information for each candidate, sorted by
 *         ranking score (best first).
 *
 * @note Thread-safe; safe to call concurrently from any thread.
 * @note Captures a snapshot at the time of the call.
 */
std::vector<FailoverCandidateDiagnostic>
getFailoverCandidateDiagnostics(const std::string& exclude_node_id);

// ============================================================================
// § 2 Failover execution diagnostic
// ============================================================================

/**
 * @brief Diagnostic information about a failover operation.
 *
 * Captures the history and outcomes of failover attempts, enabling
 * root-cause analysis of failover failures or delays.
 */
struct FailoverExecutionDiagnostic {
    /// Unique failover operation ID
    std::string failover_id;

    /// Timestamp when failover was initiated
    std::chrono::system_clock::time_point initiated_at;

    /// Node ID of the failed leader
    std::string failed_leader_id;

    /// Replica ID selected for promotion
    std::string promoted_replica_id;

    /// Outcome of the failover attempt
    enum class Outcome {
        PENDING,          ///< Failover in progress
        SUCCESS,          ///< Promotion completed
        FAILED,           ///< Promotion failed (see failure_reason)
        CANCELLED,        ///< Failover was cancelled (see cancellation_reason)        
    };
    Outcome outcome{Outcome::PENDING};

    /// If outcome == FAILED, reason for the failure
    std::string failure_reason;

    /// If outcome == CANCELLED, reason for cancellation
    std::string cancellation_reason;

    /// Duration of failover operation
    std::optional<std::chrono::milliseconds> duration_ms;

    /// Sequence of events during failover
    struct Event {
        std::chrono::system_clock::time_point timestamp;
        std::string description;
    };
    std::vector<Event> event_log;

    /// Pre-failover replication lag of the promoted replica
    int64_t replica_lag_before_promotion_ms{0};

    /// Post-failover lag advancement rate (entries/sec)
    double lag_advancement_rate{0.0};
};

/**
 * @brief Get diagnostic information about the last failover operation.
 *
 * Returns detailed information about the most recent failover (whether
 * successful or failed), enabling operators to investigate failover issues.
 *
 * @return Diagnostic information, or std::nullopt if no failover has occurred.
 *
 * @note Thread-safe; returns a snapshot of the last operation.
 */
std::optional<FailoverExecutionDiagnostic> getLastFailoverDiagnostic();

/**
 * @brief Get diagnostic information about a specific failover operation.
 *
 * @param failover_id ID of the failover operation.
 * @return Diagnostic information, or std::nullopt if not found.
 */
std::optional<FailoverExecutionDiagnostic> getFailoverDiagnostic(
    const std::string& failover_id);

/**
 * @brief Get history of recent failover operations.
 *
 * @param max_entries Maximum number of entries to return (0 = all available).
 * @return Vector of failover diagnostics, ordered by timestamp (newest first).
 *
 * @note Useful for trend analysis (e.g., are failovers becoming slower?)
 */
std::vector<FailoverExecutionDiagnostic> getFailoverHistory(
    size_t max_entries = 0);

// ============================================================================
// § 3 Replica health state transitions
// ============================================================================

/**
 * @brief Diagnostic information about replica health state changes.
 *
 * Enables tracking of when and why replicas transition between health states,
 * useful for diagnosing intermittent replica issues.
 */
struct ReplicaHealthTransitionDiagnostic {
    /// Node ID of the replica
    std::string node_id;

    /// Previous health status
    enum class HealthStatus {
        UNKNOWN,
        HEALTHY,
        DEGRADED,
        UNHEALTHY,
    };
    HealthStatus previous_status{HealthStatus::UNKNOWN};

    /// New health status
    HealthStatus new_status{HealthStatus::UNKNOWN};

    /// Timestamp of transition
    std::chrono::system_clock::time_point transitioned_at;

    /// Reason for health change
    std::string reason;

    /// Time spent in previous state
    std::optional<std::chrono::milliseconds> duration_in_previous_state;

    /// Diagnostics data that triggered the transition (e.g., lag values)
    std::string diagnostic_data;
};

/**
 * @brief Get recent health state transitions for a specific replica.
 *
 * @param node_id Replica node ID.
 * @param max_entries Maximum number of transitions to return.
 * @return Vector of transitions, ordered by timestamp (newest first).
 */
std::vector<ReplicaHealthTransitionDiagnostic>
getReplicaHealthHistory(const std::string& node_id, size_t max_entries = 10);

/**
 * @brief Get recent health state transitions for all replicas.
 *
 * @param max_entries Maximum total transitions to return.
 * @return Vector of transitions for all replicas, ordered by timestamp.
 */
std::vector<ReplicaHealthTransitionDiagnostic> getClusterHealthHistory(
    size_t max_entries = 50);

// ============================================================================
// § 4 Promotion eligibility analysis
// ============================================================================

/**
 * @brief Analysis of whether a replica can be promoted.
 *
 * Provides detailed breakdown of promotion eligibility checks, enabling
 * operators to understand why a replica cannot be promoted if needed for
 * manual emergency failover.
 */
struct PromotionEligibilityAnalysis {
    /// Replica node ID
    std::string node_id;

    /// Can the replica be promoted?
    bool is_eligible{false};

    /// Analysis of each eligibility criterion
    struct Criterion {
        std::string name;           ///< e.g., "quorum_check", "lag_check"
        bool passed{false};
        std::string reason;
        std::string remediation;    ///< How to fix if failed
    };
    std::vector<Criterion> criteria;

    /// Estimated time until replica becomes eligible (if applicable)
    std::optional<std::chrono::milliseconds> estimated_time_to_eligible;

    /// Timestamp of analysis
    std::chrono::system_clock::time_point analyzed_at;
};

/**
 * @brief Analyze promotion eligibility for a specific replica.
 *
 * @param node_id Replica node ID to analyze.
 * @return Detailed eligibility analysis.
 *
 * @note Thread-safe; returns a snapshot at the time of the call.
 */
PromotionEligibilityAnalysis analyzePromotionEligibility(
    const std::string& node_id);

/**
 * @brief Analyze promotion eligibility for all replicas.
 *
 * @return Vector of eligibility analyses, sorted by node ID.
 */
std::vector<PromotionEligibilityAnalysis> analyzeAllReplicasPromotion();

// ============================================================================
// § 5 Consensus health monitoring
// ============================================================================

/**
 * @brief Diagnostic information about consensus layer health.
 *
 * Tracks quorum health, leader heartbeat, and other consensus-layer
 * indicators that affect failover readiness.
 */
struct ConsensusHealthDiagnostic {
    /// Is a leader currently elected?
    bool has_leader{false};

    /// Current leader node ID (empty if no leader)
    std::string leader_id;

    /// Is the leader responsive (recent heartbeat)?
    bool leader_responsive{false};

    /// Time since last leader heartbeat
    std::chrono::milliseconds time_since_heartbeat;

    /// Number of replicas in quorum
    int32_t quorum_size{0};

    /// Number of replicas currently reachable
    int32_t replicas_reachable{0};

    /// Is quorum healthy (reachable >= quorum_size / 2 + 1)?
    bool quorum_healthy{false};

    /// Current election term
    uint64_t current_term{0};

    /// Number of elections in the last hour
    uint32_t elections_last_hour{0};

    /// Time of diagnostic capture
    std::chrono::system_clock::time_point captured_at;
};

/**
 * @brief Get diagnostic information about consensus health.
 *
 * @return Current consensus health state.
 *
 * @note Thread-safe; returns a snapshot.
 */
ConsensusHealthDiagnostic getConsensusHealthDiagnostic();

}  // namespace replication
}  // namespace themisdb
