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

/**
 * @brief Result type for subagent operations.
 *
 * Returns either a success value or an error string.
 */
template <typename T>
using SubagentResult = tl::expected<T, std::string>;

/**
 * @brief Validation error for subagent configuration.
 */
struct SubagentValidationError {
    std::string field;    ///< Configuration field that failed (e.g., "model_id")
    std::string reason;   ///< Human-readable error (e.g., "model not found")
    std::string detail;   ///< Additional context
};

/**
 * @brief Subagent Factory: Creates and manages independent LLM Inferencing Subagents.
 *
 * This is the primary factory for creating independent subagent instances.
 * Each subagent maintains isolated configuration, budget, and policy enforcement
 * while sharing common infrastructure (models, caches, database, worker pool).
 *
 * ### Usage
 *
 * @code
 *   // Create factory with shared infrastructure
 *   auto factory = SubagentFactory::create(
 *       llm_plugin,
 *       worker_pool,
 *       model_loader,
 *       lora_manager
 *   );
 *
 *   // Define subagent configuration
 *   SubagentConfig config;
 *   config.id = "assistant_1";
 *   config.model_id = "mistral-7b";
 *   config.lora_adapter_id = "customer-support";
 *   config.budget.max_tokens_per_minute = 50000;
 *   config.policy.prompt_policy_id = "no-jailbreak";
 *
 *   // Create subagent instance
 *   auto result = factory->createSubagent(config);
 *   if (!result) {
 *       std::cerr << "Subagent creation failed: " << result.error() << std::endl;
 *       return;
 *   }
 *   auto subagent = result.value();
 *
 *   // Use subagent for inference
 *   InferenceRequest req;
 *   req.prompt = "Hello, world!";
 *   auto inference_result = subagent->infer(req);
 *
 *   // Shutdown
 *   factory->destroySubagent(subagent->id());
 * @endcode
 */
class SubagentFactory {
public:
    /**
     * @brief Factory configuration for shared infrastructure.
     */
    struct Config {
        /// Maximum number of subagents to allow (0 = unlimited).
        size_t max_subagents = 0;

        /// Default isolation level for new subagents.
        SubagentIsolationLevel default_isolation_level = SubagentIsolationLevel::STRICT;

        /// Enable per-subagent audit logging.
        bool enable_audit_logging = true;

        /// Enable per-subagent metrics collection.
        bool enable_metrics = true;

        /// Default timeout for resource allocation operations (ms).
        int resource_alloc_timeout_ms = 5000;
    };

    /**
     * @brief Create a new subagent factory.
     *
     * @param plugin       LLM inference plugin (not owned; must outlive factory).
     * @param worker_pool  Shared worker pool for async tasks.
     * @param model_loader Model cache/loader for model lifecycle.
     * @param lora_manager LoRA adapter manager for adapter lifecycle.
     * @param quota_manager Token quota manager (optional, creates local if null).
     * @param config       Factory configuration (default: sensible defaults).
     * @return New factory instance, or error string.
     *
     * The factory does not take ownership of pointer arguments; they must
     * remain valid for the lifetime of the factory.
     */
    static SubagentResult<std::unique_ptr<SubagentFactory>> create(
        ILLMPlugin* plugin,
        std::shared_ptr<SharedWorkerPool> worker_pool,
        std::shared_ptr<ModelLoader> model_loader,
        std::shared_ptr<MultiLoRAManager> lora_manager,
        std::shared_ptr<TokenQuotaManager> quota_manager = nullptr,
        const Config& config = Config{});

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
     * @brief Validate a subagent configuration without creating an instance.
     *
     * Performs checks:
     *   - model_id exists in model cache
     *   - lora_adapter_id (if specified) is compatible with model
     *   - policy_id references registered policy
     *   - ethics_profile_id references registered profile
     *   - budget constraints are reasonable
     *
     * @param config Configuration to validate.
     * @return Empty vector if valid; non-empty vector of errors if invalid.
     */
    virtual std::vector<SubagentValidationError> validateConfig(
        const SubagentConfig& config) = 0;

    /**
     * @brief Create a new subagent instance.
     *
     * Performs validation, allocates resources (quota, thread pool slots),
     * and transitions the subagent to LOADING state.
     *
     * The subagent is not yet ready for inference; call subagent->load()
     * to complete the loading phase.
     *
     * @param config Subagent configuration.
     * @return New Subagent instance, or error string.
     *
     * Error cases:
     *   - Configuration validation failed (see validateConfig())
     *   - Model/adapter not found
     *   - Resource allocation exhausted
     *   - Max subagents limit reached
     *
     * @see Subagent::load()
     */
    virtual SubagentResult<std::shared_ptr<Subagent>> createSubagent(
        const SubagentConfig& config) = 0;

    /**
     * @brief Destroy a subagent and release its resources.
     *
     * Transitions the subagent to UNLOADING, waits for in-flight requests,
     * then TERMINATED. Resources (VRAM, quota, thread pool slots) are released.
     *
     * After this call the subagent handle should not be used.
     *
     * @param subagent_id Subagent ID (e.g., "assistant_1").
     * @param timeout_ms  Maximum time to wait for in-flight requests (default: 30s).
     * @return Success if destroyed, error string if not found or timeout.
     */
    virtual SubagentResult<void> destroySubagent(
        const std::string& subagent_id,
        int timeout_ms = 30000) = 0;

    // ========================================================================
    // Subagent Discovery and Inspection
    // ========================================================================

    /**
     * @brief Get a subagent by ID.
     *
     * @param subagent_id Subagent ID.
     * @return Subagent instance, or nullptr if not found.
     */
    virtual std::shared_ptr<Subagent> getSubagent(
        const std::string& subagent_id) = 0;

    /**
     * @brief List all active subagents.
     *
     * @return Vector of subagent IDs that are currently active (not terminated).
     */
    virtual std::vector<std::string> listSubagents() = 0;

    /**
     * @brief Get metrics for a subagent.
     *
     * @param subagent_id Subagent ID.
     * @return Metrics snapshot, or error if not found.
     */
    virtual SubagentResult<SubagentMetrics> getSubagentMetrics(
        const std::string& subagent_id) = 0;

    /**
     * @brief Get current state of a subagent.
     *
     * @param subagent_id Subagent ID.
     * @return Subagent state, or error if not found.
     */
    virtual SubagentResult<SubagentState> getSubagentState(
        const std::string& subagent_id) = 0;

    // ========================================================================
    // Policy and Quota Management
    // ========================================================================

    /**
     * @brief Register or update a prompt policy for use by subagents.
     *
     * Policies are referenced by policy_id in SubagentConfig.
     *
     * @param policy_id Unique policy identifier.
     * @param policy    PromptPolicy instance.
     * @return Success, or error string.
     */
    virtual SubagentResult<void> registerPromptPolicy(
        const std::string& policy_id,
        std::shared_ptr<PromptPolicy> policy) = 0;

    /**
     * @brief Unregister a prompt policy.
     *
     * Fails if any subagent is currently using this policy.
     *
     * @param policy_id Policy identifier.
     * @return Success, or error string.
     */
    virtual SubagentResult<void> unregisterPromptPolicy(
        const std::string& policy_id) = 0;

    // ========================================================================
    // Statistics and Observability
    // ========================================================================

    /**
     * @brief Get factory-level statistics.
     */
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
     * @brief Get factory statistics.
     */
    virtual FactoryStats getFactoryStats() = 0;

protected:
    SubagentFactory() = default;
};

} // namespace llm
} // namespace themis
