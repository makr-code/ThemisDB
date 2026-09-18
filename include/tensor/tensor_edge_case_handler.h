/**
 * @file tensor_edge_case_handler.h
 * @brief Deterministic edge case handling for tensor fingerprint graph and ingestion bridge.
 *
 * @version 1.0.0
 * @date 2026-09-01
 *
 * This module implements robust, fail-safe handling for 10-15 critical edge scenarios
 * in the tensor fingerprint graph and ingestion bridge, ensuring deterministic behavior
 * under all failure conditions with explicit error codes and recovery paths.
 *
 * ### Supported Edge Scenarios
 *
 * **Graph Operations (TEDGE-01..06)**:
 * - TEDGE-01: Invalid adapter references (missing keys in graph)
 * - TEDGE-02: Out-of-bounds index accesses (k > graph size)
 * - TEDGE-03: Stale fingerprints (NaN/Inf/invalid norms)
 * - TEDGE-04: Self-similarity computation failures
 * - TEDGE-05: Cross-adapter comparison with null/empty trains
 * - TEDGE-06: Concurrent modification during traversal
 *
 * **Bridge Operations (TEDGE-07..10)**:
 * - TEDGE-07: Bridge routing failures under load
 * - TEDGE-08: Adapter communication timeouts/disconnects
 * - TEDGE-09: Invalid decomposition results (rank overflow, NaN)
 * - TEDGE-10: Kappa-gate threshold violations
 *
 * **Graph Export/Replay (TEDGE-11..13)**:
 * - TEDGE-11: Export serialization failures
 * - TEDGE-12: Replay deserialization with corrupted fingerprints
 * - TEDGE-13: Partial graph loss recovery
 *
 * **Memory Pressure (TEDGE-14..15)**:
 * - TEDGE-14: Out-of-memory during fingerprint computation
 * - TEDGE-15: Memory exhaustion under concurrent access
 *
 * ### Error Code Mapping
 *
 * All errors map to ERR_TENSOR_* codes from include/utils/error_registry.h:
 * - ERR_TENSOR_GRAPH_INVALID_SELF_IP (9510)
 * - ERR_TENSOR_GRAPH_OTHER_TRAIN_NOT_FOUND (9513)
 * - ERR_TENSOR_INDEX_LOOKUP_FAILED (9521)
 * - ERR_TENSOR_ADAPTER_NOT_FOUND (9531)
 * - ERR_TENSOR_FINGERPRINT_COMPUTATION_FAILED (9540)
 * - ERR_TENSOR_CONCURRENT_MODIFICATION (9570)
 * - And others as documented
 *
 * ### Recovery Strategies
 *
 * Each error scenario has a documented recovery path:
 * - **Retry**: Transient failures (network timeouts, temporary unavailability)
 * - **Fallback**: Use cached/degraded results when primary path fails
 * - **Graceful Degradation**: Return partial results with error indication
 * - **Fail-Closed**: Explicitly reject operation with clear error code
 *
 * ### Usage Example
 *
 * ```cpp
 * #include "tensor/tensor_edge_case_handler.h"
 *
 * themis::tensor::TensorEdgeCaseHandler handler;
 *
 * // Handle invalid adapter reference
 * auto result = handler.handleInvalidAdapterReference(
 *     "missing_adapter_key",
 *     "query_adapter_key");
 * if (!result.success) {
 *     std::cerr << "Error: " << result.error_message
 *               << " (code=" << result.error_code << ")" << std::endl;
 *     // Implement recovery strategy (retry, fallback, etc.)
 * }
 * ```
 *
 * ### Thread Safety
 *
 * This handler is NOT thread-safe. Wrap with a shared_mutex if using in
 * concurrent contexts:
 *
 * ```cpp
 * std::shared_mutex handler_mu;
 * TensorEdgeCaseHandler handler;
 *
 * {
 *     std::shared_lock lock(handler_mu);
 *     auto result = handler.handleInvalidAdapterReference(...);
 * }
 * ```
 *
 * ### RAII Safety
 *
 * All resources are managed via std::unique_ptr or std::shared_ptr.
 * No raw new/delete used in public APIs.
 *
 * @see include/utils/error_registry.h
 * @see src/tensor/tensor_fingerprint_graph.cpp
 * @see src/tensor/tensor_ingestion_bridge.cpp
 */

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <cstdint>

