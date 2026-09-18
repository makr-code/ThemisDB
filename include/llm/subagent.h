/**
 * @file subagent.h
 * @brief Individual Subagent interface for independent LLM inference with
 *        isolated configuration, budget, and policy enforcement.
 *
 * @note **Subagent Instance Interface**: Represents a single orchestrated
 *       LLM inference entity created and managed by SubagentFactory.
 *
 * ## Purpose
 *
 * A Subagent is an individual inference instance created by SubagentFactory
 * with independent configuration. It handles:
 *   1. Model/adapter lifecycle (load, warm, unload)
 *   2. Inference submission and result retrieval
 *   3. Per-subagent quota enforcement and policy gating
 *   4. Observability (audit logging, metrics, correlation IDs)
 *   5. State management (CREATED → LOADING → READY → ...)
 *
 * ## Lifecycle
 *
 * ```
 * CREATED → LOADING (load model/adapter)
 *        → READY (ready for inference)
 *        → PAUSED (quota exhausted / policy violation)
 *        → READY (quota reset / issue resolved)
 *        → UNLOADING (shutdown initiated)
 *        → TERMINATED (resources released)
 *
 * Any state → ERROR (on resource exhaustion or fatal error)
 * ```
 *
 * ## Thread Safety
 *
 * Subagent methods are thread-safe. Multiple threads can submit inference
 * requests concurrently; they are queued and processed by the shared worker pool.
 */

#pragma once

#include "llm/subagent_config.h"
#include "llm/subagent_factory.h"
#include "llm/llm_plugin_interface.h"
#include "llm/llm_correlation_context.h"
#include "llm/token_quota_manager.h"

#include <functional>
#include <future>
#include <memory>
#include <string>
#include <chrono>
#include <optional>
#include <vector>

namespace themis {
namespace llm {

struct SubagentInferenceResult {
    bool success = false;
    std::string output;
    std::string error;
    size_t tokens_consumed = 0;
    int latency_ms = 0;
    std::string trace_id;
};

class Subagent {
public:
    /**
     * @brief Subagent.
     * @return Return value.
     */
    virtual ~Subagent() = default;

    // ========================================================================
    // Identity and Configuration
    // ========================================================================

    /**
     * @brief Id.
     * @return Return value.
     */
    virtual const std::string& id() const = 0;

    /**
     * @brief Config.
     * @return Return value.
     */
    virtual const SubagentConfig& config() const = 0;

    // ========================================================================
    // Lifecycle Management
    // ========================================================================

    /**
     * @brief Get State.
     * @return Return value.
     */
    virtual SubagentState getState() const = 0;

    virtual SubagentResult<void> load(int timeout_ms = 60000) = 0;

    virtual SubagentResult<void> warm(int timeout_ms = 30000) = 0;

    virtual SubagentResult<void> unload(int timeout_ms = 30000) = 0;

    // ========================================================================
    // Inference Operations
    // ========================================================================

    virtual SubagentInferenceResult infer(
        const InferenceRequest& request,
        const std::optional<LLMCorrelationContext>& ctx = std::nullopt) = 0;

    virtual std::future<SubagentInferenceResult> inferAsync(
        const InferenceRequest& request,
        const std::optional<LLMCorrelationContext>& ctx = std::nullopt) = 0;

    virtual SubagentInferenceResult inferStream(
        const InferenceRequest& request,
        std::function<void(const std::string&)> on_token,
        const std::optional<LLMCorrelationContext>& ctx = std::nullopt) = 0;

    virtual std::vector<SubagentInferenceResult> inferBatch(
        const std::vector<InferenceRequest>& requests,
        const std::optional<LLMCorrelationContext>& ctx = std::nullopt) = 0;

    // ========================================================================
    // Observability
    // ========================================================================

    /**
     * @brief Get Metrics.
     * @return Return value.
     */
    virtual SubagentMetrics getMetrics() const = 0;

    /**
     * @brief Reset Metrics.
     */
    virtual void resetMetrics() = 0;

    /**
     * @brief Get Last Error.
     * @return Return value.
     */
    virtual std::string getLastError() const = 0;

    // ========================================================================
    // Resource Management
    // ========================================================================

    /**
     * @brief Is Ready.
     * @return True when the operation succeeds.
     */
    virtual bool isReady() const = 0;

    /**
     * @brief Pause.
     * @return Return value.
     */
    virtual SubagentResult<void> pause() = 0;

    /**
     * @brief Resume.
     * @return Return value.
     */
    virtual SubagentResult<void> resume() = 0;

    /**
     * @brief Check Quota.
     * @param[in] estimated_tokens Input parameter.
     * @return Return value.
     */
    virtual QuotaCheckResult checkQuota(size_t estimated_tokens) const = 0;

    /**
     * @brief Consume Quota.
     * @param[in] tokens Input parameter.
     * @return Return value.
     */
    virtual QuotaCheckResult consumeQuota(size_t tokens) = 0;

    /**
     * @brief Reset Quota.
     */
    virtual void resetQuota() = 0;
};

} // namespace llm
} // namespace themis
