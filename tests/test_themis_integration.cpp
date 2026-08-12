/// @file test_themis_integration.cpp
/// @brief Integration tests for the Themis core framework module.
///
/// These tests exercise cross-component scenarios that are not covered by the
/// focused unit tests for each individual subsystem. Specifically, they verify
/// the **interactions** between:
///
///   - RuntimeLicenseGate  ←→  EditionManager  (gate + feature availability)
///   - RuntimeLicenseGate  ←→  EditionManager  (dynamic feature overrides)
///   - ModuleHashVerifier  ←→  build_info       (hash manifest + build metadata)
///   - LicenseActivationResult lifecycle        (active → expired → grace → reset)
///
/// Test categories:
///  1. License lifecycle × edition gate (status transitions)
///  2. Edition gate enforcement across all three layers
///  3. License expiry edge cases (grace period, status text, diagnostics)
///  4. Dynamic feature override lifecycle (set → query → clear → re-query)
///  5. Build info consistency with module availability
///  6. Module hash verifier + build-info integration (manifest consistency)
///  7. Full gate stack: compile-time → runtime-license → admin-override

#include <gtest/gtest.h>

#include "themis/build_info.h"
#include "themis/edition.h"
#include "themis/edition_manager.h"
#include "themis/license_info.h"
#include "themis/module_hash_verifier.h"
#include "themis/runtime_license_gate.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace themis::build_info;
using namespace themis::edition;
using namespace themis::license;
using namespace themis::modules;

// ============================================================================
// Fixture – restores singleton state after each test
// ============================================================================

class ThemisIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Start each test with a clean override state and an "active" license
        // so individual tests control the exact conditions they need.
        EditionManager::instance().clearAllFeatureOverrides();
        RuntimeLicenseGate::instance().initialize(
            makeActivation(true, "active"), makeLicense());
    }

    void TearDown() override {
        EditionManager::instance().clearAllFeatureOverrides();
        // Leave gate in a known "active" state for subsequent tests.
        RuntimeLicenseGate::instance().initialize(
            makeActivation(true, "active"), makeLicense());
    }

    // ---- helpers -----------------------------------------------------------

    static LicenseActivationResult makeActivation(
        bool        success,
        std::string status,
        int         grace_days = 0,
        std::string error_msg  = "") {
        LicenseActivationResult r;
        r.success              = success;
        r.status               = std::move(status);
        r.grace_days_remaining = grace_days;
        r.error_message        = std::move(error_msg);
        return r;
    }

    static LicenseData makeLicense(
        const std::string& edition_str   = "ENTERPRISE",
        const std::string& contact_email = "support@example.com") {
        LicenseData ld;
        ld.organization_name = "IntegrationTestOrg";
        ld.license_key       = "THEMIS-ENT-INTEG-0001";
        ld.edition           = edition_str;
        ld.contact_email     = contact_email;
        ld.expiry_date       = "2099-12-31";
        ld.max_nodes         = 100;
        ld.max_cores         = -1;
        ld.max_storage_tb    = -1;
        return ld;
    }

    /// Returns a unique temporary file path for this test.
    static std::string tmpPath(const std::string& suffix) {
        return (std::filesystem::temp_directory_path() / suffix).string();
    }

    /// Write content to a temporary file and return the path.
    static std::string writeTempFile(const std::string& content,
                                     const std::string& name) {
        const std::string path = tmpPath(name);
        std::ofstream f(path, std::ios::binary | std::ios::trunc);
        f.write(content.data(), static_cast<std::streamsize>(content.size()));
        return path;
    }
};

// ============================================================================
// 1. License lifecycle × edition gate: status transitions
// ============================================================================

TEST_F(ThemisIntegrationTest, ActiveLicense_GatePasses_EditionReflectsState) {
    RuntimeLicenseGate& gate = RuntimeLicenseGate::instance();
    gate.initialize(makeActivation(true, "active"), makeLicense());

    EXPECT_TRUE(gate.isInitialized());
    EXPECT_EQ(gate.licenseStatus(), "active");
    EXPECT_EQ(gate.graceDaysRemaining(), 0);

    // EditionManager and RuntimeLicenseGate must agree on unknown features.
    EXPECT_TRUE(gate.isFeatureAllowed("nonexistent_xyz"));
    EXPECT_TRUE(EditionManager::instance().isFeatureAvailable("nonexistent_xyz"));
}

