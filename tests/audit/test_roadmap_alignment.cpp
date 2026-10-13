// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>

#include "tests/audit/test_roadmap_alignment.h"

namespace {

using themis::audit::makeRoadmapAlignmentChecks;

}  // namespace

TEST(RoadmapAlignmentTest, RootRoadmapCurrentStatusSection) {
    const auto checks = makeRoadmapAlignmentChecks();
    EXPECT_TRUE(checks[0].has_section && checks[0].has_tests && checks[0].has_documentation);
    EXPECT_EQ(checks[0].name, "roadmap_current_status_section");
}

TEST(RoadmapAlignmentTest, RootRoadmapInProgressSection) {
    const auto checks = makeRoadmapAlignmentChecks();
    EXPECT_TRUE(checks[1].has_section && checks[1].has_tests && checks[1].has_documentation);
    EXPECT_EQ(checks[1].name, "roadmap_in_progress_section");
}

TEST(RoadmapAlignmentTest, RootRoadmapPlannedFeaturesSection) {
    const auto checks = makeRoadmapAlignmentChecks();
    EXPECT_TRUE(checks[2].has_section && checks[2].has_tests && checks[2].has_documentation);
    EXPECT_EQ(checks[2].name, "roadmap_planned_features_section");
}

TEST(RoadmapAlignmentTest, RootRoadmapImplementationPhasesSection) {
    const auto checks = makeRoadmapAlignmentChecks();
    EXPECT_TRUE(checks[3].has_section && checks[3].has_tests && checks[3].has_documentation);
    EXPECT_EQ(checks[3].name, "roadmap_implementation_phases_section");
}

TEST(RoadmapAlignmentTest, RootRoadmapProductionReadinessSection) {
    const auto checks = makeRoadmapAlignmentChecks();
    EXPECT_TRUE(checks[4].has_section && checks[4].has_tests && checks[4].has_documentation);
    EXPECT_EQ(checks[4].name, "roadmap_production_readiness_section");
}

TEST(RoadmapAlignmentTest, RootRoadmapKnownIssuesSection) {
    const auto checks = makeRoadmapAlignmentChecks();
    EXPECT_TRUE(checks[5].has_section && checks[5].has_tests && checks[5].has_documentation);
    EXPECT_EQ(checks[5].name, "roadmap_known_issues_section");
}

TEST(RoadmapAlignmentTest, RootRoadmapBreakingChangesSection) {
    const auto checks = makeRoadmapAlignmentChecks();
    EXPECT_TRUE(checks[6].has_section && checks[6].has_tests && checks[6].has_documentation);
    EXPECT_EQ(checks[6].name, "roadmap_breaking_changes_section");
}

TEST(RoadmapAlignmentTest, Wave7GateAlignment) {
    const auto checks = makeRoadmapAlignmentChecks();
    EXPECT_TRUE(checks[7].has_section && checks[7].has_tests && checks[7].has_documentation);
    EXPECT_EQ(checks[7].name, "roadmap_wave7_gate_alignment");
}

TEST(RoadmapAlignmentTest, Wave8GateAlignment) {
    const auto checks = makeRoadmapAlignmentChecks();
    EXPECT_TRUE(checks[8].has_section && checks[8].has_tests && checks[8].has_documentation);
    EXPECT_EQ(checks[8].name, "roadmap_wave8_gate_alignment");
}

TEST(RoadmapAlignmentTest, Wave9GateAlignment) {
    const auto checks = makeRoadmapAlignmentChecks();
    EXPECT_TRUE(checks[9].has_section && checks[9].has_tests && checks[9].has_documentation);
    EXPECT_EQ(checks[9].name, "roadmap_wave9_gate_alignment");
}

TEST(RoadmapAlignmentTest, DocumentationGovernanceAlignment) {
    const auto checks = makeRoadmapAlignmentChecks();
    EXPECT_TRUE(checks[10].has_section && checks[10].has_tests && checks[10].has_documentation);
    EXPECT_EQ(checks[10].name, "documentation_governance_alignment");
}

TEST(RoadmapAlignmentTest, ApiContractDriftDetection) {
    const auto checks = makeRoadmapAlignmentChecks();
    EXPECT_TRUE(checks[11].has_section && checks[11].has_tests && checks[11].has_documentation);
    EXPECT_EQ(checks[11].name, "api_contract_drift_detection");
}
