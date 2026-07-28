/**
 * @file test_federated_distillation_coordinator_focused.cpp
 * @brief Focused unit tests for federated distillation coordination.
 *
 * Tests the core distributed knowledge distillation paths against the actual
 * production API surface:
 * - SoftLabel creation and JSON round-trip
 * - FederatedDistillationCoordinator lifecycle and round tracking
 * - DP budget verification and privacy budget remaining
 * - Policy gate blocking broadcast
 * - Student registration and broadcast delivery
 *
 * Target: Production readiness validation for distillation layer.
 * Q3 2026 Hardening: policy-gated flows, DP enforcement, deterministic behavior.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <nlohmann/json.hpp>
#include <vector>
#include <memory>

#include "distributed_knowledge/federated_distillation_coordinator.h"

using namespace themis::distributed_knowledge;
using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Test Fixtures
// ─────────────────────────────────────────────────────────────────────────────

class FederatedDistillationCoordinatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        cfg_.dp_epsilon          = 0.5;
        cfg_.dp_delta            = 1e-5;
        cfg_.dp_sensitivity      = 0.1;
        cfg_.temperature         = 4.0;
        cfg_.alpha               = 0.5;
        cfg_.max_rounds          = 0;     // unlimited
        cfg_.min_utility_threshold = 0.9;
        cfg_.require_dp          = true;
    }

    DistillationConfig cfg_;

    /// Build a minimal set of SoftLabels for testing.
    static std::vector<SoftLabel> makeLabels(size_t n, const std::string& teacher_id = "t1") {
        std::vector<SoftLabel> labels;
        labels.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            SoftLabel sl;
            sl.query_id      = "q" + std::to_string(i);
            sl.probabilities = {0.6, 0.4};
            sl.temperature   = 4.0;
            sl.teacher_id    = teacher_id;
            labels.push_back(sl);
        }
        return labels;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// SoftLabel Tests (FDC-01..FDC-03)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test FDC-01: SoftLabel creation with required fields.
 *
 * Verifies that a SoftLabel can be constructed and all documented fields
 * are accessible.
 */
TEST_F(FederatedDistillationCoordinatorTest, CreateSoftLabel) {
    SoftLabel sl = makeLabels(1)[0];

    EXPECT_EQ(sl.query_id,               "q0");
    EXPECT_EQ(sl.probabilities.size(),   2u);
    EXPECT_DOUBLE_EQ(sl.temperature,     4.0);
    EXPECT_EQ(sl.teacher_id,             "t1");
}

/**
 * @test FDC-02: DistillationConfig validation — valid config.
 *
 * Verifies that a properly constructed DistillationConfig reports isValid().
 */
TEST_F(FederatedDistillationCoordinatorTest, ValidConfigReportsValid) {
    EXPECT_TRUE(cfg_.isValid());
}

/**
 * @test FDC-03: SoftLabel JSON serialization round-trip.
 *
 * Verifies that SoftLabel::toJson() and SoftLabel::fromJson() preserve all fields.
 */
TEST_F(FederatedDistillationCoordinatorTest, SoftLabelJsonRoundTrip) {
    SoftLabel original;
    original.query_id      = "q-rt";
    original.probabilities = {0.7, 0.2, 0.1};
    original.temperature   = 3.5;
    original.teacher_id    = "teacher-42";

    json j = original.toJson();
    SoftLabel restored = SoftLabel::fromJson(j);

    EXPECT_EQ(restored.query_id,              original.query_id);
    EXPECT_EQ(restored.probabilities.size(),  original.probabilities.size());
    EXPECT_DOUBLE_EQ(restored.temperature,    original.temperature);
    EXPECT_EQ(restored.teacher_id,            original.teacher_id);
}

// ─────────────────────────────────────────────────────────────────────────────
// Coordinator Lifecycle Tests (FDC-04..FDC-06)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test FDC-04: Coordinator initial state — round 0, no submissions.
 *
 * Verifies that a freshly constructed coordinator has currentRound()==0,
 * submittedCount()==0, and no last round.
 */
TEST_F(FederatedDistillationCoordinatorTest, CoordinatorInitialState) {
    FederatedDistillationCoordinator coordinator(cfg_);

    EXPECT_EQ(coordinator.currentRound(),   0u);
    EXPECT_EQ(coordinator.submittedCount(), 0u);
    EXPECT_FALSE(coordinator.lastRound().has_value());
}

/**
 * @test FDC-05: submitSoftLabels() + broadcastToStudents() advances round.
 *
 * Verifies that after submitting labels and broadcasting, currentRound()==1
 * and lastRound() has a value.
 */
TEST_F(FederatedDistillationCoordinatorTest, SubmitAndBroadcastAdvancesRound) {
    FederatedDistillationCoordinator coordinator(cfg_);

    coordinator.submitSoftLabels("teacher-1", makeLabels(3));
    ASSERT_NO_THROW(coordinator.broadcastToStudents());

    EXPECT_EQ(coordinator.currentRound(), 1u);
    EXPECT_TRUE(coordinator.lastRound().has_value());
}

/**
 * @test FDC-06: broadcastToStudents() throws when no labels submitted.
 *
 * Verifies that calling broadcastToStudents() without prior
 * submitSoftLabels() throws std::runtime_error.
 */
TEST_F(FederatedDistillationCoordinatorTest, BroadcastWithoutSubmitThrows) {
    FederatedDistillationCoordinator coordinator(cfg_);

    EXPECT_THROW(coordinator.broadcastToStudents(), std::runtime_error);
}

