/**
 * @file subagent_factory.h
 * @brief Factory interface for creating and managing independent LLM Inferencing
 *        Subagents with isolated model/LoRA configuration and resource tracking.
 *
 * @note **Production-Grade Factory Pattern**: Encapsulates subagent lifecycle
 *       (creation, configuration validation, resource allocation) without
 *       modifying existing AIOrchestrator or AsyncInferenceEngine interfaces.
 *
 * ## Purpose
 *
 * SubagentFactory is the primary entry point for creating independent LLM
 * inference subagents. It handles:
 *   1. Configuration validation (model_id, lora_adapter_id, budget, policy)
 *   2. Resource pre-allocation (VRAM, quota buckets, thread pool slots)
 *   3. Lifecycle state machine initialization
 *   4. Integration with shared infrastructure (model cache, quota manager, policies)
 *   5. Subagent registry and discovery
 *
 * ## Architecture
 *
 * A subagent factory maintains:
 *   - Shared plugin reference (ILLMPlugin for inference)
 *   - Shared worker pool (SharedWorkerPool for async execution)
 *   - Shared model manager (ModelLoader for model caching)
 *   - Shared adapter manager (MultiLoRAManager for LoRA lifecycle)
 *   - Shared quota manager (TokenQuotaManager for per-subagent budgets)
 *   - Subagent registry (for discovery and lifecycle tracking)
 *
 * Multiple subagents created from the same factory share all infrastructure
 * but maintain independent configuration, quotas, and audit trails.
 *
 * ## Thread Safety
 *
 * All factory methods are thread-safe (guarded by internal mutex).
 * Created subagents are self-contained and safe for concurrent use.
 *
 * ## Non-Breaking Design
 *
 * SubagentFactory is purely additive; it does not modify existing APIs.
 * Callers who do not use subagents are unaffected.
 */

#pragma once

#include "llm/subagent_config.h"
#include "llm/llm_plugin_interface.h"
#include "llm/shared_worker_pool.h"
#include "llm/token_quota_manager.h"
#include "llm/prompt_policy.h"
#include "llm/model_loader.h"
#include "llm/multi_lora_manager.h"
#include "llm/llm_interaction_store.h"
#include "utils/expected.h"

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <chrono>

namespace themis {
namespace llm {

// Forward declarations
class Subagent;
class SubagentLifecycleManager;
class ModelLoader;

template <typename T>
using SubagentResult = tl::expected<T, std::string>;

struct SubagentValidationError {
    std::string field;    ///< Configuration field that failed (e.g., "model_id")
    std::string reason;   ///< Human-readable error (e.g., "model not found")
    std::string detail;   ///< Additional context
};

class SubagentFactory {
public:
    struct Config {
        size_t max_subagents = 0;

        SubagentIsolationLevel default_isolation_level = SubagentIsolationLevel::STRICT;

        bool enable_audit_logging = true;

        bool enable_metrics = true;

        int resource_alloc_timeout_ms = 5000;
    };

    static SubagentResult<std::unique_ptr<SubagentFactory>> create(
        ILLMPlugin* plugin,
        std::shared_ptr<SharedWorkerPool> worker_pool,
        std::shared_ptr<ModelLoader> model_loader,
        std::shared_ptr<MultiLoRAManager> lora_manager,
        std::shared_ptr<TokenQuotaManager> quota_manager = nullptr);

    /**
     * @brief Create.
     * @param[in,out] plugin Input/output parameter.
     * @param[in] worker_pool Input parameter.
     * @param[in] model_loader Input parameter.
     * @param[in] lora_manager Input parameter.
     * @param[in] quota_manager Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    static SubagentResult<std::unique_ptr<SubagentFactory>> create(
        ILLMPlugin* plugin,
        std::shared_ptr<SharedWorkerPool> worker_pool,
        std::shared_ptr<ModelLoader> model_loader,
        std::shared_ptr<MultiLoRAManager> lora_manager,
        std::shared_ptr<TokenQuotaManager> quota_manager,
        const Config& config);

    /**
     * @brief Subagent Factory.
     * @return Return value.
     */
    virtual ~SubagentFactory() = default;

    // Non-copyable, non-moveable (internal mutex state)
    SubagentFactory(const SubagentFactory&) = delete;
    SubagentFactory& operator=(const SubagentFactory&) = delete;
    SubagentFactory(SubagentFactory&&) = delete;
    SubagentFactory& operator=(SubagentFactory&&) = delete;

    // ========================================================================
    // Subagent Lifecycle Management
    // ========================================================================

    /**
     * @brief Validate Config.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    virtual std::vector<SubagentValidationError> validateConfig(
        const SubagentConfig& config) = 0;

    /**
     * @brief Create Subagent.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    virtual SubagentResult<std::shared_ptr<Subagent>> createSubagent(
        const SubagentConfig& config) = 0;

    virtual SubagentResult<void> destroySubagent(
        const std::string& subagent_id,
        int timeout_ms = 30000) = 0;

    // ========================================================================
    // Subagent Discovery and Inspection
    // ========================================================================

    /**
     * @brief Get Subagent.
     * @param[in] subagent_id Identifier of the subagent.
     * @return Return value.
     */
    virtual std::shared_ptr<Subagent> getSubagent(
        const std::string& subagent_id) = 0;

    /**
     * @brief List Subagents.
     * @return Return value.
     */
    virtual std::vector<std::string> listSubagents() = 0;

    /**
     * @brief Get Subagent Metrics.
     * @param[in] subagent_id Identifier of the subagent.
     * @return Return value.
     */
    virtual SubagentResult<SubagentMetrics> getSubagentMetrics(
        const std::string& subagent_id) = 0;

    /**
     * @brief Get Subagent State.
     * @param[in] subagent_id Identifier of the subagent.
     * @return Return value.
     */
    virtual SubagentResult<SubagentState> getSubagentState(
        const std::string& subagent_id) = 0;

    // ========================================================================
    // Policy and Quota Management
    // ========================================================================

    /**
     * @brief Register Prompt Policy.
     * @param[in] policy_id Identifier of the policy.
     * @param[in] policy Input parameter.
     * @return Return value.
     */
    virtual SubagentResult<void> registerPromptPolicy(
        const std::string& policy_id,
        std::shared_ptr<PromptPolicy> policy) = 0;

    /**
     * @brief Unregister Prompt Policy.
     * @param[in] policy_id Identifier of the policy.
     * @return Return value.
     */
    virtual SubagentResult<void> unregisterPromptPolicy(
        const std::string& policy_id) = 0;

    // ========================================================================
    // Statistics and Observability
    // ========================================================================

    struct FactoryStats {
        size_t total_created = 0;          ///< Total subagents created (lifetime)
        size_t total_destroyed = 0;        ///< Total subagents destroyed
        size_t currently_active = 0;       ///< Currently active subagents
        size_t total_inference_requests = 0; ///< Total inference requests
        size_t total_successful_inferences = 0; ///< Successful inferences
        size_t total_failed_inferences = 0; ///< Failed inferences
        std::chrono::steady_clock::time_point factory_start_time;
    };

    /**
     * @brief Get Factory Stats.
     * @return Return value.
     */
    virtual FactoryStats getFactoryStats() = 0;

protected:
    SubagentFactory() = default;
};

} // namespace llm
} // namespace themis
