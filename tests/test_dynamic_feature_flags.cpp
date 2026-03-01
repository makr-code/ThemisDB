/// @file test_dynamic_feature_flags.cpp
/// @brief Unit tests for dynamic feature-flag override support in EditionManager.
///
/// Tests the setFeatureOverride / clearFeatureOverride / clearAllFeatureOverrides
/// / hasFeatureOverride / getFeatureOverride API and its interaction with the
/// compile-time edition gate and RuntimeLicenseGate.
///
/// Test categories:
///  1. Singleton / baseline
///  2. setFeatureOverride(false) – admin disables an otherwise available feature
///  3. setFeatureOverride(true) – cannot bypass compile-time edition gate
///  4. clearFeatureOverride / clearAllFeatureOverrides
///  5. hasFeatureOverride / getFeatureOverride introspection
///  6. Override interaction with feature enumeration (getAvailableFeatures)
///  7. Thread-safety smoke test
///  8. Upgrade message reflects override state

#include <gtest/gtest.h>
#include "themis/edition_manager.h"
#include "themis/edition.h"
#include "themis/runtime_license_gate.h"
#include "themis/license_info.h"

#include <atomic>
#include <string>
#include <thread>
#include <vector>

using namespace themis::edition;
using namespace themis::license;

// ============================================================================
// Test fixture: always clean up overrides after each test so tests are
// independent regardless of singleton state.
// ============================================================================

class DynamicFeatureFlagTest : public ::testing::Test {
protected:
    void SetUp() override {
        EditionManager::instance().clearAllFeatureOverrides();
    }
    void TearDown() override {
        EditionManager::instance().clearAllFeatureOverrides();
    }

    static LicenseActivationResult makeResult(bool success,
                                               const std::string& status,
                                               int grace_days = 0) {
        LicenseActivationResult r;
        r.success              = success;
        r.status               = status;
        r.grace_days_remaining = grace_days;
        return r;
    }

    static LicenseData makeLicense(const std::string& edition_str = "ENTERPRISE") {
        LicenseData ld;
        ld.organization_name = "TestOrg";
        ld.license_key       = "THEMIS-ENT-DYN-0001";
        ld.edition           = edition_str;
        ld.contact_email     = "test@example.com";
        ld.expiry_date       = "2099-12-31";
        ld.max_nodes         = 100;
        ld.max_cores         = -1;
        ld.max_storage_tb    = -1;
        return ld;
    }
};

// ============================================================================
// 1. Singleton / baseline – no overrides initially
// ============================================================================

TEST_F(DynamicFeatureFlagTest, NoOverridesInitially) {
    EXPECT_FALSE(EditionManager::instance().hasFeatureOverride("enterprise_plugins"));
    EXPECT_FALSE(EditionManager::instance().hasFeatureOverride("multi_master"));
    EXPECT_EQ(EditionManager::instance().getFeatureOverride("rbac"), std::nullopt);
}

// ============================================================================
// 2. setFeatureOverride(false) blocks an otherwise gated feature
// ============================================================================

TEST_F(DynamicFeatureFlagTest, OverrideFalseAlwaysBlocks) {
    // Set up an active license so the gate itself would pass on Enterprise.
    RuntimeLicenseGate::instance().initialize(makeResult(true, "active"), makeLicense());

    // Disable a gated feature via admin override.
    EditionManager::instance().setFeatureOverride("enterprise_plugins", false);

    std::string err;
    bool allowed = EditionManager::instance().isFeatureAvailable("enterprise_plugins", err);
    EXPECT_FALSE(allowed);
    EXPECT_FALSE(err.empty());
    // Error message must mention "administratively disabled".
    EXPECT_NE(err.find("administratively disabled"), std::string::npos)
        << "Expected 'administratively disabled' in: " << err;
}

