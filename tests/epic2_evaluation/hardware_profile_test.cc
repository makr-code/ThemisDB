#include <algorithm>
#include <gtest/gtest.h>

#include "hardware_profile.h"

namespace {

using themis::evaluation::AcceleratorClass;
using themis::evaluation::DeploymentProfileId;
using themis::evaluation::HardwareProfile;
using themis::evaluation::HardwareProfileRegistry;
using themis::evaluation::LayerId;
using themis::evaluation::LayerSizingRule;
using themis::evaluation::NetworkFabric;
using themis::evaluation::ResourceSizingBand;
using themis::evaluation::StorageTier;
using themis::evaluation::TierTransitionRequest;
using themis::evaluation::TieringPolicy;

HardwareProfile makeInvalidProfile() {
    return {
        DeploymentProfileId::Production,
        "broken",
        "intentionally incomplete profile for validation coverage",
        ResourceSizingBand{32, 16},
        ResourceSizingBand{128, 64},
        ResourceSizingBand{2, 1},
        ResourceSizingBand{2048, 1024},
        ResourceSizingBand{0, 0},
        ResourceSizingBand{0, 0},
        ResourceSizingBand{10, 5},
        AcceleratorClass::RequiredGpu,
        NetworkFabric::Datacenter,
        TieringPolicy{256, 128, true, false},
        {StorageTier::Hot, StorageTier::Warm},
        {LayerSizingRule{LayerId::AnnFrontdoor, StorageTier::Hot, 32, 16, 256, 0, 0}},
    };
}

TEST(HardwareProfile, ParseDeploymentProfileAliases) {
    EXPECT_EQ(
        themis::evaluation::parseDeploymentProfileId("dev"),
        DeploymentProfileId::Development
    );
    EXPECT_EQ(
        themis::evaluation::parseDeploymentProfileId("production"),
        DeploymentProfileId::Production
    );
    EXPECT_EQ(
        themis::evaluation::parseDeploymentProfileId("high-perf-federated"),
        DeploymentProfileId::HighPerformanceFederated
    );
    EXPECT_EQ(
        themis::evaluation::parseDeploymentProfileId("federated"),
        DeploymentProfileId::HighPerformanceFederated
    );
    EXPECT_FALSE(themis::evaluation::parseDeploymentProfileId("unknown").has_value());
}

TEST(HardwareProfile, BuiltInsValidateAndCoverAllLayers) {
    const auto profiles = themis::evaluation::defaultHardwareProfiles();
    ASSERT_EQ(profiles.size(), 3U);

    for (const auto& profile : profiles) {
        const auto validation = themis::evaluation::validateHardwareProfile(profile);
        EXPECT_TRUE(validation.ok()) << profile.canonical_name;

        EXPECT_NE(
            themis::evaluation::findLayerSizingRule(profile, LayerId::AnnFrontdoor),
            nullptr
        );
        EXPECT_NE(
            themis::evaluation::findLayerSizingRule(profile, LayerId::TensorMidLayer),
            nullptr
        );
        EXPECT_NE(
            themis::evaluation::findLayerSizingRule(profile, LayerId::GraphTruthLayer),
            nullptr
        );
        EXPECT_NE(
            themis::evaluation::findLayerSizingRule(profile, LayerId::LlmFinalLayer),
            nullptr
        );
    }
}

TEST(HardwareProfile, ValidationRejectsIncompleteOrIncompatibleProfile) {
    const auto validation = themis::evaluation::validateHardwareProfile(makeInvalidProfile());
    EXPECT_FALSE(validation.ok());
    EXPECT_GE(validation.errors.size(), 5U);
}

TEST(HardwareProfile, RegistryInitializesAndActivatesProfiles) {
    auto registry = HardwareProfileRegistry::withBuiltIns();
    const auto validation = registry.validate();
    ASSERT_TRUE(validation.ok());

    ASSERT_NE(registry.activeProfile(), nullptr);
    EXPECT_EQ(
        registry.activeProfile()->id,
        DeploymentProfileId::Development
    );

    std::string error = {};
    EXPECT_TRUE(registry.activate("production", &error)) << error;
    ASSERT_NE(registry.activeProfile(), nullptr);
    EXPECT_EQ(
        registry.activeProfile()->id,
        DeploymentProfileId::Production
    );
}

TEST(HardwareProfile, AnnBreakEvenThresholdsScaleWithProfileSize) {
    const auto profiles = themis::evaluation::defaultHardwareProfiles();
    const auto* dev = themis::evaluation::findHardwareProfile(profiles, DeploymentProfileId::Development);
    const auto* prod = themis::evaluation::findHardwareProfile(profiles, DeploymentProfileId::Production);
    const auto* fed = themis::evaluation::findHardwareProfile(
        profiles,
        DeploymentProfileId::HighPerformanceFederated
    );

    ASSERT_NE(dev, nullptr);
    ASSERT_NE(prod, nullptr);
    ASSERT_NE(fed, nullptr);

    const auto* dev_ann = themis::evaluation::findLayerSizingRule(*dev, LayerId::AnnFrontdoor);
    const auto* prod_ann = themis::evaluation::findLayerSizingRule(*prod, LayerId::AnnFrontdoor);
    const auto* fed_ann = themis::evaluation::findLayerSizingRule(*fed, LayerId::AnnFrontdoor);

    ASSERT_NE(dev_ann, nullptr);
    ASSERT_NE(prod_ann, nullptr);
    ASSERT_NE(fed_ann, nullptr);

    EXPECT_LT(
        dev_ann->diskann_break_even_million_vectors,
        prod_ann->diskann_break_even_million_vectors
    );
    EXPECT_LT(
        prod_ann->diskann_break_even_million_vectors,
        fed_ann->diskann_break_even_million_vectors
    );
}

TEST(HardwareProfile, TransitionRejectsPinnedHotDataDemotion) {
    HardwareProfileRegistry registry = HardwareProfileRegistry::withBuiltIns();
    ASSERT_TRUE(registry.activate(DeploymentProfileId::Production));

    const auto result = registry.transitionTo(
        DeploymentProfileId::HighPerformanceFederated,
        TierTransitionRequest{
            StorageTier::Hot,
            StorageTier::Cold,
            true,
            true,
            false,
            false,
        }
    );

    EXPECT_FALSE(result.ok());
    ASSERT_FALSE(result.errors.empty());
    EXPECT_NE(result.errors.front().find("pinned hot data"), std::string::npos);
}

TEST(HardwareProfile, TransitionRejectsCrossShardDowngradeToDevelopmentNetwork) {
    HardwareProfileRegistry registry = HardwareProfileRegistry::withBuiltIns();
    ASSERT_TRUE(registry.activate(DeploymentProfileId::HighPerformanceFederated));

    const auto result = registry.transitionTo(
        DeploymentProfileId::Development,
        TierTransitionRequest{
            StorageTier::Warm,
            StorageTier::Warm,
            false,
            false,
            true,
            false,
        }
    );

    EXPECT_FALSE(result.ok());
    EXPECT_NE(
        std::find_if(result.errors.begin(), result.errors.end(), [](const auto& error) {
            return error.find("25 Gbps") != std::string::npos;
        }),
        result.errors.end()
    );
}

TEST(HardwareProfile, TransitionAllowsWarmRebalanceIntoFederatedProfile) {
    HardwareProfileRegistry registry = HardwareProfileRegistry::withBuiltIns();
    ASSERT_TRUE(registry.activate(DeploymentProfileId::Production));

    const auto result = registry.transitionTo(
        DeploymentProfileId::HighPerformanceFederated,
        TierTransitionRequest{
            StorageTier::Warm,
            StorageTier::Warm,
            false,
            true,
            true,
            true,
        }
    );

    EXPECT_TRUE(result.ok());
}

TEST(HardwareProfile, ValidateDetectsDuplicateProfileId) {
    // Build a registry with two profiles sharing the same DeploymentProfileId
    // but different canonical names — validate() must surface a duplicate-id error.
    auto registry = HardwareProfileRegistry::withBuiltIns();
    const auto* dev = registry.find(DeploymentProfileId::Development);
    ASSERT_NE(dev, nullptr);

    HardwareProfile dup = *dev;
    dup.canonical_name = "development_alias";

    HardwareProfileRegistry custom({*dev, dup});
    const auto result = custom.validate();

    EXPECT_FALSE(result.ok());
    const bool has_duplicate_id_error = std::any_of(
        result.errors.begin(), result.errors.end(),
        [](const std::string& e) {
            return e.find("duplicate profile id") != std::string::npos;
        }
    );
    EXPECT_TRUE(has_duplicate_id_error);
}

TEST(HardwareProfile, ConstructorWithInvalidFirstProfileLeavesNullActiveProfile) {
    // An invalid first profile must not become the active profile after construction.
    HardwareProfileRegistry registry({makeInvalidProfile()});
    EXPECT_EQ(registry.activeProfile(), nullptr);
}

} // namespace
