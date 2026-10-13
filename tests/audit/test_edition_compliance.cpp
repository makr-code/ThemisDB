// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>

#include "tests/audit/test_edition_compliance.h"

namespace {

using themis::audit::makeEditionProfiles;

}  // namespace

TEST(EditionComplianceTest, CommunityEditionOpenSourceOnly) {
    const auto profiles = makeEditionProfiles();
    EXPECT_TRUE(profiles[0].public_api_restricted && profiles[0].feature_gating && profiles[0].compliance_controls);
    EXPECT_EQ(profiles[0].edition, "community");
}

TEST(EditionComplianceTest, CommunityEditionPublicApiBoundary) {
    const auto profiles = makeEditionProfiles();
    EXPECT_TRUE(profiles[1].public_api_restricted && profiles[1].feature_gating && profiles[1].compliance_controls);
    EXPECT_EQ(profiles[1].edition, "community_public_api_boundary");
}

TEST(EditionComplianceTest, CommunityEditionLicenseCompliance) {
    const auto profiles = makeEditionProfiles();
    EXPECT_TRUE(profiles[2].public_api_restricted && profiles[2].feature_gating && profiles[2].compliance_controls);
    EXPECT_EQ(profiles[2].edition, "community_license_compliance");
}

TEST(EditionComplianceTest, CommunityEditionFlagGate) {
    const auto profiles = makeEditionProfiles();
    EXPECT_TRUE(profiles[3].public_api_restricted && profiles[3].feature_gating && profiles[3].compliance_controls);
    EXPECT_EQ(profiles[3].edition, "community_flag_gate");
}

TEST(EditionComplianceTest, MinimalEditionSlimDependencySet) {
    const auto profiles = makeEditionProfiles();
    EXPECT_TRUE(profiles[4].public_api_restricted && profiles[4].feature_gating && profiles[4].compliance_controls);
    EXPECT_EQ(profiles[4].edition, "minimal");
}

TEST(EditionComplianceTest, MinimalEditionCoreApiAvailable) {
    const auto profiles = makeEditionProfiles();
    EXPECT_TRUE(profiles[5].public_api_restricted && profiles[5].feature_gating && profiles[5].compliance_controls);
    EXPECT_EQ(profiles[5].edition, "minimal_core_api_set");
}

TEST(EditionComplianceTest, MinimalEditionDependencyBudget) {
    const auto profiles = makeEditionProfiles();
    EXPECT_TRUE(profiles[6].public_api_restricted && profiles[6].feature_gating && profiles[6].compliance_controls);
    EXPECT_EQ(profiles[6].edition, "minimal_dependency_budget");
}

TEST(EditionComplianceTest, EnterpriseEditionAdvancedFeaturesEnabled) {
    const auto profiles = makeEditionProfiles();
    EXPECT_TRUE(profiles[7].public_api_restricted && profiles[7].feature_gating && profiles[7].compliance_controls);
    EXPECT_EQ(profiles[7].edition, "enterprise");
}

TEST(EditionComplianceTest, EnterpriseEditionAdvancedFeatureGate) {
    const auto profiles = makeEditionProfiles();
    EXPECT_TRUE(profiles[8].public_api_restricted && profiles[8].feature_gating && profiles[8].compliance_controls);
    EXPECT_EQ(profiles[8].edition, "enterprise_advanced_features");
}

TEST(EditionComplianceTest, EnterpriseEditionAuditSuite) {
    const auto profiles = makeEditionProfiles();
    EXPECT_TRUE(profiles[9].public_api_restricted && profiles[9].feature_gating && profiles[9].compliance_controls);
    EXPECT_EQ(profiles[9].edition, "enterprise_audit_suite");
}

TEST(EditionComplianceTest, EnterpriseEditionPerformanceTuning) {
    const auto profiles = makeEditionProfiles();
    EXPECT_TRUE(profiles[10].public_api_restricted && profiles[10].feature_gating && profiles[10].compliance_controls);
    EXPECT_EQ(profiles[10].edition, "enterprise_perf_tuning");
}

TEST(EditionComplianceTest, MilitaryEditionStrictSecurityPolicies) {
    const auto profiles = makeEditionProfiles();
    EXPECT_TRUE(profiles[11].public_api_restricted && profiles[11].feature_gating && profiles[11].compliance_controls);
    EXPECT_EQ(profiles[11].edition, "military");
}

TEST(EditionComplianceTest, MilitaryEditionSecurityPolicyGate) {
    const auto profiles = makeEditionProfiles();
    EXPECT_TRUE(profiles[12].public_api_restricted && profiles[12].feature_gating && profiles[12].compliance_controls);
    EXPECT_EQ(profiles[12].edition, "military_security_policy");
}

TEST(EditionComplianceTest, MilitaryEditionCertificationReadiness) {
    const auto profiles = makeEditionProfiles();
    EXPECT_TRUE(profiles[13].public_api_restricted && profiles[13].feature_gating && profiles[13].compliance_controls);
    EXPECT_EQ(profiles[13].edition, "military_certification_readiness");
}

TEST(EditionComplianceTest, MilitaryEditionHardeningFlagsValid) {
    const auto profiles = makeEditionProfiles();
    EXPECT_TRUE(profiles[14].public_api_restricted && profiles[14].feature_gating && profiles[14].compliance_controls);
    EXPECT_EQ(profiles[14].edition, "military_hardening_flags");
}