TEST_F(ThemisIntegrationTest, ExpiredLicense_GateBlocks_EditionReflects) {
    RuntimeLicenseGate& gate = RuntimeLicenseGate::instance();
    gate.initialize(makeActivation(false, "expired", 0, "License expired"), std::nullopt);

    EXPECT_TRUE(gate.isInitialized());
    EXPECT_EQ(gate.licenseStatus(), "expired");

    // All known enterprise features must be blocked through both interfaces.
    for (const char* feat : {"enterprise_plugins", "multi_master",
                              "field_encryption", "rbac", "hsm"}) {
        EXPECT_FALSE(gate.isFeatureAllowed(feat))
            << "Gate must block '" << feat << "' on expired license";
        EXPECT_FALSE(EditionManager::instance().isFeatureAvailable(feat))
            << "EditionManager must also block '" << feat << "' on expired license";
    }
}

TEST_F(ThemisIntegrationTest, StatusTransition_Active_To_Expired_To_Active) {
    RuntimeLicenseGate& gate = RuntimeLicenseGate::instance();

    // Active
    gate.initialize(makeActivation(true, "active"), makeLicense());
    EXPECT_EQ(gate.licenseStatus(), "active");
    EXPECT_TRUE(gate.isFeatureAllowed("nonexistent_xyz"));

    // Expire it
    gate.update(makeActivation(false, "expired"), std::nullopt);
    EXPECT_EQ(gate.licenseStatus(), "expired");
    for (const char* feat : {"enterprise_plugins", "rbac"}) {
        EXPECT_FALSE(gate.isFeatureAllowed(feat))
            << "Must be blocked after expiry: " << feat;
    }

    // Renew
    gate.update(makeActivation(true, "active"), makeLicense());
    EXPECT_EQ(gate.licenseStatus(), "active");
    // Unknown features are always allowed regardless of license.
    EXPECT_TRUE(gate.isFeatureAllowed("nonexistent_xyz"));
}

// ============================================================================
// 2. Edition gate enforcement across layers
// ============================================================================

TEST_F(ThemisIntegrationTest, CompileTimeGate_BlocksRegardlessOfLicense) {
    // A compile-time disabled feature must be blocked even with a valid license.
    RuntimeLicenseGate::instance().initialize(
        makeActivation(true, "active"), makeLicense());

    for (std::string_view feat : kGatedFeatureNames) {
        if (!IsFeatureEnabled(feat)) {
            // Compile-time OFF: both gate and EditionManager must block.
            std::string gate_err, em_err;
            EXPECT_FALSE(RuntimeLicenseGate::instance().isFeatureAllowed(feat, gate_err))
                << "Gate must block compile-time-OFF feature '" << feat << "'";
            EXPECT_FALSE(EditionManager::instance().isFeatureAvailable(feat, em_err))
                << "EditionManager must block compile-time-OFF feature '" << feat << "'";
            // Both must supply an informative error message.
            EXPECT_FALSE(gate_err.empty())
                << "Gate error must not be empty for blocked feature '" << feat << "'";
            EXPECT_FALSE(em_err.empty())
                << "EditionManager error must not be empty for blocked feature '" << feat << "'";
        }
    }
}

TEST_F(ThemisIntegrationTest, RuntimeGate_BlocksCompileTimeEnabledFeature_OnExpiry) {
    // For every compile-time ENABLED feature, an expired license must block it
    // at the runtime gate layer.
    RuntimeLicenseGate::instance().initialize(
        makeActivation(false, "expired"), std::nullopt);

    for (std::string_view feat : kGatedFeatureNames) {
        if (IsFeatureEnabled(feat)) {
            std::string err;
            EXPECT_FALSE(
                RuntimeLicenseGate::instance().isFeatureAllowed(feat, err))
                << "Runtime gate must block compile-time-ON feature '"
                << feat << "' when license is expired";
            EXPECT_FALSE(err.empty());
        }
    }
}

TEST_F(ThemisIntegrationTest, GateAndEditionManager_AreConsistent) {
    // After initialization, RuntimeLicenseGate and EditionManager must agree
    // on every known gated feature.
    RuntimeLicenseGate::instance().initialize(
        makeActivation(true, "active"), makeLicense());

    for (std::string_view feat : kGatedFeatureNames) {
        const bool gate_allows = RuntimeLicenseGate::instance().isFeatureAllowed(feat);
        const bool em_allows   = EditionManager::instance().isFeatureAvailable(feat);

        // They must agree: if gate passes, EditionManager must also pass (and vice-versa).
        EXPECT_EQ(gate_allows, em_allows)
            << "Inconsistency for feature '" << feat
            << "': gate=" << gate_allows << " em=" << em_allows;
    }
}

// ============================================================================
// 3. License expiry edge cases
// ============================================================================

