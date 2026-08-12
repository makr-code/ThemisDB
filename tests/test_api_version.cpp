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

// Docs: "v1" resolves to latest minor.patch for major == 1 (API_VERSIONING.md line 42)
TEST_F(APIVersionManagerTest, ResolveVersionMajorOnly) {
    auto version = manager.resolveVersion("v1");
    // Must resolve to current (latest) version, not v1.0.0
    EXPECT_EQ(version, manager.getCurrentVersion())
        << "v1 should resolve to latest supported version, not v1.0.0";
    EXPECT_EQ(version.major, 1);
}

// Docs: "v1.4" resolves to latest patch for major.minor == 1.4 (API_VERSIONING.md line 41)
TEST_F(APIVersionManagerTest, ResolveVersionMajorMinorOnly) {
    auto version = manager.resolveVersion("v1.4");
    EXPECT_EQ(version.major, 1);
    EXPECT_EQ(version.minor, 4);
    // Must be the latest patch for 1.4.x, which equals the current patch
    EXPECT_EQ(version.patch, APIVersionConfig::CURRENT_PATCH)
        << "v1.4 should resolve to latest patch for minor 4, not v1.4.0";
}

// Major-only with no matching major falls back to current
TEST_F(APIVersionManagerTest, ResolveVersionUnknownMajorFallsBack) {
    auto version = manager.resolveVersion("v9");
    EXPECT_EQ(version, manager.getCurrentVersion());
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

// ============================================================================
// RouteVersionRouter Tests
// ============================================================================

#include "server/route_version_router.h"

using themis::server::RouteVersionRouter;

// ---------------------------------------------------------------------------
// isVersioned()
// ---------------------------------------------------------------------------

TEST(RouteVersionRouter, IsVersioned_V1) {
    EXPECT_TRUE(RouteVersionRouter::isVersioned("/v1/documents"));
    EXPECT_TRUE(RouteVersionRouter::isVersioned("/v1/query/aql"));
    EXPECT_TRUE(RouteVersionRouter::isVersioned("/v1/"));
}

TEST(RouteVersionRouter, IsVersioned_V2) {
    EXPECT_TRUE(RouteVersionRouter::isVersioned("/v2/documents"));
    EXPECT_TRUE(RouteVersionRouter::isVersioned("/v2/query/stream"));
}

TEST(RouteVersionRouter, IsVersioned_ApiPrefix) {
    EXPECT_TRUE(RouteVersionRouter::isVersioned("/api/v1/graphql"));
    EXPECT_TRUE(RouteVersionRouter::isVersioned("/api/v2/jobs"));
}

TEST(RouteVersionRouter, IsVersioned_Unversioned) {
    EXPECT_FALSE(RouteVersionRouter::isVersioned("/documents"));
    EXPECT_FALSE(RouteVersionRouter::isVersioned("/query/aql"));
    EXPECT_FALSE(RouteVersionRouter::isVersioned("/health"));
    EXPECT_FALSE(RouteVersionRouter::isVersioned("/"));
    EXPECT_FALSE(RouteVersionRouter::isVersioned(""));
}

// ---------------------------------------------------------------------------
// extractVersion()
// ---------------------------------------------------------------------------

TEST(RouteVersionRouter, ExtractVersion_V1) {
    EXPECT_EQ(RouteVersionRouter::extractVersion("/v1/documents"), 1);
    EXPECT_EQ(RouteVersionRouter::extractVersion("/v1/"), 1);
}

TEST(RouteVersionRouter, ExtractVersion_V2) {
    EXPECT_EQ(RouteVersionRouter::extractVersion("/v2/query/stream"), 2);
}

TEST(RouteVersionRouter, ExtractVersion_Unversioned) {
    EXPECT_EQ(RouteVersionRouter::extractVersion("/documents"), 0);
    EXPECT_EQ(RouteVersionRouter::extractVersion("/"), 0);
    EXPECT_EQ(RouteVersionRouter::extractVersion(""), 0);
}

TEST(RouteVersionRouter, ExtractVersion_ApiNested) {
    EXPECT_EQ(RouteVersionRouter::extractVersion("/api/v1/graphql"), 1);
    EXPECT_EQ(RouteVersionRouter::extractVersion("/api/v2/jobs"), 2);
}

// ---------------------------------------------------------------------------
// stripVersionPrefix()
// ---------------------------------------------------------------------------

TEST(RouteVersionRouter, StripPrefix_V1) {
    EXPECT_EQ(RouteVersionRouter::stripVersionPrefix("/v1/documents/abc"),
              "/documents/abc");
}

TEST(RouteVersionRouter, StripPrefix_V2) {
    EXPECT_EQ(RouteVersionRouter::stripVersionPrefix("/v2/query/stream"),
              "/query/stream");
}

TEST(RouteVersionRouter, StripPrefix_Unversioned) {
    EXPECT_EQ(RouteVersionRouter::stripVersionPrefix("/documents"),
              "/documents");
    EXPECT_EQ(RouteVersionRouter::stripVersionPrefix("/"),
              "/");
}

TEST(RouteVersionRouter, StripPrefix_ApiNested_NotStripped) {
    // /api/v1/graphql is kept as-is (only /vN/ top-level prefix is stripped)
    EXPECT_EQ(RouteVersionRouter::stripVersionPrefix("/api/v1/graphql"),
              "/api/v1/graphql");
}

// ---------------------------------------------------------------------------
// normalize()
// ---------------------------------------------------------------------------

TEST(RouteVersionRouter, Normalize_V1Path) {
    RouteVersionRouter vr;
    auto n = vr.normalize("/v1/documents/abc");
    EXPECT_EQ(n.version, 1);
    EXPECT_EQ(n.path, "/documents/abc");
}

TEST(RouteVersionRouter, Normalize_V2Path) {
    RouteVersionRouter vr;
    auto n = vr.normalize("/v2/query/stream");
    EXPECT_EQ(n.version, 2);
    EXPECT_EQ(n.path, "/query/stream");
}

TEST(RouteVersionRouter, Normalize_UnversionedPath) {
    RouteVersionRouter vr;
    auto n = vr.normalize("/documents/abc");
    EXPECT_EQ(n.version, 0);
    EXPECT_EQ(n.path, "/documents/abc");
}

TEST(RouteVersionRouter, Normalize_ApiPrefixedPath) {
    RouteVersionRouter vr;
    auto n = vr.normalize("/api/v1/graphql");
    EXPECT_EQ(n.version, 1);
    // /api/v1/graphql is kept intact (not a top-level /vN/ path)
    EXPECT_EQ(n.path, "/api/v1/graphql");
}

// ---------------------------------------------------------------------------
// getRedirectTarget()
// ---------------------------------------------------------------------------

TEST(RouteVersionRouter, Redirect_UnversionedPath) {
    RouteVersionRouter vr;
    auto t = vr.getRedirectTarget("/documents");
    ASSERT_TRUE(t.has_value());
    EXPECT_EQ(*t, "/v1/documents");
}

TEST(RouteVersionRouter, Redirect_UnversionedPathWithQuery) {
    RouteVersionRouter vr;
    // /query/* is an exempt (unversioned) path handled directly by the server
    // without redirection; verify it is not redirected.
    auto t = vr.getRedirectTarget("/query/aql?limit=10");
    EXPECT_FALSE(t.has_value());
}

TEST(RouteVersionRouter, Redirect_UnversionedDocumentPath) {
    RouteVersionRouter vr;
    // /documents is a truly unversioned path that the router SHOULD redirect
    auto t = vr.getRedirectTarget("/some-unversioned-path");
    ASSERT_TRUE(t.has_value());
    EXPECT_EQ(*t, "/v1/some-unversioned-path");
}

TEST(RouteVersionRouter, NoRedirect_AlreadyVersionedV1) {
    RouteVersionRouter vr;
    EXPECT_FALSE(vr.getRedirectTarget("/v1/documents").has_value());
}

TEST(RouteVersionRouter, NoRedirect_AlreadyVersionedV2) {
    RouteVersionRouter vr;
    EXPECT_FALSE(vr.getRedirectTarget("/v2/query/stream").has_value());
}

TEST(RouteVersionRouter, NoRedirect_HealthEndpoint) {
    RouteVersionRouter vr;
    EXPECT_FALSE(vr.getRedirectTarget("/health").has_value());
}

TEST(RouteVersionRouter, NoRedirect_MetricsEndpoint) {
    RouteVersionRouter vr;
    EXPECT_FALSE(vr.getRedirectTarget("/metrics").has_value());
}

TEST(RouteVersionRouter, NoRedirect_GraphQLWebSocket) {
    RouteVersionRouter vr;
    EXPECT_FALSE(vr.getRedirectTarget("/graphql").has_value());
}

TEST(RouteVersionRouter, NoRedirect_RootPath) {
    RouteVersionRouter vr;
    EXPECT_FALSE(vr.getRedirectTarget("/").has_value());
}

// ============================================================================
// APIVersionRange Tests
// ============================================================================

TEST(APIVersionRangeTest, ParseValidRange) {
    auto range = APIVersionRange::parse("1.0-2.0");
    ASSERT_TRUE(range.has_value());
    EXPECT_EQ(range->min_version, (APIVersion{1, 0, 0}));
    EXPECT_EQ(range->max_version, (APIVersion{2, 0, 0}));
}

TEST(APIVersionRangeTest, ParseValidRangeSemver) {
    auto range = APIVersionRange::parse("1.2.0-1.4.1");
    ASSERT_TRUE(range.has_value());
    EXPECT_EQ(range->min_version, (APIVersion{1, 2, 0}));
    EXPECT_EQ(range->max_version, (APIVersion{1, 4, 1}));
}

TEST(APIVersionRangeTest, ParseInvalidRange_NoDash) {
    EXPECT_FALSE(APIVersionRange::parse("1.0").has_value());
}

TEST(APIVersionRangeTest, ParseInvalidRange_BadVersion) {
    EXPECT_FALSE(APIVersionRange::parse("invalid-2.0").has_value());
}

TEST(APIVersionRangeTest, ParseInvalidRange_MinGreaterThanMax) {
    EXPECT_FALSE(APIVersionRange::parse("2.0-1.0").has_value());
}

TEST(APIVersionRangeTest, ParseRangeWithSpacesAroundDash) {
    // HTTP clients sometimes include spaces: "1.0 - 2.0"
    auto range = APIVersionRange::parse("1.0 - 2.0");
    ASSERT_TRUE(range.has_value());
    EXPECT_EQ(range->min_version, (APIVersion{1, 0, 0}));
    EXPECT_EQ(range->max_version, (APIVersion{2, 0, 0}));
}

TEST(APIVersionRangeTest, ParseRangeWithLeadingTrailingWhitespace) {
    auto range = APIVersionRange::parse("  1.2  -  1.4  ");
    ASSERT_TRUE(range.has_value());
    EXPECT_EQ(range->min_version, (APIVersion{1, 2, 0}));
    EXPECT_EQ(range->max_version, (APIVersion{1, 4, 0}));
}

TEST(APIVersionRangeTest, Contains) {
    APIVersionRange range{APIVersion{1, 0, 0}, APIVersion{2, 0, 0}};
    EXPECT_TRUE(range.contains(APIVersion{1, 0, 0}));
    EXPECT_TRUE(range.contains(APIVersion{1, 4, 1}));
    EXPECT_TRUE(range.contains(APIVersion{2, 0, 0}));
    EXPECT_FALSE(range.contains(APIVersion{0, 9, 0}));
    EXPECT_FALSE(range.contains(APIVersion{2, 0, 1}));
}

// ============================================================================
// APIVersionManager::resolveVersionRange Tests
// ============================================================================

class APIVersionRangeResolutionTest : public ::testing::Test {
protected:
    APIVersionManager manager;
};

TEST_F(APIVersionRangeResolutionTest, ResolvesHighestVersionInRange) {
    // Range [1.0, 1.3] should resolve to v1.3.0 (highest supported in range)
    APIVersionRange range{APIVersion{1, 0, 0}, APIVersion{1, 3, 0}};
    auto resolved = manager.resolveVersionRange(range);
    EXPECT_EQ(resolved, (APIVersion{1, 3, 0}));
}

TEST_F(APIVersionRangeResolutionTest, ResolvesToCurrentWhenRangeCoversCurrent) {
    // A wide range should resolve to the current version
    APIVersionRange range{APIVersion{1, 0, 0}, APIVersion{9, 9, 9}};
    auto resolved = manager.resolveVersionRange(range);
    EXPECT_EQ(resolved, manager.getCurrentVersion());
}

TEST_F(APIVersionRangeResolutionTest, FallsBackToCurrentWhenNoMatch) {
    // Range that covers no supported version
    APIVersionRange range{APIVersion{5, 0, 0}, APIVersion{6, 0, 0}};
    auto resolved = manager.resolveVersionRange(range);
    EXPECT_EQ(resolved, manager.getCurrentVersion());
}

TEST_F(APIVersionRangeResolutionTest, ExactVersionRange) {
    // Single-version range
    APIVersionRange range{APIVersion{1, 2, 0}, APIVersion{1, 2, 0}};
    auto resolved = manager.resolveVersionRange(range);
    EXPECT_EQ(resolved, (APIVersion{1, 2, 0}));
}

// ============================================================================
// BreakingChange Tests
// ============================================================================

class BreakingChangeTest : public ::testing::Test {
protected:
    APIVersionManager manager;
};

TEST_F(BreakingChangeTest, NoBreakingChangeRegistered) {
    auto result = manager.isBreakingChange(APIVersion{1, 0, 0}, APIVersion{1, 4, 1});
    EXPECT_FALSE(result.has_value());
}

TEST_F(BreakingChangeTest, GlobalBreakingChangeDetected) {
    BreakingChangeInfo bc;
    bc.introduced_in = APIVersion{2, 0, 0};
    bc.description = "Response format changed";
    manager.registerBreakingChange(bc);

    auto result = manager.isBreakingChange(APIVersion{1, 4, 1}, APIVersion{2, 0, 0});
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->description, "Response format changed");
}