TEST_F(DynamicFeatureFlagTest, OverrideFalseBlocksUnknownFeature) {
    // Unknown features are normally always allowed, but an explicit false
    // override should block them.
    EditionManager::instance().setFeatureOverride("custom_feature_xyz", false);

    std::string err;
    bool allowed = EditionManager::instance().isFeatureAvailable("custom_feature_xyz", err);
    EXPECT_FALSE(allowed);
    EXPECT_FALSE(err.empty());
}

TEST_F(DynamicFeatureFlagTest, OverrideFalseBlocksAllKnownGatedFeatures) {
    RuntimeLicenseGate::instance().initialize(makeResult(true, "active"), makeLicense());

    for (std::string_view feat : kGatedFeatureNames) {
        EditionManager::instance().setFeatureOverride(feat, false);
        EXPECT_FALSE(EditionManager::instance().isFeatureAvailable(feat))
            << "Feature '" << feat << "' must be blocked by admin override";
        EditionManager::instance().clearFeatureOverride(feat);
    }
}

// ============================================================================
// 3. setFeatureOverride(true) – cannot bypass compile-time edition gate
// ============================================================================

TEST_F(DynamicFeatureFlagTest, OverrideTrueCannotBypassCompileTimeGate) {
    // A compile-time disabled feature (community build) stays blocked even
    // when the override is set to true.
    RuntimeLicenseGate::instance().initialize(makeResult(true, "active"), makeLicense());

    for (std::string_view feat : kGatedFeatureNames) {
        EditionManager::instance().setFeatureOverride(feat, true);

        std::string err;
        bool allowed = EditionManager::instance().isFeatureAvailable(std::string(feat), err);

        if (!IsFeatureEnabled(feat)) {
            // Compile-time gate must still block, regardless of override.
            EXPECT_FALSE(allowed) << "Override=true must not bypass compile-time gate for '"
                                  << feat << "'";
            EXPECT_FALSE(err.empty());
        }

        EditionManager::instance().clearFeatureOverride(feat);
    }
}

TEST_F(DynamicFeatureFlagTest, OverrideTrueDoesNotAffectBehaviorWhenAlreadyAllowed) {
    // For features that ARE compile-time enabled (Enterprise+ build),
    // override=true must not break them.
    RuntimeLicenseGate::instance().initialize(makeResult(true, "active"), makeLicense());

    for (std::string_view feat : kGatedFeatureNames) {
        if (!IsFeatureEnabled(feat)) {
            continue; // already compile-time blocked; tested above
        }
        EditionManager::instance().setFeatureOverride(feat, true);
        std::string err;
        EXPECT_TRUE(EditionManager::instance().isFeatureAvailable(std::string(feat), err))
            << "Override=true must not block a feature that passes edition+license. err: " << err;
        EditionManager::instance().clearFeatureOverride(feat);
    }
}

// ============================================================================
// 4. clearFeatureOverride / clearAllFeatureOverrides
// ============================================================================

TEST_F(DynamicFeatureFlagTest, ClearOverrideRestoresBehavior) {
    RuntimeLicenseGate::instance().initialize(makeResult(true, "active"), makeLicense());

    // Set and verify the override blocks.
    EditionManager::instance().setFeatureOverride("rbac", false);
    EXPECT_FALSE(EditionManager::instance().isFeatureAvailable("rbac"));

    // Clear the override; behavior reverts to edition+license gate.
    EditionManager::instance().clearFeatureOverride("rbac");
    EXPECT_FALSE(EditionManager::instance().hasFeatureOverride("rbac"));

    // After clearing, the result is purely determined by edition+license.
    // (Both paths — allowed or blocked — are valid depending on the build.)
    // The important thing is that we are no longer blocked due to override.
    std::string err;
    bool after_clear = EditionManager::instance().isFeatureAvailable("rbac", err);
    if (!after_clear) {
        // Must be because of edition or license, not admin override.
        EXPECT_EQ(err.find("administratively disabled"), std::string::npos)
            << "After clearing override, error must not mention admin disabled. err: " << err;
    }
}

