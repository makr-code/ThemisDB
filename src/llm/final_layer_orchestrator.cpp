#include "llm/final_layer_orchestrator.h"

#include <utility>

namespace themis::llm {

void FinalLayerOrchestrator::setAdapterRegistry(std::shared_ptr<AdapterRegistry> registry) {
    adapter_registry_ = std::move(registry);
}

void FinalLayerOrchestrator::setModelRouter(std::shared_ptr<ModelRouter> router) {
    model_router_ = std::move(router);
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
    std::vector<FinalLayerPackage> packages;
    packages.reserve(packages_.size());
    for (const auto& [_, package] : packages_) {
        packages.push_back(package);
    }
    return packages;
}

FinalLayerResolution FinalLayerOrchestrator::resolve(const FinalLayerRequest& request) const {
    FinalLayerResolution resolution;

    const FinalLayerPackage* package = nullptr;
    if (!request.requested_package_id.empty()) {
        package = findPackageById(request.requested_package_id);
        if (!package) {
            resolution.errors.push_back("Requested package '" + request.requested_package_id + "' not found");
            return resolution;
        }
    } else if (model_router_) {
        const auto routed = model_router_->route(request.prompt, request.metadata);
        if (routed.matched) {
            package = findActivePackageForModel(routed.model_id);
            resolution.routing_reason = "model router matched rule '" + routed.rule_id + "'";
            resolution.model_id = routed.model_id;
        }
    }

    if (!package) {
        if (packages_.size() == 1u) {
            package = &packages_.begin()->second;
        } else {
            resolution.errors.push_back("No final-layer package could be selected");
            return resolution;
        }
    }

    if (package->status != FinalLayerPackageStatus::ACTIVE) {
        resolution.errors.push_back("Package '" + package->package_id + "' is not active");
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
            resolution.errors = compatibility.errors;
            resolution.warnings = compatibility.warnings;
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
    resolution.resolved = true;
    return resolution;
}

std::vector<FinalLayerCompatibilityRow> FinalLayerOrchestrator::buildCompatibilityMatrix(
    const std::string& package_id) const {
    std::vector<FinalLayerCompatibilityRow> rows;
    const auto* package = findPackageById(package_id);
    if (!package || !adapter_registry_) {
        return rows;
    }

    auto add_row = [&](const std::string& adapter_id) {
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
