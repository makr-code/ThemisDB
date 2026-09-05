#include "../include/hardware_profile.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <utility>

namespace themis::evaluation {
namespace {

[[nodiscard]] std::string normalizeToken(std::string_view value) {
    std::string normalized = {};
    normalized.reserve(value.size());
    for (const auto ch : value) {
        const auto lower = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        if (lower == ' ' || lower == '-' || lower == '/') {
            normalized.push_back('_');
            continue;
        }
        normalized.push_back(lower);
    }
    return normalized;
}

[[nodiscard]] bool containsTier(const std::vector<StorageTier>& tiers, StorageTier tier) {
    return std::find(tiers.begin(), tiers.end(), tier) != tiers.end();
}

[[nodiscard]] bool layerCovered(const std::vector<LayerSizingRule>& rules, LayerId layer) {
    return std::find_if(rules.begin(), rules.end(), [&](const auto& rule) {
               return rule.layer == layer;
           }) != rules.end();
}

[[nodiscard]] std::vector<LayerSizingRule> developmentLayerRules() {
    return {
        {LayerId::AnnFrontdoor, StorageTier::Hot, 16, 24, 256, 0, 5},
        {LayerId::TensorMidLayer, StorageTier::Warm, 8, 16, 256, 0, 0},
        {LayerId::GraphTruthLayer, StorageTier::Hot, 8, 12, 128, 0, 0},
        {LayerId::LlmFinalLayer, StorageTier::Hot, 8, 16, 128, 8, 0},
    };
}

[[nodiscard]] std::vector<LayerSizingRule> productionLayerRules() {
    return {
        {LayerId::AnnFrontdoor, StorageTier::Hot, 64, 96, 1024, 0, 10},
        {LayerId::TensorMidLayer, StorageTier::Warm, 32, 64, 1024, 0, 0},
        {LayerId::GraphTruthLayer, StorageTier::Hot, 24, 48, 512, 0, 0},
        {LayerId::LlmFinalLayer, StorageTier::Hot, 16, 32, 256, 24, 0},
    };
}

[[nodiscard]] std::vector<LayerSizingRule> federatedLayerRules() {
    return {
        {LayerId::AnnFrontdoor, StorageTier::Warm, 96, 160, 2048, 0, 20},
        {LayerId::TensorMidLayer, StorageTier::Warm, 64, 128, 2048, 0, 0},
        {LayerId::GraphTruthLayer, StorageTier::Hot, 48, 96, 1024, 0, 0},
        {LayerId::LlmFinalLayer, StorageTier::Hot, 32, 64, 512, 48, 0},
    };
}

} // namespace

std::string toString(DeploymentProfileId profile_id) {
    switch (profile_id) {
        case DeploymentProfileId::Development:
            return "development";
        case DeploymentProfileId::Production:
            return "production";
        case DeploymentProfileId::HighPerformanceFederated:
            return "high_performance_federated";
    }
    return "development";
}

std::string toString(StorageTier tier) {
    switch (tier) {
        case StorageTier::Hot:
            return "hot";
        case StorageTier::Warm:
            return "warm";
        case StorageTier::Cold:
            return "cold";
    }
    return "hot";
}

std::string toString(NetworkFabric fabric) {
    switch (fabric) {
        case NetworkFabric::Workstation:
            return "workstation";
        case NetworkFabric::Datacenter:
            return "datacenter";
        case NetworkFabric::HighBandwidthFabric:
            return "high_bandwidth_fabric";
    }
    return "workstation";
}

std::string toString(AcceleratorClass accelerator_class) {
    switch (accelerator_class) {
        case AcceleratorClass::CpuOnly:
            return "cpu_only";
        case AcceleratorClass::OptionalGpu:
            return "optional_gpu";
        case AcceleratorClass::RequiredGpu:
            return "required_gpu";
    }
    return "cpu_only";
}

std::string toString(LayerId layer) {
    switch (layer) {
        case LayerId::AnnFrontdoor:
            return "ann_frontdoor";
        case LayerId::TensorMidLayer:
            return "tensor_mid_layer";
        case LayerId::GraphTruthLayer:
            return "graph_truth_layer";
        case LayerId::LlmFinalLayer:
            return "llm_final_layer";
    }
    return "ann_frontdoor";
}

std::optional<DeploymentProfileId> parseDeploymentProfileId(std::string_view value) {
    const auto normalized = normalizeToken(value);
    if (normalized == "development" || normalized == "dev") {
        return DeploymentProfileId::Development;
    }
    if (normalized == "production" || normalized == "prod") {
        return DeploymentProfileId::Production;
    }
    if (normalized == "high_performance_federated" || normalized == "high_perf_federated" ||
        normalized == "highperformancefederated" || normalized == "federated") {
        return DeploymentProfileId::HighPerformanceFederated;
    }
    return std::nullopt;
}

std::optional<StorageTier> parseStorageTier(std::string_view value) {
    const auto normalized = normalizeToken(value);
    if (normalized == "hot") {
        return StorageTier::Hot;
    }
    if (normalized == "warm") {
        return StorageTier::Warm;
    }
    if (normalized == "cold") {
        return StorageTier::Cold;
    }
    return std::nullopt;
}

std::vector<HardwareProfile> defaultHardwareProfiles() {
    return {
        {
            DeploymentProfileId::Development,
            "development",
            "Local development and functional validation profile for reduced layered workloads.",
            {8, 16},
            {32, 64},
            {1, 1},
            {512, 1024},
            {0, 1},
            {0, 12},
            {1, 10},
            AcceleratorClass::OptionalGpu,
            NetworkFabric::Workstation,
            {64, 512, false, false},
            {StorageTier::Hot, StorageTier::Warm, StorageTier::Cold},
            developmentLayerRules(),
        },
        {
            DeploymentProfileId::Production,
            "production",
            "Single-instance production profile for ANN, tensor, graph, and LLM integration.",
            {16, 32},
            {128, 256},
            {2, 4},
            {2048, 4096},
            {1, 2},
            {24, 48},
            {10, 25},
            AcceleratorClass::RequiredGpu,
            NetworkFabric::Datacenter,
            {256, 2048, true, true},
            {StorageTier::Hot, StorageTier::Warm, StorageTier::Cold},
            productionLayerRules(),
        },
        {
            DeploymentProfileId::HighPerformanceFederated,
            "high_performance_federated",
            "Distributed high-performance/federated profile for larger shards, heavier RAG, and multi-GPU LLM workloads.",
            {32, 64},
            {256, 512},
            {4, 8},
            {4096, 16384},
            {2, 4},
            {48, 80},
            {100, 200},
            AcceleratorClass::RequiredGpu,
            NetworkFabric::HighBandwidthFabric,
            {512, 4096, true, true},
            {StorageTier::Hot, StorageTier::Warm, StorageTier::Cold},
            federatedLayerRules(),
        },
    };
}

HardwareProfileValidationResult validateHardwareProfile(const HardwareProfile& profile) {
    HardwareProfileValidationResult result;

    auto validate_band = [&](const ResourceSizingBand& band, std::string_view label) {
        if (band.minimum == 0 && band.recommended == 0) {
            result.errors.push_back(std::string(label) + " sizing band is missing");
            return;
        }
        if (band.recommended < band.minimum) {
            result.errors.push_back(
                std::string(label) + " recommended value is below the minimum sizing"
            );
        }
    };

    if (profile.canonical_name.empty()) {
        result.errors.push_back("canonical_name must not be empty");
    }

    validate_band(profile.cpu_cores, "cpu_cores");
    validate_band(profile.ram_gib, "ram_gib");
    validate_band(profile.nvme_devices, "nvme_devices");
    validate_band(profile.nvme_capacity_gib, "nvme_capacity_gib");
    validate_band(profile.network_gbps, "network_gbps");

    if (profile.accelerator_class == AcceleratorClass::RequiredGpu) {
        validate_band(profile.gpu_count, "gpu_count");
        validate_band(profile.gpu_vram_gib, "gpu_vram_gib");
        if (profile.gpu_count.minimum == 0 || profile.gpu_vram_gib.minimum == 0) {
            result.errors.push_back("required_gpu profiles must define non-zero minimum GPU resources");
        }
    } else if (profile.gpu_vram_gib.recommended < profile.gpu_vram_gib.minimum) {
        result.errors.push_back("gpu_vram_gib recommended value is below the minimum sizing");
    }

    if (profile.supported_tiers.empty()) {
        result.errors.push_back("supported_tiers must not be empty");
    } else {
        std::set<StorageTier> unique_tiers(profile.supported_tiers.begin(), profile.supported_tiers.end());
        if (unique_tiers.size() != profile.supported_tiers.size()) {
            result.errors.push_back("supported_tiers contains duplicate entries");
        }
        for (const auto required_tier : {StorageTier::Hot, StorageTier::Warm, StorageTier::Cold}) {
            if (!containsTier(profile.supported_tiers, required_tier)) {
                result.errors.push_back(
                    "supported_tiers must cover hot, warm, and cold for layered deployments"
                );
                break;
            }
        }
    }

    if (profile.tiering.warm_tier_max_gib < profile.tiering.hot_tier_max_gib) {
        result.errors.push_back("warm_tier_max_gib must be greater than or equal to hot_tier_max_gib");
    }

    for (const auto layer : {LayerId::AnnFrontdoor, LayerId::TensorMidLayer, LayerId::GraphTruthLayer, LayerId::LlmFinalLayer}) {
        if (!layerCovered(profile.layer_rules, layer)) {
            result.errors.push_back("missing layer sizing rule for " + toString(layer));
        }
    }

    for (const auto& rule : profile.layer_rules) {
        if (!containsTier(profile.supported_tiers, rule.preferred_tier)) {
            result.errors.push_back(
                "layer rule '" + toString(rule.layer) + "' references an unsupported tier"
            );
        }
        if (rule.recommended_ram_gib < rule.minimum_ram_gib) {
            result.errors.push_back(
                "layer rule '" + toString(rule.layer) +
                "' has recommended RAM below the minimum sizing"
            );
        }
        if (rule.layer == LayerId::AnnFrontdoor && rule.diskann_break_even_million_vectors == 0) {
            result.errors.push_back("ANN layer must define a DiskANN break-even threshold");
        }
        if (rule.layer == LayerId::LlmFinalLayer &&
            profile.accelerator_class == AcceleratorClass::RequiredGpu &&
            rule.recommended_gpu_vram_gib == 0) {
            result.errors.push_back(
                "LLM layer requires GPU VRAM guidance when the profile requires GPUs"
            );
        }
        if (rule.recommended_gpu_vram_gib > profile.gpu_vram_gib.recommended) {
            result.errors.push_back(
                "layer rule '" + toString(rule.layer) +
                "' requires more GPU VRAM than the profile recommends"
            );
        }
    }

    if (profile.network_fabric == NetworkFabric::HighBandwidthFabric &&
        profile.network_gbps.minimum < 100) {
        result.errors.push_back("high-bandwidth fabric profiles must expose at least 100 Gbps");
    }
    if (profile.network_fabric == NetworkFabric::Datacenter &&
        profile.network_gbps.minimum < 10) {
        result.errors.push_back("datacenter profiles must expose at least 10 Gbps");
    }

    return result;
}

const HardwareProfile* findHardwareProfile(
    std::span<const HardwareProfile> profiles,
    DeploymentProfileId profile_id
) {
    const auto it = std::find_if(profiles.begin(), profiles.end(), [&](const auto& profile) {
        return profile.id == profile_id;
    });
    return it == profiles.end() ? nullptr : &(*it);
}

const HardwareProfile* findHardwareProfile(
    std::span<const HardwareProfile> profiles,
    std::string_view profile_name
) {
    const auto profile_id = parseDeploymentProfileId(profile_name);
    return profile_id ? findHardwareProfile(profiles, *profile_id) : nullptr;
}

const LayerSizingRule* findLayerSizingRule(const HardwareProfile& profile, LayerId layer) {
    const auto it = std::find_if(profile.layer_rules.begin(), profile.layer_rules.end(), [&](const auto& rule) {
        return rule.layer == layer;
    });
    return it == profile.layer_rules.end() ? nullptr : &(*it);
}

TierTransitionResult validateTierTransition(
    const HardwareProfile& current,
    const HardwareProfile& target,
    const TierTransitionRequest& request
) {
    TierTransitionResult result;

    const auto current_validation = validateHardwareProfile(current);
    const auto target_validation = validateHardwareProfile(target);
    result.errors.insert(result.errors.end(), current_validation.errors.begin(), current_validation.errors.end());
    result.errors.insert(result.errors.end(), target_validation.errors.begin(), target_validation.errors.end());
    result.warnings.insert(result.warnings.end(), current_validation.warnings.begin(), current_validation.warnings.end());
    result.warnings.insert(result.warnings.end(), target_validation.warnings.begin(), target_validation.warnings.end());

    if (!containsTier(target.supported_tiers, request.target_tier)) {
        result.errors.push_back(
            "target profile '" + target.canonical_name + "' does not support tier '" +
            toString(request.target_tier) + "'"
        );
    }

    if (request.source_tier != request.target_tier && !target.tiering.supports_live_tier_rebalancing) {
        result.errors.push_back(
            "target profile '" + target.canonical_name + "' does not support live tier rebalancing"
        );
    }

    if (request.hot_data_pinned && request.target_tier != StorageTier::Hot) {
        result.errors.push_back("pinned hot data cannot be demoted away from the hot tier");
    }

    if (request.target_tier == StorageTier::Cold &&
        target.tiering.cold_tier_requires_remote_storage &&
        !request.remote_cold_storage_available) {
        result.errors.push_back(
            "cold-tier activation requires remote cold storage for profile '" + target.canonical_name + "'"
        );
    }

    if (request.cross_shard_graph_validation_active && target.network_gbps.minimum < 25) {
        result.errors.push_back(
            "cross-shard graph validation requires at least 25 Gbps target networking"
        );
    }

    if (request.gpu_resident_llm_active) {
        const auto* llm_rule = findLayerSizingRule(target, LayerId::LlmFinalLayer);
        if (target.gpu_count.minimum == 0 || target.gpu_vram_gib.minimum == 0 || llm_rule == nullptr ||
            llm_rule->recommended_gpu_vram_gib == 0) {
            result.errors.push_back(
                "GPU-resident LLM workloads cannot switch to a profile without guaranteed GPU capacity"
            );
        }
    }

    if (current.id == target.id && request.source_tier == request.target_tier) {
        result.warnings.push_back("requested transition keeps the current profile and tier unchanged");
    }

    return result;
}

HardwareProfileRegistry::HardwareProfileRegistry()
    : HardwareProfileRegistry(defaultHardwareProfiles()) {}

HardwareProfileRegistry::HardwareProfileRegistry(std::vector<HardwareProfile> profiles)
    : profiles_(std::move(profiles)) {
    if (!profiles_.empty() && validateHardwareProfile(profiles_.front()).ok()) {
        active_profile_ = profiles_.front().id;
    }
}

HardwareProfileRegistry HardwareProfileRegistry::withBuiltIns() {
    return HardwareProfileRegistry(defaultHardwareProfiles());
}

std::span<const HardwareProfile> HardwareProfileRegistry::profiles() const {
    return profiles_;
}

const HardwareProfile* HardwareProfileRegistry::activeProfile() const {
    return active_profile_ ? find(*active_profile_) : nullptr;
}

const HardwareProfile* HardwareProfileRegistry::find(DeploymentProfileId profile_id) const {
    return findHardwareProfile(profiles_, profile_id);
}

const HardwareProfile* HardwareProfileRegistry::find(std::string_view profile_name) const {
    return findHardwareProfile(profiles_, profile_name);
}

bool HardwareProfileRegistry::activate(DeploymentProfileId profile_id, std::string* error) {
    const auto* profile = find(profile_id);
    if (profile == nullptr) {
        if (error != nullptr) {
            *error = "profile not registered: " + toString(profile_id);
        }
        return false;
    }
    const auto validation = validateHardwareProfile(*profile);
    if (!validation.ok()) {
        if (error != nullptr) {
            *error = validation.errors.front();
        }
        return false;
    }
    active_profile_ = profile_id;
    return true;
}

bool HardwareProfileRegistry::activate(std::string_view profile_name, std::string* error) {
    const auto profile_id = parseDeploymentProfileId(profile_name);
    if (!profile_id) {
        if (error != nullptr) {
            *error = "unknown profile alias: " + std::string(profile_name);
        }
        return false;
    }
    return activate(*profile_id, error);
}

HardwareProfileValidationResult HardwareProfileRegistry::validate() const {
    HardwareProfileValidationResult result;
    std::set<std::string> names;
    std::set<DeploymentProfileId> ids;

    for (const auto& profile : profiles_) {
        const auto profile_validation = validateHardwareProfile(profile);
        result.errors.insert(result.errors.end(), profile_validation.errors.begin(), profile_validation.errors.end());
        result.warnings.insert(result.warnings.end(), profile_validation.warnings.begin(), profile_validation.warnings.end());
        if (!names.insert(profile.canonical_name).second) {
            result.errors.push_back("duplicate profile name registered: " + profile.canonical_name);
        }
        if (!ids.insert(profile.id).second) {
            result.errors.push_back("duplicate profile id registered: " + toString(profile.id));
        }
    }

    return result;
}

TierTransitionResult HardwareProfileRegistry::transitionTo(
    DeploymentProfileId target_profile,
    const TierTransitionRequest& request
) const {
    TierTransitionResult result;

    const auto* current = activeProfile();
    if (current == nullptr) {
        result.errors.push_back("no active profile is set");
        return result;
    }

    const auto* target = find(target_profile);
    if (target == nullptr) {
        result.errors.push_back("target profile is not registered");
        return result;
    }

    return validateTierTransition(*current, *target, request);
}

} // namespace themis::evaluation
