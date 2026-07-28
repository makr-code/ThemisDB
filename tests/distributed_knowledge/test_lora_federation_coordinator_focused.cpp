/**
 * @file test_lora_federation_coordinator_focused.cpp
 * @brief Focused unit tests for LoRA federated aggregation coordination.
 *
 * Tests the core distributed knowledge LoRA federation paths against the
 * actual production API surface:
 * - EncryptedGradient submission and round tracking
 * - triggerAggregation() state machine and error conditions
 * - FederationConfig validation and coordinator lifecycle
 * - timeout semantics and partial-failure handling
 *
 * Target: Production readiness validation for federated LoRA layer.
 * Q3 2026 Hardening: timeout semantics, aggregation merge contracts, determinism.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <vector>
#include <memory>

#include "distributed_knowledge/lora_federation_coordinator.h"

using namespace themis::distributed_knowledge;
using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Test Fixtures
// ─────────────────────────────────────────────────────────────────────────────

class LoRAFederationCoordinatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        cfg_.min_participants = 2;
        cfg_.max_participants = 8;
        cfg_.dp_epsilon       = 0.5;
        cfg_.dp_delta         = 1e-5;
        cfg_.dp_sensitivity   = 1.0;
        cfg_.max_rounds       = 0;  // unlimited
    }

    FederationConfig cfg_;

    /// Build a minimal EncryptedGradient for a given shard/round.
    static EncryptedGradient makeGradient(const std::string& shard_id, uint64_t round,
                                          size_t samples = 100) {
        EncryptedGradient g;
        g.shard_id     = shard_id;
        g.round        = round;
        g.sample_count = samples;
        g.data         = {{"w0", 0.01}, {"w1", -0.02}};
        return g;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Configuration and Lifecycle Tests (LFC-01..LFC-03)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test LFC-01: FederationConfig validation — valid config.
 *
 * Verifies that a properly constructed FederationConfig reports isValid() == true.
 */
TEST_F(LoRAFederationCoordinatorTest, ValidConfigReportsValid) {
    EXPECT_TRUE(cfg_.isValid());
}

/**
 * @test LFC-02: FederationConfig validation — invalid epsilon rejected.
 *
 * Verifies that a config with dp_epsilon == 0 is rejected by isValid().
 */
TEST_F(LoRAFederationCoordinatorTest, InvalidEpsilonRejected) {
    cfg_.dp_epsilon = 0.0;
    EXPECT_FALSE(cfg_.isValid());
}

/**
 * @test LFC-03: Coordinator initial state — round 0, no submissions.
 *
 * Verifies that a freshly constructed coordinator starts at round 0 with
 * no pending gradients and no completed delta.
 */
