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

/// @brief Canonical deployment profiles supported by the evaluation module.
enum class DeploymentProfileId {
    Development,
    Production,
    HighPerformanceFederated
};

/// @brief Storage temperature tiers used by layered deployments.
enum class StorageTier {
    Hot,
    Warm,
    Cold
};

/// @brief Network class expected by a deployment profile.
enum class NetworkFabric {
    Workstation,
    Datacenter,
    HighBandwidthFabric
};

/// @brief Accelerator requirement for the profile's representative workloads.
enum class AcceleratorClass {
    CpuOnly,
    OptionalGpu,
    RequiredGpu
};

/// @brief Retrieval layers that must be covered by each hardware profile.
enum class LayerId {
    AnnFrontdoor,
    TensorMidLayer,
    GraphTruthLayer,
    LlmFinalLayer
};

/// @brief Minimum and recommended sizing band for a single hardware dimension.
struct ResourceSizingBand {
    std::size_t minimum = 0;
    std::size_t recommended = 0;
};

/// @brief Layer-specific sizing and tier guidance for a single hardware profile.
struct LayerSizingRule {
    LayerId layer = LayerId::AnnFrontdoor;
    StorageTier preferred_tier = StorageTier::Hot;
    std::size_t minimum_ram_gib = 0;
    std::size_t recommended_ram_gib = 0;
    std::size_t minimum_nvme_gib = 0;
    std::size_t recommended_gpu_vram_gib = 0;
    std::size_t diskann_break_even_million_vectors = 0;
};

/// @brief Tier sizing and live-rebalancing policy for one deployment profile.
struct TieringPolicy {
    std::size_t hot_tier_max_gib = 0;
    std::size_t warm_tier_max_gib = 0;
    bool cold_tier_requires_remote_storage = false;
    bool supports_live_tier_rebalancing = false;
};

/// @brief Complete hardware profile definition consumed by planning and validation code.
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

/// @brief Validation output for one profile or registry snapshot.
struct HardwareProfileValidationResult {
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    [[nodiscard]] bool ok() const { return errors.empty(); }
};

/// @brief Runtime context for validating a live storage-tier/profile switch.
struct TierTransitionRequest {
    StorageTier source_tier = StorageTier::Hot;
    StorageTier target_tier = StorageTier::Hot;
    bool hot_data_pinned = false;
    bool remote_cold_storage_available = false;
    bool cross_shard_graph_validation_active = false;
    bool gpu_resident_llm_active = false;
};

/// @brief Validation output for a requested storage-tier/profile transition.
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

/// @brief Parse a user-facing profile identifier or alias.
[[nodiscard]] std::optional<DeploymentProfileId> parseDeploymentProfileId(std::string_view value);

/// @brief Parse a storage-tier selector from config or CLI input.
[[nodiscard]] std::optional<StorageTier> parseStorageTier(std::string_view value);

/**
 * @brief Build the canonical development, production, and federated profiles.
 *
 * The returned profiles define minimum and recommended sizing bands for CPU,
 * RAM, NVMe, GPU, and network capacity, plus per-layer tiering guidance.
 *
 * @return Source-controlled built-in hardware profiles.
 */
[[nodiscard]] std::vector<HardwareProfile> defaultHardwareProfiles();

/**
 * @brief Validate one hardware profile for completeness and internal consistency.
 *
 * Validation rejects incomplete or contradictory sizing bands, missing layer
 * coverage, unsupported storage-tier references, and GPU/network requirements
 * that are incompatible with the declared accelerator or deployment class.
 *
 * @param profile Profile definition to validate.
 * @return Errors and warnings describing the validation outcome.
 */
[[nodiscard]] HardwareProfileValidationResult validateHardwareProfile(const HardwareProfile& profile);

/// @brief Find a profile by canonical enum identifier.
[[nodiscard]] const HardwareProfile* findHardwareProfile(
    std::span<const HardwareProfile> profiles,
    DeploymentProfileId profile_id
);

/// @brief Find a profile by canonical or alias string identifier.
[[nodiscard]] const HardwareProfile* findHardwareProfile(
    std::span<const HardwareProfile> profiles,
    std::string_view profile_name
);

/// @brief Look up the layer-specific sizing rule inside a profile.
[[nodiscard]] const LayerSizingRule* findLayerSizingRule(const HardwareProfile& profile, LayerId layer);

/**
 * @brief Validate a live switch between deployment profiles or storage tiers.
 *
 * The transition guard rejects hot-to-cold demotions with pinned data, cold-tier
 * activation without remote storage when the target profile requires it, GPU-free
 * targets for active GPU-resident LLM workloads, and low-bandwidth targets while
 * cross-shard graph validation is active.
 *
 * @param current Current active hardware profile.
 * @param target Target hardware profile.
 * @param request Runtime storage-tier transition context.
 * @return Transition validation result.
 */
[[nodiscard]] TierTransitionResult validateTierTransition(
    const HardwareProfile& current,
    const HardwareProfile& target,
    const TierTransitionRequest& request
);

/**
 * @brief Small registry for initialization, lookup, activation, and transition checks.
 */
class HardwareProfileRegistry {
public:
    HardwareProfileRegistry();
    explicit HardwareProfileRegistry(std::vector<HardwareProfile> profiles);

    /// @brief Construct a registry pre-populated with the canonical built-in profiles.
    [[nodiscard]] static HardwareProfileRegistry withBuiltIns();

    /// @brief Expose the registered profile set for planner or test inspection.
    [[nodiscard]] std::span<const HardwareProfile> profiles() const;

    /// @brief Return the currently active profile, or nullptr before initialization.
    [[nodiscard]] const HardwareProfile* activeProfile() const;

    /// @brief Lookup by enum identifier.
    [[nodiscard]] const HardwareProfile* find(DeploymentProfileId profile_id) const;

    /// @brief Lookup by canonical or alias string identifier.
    [[nodiscard]] const HardwareProfile* find(std::string_view profile_name) const;

    /**
     * @brief Activate a registered profile by enum identifier.
     *
     * @param profile_id Target active profile.
     * @param error Optional error string populated when activation fails.
     * @return True when the profile exists and validation succeeded.
     */
    [[nodiscard]] bool activate(DeploymentProfileId profile_id, std::string* error = nullptr);

    /**
     * @brief Activate a registered profile by canonical or alias string identifier.
     *
     * @param profile_name Canonical name or alias.
     * @param error Optional error string populated when activation fails.
     * @return True when the profile exists and validation succeeded.
     */
    [[nodiscard]] bool activate(std::string_view profile_name, std::string* error = nullptr);

    /// @brief Validate every registered profile and surface duplicate-name issues.
    [[nodiscard]] HardwareProfileValidationResult validate() const;

    /**
     * @brief Validate a transition from the current active profile to a target profile.
     *
     * @param target_profile Target profile identifier.
     * @param request Runtime tier-switch context.
     * @return Transition validation result. Fails if no active profile is set.
     */
    [[nodiscard]] TierTransitionResult transitionTo(
        DeploymentProfileId target_profile,
        const TierTransitionRequest& request
    ) const;

private:
    std::vector<HardwareProfile> profiles_;
    std::optional<DeploymentProfileId> active_profile_;
};

} // namespace themis::evaluation
