/**
 * @file ingestion_api_contract.h
 * @brief Frozen ingestion contract: write-path durability, back-pressure, batch atomicity,
 *        schema validation, and error taxonomy.
 *
 * This header defines the normative contract for the ingestion module.
 * All ingestion components (IngestionCoordinator, IngestionManager, workflow engine,
 * connector adapters, schema validators) must honour these semantics within v1.x.
 *
 * ## Write-Path Durability Contract
 *
 * Once an ack is returned to the producer, the data MUST be durable (either
 * written to a WAL or accepted by a quorum of replicas).  Silent drops after
 * ack are a contract violation.
 *
 * ## Back-Pressure Contract
 *
 * When the ingestion buffer is full, the producer receives an explicit
 * INGESTION_BUFFER_FULL signal.  Silent blocking (holding the producer without
 * signalling) is a contract violation.  The signal must be returned within
 * kBackPressureSignalDeadline of the buffer-full condition being detected.
 *
 * ## Batch Atomicity Contract
 *
 * A batch is committed atomically: readers see either zero rows (pre-commit)
 * or all rows (post-commit) of the batch.  A partial batch is never visible.
 * On partial failure during commit, the entire batch is rolled back.
 *
 * ## Schema Validation Contract
 *
 * Schema validation is performed BEFORE the ack is sent to the producer.
 * A schema violation surfaces INGESTION_SCHEMA_INVALID and no data is written.
 *
 * ## Versioning
 *
 * This contract is stable within v1.x.  Breaking changes require a v2.0 bump.
 *
 * @see src/ingestion/ROADMAP.md — Phase 1 item
 * @see include/ingestion/ingestion_coordinator.h
 * @see include/ingestion/ingestion_manager.h
 * @see include/ingestion/semantic_validator.h
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <string>

#include "utils/retry_contract.h"

namespace themis {
namespace ingestion {

// ============================================================================
// § 1  Buffer and batch sizing
// ============================================================================

/// Default ingestion buffer capacity (number of pending rows).
inline constexpr std::size_t kDefaultBufferCapacity = 65'536u;

/// Maximum ingestion buffer capacity (operator-configurable upper bound).
inline constexpr std::size_t kMaxBufferCapacity = 1'000'000u;

/// Default maximum batch size (rows per atomic commit).
inline constexpr std::size_t kDefaultBatchSize = 1'000u;

/// Maximum batch size (operator-configurable upper bound).
inline constexpr std::size_t kMaxBatchSize = 100'000u;

// ============================================================================
// § 2  Timing contracts
// ============================================================================

/// Maximum latency for back-pressure signal delivery after buffer-full.
inline constexpr std::chrono::milliseconds kBackPressureSignalDeadline{50};

/// Default ingestion write timeout (producer-to-ack round trip).
inline constexpr std::chrono::seconds kDefaultWriteTimeout{10};

/// Maximum allowed ingestion write timeout.
inline constexpr std::chrono::seconds kMaxWriteTimeout{120};

/// Default batch flush interval (time-based trigger when batch not full).
inline constexpr std::chrono::milliseconds kDefaultBatchFlushInterval{500};

// ============================================================================
// § 3  Quota contract
// ============================================================================

/// Default maximum rows per second per producer (0 = unlimited).
inline constexpr std::uint64_t kDefaultProducerRateLimit = 0u;

/// Default maximum bytes per second per producer (0 = unlimited).
inline constexpr std::uint64_t kDefaultProducerBytesRateLimit = 0u;

// ============================================================================
// § 4  Durability levels
// ============================================================================

/**
 * @brief Durability guarantee level for an ingestion ack.
 */
enum class DurabilityLevel : int {
    /// Ack after WAL write on the local node only.
    LocalWal   = 0,
    /// Ack after quorum-acknowledged write across replicas.
    QuorumSync = 1,
    /// Ack after full fsync on the local node (strongest single-node guarantee).
    FsyncLocal = 2,
};

// ============================================================================
// § 5  Error taxonomy
// ============================================================================

/**
 * @brief Canonical error codes for the ingestion module.
 *
 * New codes must be appended; existing values must never be renumbered.
 */
