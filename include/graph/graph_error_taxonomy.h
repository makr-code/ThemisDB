/**
 * @file graph_error_taxonomy.h
 * @brief Graph Module Error Taxonomy and Contract Definitions
 *
 * Defines the canonical error model for all Graph module components,
 * including error codes, classes, and recovery semantics.
 *
 * @version 1.0.0
 * @date 2026-08-01
 * @author Graph Module Hardening Phase 1
 *
 * Error Classification:
 * - DENIAL: Operation cannot proceed (invalid input, constraint violation)
 * - FALLBACK: Acceleration unavailable, fallback to CPU required
 * - REASONING_CONFLICT: Knowledge graph inference conflict detected
 */

#pragma once

#include <string>
#include <string_view>
#include <cstdint>
#include <ostream>

namespace themis {
namespace graph {

// ─────────────────────────────────────────────────────────────────────────────
// Error Categories (Phase 1 Taxonomy)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Graph module error category enumeration.
 *
 * Classifies errors into three semantic categories for routing recovery behavior:
 *
 * - **DENIAL**: Operation cannot proceed. Invalid input, constraint violation,
 *   or resource exhaustion. Recovery: caller must fix preconditions.
 *
 * - **FALLBACK**: Acceleration path unavailable (GPU, distributed). Operation
 *   can retry on CPU. Recovery: automatic fallback or retry.
 *
 * - **REASONING_CONFLICT**: Knowledge graph inference detected contradiction
 *   or unsatisfiable constraint set. Recovery: operator intervention required.
 */
enum class ErrorCategory : uint8_t {
    /// Operation cannot proceed; precondition violated
    DENIAL = 0,
    /// Acceleration unavailable; CPU fallback possible
    FALLBACK = 1,
    /// Reasoning conflict; inference contradiction
    REASONING_CONFLICT = 2
};

/**
 * @brief Graph module error codes (Phase 1 frozen).
 *
 * Each code specifies category, scope, and recovery semantics.
 * Format: GRAPHxxyyzz where xx=component, yy=category, zz=sequence.
 *
 * Invariant: Do not add codes without Phase 1 contract review.
 */
enum class GraphErrorCode : uint32_t {
    // Sentinel
    SUCCESS = 0,

    // ─────────────────────────────────────────────────────────────────────────
    // QUERY OPTIMIZER Errors (GRAPH01)
    // ─────────────────────────────────────────────────────────────────────────

    /// DENIAL: Null or invalid query AST passed to optimizer
    OPT_INVALID_QUERY_AST = 0x01010001,
    /// DENIAL: Query pattern not supported by current optimizer version
    OPT_UNSUPPORTED_QUERY_PATTERN = 0x01010002,
    /// DENIAL: Graph statistics unavailable or stale
    OPT_MISSING_GRAPH_STATISTICS = 0x01010003,
    /// DENIAL: Cost model calculation overflow
    OPT_COST_CALC_OVERFLOW = 0x01010004,
    /// FALLBACK: GPU acceleration unavailable; CPU fallback required
    OPT_GPU_UNAVAILABLE = 0x01020001,
    /// FALLBACK: Distributed query planner offline
    OPT_DISTRIBUTED_PLANNER_OFFLINE = 0x01020002,
    /// REASONING_CONFLICT: Constraint set detected as unsatisfiable
    OPT_UNSATISFIABLE_CONSTRAINTS = 0x01030001,

    // ─────────────────────────────────────────────────────────────────────────
    // TRAVERSAL & EXECUTION Errors (GRAPH02)
    // ─────────────────────────────────────────────────────────────────────────

    /// DENIAL: Source or target vertex not found in graph
    TRAV_VERTEX_NOT_FOUND = 0x02010001,
    /// DENIAL: Invalid edge type or property filter
    TRAV_INVALID_EDGE_FILTER = 0x02010002,
    /// DENIAL: Frontier exceeded maximum size limit
    TRAV_FRONTIER_OVERFLOW = 0x02010003,
    /// DENIAL: Path depth exceeds configured maximum
    TRAV_MAX_DEPTH_EXCEEDED = 0x02010004,
    /// DENIAL: Timeout elapsed during traversal
    TRAV_TIMEOUT = 0x02010005,
    /// DENIAL: Parallel traversal thread creation failed
    TRAV_THREAD_CREATION_FAILED = 0x02010006,
    /// FALLBACK: GPU memory exhausted; CPU fallback required
    TRAV_GPU_MEMORY_EXHAUSTED = 0x02020001,
    /// FALLBACK: GPU kernel launch failed; CPU fallback required
    TRAV_GPU_KERNEL_FAILED = 0x02020002,
    /// FALLBACK: Distributed shard unavailable; retry or use cache
    TRAV_SHARD_UNAVAILABLE = 0x02020003,
    /// REASONING_CONFLICT: Constraint cannot be satisfied during traversal
    TRAV_CONSTRAINT_VIOLATION = 0x02030001,

