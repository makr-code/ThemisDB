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

    [[nodiscard]] bool registerPackage(const FinalLayerPackage& package);
    [[nodiscard]] bool updatePackage(const FinalLayerPackage& package);
    bool setPackageStatus(const std::string& package_id,
                          FinalLayerPackageStatus new_status);
    [[nodiscard]] std::vector<FinalLayerPackage> listPackages() const;

    [[nodiscard]] FinalLayerResolution resolve(const FinalLayerRequest& request) const;
    [[nodiscard]] std::vector<FinalLayerCompatibilityRow> buildCompatibilityMatrix(
        const std::string& package_id) const;

private:
    [[nodiscard]] const FinalLayerPackage* findPackageById(const std::string& package_id) const;
    [[nodiscard]] const FinalLayerPackage* findActivePackageForModel(const std::string& model_id) const;

    std::shared_ptr<AdapterRegistry> adapter_registry_;
    std::shared_ptr<ModelRouter> model_router_;
    std::unordered_map<std::string, FinalLayerPackage> packages_;
};

} // namespace llm
} // namespace themis
