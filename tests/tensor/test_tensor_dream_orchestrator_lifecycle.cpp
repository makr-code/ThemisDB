#include <gtest/gtest.h>

#include "tensor/dream_orchestrator.h"

#include <string>
#include <vector>

using themis::tensor::DreamOrchestrator;
using themis::tensor::DreamRunAuditReasonClass;
using themis::tensor::DreamRunDescriptor;
using themis::tensor::DreamRunState;

TEST(DreamOrchestratorLifecycle, DOR01_StartDreamRunRequiresDreamResearchMode) {
    DreamOrchestrator orchestrator;
    std::string error;
    DreamRunAuditReasonClass reason_class = DreamRunAuditReasonClass::none;

    const auto descriptor = orchestrator.StartDreamRun(
        "production",
        "policy-v1",
        "dataset://alpha/snapshot-42",
        {7, 11, 13},
        &error,
        &reason_class);

    EXPECT_FALSE(descriptor.has_value());
    EXPECT_EQ(reason_class, DreamRunAuditReasonClass::invalid_mode);
    EXPECT_NE(error.find("dream_research"), std::string::npos);
    EXPECT_EQ(orchestrator.auditLog().empty(), false);
}

TEST(DreamOrchestratorLifecycle, DOR02_StartDreamRunPersistsDescriptorByRunId) {
    DreamOrchestrator orchestrator;
    const auto descriptor = orchestrator.StartDreamRun(
        "dream_research",
        "policy-v1",
        "dataset://alpha/snapshot-42",
        {7, 11, 13});

    ASSERT_TRUE(descriptor.has_value());
    EXPECT_EQ(descriptor->mode, "dream_research");
    EXPECT_EQ(descriptor->state, DreamRunState::R0);
    EXPECT_TRUE(descriptor->synthetic);
    EXPECT_TRUE(descriptor->dream_mode);

    const auto stored = orchestrator.getRunById(descriptor->run_id);
    ASSERT_TRUE(stored.has_value());
    EXPECT_EQ(stored->run_id, descriptor->run_id);
    EXPECT_EQ(stored->dataset_ref, descriptor->dataset_ref);
    EXPECT_EQ(stored->seed_set, descriptor->seed_set);
}

TEST(DreamOrchestratorLifecycle, DOR03_AllowedTransitionMatrixRestrictsR0ToR3) {
    DreamOrchestrator orchestrator;
    const auto descriptor = orchestrator.StartDreamRun(
        "dream_research",
        "policy-v1",
        "dataset://alpha/snapshot-42",
        {1, 2, 3});
    ASSERT_TRUE(descriptor.has_value());

    std::string detail;
    DreamRunAuditReasonClass reason_class = DreamRunAuditReasonClass::none;
    EXPECT_TRUE(orchestrator.transitionRun(descriptor->run_id, DreamRunState::R1, &detail, &reason_class));
    EXPECT_EQ(reason_class, DreamRunAuditReasonClass::none);

    EXPECT_TRUE(orchestrator.transitionRun(descriptor->run_id, DreamRunState::R2, &detail, &reason_class));
    EXPECT_TRUE(orchestrator.transitionRun(descriptor->run_id, DreamRunState::R3, &detail, &reason_class));

    EXPECT_FALSE(orchestrator.transitionRun(descriptor->run_id, DreamRunState::R0, &detail, &reason_class));
    EXPECT_EQ(reason_class, DreamRunAuditReasonClass::invalid_transition);
    EXPECT_NE(detail.find("Invalid transition"), std::string::npos);
}

TEST(DreamOrchestratorLifecycle, DOR04_FailedStartIsLoggedWithReasonClass) {
    DreamOrchestrator orchestrator;
    std::string error;
    DreamRunAuditReasonClass reason_class = DreamRunAuditReasonClass::none;

    (void)orchestrator.StartDreamRun(
        "analysis",
        "policy-v1",
        "dataset://alpha/snapshot-42",
        {9, 9},
        &error,
        &reason_class);

    auto log = orchestrator.auditLog();
    ASSERT_FALSE(log.empty());
    EXPECT_EQ(log.back().reason_class, DreamRunAuditReasonClass::invalid_mode);
    EXPECT_EQ(log.back().phase, "start");
    EXPECT_NE(log.back().reason.find("dream_research"), std::string::npos);
}

TEST(DreamOrchestratorLifecycle, DOR05_AbortAndFailureAreAudited) {
    DreamOrchestrator orchestrator;
    const auto descriptor = orchestrator.StartDreamRun(
        "dream_research",
        "policy-v1",
        "dataset://alpha/snapshot-42",
        {1, 2, 3});
    ASSERT_TRUE(descriptor.has_value());

    EXPECT_TRUE(orchestrator.abortRun(descriptor->run_id, "operator abort"));
    EXPECT_TRUE(orchestrator.failRun(descriptor->run_id, "policy gate failed",
                                   DreamRunAuditReasonClass::generic_failure));

    const auto log = orchestrator.auditLog();
    EXPECT_GE(log.size(), 3u);
    EXPECT_EQ(log.back().reason_class, DreamRunAuditReasonClass::generic_failure);
}
