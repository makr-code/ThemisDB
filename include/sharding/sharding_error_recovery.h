/**
 * @file sharding_error_recovery.h
 * @brief Phase 3: Standardized Error Recovery Policies for Sharding Failures
 *
 * This header defines the canonical recovery strategy for each error code in the
 * sharding module. All components MUST follow these recovery policies to ensure
 * consistent fail-safe behavior across the system.
 *
 * @section strategy Recovery Strategy Categories
 *
 * - FAIL_CLOSED: Do not proceed; return error immediately. (e.g., QUORUM_LOST)
 * - RETRY_WITH_BACKOFF: Attempt retry with exponential backoff. (e.g., SHARD_UNAVAILABLE)
 * - DEGRADE_READONLY: Fall back to read-only mode. (e.g., COORDINATOR_FAILURE)
 * - TIMEOUT_AND_ABORT: Wait up to configured timeout, then abort. (e.g., CONSENSUS_TIMEOUT)
 * - ROLLBACK_AUTOMATIC: Automatically roll back pending changes. (e.g., MIGRATION_FAULT)
 * - RECOVERY_REQUIRED: Manual operator intervention required. (e.g., WAL_CORRUPTION)
 *
 * @section idempotence Idempotence Guarantee
 *
 * All recovery paths MUST be idempotent: executing a recovery action twice with
 * the same input must produce the same output as executing it once. This is
 * essential for fault-tolerant distributed systems where recovery actions may
 * be retried due to transient failures.
 *
 * @see src/sharding/ROADMAP.md — Phase 3 item
 * @see tests/sharding/test_sharding_phase3_edgecases.cpp
 * @see docs/sharding/QUORUM_LOSS_RUNBOOK.md
 */

#pragma once

#include <cstdint>
#include <string>
#include <utility>

namespace themis {
namespace sharding {

// Forward declaration
enum class ShardingErrorCode : int;

/**
 * @brief Recovery action strategy for a given error code.
 *
 * These strategies are normative: every place that encounters one of the error
 * codes MUST apply the corresponding recovery strategy.
 */
enum class ErrorRecoveryStrategy : uint8_t {
    /// Do not proceed; return error immediately. Never degrade silently.
    FAIL_CLOSED                = 0,

    /// Attempt retry with exponential backoff (bounded by kRetryMaxAttempts).
    RETRY_WITH_BACKOFF        = 1,

    /// Fall back to read-only mode; block all write operations.
    DEGRADE_READONLY          = 2,

    /// Wait up to kErrorRecoveryTimeout, then abort operation.
    TIMEOUT_AND_ABORT         = 3,

    /// Automatically roll back all pending changes in a safe order.
    ROLLBACK_AUTOMATIC        = 4,

    /// Manual operator intervention required; halt operation.
    RECOVERY_REQUIRED         = 5,
};

/**
 * @brief Recovery action to be taken for a specific error.
 *
 * This struct encapsulates the strategy and its parameters (e.g., retry count).
 */
struct RecoveryAction {
    /// Strategy for recovering from this error.
    ErrorRecoveryStrategy strategy;

    /// Retry count for RETRY_WITH_BACKOFF strategy (0 for non-retry strategies).
    int retry_count;

    /// Timeout in milliseconds for TIMEOUT_AND_ABORT (0 for non-timeout strategies).
    int timeout_ms;

    /// Human-readable description of the recovery action.
    std::string description;

    RecoveryAction(
        ErrorRecoveryStrategy strat,
        int retries = 0,
        int timeout = 0,
        const std::string& desc = ""
    ) : strategy(strat), retry_count(retries), timeout_ms(timeout), description(desc) {}
};

// ============================================================================
// § 1  Recovery Strategy Mapping
// ============================================================================

/**
 * @brief Get the recovery action for a given error code.
 *
 * This function is the canonical source of truth for error recovery. All
 * sharding components MUST use this to determine how to handle errors.
 *
 * @param ec The error code.
 * @return Recovery action to be taken.
 *
 * @note Recovery strategies are deterministic and stateless (always return the
 *       same action for the same input).
 */
[[nodiscard]] inline RecoveryAction getRecoveryAction(ShardingErrorCode ec) noexcept;

/**
 * @brief Get a human-readable name for an error code.
 *
 * @param ec The error code.
 * @return String name (e.g., "QUORUM_LOST").
 */
[[nodiscard]] std::string errorCodeName(ShardingErrorCode ec) noexcept;

/**
 * @brief Check if an error code mandates fail-closed behavior.
 *
 * This is equivalent to checking if getRecoveryAction(ec).strategy == FAIL_CLOSED,
 * but provided separately for convenience.
 *
 * @param ec The error code.
 * @return true if this error MUST NOT be silently degraded.
 *
 * @see isShardingFailClosedCode() in sharding_api_contract.h
 */
[[nodiscard]] inline bool isFailClosedError(ShardingErrorCode ec) noexcept;

// ============================================================================
// § 2  Recovery Path Idempotence Guarantees
// ============================================================================

/**
 * @brief Marker interface for idempotent recovery operations.
 *
 * Types implementing this interface guarantee that:
 * - execute() with the same input produces the same output on retry
 * - No side effects persist on repeated execution
 * - State is not duplicated (e.g., no duplicate WAL entries)
 *
 * Implementations are responsible for:
 * - Unique request/operation IDs (e.g., transaction-id, operation-id)
 * - Deduplication (checking if operation already completed)
 * - Immutable recovery state (no conflicting concurrent modifications)
 */
class IdempotentRecoveryOperation {
public:
    virtual ~IdempotentRecoveryOperation() = default;

    /// Execute the recovery operation. Safe to call multiple times.
    /// @return Pair of (success, description).
    virtual std::pair<bool, std::string> execute() = 0;

    /// Get a unique operation identifier (e.g., transaction-id).
    /// Used for deduplication and log tracking.
    virtual std::string getOperationId() const = 0;
};

// ============================================================================
// § 3  Recovery Configuration Constants
// ============================================================================

/// Maximum attempts for RETRY_WITH_BACKOFF strategy.
inline constexpr int kRetryMaxAttempts = 5;

/// Initial backoff interval for retries in milliseconds.
inline constexpr int kRetryBackoffMs = 100;

/// Maximum backoff interval in milliseconds (exponential cap).
inline constexpr int kRetryMaxBackoffMs = 10000;

/// Default timeout for TIMEOUT_AND_ABORT strategy in milliseconds.
inline constexpr int kErrorRecoveryTimeout = 30000;  // 30 seconds

/// Timeout for automatic rollback operations in milliseconds.
inline constexpr int kRollbackTimeout = 60000;  // 60 seconds

} // namespace sharding
} // namespace themis

// Include implementation (out-of-line definitions)
#include "sharding_error_recovery_impl.h"

