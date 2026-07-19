/**
 * @file final_layer_orchestrator.h
 * @brief Orchestrates final-layer package deployment, staging, and resolution with compatibility gating.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "llm/adapter_registry.h"
#include "llm/model_router.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

namespace themis {
namespace llm {

/**
 * @brief Lifecycle state of one final-layer package.
 */
enum class FinalLayerPackageStatus : uint8_t {
    ACTIVE,
    DEPRECATED,
    DISABLED,
};

/**
 * @brief Deployment stage of one final-layer package.
 *
 * This stage models promotion and rollback workflows independent from
 * package status. Status controls generic availability, while deployment
 * stage controls governance transitions.
 */
enum class FinalLayerDeploymentStage : uint8_t {
    DRAFT,
    STAGING,
    CANARY,
    PRODUCTION,
    PREVIOUS_KNOWN_GOOD,
};

/**
 * @brief Promotion/rollback governance policy.
 */
struct FinalLayerTransitionPolicy {
    bool require_compatibility_gate = true;
    bool allow_direct_draft_to_production = false;
};

/**
 * @brief Package-oriented unit of deployment for the final LLM/LoRA layer.
 */
struct FinalLayerPackage {
    std::string package_id;
    std::string target_model_id;
    std::string model_family;
    std::string base_model_version;
    std::string primary_adapter_id;
    std::string draft_adapter_id;
    std::string domain;
    std::string task_type;
    FinalLayerPackageStatus status = FinalLayerPackageStatus::ACTIVE;
    FinalLayerDeploymentStage deployment_stage = FinalLayerDeploymentStage::PRODUCTION;
};

/**
 * @brief Input for final-layer resolution.
 */
struct FinalLayerRequest {
    std::string prompt;
    nlohmann::json metadata = nlohmann::json::object();
    std::string requested_package_id;
    std::string base_model_name;
    std::string base_model_version;
    std::string correlation_id;
    std::string confidence_policy_version;
    std::string confidence_threshold_key;
    std::string upstream_routing_reason_code;
    std::string escalation_source_layer;
    bool allow_draft_adapter = true;
};

/**
 * @brief One compatibility row in the final-layer compatibility matrix.
 */
struct FinalLayerCompatibilityRow {
    std::string adapter_id;
    std::string target_model_id;
    bool compatible = false;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

/**
 * @brief Resolution result for one final-layer request.
 */
struct FinalLayerResolution {
    bool resolved = false;
    std::string model_id;
    std::string package_id;
    std::string primary_adapter_id;
    std::string draft_adapter_id;
    std::string routing_reason;
    std::string routing_reason_code;
    std::string correlation_id;
    std::string confidence_policy_version;
    std::string confidence_threshold_key;
    std::string fallback_mode;
    std::string fallback_reason_code;
    std::string escalation_source_layer;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

/**
 * @brief Final LLM/LoRA orchestration layer above ANN, Tensor, and Graph stages.
 *
 * Responsibilities:
 * - package-oriented adapter lifecycle management
 * - request-to-model routing via ModelRouter
 * - compatibility validation against target base models
 * - draft-adapter discovery for model-family-specific speculative decoding
 */
class FinalLayerOrchestrator {
public:
    FinalLayerOrchestrator() = default;
    ~FinalLayerOrchestrator() = default;

    void setAdapterRegistry(std::shared_ptr<AdapterRegistry> registry);
    void setModelRouter(std::shared_ptr<ModelRouter> router);
    void setTransitionPolicy(FinalLayerTransitionPolicy policy);
    [[nodiscard]] FinalLayerTransitionPolicy transitionPolicy() const;

    [[nodiscard]] bool registerPackage(const FinalLayerPackage& package);
    [[nodiscard]] bool updatePackage(const FinalLayerPackage& package);
    bool setPackageStatus(const std::string& package_id,
                          FinalLayerPackageStatus new_status);
    [[nodiscard]] std::vector<FinalLayerPackage> listPackages() const;

    /**
     * @brief Promote one package to the requested deployment stage.
     *
     * Enforces a governance state-machine and optional compatibility gate.
     * Allowed transitions by default:
     * - DRAFT -> STAGING
     * - STAGING -> CANARY
     * - CANARY -> PRODUCTION
     *
     * Direct DRAFT -> PRODUCTION can be enabled via policy.
     *
     * @param package_id          package to promote
     * @param target_stage        requested destination stage
     * @param base_model_name     optional override for compatibility check
     * @param base_model_version  optional override for compatibility check
     * @return true on successful transition; false otherwise
     */
    [[nodiscard]] bool promotePackage(const std::string& package_id,
                                      FinalLayerDeploymentStage target_stage,
                                      const std::string& base_model_name = {},
                                      const std::string& base_model_version = {});

    /**
     * @brief Roll back traffic from one package to another known-good package.
     *
     * Source package is demoted to STAGING and marked DEPRECATED.
     * Target package is promoted to PREVIOUS_KNOWN_GOOD and marked ACTIVE.
     *
     * @param source_package_id      currently serving package
     * @param rollback_target_id     package to become serving target
     * @param base_model_name        optional compatibility override
     * @param base_model_version     optional compatibility override
     * @return true on successful rollback transition; false otherwise
     */
    [[nodiscard]] bool rollbackToPackage(const std::string& source_package_id,
                                         const std::string& rollback_target_id,
                                         const std::string& base_model_name = {},
                                         const std::string& base_model_version = {});

    [[nodiscard]] FinalLayerResolution resolve(const FinalLayerRequest& request) const;
    [[nodiscard]] std::vector<FinalLayerCompatibilityRow> buildCompatibilityMatrix(
        const std::string& package_id) const;

private:
    [[nodiscard]] const FinalLayerPackage* findPackageById(const std::string& package_id) const;
    [[nodiscard]] const FinalLayerPackage* findActivePackageForModel(const std::string& model_id) const;

    std::shared_ptr<AdapterRegistry> adapter_registry_;
    std::shared_ptr<ModelRouter> model_router_;
    std::unordered_map<std::string, FinalLayerPackage> packages_;
    FinalLayerTransitionPolicy transition_policy_{};
};

} // namespace llm
} // namespace themis
