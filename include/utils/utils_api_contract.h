// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors
#pragma once

/**
 * @file utils_api_contract.h
 * @brief Frozen API contract for the ThemisDB utils module.
 * @version 1.0.0
 *
 * @section purpose Purpose
 * The utils module provides cross-cutting infrastructure: audit logging,
 * batch operation management, Bloom filters, concurrency helpers, retry
 * scheduling, serialisation, and metrics collection.  All utilities must
 * be side-effect-free unless they explicitly manage external resources.
 *
 * @section contracts API Contracts
 *
 * ### AuditLogger
 * - `log()` is non-blocking for normal writes; flushes are synchronous.
 * - Log entries are immutable once written.
 * - If the sink is full → UTILS_AUDIT_OVERFLOW (entries are NOT silently
 *   dropped unless the "drop-on-full" policy is explicitly configured).
 *
 * ### BatchOperationManager
 * - Batch operations are committed atomically; partial failure triggers full
 *   rollback and returns UTILS_BATCH_ROLLBACK.
 * - Maximum batch size is configurable at construction; exceeding it returns
 *   UTILS_BATCH_SIZE_EXCEEDED.
 *
 * ### BloomFilter
 * - `contains()` never returns a false negative.
 * - `falsePositiveRate()` is bounded by the configured target FPR at creation.
 * - After `clear()`, all subsequent `contains()` calls return false.
 *
 * ### RetryScheduler
 * - Retry attempts are bounded by `maxAttempts`; exhaustion → UTILS_RETRY_EXHAUSTED.
 * - Jitter is deterministic given a fixed seed (for testability).
 *
 * ### Serialisation
 * - `serialize()` / `deserialize()` round-trip is lossless for supported types.
 * - Deserialising malformed input returns UTILS_DESER_INVALID.
 *
 * @section error_taxonomy Error Taxonomy
 * | Code                       | Meaning                                       |
 * |----------------------------|-----------------------------------------------|
 * | UTILS_AUDIT_OVERFLOW       | Audit log sink full; entries queued/dropped   |
 * | UTILS_BATCH_ROLLBACK       | Batch partially failed; fully rolled back     |
 * | UTILS_BATCH_SIZE_EXCEEDED  | Batch exceeds configured maximum size         |
 * | UTILS_RETRY_EXHAUSTED      | Max retry attempts reached                    |
 * | UTILS_DESER_INVALID        | Deserialisation input is malformed            |
 * | UTILS_POOL_EXHAUSTED       | Thread/connection pool exhausted              |
 *
 * @section threading Threading Guarantees
 * - `AuditLogger` is thread-safe (lock-free ring buffer).
 * - `BatchOperationManager` is NOT thread-safe; callers must serialise access.
 * - `BloomFilter` read path is thread-safe; mutations require external sync.
 *
 * @section contract_freeze Contract Freeze
 * Frozen for ThemisDB v2.x.
 *
 * @see src/utils/ROADMAP.md — Phase 1 gate
 * @see benchmarks/utils/bench_utils_release_gates.cpp
 * @see tests/utils/test_utils_contract_hardening_focused.cpp
 */

#include <cstdint>
#include <optional>
#include <string>

namespace themis::utils {

/// @brief Error codes for the utils module.
enum class UtilsError : int32_t {
    kAuditOverflow      = 7300, ///< Audit log sink full
    kBatchRollback      = 7301, ///< Batch failed; rolled back
    kBatchSizeExceeded  = 7302, ///< Batch exceeds max size
    kRetryExhausted     = 7303, ///< Max retry attempts reached
    kDeserInvalid       = 7304, ///< Malformed deserialisation input
    kPoolExhausted      = 7305, ///< Thread/connection pool exhausted
};

/// @brief Bloom filter configuration.
struct BloomFilterConfig {
    uint64_t expectedItems{100000};
    double   targetFalsePositiveRate{0.01};
};

/// @brief Retry policy configuration.
struct RetryPolicy {
    uint32_t maxAttempts{3};
    std::chrono::milliseconds initialDelay{100};
    double   backoffMultiplier{2.0};
    bool     withJitter{true};
};

} // namespace themis::utils