enum class IngestionErrorCode : int {
    /// No error; data accepted and acked.
    OK = 0,

    /// Ingestion buffer is full; producer must back off.
    INGESTION_BUFFER_FULL = 1,

    /// Row or batch failed schema validation; no data written.
    INGESTION_SCHEMA_INVALID = 2,

    /// A row violates a primary-key or unique constraint.
    INGESTION_DUPLICATE_KEY = 3,

    /// The ingestion quota (rate or byte limit) has been exceeded.
    INGESTION_QUOTA_EXCEEDED = 4,

    /// Write timed out before ack was sent; data NOT committed.
    INGESTION_TIMEOUT = 5,

    /// Batch commit failed partway through; entire batch rolled back.
    INGESTION_BATCH_ROLLBACK = 6,

    /// Downstream persistence layer is unavailable.
    INGESTION_BACKEND_UNAVAILABLE = 7,

    /// The connector (Kafka, S3, CDC) could not deliver the event.
    INGESTION_CONNECTOR_FAILED = 8,

    /// Internal ingestion error.
    INTERNAL_ERROR = 9,
};

/// Returns true for errors that indicate the producer should retry later.
[[nodiscard]] inline constexpr bool isBackOffCode(IngestionErrorCode code) noexcept {
    return code == IngestionErrorCode::INGESTION_BUFFER_FULL
        || code == IngestionErrorCode::INGESTION_QUOTA_EXCEEDED
        || code == IngestionErrorCode::INGESTION_TIMEOUT
        || code == IngestionErrorCode::INGESTION_BACKEND_UNAVAILABLE;
}

/// Returns true for errors that indicate a permanent rejection (no retry).
[[nodiscard]] inline constexpr bool isPermanentRejection(IngestionErrorCode code) noexcept {
    return code == IngestionErrorCode::INGESTION_SCHEMA_INVALID
        || code == IngestionErrorCode::INGESTION_DUPLICATE_KEY;
}

/**
 * @brief Map ingestion failures to canonical timeout-source taxonomy.
 */
[[nodiscard]] inline constexpr themis::utils::RetryTimeoutSource toRetryTimeoutSource(
    IngestionErrorCode code) noexcept {
    if (code == IngestionErrorCode::INGESTION_TIMEOUT) {
        return themis::utils::RetryTimeoutSource::PER_ATTEMPT;
    }
    if (code == IngestionErrorCode::INGESTION_BACKEND_UNAVAILABLE) {
        return themis::utils::RetryTimeoutSource::BACKEND;
    }
    return themis::utils::RetryTimeoutSource::NONE;
}

/**
 * @brief Map ingestion outcomes to canonical retry-exhaustion reasons.
 */
[[nodiscard]] inline constexpr themis::utils::RetryExhaustionReason
toRetryExhaustionReason(IngestionErrorCode code, bool retries_exhausted) noexcept {
    if (isPermanentRejection(code)) {
        return themis::utils::RetryExhaustionReason::NON_RETRYABLE;
    }
    if (retries_exhausted && isBackOffCode(code)) {
        return themis::utils::RetryExhaustionReason::MAX_ATTEMPTS_REACHED;
    }
    if (code == IngestionErrorCode::INGESTION_TIMEOUT) {
        return themis::utils::RetryExhaustionReason::TIME_BUDGET_EXCEEDED;
    }
    return themis::utils::RetryExhaustionReason::NONE;
}

// ============================================================================
// § 6  Ack contract
//
// An ack MUST NOT be sent until ALL of the following are true:
//   a) Schema validation passed.
//   b) The data has been written to the WAL (or quorum, per durability level).
//   c) The batch sequence number has been assigned.
//
// Sending an early ack followed by a write failure is a contract violation.
// ============================================================================

/// Returns true when a durability level satisfies the strong-ack contract.
[[nodiscard]] inline constexpr bool isStrongAck(DurabilityLevel level) noexcept {
    return level == DurabilityLevel::QuorumSync || level == DurabilityLevel::FsyncLocal;
}

} // namespace ingestion
} // namespace themis
