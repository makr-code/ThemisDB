/**
 * @file illm_resource_policy.h
 * @brief Abstract LLM resource-limit policy for ThemisDB inference paths.
 *
 * Governs per-inference context-token budgets, the number of concurrently
 * loaded model instances, and per-model VRAM allocation ceilings.  The
 * policy sits in Tier 2 of the four-tier resource-governance chain:
 *
 * @code
 *   compile-time constexpr (edition.h)   ← absolute ceiling, never overridable
 *   RuntimeLicenseGate                   ← edition-tier ceiling
 *   ILLMResourcePolicy  (this file)      ← signed-plugin fine-tuning
 *   LLMResourceConfig                    ← per-deployment operational tuning
 * @endcode
 *
 * All implementations must be individually thread-safe.
 *
 * @note This interface is part of the edition-policy plugin contract
 *       (IEditionPolicyPlugin::createLLMResourcePolicy).  Claimed limits are
 *       validated against the compile-time ceilings
 *       edition::LLM_MAX_CONTEXT_TOKENS, edition::LLM_MAX_MODEL_INSTANCES, and
 *       edition::LLM_MAX_VRAM_PER_MODEL_MB before a policy is accepted.
 */

#pragma once

#include <cstdint>
#include <string>

namespace themis {
namespace llm {

/**
 * @brief Abstract LLM resource-limit policy.
 *
 * Controls the three primary LLM resource axes:
 *  - **Context tokens** — maximum prompt + completion tokens per inference call.
 *  - **Model instances** — maximum number of models loaded simultaneously.
 *  - **Per-model VRAM** — maximum video RAM reserved per loaded model instance.
 *
 * Implementations are installed into EditionManager via
 * `EditionManager::installLLMResourcePolicy()` and are queried at every LLM
 * dispatch point.
 */
class ILLMResourcePolicy {
public:
    virtual ~ILLMResourcePolicy() = default;

    // Non-copyable, non-movable by default.
    ILLMResourcePolicy(const ILLMResourcePolicy&)            = delete;
    ILLMResourcePolicy& operator=(const ILLMResourcePolicy&) = delete;

    // -------------------------------------------------------------------------
    // Context-token limits
    // -------------------------------------------------------------------------

    /**
     * @brief Return true iff an inference request for @p token_count tokens
     *        is permitted under the current policy.
     *
     * Does not modify any state.  A value of 0 for the policy's
     * maxContextTokens() means unlimited — this method must return @c true
     * for any positive @p token_count in that case.
     *
     * @param token_count  Total token budget (prompt + completion) requested.
     * @return true when the request fits within the policy.
     */
    [[nodiscard]] virtual bool canAllocateContext(int64_t token_count) const = 0;

    /**
     * @brief Declared maximum context-token budget this policy allows per inference.
     *
     * @return Maximum token count; 0 signals unlimited.
     */
    [[nodiscard]] virtual int64_t maxContextTokens() const noexcept = 0;

    // -------------------------------------------------------------------------
    // Model-instance limits
    // -------------------------------------------------------------------------

    /**
     * @brief Return true iff loading one more model instance is permitted.
     *
     * Should consult activeModelInstances() to make the decision.  Does not
     * modify accounting state — call onModelLoaded() only after the load
     * succeeds.
     *
     * @return true when another model instance may be loaded.
     */
    [[nodiscard]] virtual bool canLoadModel() const = 0;

    /**
     * @brief Declared maximum number of concurrently loaded model instances.
     *
     * @return Maximum instance count; -1 signals unlimited.
     */
    [[nodiscard]] virtual int32_t maxModelInstances() const noexcept = 0;

    /**
     * @brief Notify the policy that a model with @p model_id has been loaded.
     *
     * Updates internal accounting.  Thread-safe.
     *
     * @param model_id  Unique model identifier (non-empty string).
     */
    virtual void onModelLoaded(const std::string& model_id) = 0;

    /**
     * @brief Notify the policy that the model @p model_id has been unloaded.
     *
     * Updates internal accounting.  Implementations must clamp to zero on
     * mismatched calls to prevent underflow.  Thread-safe.
     *
     * @param model_id  Model identifier previously passed to onModelLoaded().
     */
    virtual void onModelUnloaded(const std::string& model_id) = 0;

    /**
     * @brief Return the number of model instances currently tracked as active.
     */
    [[nodiscard]] virtual int32_t activeModelInstances() const = 0;

    // -------------------------------------------------------------------------
    // Per-model VRAM limits
    // -------------------------------------------------------------------------

    /**
     * @brief Return true iff allocating @p vram_mb MiB for a single model is permitted.
     *
     * @param vram_mb  VRAM to allocate in mebibytes.
     * @return true when the allocation fits within the per-model ceiling.
     */
    [[nodiscard]] virtual bool canAllocateModelVRAM(int64_t vram_mb) const = 0;

    /**
     * @brief Declared maximum VRAM per model instance in mebibytes.
     *
     * @return VRAM ceiling in MiB; 0 signals unlimited.
     */
    [[nodiscard]] virtual int64_t maxVRAMPerModelMB() const noexcept = 0;

    // -------------------------------------------------------------------------
    // Status
    // -------------------------------------------------------------------------

    /**
     * @brief Return true when any LLM acceleration is permitted by this policy.
     *
     * Implementations should return false when all model-instance or
     * context-token limits are exhausted and no further work can be dispatched.
     */
    [[nodiscard]] virtual bool isLLMEnabled() const noexcept = 0;

protected:
    ILLMResourcePolicy() = default;
    ILLMResourcePolicy(ILLMResourcePolicy&&) noexcept = default;
    ILLMResourcePolicy& operator=(ILLMResourcePolicy&&) noexcept = default;
};

} // namespace llm
} // namespace themis