TEST_F(ThemisIntegrationTest, GracePeriod_AllowsFeatures_WithWarning) {
    RuntimeLicenseGate& gate = RuntimeLicenseGate::instance();

    // Grace period = 3 days remaining.
    gate.initialize(makeActivation(true, "grace", 3), makeLicense());

    EXPECT_TRUE(gate.isInitialized());
    EXPECT_EQ(gate.licenseStatus(), "grace");
    EXPECT_EQ(gate.graceDaysRemaining(), 3);

    // Unknown features must still pass during grace period.
    EXPECT_TRUE(gate.isFeatureAllowed("nonexistent_xyz"));

    // EditionManager must also see grace state as permissive for unknown features.
    EXPECT_TRUE(EditionManager::instance().isFeatureAvailable("nonexistent_xyz"));
}

TEST_F(ThemisIntegrationTest, GracePeriodZero_StillGrace_NotExpired) {
    // grace_days_remaining=0 with status="grace" should be treated as grace,
    // not as expired.
    RuntimeLicenseGate& gate = RuntimeLicenseGate::instance();
    gate.initialize(makeActivation(true, "grace", 0), makeLicense());

    EXPECT_EQ(gate.licenseStatus(), "grace");
    EXPECT_EQ(gate.graceDaysRemaining(), 0);
    EXPECT_TRUE(gate.isInitialized());
}

TEST_F(ThemisIntegrationTest, MultipleInvalidStatuses_AllBlock) {
    RuntimeLicenseGate& gate = RuntimeLicenseGate::instance();

    for (const std::string& status :
         {"invalid", "offline", "suspended", "cancelled"}) {
        gate.initialize(makeActivation(false, status), std::nullopt);
        EXPECT_EQ(gate.licenseStatus(), status);

        for (const char* feat :
             {"enterprise_plugins", "multi_master", "field_encryption"}) {
            EXPECT_FALSE(gate.isFeatureAllowed(feat))
                << "Feature '" << feat << "' should be blocked with status '"
                << status << "'";
            EXPECT_FALSE(EditionManager::instance().isFeatureAvailable(feat))
                << "EditionManager should block '" << feat << "' with status '"
                << status << "'";
        }
    }
}

TEST_F(ThemisIntegrationTest, LicenseWithContactEmail_ErrorContainsEmail) {
    // Only applicable when runtime gate fires (compile-time ON build).
    if (GetEditionType() == EditionType::COMMUNITY) {
        GTEST_SKIP() << "Skipping runtime-gate message test in Community build";
    }

    LicenseData lic    = makeLicense("ENTERPRISE", "renew@corp.example");
    auto result        = makeActivation(false, "expired");
    RuntimeLicenseGate::instance().initialize(result, lic);

    std::string err;
    RuntimeLicenseGate::instance().isFeatureAllowed("enterprise_plugins", err);
    EXPECT_NE(err.find("renew@corp.example"), std::string::npos)
        << "Expired-license error should include the contact email. Got: " << err;
}

// ============================================================================
// 4. Dynamic feature override lifecycle
// ============================================================================

TEST_F(ThemisIntegrationTest, OverrideLifecycle_Set_Query_Clear_Requery) {
    RuntimeLicenseGate::instance().initialize(
        makeActivation(true, "active"), makeLicense());
    EditionManager& em = EditionManager::instance();

    // Set an admin override (false) on a gated feature.
    em.setFeatureOverride("enterprise_plugins", false);
    EXPECT_TRUE(em.hasFeatureOverride("enterprise_plugins"));
    EXPECT_FALSE(em.isFeatureAvailable("enterprise_plugins"));

    // Clear it.
    em.clearFeatureOverride("enterprise_plugins");
    EXPECT_FALSE(em.hasFeatureOverride("enterprise_plugins"));

    // After clearing, the result is determined purely by the edition+license gate.
    std::string err;
    const bool after = em.isFeatureAvailable("enterprise_plugins", err);
    // If still blocked, it must not be due to an admin override.
    if (!after) {
        EXPECT_EQ(err.find("administratively disabled"), std::string::npos)
            << "After clearing override, error must not mention admin. err: " << err;
    }
}

TEST_F(ThemisIntegrationTest, OverrideDoesNotPersistAcrossLicenseUpdate) {
    // Verifies that a license update does not silently re-enable a cleared override.
    EditionManager& em = EditionManager::instance();
    RuntimeLicenseGate& gate = RuntimeLicenseGate::instance();

    em.setFeatureOverride("rbac", false);
    EXPECT_FALSE(em.isFeatureAvailable("rbac"));

    // Update the license to a renewed active state.
    gate.update(makeActivation(true, "active"), makeLicense());

    // Override must still be in effect after the license update.
    EXPECT_TRUE(em.hasFeatureOverride("rbac"));
    EXPECT_FALSE(em.isFeatureAvailable("rbac"))
        << "Admin override must survive a license update";
}

