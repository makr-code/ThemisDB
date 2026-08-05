/**
 * @file subagent_coordinator.h
 * @brief Coordinator for fan-out parallel inference across multiple subagents
 *        with partial-failure handling and merge strategies.
 *
 * @note **Orchestration Layer**: Coordinates parallel inference across multiple
 *       subagents with merge strategies, partial-failure recovery, and result
 *       aggregation.
 *
 * ## Purpose
 *
 * SubagentCoordinator enables parallel inference across multiple independent
 * subagents while handling partial failures gracefully. It:
 *   1. Distributes inference requests to multiple subagents (fan-out)
 *   2. Collects results from all subagents (fan-in)
 *   3. Handles partial failures (some subagents fail, others succeed)
 *   4. Applies merge strategies (first-win, majority-vote, best-score)
 *   5. Provides detailed failure diagnostics
 *
 * ## Architecture
 *
 * The coordinator follows a fork-join pattern:
 *   1. Request arrives for a set of subagents
 *   2. Coordinator submits request to all subagents asynchronously (fan-out)
 *   3. Each subagent infers independently and returns result
 *   4. Coordinator collects results and merges using strategy (fan-in)
 *   5. Final result is returned to caller (with partial-failure metadata)
 *
 * Subagents are isolated: failures in one do not affect others.
 *
 * ## Merge Strategies
 *
 * - **FirstWin**: Return first successful result (lowest latency)
 * - **MajorityVote**: Aggregate results by consensus (requires structured output)
 * - **BestScore**: Return result with highest confidence/quality score
 * - **Ensemble**: Combine results from all subagents (e.g., average scores)
 *
 * ## Thread Safety
 *
 * All coordinator methods are thread-safe. The coordinator itself uses
 * internal locks; subagents are accessed safely via future continuations.
 */

#pragma once

#include "llm/subagent_factory.h"
#include "llm/llm_correlation_context.h"

#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <optional>
#include <variant>

namespace themis {
namespace llm {

// ============================================================================
// § 1  Merge Strategies and Results
// ============================================================================

/**
 * @brief Merge strategy for aggregating results from multiple subagents.
 */
enum class SubagentMergeStrategy {
    /// Return first successful result (lowest latency).
    FIRST_WIN = 0,

    /// Require all subagents to succeed; fail if any fails.
    ALL_SUCCEED = 1,

    /// Aggregate via majority vote (requires structured output).
    MAJORITY_VOTE = 2,

    /// Return result with highest confidence/quality score.
    BEST_SCORE = 3,

    /// Combine results from all subagents (average, concatenate, etc.).
    ENSEMBLE = 4,

    /// Custom merge function (caller-provided).
    CUSTOM = 5,
};

/**
 * @brief Result from a single subagent in a coordinated inference.
 */
struct SubagentCoordinatorResult {
    /// Subagent ID that produced this result.
    std::string subagent_id;

    /// Success flag.
    bool success = false;

    /// Generated output (if successful).
    std::string output;

    /// Error message (if failed).
    std::string error;

    /// Tokens consumed by this subagent.
    size_t tokens_consumed = 0;

    /// Latency for this subagent (ms).
    int latency_ms = 0;

    /// Optional quality score (for BEST_SCORE merge).
    /// Range [0, 1]; higher = better. May be unset (0.0).
    float quality_score = 0.0f;

    /// Trace ID for observability.
    std::string trace_id;
};

/**
 * @brief Aggregated result from SubagentCoordinator.
 */
struct SubagentCoordinatorAggregateResult {
    /// Overall success: true if merge strategy was satisfied.
    bool success = false;

    /// Merged/final output.
    std::string merged_output;

    /// Human-readable summary of result.
    std::string summary;

    /// Per-subagent results (in same order as request).
    std::vector<SubagentCoordinatorResult> per_subagent_results;

    /// Number of successful subagent inferences.
    size_t num_successful = 0;

    /// Number of failed subagent inferences.
    size_t num_failed = 0;

    /// Total tokens consumed across all subagents.
    size_t total_tokens_consumed = 0;

    /// Wall-clock time for fan-out/fan-in (ms).
    int total_latency_ms = 0;

    /// Merge strategy used.
    SubagentMergeStrategy strategy = SubagentMergeStrategy::FIRST_WIN;
};

/**
 * @brief Custom merge function signature.
 *
 * Called by the coordinator to merge results when strategy is CUSTOM.
 *
 * @param results Per-subagent results (in request order).
 * @return Merged output string, or error string.
 */
using SubagentCustomMergeFn = std::function<SubagentResult<std::string>(
    const std::vector<SubagentCoordinatorResult>&)>;

// ============================================================================
// § 2  Coordinator Configuration
// ============================================================================

/**
 * @brief Coordinator configuration for parallel subagent inference.
 */
struct SubagentCoordinatorConfig {
    /// Merge strategy for aggregating results.
    SubagentMergeStrategy strategy = SubagentMergeStrategy::FIRST_WIN;

    /// Custom merge function (used when strategy == CUSTOM).
    SubagentCustomMergeFn custom_merge_fn;

    /// Per-subagent timeout (ms). 0 = use subagent default.
    int timeout_ms = 0;

    /// Correlation context for tracing (optional).
    std::optional<LLMCorrelationContext> correlation_context;

    /// Enable detailed per-subagent logging.
    bool verbose_logging = false;

    /// Fail overall if any subagent fails (vs allowing partial success).
    bool fail_on_any_error = false;