TEST_F(DynamicFeatureFlagTest, ClearOverrideIsNoOpWhenNotSet) {
    EXPECT_NO_THROW(EditionManager::instance().clearFeatureOverride("nonexistent_xyz"));
    EXPECT_FALSE(EditionManager::instance().hasFeatureOverride("nonexistent_xyz"));
}

TEST_F(DynamicFeatureFlagTest, ClearAllOverridesRemovesAll) {
    EditionManager::instance().setFeatureOverride("rbac", false);
    EditionManager::instance().setFeatureOverride("hsm", false);
    EditionManager::instance().setFeatureOverride("multi_master", true);

    EXPECT_TRUE(EditionManager::instance().hasFeatureOverride("rbac"));
    EXPECT_TRUE(EditionManager::instance().hasFeatureOverride("hsm"));
    EXPECT_TRUE(EditionManager::instance().hasFeatureOverride("multi_master"));

    EditionManager::instance().clearAllFeatureOverrides();

    EXPECT_FALSE(EditionManager::instance().hasFeatureOverride("rbac"));
    EXPECT_FALSE(EditionManager::instance().hasFeatureOverride("hsm"));
    EXPECT_FALSE(EditionManager::instance().hasFeatureOverride("multi_master"));
}

// ============================================================================
// 5. hasFeatureOverride / getFeatureOverride introspection
// ============================================================================

TEST_F(DynamicFeatureFlagTest, HasOverride_FalseWhenNotSet) {
    EXPECT_FALSE(EditionManager::instance().hasFeatureOverride("enterprise_plugins"));
}

TEST_F(DynamicFeatureFlagTest, HasOverride_TrueAfterSet) {
    EditionManager::instance().setFeatureOverride("enterprise_plugins", false);
    EXPECT_TRUE(EditionManager::instance().hasFeatureOverride("enterprise_plugins"));
}

TEST_F(DynamicFeatureFlagTest, GetOverride_NulloptWhenNotSet) {
    EXPECT_EQ(EditionManager::instance().getFeatureOverride("field_encryption"),
              std::nullopt);
}

TEST_F(DynamicFeatureFlagTest, GetOverride_FalseWhenSetFalse) {
    EditionManager::instance().setFeatureOverride("field_encryption", false);
    auto override_val = EditionManager::instance().getFeatureOverride("field_encryption");
    ASSERT_TRUE(override_val.has_value());
    EXPECT_FALSE(*override_val);
}

TEST_F(DynamicFeatureFlagTest, GetOverride_TrueWhenSetTrue) {
    EditionManager::instance().setFeatureOverride("rbac", true);
    auto override_val = EditionManager::instance().getFeatureOverride("rbac");
    ASSERT_TRUE(override_val.has_value());
    EXPECT_TRUE(*override_val);
}

TEST_F(DynamicFeatureFlagTest, OverrideCanBeUpdated) {
    EditionManager::instance().setFeatureOverride("hsm", false);
    ASSERT_FALSE(*EditionManager::instance().getFeatureOverride("hsm"));

    // Update to true.
    EditionManager::instance().setFeatureOverride("hsm", true);
    ASSERT_TRUE(*EditionManager::instance().getFeatureOverride("hsm"));
}

// ============================================================================
// 6. Override interaction with feature enumeration
// ============================================================================

