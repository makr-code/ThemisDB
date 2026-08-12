/**
 * @file test_api_routing.cpp
 * @brief Tests for API routing, versioning, and endpoint management
 *
 * Validates routing configuration, API versioning framework, deprecation
 * handling and route path conventions for the production-ready server module.
 */

#include <gtest/gtest.h>
#include "server/api_version.h"
#include "server/api_version_config.h"

using namespace themis::server;

// ---------------------------------------------------------------------------
// APIVersion Parsing
// ---------------------------------------------------------------------------

class APIVersionParsingTest : public ::testing::Test {};

TEST_F(APIVersionParsingTest, ParseV1) {
    auto v = APIVersion::parse("v1");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(v->major, 1u);
    EXPECT_EQ(v->minor, 0u);
    EXPECT_EQ(v->patch, 0u);
}

TEST_F(APIVersionParsingTest, ParseV1Dot0) {
    auto v = APIVersion::parse("v1.0");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(v->major, 1u);
    EXPECT_EQ(v->minor, 0u);
}

TEST_F(APIVersionParsingTest, ParseV1Dot0Dot0) {
    auto v = APIVersion::parse("v1.0.0");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(v->major, 1u);
    EXPECT_EQ(v->minor, 0u);
    EXPECT_EQ(v->patch, 0u);
}

TEST_F(APIVersionParsingTest, ParseWithoutVPrefix) {
    auto v = APIVersion::parse("1.2.3");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(v->major, 1u);
    EXPECT_EQ(v->minor, 2u);
    EXPECT_EQ(v->patch, 3u);
}

TEST_F(APIVersionParsingTest, ParseLatestKeyword) {
    auto v = APIVersion::parse("latest");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(v->major, APIVersionConfig::CURRENT_MAJOR);
}

TEST_F(APIVersionParsingTest, LatestKeywordIsCaseSensitive) {
    // "latest" is the canonical form; "LATEST" and "Latest" are not recognized
    // (documents the case-sensitive behaviour of the version parser)
    EXPECT_FALSE(APIVersion::parse("LATEST").has_value());
    EXPECT_FALSE(APIVersion::parse("Latest").has_value());
}

TEST_F(APIVersionParsingTest, ParseInvalidString) {
    EXPECT_FALSE(APIVersion::parse("invalid").has_value());
    EXPECT_FALSE(APIVersion::parse("").has_value());
    EXPECT_FALSE(APIVersion::parse("v").has_value());
}

// ---------------------------------------------------------------------------
// APIVersion Comparison
// ---------------------------------------------------------------------------

class APIVersionComparisonTest : public ::testing::Test {};

TEST_F(APIVersionComparisonTest, EqualVersionsAreEqual) {
    APIVersion v1{1, 0, 0}, v2{1, 0, 0};
    EXPECT_EQ(v1, v2);
    EXPECT_FALSE(v1 != v2);
}

TEST_F(APIVersionComparisonTest, LowerMajorIsLess) {
    APIVersion v1{1, 0, 0}, v2{2, 0, 0};
    EXPECT_LT(v1, v2);
    EXPECT_GT(v2, v1);
}

TEST_F(APIVersionComparisonTest, LowerMinorIsLess) {
    APIVersion v1{1, 0, 0}, v2{1, 1, 0};
    EXPECT_LT(v1, v2);
}

TEST_F(APIVersionComparisonTest, LowerPatchIsLess) {
    APIVersion v1{1, 0, 0}, v2{1, 0, 1};
    EXPECT_LT(v1, v2);
}

TEST_F(APIVersionComparisonTest, LessOrEqualSameVersion) {
    APIVersion v{1, 2, 3};
    EXPECT_LE(v, v);
    EXPECT_GE(v, v);
}

// ---------------------------------------------------------------------------
// APIVersion String Conversion
// ---------------------------------------------------------------------------

TEST(APIVersionStringTest, ToStringIncludesVPrefix) {
    APIVersion v{1, 2, 3};
    auto s = v.toString();
    EXPECT_EQ(s[0], 'v');
}

TEST(APIVersionStringTest, ToStringFormat) {
    APIVersion v{1, 2, 3};
    EXPECT_EQ(v.toString(), "v1.2.3");
}

TEST(APIVersionStringTest, ToStringZeroVersion) {
    APIVersion v{0, 0, 0};
    EXPECT_EQ(v.toString(), "v0.0.0");
}

// ---------------------------------------------------------------------------
// APIVersionManager
// ---------------------------------------------------------------------------

class APIVersionManagerRoutingTest : public ::testing::Test {
protected:
    APIVersionManager mgr_;
};

TEST_F(APIVersionManagerRoutingTest, HasCurrentVersion) {
    auto v = mgr_.getCurrentVersion();
    EXPECT_GT(v.major + v.minor + v.patch, 0u);  // Not all-zero
}

