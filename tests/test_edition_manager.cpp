/// @file test_edition_manager.cpp
/// @brief Unit tests for EditionManager – Community / Enterprise / Hyperscaler
///        edition feature gating.
///
/// Test categories:
///  1. Singleton identity
///  2. Edition information (getEditionType, getEditionName, limits)
///  3. Feature availability (compile-time OFF → blocked; unknown → allowed)
///  4. Resource-limit checks (node count, VRAM)
///  5. Feature enumeration (getAvailableFeatures / getUnavailableFeatures)
///  6. Upgrade messages
///  7. Integration with RuntimeLicenseGate state

#include <gtest/gtest.h>
#include "themis/edition_manager.h"
#include "themis/edition.h"
#include "themis/runtime_license_gate.h"
#include "themis/license_info.h"

using namespace themis::edition;
using namespace themis::license;

// ============================================================================
// Helpers
// ============================================================================

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
    ld.license_key       = "THEMIS-ENT-TEST-0001";
    ld.edition           = edition_str;
    ld.contact_email     = "test@example.com";
    ld.expiry_date       = "2099-12-31";
    ld.max_nodes         = 100;
    ld.max_cores         = -1;
    ld.max_storage_tb    = -1;
    return ld;
}

// ============================================================================
// 1. Singleton identity
// ============================================================================

TEST(EditionManager, SingletonReturnsSameInstance) {
    EditionManager& a = EditionManager::instance();
    EditionManager& b = EditionManager::instance();
    EXPECT_EQ(&a, &b);
}

// ============================================================================
// 2. Edition information
// ============================================================================

TEST(EditionManager, GetEditionTypeReturnsValidValue) {
    const EditionType et = EditionManager::instance().getEditionType();
    EXPECT_TRUE(et == EditionType::COMMUNITY ||
                et == EditionType::ENTERPRISE ||
                et == EditionType::HYPERSCALER ||
                et == EditionType::UNKNOWN);
}

TEST(EditionManager, GetEditionNameNotEmpty) {
    EXPECT_FALSE(EditionManager::instance().getEditionName().empty());
}

TEST(EditionManager, GetMaxNodesBounded) {
    const int max = EditionManager::instance().getMaxNodes();
    // Must be positive or -1 (unlimited).
    EXPECT_TRUE(max > 0 || max == -1);
}

TEST(EditionManager, GetMaxVRAMGBBounded) {
    const int max = EditionManager::instance().getMaxVRAMGB();
    EXPECT_TRUE(max > 0 || max == -1);
}

TEST(EditionManager, EditionInfoMatchesEditionHeader) {
    EXPECT_EQ(EditionManager::instance().getEditionType(), GetEditionType());
    EXPECT_EQ(EditionManager::instance().getEditionName(), EDITION_STRING);
    EXPECT_EQ(EditionManager::instance().getMaxNodes(), SHARDING_MAX_NODES);
    EXPECT_EQ(EditionManager::instance().getMaxVRAMGB(), GPU_MAX_VRAM_GB);
}

// ============================================================================
// 3. Feature availability
// ============================================================================

TEST(EditionManager, UnknownFeatureAlwaysAvailable) {
    // After any gate state, unknown features must pass.
    RuntimeLicenseGate::instance().initialize(makeResult(false, "expired"));
    EXPECT_TRUE(EditionManager::instance().isFeatureAvailable("totally_unknown_xyz"));
}

TEST(EditionManager, UnknownFeatureNoErrorMessage) {
    RuntimeLicenseGate::instance().initialize(makeResult(false, "invalid"));
    std::string err = {};
    bool allowed = EditionManager::instance().isFeatureAvailable("another_unknown", err);
    EXPECT_TRUE(allowed);
    EXPECT_TRUE(err.empty());
}