namespace themis {
namespace tensor {

// Forward declarations
class TensorFingerprintGraph;
class TensorIngestionBridge;

// ============================================================================
// EdgeCaseResult — Standard return value for edge case handlers
// ============================================================================

/**
 * @brief Result structure for edge case handling operations.
 *
 * Contains success flag, error code, and recovery action information.
 */
struct EdgeCaseResult {
    /// Whether the operation succeeded or failed.
    bool success = false;

    /// THEMIS error code (from include/utils/error_registry.h).
    int error_code = 0;

    /// Human-readable error message for logging.
    std::string error_message;

    /// Recommended recovery action ("retry", "fallback", "degrade", "fail-closed").
    std::string recovery_action = "fail-closed";

    /// Whether the error is recoverable (true) or permanent (false).
    bool is_recoverable = false;

    /// Technical details (stack trace, context, etc.).
    std::string technical_details;

    /// Success factory method.
    static EdgeCaseResult Ok(const std::string& message = "") noexcept;

    /// Failure factory method.
    static EdgeCaseResult Error(int code, const std::string& message,
                                const std::string& recovery_action = "fail-closed",
                                bool recoverable = false) noexcept;
};

// ============================================================================
// TensorEdgeCaseHandler — Central handler for all edge scenarios
// ============================================================================

/**
 * @brief Centralized handler for deterministic edge case scenarios.
 *
 * This class manages all edge case scenarios for the tensor module,
 * ensuring deterministic behavior, explicit error codes, and well-defined
 * recovery paths.
 */
class TensorEdgeCaseHandler {
public:
    /// Constructor: initializes the handler with default recovery strategies.
    TensorEdgeCaseHandler() noexcept;

    /// Destructor: cleans up all resources.
    ~TensorEdgeCaseHandler() noexcept = default;

    // ─────────────────────────────────────────────────────────────────────
    // Graph Operations: TEDGE-01..06
    // ─────────────────────────────────────────────────────────────────────

    /**
     * @brief Handle invalid adapter reference in graph.
     *
     * **Scenario (TEDGE-01)**: Caller references an adapter key that does not
     * exist in the graph. This is a common failure when adapters are removed
     * or the graph becomes stale.
     *
     * **Error Code**: ERR_TENSOR_ADAPTER_NOT_FOUND (9531)
     *
     * **Recovery**: Fail-closed with clear error indication. Caller must
     * verify adapter exists before calling graph methods.
     *
     * @param adapter_key    The invalid adapter key.
     * @param context_op     Context operation ("findSimilar", "entry", etc.).
     * @return Result with error code 9531 and fail-closed recovery.
     */
    [[nodiscard]] EdgeCaseResult handleInvalidAdapterReference(
        const std::string& adapter_key,
        const std::string& context_op = "graph_query") noexcept;

    /**
     * @brief Handle out-of-bounds index access.
     *
     * **Scenario (TEDGE-02)**: Caller requests top-k results where k > graph size.
     * Requesting top-1000 adapters when graph has only 50 adapters.
     *
     * **Error Code**: ERR_TENSOR_INDEX_LOOKUP_FAILED (9521)
     *
     * **Recovery**: Graceful degradation. Return top-min(k, size) results.
     *
     * @param requested_k     The requested number of results.
     * @param actual_size     Actual number of adapters in graph.
     * @return Result with error code, recovery="degrade".
     */
    [[nodiscard]] EdgeCaseResult handleOutOfBoundsIndex(
        std::size_t requested_k,
        std::size_t actual_size) noexcept;

    /**
     * @brief Handle stale/invalid fingerprints (NaN/Inf/invalid norm).
     *
     * **Scenario (TEDGE-03)**: Fingerprint entry has NaN, Inf, or invalid norm.
     * This indicates data corruption or computation failure upstream.
     *
     * **Error Code**: ERR_TENSOR_GRAPH_INVALID_SELF_IP (9510)
     *
     * **Recovery**: Fail-closed with data corruption indication. Requires
     * fingerprint recomputation or graph reconstruction.
     *
     * @param adapter_key    The adapter with invalid fingerprint.
     * @param norm_value     The invalid norm value (NaN/Inf).
     * @return Result with error code 9510 and fail-closed recovery.
     */
    [[nodiscard]] EdgeCaseResult handleStaleFingerprint(
        const std::string& adapter_key,
        float norm_value) noexcept;

