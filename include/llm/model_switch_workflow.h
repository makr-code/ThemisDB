/**
 * @file model_switch_workflow.h
 * @brief Model-switch workflow, ratchet compatibility matrix, and rebuild policy
 *        for SOP-compliant LLM base-model transitions.
 *
 * Implements Phase 6 of EPIC 1.5: safe and auditable base-model switch with
 * explicit versioned compatibility tracking, rebuild-first logic, and
 * end-to-end policy evaluation gates.
 *
 * Design goals:
 * - The compatibility matrix is versionable and serializable to JSON.
 * - Ratchet semantics prevent rollback of the minimum-required model version
 *   unless an explicit override is supplied by an operator.
 * - The rebuild policy is data-driven; callers do not embed switch logic.
 * - Every switch attempt produces a fully auditable ModelSwitchResult.
 */

#pragma once

#include "llm/adapter_registry.h"
#include "llm/final_layer_orchestrator.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace themis {
namespace llm {

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------

class ModelSwitchWorkflow;

// ---------------------------------------------------------------------------
// Semantic version helper
// ---------------------------------------------------------------------------

/**
 * @brief Lightweight three-part semantic version used by the compatibility matrix.
 *
 * Comparison follows SemVer: major > minor > patch.
 * An empty/zero version compares less than any non-zero version.
 */
struct SemVer {
    int major = 0; ///< Major component (breaking changes)
    int minor = 0; ///< Minor component (backward-compatible additions)
    int patch = 0; ///< Patch component (backward-compatible fixes)

    /**
     * @brief Parse a dot-separated version string such as "3.1.0" or "2.4".
     * @param s Version string; missing components default to 0.
     * @return Parsed SemVer; on parse error all fields are 0.
     */
    [[nodiscard]] static SemVer parse(const std::string& s);

    [[nodiscard]] std::string toString() const;

    bool operator<(const SemVer& o) const noexcept;
    bool operator<=(const SemVer& o) const noexcept { return !(o < *this); }
    bool operator==(const SemVer& o) const noexcept;
    bool operator>=(const SemVer& o) const noexcept { return !(*this < o); }
    bool operator>(const SemVer& o) const noexcept { return o < *this; }

    [[nodiscard]] nlohmann::json toJson() const;
    [[nodiscard]] static SemVer fromJson(const nlohmann::json& j);
};

// ---------------------------------------------------------------------------
// Ratchet Compatibility Matrix
// ---------------------------------------------------------------------------

/**
 * @brief One row in the ratchet compatibility matrix.
 *
 * Records which (adapter_id, model_family) combination requires at least
 * @c min_model_version and optionally at most @c max_model_version_exclusive.
 * The entry is keyed by (adapter_id, model_family).
 */
struct RatchetCompatibilityEntry {
    std::string adapter_id;              ///< Registered adapter identifier
    std::string model_family;            ///< Target model family (e.g. "llama")
    SemVer      min_model_version;       ///< Inclusive lower bound (ratchet floor)
    SemVer      max_model_version_excl;  ///< Exclusive upper bound; {0,0,0} = unbounded

    /**
     * @brief Return true if @p version satisfies this entry's version range.
     * @param version Candidate model version.
     */
    [[nodiscard]] bool isSatisfiedBy(const SemVer& version) const noexcept;

    [[nodiscard]] nlohmann::json toJson() const;
    [[nodiscard]] static RatchetCompatibilityEntry fromJson(const nlohmann::json& j);
};

/**
 * @brief Versioned, ratchet-enforced compatibility matrix for LoRA packages.
 *
 * The matrix maps (adapter_id, model_family) pairs to an allowed model-version
 * range.  "Ratchet" means the minimum-required version for a given entry can
 * only be advanced forward; attempts to lower it are rejected unless the caller
 * explicitly passes @c allow_downgrade = true (operator-override path).
 *
 * The matrix carries a schema version so that persisted snapshots can be
 * validated on load.
 *
 * Thread safety: all mutating methods are NOT thread-safe.  The caller must
 * synchronize externally when shared across threads.
 */
class RatchetCompatibilityMatrix {
public:
    /**
     * @brief Construct an empty matrix with the given schema version string.
     * @param schema_version Human-readable schema version (e.g. "1.0.0").
     */
    explicit RatchetCompatibilityMatrix(std::string schema_version = "1.0.0");

    // ------------------------------------------------------------------
    // Entry management
    // ------------------------------------------------------------------

