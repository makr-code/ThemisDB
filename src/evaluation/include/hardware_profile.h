/**
 * @file hardware_profile.h
 * @brief EPIC 2.1 hardware profile contracts for layered ThemisDB deployments.
 */

#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace themis::evaluation {

enum class DeploymentProfileId {
    Development,
    Production,
    HighPerformanceFederated
};

enum class StorageTier {
    Hot,
    Warm,
    Cold
};

enum class NetworkFabric {
    Workstation,
    Datacenter,
    HighBandwidthFabric
};

enum class AcceleratorClass {
    CpuOnly,
    OptionalGpu,
    RequiredGpu
};

enum class LayerId {
    AnnFrontdoor,
    TensorMidLayer,
    GraphTruthLayer,
    LlmFinalLayer
};

struct ResourceSizingBand {
    std::size_t minimum = 0;
    std::size_t recommended = 0;
};

struct LayerSizingRule {
    LayerId layer = LayerId::AnnFrontdoor;
    StorageTier preferred_tier = StorageTier::Hot;
    std::size_t minimum_ram_gib = 0;
    std::size_t recommended_ram_gib = 0;
    std::size_t minimum_nvme_gib = 0;
    std::size_t recommended_gpu_vram_gib = 0;
    std::size_t diskann_break_even_million_vectors = 0;
};

struct TieringPolicy {
    std::size_t hot_tier_max_gib = 0;
    std::size_t warm_tier_max_gib = 0;
    bool cold_tier_requires_remote_storage = false;
    bool supports_live_tier_rebalancing = false;
};

struct HardwareProfile {
    DeploymentProfileId id = DeploymentProfileId::Development;
    std::string canonical_name;
    std::string description;
    ResourceSizingBand cpu_cores;
    ResourceSizingBand ram_gib;
    ResourceSizingBand nvme_devices;
    ResourceSizingBand nvme_capacity_gib;
    ResourceSizingBand gpu_count;
    ResourceSizingBand gpu_vram_gib;
    ResourceSizingBand network_gbps;
    AcceleratorClass accelerator_class = AcceleratorClass::CpuOnly;
    NetworkFabric network_fabric = NetworkFabric::Workstation;
    TieringPolicy tiering;
    std::vector<StorageTier> supported_tiers;
    std::vector<LayerSizingRule> layer_rules;
};

struct HardwareProfileValidationResult {
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    [[nodiscard]] bool ok() const { return errors.empty(); }
};

struct TierTransitionRequest {
    StorageTier source_tier = StorageTier::Hot;
    StorageTier target_tier = StorageTier::Hot;
    bool hot_data_pinned = false;
    bool remote_cold_storage_available = false;
    bool cross_shard_graph_validation_active = false;
    bool gpu_resident_llm_active = false;
};

struct TierTransitionResult {
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    [[nodiscard]] bool ok() const { return errors.empty(); }
};

[[nodiscard]] std::string toString(DeploymentProfileId profile_id);
[[nodiscard]] std::string toString(StorageTier tier);
[[nodiscard]] std::string toString(NetworkFabric fabric);
[[nodiscard]] std::string toString(AcceleratorClass accelerator_class);
[[nodiscard]] std::string toString(LayerId layer);

[[nodiscard]] std::optional<DeploymentProfileId> parseDeploymentProfileId(std::string_view value);

[[nodiscard]] std::optional<StorageTier> parseStorageTier(std::string_view value);

[[nodiscard]] std::vector<HardwareProfile> defaultHardwareProfiles();

[[nodiscard]] HardwareProfileValidationResult validateHardwareProfile(const HardwareProfile& profile);

[[nodiscard]] const HardwareProfile* findHardwareProfile(
    std::span<const HardwareProfile> profiles,
    DeploymentProfileId profile_id
);

[[nodiscard]] const HardwareProfile* findHardwareProfile(
    std::span<const HardwareProfile> profiles,
    std::string_view profile_name
);

[[nodiscard]] const LayerSizingRule* findLayerSizingRule(const HardwareProfile& profile, LayerId layer);

[[nodiscard]] TierTransitionResult validateTierTransition(
    const HardwareProfile& current,
    const HardwareProfile& target,
    const TierTransitionRequest& request
);

class HardwareProfileRegistry {
public:
    HardwareProfileRegistry();
    /**
     * @brief Hardware Profile Registry.
     * @param[in] profiles Input parameter.
     * @return Return value.
     */
    explicit HardwareProfileRegistry(std::vector<HardwareProfile> profiles);

    [[nodiscard]] static HardwareProfileRegistry withBuiltIns();

    [[nodiscard]] std::span<const HardwareProfile> profiles() const;

    [[nodiscard]] const HardwareProfile* activeProfile() const;

    [[nodiscard]] const HardwareProfile* find(DeploymentProfileId profile_id) const;

    [[nodiscard]] const HardwareProfile* find(std::string_view profile_name) const;

    [[nodiscard]] bool activate(DeploymentProfileId profile_id, std::string* error = nullptr);

    [[nodiscard]] bool activate(std::string_view profile_name, std::string* error = nullptr);

    [[nodiscard]] HardwareProfileValidationResult validate() const;

    [[nodiscard]] TierTransitionResult transitionTo(
        DeploymentProfileId target_profile,
        const TierTransitionRequest& request
    ) const;

private:
    std::vector<HardwareProfile> profiles_;
    std::optional<DeploymentProfileId> active_profile_;
};

} // namespace themis::evaluation