    /**
     * @brief Handle self-similarity computation failure.
     *
     * **Scenario (TEDGE-04)**: Computing adapter's similarity to itself should
     * yield 1.0, but computation fails or returns invalid value.
     *
     * **Error Code**: ERR_TENSOR_GRAPH_INVALID_SELF_IP (9510)
     *
     * **Recovery**: Retry with validation, then fail-closed if persistent.
     *
     * @param adapter_key    The adapter being compared to itself.
     * @param computed_score The invalid similarity score.
     * @return Result with error code and retry/fail-closed recovery.
     */
    [[nodiscard]] EdgeCaseResult handleSelfSimilarityFailure(
        const std::string& adapter_key,
        float computed_score) noexcept;

    /**
     * @brief Handle cross-adapter comparison with null/empty trains.
     *
     * **Scenario (TEDGE-05)**: Attempting to compare adapters where one or
     * both have empty or null TT-train representations.
     *
     * **Error Code**: ERR_TENSOR_GRAPH_OTHER_TRAIN_NOT_FOUND (9513)
     *
     * **Recovery**: Fail-closed. Trains must be non-empty for comparison.
     *
     * @param key_a          First adapter key.
     * @param key_b          Second adapter key.
     * @param null_train_key Which adapter has null/empty train (if known).
     * @return Result with error code 9513 and fail-closed recovery.
     */
    [[nodiscard]] EdgeCaseResult handleNullTrainComparison(
        const std::string& key_a,
        const std::string& key_b,
        const std::string& null_train_key = "") noexcept;

    /**
     * @brief Handle concurrent modification detected during traversal.
     *
     * **Scenario (TEDGE-06)**: Graph structure changes during an ongoing query
     * (adapter added/removed while findSimilar is executing).
     *
     * **Error Code**: ERR_TENSOR_CONCURRENT_MODIFICATION (9570)
     *
     * **Recovery**: Retry with full read-lock, or fail-closed if contention high.
     *
     * @param operation      Operation being performed ("findSimilar", etc.).
     * @param affected_key   Adapter key that was modified.
     * @return Result with error code 9570 and retry/fail-closed recovery.
     */
    [[nodiscard]] EdgeCaseResult handleConcurrentModification(
        const std::string& operation,
        const std::string& affected_key = "") noexcept;

    // ─────────────────────────────────────────────────────────────────────
    // Bridge Operations: TEDGE-07..10
    // ─────────────────────────────────────────────────────────────────────

    /**
     * @brief Handle bridge routing failures under load.
     *
     * **Scenario (TEDGE-07)**: Ingestion bridge cannot route request to
     * appropriate decomposer (queue full, all decomposers busy).
     *
     * **Error Code**: ERR_TENSOR_INDEX_ROUTING_FAILED (9522)
     *
     * **Recovery**: Retry with exponential backoff, or queue for later.
     *
     * @param load_level     Current system load (0.0..1.0+).
     * @param queue_depth    Current routing queue depth.
     * @return Result with error code 9522 and retry recovery.
     */
    [[nodiscard]] EdgeCaseResult handleBridgeRoutingFailure(
        double load_level,
        std::size_t queue_depth) noexcept;

    /**
     * @brief Handle adapter communication timeout/disconnect.
     *
     * **Scenario (TEDGE-08)**: Adapter service is unreachable, network timeout,
     * or connection drop during decomposition request.
     *
     * **Error Code**: ERR_TENSOR_ADAPTER_COMMUNICATION_ERROR (9532)
     *
     * **Recovery**: Retry with backoff, or fallback to cached/degraded results.
     *
     * @param adapter_key    The unreachable adapter.
     * @param timeout_ms     Timeout duration in milliseconds.
     * @return Result with error code 9532 and retry/fallback recovery.
     */
    [[nodiscard]] EdgeCaseResult handleAdapterCommunicationFailure(
        const std::string& adapter_key,
        uint32_t timeout_ms) noexcept;