    /**
     * @brief Register or advance the minimum required version for an entry.
     *
     * If no entry exists for (adapter_id, model_family) it is created.
     * If an entry exists and @p min_version is higher than the current floor,
     * the floor is advanced (ratchet forward).
     * If @p min_version is lower than the current floor the call is rejected
     * and returns false unless @p allow_downgrade is true (operator override).
     *
     * @param adapter_id        Adapter identifier.
     * @param model_family      Target model family.
     * @param min_version       New required minimum model version.
     * @param max_version_excl  Optional exclusive upper bound; {0,0,0} = none.
     * @param allow_downgrade   When true the ratchet floor may be lowered
     *                          (requires operator approval in production).
     * @return true on success, false if the ratchet constraint would be violated.
     */
    [[nodiscard]] bool registerEntry(const std::string& adapter_id,
                                     const std::string& model_family,
                                     const SemVer& min_version,
                                     const SemVer& max_version_excl = SemVer{},
                                     bool allow_downgrade = false);

    /**
     * @brief Look up an entry by adapter_id and model_family.
     * @return Entry if found, std::nullopt otherwise.
     */
    [[nodiscard]] std::optional<RatchetCompatibilityEntry>
    findEntry(const std::string& adapter_id,
              const std::string& model_family) const;

    /**
     * @brief Check whether @p model_version satisfies all constraints for
     *        @p adapter_id targeting @p model_family.
     *
     * If no entry is registered for the pair the call returns true (open policy).
     *
     * @param adapter_id    Adapter identifier.
     * @param model_family  Target model family.
     * @param model_version Candidate model version string (parsed internally).
     * @return true if compatible or no entry registered.
     */
    [[nodiscard]] bool isCompatible(const std::string& adapter_id,
                                    const std::string& model_family,
                                    const std::string& model_version) const;

    /**
     * @brief Return all registered entries.
     */
    [[nodiscard]] const std::vector<RatchetCompatibilityEntry>& entries() const noexcept;

    /**
     * @brief Return the schema version string.
     */
    [[nodiscard]] const std::string& schemaVersion() const noexcept;

    // ------------------------------------------------------------------
    // Serialization
    // ------------------------------------------------------------------

    /**
     * @brief Serialize the full matrix to JSON.
     * @return JSON object with "schema_version" and "entries" fields.
     */
    [[nodiscard]] nlohmann::json toJson() const;

    /**
     * @brief Deserialize a matrix from a JSON object produced by toJson().
     * @param j Source JSON.
     * @return Populated RatchetCompatibilityMatrix.
     * @throws std::invalid_argument if required fields are missing.
     */
    [[nodiscard]] static RatchetCompatibilityMatrix fromJson(const nlohmann::json& j);

private:
    std::string schema_version_;
    std::vector<RatchetCompatibilityEntry> entries_;
};

// ---------------------------------------------------------------------------
// Rebuild policy
// ---------------------------------------------------------------------------

/**
 * @brief Conditions that mandate a LoRA adapter rebuild when the base model changes.
 */
enum class RebuildTrigger : uint8_t {
    /// Rebuild if the model architecture (e.g., "llama" vs "mistral") changes.
    ARCHITECTURE_CHANGE,
    /// Rebuild if the tokenizer type or vocabulary changes.
    TOKENIZER_CHANGE,
    /// Rebuild if the hidden-dimension or layer count changes.
    LAYER_DIMENSION_CHANGE,
    /// Rebuild when the model version advances past the adapter's recorded range.
    VERSION_OUT_OF_RANGE,
};

/**
 * @brief Data-driven rebuild policy for model-switch operations.
 *
 * The policy lists which changes require a rebuild and whether an unresolved
 * rebuild requirement should block the switch (fail-closed) or be surfaced
 * as a warning (fail-open/degraded).
 */
struct RebuildPolicy {
    /// Conditions that trigger a mandatory rebuild.
    std::vector<RebuildTrigger> triggers = {
        RebuildTrigger::ARCHITECTURE_CHANGE,
        RebuildTrigger::TOKENIZER_CHANGE,
        RebuildTrigger::VERSION_OUT_OF_RANGE,
    };

    /**
     * When true a triggered rebuild requirement causes the switch to fail
     * (BLOCKED result) rather than proceeding with REBUILD_REQUIRED status.
     */
    bool fail_closed_on_rebuild = false;

    /**
     * @brief Return true if at least one trigger in @p policy matches
     *        @p trigger.
     */
    [[nodiscard]] bool isTriggerActive(RebuildTrigger trigger) const noexcept;

    [[nodiscard]] nlohmann::json toJson() const;
    [[nodiscard]] static RebuildPolicy fromJson(const nlohmann::json& j);
};

// ---------------------------------------------------------------------------
// Model switch request & result
// ---------------------------------------------------------------------------

/**
 * @brief Input descriptor for a model-switch operation.
 */
struct ModelSwitchRequest {
    std::string package_id;          ///< Package being switched
    std::string source_model_name;   ///< Current base model name
    std::string source_model_version;///< Current base model version string
    std::string target_model_name;   ///< Requested target base model name
    std::string target_model_version;///< Requested target base model version string
    std::string target_model_family; ///< Target model family (e.g. "llama")

