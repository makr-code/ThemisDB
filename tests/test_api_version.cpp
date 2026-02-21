/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_api_version.cpp                               ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-02-21 14:17:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     297                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_api_version.cpp
 * @brief Unit tests for API versioning functionality
 */

#include <gtest/gtest.h>
#include "server/api_version.h"
#include "server/api_version_config.h"

using namespace themis::server;

class APIVersionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup runs before each test
    }

    void TearDown() override {
        // Cleanup after each test
    }
};

// Test APIVersion parsing
TEST_F(APIVersionTest, ParseValidVersion) {
    auto v1 = APIVersion::parse("v1.4.1");
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(v1->major, 1);
    EXPECT_EQ(v1->minor, 4);
    EXPECT_EQ(v1->patch, 1);
}

TEST_F(APIVersionTest, ParseVersionWithoutV) {
    auto v1 = APIVersion::parse("1.4.1");
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(v1->major, 1);
    EXPECT_EQ(v1->minor, 4);
    EXPECT_EQ(v1->patch, 1);
}

TEST_F(APIVersionTest, ParseVersionMinorOnly) {
    auto v1 = APIVersion::parse("v1.4");
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(v1->major, 1);
    EXPECT_EQ(v1->minor, 4);
    EXPECT_EQ(v1->patch, 0);
}

TEST_F(APIVersionTest, ParseVersionMajorOnly) {
    auto v1 = APIVersion::parse("v1");
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(v1->major, 1);
    EXPECT_EQ(v1->minor, 0);
    EXPECT_EQ(v1->patch, 0);
}

TEST_F(APIVersionTest, ParseLatestKeyword) {
    auto v1 = APIVersion::parse("latest");
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(v1->major, APIVersionConfig::CURRENT_MAJOR);
    EXPECT_EQ(v1->minor, APIVersionConfig::CURRENT_MINOR);
    EXPECT_EQ(v1->patch, APIVersionConfig::CURRENT_PATCH);
}

TEST_F(APIVersionTest, ParseInvalidVersion) {
    auto v1 = APIVersion::parse("invalid");
    EXPECT_FALSE(v1.has_value());
}

TEST_F(APIVersionTest, ParseEmptyString) {
    auto v1 = APIVersion::parse("");
    EXPECT_FALSE(v1.has_value());
}

// Test version string conversion
TEST_F(APIVersionTest, ToStringFormatted) {
    APIVersion v{1, 4, 1};
    EXPECT_EQ(v.toString(), "v1.4.1");
}

// Test version comparison operators
TEST_F(APIVersionTest, VersionEquality) {
    APIVersion v1{1, 4, 1};
    APIVersion v2{1, 4, 1};
    APIVersion v3{1, 4, 2};
    
    EXPECT_TRUE(v1 == v2);
    EXPECT_FALSE(v1 == v3);
    EXPECT_TRUE(v1 != v3);
}

TEST_F(APIVersionTest, VersionComparison) {
    APIVersion v1_0{1, 0, 0};
    APIVersion v1_4{1, 4, 0};
    APIVersion v1_4_1{1, 4, 1};
    APIVersion v2_0{2, 0, 0};
    
    // Less than
    EXPECT_TRUE(v1_0 < v1_4);
    EXPECT_TRUE(v1_4 < v1_4_1);
    EXPECT_TRUE(v1_4_1 < v2_0);
    EXPECT_FALSE(v1_4 < v1_0);
    
    // Greater than
    EXPECT_TRUE(v1_4 > v1_0);
    EXPECT_TRUE(v2_0 > v1_4_1);
    EXPECT_FALSE(v1_0 > v1_4);
    
    // Less than or equal
    EXPECT_TRUE(v1_0 <= v1_4);
    EXPECT_TRUE(v1_4 <= v1_4);
    
    // Greater than or equal
    EXPECT_TRUE(v1_4 >= v1_0);
    EXPECT_TRUE(v1_4 >= v1_4);
}

// Test APIVersionManager
class APIVersionManagerTest : public ::testing::Test {
protected:
    APIVersionManager manager;
};

