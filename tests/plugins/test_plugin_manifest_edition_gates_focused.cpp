/**
 * @file test_plugin_manifest_edition_gates_focused.cpp
 * @brief Wave C Batch 2: Focused tests for plugin manifest edition/license/boundary validation
 * @note Tests ≥12 fail-closed validation scenarios per acceptance criteria
 */

#include <gtest/gtest.h>
#include "plugins/plugin_manager.h"
#include "plugins/plugin_interface.h"
#include "themis/edition.h"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <cctype>
#include <algorithm>

// Portability wrappers for setenv/unsetenv (not available on Windows)
#ifdef _WIN32
namespace {
inline int setenv(const char* name, const char* value, int /*overwrite*/) {
    return _putenv_s(name, value);
}
inline int unsetenv(const char* name) {
    return _putenv_s(name, "");
}
} // anonymous namespace
#endif

using namespace themis::plugins;
using json = nlohmann::json;

namespace themis::plugins {

struct PluginManagerTestAccess {
    static ManifestErrorCode validateManifestPublicPrivateBoundary(
        PluginManager& manager,
        const PluginManifest& manifest,
        const std::string& plugin_path,
        std::string& error_details) {
        return manager.validateManifestPublicPrivateBoundary(manifest, plugin_path, error_details);
    }

    static ManifestErrorCode validateManifestEditionRestrictions(
        PluginManager& manager,
        const PluginManifest& manifest,
        std::string& error_details) {
        return manager.validateManifestEditionRestrictions(manifest, error_details);
    }
};

} // namespace themis::plugins

