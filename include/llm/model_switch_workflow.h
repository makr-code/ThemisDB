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

struct SemVer {
    int major = 0; ///< Major component (breaking changes)
    int minor = 0; ///< Minor component (backward-compatible additions)
    int patch = 0; ///< Patch component (backward-compatible fixes)

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

struct RatchetCompatibilityEntry {
    std::string adapter_id;              ///< Registered adapter identifier
    std::string model_family;            ///< Target model family (e.g. "llama")
    SemVer      min_model_version;       ///< Inclusive lower bound (ratchet floor)
    SemVer      max_model_version_excl;  ///< Exclusive upper bound; {0,0,0} = unbounded

    [[nodiscard]] bool isSatisfiedBy(const SemVer& version) const noexcept;

    [[nodiscard]] nlohmann::json toJson() const;
    [[nodiscard]] static RatchetCompatibilityEntry fromJson(const nlohmann::json& j);
};

class RatchetCompatibilityMatrix {
public:
    explicit RatchetCompatibilityMatrix(std::string schema_version = "1.0.0");

    // ------------------------------------------------------------------
    // Entry management
    // ------------------------------------------------------------------

    [[nodiscard]] bool registerEntry(const std::string& adapter_id,
                                     const std::string& model_family,
                                     const SemVer& min_version,
                                     const SemVer& max_version_excl = SemVer{},
                                     bool allow_downgrade = false);

    [[nodiscard]] std::optional<RatchetCompatibilityEntry>
    findEntry(const std::string& adapter_id,
              const std::string& model_family) const;

    [[nodiscard]] bool isCompatible(const std::string& adapter_id,
                                    const std::string& model_family,
                                    const std::string& model_version) const;

    [[nodiscard]] const std::vector<RatchetCompatibilityEntry>& entries() const noexcept;

    [[nodiscard]] const std::string& schemaVersion() const noexcept;

    // ------------------------------------------------------------------
    // Serialization
    // ------------------------------------------------------------------

    [[nodiscard]] nlohmann::json toJson() const;

    [[nodiscard]] static RatchetCompatibilityMatrix fromJson(const nlohmann::json& j);

private:
    std::string schema_version_;
    std::vector<RatchetCompatibilityEntry> entries_;
};

// ---------------------------------------------------------------------------
// Rebuild policy
// ---------------------------------------------------------------------------

enum class RebuildTrigger : uint8_t {
    ARCHITECTURE_CHANGE,
    TOKENIZER_CHANGE,
    LAYER_DIMENSION_CHANGE,
    VERSION_OUT_OF_RANGE,
};

struct RebuildPolicy {
    std::vector<RebuildTrigger> triggers = {
        RebuildTrigger::ARCHITECTURE_CHANGE,
        RebuildTrigger::TOKENIZER_CHANGE,
        RebuildTrigger::VERSION_OUT_OF_RANGE,
    };

    bool fail_closed_on_rebuild = false;

    [[nodiscard]] bool isTriggerActive(RebuildTrigger trigger) const noexcept;

    [[nodiscard]] nlohmann::json toJson() const;
    [[nodiscard]] static RebuildPolicy fromJson(const nlohmann::json& j);
};

// ---------------------------------------------------------------------------
// Model switch request & result
// ---------------------------------------------------------------------------

struct ModelSwitchRequest {
    std::string package_id;          ///< Package being switched
    std::string source_model_name;   ///< Current base model name
    std::string source_model_version;///< Current base model version string
    std::string target_model_name;   ///< Requested target base model name
    std::string target_model_version;///< Requested target base model version string
    std::string target_model_family; ///< Target model family (e.g. "llama")

    std::string correlation_id;

    bool force_revalidation = false;
};

struct ModelSwitchCheckResult {
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

enum class ModelSwitchOutcome : uint8_t {
    COMPATIBLE,
    REBUILD_REQUIRED,
    BLOCKED,
    INCOMPATIBLE,
};

struct ModelSwitchResult {
    ModelSwitchOutcome outcome = ModelSwitchOutcome::INCOMPATIBLE;

    std::vector<ModelSwitchCheckResult> checks;

    std::vector<std::string> errors;

    std::vector<std::string> warnings;

    std::vector<RebuildTrigger> active_rebuild_triggers;

    std::string correlation_id;

    [[nodiscard]] bool canServe() const noexcept {
        return outcome == ModelSwitchOutcome::COMPATIBLE;
    }

    [[nodiscard]] bool needsRebuild() const noexcept {
        return outcome == ModelSwitchOutcome::REBUILD_REQUIRED ||
               outcome == ModelSwitchOutcome::BLOCKED;
    }

    [[nodiscard]] nlohmann::json toJson() const;
};

// ---------------------------------------------------------------------------
// ModelSwitchWorkflow
// ---------------------------------------------------------------------------

class ModelSwitchWorkflow {
public:
    ModelSwitchWorkflow(std::shared_ptr<AdapterRegistry> registry,
                        std::shared_ptr<FinalLayerOrchestrator> orchestrator,
                        RatchetCompatibilityMatrix matrix = RatchetCompatibilityMatrix{},
                        RebuildPolicy policy = RebuildPolicy{});

    // ------------------------------------------------------------------
    // Core operation
    // ------------------------------------------------------------------

    [[nodiscard]] ModelSwitchResult executeSwitch(const ModelSwitchRequest& request) const;

    // ------------------------------------------------------------------
    // Configuration accessors
    // ------------------------------------------------------------------

    /**
     * @brief Set Compatibility Matrix.
     * @param[in] matrix Input parameter.
     */
    void setCompatibilityMatrix(RatchetCompatibilityMatrix matrix);

    [[nodiscard]] const RatchetCompatibilityMatrix& compatibilityMatrix() const noexcept;

    /**
     * @brief Set Rebuild Policy.
     * @param[in] policy Input parameter.
     */
    void setRebuildPolicy(RebuildPolicy policy);

    [[nodiscard]] const RebuildPolicy& rebuildPolicy() const noexcept;

    // ------------------------------------------------------------------
    // Utility
    // ------------------------------------------------------------------

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