TEST_F(APIVersionManagerTest, GetCurrentVersion) {
    auto version = manager.getCurrentVersion();
    EXPECT_EQ(version.major, APIVersionConfig::CURRENT_MAJOR);
    EXPECT_EQ(version.minor, APIVersionConfig::CURRENT_MINOR);
    EXPECT_EQ(version.patch, APIVersionConfig::CURRENT_PATCH);
}

TEST_F(APIVersionManagerTest, GetMinimumVersion) {
    auto version = manager.getMinimumVersion();
    EXPECT_EQ(version.major, APIVersionConfig::MINIMUM_MAJOR);
    EXPECT_EQ(version.minor, APIVersionConfig::MINIMUM_MINOR);
    EXPECT_EQ(version.patch, APIVersionConfig::MINIMUM_PATCH);
}

TEST_F(APIVersionManagerTest, IsVersionSupported) {
    EXPECT_TRUE(manager.isVersionSupported(APIVersion{1, 0, 0}));
    EXPECT_TRUE(manager.isVersionSupported(APIVersion{1, 4, 1}));
    EXPECT_FALSE(manager.isVersionSupported(APIVersion{2, 0, 0})); // Future version
    EXPECT_FALSE(manager.isVersionSupported(APIVersion{0, 9, 0})); // Too old
}

TEST_F(APIVersionManagerTest, ResolveVersionEmpty) {
    auto version = manager.resolveVersion("");
    EXPECT_EQ(version, manager.getCurrentVersion());
}

TEST_F(APIVersionManagerTest, ResolveVersionValid) {
    auto version = manager.resolveVersion("v1.3.0");
    EXPECT_EQ(version.major, 1);
    EXPECT_EQ(version.minor, 3);
    EXPECT_EQ(version.patch, 0);
}

TEST_F(APIVersionManagerTest, ResolveVersionInvalid) {
    auto version = manager.resolveVersion("invalid");
    // Should fall back to current version
    EXPECT_EQ(version, manager.getCurrentVersion());
}

TEST_F(APIVersionManagerTest, ResolveVersionUnsupported) {
    auto version = manager.resolveVersion("v2.0.0");
    // Should fall back to current version
    EXPECT_EQ(version, manager.getCurrentVersion());
}

TEST_F(APIVersionManagerTest, GetSupportedVersions) {
    auto versions = manager.getSupportedVersions();
    EXPECT_FALSE(versions.empty());
    EXPECT_GE(versions.size(), 6); // At least v1.0 through v1.4.1
    
    // Check that current version is in the list
    bool found_current = false;
    for (const auto& v : versions) {
        if (v == manager.getCurrentVersion()) {
            found_current = true;
            break;
        }
    }
    EXPECT_TRUE(found_current);
}

TEST_F(APIVersionManagerTest, GetDeprecationInfoNonExistent) {
    auto deprecation = manager.getDeprecationInfo("/api/test", APIVersion{1, 4, 0});
    EXPECT_FALSE(deprecation.has_value());
}

TEST_F(APIVersionManagerTest, RegisterAndGetDeprecation) {
    APIDeprecationInfo info;
    info.deprecated_in = APIVersion{1, 3, 0};
    info.removed_in = APIVersion{2, 0, 0};
    info.reason = "Feature superseded by new implementation";
    info.migration_guide_url = "https://docs.themisdb.com/migration/old-to-new";
    info.alternative = "/api/new-endpoint";
    
    manager.registerDeprecation("/api/old-endpoint", info);
    
    // Should return deprecation info for affected versions
    auto deprecation = manager.getDeprecationInfo("/api/old-endpoint", APIVersion{1, 3, 0});
    ASSERT_TRUE(deprecation.has_value());
    EXPECT_EQ(deprecation->reason, "Feature superseded by new implementation");
    EXPECT_EQ(deprecation->migration_guide_url, "https://docs.themisdb.com/migration/old-to-new");
    EXPECT_EQ(deprecation->alternative, "/api/new-endpoint");
}

