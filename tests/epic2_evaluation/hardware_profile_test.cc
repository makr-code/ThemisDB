/**
 * @file hardware_profile_test.cc
 * @brief Contract tests for IHardwareProfileRegistry (sub-issue #5437).
 *
 * Validates factory construction, stock profile population, profile lookup,
 * custom profile registration, detectLocal, and ANN backend recommendation.
 * Production hardware probe logic is tracked in sub-issue #5437.
 */

#include "evaluation/include/hardware_profile.h"

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

using namespace themis::evaluation;

class HardwareProfileRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        registry_ = makeHardwareProfileRegistry();
        ASSERT_NE(registry_, nullptr);
    }

    std::unique_ptr<IHardwareProfileRegistry> registry_;
};

TEST_F(HardwareProfileRegistryTest, FactoryReturnsNonNull) {
    EXPECT_NE(registry_, nullptr);
}

TEST_F(HardwareProfileRegistryTest, StockProfilesPrePopulated) {
    auto ids = registry_->listProfiles();
    EXPECT_FALSE(ids.empty());
}

TEST_F(HardwareProfileRegistryTest, LookupKnownStockProfile) {
    auto profile = registry_->lookup("cpu-only-16gb");
    EXPECT_TRUE(profile.has_value());
    EXPECT_EQ(profile->id, "cpu-only-16gb");
}

TEST_F(HardwareProfileRegistryTest, LookupUnknownProfileReturnsNullopt) {
    auto profile = registry_->lookup("nonexistent-profile");
    EXPECT_FALSE(profile.has_value());
}

TEST_F(HardwareProfileRegistryTest, RegisterCustomProfile) {
    HardwareProfile custom;
    custom.id           = "custom-arm-8gb";
    custom.display_name = "Custom ARM 8 GiB";
    custom.dram_bytes   = 8ULL << 30;
    custom.accelerator  = AcceleratorFamily::None;

    registry_->registerProfile(custom);

    auto found = registry_->lookup("custom-arm-8gb");
    EXPECT_TRUE(found.has_value());
    EXPECT_EQ(found->display_name, "Custom ARM 8 GiB");
}

TEST_F(HardwareProfileRegistryTest, ListProfilesIncludesRegistered) {
    HardwareProfile custom;
    custom.id = "custom-test-profile";
    registry_->registerProfile(custom);

    auto ids = registry_->listProfiles();
    bool found = false;
    for (const auto& id : ids) {
        if (id == "custom-test-profile") found = true;
    }
    EXPECT_TRUE(found);
}

TEST_F(HardwareProfileRegistryTest, DetectLocalDoesNotThrow) {
    EXPECT_NO_THROW(registry_->detectLocal());
}

TEST_F(HardwareProfileRegistryTest, DetectLocalReturnsProfile) {
    HardwareProfile local = registry_->detectLocal();
    // Scaffold: profile ID may be a placeholder, but must not be empty.
    EXPECT_FALSE(local.id.empty());
}

TEST_F(HardwareProfileRegistryTest, RecommendAnnBackendDoesNotThrow) {
    HardwareProfile p;
    p.id         = "cpu-only-16gb";
    p.dram_bytes = 16ULL << 30;
    EXPECT_NO_THROW(registry_->recommendAnnBackend(p, 1'000'000));
}

TEST_F(HardwareProfileRegistryTest, RecommendAnnBackendReturnsNonEmpty) {
    HardwareProfile p;
    p.id         = "gpu-a100-80gb-nvme-4tb";
    p.dram_bytes = 512ULL << 30;
    p.vram_bytes = 80ULL << 30;
    std::string backend = registry_->recommendAnnBackend(p, 10'000'000);
    EXPECT_FALSE(backend.empty());
}