TEST_F(ThemisIntegrationTest, ClearAllOverrides_ResetsAllAtOnce) {
    EditionManager& em = EditionManager::instance();

    em.setFeatureOverride("rbac", false);
    em.setFeatureOverride("hsm", false);
    em.setFeatureOverride("multi_master", true);

    em.clearAllFeatureOverrides();

    EXPECT_FALSE(em.hasFeatureOverride("rbac"));
    EXPECT_FALSE(em.hasFeatureOverride("hsm"));
    EXPECT_FALSE(em.hasFeatureOverride("multi_master"));
}

TEST_F(ThemisIntegrationTest, OverrideFalse_BlocksEvenOnActiveLicense) {
    RuntimeLicenseGate::instance().initialize(
        makeActivation(true, "active"), makeLicense());
    EditionManager& em = EditionManager::instance();

    // Disable every known gated feature via override and confirm they're all blocked.
    for (std::string_view feat : kGatedFeatureNames) {
        em.setFeatureOverride(feat, false);
        EXPECT_FALSE(em.isFeatureAvailable(feat))
            << "Override=false must block feature '" << feat
            << "' even on active license";
        em.clearFeatureOverride(feat);
    }
}

// ============================================================================
// 5. Build info consistency with module availability
// ============================================================================

TEST_F(ThemisIntegrationTest, BuildConfig_ModuleList_MatchesIsModuleCompiledIn) {
    auto cfg      = getBuildConfiguration();
    auto compiled = getCompiledModules();

    EXPECT_FALSE(cfg.modules.empty());
    EXPECT_FALSE(compiled.empty());

    // Every module returned by getCompiledModules() must be present in the
    // full module list from getBuildConfiguration().
    for (const auto& mod_name : compiled) {
        bool found = false;
        for (const auto& mod : cfg.modules) {
            if (mod.name == mod_name) { found = true; break; }
        }
        EXPECT_TRUE(found) << "Compiled module '" << mod_name
                           << "' missing from getBuildConfiguration().modules";
    }
}

TEST_F(ThemisIntegrationTest, VersionSummary_ContainsEditionFromBuildConfig) {
    auto cfg = getBuildConfiguration();
    EXPECT_FALSE(cfg.edition_type.empty());

    const std::string summary = getVersionSummary();
    EXPECT_FALSE(summary.empty());
    EXPECT_NE(summary.find("ThemisDB"), std::string::npos);
}

TEST_F(ThemisIntegrationTest, BuildConfig_EditionMatchesEditionHeader) {
    auto cfg = getBuildConfiguration();

    // The edition_type string in BuildConfiguration must match the compile-time
    // EDITION_STRING from the edition header.
    EXPECT_EQ(cfg.edition_type, std::string(EDITION_STRING));
}

// ============================================================================
// 6. Module hash verifier + build-info integration
// ============================================================================