TEST_F(APIVersionManagerTest, DeprecationVersionRange) {
    APIDeprecationInfo info;
    info.deprecated_in = APIVersion{1, 2, 0};
    info.removed_in = APIVersion{1, 5, 0};
    info.reason = "Test deprecation";
    
    manager.registerDeprecation("/api/deprecated", info);
    
    // Before deprecation - should not be deprecated
    auto before = manager.getDeprecationInfo("/api/deprecated", APIVersion{1, 1, 0});
    EXPECT_FALSE(before.has_value());
    
    // During deprecation period - should be deprecated
    auto during = manager.getDeprecationInfo("/api/deprecated", APIVersion{1, 3, 0});
    EXPECT_TRUE(during.has_value());
    
    // After removal - should not return deprecation info
    auto after = manager.getDeprecationInfo("/api/deprecated", APIVersion{1, 5, 0});
    EXPECT_FALSE(after.has_value());
}

// Test version ordering
TEST_F(APIVersionManagerTest, VersionOrdering) {
    auto versions = manager.getSupportedVersions();
    
    // Versions should be in ascending order
    for (size_t i = 1; i < versions.size(); ++i) {
        EXPECT_TRUE(versions[i-1] < versions[i] || versions[i-1] == versions[i]);
    }
}

// Integration test
TEST(APIVersionIntegrationTest, FullVersionNegotiationFlow) {
    APIVersionManager manager;
    
    // Client requests with version header
    std::string client_version = "v1.4.0";
    auto resolved = manager.resolveVersion(client_version);
    
    EXPECT_TRUE(manager.isVersionSupported(resolved));
    EXPECT_EQ(resolved.toString(), "v1.4.0");
    
    // Check for deprecations
    auto deprecation = manager.getDeprecationInfo("/api/entities", resolved);
    EXPECT_FALSE(deprecation.has_value()); // No deprecations currently
}

// Edge cases
TEST(APIVersionEdgeCases, LargeVersionNumbers) {
    APIVersion v{999, 999, 999};
    EXPECT_EQ(v.toString(), "v999.999.999");
}

TEST(APIVersionEdgeCases, ZeroVersion) {
    APIVersion v{0, 0, 0};
    EXPECT_EQ(v.toString(), "v0.0.0");
}

TEST(APIVersionEdgeCases, PartialVersionComparison) {
    APIVersion v1_4{1, 4, 0};
    APIVersion v1_4_1{1, 4, 1};
    
    EXPECT_TRUE(v1_4 < v1_4_1);
    EXPECT_FALSE(v1_4 > v1_4_1);
}

// ---------------------------------------------------------------------------
// APIVersionManager / version endpoint contract
// These tests verify the fields that are exposed via GET /version
// ---------------------------------------------------------------------------

TEST(APIVersionEndpointContract, CurrentVersionMatchesConfig) {
    APIVersionManager mgr;
    auto cur = mgr.getCurrentVersion();
    EXPECT_EQ(cur.major, APIVersionConfig::CURRENT_MAJOR);
    EXPECT_EQ(cur.minor, APIVersionConfig::CURRENT_MINOR);
    EXPECT_EQ(cur.patch, APIVersionConfig::CURRENT_PATCH);
}

TEST(APIVersionEndpointContract, MinimumVersionMatchesConfig) {
    APIVersionManager mgr;
    auto min = mgr.getMinimumVersion();
    EXPECT_EQ(min.major, APIVersionConfig::MINIMUM_MAJOR);
    EXPECT_EQ(min.minor, APIVersionConfig::MINIMUM_MINOR);
    EXPECT_EQ(min.patch, APIVersionConfig::MINIMUM_PATCH);
}

TEST(APIVersionEndpointContract, SupportedVersionsContainCurrentAndMinimum) {
    APIVersionManager mgr;
    auto supported = mgr.getSupportedVersions();
    ASSERT_FALSE(supported.empty());

    auto cur = mgr.getCurrentVersion();
    auto min = mgr.getMinimumVersion();

    EXPECT_NE(std::find(supported.begin(), supported.end(), cur), supported.end())
        << "current version must be in supported_api_versions";
    EXPECT_NE(std::find(supported.begin(), supported.end(), min), supported.end())
        << "minimum version must be in supported_api_versions";
}

TEST(APIVersionEndpointContract, CurrentVersionToStringHasVPrefix) {
    APIVersionManager mgr;
    auto s = mgr.getCurrentVersion().toString();
    EXPECT_EQ(s[0], 'v') << "version string must start with 'v'";
}