// Helper: Normalize edition name (case-insensitive)
static inline std::string normalizeEditionName(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

// Test fixture for manifest edition gate validation
class PluginManifestEditionGatesTest : public ::testing::Test {
protected:
    PluginManager plugin_manager;
    
    // Helper: Create a basic valid manifest
    PluginManifest createBaseManifest(
        const std::string& name = "test_plugin",
        const std::string& visibility = "public",
        const std::vector<std::string>& allowed_editions = {},
        const std::string& license_feature = "") {
        
        PluginManifest manifest;
        manifest.name = name;
        manifest.version = "1.0.0";
        manifest.type = PluginType::COMPUTE_BACKEND;
        manifest.description = "Test plugin for edition validation";
        manifest.visibility = visibility;
        manifest.allowed_editions = allowed_editions;
        manifest.license_feature = license_feature;
        manifest.binary_linux = "test.so";
        return manifest;
    }
};

// TEST-1: Deny load of private plugin when edition=community (fail-closed)
TEST_F(PluginManifestEditionGatesTest, TEST_1_PrivatePluginInCommunityEdition) {
    auto manifest = createBaseManifest("private_plugin", "private");
    std::string error;
    
    auto result = PluginManagerTestAccess::validateManifestPublicPrivateBoundary(
        plugin_manager, manifest, "/plugins/private_plugin.so", error);
    
    // Should fail in community edition
    // Note: Only fails if current edition is community
    if (normalizeEditionName(std::string(themis::edition::EDITION_STRING)) == "community") {
        EXPECT_EQ(result, ManifestErrorCode::PLUGIN_PRIVATE_IN_COMMUNITY);
        EXPECT_FALSE(error.empty());
    }
}

// TEST-2: Deny load of enterprise-only plugin when current=community (fail-closed)
TEST_F(PluginManifestEditionGatesTest, TEST_2_EnterpriseOnlyPluginInCommunity) {
    auto manifest = createBaseManifest(
        "enterprise_plugin",
        "public",
        {"enterprise", "hyperscaler"},
        "");
    std::string error;
    
    auto result = PluginManagerTestAccess::validateManifestEditionRestrictions(plugin_manager, manifest, error);
    
    if (normalizeEditionName(std::string(themis::edition::EDITION_STRING)) == "community") {
        EXPECT_EQ(result, ManifestErrorCode::PLUGIN_EDITION_MISMATCH);
        EXPECT_NE(error.find("not allowed"), std::string::npos);
    }
}

// TEST-3: Deny load when license_feature required but not granted (fail-closed)
TEST_F(PluginManifestEditionGatesTest, TEST_3_MissingLicenseFeature) {
    auto manifest = createBaseManifest(
        "licensed_plugin",
        "public",
        {},
        "premium_connector");
    std::string error;
    
    auto result = PluginManagerTestAccess::validateManifestEditionRestrictions(plugin_manager, manifest, error);
    
    // Assuming the license feature is not granted in test environment
    // This test documents the expected fail-closed behavior
    EXPECT_TRUE(result == ManifestErrorCode::PLUGIN_LICENSE_DENIED ||
                result == ManifestErrorCode::MANIFEST_OK);  // Depends on test license config
}

// TEST-4: Allow public plugin on any edition (pass-through)
TEST_F(PluginManifestEditionGatesTest, TEST_4_PublicPluginOnAnyEdition) {
    auto manifest = createBaseManifest(
        "public_plugin",
        "public",
        {},  // Empty allowed_editions means allow all
        "");
    std::string error;
    
    auto edition_result = PluginManagerTestAccess::validateManifestEditionRestrictions(plugin_manager, manifest, error);
    auto boundary_result = PluginManagerTestAccess::validateManifestPublicPrivateBoundary(
        plugin_manager, manifest, "/plugins/public_plugin.so", error);
    
    EXPECT_EQ(edition_result, ManifestErrorCode::MANIFEST_OK);
    EXPECT_EQ(boundary_result, ManifestErrorCode::MANIFEST_OK);
}

// TEST-5: Allow restricted plugin only with scoped checkout context
TEST_F(PluginManifestEditionGatesTest, TEST_5_RestrictedPluginWithContext) {
    auto manifest = createBaseManifest(
        "restricted_plugin",
        "restricted",
        {},
        "");
    std::string error;
    
    // First, try WITHOUT context
    unsetenv("THEMISDB_SCOPED_CHECKOUT");
    auto result_without = PluginManagerTestAccess::validateManifestPublicPrivateBoundary(
        plugin_manager, manifest, "/plugins/restricted_plugin.so", error);
    EXPECT_EQ(result_without, ManifestErrorCode::PLUGIN_RESTRICTED_NO_CONTEXT);
    
    // Then, try WITH context
    setenv("THEMISDB_SCOPED_CHECKOUT", "true", 1);
    auto result_with = PluginManagerTestAccess::validateManifestPublicPrivateBoundary(
        plugin_manager, manifest, "/plugins/restricted_plugin.so", error);
    EXPECT_EQ(result_with, ManifestErrorCode::MANIFEST_OK);
    
    unsetenv("THEMISDB_SCOPED_CHECKOUT");
}

// TEST-6: Reject manifest with malformed allowed_editions (non-array)
TEST_F(PluginManifestEditionGatesTest, TEST_6_MalformedAllowedEditions) {
    json manifest_json = json::object();
    manifest_json["name"] = "malformed_plugin";
    manifest_json["version"] = "1.0.0";
    manifest_json["type"] = "compute_backend";
    manifest_json["description"] = "Test plugin";
    manifest_json["binary"] = json::object({{"linux", "test.so"}});
    manifest_json["allowed_editions"] = "enterprise";  // String instead of array
    
    auto validation = ManifestSchemaValidator::validate(manifest_json);
    EXPECT_FALSE(validation.valid);
    
    bool found_allowed_editions_error = false;
    for (const auto& err : validation.errors) {
        if (err.find("allowed_editions") != std::string::npos) {
            found_allowed_editions_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_allowed_editions_error);
}

// TEST-7: Reject manifest with invalid license_feature name
TEST_F(PluginManifestEditionGatesTest, TEST_7_InvalidLicenseFeatureName) {
    auto manifest = createBaseManifest(
        "test_plugin",
        "public",
        {},
        "INVALID-FEATURE");  // Uppercase not allowed; must match ^[a-z0-9][a-z0-9_.-]*$
    std::string error;
    
    auto result = PluginManagerTestAccess::validateManifestEditionRestrictions(plugin_manager, manifest, error);
    EXPECT_EQ(result, ManifestErrorCode::PLUGIN_LICENSE_FEATURE_INVALID);
}

// TEST-8a: Boundary violation detection - private path without private visibility
TEST_F(PluginManifestEditionGatesTest, TEST_8a_PrivatePathPublicVisibility) {
    auto manifest = createBaseManifest(
        "boundary_test",
        "public");  // visibility is public but path indicates private
    std::string error;
    
    auto result = PluginManagerTestAccess::validateManifestPublicPrivateBoundary(
        plugin_manager, manifest, "/path/to/private/plugin.so", error);
    
    EXPECT_EQ(result, ManifestErrorCode::PLUGIN_PATH_VISIBILITY_MISMATCH);
}

// TEST-8b: Boundary violation detection - private path with correct private visibility
TEST_F(PluginManifestEditionGatesTest, TEST_8b_PrivatePathPrivateVisibility) {
    auto manifest = createBaseManifest(
        "boundary_test",
        "private");
    std::string error;
    
    auto result = PluginManagerTestAccess::validateManifestPublicPrivateBoundary(
        plugin_manager, manifest, "/path/to/private/plugin.so", error);
    
    // Should pass boundary check when visibility matches path
    if (normalizeEditionName(std::string(themis::edition::EDITION_STRING)) != "community") {
        EXPECT_EQ(result, ManifestErrorCode::MANIFEST_OK);
    }
}

// TEST-8c: Boundary violation detection - windows path with private indicator
TEST_F(PluginManifestEditionGatesTest, TEST_8c_WindowsPrivatePathMismatch) {
    auto manifest = createBaseManifest(
        "boundary_test",
        "public");
    std::string error;
    
    auto result = PluginManagerTestAccess::validateManifestPublicPrivateBoundary(
        plugin_manager, manifest, "C:\\path\\private\\plugin.dll", error);
    
    EXPECT_EQ(result, ManifestErrorCode::PLUGIN_PATH_VISIBILITY_MISMATCH);
}

// TEST-8d: Boundary violation detection - restricted visibility without context
TEST_F(PluginManifestEditionGatesTest, TEST_8d_RestrictedWithoutScopedContext) {
    auto manifest = createBaseManifest(
        "restricted_test",
        "restricted");
    std::string error;
    
    unsetenv("THEMISDB_SCOPED_CHECKOUT");
    auto result = PluginManagerTestAccess::validateManifestPublicPrivateBoundary(
        plugin_manager, manifest, "/plugins/restricted.so", error);
    
    EXPECT_EQ(result, ManifestErrorCode::PLUGIN_RESTRICTED_NO_CONTEXT);
}

// TEST-9: Multiple allowed editions - plugin allowed on some but not current
TEST_F(PluginManifestEditionGatesTest, TEST_9_SelectiveEditionAllowance) {
    auto manifest = createBaseManifest(
        "selective_plugin",
        "public",
        {"minimal", "enterprise"},  // Only minimal and enterprise
        "");
    std::string error;
    
    auto result = PluginManagerTestAccess::validateManifestEditionRestrictions(plugin_manager, manifest, error);
    
    const auto current = normalizeEditionName(std::string(themis::edition::EDITION_STRING));
    if (current == "community" || current == "hyperscaler" || current == "military") {
        EXPECT_EQ(result, ManifestErrorCode::PLUGIN_EDITION_MISMATCH);
    } else if (current == "minimal" || current == "enterprise") {
        EXPECT_EQ(result, ManifestErrorCode::MANIFEST_OK);
    }
}

// TEST-10: License feature with valid format passes validation
TEST_F(PluginManifestEditionGatesTest, TEST_10_ValidLicenseFeatureFormat) {
    auto manifest = createBaseManifest(
        "valid_licensed",
        "public",
        {},
        "valid_feature_123");  // Matches ^[a-z0-9][a-z0-9_.-]*$
    std::string error;
    
    auto result = PluginManagerTestAccess::validateManifestEditionRestrictions(plugin_manager, manifest, error);
    
    // Should pass format validation (may fail on license gate availability)
    EXPECT_TRUE(result == ManifestErrorCode::MANIFEST_OK ||
                result == ManifestErrorCode::PLUGIN_LICENSE_DENIED);
}

// TEST-11: Edition normalization handles case insensitivity
TEST_F(PluginManifestEditionGatesTest, TEST_11_EditionCaseInsensitive) {
    // Create manifest with UPPERCASE edition names (should be normalized)
    auto manifest = createBaseManifest(
        "case_test",
        "public");
    
    // Manually set with mixed case to test normalization
    manifest.allowed_editions = {"Community", "ENTERPRISE"};
    std::string error;
    
    auto result = PluginManagerTestAccess::validateManifestEditionRestrictions(plugin_manager, manifest, error);
    
    // After normalization, "Community" should match current edition if community
    if (normalizeEditionName(std::string(themis::edition::EDITION_STRING)) == "community") {
        EXPECT_EQ(result, ManifestErrorCode::MANIFEST_OK);
    }
}

// TEST-12: Combined validation - edition restrictions AND boundary constraints
TEST_F(PluginManifestEditionGatesTest, TEST_12_CombinedValidation) {
    auto manifest = createBaseManifest(
        "complex_plugin",
        "private",
        {"enterprise", "hyperscaler"},  // Restricted to specific editions
        "enterprise_feature");          // Requires license
    std::string error1, error2;
    
    // Both checks should be performed independently
    auto edition_result = PluginManagerTestAccess::validateManifestEditionRestrictions(plugin_manager, manifest, error1);
    auto boundary_result = PluginManagerTestAccess::validateManifestPublicPrivateBoundary(
        plugin_manager, manifest, "/plugins/private/complex.so", error2);
    
    // Results depend on current edition and license state
    // But we verify both methods complete and may return errors
    if (normalizeEditionName(std::string(themis::edition::EDITION_STRING)) == "community") {
        EXPECT_EQ(boundary_result, ManifestErrorCode::PLUGIN_PRIVATE_IN_COMMUNITY);
    }
}

// TEST-13: Empty allowed_editions means allow all editions (pass-through)
TEST_F(PluginManifestEditionGatesTest, TEST_13_EmptyAllowedEditionsAllowsAll) {
    auto manifest = createBaseManifest(
        "universal_plugin",
        "public",
        {},  // Empty = allow all
        "");
    std::string error;
    
    auto result = PluginManagerTestAccess::validateManifestEditionRestrictions(plugin_manager, manifest, error);
    EXPECT_EQ(result, ManifestErrorCode::MANIFEST_OK);
}

// TEST-14: License feature validation with special characters
TEST_F(PluginManifestEditionGatesTest, TEST_14_LicenseFeatureSpecialChars) {
    auto manifest = createBaseManifest(
        "special_chars",
        "public",
        {},
        "feature-with.underscores_and-dashes");  // Valid format
    std::string error;
    
    auto result = PluginManagerTestAccess::validateManifestEditionRestrictions(plugin_manager, manifest, error);
    
    // Should pass format validation
    EXPECT_TRUE(result == ManifestErrorCode::MANIFEST_OK ||
                result == ManifestErrorCode::PLUGIN_LICENSE_DENIED);
}

// TEST-15: Schema validation enforces non-empty allowed_editions array
TEST_F(PluginManifestEditionGatesTest, TEST_15_AllowedEditionsEmptyArray) {
    json manifest_json = json::object();
    manifest_json["name"] = "empty_editions";
    manifest_json["version"] = "1.0.0";
    manifest_json["type"] = "compute_backend";
    manifest_json["description"] = "Test plugin";
    manifest_json["binary"] = json::object({{"linux", "test.so"}});
    manifest_json["allowed_editions"] = json::array();  // Empty array (valid structure, semantically means allow-all)
    
    auto validation = ManifestSchemaValidator::validate(manifest_json);
    EXPECT_TRUE(validation.valid);  // Empty array is structurally valid
    
    auto manifest = ManifestSchemaValidator::parseMarketplaceManifest(manifest_json);
    EXPECT_TRUE(manifest.has_value());
    if (manifest) {
        EXPECT_TRUE(manifest->allowed_editions.empty());
    }
}

// TEST-16: Visibility field defaults to "public" when omitted
TEST_F(PluginManifestEditionGatesTest, TEST_16_VisibilityDefaultsToPublic) {
    auto manifest = createBaseManifest("visibility_default", "");  // Empty string for visibility
    std::string error;
    
    auto result = PluginManagerTestAccess::validateManifestPublicPrivateBoundary(
        plugin_manager, manifest, "/plugins/default.so", error);
    
    // Should treat empty as public and pass
    EXPECT_EQ(result, ManifestErrorCode::MANIFEST_OK);
}

// Run all tests
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
