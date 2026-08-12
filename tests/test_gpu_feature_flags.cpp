#include <gtest/gtest.h>
#include "themis/gpu/feature_flags.h"
#include "themis/edition.h"

using namespace themis::gpu;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
class FeatureFlagsTest : public ::testing::Test {
protected:
    void SetUp() override {
        GPUFeatureFlags::GetInstance().resetToDefaults();
    }
    void TearDown() override {
        GPUFeatureFlags::GetInstance().resetToDefaults();
    }
};

// ---------------------------------------------------------------------------
// Feature names
// ---------------------------------------------------------------------------

TEST_F(FeatureFlagsTest, FeatureName_AllKnown) {
    using F = GPUFeatureFlags::Feature;
    EXPECT_STREQ("MEMORY_POOL",      GPUFeatureFlags::featureName(F::MEMORY_POOL));
    EXPECT_STREQ("ASYNC_LAUNCHER",   GPUFeatureFlags::featureName(F::ASYNC_LAUNCHER));
    EXPECT_STREQ("MULTI_GPU",        GPUFeatureFlags::featureName(F::MULTI_GPU));
    EXPECT_STREQ("TENSOR_OPS",       GPUFeatureFlags::featureName(F::TENSOR_OPS));
    EXPECT_STREQ("POLICY_GATE",      GPUFeatureFlags::featureName(F::POLICY_GATE));
    EXPECT_STREQ("AUDIT_LOG",        GPUFeatureFlags::featureName(F::AUDIT_LOG));
    EXPECT_STREQ("METRICS",          GPUFeatureFlags::featureName(F::METRICS));
    EXPECT_STREQ("LOAD_BALANCER",    GPUFeatureFlags::featureName(F::LOAD_BALANCER));
    EXPECT_STREQ("KERNEL_VALIDATOR", GPUFeatureFlags::featureName(F::KERNEL_VALIDATOR));
    EXPECT_STREQ("ALERTS",           GPUFeatureFlags::featureName(F::ALERTS));
}

// ---------------------------------------------------------------------------
// getAll() returns all registered features
// ---------------------------------------------------------------------------

TEST_F(FeatureFlagsTest, GetAll_Returns10Entries) {
    const auto all = GPUFeatureFlags::GetInstance().getAll();
    EXPECT_EQ(14u, all.size());
}

TEST_F(FeatureFlagsTest, GetAll_NoneOverridden_Initially) {
    for (const auto& s : GPUFeatureFlags::GetInstance().getAll()) {
        EXPECT_FALSE(s.overridden) << "Feature " << s.name << " should not be overridden";
    }
}

// ---------------------------------------------------------------------------
// Edition name is non-empty
// ---------------------------------------------------------------------------

TEST_F(FeatureFlagsTest, EditionName_NonEmpty) {
    EXPECT_FALSE(GPUFeatureFlags::editionName().empty());
}

// ---------------------------------------------------------------------------
// Enable / disable overrides
// ---------------------------------------------------------------------------

TEST_F(FeatureFlagsTest, Enable_OverridesDefault) {
    auto& ff = GPUFeatureFlags::GetInstance();
    ff.disable(GPUFeatureFlags::Feature::MEMORY_POOL);
    EXPECT_FALSE(ff.isEnabled(GPUFeatureFlags::Feature::MEMORY_POOL));
    ff.enable(GPUFeatureFlags::Feature::MEMORY_POOL);
    EXPECT_TRUE(ff.isEnabled(GPUFeatureFlags::Feature::MEMORY_POOL));
}

TEST_F(FeatureFlagsTest, Disable_OverridesDefault) {
    auto& ff = GPUFeatureFlags::GetInstance();
    ff.disable(GPUFeatureFlags::Feature::METRICS);
    EXPECT_FALSE(ff.isEnabled(GPUFeatureFlags::Feature::METRICS));
}