TEST_F(LoRAFederationCoordinatorTest, CoordinatorInitialState) {
    LoRAFederationCoordinator coordinator(cfg_);

    EXPECT_EQ(coordinator.currentRound(), 0u);
    EXPECT_EQ(coordinator.submittedCount(), 0u);
    EXPECT_FALSE(coordinator.lastDelta().has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// Gradient Submission Tests (LFC-04..LFC-06)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test LFC-04: submitGradient() increments submittedCount.
 *
 * Verifies that submitting a gradient for round 0 increments submittedCount().
 */
TEST_F(LoRAFederationCoordinatorTest, SubmitGradientIncrementsCount) {
    LoRAFederationCoordinator coordinator(cfg_);

    coordinator.submitGradient(makeGradient("shard-001", 0));
    EXPECT_EQ(coordinator.submittedCount(), 1u);
}

/**
 * @test LFC-05: Duplicate gradient submission is idempotent.
 *
 * Verifies that submitting two gradients from the same shard for the same
 * round only counts once (idempotency contract).
 */
TEST_F(LoRAFederationCoordinatorTest, DuplicateGradientIsIdempotent) {
    LoRAFederationCoordinator coordinator(cfg_);

    coordinator.submitGradient(makeGradient("shard-001", 0));
    coordinator.submitGradient(makeGradient("shard-001", 0));  // duplicate
    EXPECT_EQ(coordinator.submittedCount(), 1u);
}

/**
 * @test LFC-06: Multiple distinct shard submissions are counted correctly.
 *
 * Verifies that two distinct shards contributing gradients results in
 * submittedCount() == 2.
 */
TEST_F(LoRAFederationCoordinatorTest, TwoShardSubmissionsCountedCorrectly) {
    LoRAFederationCoordinator coordinator(cfg_);

    coordinator.submitGradient(makeGradient("shard-001", 0));
    coordinator.submitGradient(makeGradient("shard-002", 0));
    EXPECT_EQ(coordinator.submittedCount(), 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Aggregation Tests (LFC-07..LFC-10)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test LFC-07: triggerAggregation() throws when below min_participants.
 *
 * Verifies that calling triggerAggregation() with only one gradient submitted
 * (min_participants == 2) throws std::runtime_error.
 */
TEST_F(LoRAFederationCoordinatorTest, AggregationThrowsBelowMinParticipants) {
    LoRAFederationCoordinator coordinator(cfg_);
    coordinator.submitGradient(makeGradient("shard-001", 0));

    EXPECT_THROW(coordinator.triggerAggregation(), std::runtime_error);
}

/**
 * @test LFC-08: triggerAggregation() succeeds with min_participants met.
 *
 * Verifies that with two gradients submitted (== min_participants),
 * triggerAggregation() returns a valid GlobalAdapterDelta.
 */
TEST_F(LoRAFederationCoordinatorTest, AggregationSucceedsWithMinParticipants) {
    LoRAFederationCoordinator coordinator(cfg_);
    coordinator.submitGradient(makeGradient("shard-001", 0));
    coordinator.submitGradient(makeGradient("shard-002", 0));

    ASSERT_NO_THROW({
        GlobalAdapterDelta delta = coordinator.triggerAggregation();
        EXPECT_EQ(delta.participants, 2u);
        EXPECT_FALSE(delta.version.empty());
    });
}

/**
 * @test LFC-09: Successful aggregation advances the round counter.
 *
 * Verifies that after triggerAggregation() succeeds, currentRound() is 1.
 */
TEST_F(LoRAFederationCoordinatorTest, AggregationAdvancesRound) {
    LoRAFederationCoordinator coordinator(cfg_);
    coordinator.submitGradient(makeGradient("shard-001", 0));
    coordinator.submitGradient(makeGradient("shard-002", 0));
    coordinator.triggerAggregation();

    EXPECT_EQ(coordinator.currentRound(), 1u);
    EXPECT_TRUE(coordinator.lastDelta().has_value());
}

/**
 * @test LFC-10: triggerAggregation() with timeout_ms=1 throws on zero-gradient round.
 *
 * Verifies timeout error handling: when no gradients are submitted and a
 * 1 ms timeout is requested, a std::runtime_error is thrown.
 */
TEST_F(LoRAFederationCoordinatorTest, AggregationTimeoutHandling) {
    LoRAFederationCoordinator coordinator(cfg_);
    // No gradients submitted — aggregation should fail with a meaningful error.
    EXPECT_THROW(coordinator.triggerAggregation(1u), std::runtime_error);
}

// ─────────────────────────────────────────────────────────────────────────────
// Multiple Rounds and Serialization Tests (LFC-11..LFC-12)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test LFC-11: Multiple consecutive aggregation rounds.
 *
 * Verifies that the coordinator correctly handles two sequential rounds,
 * advancing round counter with each successful aggregation.
 */
TEST_F(LoRAFederationCoordinatorTest, MultipleConsecutiveRounds) {
    LoRAFederationCoordinator coordinator(cfg_);

    for (uint64_t round = 0; round < 2; ++round) {
        coordinator.submitGradient(makeGradient("shard-001", round));
        coordinator.submitGradient(makeGradient("shard-002", round));
        ASSERT_NO_THROW(coordinator.triggerAggregation());
    }

    EXPECT_EQ(coordinator.currentRound(), 2u);
}

/**
 * @test LFC-12: GlobalAdapterDelta JSON serialization round-trip.
 *
 * Verifies that GlobalAdapterDelta serialises to JSON and deserialises back
 * with consistent field values.
 */
TEST_F(LoRAFederationCoordinatorTest, GlobalAdapterDeltaSerializationRoundTrip) {
    GlobalAdapterDelta delta;
    delta.round        = 3;
    delta.version      = "global-v3";
    delta.participants = 4;
    delta.algorithm    = "FedAvg";
    delta.epsilon_spent = 0.1;
    delta.delta        = {{"w0", 0.005}};

    json j = delta.toJson();
    GlobalAdapterDelta restored = GlobalAdapterDelta::fromJson(j);

    EXPECT_EQ(restored.round,        delta.round);
    EXPECT_EQ(restored.version,      delta.version);
    EXPECT_EQ(restored.participants, delta.participants);
    EXPECT_EQ(restored.algorithm,    delta.algorithm);
    EXPECT_DOUBLE_EQ(restored.epsilon_spent, delta.epsilon_spent);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test Summary
// ─────────────────────────────────────────────────────────────────────────────

/*
 * Test Coverage Summary (LFC-01..LFC-12):
 *
 * LFC-01: Valid FederationConfig reports isValid()
 * LFC-02: Invalid epsilon rejected by isValid()
 * LFC-03: Coordinator initial state (round=0, no submissions)
 * LFC-04: submitGradient() increments submittedCount
 * LFC-05: Duplicate gradient submission is idempotent
 * LFC-06: Two distinct shard submissions counted correctly
 * LFC-07: triggerAggregation() throws below min_participants
 * LFC-08: triggerAggregation() succeeds with min_participants met
 * LFC-09: Successful aggregation advances round counter
 * LFC-10: Timeout error handling (triggerAggregation with 1ms)
 * LFC-11: Multiple consecutive aggregation rounds
 * LFC-12: GlobalAdapterDelta JSON serialization round-trip
 *
 * Target: Q3 2026 Hardening - aggregation coordination, merge contracts, timeout semantics.
 * Status: Focused unit test suite for LoRA federation coordination layer.
 */
