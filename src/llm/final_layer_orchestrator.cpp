/**
 * @file final_layer_orchestrator.cpp
 * @brief Orchestrates final-layer package deployment, staging, and resolution with compatibility gating.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=2, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "llm/final_layer_orchestrator.h"
#include "observability/layer_decision_log.h"
#include "observability/reason_codes.h"
#include "observability/telemetry_keys.h"

#include <algorithm>
#include <string>
#include <utility>

namespace themis::llm {

namespace {

[[nodiscard]] bool isServingStage(FinalLayerDeploymentStage stage) {
    return stage == FinalLayerDeploymentStage::PRODUCTION ||
           stage == FinalLayerDeploymentStage::CANARY ||
           stage == FinalLayerDeploymentStage::PREVIOUS_KNOWN_GOOD;
}

[[nodiscard]] bool canPromote(FinalLayerDeploymentStage current,
                              FinalLayerDeploymentStage target,
                              const FinalLayerTransitionPolicy& policy) {
    if (current == target) {
        return true;
    }
    if (current == FinalLayerDeploymentStage::DRAFT &&
        target == FinalLayerDeploymentStage::STAGING) {
        return true;
    }
    if (current == FinalLayerDeploymentStage::STAGING &&
        target == FinalLayerDeploymentStage::CANARY) {
        return true;
    }
    if (current == FinalLayerDeploymentStage::CANARY &&
        target == FinalLayerDeploymentStage::PRODUCTION) {
        return true;
    }
    if (current == FinalLayerDeploymentStage::DRAFT &&
        target == FinalLayerDeploymentStage::PRODUCTION &&
        policy.allow_direct_draft_to_production) {
        return true;
    }
    return false;
}

[[nodiscard]] bool passesCompatibilityGate(const FinalLayerPackage& package,
                                           const std::shared_ptr<AdapterRegistry>& registry,
                                           const std::string& base_model_name,
                                           const std::string& base_model_version) {
    if (!registry) {
        return true;
    }

    const auto compatibility = registry->validateCompatibility(
        package.primary_adapter_id,
        base_model_name.empty() ? package.target_model_id : base_model_name,
        base_model_version.empty() ? package.base_model_version : base_model_version);
    return compatibility.compatible;
}

}

void FinalLayerOrchestrator::setAdapterRegistry(std::shared_ptr<AdapterRegistry> registry) {
    adapter_registry_ = std::move(registry);
}

void FinalLayerOrchestrator::setModelRouter(std::shared_ptr<ModelRouter> router) {
    model_router_ = std::move(router);
}

void FinalLayerOrchestrator::setTransitionPolicy(FinalLayerTransitionPolicy policy) {
    transition_policy_ = policy;
}

FinalLayerTransitionPolicy FinalLayerOrchestrator::transitionPolicy() const {
    return transition_policy_;
}

bool FinalLayerOrchestrator::registerPackage(const FinalLayerPackage& package) {
    if (package.package_id.empty() || package.target_model_id.empty() || package.primary_adapter_id.empty()) {
        return false;
    }
    return packages_.emplace(package.package_id, package).second;
}

bool FinalLayerOrchestrator::updatePackage(const FinalLayerPackage& package) {
    if (package.package_id.empty() || !packages_.contains(package.package_id)) {
        return false;
    }
    packages_[package.package_id] = package;
    return true;
}

bool FinalLayerOrchestrator::setPackageStatus(const std::string& package_id,
                                              FinalLayerPackageStatus new_status) {
    auto it = packages_.find(package_id);
    if (it == packages_.end()) {
        return false;
    }
    it->second.status = new_status;
    return true;
}

std::vector<FinalLayerPackage> FinalLayerOrchestrator::listPackages() const {
    std::vector<FinalLayerPackage> packages = {};

    packages.reserve(packages_.size());
    for (const auto& [_, package] : packages_) {
        packages.push_back(package);
    }
    return packages;
}

bool FinalLayerOrchestrator::promotePackage(const std::string& package_id,
                                            FinalLayerDeploymentStage target_stage,
                                            const std::string& base_model_name,
                                            const std::string& base_model_version) {
    auto it = packages_.find(package_id);
    if (it == packages_.end()) {
        return false;
    }

    auto& package = it->second;
    if (package.status == FinalLayerPackageStatus::DISABLED) {
        return false;
    }

    if (!canPromote(package.deployment_stage, target_stage, transition_policy_)) {
        return false;
    }

    if (transition_policy_.require_compatibility_gate) {
        if (!passesCompatibilityGate(package, adapter_registry_, base_model_name, base_model_version)) {
            return false;
        }
    }

    package.deployment_stage = target_stage;
    if (isServingStage(target_stage)) {
        package.status = FinalLayerPackageStatus::ACTIVE;
    }
    return true;
}

bool FinalLayerOrchestrator::rollbackToPackage(const std::string& source_package_id,
                                               const std::string& rollback_target_id,
                                               const std::string& base_model_name,
                                               const std::string& base_model_version) {
    if (source_package_id == rollback_target_id) {
        return false;
    }

    auto source_it = packages_.find(source_package_id);
    auto target_it = packages_.find(rollback_target_id);
    if (source_it == packages_.end() || target_it == packages_.end()) {
        return false;
    }

    auto& source = source_it->second;
    auto& target = target_it->second;

    if (transition_policy_.require_compatibility_gate) {
        if (!passesCompatibilityGate(target, adapter_registry_, base_model_name, base_model_version)) {
            return false;
        }
    }

    source.status = FinalLayerPackageStatus::DEPRECATED;
    source.deployment_stage = FinalLayerDeploymentStage::STAGING;

    target.status = FinalLayerPackageStatus::ACTIVE;
    target.deployment_stage = FinalLayerDeploymentStage::PREVIOUS_KNOWN_GOOD;
    return true;
}

FinalLayerResolution FinalLayerOrchestrator::resolve(const FinalLayerRequest& request) const {
    FinalLayerResolution resolution;
    resolution.correlation_id = request.correlation_id;
    resolution.confidence_policy_version = request.confidence_policy_version;
    resolution.confidence_threshold_key = request.confidence_threshold_key;
    resolution.escalation_source_layer = request.escalation_source_layer;
    resolution.fallback_mode = std::string(observability::reason_codes::fallback_mode::kNone);

    auto emit_resolution = [&resolution]() {
        observability::emitLayerDecisionLog(
            observability::telemetry::layers::kFinalLayer,
            resolution.correlation_id,
            resolution.routing_reason_code,
            resolution.confidence_policy_version.empty()
                ? std::string(observability::reason_codes::kPolicyVersionDefault)
                : resolution.confidence_policy_version,
            resolution.confidence_threshold_key.empty()
                ? std::string(observability::reason_codes::tensor_rag::kThresholdKeyNone)
                : resolution.confidence_threshold_key,
            resolution.fallback_mode,
            resolution.fallback_reason_code,
            resolution.escalation_source_layer.empty()
                ? std::string(observability::telemetry::layers::kFinalLayer)
                : resolution.escalation_source_layer,
            resolution.resolved);
    };

    const FinalLayerPackage* package = nullptr;
    if (!request.requested_package_id.empty()) {
        package = findPackageById(request.requested_package_id);
        if (!package) {
            resolution.routing_reason_code = std::string(observability::reason_codes::final_layer::kPackageNotFound);
            resolution.fallback_mode = std::string(observability::reason_codes::fallback_mode::kFailClosed);
            resolution.fallback_reason_code = std::string(observability::reason_codes::final_layer::kFallbackPackageNotFound);
            resolution.errors.push_back("Requested package '" + request.requested_package_id + "' not found");
            emit_resolution();
            return resolution;
        }
    } else if (model_router_) {
        const auto routed = model_router_->route(request.prompt, request.metadata);
        if (routed.matched) {
            package = findActivePackageForModel(routed.model_id);
            resolution.routing_reason = "model router matched rule '" + routed.rule_id + "'";
            resolution.routing_reason_code = std::string(observability::reason_codes::final_layer::kRoutedByModelRule);
            resolution.model_id = routed.model_id;
        }
    }

    if (!package) {
        if (static_cast<int>(packages_.size()) == 1u) {
            package = &packages_.begin()->second;
            if (resolution.routing_reason_code.empty()) {
                resolution.routing_reason_code = std::string(observability::reason_codes::final_layer::kSinglePackageSelected);
            }
        } else {
            resolution.routing_reason_code = std::string(observability::reason_codes::final_layer::kPackageUnresolved);
            resolution.fallback_mode = std::string(observability::reason_codes::fallback_mode::kFailClosed);
            resolution.fallback_reason_code = std::string(observability::reason_codes::final_layer::kFallbackPackageSelectionFailed);
            resolution.errors.push_back("No final-layer package could be selected");
            emit_resolution();
            return resolution;
        }
    }

    if (package->status != FinalLayerPackageStatus::ACTIVE) {
        resolution.routing_reason_code = std::string(observability::reason_codes::final_layer::kPackageNotActive);
        resolution.fallback_mode = std::string(observability::reason_codes::fallback_mode::kFailClosed);
        resolution.fallback_reason_code = std::string(observability::reason_codes::final_layer::kFallbackPackageNotActive);
        resolution.errors.push_back("Package '" + package->package_id + "' is not active");
        emit_resolution();
        return resolution;
    }

    if (!isServingStage(package->deployment_stage)) {
        resolution.routing_reason_code = std::string(observability::reason_codes::final_layer::kPackageNotDeployable);
        resolution.fallback_mode = std::string(observability::reason_codes::fallback_mode::kFailClosed);
        resolution.fallback_reason_code = std::string(observability::reason_codes::final_layer::kFallbackPackageNotDeployable);
        resolution.errors.push_back("Package '" + package->package_id + "' is not in a serving deployment stage");
        emit_resolution();
        return resolution;
    }

    resolution.package_id = package->package_id;
    resolution.model_id = package->target_model_id;
    resolution.primary_adapter_id = package->primary_adapter_id;

    if (adapter_registry_) {
        auto compatibility = adapter_registry_->validateCompatibility(
            package->primary_adapter_id,
            request.base_model_name.empty() ? package->target_model_id : request.base_model_name,
            request.base_model_version.empty() ? package->base_model_version : request.base_model_version);
        if (!compatibility.compatible) {
            resolution.routing_reason_code = std::string(observability::reason_codes::final_layer::kCompatibilityRejected);
            resolution.fallback_mode = std::string(observability::reason_codes::fallback_mode::kFailClosed);
            resolution.fallback_reason_code = std::string(observability::reason_codes::final_layer::kFallbackCompatibilityRejected);
            resolution.errors = compatibility.errors;
            resolution.warnings = compatibility.warnings;
            emit_resolution();
            return resolution;
        }
        resolution.warnings = compatibility.warnings;

        if (request.allow_draft_adapter) {
            if (!package->draft_adapter_id.empty()) {
                resolution.draft_adapter_id = package->draft_adapter_id;
            } else if (!package->model_family.empty()) {
                auto draft = adapter_registry_->findDraftAdapterForFamily(package->model_family);
                if (draft.has_value()) {
                    resolution.draft_adapter_id = draft->adapter_id;
                }
            }
        }
    }

    if (resolution.routing_reason.empty()) {
        resolution.routing_reason = "final-layer package selected directly";
    }
    if (resolution.routing_reason_code.empty()) {
        resolution.routing_reason_code = std::string(observability::reason_codes::final_layer::kSelectedDirect);
    }
    resolution.resolved = true;
    emit_resolution();
    return resolution;
}

std::vector<FinalLayerCompatibilityRow> FinalLayerOrchestrator::buildCompatibilityMatrix(
    const std::string& package_id) const {
    std::vector<FinalLayerCompatibilityRow> rows;
    const auto* package = findPackageById(package_id);
    if (!package || !adapter_registry_) {
        return rows;
    }

    auto add_row = [&]([[maybe_unused]] const std::string& adapter_id) {
        if (adapter_id.empty()) {
            return;
        }
        FinalLayerCompatibilityRow row;
        row.adapter_id = adapter_id;
        row.target_model_id = package->target_model_id;
        auto validation = adapter_registry_->validateCompatibility(
            adapter_id,
            package->target_model_id,
            package->base_model_version);
        row.compatible = validation.compatible;
        row.errors = std::move(validation.errors);
        row.warnings = std::move(validation.warnings);
        rows.push_back(std::move(row));
    };

    add_row(package->primary_adapter_id);
    add_row(package->draft_adapter_id);
    return rows;
}

const FinalLayerPackage* FinalLayerOrchestrator::findPackageById(const std::string& package_id) const {
    auto it = packages_.find(package_id);
    return (it != packages_.end()) ? &it->second : nullptr;
}

const FinalLayerPackage* FinalLayerOrchestrator::findActivePackageForModel(const std::string& model_id) const {
    for (const auto& [_, package] : packages_) {
        if (package.target_model_id == model_id && package.status == FinalLayerPackageStatus::ACTIVE) {
            return &package;
        }
    }
    return nullptr;
}

} // namespace themis::llm