// ─────────────────────────────────────────────────────────────────────────────
// Student Registration and Policy Tests (FDC-07..FDC-09)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test FDC-07: Registered student callback receives the broadcast round.
 *
 * Verifies that registerStudent() works and the callback is invoked with
 * the correct round number during broadcastToStudents().
 */
TEST_F(FederatedDistillationCoordinatorTest, RegisteredStudentReceivesBroadcast) {
    FederatedDistillationCoordinator coordinator(cfg_);

    uint64_t received_round = 0;
    coordinator.registerStudent("student-1", [&](const DistillationRound& r) {
        received_round = r.round;
    });

    coordinator.submitSoftLabels("teacher-1", makeLabels(2));
    coordinator.broadcastToStudents();

    EXPECT_EQ(received_round, 1u);
}

/**
 * @test FDC-08: DP budget verification allows broadcast within budget.
 *
 * Verifies that verifyPrivacyBudget() returns true after one round when
 * max_rounds == 0 (unlimited budget).
 */
TEST_F(FederatedDistillationCoordinatorTest, PrivacyBudgetVerificationUnlimited) {
    FederatedDistillationCoordinator coordinator(cfg_);

    // Before any broadcasts, budget is valid
    EXPECT_TRUE(coordinator.verifyPrivacyBudget());

    coordinator.submitSoftLabels("teacher-1", makeLabels(2));
    coordinator.broadcastToStudents();

    // After one round, budget still valid (unlimited)
    EXPECT_TRUE(coordinator.verifyPrivacyBudget());
}

/**
 * @test FDC-09: Policy gate blocks broadcast.
 *
 * Verifies that a policy gate returning false causes broadcastToStudents()
 * to throw std::runtime_error.
 */
TEST_F(FederatedDistillationCoordinatorTest, PolicyGateBlocksBroadcast) {
    FederatedDistillationCoordinator coordinator(cfg_);
    coordinator.setPolicyGate([](uint64_t, const std::string&) { return false; });
    coordinator.submitSoftLabels("teacher-1", makeLabels(2));

    EXPECT_THROW(coordinator.broadcastToStudents(), std::runtime_error);
}

// ─────────────────────────────────────────────────────────────────────────────
// Privacy Budget and Model Card Tests (FDC-10..FDC-12)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test FDC-10: privacyBudgetRemaining() returns max for unlimited rounds.
 *
 * Verifies that when max_rounds == 0, privacyBudgetRemaining() returns
 * std::numeric_limits<double>::max().
 */
TEST_F(FederatedDistillationCoordinatorTest, PrivacyBudgetRemainingUnlimited) {
    cfg_.max_rounds = 0;
    FederatedDistillationCoordinator coordinator(cfg_);

    EXPECT_EQ(coordinator.privacyBudgetRemaining(),
              std::numeric_limits<double>::max());
}

/**
 * @test FDC-11: Multiple students all receive the broadcast.
 *
 * Verifies that when three students are registered, all three receive
 * the broadcast round.
 */
TEST_F(FederatedDistillationCoordinatorTest, MultipleStudentsAllReceiveBroadcast) {
    FederatedDistillationCoordinator coordinator(cfg_);

    std::vector<uint64_t> received_rounds;
    for (int i = 0; i < 3; ++i) {
        coordinator.registerStudent("student-" + std::to_string(i),
            [&received_rounds](const DistillationRound& r) {
                received_rounds.push_back(r.round);
            });
    }

    coordinator.submitSoftLabels("teacher-1", makeLabels(2));
    coordinator.broadcastToStudents();

    EXPECT_EQ(received_rounds.size(), 3u);
    for (uint64_t r : received_rounds) {
        EXPECT_EQ(r, 1u);
    }
}

/**
 * @test FDC-12: generateModelCard() captures round and DP metadata.
 *
 * Verifies that after one broadcast, generateModelCard() returns a snapshot
 * with rounds_completed == 1 and the correct dp_epsilon_per_round.
 */
TEST_F(FederatedDistillationCoordinatorTest, GenerateModelCardAfterBroadcast) {
    FederatedDistillationCoordinator coordinator(cfg_);
    coordinator.submitSoftLabels("teacher-1", makeLabels(2));
    coordinator.broadcastToStudents();

    DistillationModelCard card = coordinator.generateModelCard("coordinator-test");

    EXPECT_EQ(card.coordinator_id,    "coordinator-test");
    EXPECT_EQ(card.rounds_completed,  1u);
    EXPECT_DOUBLE_EQ(card.dp_epsilon_per_round, cfg_.dp_epsilon);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test Summary
// ─────────────────────────────────────────────────────────────────────────────

/*
 * Test Coverage Summary (FDC-01..FDC-12):
 *
 * FDC-01: SoftLabel creation with required fields
 * FDC-02: Valid DistillationConfig reports isValid()
 * FDC-03: SoftLabel JSON serialization round-trip
 * FDC-04: Coordinator initial state (round=0, no submissions)
 * FDC-05: submitSoftLabels + broadcastToStudents advances round
 * FDC-06: broadcastToStudents without prior submit throws
 * FDC-07: Registered student callback receives the broadcast round
 * FDC-08: DP budget verification allows broadcast within unlimited budget
 * FDC-09: Policy gate blocks broadcast
 * FDC-10: privacyBudgetRemaining() returns max for unlimited rounds
 * FDC-11: Multiple students all receive the broadcast
 * FDC-12: generateModelCard() captures round and DP metadata
 *
 * Target: Q3 2026 Hardening - policy gates, DP enforcement, deterministic workflows.
 * Status: Focused unit test suite for federated distillation coordination layer.
 */