TEST(EditionManager, EnterpriseFeatureBlockedWhenCompileTimeOff) {
    // All known enterprise features must be blocked in a Community build
    // (compile-time OFF) regardless of runtime license state.
    RuntimeLicenseGate::instance().initialize(makeResult(true, "active"), makeLicense());

    for (const char* feat : {"enterprise_plugins", "multi_master",
                              "field_encryption", "rbac", "hsm"}) {
        std::string err = {};
        bool allowed = EditionManager::instance().isFeatureAvailable(feat, err);
        if (!IsFeatureEnabled(feat)) {
            EXPECT_FALSE(allowed) << "Feature '" << feat
                << "' must be blocked in Community build (compile-time OFF)";
            EXPECT_FALSE(err.empty()) << "Error message must be set when blocked";
        }
    }
}

TEST(EditionManager, EnterpriseFeatureBlockedWithExpiredLicense) {
    RuntimeLicenseGate::instance().initialize(
        makeResult(false, "expired", 0), std::nullopt);

    for (const char* feat : {"enterprise_plugins", "multi_master",
                              "field_encryption", "rbac", "hsm"}) {
        std::string err = {};
        bool allowed = EditionManager::instance().isFeatureAvailable(feat, err);
        // Either compile-time gate (Community build) or runtime gate fires —
        // either way the feature must be blocked.
        EXPECT_FALSE(allowed) << "Feature '" << feat
            << "' must be blocked with expired license";
        EXPECT_FALSE(err.empty());
    }
}

// ============================================================================
// 4. Resource-limit checks – node count
// ============================================================================

TEST(EditionManager, NodeLimitWithinBound_Allowed) {
    const int limit = EditionManager::instance().getMaxNodes();
    if (limit < 0) {
        GTEST_SKIP() << "Unlimited nodes (Hyperscaler) – no upper bound to test";
    }
    std::string err = {};
    EXPECT_TRUE(EditionManager::instance().checkNodeLimit(limit, err));
    EXPECT_TRUE(err.empty());
}

TEST(EditionManager, NodeLimitExceeded_Blocked) {
    const int limit = EditionManager::instance().getMaxNodes();
    if (limit < 0) {
        GTEST_SKIP() << "Unlimited nodes (Hyperscaler) – skip exceeded test";
    }
    std::string err = {};
    EXPECT_FALSE(EditionManager::instance().checkNodeLimit(limit + 1, err));
    EXPECT_FALSE(err.empty());
    // Error message must mention the edition.
    EXPECT_NE(err.find(std::string(EDITION_STRING)), std::string::npos)
        << "Error should mention edition. Got: " << err;
}

TEST(EditionManager, NodeLimitZero_Allowed) {
    std::string err = {};
    EXPECT_TRUE(EditionManager::instance().checkNodeLimit(0, err));
}

TEST(EditionManager, NodeLimitOne_Allowed) {
    std::string err = {};
    EXPECT_TRUE(EditionManager::instance().checkNodeLimit(1, err));
}

// ============================================================================
// 4b. Resource-limit checks – VRAM
// ============================================================================

TEST(EditionManager, VRAMLimitWithinBound_Allowed) {
    const int limit = EditionManager::instance().getMaxVRAMGB();
    if (limit < 0) {
        GTEST_SKIP() << "Unlimited VRAM (Hyperscaler) – no upper bound to test";
    }
    std::string err = {};
    EXPECT_TRUE(EditionManager::instance().checkVRAMLimit(limit, err));
    EXPECT_TRUE(err.empty());
}

TEST(EditionManager, VRAMLimitExceeded_Blocked) {
    const int limit = EditionManager::instance().getMaxVRAMGB();
    if (limit < 0) {
        GTEST_SKIP() << "Unlimited VRAM (Hyperscaler) – skip exceeded test";
    }
    std::string err = {};
    EXPECT_FALSE(EditionManager::instance().checkVRAMLimit(limit + 1, err));
    EXPECT_FALSE(err.empty());
    EXPECT_NE(err.find(std::string(EDITION_STRING)), std::string::npos)
        << "Error should mention edition. Got: " << err;
}