    // ─────────────────────────────────────────────────────────────────────────
    // SEMANTIC & REASONING Errors (GRAPH03)
    // ─────────────────────────────────────────────────────────────────────────

    /// DENIAL: Invalid inference rule syntax
    REASON_INVALID_RULE_SYNTAX = 0x03010001,
    /// DENIAL: Ontology not loaded or corrupted
    REASON_ONTOLOGY_LOAD_FAILED = 0x03010002,
    /// DENIAL: Rule variable binding failed (no matching facts)
    REASON_BINDING_FAILED = 0x03010003,
    /// REASONING_CONFLICT: Inferred fact contradicts known fact
    REASON_INFERENCE_CONFLICT = 0x03030001,
    /// REASONING_CONFLICT: Cyclic rule dependency detected
    REASON_CYCLIC_RULE = 0x03030002,

    // ─────────────────────────────────────────────────────────────────────────
    // TENSOR UTILITIES Errors (GRAPH04)
    // ─────────────────────────────────────────────────────────────────────────

    /// DENIAL: Invalid tensor dimensions or shape mismatch
    TENSOR_INVALID_SHAPE = 0x04010001,
    /// DENIAL: Fingerprint computation failed (NaN or inf)
    TENSOR_FINGERPRINT_FAILED = 0x04010002,
    /// DENIAL: Graph deduplication threshold out of range
    TENSOR_INVALID_THRESHOLD = 0x04010003,
    /// FALLBACK: GPU tensor operation unavailable
    TENSOR_GPU_OP_UNAVAILABLE = 0x04020001,

    // ─────────────────────────────────────────────────────────────────────────
    // DISTRIBUTED GRAPH Errors (GRAPH05)
    // ─────────────────────────────────────────────────────────────────────────

    /// DENIAL: Shard configuration invalid or incomplete
    DIST_INVALID_SHARD_CONFIG = 0x05010001,
    /// DENIAL: Cross-shard merge operation failed (incompatible results)
    DIST_MERGE_FAILED = 0x05010002,
    /// DENIAL: Vertex not hashed to any shard
    DIST_VERTEX_UNHASHED = 0x05010003,
    /// FALLBACK: Shard peer offline; operation deferred
    DIST_SHARD_PEER_OFFLINE = 0x05020001,
    /// FALLBACK: RPC timeout on cross-shard communication
    DIST_RPC_TIMEOUT = 0x05020002,

    // ─────────────────────────────────────────────────────────────────────────
    // RESOURCE & PLANNING Errors (GRAPH06)
    // ─────────────────────────────────────────────────────────────────────────

    /// DENIAL: Plan cache full; LRU eviction needed
    CACHE_PLAN_CACHE_FULL = 0x06010001,
    /// DENIAL: Resource pool exhausted
    POOL_RESOURCE_EXHAUSTED = 0x06010002,
    /// DENIAL: Load balancing decision failed
    LB_DECISION_FAILED = 0x06010003,
    /// FALLBACK: Cache miss on frequently-accessed plan
    CACHE_MISS = 0x06020001,

    // ─────────────────────────────────────────────────────────────────────────
    // GENERIC / INFRASTRUCTURE Errors (GRAPH07)
    // ─────────────────────────────────────────────────────────────────────────