    /// Maximum time to wait for slowest subagent (override per-subagent timeout).
    /// 0 = no override (use per-subagent timeout).
    int max_total_latency_ms = 0;
};

// ============================================================================
// § 3  SubagentCoordinator Interface
// ============================================================================

/**
 * @brief Coordinator for parallel inference across multiple subagents.
 *
 * Enables distributed inference across independent subagents with merge
 * strategies, partial-failure handling, and result aggregation.
 *
 * ### Usage
 *
 * @code
 *   // Create coordinator
 *   auto coordinator = SubagentCoordinator::create(factory);
 *
 *   // Define config
 *   SubagentCoordinatorConfig config;
 *   config.strategy = SubagentMergeStrategy::FIRST_WIN;
 *   config.timeout_ms = 5000;
 *
 *   // Request to multiple subagents
 *   std::vector<std::string> subagent_ids = {"assistant_1", "analyzer_2", "writer_3"};
 *   InferenceRequest request;
 *   request.prompt = "Summarize the key points";
 *
 *   // Fan-out inference across all subagents
 *   auto result = coordinator->inferMultiple(subagent_ids, request, config);
 *
 *   if (result.success) {
 *       std::cout << "Merged result: " << result.merged_output << std::endl;
 *       std::cout << "Successful: " << result.num_successful
 *                 << " / " << result.per_subagent_results.size() << std::endl;
 *   } else {
 *       std::cout << "Coordination failed: " << result.summary << std::endl;
 *   }
 * @endcode
 */
class SubagentCoordinator {
public:
    /**
     * @brief Create a new subagent coordinator.
     *
     * @param factory SubagentFactory for subagent discovery and access.
     * @return New coordinator instance, or error string.
     */
    static SubagentResult<std::unique_ptr<SubagentCoordinator>> create(
        std::shared_ptr<SubagentFactory> factory);

    virtual ~SubagentCoordinator() = default;

    // Non-copyable
    SubagentCoordinator(const SubagentCoordinator&) = delete;
    SubagentCoordinator& operator=(const SubagentCoordinator&) = delete;

    // ========================================================================
    // Parallel Inference Coordination
    // ========================================================================

    /**
     * @brief Submit inference request to multiple subagents and collect results.
     *
     * Fan-out: submits request to all subagents asynchronously
     * Fan-in: collects results as they complete
     * Merge: applies merge strategy to aggregate results
     *
     * @param subagent_ids  Vector of subagent IDs to include.
     * @param request       Inference request.
     * @param config        Coordinator configuration (merge strategy, timeout, etc.).
     * @return Aggregated result with per-subagent details.
     *
     * Error cases:
     *   - Subagent not found
     *   - Subagent not in READY state
     *   - All subagents fail (when fail_on_any_error is true)
     *   - Merge strategy fails
     */
    virtual SubagentCoordinatorAggregateResult inferMultiple(
        const std::vector<std::string>& subagent_ids,
        const InferenceRequest& request,
        const SubagentCoordinatorConfig& config) = 0;

    /**
     * @brief Submit batch inference to multiple subagents.
     *
     * Each request in the batch is submitted to all subagents and merged.
     *
     * @param subagent_ids Vector of subagent IDs.
     * @param requests     Vector of inference requests.
     * @param config       Coordinator configuration.
     * @return Vector of aggregated results (same order as input requests).
     */
    virtual std::vector<SubagentCoordinatorAggregateResult> inferMultipleBatch(
        const std::vector<std::string>& subagent_ids,
        const std::vector<InferenceRequest>& requests,
        const SubagentCoordinatorConfig& config) = 0;

    // ========================================================================
    // Observability and Diagnostics
    // ========================================================================

    /**
     * @brief Get detailed diagnostics from last coordination operation.
     *
     * Returns logs, timing, and failure details for debugging.
     */
    struct CoordinationDiagnostics {
        std::string summary;                          ///< Summary of operation
        std::vector<std::string> per_subagent_logs;   ///< Detailed logs per subagent
        std::chrono::milliseconds fan_out_latency;    ///< Time to submit all requests
        std::chrono::milliseconds fan_in_latency;     ///< Time to collect all results
        std::chrono::milliseconds merge_latency;      ///< Time to merge results
        bool merge_failed = false;                     ///< Merge operation failed
        std::string merge_error;                       ///< Merge error message
    };

    /**
     * @brief Get diagnostics from last coordination operation.
     */
    virtual CoordinationDiagnostics getLastDiagnostics() = 0;

    // ========================================================================
    // Statistics
    // ========================================================================

    /**
     * @brief Coordinator-level statistics.
     */
    struct CoordinatorStats {
        uint64_t total_coordinations = 0;        ///< Total coordination operations
        uint64_t successful_coordinations = 0;  ///< Successful operations
        uint64_t failed_coordinations = 0;      ///< Failed operations
        uint64_t total_subagent_requests = 0;   ///< Total requests submitted to subagents
        uint64_t total_subagent_successes = 0;  ///< Successful subagent responses
        uint64_t total_subagent_failures = 0;   ///< Failed subagent responses
    };

    /**
     * @brief Get coordinator statistics.
     */
    virtual CoordinatorStats getStats() = 0;

    /**
     * @brief Reset statistics counters.
     */
    virtual void resetStats() = 0;

protected:
    SubagentCoordinator() = default;
};

} // namespace llm
} // namespace themis