    /// Operator/request correlation identifier for audit logs.
    std::string correlation_id;

    /// When true the compatibility gate runs even if source and target are
    /// the same model version (useful for explicit re-validation).
    bool force_revalidation = false;
};

/**
 * @brief Per-check result emitted during a model-switch.
 */
struct ModelSwitchCheckResult {
    /**
     * @brief Category of the check.
     */
    enum class CheckKind : uint8_t {
        RATCHET_MATRIX,     ///< Ratchet compatibility matrix lookup
        ARCHITECTURE,       ///< Model architecture compatibility
        TOKENIZER,          ///< Tokenizer compatibility
        LAYER_DIMENSIONS,   ///< Hidden size / layer count compatibility
        QUANTIZATION,       ///< Quantization format compatibility
        REBUILD_POLICY,     ///< Rebuild policy evaluation
        PROMPT_FORMAT,      ///< Prompt/chat template compatibility
    };

    CheckKind kind;
    bool passed = false;
    bool rebuild_required = false; ///< Check triggers rebuild but does not fail
    std::string message;
};

/**
 * @brief Outcome of a model-switch attempt.
 */
enum class ModelSwitchOutcome : uint8_t {
    /// Switch succeeded; adapter is compatible with the new model.
    COMPATIBLE,
    /// Switch requires a rebuild before the adapter can be used.
    REBUILD_REQUIRED,
    /// Switch was blocked because a required rebuild is pending (fail-closed).
    BLOCKED,
    /// Switch failed due to an incompatibility that cannot be resolved by rebuild.
    INCOMPATIBLE,
};

/**
 * @brief Full result of a ModelSwitchWorkflow::executeSwitch() call.
 */
struct ModelSwitchResult {
    ModelSwitchOutcome outcome = ModelSwitchOutcome::INCOMPATIBLE;

    /// Individual check results for diagnostics.
    std::vector<ModelSwitchCheckResult> checks;

    /// Human-readable error messages (non-empty only on INCOMPATIBLE or BLOCKED).
    std::vector<std::string> errors;

    /// Non-fatal diagnostic messages.
    std::vector<std::string> warnings;

    /// Rebuild triggers that were activated (empty when outcome is COMPATIBLE).
    std::vector<RebuildTrigger> active_rebuild_triggers;

    /// Echo of the correlation_id from the request.
    std::string correlation_id;

    /**
     * @brief Return true when the adapter can serve inference immediately
     *        (COMPATIBLE outcome).
     */
    [[nodiscard]] bool canServe() const noexcept {
        return outcome == ModelSwitchOutcome::COMPATIBLE;
    }

    /**
     * @brief Return true when the switch requires a rebuild before serving.
     */
    [[nodiscard]] bool needsRebuild() const noexcept {
        return outcome == ModelSwitchOutcome::REBUILD_REQUIRED ||
               outcome == ModelSwitchOutcome::BLOCKED;
    }

    [[nodiscard]] nlohmann::json toJson() const;
};

// ---------------------------------------------------------------------------
// ModelSwitchWorkflow
// ---------------------------------------------------------------------------

/**
 * @brief Orchestrates SOP-compliant LLM base-model switches for LoRA packages.
 *
 * The workflow performs:
 * 1. Ratchet-matrix lookup — verifies the target model version satisfies the
 *    registered floor for each adapter in the package.
 * 2. Per-check compatibility gates — architecture, tokenizer, layer dimensions,
 *    quantization, and prompt-format checks via AdapterRegistry.
 * 3. Rebuild-policy evaluation — classifies the required action
 *    (COMPATIBLE / REBUILD_REQUIRED / BLOCKED / INCOMPATIBLE).
 * 4. FinalLayerOrchestrator transition — updates the package deployment state
 *    when the switch is COMPATIBLE.
 *
 * The workflow is stateless between calls; all persistent state lives in the
 * injected registry, orchestrator, and matrix.
 */
class ModelSwitchWorkflow {
public:
    /**
     * @brief Construct a workflow with the required collaborators.
     *
     * @param registry      Adapter registry used for compatibility validation.
     *                      Must not be null.
     * @param orchestrator  Final-layer orchestrator managing package lifecycle.
     *                      Must not be null.
     * @param matrix        Ratchet compatibility matrix.  A default-constructed
     *                      (empty) matrix accepts all model versions.
     * @param policy        Rebuild policy controlling which changes mandate a
     *                      rebuild and whether missing rebuilds fail-close.
     *
     * @throws std::invalid_argument if @p registry or @p orchestrator is null.
     */
    ModelSwitchWorkflow(std::shared_ptr<AdapterRegistry> registry,
                        std::shared_ptr<FinalLayerOrchestrator> orchestrator,
                        RatchetCompatibilityMatrix matrix = RatchetCompatibilityMatrix{},
                        RebuildPolicy policy = RebuildPolicy{});