    /**
     * @brief Handle invalid decomposition results.
     *
     * **Scenario (TEDGE-09)**: Bridge receives decomposition result with
     * invalid data: rank overflow, NaN in cores, dimension mismatch.
     *
     * **Error Code**: ERR_TENSOR_FINGERPRINT_COMPUTATION_FAILED (9540)
     *
     * **Recovery**: Reject result, retry decomposition with different params.
     *
     * @param chunk_id       Chunk being decomposed.
     * @param error_detail   Description of invalid result.
     * @return Result with error code 9540 and retry recovery.
     */
    [[nodiscard]] EdgeCaseResult handleInvalidDecompositionResult(
        const std::string& chunk_id,
        const std::string& error_detail) noexcept;

    /**
     * @brief Handle kappa-gate threshold violation.
     *
     * **Scenario (TEDGE-10)**: κ-gate (kappa gate) estimates that compression
     * won't achieve min_kappa threshold, but caller ignores and forces decompose.
     *
     * **Error Code**: ERR_TENSOR_FINGERPRINT_COMPUTATION_FAILED (9540)
     *
     * **Recovery**: Graceful degradation or fail-closed based on policy.
     *
     * @param embedding_dim  Embedding dimension.
     * @param estimated_kappa Estimated compression ratio.
     * @param min_kappa      Minimum required ratio.
     * @return Result with error code and degrade/fail-closed recovery.
     */
    [[nodiscard]] EdgeCaseResult handleKappaGateViolation(
        std::size_t embedding_dim,
        double estimated_kappa,
        double min_kappa) noexcept;

    // ─────────────────────────────────────────────────────────────────────
    // Graph Export/Replay: TEDGE-11..13
    // ─────────────────────────────────────────────────────────────────────

    /**
     * @brief Handle export serialization failures.
     *
     * **Scenario (TEDGE-11)**: Serializing graph to file/buffer fails:
     * I/O error, permission denied, or serializer exception.
     *
     * **Error Code**: ERR_TENSOR_PERSISTENCE_FAILED (9561)
     *
     * **Recovery**: Retry with different path, or fail-closed.
     *
     * @param export_path    Target export path.
     * @param error_detail   System error message.
     * @return Result with error code 9561 and retry/fail-closed recovery.
     */
    [[nodiscard]] EdgeCaseResult handleExportSerializationFailure(
        const std::string& export_path,
        const std::string& error_detail) noexcept;

    /**
     * @brief Handle corrupted fingerprint during replay.
     *
     * **Scenario (TEDGE-12)**: Replaying graph from persisted state, but
     * fingerprint data is corrupted: deserialization fails, checksums mismatch.
     *
     * **Error Code**: ERR_TENSOR_PERSISTENCE_FAILED (9561)
     *
     * **Recovery**: Skip corrupt entry, or abort replay with full rebuild.
     *
     * @param adapter_key    The corrupt entry's adapter key.
     * @param corruption_type Description ("invalid_checksum", "deserialization_error", etc.).
     * @return Result with error code 9561 and skip/rebuild recovery.
     */
    [[nodiscard]] EdgeCaseResult handleReplayDeserialization(
        const std::string& adapter_key,
        const std::string& corruption_type) noexcept;

    /**
     * @brief Handle partial graph loss recovery.
     *
     * **Scenario (TEDGE-13)**: Detecting that graph persistence is incomplete:
     * some entries missing, or state file truncated/partial.
     *
     * **Error Code**: ERR_TENSOR_PERSISTENCE_FAILED (9561)
     *
     * **Recovery**: Partial recovery (load what's available) with warning,
     * or full reconstruction from source.
     *
     * @param total_entries  Expected number of graph entries.
     * @param recovered_entries Actual entries successfully recovered.
     * @return Result with error code 9561 and degrade/rebuild recovery.
     */
    [[nodiscard]] EdgeCaseResult handlePartialGraphLossRecovery(
        std::size_t total_entries,
        std::size_t recovered_entries) noexcept;