TEST(EditionManager, VRAMLimitZero_Allowed) {
    std::string err = {};
    EXPECT_TRUE(EditionManager::instance().checkVRAMLimit(0, err));
}

// ============================================================================
// 5. Feature enumeration
// ============================================================================

TEST(EditionManager, GetAvailableFeaturesReturnsSubset) {
    const auto avail   = EditionManager::instance().getAvailableFeatures();
    const auto unavail = EditionManager::instance().getUnavailableFeatures();

    // Available + unavailable must cover all known gated features.
    EXPECT_EQ(avail.size() + unavail.size(), themis::edition::kGatedFeatureCount);

    // No feature should appear in both lists.
    for (const auto& f : avail) {
        for (const auto& u : unavail) {
            EXPECT_NE(f, u) << "Feature '" << f << "' appears in both lists";
        }
    }
}

TEST(EditionManager, GetAvailableFeaturesConsistentWithIsFeatureAvailable) {
    const auto avail = EditionManager::instance().getAvailableFeatures();
    for (const auto& f : avail) {
        EXPECT_TRUE(EditionManager::instance().isFeatureAvailable(f))
            << "Feature '" << f << "' in available list but isFeatureAvailable returns false";
    }
}

TEST(EditionManager, GetUnavailableFeaturesConsistentWithIsFeatureAvailable) {
    const auto unavail = EditionManager::instance().getUnavailableFeatures();
    for (const auto& f : unavail) {
        EXPECT_FALSE(EditionManager::instance().isFeatureAvailable(f))
            << "Feature '" << f << "' in unavailable list but isFeatureAvailable returns true";
    }
}

// ============================================================================
// 6. Upgrade messages
// ============================================================================

TEST(EditionManager, UpgradeMessage_AvailableFeature_Empty) {
    // If the feature is available, there is no upgrade message.
    const auto avail = EditionManager::instance().getAvailableFeatures();
    for (const auto& f : avail) {
        EXPECT_TRUE(EditionManager::instance().getUpgradeMessage(f).empty())
            << "No upgrade message expected for available feature '" << f << "'";
    }
}

TEST(EditionManager, UpgradeMessage_UnavailableFeature_NotEmpty) {
    const auto unavail = EditionManager::instance().getUnavailableFeatures();
    for (const auto& f : unavail) {
        EXPECT_FALSE(EditionManager::instance().getUpgradeMessage(f).empty())
            << "Upgrade message expected for unavailable feature '" << f << "'";
    }
}

TEST(EditionManager, UpgradeMessage_UnknownFeature_Empty) {
    // Unknown features are always available → no upgrade message.
    RuntimeLicenseGate::instance().initialize(makeResult(true, "active"), makeLicense());
    EXPECT_TRUE(EditionManager::instance().getUpgradeMessage("totally_unknown").empty());
}

// ============================================================================
// 7. Integration with RuntimeLicenseGate
// ============================================================================

TEST(EditionManager, ReflectsLicenseGateStateChange) {
    // Switch license gate to active → update → expired.
    // The EditionManager must reflect the change because it delegates to the gate.
    RuntimeLicenseGate& gate = RuntimeLicenseGate::instance();

    gate.update(makeResult(true, "active"), makeLicense());
    // "nonexistent_feature_xyz" is always allowed regardless of gate state.
    EXPECT_TRUE(EditionManager::instance().isFeatureAvailable("nonexistent_feature_xyz"));

    gate.update(makeResult(false, "expired"), std::nullopt);
    // Known enterprise features must be blocked on expired license.
    for (const char* feat : {"enterprise_plugins", "multi_master",
                              "field_encryption", "rbac", "hsm"}) {
        EXPECT_FALSE(EditionManager::instance().isFeatureAvailable(feat))
            << "Feature '" << feat << "' should be blocked after license expiry";
    }
}