TEST_F(DynamicFeatureFlagTest, OverrideFalseMovesFeatureToUnavailableList) {
    RuntimeLicenseGate::instance().initialize(makeResult(true, "active"), makeLicense());

    // Find the first compile-time enabled feature (if any) so we have
    // something to move to the unavailable list.
    std::string target_feat;
    for (std::string_view feat : kGatedFeatureNames) {
        if (IsFeatureEnabled(feat)) {
            target_feat = std::string(feat);
            break;
        }
    }

    if (target_feat.empty()) {
        GTEST_SKIP() << "No compile-time-enabled gated features in this build";
    }

    // Before override: feature should be in available list.
    {
        auto avail = EditionManager::instance().getAvailableFeatures();
        bool found = false;
        for (const auto& f : avail) { if (f == target_feat) { found = true; break; } }
        EXPECT_TRUE(found) << "Feature '" << target_feat << "' expected in available list";
    }

    // Set admin override to false.
    EditionManager::instance().setFeatureOverride(target_feat, false);

    // After override: feature must appear in unavailable list.
    {
        auto unavail = EditionManager::instance().getUnavailableFeatures();
        bool found_unavail = false;
        for (const auto& f : unavail) {
            if (f == target_feat) { found_unavail = true; break; }
        }
        EXPECT_TRUE(found_unavail) << "Feature '" << target_feat
            << "' expected in unavailable list after override=false";

        // Must NOT appear in available list simultaneously.
        auto avail = EditionManager::instance().getAvailableFeatures();
        for (const auto& f : avail) {
            EXPECT_NE(f, target_feat) << "Feature '" << target_feat
                << "' must not appear in both available and unavailable lists";
        }
    }
}

TEST_F(DynamicFeatureFlagTest, TotalFeatureCountUnchangedByOverride) {
    // The total count of known gated features never changes; overrides just
    // shift features between the available and unavailable buckets.
    EditionManager::instance().setFeatureOverride("rbac", false);
    EditionManager::instance().setFeatureOverride("hsm", false);

    const auto avail   = EditionManager::instance().getAvailableFeatures();
    const auto unavail = EditionManager::instance().getUnavailableFeatures();

    EXPECT_EQ(avail.size() + unavail.size(), kGatedFeatureCount);
}

// ============================================================================
// 7. Thread-safety smoke test
// ============================================================================

TEST_F(DynamicFeatureFlagTest, ConcurrentOverrideAndCheckDoNotCrash) {
    RuntimeLicenseGate::instance().initialize(makeResult(true, "active"), makeLicense());

    constexpr int kThreads    = 6;
    constexpr int kIterations = 300;

    std::atomic<int> errors{0};
    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    // Writers
    for (int t = 0; t < kThreads / 2; ++t) {
        threads.emplace_back([&errors] {
            const std::string feat = "rbac";
            for (int i = 0; i < kIterations; ++i) {
                try {
                    EditionManager::instance().setFeatureOverride(feat, i % 2 == 0);
                    if (i % 5 == 0) {
                        EditionManager::instance().clearFeatureOverride(feat);
                    }
                } catch (...) {
                    ++errors;
                }
            }
        });
        (void)t;
    }

    // Readers
    for (int t = 0; t < kThreads / 2; ++t) {
        threads.emplace_back([&errors] {
            for (int i = 0; i < kIterations; ++i) {
                try {
                    (void)EditionManager::instance().isFeatureAvailable("rbac");
                    (void)EditionManager::instance().hasFeatureOverride("rbac");
                    (void)EditionManager::instance().getFeatureOverride("rbac");
                } catch (...) {
                    ++errors;
                }
            }
        });
        (void)t;
    }

    for (auto& th : threads) { th.join(); }

    EXPECT_EQ(errors.load(), 0) << "Concurrent override operations must not throw";
}

// ============================================================================
// 8. Upgrade message reflects override state
// ============================================================================

TEST_F(DynamicFeatureFlagTest, UpgradeMessageNonEmptyWhenAdminDisabled) {
    RuntimeLicenseGate::instance().initialize(makeResult(true, "active"), makeLicense());

    // Pick a feature and block it with an admin override.
    EditionManager::instance().setFeatureOverride("enterprise_plugins", false);

    // Even if edition+license would allow it, the feature is now unavailable.
    // getUpgradeMessage should return a non-empty message.
    std::string msg = EditionManager::instance().getUpgradeMessage("enterprise_plugins");
    // The message is non-empty only when the feature is unavailable.
    if (!EditionManager::instance().isFeatureAvailable("enterprise_plugins")) {
        EXPECT_FALSE(msg.empty())
            << "Upgrade message must not be empty for an unavailable feature";
    }
}
