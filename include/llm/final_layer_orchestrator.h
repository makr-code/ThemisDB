/**
 * @file final_layer_orchestrator.h
 * @brief Orchestrates final-layer package deployment, staging, and resolution with compatibility gating.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
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

enum class FinalLayerPackageStatus : uint8_t {
    ACTIVE,
    DEPRECATED,
    DISABLED,
};

enum class FinalLayerDeploymentStage : uint8_t {
    DRAFT,
    STAGING,
    CANARY,
    PRODUCTION,
    PREVIOUS_KNOWN_GOOD,
};

struct FinalLayerTransitionPolicy {
    bool require_compatibility_gate = true;
    bool allow_direct_draft_to_production = false;
};

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

struct FinalLayerCompatibilityRow {
    std::string adapter_id;
    std::string target_model_id;
    bool compatible = false;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

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

class FinalLayerOrchestrator {
public:
    FinalLayerOrchestrator() = default;
    ~FinalLayerOrchestrator() = default;

    /**
     * @brief Set Adapter Registry.
     * @param[in] registry Input parameter.
     */
    void setAdapterRegistry(std::shared_ptr<AdapterRegistry> registry);
    /**
     * @brief Set Model Router.
     * @param[in] router Input parameter.
     */
    void setModelRouter(std::shared_ptr<ModelRouter> router);
    /**
     * @brief Set Transition Policy.
     * @param[in] policy Input parameter.
     */
    void setTransitionPolicy(FinalLayerTransitionPolicy policy);
    [[nodiscard]] FinalLayerTransitionPolicy transitionPolicy() const;

    [[nodiscard]] bool registerPackage(const FinalLayerPackage& package);
    [[nodiscard]] bool updatePackage(const FinalLayerPackage& package);
    /**
     * @brief Set Package Status.
     * @param[in] package_id Identifier of the package.
     * @param[in] new_status Input parameter.
     * @return True when the operation succeeds.
     */
    bool setPackageStatus(const std::string& package_id,
                          FinalLayerPackageStatus new_status);
    [[nodiscard]] std::vector<FinalLayerPackage> listPackages() const;

    [[nodiscard]] bool promotePackage(const std::string& package_id,
                                      FinalLayerDeploymentStage target_stage,
                                      const std::string& base_model_name = {},
                                      const std::string& base_model_version = {});

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
