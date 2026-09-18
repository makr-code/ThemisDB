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

#include <functional>
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

enum class SubagentMergeStrategy {
    FIRST_WIN = 0,

    ALL_SUCCEED = 1,

    MAJORITY_VOTE = 2,

    BEST_SCORE = 3,

    ENSEMBLE = 4,

    CUSTOM = 5,
};

struct SubagentCoordinatorResult {
    std::string subagent_id;

    bool success = false;

    std::string output;

    std::string error;

    size_t tokens_consumed = 0;

    int latency_ms = 0;

    float quality_score = 0.0f;

    std::string trace_id;
};

struct SubagentCoordinatorAggregateResult {
    bool success = false;

    std::string merged_output;

    std::string summary;

    std::vector<SubagentCoordinatorResult> per_subagent_results;

    size_t num_successful = 0;

    size_t num_failed = 0;

    size_t total_tokens_consumed = 0;

    int total_latency_ms = 0;

    SubagentMergeStrategy strategy = SubagentMergeStrategy::FIRST_WIN;
};

using SubagentCustomMergeFn = std::function<std::optional<std::string>(
    const std::vector<SubagentCoordinatorResult>&)>;

// ============================================================================
// § 2  Coordinator Configuration
// ============================================================================

struct SubagentCoordinatorConfig {
    SubagentMergeStrategy strategy = SubagentMergeStrategy::FIRST_WIN;

    SubagentCustomMergeFn custom_merge_fn;

    int timeout_ms = 0;

    std::optional<LLMCorrelationContext> correlation_context;

    bool verbose_logging = false;

    bool fail_on_any_error = false;

    int max_total_latency_ms = 0;
};

// ============================================================================
// § 3  SubagentCoordinator Interface
// ============================================================================

class SubagentCoordinator {
public:
    /**
     * @brief Create.
     * @param[in] factory Input parameter.
     * @return Return value.
     */
    static SubagentResult<std::unique_ptr<SubagentCoordinator>> create(
        std::shared_ptr<SubagentFactory> factory);

    /**
     * @brief Subagent Coordinator.
     * @return Return value.
     */
    virtual ~SubagentCoordinator() = default;

    // Non-copyable
    SubagentCoordinator(const SubagentCoordinator&) = delete;
    SubagentCoordinator& operator=(const SubagentCoordinator&) = delete;

    // ========================================================================
    // Parallel Inference Coordination
    // ========================================================================

    /**
     * @brief Infer Multiple.
     * @param[in] subagent_ids Input parameter.
     * @param[in] request Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    virtual SubagentCoordinatorAggregateResult inferMultiple(
        const std::vector<std::string>& subagent_ids,
        const InferenceRequest& request,
        const SubagentCoordinatorConfig& config) = 0;

    /**
     * @brief Infer Multiple Batch.
     * @param[in] subagent_ids Input parameter.
     * @param[in] requests Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    virtual std::vector<SubagentCoordinatorAggregateResult> inferMultipleBatch(
        const std::vector<std::string>& subagent_ids,
        const std::vector<InferenceRequest>& requests,
        const SubagentCoordinatorConfig& config) = 0;

    // ========================================================================
    // Observability and Diagnostics
    // ========================================================================

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
     * @brief Get Last Diagnostics.
     * @return Return value.
     */
    virtual CoordinationDiagnostics getLastDiagnostics() = 0;

    // ========================================================================
    // Statistics
    // ========================================================================

    struct CoordinatorStats {
        uint64_t total_coordinations = 0;        ///< Total coordination operations
        uint64_t successful_coordinations = 0;  ///< Successful operations
        uint64_t failed_coordinations = 0;      ///< Failed operations
        uint64_t total_subagent_requests = 0;   ///< Total requests submitted to subagents
        uint64_t total_subagent_successes = 0;  ///< Successful subagent responses
        uint64_t total_subagent_failures = 0;   ///< Failed subagent responses
    };

    /**
     * @brief Get Stats.
     * @return Return value.
     */
    virtual CoordinatorStats getStats() = 0;

    /**
     * @brief Reset Stats.
     */
    virtual void resetStats() = 0;

protected:
    SubagentCoordinator() = default;
};

} // namespace llm
} // namespace themis