TEST_F(FeatureFlagsTest, Disable_MarksOverridden) {
    auto& ff = GPUFeatureFlags::GetInstance();
    ff.disable(GPUFeatureFlags::Feature::ALERTS);
    bool found = false;
    for (const auto& s : ff.getAll()) {
        if (s.name == "ALERTS") {
            EXPECT_TRUE(s.overridden);
            EXPECT_FALSE(s.enabled);
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(FeatureFlagsTest, Enable_MarksOverridden) {
    auto& ff = GPUFeatureFlags::GetInstance();
    ff.enable(GPUFeatureFlags::Feature::MULTI_GPU);
    bool found = false;
    for (const auto& s : ff.getAll()) {
        if (s.name == "MULTI_GPU") {
            EXPECT_TRUE(s.overridden);
            EXPECT_TRUE(s.enabled);
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// ---------------------------------------------------------------------------
// resetToDefaults clears all overrides
// ---------------------------------------------------------------------------

TEST_F(FeatureFlagsTest, ResetToDefaults_ClearsOverrides) {
    auto& ff = GPUFeatureFlags::GetInstance();
    ff.disable(GPUFeatureFlags::Feature::MEMORY_POOL);
    ff.disable(GPUFeatureFlags::Feature::METRICS);
    ff.enable(GPUFeatureFlags::Feature::MULTI_GPU);
    ff.resetToDefaults();
    for (const auto& s : ff.getAll()) {
        EXPECT_FALSE(s.overridden) << "After reset, " << s.name << " should not be overridden";
    }
}

// ---------------------------------------------------------------------------
// Community edition defaults: pool/launcher/policy/audit/metrics/validator/alerts ON
// multi-GPU and load-balancer and tensor-ops depend on edition
// ---------------------------------------------------------------------------

TEST_F(FeatureFlagsTest, CommunityOrAbove_CoreFeatures_AreEnabled) {
    // These features should be enabled in Community and above.
    const auto ed = themis::edition::GetEditionType();
    if (ed == themis::edition::EditionType::UNKNOWN) {
        GTEST_SKIP() << "capability:edition_known=false;reason=unknown_edition";
    }
    auto& ff = GPUFeatureFlags::GetInstance();
    EXPECT_TRUE(ff.isEnabled(GPUFeatureFlags::Feature::MEMORY_POOL));
    EXPECT_TRUE(ff.isEnabled(GPUFeatureFlags::Feature::ASYNC_LAUNCHER));
    EXPECT_TRUE(ff.isEnabled(GPUFeatureFlags::Feature::POLICY_GATE));
    EXPECT_TRUE(ff.isEnabled(GPUFeatureFlags::Feature::AUDIT_LOG));
    EXPECT_TRUE(ff.isEnabled(GPUFeatureFlags::Feature::METRICS));
    EXPECT_TRUE(ff.isEnabled(GPUFeatureFlags::Feature::KERNEL_VALIDATOR));
    EXPECT_TRUE(ff.isEnabled(GPUFeatureFlags::Feature::ALERTS));
}

TEST_F(FeatureFlagsTest, MultiGPU_DependsOnEdition) {
    const auto ed = themis::edition::GetEditionType();
    auto& ff = GPUFeatureFlags::GetInstance();
    bool is_enterprise_or_above = (ed == themis::edition::EditionType::ENTERPRISE ||
                                   ed == themis::edition::EditionType::HYPERSCALER);
    EXPECT_EQ(is_enterprise_or_above, ff.isEnabled(GPUFeatureFlags::Feature::MULTI_GPU));
    EXPECT_EQ(is_enterprise_or_above, ff.isEnabled(GPUFeatureFlags::Feature::LOAD_BALANCER));
    EXPECT_EQ(is_enterprise_or_above, ff.isEnabled(GPUFeatureFlags::Feature::TENSOR_OPS));
}

// ---------------------------------------------------------------------------
// Toggle round-trip
// ---------------------------------------------------------------------------

TEST_F(FeatureFlagsTest, ToggleMultipleTimes) {
    auto& ff = GPUFeatureFlags::GetInstance();
    ff.enable(GPUFeatureFlags::Feature::ASYNC_LAUNCHER);
    EXPECT_TRUE(ff.isEnabled(GPUFeatureFlags::Feature::ASYNC_LAUNCHER));
    ff.disable(GPUFeatureFlags::Feature::ASYNC_LAUNCHER);
    EXPECT_FALSE(ff.isEnabled(GPUFeatureFlags::Feature::ASYNC_LAUNCHER));
    ff.enable(GPUFeatureFlags::Feature::ASYNC_LAUNCHER);
    EXPECT_TRUE(ff.isEnabled(GPUFeatureFlags::Feature::ASYNC_LAUNCHER));
}

// ---------------------------------------------------------------------------
// Independent flags do not interfere
// ---------------------------------------------------------------------------

TEST_F(FeatureFlagsTest, IndependentFlags_DoNotInterfere) {
    auto& ff = GPUFeatureFlags::GetInstance();
    ff.disable(GPUFeatureFlags::Feature::MEMORY_POOL);
    ff.enable(GPUFeatureFlags::Feature::METRICS);
    EXPECT_FALSE(ff.isEnabled(GPUFeatureFlags::Feature::MEMORY_POOL));
    EXPECT_TRUE(ff.isEnabled(GPUFeatureFlags::Feature::METRICS));
}

// ---------------------------------------------------------------------------
// All features accessible via getAll() map
// ---------------------------------------------------------------------------

TEST_F(FeatureFlagsTest, GetAll_ContainsCorrectNames) {
    const std::vector<std::string> expected = {
        "MEMORY_POOL", "ASYNC_LAUNCHER", "MULTI_GPU", "TENSOR_OPS",
        "POLICY_GATE", "AUDIT_LOG", "METRICS", "LOAD_BALANCER",
        "KERNEL_VALIDATOR", "ALERTS", "WASM_SANDBOX", "MIG_MANAGER",
        "VULKAN_BACKEND", "PEER_TO_PEER"
    };
    auto all = GPUFeatureFlags::GetInstance().getAll();
    ASSERT_EQ(expected.size(), all.size());
    for (size_t i = 0; i < all.size(); ++i) {
        EXPECT_EQ(expected[i], all[i].name);
    }
}