    // ─────────────────────────────────────────────────────────────────────
    // Memory Pressure: TEDGE-14..15
    // ─────────────────────────────────────────────────────────────────────

    /**
     * @brief Handle out-of-memory during fingerprint computation.
     *
     * **Scenario (TEDGE-14)**: Allocating fingerprint structures, TT-cores,
     * or temporary buffers fails due to insufficient memory.
     *
     * **Error Code**: ERR_TENSOR_FINGERPRINT_COMPUTATION_FAILED (9540)
     *
     * **Recovery**: Fail-closed with memory exhaustion indication. Requires
     * freeing memory or rejecting new requests.
     *
     * @param requested_bytes Bytes that could not be allocated.
     * @param operation       Operation that failed ("fingerprint_compute", etc.).
     * @return Result with error code 9540 and fail-closed recovery.
     */
    [[nodiscard]] EdgeCaseResult handleOutOfMemoryDuringComputation(
        std::size_t requested_bytes,
        const std::string& operation) noexcept;

    /**
     * @brief Handle memory exhaustion under concurrent access.
     *
     * **Scenario (TEDGE-15)**: During high concurrency (many threads
     * accessing graph), memory growth becomes unbounded or exceeds limit.
     *
     * **Error Code**: ERR_TENSOR_LOCK_ACQUISITION_FAILED (9571)
     *
     * **Recovery**: Throttle new requests, trigger cleanup, or fail-closed.
     *
     * @param current_memory_bytes Current memory usage.
     * @param max_memory_bytes     Memory limit.
     * @param concurrent_threads   Number of concurrent threads.
     * @return Result with error code 9571 and throttle/fail-closed recovery.
     */
    [[nodiscard]] EdgeCaseResult handleConcurrentMemoryExhaustion(
        std::size_t current_memory_bytes,
        std::size_t max_memory_bytes,
        std::size_t concurrent_threads) noexcept;

    // ─────────────────────────────────────────────────────────────────────
    // Diagnostics & Observability
    // ─────────────────────────────────────────────────────────────────────

    /**
     * @brief Enable detailed error diagnostics for debugging.
     *
     * When enabled, all error handlers emit detailed diagnostics including
     * stack traces and context snapshots.
     *
     * @param enable If true, enable diagnostics; if false, use minimal logging.
     */
    void setDetailedDiagnostics(bool enable) noexcept;

    /**
     * @brief Get error statistics.
     *
     * @return Struct containing counters for each edge case handled.
     */
    struct EdgeCaseStats {
        std::size_t invalid_adapter_refs = 0;
        std::size_t out_of_bounds_accesses = 0;
        std::size_t stale_fingerprints = 0;
        std::size_t self_similarity_failures = 0;
        std::size_t null_train_comparisons = 0;
        std::size_t concurrent_modifications = 0;
        std::size_t bridge_routing_failures = 0;
        std::size_t adapter_comm_failures = 0;
        std::size_t invalid_decompositions = 0;
        std::size_t kappa_gate_violations = 0;
        std::size_t export_serialization_failures = 0;
        std::size_t replay_deserialization_failures = 0;
        std::size_t partial_graph_losses = 0;
        std::size_t oom_during_computation = 0;
        std::size_t concurrent_memory_exhaustion = 0;
    };

    /// Get current error statistics.
    [[nodiscard]] EdgeCaseStats getStats() const noexcept;

    /// Reset error statistics.
    void resetStats() noexcept;

private:
    // ─────────────────────────────────────────────────────────────────────
    // Private state
    // ─────────────────────────────────────────────────────────────────────

    bool detailed_diagnostics_ = false;
    EdgeCaseStats stats_;

    // ─────────────────────────────────────────────────────────────────────
    // Private helper methods
    // ─────────────────────────────────────────────────────────────────────

    /// Emit diagnostic event for error tracking.
    void emitDiagnostic(const std::string& scenario, const EdgeCaseResult& result) const noexcept;

    /// Format error message with context.
    std::string formatErrorMessage(const std::string& base_message,
                                    const std::string& context) const noexcept;

    /// Determine if error is recoverable based on type.
    bool isRecoverable(int error_code) const noexcept;
};

}  // namespace tensor
}  // namespace themis
