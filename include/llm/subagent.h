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

/**
 * @brief Result of an inference operation.
 */
struct SubagentInferenceResult {
    /// Success flag.
    bool success = false;
    /// Generated output (if successful).
    std::string output;
    /// Error message (if failed).
    std::string error;
    /// Tokens consumed (prompt + completion).
    size_t tokens_consumed = 0;
    /// Inference latency (ms).
    int latency_ms = 0;
    /// Trace ID for observability.
    std::string trace_id;
};

/**
 * @brief Individual Subagent instance for isolated LLM inference.
 *
 * Represents a single inference entity with independent model, adapter,
 * budget, and policy configuration. Created and managed by SubagentFactory.
 *
 * ### Usage
 *
 * @code
 *   // Factory creates subagent
 *   auto subagent = factory->createSubagent(config);
 *
 *   // Load model/adapter (transitions CREATED → LOADING → READY)
 *   auto load_result = subagent->load();
 *   if (!load_result) {
 *       std::cerr << "Load failed: " << load_result.error() << std::endl;
 *   }
 *
 *   // Submit inference request
 *   InferenceRequest req;
 *   req.prompt = "What is 2+2?";
 *   auto result = subagent->infer(req);
 *
 *   // Get metrics
 *   auto metrics = subagent->getMetrics();
 *   std::cout << "Tokens: " << metrics.tokens_consumed << std::endl;
 *
 *   // Unload (factory will destroy when done)
 * @endcode
 */
class Subagent {
public:
    virtual ~Subagent() = default;

    // ========================================================================
    // Identity and Configuration
    // ========================================================================

    /**
     * @brief Get the unique subagent ID.
     */
    virtual const std::string& id() const = 0;

    /**
     * @brief Get the subagent configuration.
     */
    virtual const SubagentConfig& config() const = 0;

    // ========================================================================
    // Lifecycle Management
    // ========================================================================

    /**
     * @brief Get current state of the subagent.
     */
    virtual SubagentState getState() const = 0;

    /**
     * @brief Load the model and LoRA adapter.
     *
     * Transitions CREATED → LOADING → READY.
     * Must be called before inference.
     *
     * @param timeout_ms Maximum time to wait (default: 60s).
     * @return Success, or error string.
     */
    virtual SubagentResult<void> load(int timeout_ms = 60000) = 0;

    /**
     * @brief Warm the model (pre-allocate buffers, compile kernels).
     *
     * Optional; improves first-request latency.
     * Must only be called when subagent is READY.
     *
     * @param timeout_ms Maximum time to wait (default: 30s).
     * @return Success, or error string.
     */
    virtual SubagentResult<void> warm(int timeout_ms = 30000) = 0;

    /**
     * @brief Unload the model and LoRA adapter.
     *
     * Transitions READY → UNLOADING → TERMINATED.
     * Waits for in-flight requests to complete.
     *
     * After unload, the subagent cannot be used for inference.
     * The factory should call destroySubagent() to clean up.
     *
     * @param timeout_ms Maximum time to wait for in-flight requests.
     * @return Success, or error string.
     */
    virtual SubagentResult<void> unload(int timeout_ms = 30000) = 0;

    // ========================================================================
    // Inference Operations
    // ========================================================================

    /**
     * @brief Submit an inference request (synchronous).
     *
     * Enforces policy gates and quota checks before submission.
     *
     * The request is processed by the shared worker pool and result is
     * returned when complete.
     *
     * @param request   Inference request.
     * @param ctx       Optional correlation context for tracing.
     * @return Inference result (with output or error).
     *
     * Errors:
     *   - Subagent not in READY state
     *   - Quota exceeded (if block_on_quota_violation)
     *   - Policy violation (if block_on_policy_violation)
     *   - Inference timeout
     *   - Model/adapter unavailable
     */
    virtual SubagentInferenceResult infer(
        const InferenceRequest& request,
        const std::optional<LLMCorrelationContext>& ctx = std::nullopt) = 0;

    /**
     * @brief Submit an inference request (asynchronous).
     *
     * Like infer() but returns immediately with a future.
     * The caller can block on the future to get the result.
     *
     * @param request Inference request.
     * @param ctx     Optional correlation context for tracing.
     * @return Future that resolves when inference completes.
     */
    virtual std::future<SubagentInferenceResult> inferAsync(
        const InferenceRequest& request,
        const std::optional<LLMCorrelationContext>& ctx = std::nullopt) = 0;

    /**
     * @brief Submit an inference request with streaming callback.
     *
     * Tokens are streamed to the callback as they are generated.
     *
     * @param request   Inference request.
     * @param on_token  Called for each generated token.
     * @param ctx       Optional correlation context for tracing.
     * @return Inference result (with metadata and final output).
     */
    virtual SubagentInferenceResult inferStream(
        const InferenceRequest& request,
        std::function<void(const std::string&)> on_token,
        const std::optional<LLMCorrelationContext>& ctx = std::nullopt) = 0;

    /**
     * @brief Submit a batch of inference requests.
     *
     * Requests are processed in parallel by the shared worker pool.
     *
     * @param requests  Vector of inference requests.
     * @param ctx       Optional correlation context for tracing.
     * @return Vector of inference results (same order as input).
     */
    virtual std::vector<SubagentInferenceResult> inferBatch(
        const std::vector<InferenceRequest>& requests,
        const std::optional<LLMCorrelationContext>& ctx = std::nullopt) = 0;

    // ========================================================================
    // Observability
    // ========================================================================

    /**
     * @brief Get current metrics snapshot.
     */
    virtual SubagentMetrics getMetrics() const = 0;

    /**
     * @brief Reset metrics counters (e.g., at start of quota window).
     */
    virtual void resetMetrics() = 0;

    /**
     * @brief Get the last error message (if state is ERROR).
     */
    virtual std::string getLastError() const = 0;

    // ========================================================================
    // Resource Management
    // ========================================================================

    /**
     * @brief Check if subagent is ready for inference.
     *
     * Equivalent to getState() == SubagentState::READY.
     */
    virtual bool isReady() const = 0;

    /**
     * @brief Pause inference (e.g., quota exhausted, policy violation).
     *
     * Transitions READY → PAUSED.
     * Incoming requests are rejected until resumed.
     *
     * @return Success, or error string.
     */
    virtual SubagentResult<void> pause() = 0;

    /**
     * @brief Resume inference after pause.
     *
     * Transitions PAUSED → READY.
     *
     * @return Success, or error string.
     */
    virtual SubagentResult<void> resume() = 0;

    /**
     * @brief Check current quota status.
     *
     * @return Quota check result (allowed, tokens_used, tokens_limit).
     */
    virtual QuotaCheckResult checkQuota(size_t estimated_tokens) const = 0;

    /**
     * @brief Force-consume tokens from quota (e.g., after external inference).
     *
     * @param tokens Number of tokens to consume.
     * @return QuotaCheckResult after consumption.
     */
    virtual QuotaCheckResult consumeQuota(size_t tokens) = 0;

    /**
     * @brief Reset quota window (e.g., at start of new minute).
     */
    virtual void resetQuota() = 0;
};

} // namespace llm
} // namespace themis