    // ------------------------------------------------------------------
    // Core operation
    // ------------------------------------------------------------------

    /**
     * @brief Execute a model-switch for the package described in @p request.
     *
     * Runs the full check sequence and returns a ModelSwitchResult containing
     * the outcome, individual check details, and an audit-ready JSON report.
     *
     * Side effects (only when outcome == COMPATIBLE):
     * - Calls FinalLayerOrchestrator::promotePackage() to advance the package
     *   to its next deployment stage.
     *
     * @param request  Model-switch descriptor.
     * @return ModelSwitchResult with full diagnostics.
     */
    [[nodiscard]] ModelSwitchResult executeSwitch(const ModelSwitchRequest& request) const;

    // ------------------------------------------------------------------
    // Configuration accessors
    // ------------------------------------------------------------------

    /**
     * @brief Replace the compatibility matrix used for ratchet lookups.
     * @param matrix New matrix to install.
     */
    void setCompatibilityMatrix(RatchetCompatibilityMatrix matrix);

    /**
     * @brief Return a const reference to the active compatibility matrix.
     */
    [[nodiscard]] const RatchetCompatibilityMatrix& compatibilityMatrix() const noexcept;

    /**
     * @brief Replace the rebuild policy.
     * @param policy New policy.
     */
    void setRebuildPolicy(RebuildPolicy policy);

    /**
     * @brief Return a const reference to the active rebuild policy.
     */
    [[nodiscard]] const RebuildPolicy& rebuildPolicy() const noexcept;

    // ------------------------------------------------------------------
    // Utility
    // ------------------------------------------------------------------

    /**
     * @brief Check whether source and target models differ in a way that
     *        requires further analysis.
     *
     * Returns false when both name and version are identical (no-op switch),
     * unless @p request.force_revalidation is true.
     *
     * @param request  Model-switch request to inspect.
     * @return true if a non-trivial switch is requested.
     */
    [[nodiscard]] static bool isSwitchRequired(const ModelSwitchRequest& request) noexcept;

private:
    // ------------------------------------------------------------------
    // Per-check helpers (each returns a ModelSwitchCheckResult)
    // ------------------------------------------------------------------

    [[nodiscard]] ModelSwitchCheckResult checkRatchetMatrix(
        const std::string& adapter_id,
        const std::string& target_model_family,
        const std::string& target_model_version) const;

    [[nodiscard]] ModelSwitchCheckResult checkArchitectureCompatibility(
        const std::string& adapter_id,
        const std::string& target_model_name,
        const std::string& target_model_family,
        const std::string& target_model_version) const;

    [[nodiscard]] ModelSwitchCheckResult checkTokenizerCompatibility(
        const std::string& adapter_id,
        const std::string& target_model_name,
        const std::string& target_model_family) const;

    [[nodiscard]] ModelSwitchCheckResult checkLayerDimensions(
        const std::string& adapter_id,
        const std::string& target_model_name) const;

    [[nodiscard]] ModelSwitchCheckResult checkQuantizationCompatibility(
        const std::string& adapter_id,
        const std::string& target_model_name,
        const std::string& target_model_family,
        const std::string& target_model_version) const;

    [[nodiscard]] ModelSwitchCheckResult checkPromptFormat(
        const std::string& adapter_id,
        const std::string& target_model_name) const;

    // ------------------------------------------------------------------
    // Rebuild policy evaluation
    // ------------------------------------------------------------------

    /**
     * @brief Apply the rebuild policy to the set of checks and determine the
     *        final outcome.
     *
     * @param checks            Completed check list.
     * @param policy            Active rebuild policy.
     * @param active_triggers   [out] Set of activated rebuild triggers.
     * @return Computed ModelSwitchOutcome.
     */
    [[nodiscard]] static ModelSwitchOutcome evaluateRebuildPolicy(
        const std::vector<ModelSwitchCheckResult>& checks,
        const RebuildPolicy& policy,
        std::vector<RebuildTrigger>& active_triggers);

    std::shared_ptr<AdapterRegistry> registry_;
    std::shared_ptr<FinalLayerOrchestrator> orchestrator_;
    RatchetCompatibilityMatrix matrix_;
    RebuildPolicy policy_;
};

} // namespace llm
} // namespace themis