TEST_F(BreakingChangeTest, EndpointBreakingChangeDetected) {
    BreakingChangeInfo bc;
    bc.introduced_in = APIVersion{2, 0, 0};
    bc.endpoint = "/entities";
    bc.description = "Entity response restructured";
    manager.registerBreakingChange(bc);

    auto result = manager.isBreakingChange(APIVersion{1, 4, 1}, APIVersion{2, 0, 0}, "/entities");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->description, "Entity response restructured");
}

TEST_F(BreakingChangeTest, EndpointBreakingChangeNotAffectsOtherEndpoint) {
    BreakingChangeInfo bc;
    bc.introduced_in = APIVersion{2, 0, 0};
    bc.endpoint = "/entities";
    bc.description = "Entity response restructured";
    manager.registerBreakingChange(bc);

    // A different endpoint should not be affected
    auto result = manager.isBreakingChange(APIVersion{1, 4, 1}, APIVersion{2, 0, 0}, "/documents");
    EXPECT_FALSE(result.has_value());
}

TEST_F(BreakingChangeTest, BreakingChangeNotDetectedWhenFromVersionExceedsIt) {
    BreakingChangeInfo bc;
    bc.introduced_in = APIVersion{1, 5, 0};
    bc.description = "Old breaking change";
    manager.registerBreakingChange(bc);

    // Client already at v2.0.0 — the break at v1.5.0 is already past
    auto result = manager.isBreakingChange(APIVersion{2, 0, 0}, APIVersion{2, 1, 0});
    EXPECT_FALSE(result.has_value());
}

TEST_F(BreakingChangeTest, BreakingChangeNotDetectedWhenStrictlyBeforeRange) {
    BreakingChangeInfo bc;
    bc.introduced_in = APIVersion{1, 5, 0};
    bc.description = "Minor breaking change";
    manager.registerBreakingChange(bc);

    // Upgrading from v1.0 to v1.4 does not cross v1.5
    auto result = manager.isBreakingChange(APIVersion{1, 0, 0}, APIVersion{1, 4, 0});
    EXPECT_FALSE(result.has_value());
}

TEST_F(BreakingChangeTest, BreakingChangeMigrationGuideUrl) {
    BreakingChangeInfo bc;
    bc.introduced_in = APIVersion{2, 0, 0};
    bc.description = "New format";
    bc.migration_guide_url = "https://docs.themisdb.com/migration/v1-to-v2";
    manager.registerBreakingChange(bc);

    auto result = manager.isBreakingChange(APIVersion{1, 4, 1}, APIVersion{2, 0, 0});
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->migration_guide_url, "https://docs.themisdb.com/migration/v1-to-v2");
}
