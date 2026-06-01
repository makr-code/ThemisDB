/**
 * @file model_switch.h
 * @brief Model-Switch workflow — hot and cold swap of LLM and LoRA adapters.
 *
 * Manages the lifecycle of base-model transitions and adapter compatibility
 * checks. Ensures zero-downtime model upgrades with rollback capability.
 *
 * Planned in: docs/EPIC1_MODEL_SWITCH.md
 * Sub-issue:   #5419
 */

#pragma once

#include "lora_package.h"

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace themis::retrieval {

/// Outcome of a model-switch attempt.
enum class SwitchOutcome {
    Success,
    CompatibilityError,  ///< Adapter incompatible with the target base model
    TimeoutError,        ///< Switch did not complete within the deadline
    RollbackTriggered,   ///< Performance degradation detected; reverted
    Pending,             ///< Async switch in progress
};

/// Describes a base-model version with its compatibility constraints.
struct ModelVersion {
    std::string id;           ///< Tag or content hash
    std::string architecture; ///< e.g. "llama-3-8b", "mistral-7b"
    std::string quantization; ///< e.g. "fp16", "int4"
    std::vector<std::string> compatible_adapter_tokens;
};

/// Request to switch the active base model (and optionally adapter).
struct SwitchRequest {
    ModelVersion              target_model;
    std::optional<std::string> adapter_id; ///< PortableAdapterProduct ID
    std::chrono::milliseconds  deadline{5000};
    bool                       allow_rollback = true;
};

/// Result of a completed switch attempt.
struct SwitchResult {
    SwitchOutcome outcome;
    std::string   previous_model_id;
    std::string   active_model_id;
    double        switch_latency_ms = 0.0;
    std::string   failure_reason;
};

/// Policy evaluated before each switch to gate compatibility.
struct CompatibilityPolicy {
    bool require_exact_arch_match = true;
    bool allow_quantization_mismatch = false;
    float min_eval_score_delta = -0.05f; ///< Tolerated regression post-switch
};

/**
 * @brief Model-Switch Controller interface.
 *
 * Orchestrates safe base-model and adapter transitions with compatibility
 * validation, performance guardrails, and rollback.
 */
class IModelSwitchController {
public:
    virtual ~IModelSwitchController() = default;

    /// Perform or schedule a model switch.
    virtual SwitchResult execute(const SwitchRequest& req) = 0;

    /// Rollback to the previously active model.
    virtual SwitchResult rollback() = 0;

    /// Return the currently active model version.
    virtual ModelVersion active() const = 0;

    /// Check whether a switch to the given model is compatible.
    virtual bool checkCompatibility(const ModelVersion& target,
                                    const CompatibilityPolicy& policy) const = 0;

    /// Register an observer called on every switch event.
    using SwitchObserver = std::function<void(const SwitchResult&)>;
    virtual void onSwitch(SwitchObserver obs) = 0;
};

/// Factory: create a ModelSwitchController with initial model and policy.
std::unique_ptr<IModelSwitchController> makeModelSwitchController(
    const ModelVersion& initial, const CompatibilityPolicy& policy);

} // namespace themis::retrieval