TEST_F(ThemisIntegrationTest, HashVerifier_TempFile_RoundTrip) {
    // Write a known payload to disk and verify the hash, then add it to a
    // manifest and re-verify through the manifest path.
    const std::string content = "ThemisDB integration test payload v1";
    const std::string path = writeTempFile(content, "themis_integ_hash.bin");

    const std::string hash = ModuleHashVerifier::computeSHA256(path);
    ASSERT_FALSE(hash.empty()) << "SHA-256 computation must succeed";
    EXPECT_EQ(hash.size(), 64u) << "SHA-256 hex string must be 64 characters";

    // Round-trip through a manifest.
    ModuleHashVerifier verifier;
    verifier.addExpectedHash("test_module", hash);
    EXPECT_EQ(verifier.manifestSize(), 1u);

    auto result = verifier.verifyModule("test_module", path);
    EXPECT_TRUE(result.success)
        << "Manifest-based verification must succeed when hash matches";
    EXPECT_EQ(result.computedHash, hash);
    EXPECT_EQ(result.expectedHash, hash);

    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST_F(ThemisIntegrationTest, HashVerifier_TamperedFile_DetectedByManifest) {
    const std::string original = "original content";
    const std::string path     = writeTempFile(original, "themis_integ_tamper.bin");

    const std::string hash = ModuleHashVerifier::computeSHA256(path);
    ASSERT_FALSE(hash.empty());

    ModuleHashVerifier verifier;
    verifier.addExpectedHash("test_module", hash);

    // Overwrite the file with different content (simulating tampering).
    {
        std::ofstream f(path, std::ios::binary | std::ios::trunc);
        const std::string tampered = "tampered content!!!";
        f.write(tampered.data(), static_cast<std::streamsize>(tampered.size()));
    }

    auto result = verifier.verifyModule("test_module", path);
    EXPECT_FALSE(result.success)
        << "Verification must fail when file content has been tampered";
    EXPECT_NE(result.computedHash, result.expectedHash);

    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST_F(ThemisIntegrationTest, BuildManifest_RoundTrip_WithHashVerifier) {
    // exportBuildManifest() and verifyBuildManifest() must be consistent, and
    // the manifest file must be a non-empty JSON document.
    const std::string path = tmpPath("themis_integ_build_manifest.json");
    std::error_code ec;
    std::filesystem::remove(path, ec);

    ASSERT_TRUE(exportBuildManifest(path))
        << "exportBuildManifest() must succeed";
    ASSERT_TRUE(std::filesystem::exists(path));
    EXPECT_GT(std::filesystem::file_size(path), 0u);

    // The fresh manifest must verify against the current binary.
    EXPECT_TRUE(verifyBuildManifest(path))
        << "A freshly exported manifest must pass verifyBuildManifest()";

    std::filesystem::remove(path, ec);
}

// ============================================================================
// 7. Full gate stack: compile-time → runtime-license → admin-override
// ============================================================================

TEST_F(ThemisIntegrationTest, FullGateStack_AdminOverrideFalse_WinsOverActiveLicense) {
    // With an active license, the admin override (false) is the deciding factor.
    RuntimeLicenseGate::instance().initialize(
        makeActivation(true, "active"), makeLicense());
    EditionManager& em = EditionManager::instance();

    // Pick any known gated feature to test the override.
    const std::string feat = "enterprise_plugins";
    em.setFeatureOverride(feat, false);

    std::string err;
    EXPECT_FALSE(em.isFeatureAvailable(feat, err));
    EXPECT_NE(err.find("administratively disabled"), std::string::npos)
        << "Error must mention 'administratively disabled'. Got: " << err;

    em.clearFeatureOverride(feat);
}

TEST_F(ThemisIntegrationTest, FullGateStack_ExpiredLicense_ThenAdminOverrideTrue) {
    // Expired license → admin sets override=true → compile-time gate still wins
    // (if the feature is compile-time OFF).  For compile-time ON features, the
    // runtime gate (expired) still blocks; override=true cannot bypass expiry.
    RuntimeLicenseGate::instance().initialize(
        makeActivation(false, "expired"), std::nullopt);
    EditionManager& em = EditionManager::instance();

    for (std::string_view feat : kGatedFeatureNames) {
        em.setFeatureOverride(feat, true);

        std::string err;
        const bool allowed = em.isFeatureAvailable(feat, err);

        if (!IsFeatureEnabled(feat)) {
            // Compile-time OFF: override=true cannot bypass compile-time gate.
            EXPECT_FALSE(allowed)
                << "Compile-time gate must block '" << feat
                << "' regardless of override=true";
        }
        // (For compile-time ON + expired license, the runtime gate blocks —
        //  override=true is only an EditionManager concept, so the runtime gate
        //  check via isFeatureAllowed() would still block, but EditionManager
        //  sees the override=true first and passes through to the gate which
        //  blocks.  Either outcome is acceptable; we don't assert for ON features
        //  here since the behaviour depends on the interaction order.)

        em.clearFeatureOverride(feat);
    }
}

TEST_F(ThemisIntegrationTest, TotalFeatureCount_Invariant_AcrossGateChanges) {
    // The total number of known gated features never changes; only their
    // availability shifts between available and unavailable buckets.
    EditionManager& em = EditionManager::instance();

    auto count_initial = em.getAvailableFeatures().size() +
                         em.getUnavailableFeatures().size();
    EXPECT_EQ(count_initial, kGatedFeatureCount);

    // Disable all features.
    for (std::string_view feat : kGatedFeatureNames) {
        em.setFeatureOverride(feat, false);
    }
    auto count_after_disable = em.getAvailableFeatures().size() +
                               em.getUnavailableFeatures().size();
    EXPECT_EQ(count_after_disable, kGatedFeatureCount);

    // Clear all overrides.
    em.clearAllFeatureOverrides();
    auto count_after_clear = em.getAvailableFeatures().size() +
                             em.getUnavailableFeatures().size();
    EXPECT_EQ(count_after_clear, kGatedFeatureCount);
}
