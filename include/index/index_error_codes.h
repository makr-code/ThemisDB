/**
 * @file index_error_codes.h
 * @brief Frozen error taxonomy for the Index module backend, rebuild, and distribution failure classes.
 *
 * ThemisDB | File: index_error_codes.h | Version: 1.0.0
 * Maturity: FROZEN — Q3 2026 gate delivery
 * Author: Copilot | Date: 2026-08-09
 *
 * This header defines the canonical error taxonomy for the Index module.
 * All index components (AnnFrontdoor, TieredIndexManager, ShardedIndexProxy,
 * rebuild pipeline) MUST use these codes when returning structured errors.
 *
 * ## Error Code Ranges
 *
 * | Range   | Class              |
 * |---------|--------------------|
 * | 1100–1149 | Backend errors   |
 * | 1150–1174 | Rebuild errors   |
 * | 1175–1199 | Distribution errors |
 *
 * @see include/index/ann_frontdoor.h — AnnFrontdoor::search() that uses these codes
 * @see include/index/ann_index.h — IAnnIndex backend interface
 * @see src/index/ROADMAP.md — Phase 1 contract item
 */

#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

namespace themis::index {

// ============================================================================
// § 1  IndexErrorCode — frozen taxonomy
// ============================================================================

/**
 * @brief Frozen error taxonomy for all Index module failure classes.
 *
 * Ranges:
 *  - 1100–1149: backend (ANN, HNSW, ScaNN, DiskANN, flat) errors
 *  - 1150–1174: rebuild / compaction / migration errors
 *  - 1175–1199: distributed / shard / fan-out errors
 */
enum class IndexErrorCode : uint32_t {

    // ── Backend errors (1100–1149) ───────────────────────────────────────────

    /// ANN backend search returned no results when results were expected.
    /// May indicate an empty or unbuilt index.
    BACKEND_EMPTY_RESULT = 1100,

    /// ANN backend search failed with an internal error.
    /// Non-retryable without investigation; check logs for backend details.
    BACKEND_SEARCH_FAILED = 1101,

    /// ANN backend index build or add operation failed.
    /// Rebuild will be required before further searches succeed.
    BACKEND_BUILD_FAILED = 1102,

    /// ANN backend is not initialised or has been reset.
    /// Callers must rebuild or re-register the backend before use.
    BACKEND_NOT_READY = 1103,

    /// The requested ANN strategy is unavailable (no backend registered
    /// for the strategy, or all backends for the strategy failed health checks).
    BACKEND_STRATEGY_UNAVAILABLE = 1104,

    /// The backend rejected the input dimension: the query dimension does not
    /// match the dimension the index was built with.
    BACKEND_DIM_MISMATCH = 1105,

    /// The backend returned a result with invalid distances (NaN, negative).
    /// These were filtered by the AnnFrontdoor validation layer.
    /// @see src/index/ann_frontdoor.cpp — Phase B gate validation block
    BACKEND_INVALID_RESULT = 1106,

    /// The backend reported a resource exhaustion condition
    /// (memory, file descriptors, GPU VRAM).
    BACKEND_RESOURCE_EXHAUSTED = 1107,

    // ── Rebuild / compaction / migration errors (1150–1174) ─────────────────

    /// Rebuild request rejected because a rebuild is already in progress.
    REBUILD_ALREADY_IN_PROGRESS = 1150,

    /// Rebuild failed during vector ingestion phase.
    /// Partial index state: do not serve queries until rebuild completes.
    REBUILD_INGEST_FAILED = 1151,

    /// Rebuild failed during index compaction phase.
    REBUILD_COMPACTION_FAILED = 1152,

    /// Rebuild aborted by a cancellation signal.
    REBUILD_CANCELLED = 1153,

    /// Rebuild failed because the source data store is unavailable.
    REBUILD_SOURCE_UNAVAILABLE = 1154,

    /// Rebuild exceeded the configured time budget.
    REBUILD_TIMEOUT = 1155,

    // ── Distribution / shard / fan-out errors (1175–1199) ────────────────────

    /// All shard backends for a distributed query failed.
    /// No results available; the query cannot be satisfied in degraded mode.
    DISTRIBUTION_ALL_SHARDS_FAILED = 1175,

    /// A quorum of shard backends failed; results are marked as partial
    /// (`AnnFrontdoorResult::is_distributed == true`,
    ///  `shards_succeeded < shards_attempted`).
    DISTRIBUTION_PARTIAL_FAILURE = 1176,

    /// Distributed fan-out timed out waiting for shard responses.
    DISTRIBUTION_TIMEOUT = 1177,

    /// The shard registry is empty or contains no usable shard backends.
    DISTRIBUTION_NO_SHARDS = 1178,

    /// Shard rebalancing is in progress; results may be inconsistent.
    /// The caller should retry after rebalancing completes.
    DISTRIBUTION_REBALANCING = 1179,
};

// ============================================================================
// § 2  IndexError — structured exception
// ============================================================================

/**
 * @brief Structured exception for Index module errors.
 *
 * Wraps `IndexErrorCode` with a human-readable message and an optional
 * correlation ID (from `AnnQueryContext::correlation_id`).
 *
 * **Design contract:**
 * - `IndexError` is thrown only for non-recoverable conditions that callers
 *   cannot handle via a `Result<T>` path.
 * - All public AnnFrontdoor methods prefer returning degraded results over
 *   throwing `IndexError`; exceptions indicate programming errors or
 *   unrecoverable backend failures.
 */
class IndexError : public std::runtime_error {
public:
    /**
     * @brief Construct with an error code and descriptive message.
     * @param code       Error category from `IndexErrorCode`.
     * @param message    Human-readable description of the failure.
     */
    IndexError(IndexErrorCode code, const std::string& message)
        : std::runtime_error(message), code_(code) {}

    /**
     * @brief Construct with an error code, message, and correlation ID.
     * @param code           Error category.
     * @param message        Human-readable description.
     * @param correlation_id Request correlation ID for log correlation.
     */
    IndexError(IndexErrorCode code, const std::string& message,
               std::string correlation_id)
        : std::runtime_error(message),
          code_(code),
          correlation_id_(std::move(correlation_id)) {}

    /// @return The structured error code.
    [[nodiscard]] IndexErrorCode code() const noexcept { return code_; }

    /// @return The correlation ID, or empty string if not set.
    [[nodiscard]] const std::string& correlation_id() const noexcept {
        return correlation_id_;
    }

    /// @return The numeric value of the error code.
    [[nodiscard]] uint32_t code_value() const noexcept {
        return static_cast<uint32_t>(code_);
    }

private:
    IndexErrorCode code_;
    std::string    correlation_id_;
};

} // namespace themis::index