    /// DENIAL: Unknown or unclassified error
    GENERIC_ERROR = 0x07010001,
    /// DENIAL: Not implemented (stub)
    NOT_IMPLEMENTED = 0x07010002,
    /// DENIAL: Internal consistency check failed
    INTERNAL_INVARIANT_FAILED = 0x07010003,
    /// FALLBACK: Temporary system resource unavailable
    SYSTEM_RESOURCE_UNAVAILABLE = 0x07020001,
};

// ─────────────────────────────────────────────────────────────────────────────
// Error Category Mapping (Phase 1 frozen)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Maps error code to its category.
 *
 * @param code The error code to classify
 * @return Category enum indicating error class (DENIAL, FALLBACK, REASONING_CONFLICT)
 *
 * Thread-safe: pure function, no state mutation.
 */
[[nodiscard]] constexpr ErrorCategory
getErrorCategory(GraphErrorCode code) noexcept {
    const auto c = static_cast<uint32_t>(code);
    // Error code layout: 0xMMCCSSSS (MM=module, CC=category, SSSS=sequence)
    const auto category_byte = (c >> 16) & 0xFF;
    if (category_byte == 0x02) return ErrorCategory::FALLBACK;
    if (category_byte == 0x03) return ErrorCategory::REASONING_CONFLICT;
    if (category_byte == 0x01) return ErrorCategory::DENIAL;
    return ErrorCategory::DENIAL; // Default to DENIAL for unknown codes
}

/**
 * @brief Checks if error code indicates a recoverable fallback condition.
 *
 * @param code Error code to test
 * @return true if error is FALLBACK category (GPU, distributed, cache unavailable)
 *
 * Thread-safe: pure function.
 */
[[nodiscard]] constexpr bool
isRecoverableFallback(GraphErrorCode code) noexcept {
    return getErrorCategory(code) == ErrorCategory::FALLBACK;
}

/**
 * @brief Checks if error code represents a reasoning conflict.
 *
 * @param code Error code to test
 * @return true if error is REASONING_CONFLICT category
 *
 * Thread-safe: pure function.
 */
[[nodiscard]] constexpr bool
isReasoningConflict(GraphErrorCode code) noexcept {
    return getErrorCategory(code) == ErrorCategory::REASONING_CONFLICT;
}

// ─────────────────────────────────────────────────────────────────────────────
// Error Description & Diagnostics
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Returns human-readable error description.
 *
 * @param code The error code
 * @return String describing error semantics and recovery action
 *
 * Thread-safe: uses static lookup table.
 */
[[nodiscard]] std::string_view
getErrorDescription(GraphErrorCode code) noexcept;

/**
 * @brief Returns error code as hex string (for logging).
 *
 * @param code The error code
 * @return Hex representation (e.g., "0x01010001")
 *
 * Thread-safe: pure function.
 */
[[nodiscard]] std::string
getErrorCodeHex(GraphErrorCode code) noexcept;

/**
 * @brief Error context helper for diagnostics.
 *
 * Captures error code, context, and recovery recommendation.
 * Used by fallback_incident reporting and operator diagnostics.
 */
struct ErrorContext {
    GraphErrorCode code = GraphErrorCode::SUCCESS;
    std::string component;    ///< Component where error originated (e.g., "optimizer")
    std::string operation;    ///< Operation being attempted (e.g., "plan_generation")
    std::string context;      ///< Additional context (query ID, vertex ID, etc.)
    std::string recovery_hint;  ///< Operator-facing recovery suggestion

    /**
     * @brief Constructs recovery recommendation based on error category.
     * @return String suitable for logging or UI display
     */
    [[nodiscard]] std::string
    getRecoveryRecommendation() const noexcept;

    /**
     * @brief Formats diagnostic message for operator debugging.
     * @return Structured diagnostic string
     */
    [[nodiscard]] std::string
    formatDiagnostic() const noexcept;
};

// ─────────────────────────────────────────────────────────────────────────────
// Phase 1 Contract Invariants
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @defgroup GRAPH_ERROR_INVARIANTS Phase 1 Error Handling Contract
 *
 * @par Invariant 1: Error Code Stability
 * Once a code is assigned in Phase 1, its numeric value MUST NOT change.
 * This ensures cross-version log compatibility and metrics aggregation.
 *
 * @par Invariant 2: Category Stability
 * Error category (DENIAL/FALLBACK/REASONING_CONFLICT) MUST NOT change
 * for existing codes. Adding new codes in a different category requires
 * a new code; reassigning an old code is forbidden.
 *
 * @par Invariant 3: Determinism
 * Error classification (getErrorCategory, isRecoverableFallback, etc.)
 * MUST produce identical results on any platform. No runtime variation.
 *
 * @par Invariant 4: Completeness
 * Every error code returned by graph module functions MUST be classified
 * in this taxonomy. Unclassified errors MUST be mapped to GENERIC_ERROR.
 *
 * @par Invariant 5: Thread Safety
 * All error classification functions are thread-safe and lock-free.
 * Error context construction is thread-safe (value semantics).
 *
 * @{
 */

/**
 * @brief Validates error code against Phase 1 taxonomy.
 *
 * @param code Code to validate
 * @return true if code is a known Phase 1 error code
 *
 * Use for defensive programming; unknown codes should be treated as
 * GENERIC_ERROR and logged for debugging.
 *
 * Thread-safe: pure function.
 */
[[nodiscard]] bool
isValidErrorCode(GraphErrorCode code) noexcept;

/** @} */ // end of GRAPH_ERROR_INVARIANTS

} // namespace graph
} // namespace themis