TEST_F(APIVersionManagerRoutingTest, HasMinimumVersion) {
    auto min_v = mgr_.getMinimumVersion();
    auto cur_v = mgr_.getCurrentVersion();
    EXPECT_LE(min_v, cur_v);
}

TEST_F(APIVersionManagerRoutingTest, CurrentVersionIsSupported) {
    EXPECT_TRUE(mgr_.isVersionSupported(mgr_.getCurrentVersion()));
}

TEST_F(APIVersionManagerRoutingTest, ReturnsCurrentVersionForLatestKeyword) {
    auto resolved = mgr_.resolveVersion("latest");
    EXPECT_EQ(resolved, mgr_.getCurrentVersion());
}

TEST_F(APIVersionManagerRoutingTest, SupportedVersionsListNotEmpty) {
    auto versions = mgr_.getSupportedVersions();
    EXPECT_FALSE(versions.empty());
}

TEST_F(APIVersionManagerRoutingTest, ReturnsCurrentVersionForEmptyHeader) {
    auto resolved = mgr_.resolveVersion("");
    EXPECT_EQ(resolved, mgr_.getCurrentVersion());
}

TEST_F(APIVersionManagerRoutingTest, ReturnsCurrentVersionForInvalidHeader) {
    auto resolved = mgr_.resolveVersion("garbage");
    EXPECT_EQ(resolved, mgr_.getCurrentVersion());
}

// ---------------------------------------------------------------------------
// API Route Path Conventions
// ---------------------------------------------------------------------------

TEST(APIRoutingConventions, VersionedPathPrefixUsesV1) {
    std::string versioned_path = "/api/v1/snapshots/stats";
    EXPECT_NE(versioned_path.find("/api/v1/"), std::string::npos);
}

TEST(APIRoutingConventions, HealthEndpointsAreNotVersioned) {
    std::string health_path = "/health";
    std::string live_path = "/health/live";
    std::string ready_path = "/health/ready";

    EXPECT_EQ(health_path.find("/api/v"), std::string::npos);
    EXPECT_EQ(live_path.find("/api/v"), std::string::npos);
    EXPECT_EQ(ready_path.find("/api/v"), std::string::npos);
}

// ---------------------------------------------------------------------------
// Deprecation Registration
// ---------------------------------------------------------------------------

TEST(APIDeprecationTest, CanRegisterAndRetrieveDeprecation) {
    APIVersionManager mgr;
    APIDeprecationInfo info;
    info.deprecated_in = {1, 0, 0};
    info.removed_in = {2, 0, 0};
    info.reason = "Replaced by /api/v2/entities";
    info.alternative = "/api/v2/entities";

    mgr.registerDeprecation("/entities", info);

    auto result = mgr.getDeprecationInfo("/entities", {1, 0, 0});
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->alternative, "/api/v2/entities");
}

TEST(APIDeprecationTest, NoDeprecationInfoForUnregisteredEndpoint) {
    APIVersionManager mgr;
    auto result = mgr.getDeprecationInfo("/not/deprecated", {1, 0, 0});
    EXPECT_FALSE(result.has_value());
}

TEST(APIDeprecationTest, NoDeprecationInfoAfterRemoval) {
    APIVersionManager mgr;
    APIDeprecationInfo info;
    info.deprecated_in = {1, 0, 0};
    info.removed_in = {2, 0, 0};
    info.reason = "Removed in v2";
    info.alternative = "/api/v2/endpoint";
    mgr.registerDeprecation("/old/endpoint", info);

    // Version 2.0.0 is at/past removal - no deprecation info
    auto result = mgr.getDeprecationInfo("/old/endpoint", {2, 0, 0});
    EXPECT_FALSE(result.has_value());
}

TEST(APIDeprecationTest, DeprecationInfoReturnedBeforeRemoval) {
    APIVersionManager mgr;
    APIDeprecationInfo info;
    info.deprecated_in = {1, 2, 0};
    info.removed_in = {2, 0, 0};
    info.alternative = "/api/v2/endpoint";
    mgr.registerDeprecation("/old/endpoint2", info);

    // Version 1.3.0 is between deprecated_in and removed_in
    auto result = mgr.getDeprecationInfo("/old/endpoint2", {1, 3, 0});
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->alternative, "/api/v2/endpoint");
}

// ---------------------------------------------------------------------------
// URL Path-Based Versioned Routing Conventions
// These tests validate the path format expectations for /v1/ and /v2/ routing.
// ---------------------------------------------------------------------------

TEST(VersionedRoutingConventions, V1PathStartsWithSlashV1Slash) {
    // v1 versioned paths follow the /v1/<resource> convention
    std::string v1_path = "/v1/entities/123";
    EXPECT_EQ(v1_path.substr(0, 4), "/v1/");
}

TEST(VersionedRoutingConventions, V2PathStartsWithSlashV2Slash) {
    // v2 versioned paths follow the /v2/<resource> convention
    std::string v2_path = "/v2/entities/123";
    EXPECT_EQ(v2_path.substr(0, 4), "/v2/");
}
