/**
 * @file test_updates_determinism_edge_cases_focused.cpp
 * @brief Phase 4: Focused deterministic edge-case tests for UpdatesEdgeCaseHandler
 * @version 1.0.0
 * @since 2.4.0 (Q4 2026)
 *
 * Coverage targets:
 *  - All 9 primary edge-case detection categories (UDE-01..UDE-10)
 *  - classifyEdgeCase(), isFatal(), requiresIsolation() helpers (UDE-11..UDE-15)
 *  - Statistics tracking under single-threaded and concurrent use (UDE-16..UDE-19)
 *  - Determinism: identical input → identical result (UDE-20)
 *
 * Test IDs: UDE-01..UDE-20
 * CTest labels: updates;edge_cases;phase4
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>
#include "updates/update_state_machine.h"
#include "updates/updates_edge_case_handler.h"

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

using namespace themis::updates;

// ============================================================================
// Fixtures
// ============================================================================

class EdgeCaseHandlerTest : public ::testing::Test {
protected:
    UpdateStateMachine sm_;
    UpdatesEdgeCaseHandler handler_;

    void SetUp() override {
    }
};

// ============================================================================
// UDE-01..UDE-10: detectAndHandle scenarios
// ============================================================================

/// UDE-01: IDLE state with no hint → no edge case detected
TEST_F(EdgeCaseHandlerTest, UDE_01_IDLE_NoEdgeCase) {
    ASSERT_EQ(sm_.currentState(), UpdateState::IDLE);
    auto result = handler_.detectAndHandle(sm_, "");
    EXPECT_FALSE(result.handled)
        << "IDLE state with empty context should not trigger an edge case";
}

/// UDE-02: FAILED state lock detected (7402)
TEST_F(EdgeCaseHandlerTest, UDE_02_FailedStateLockDetected) {
    // Drive the machine to FAILED
    ASSERT_TRUE(sm_.transition(UpdateState::DOWNLOADING, "1.0.0", "start"));
    ASSERT_TRUE(sm_.transition(UpdateState::FAILED, "", "error"));

    auto result = handler_.detectAndHandle(sm_, "");
    ASSERT_TRUE(result.handled);
    EXPECT_EQ(result.error_code, DiagnosticErrorCode::STATE_FAILED_LOCKED);
    EXPECT_FALSE(result.requires_rollback)
        << "STATE_FAILED_LOCKED does not recommend rollback; operator reset needed";
}

/// UDE-03: Cascade rollback detection (7423)
TEST_F(EdgeCaseHandlerTest, UDE_03_CascadeRollbackDetected) {
    ASSERT_TRUE(sm_.transition(UpdateState::DOWNLOADING, "1.0.0", "start"));
    ASSERT_TRUE(sm_.transition(UpdateState::ROLLING_BACK, "", "rollback"));

    auto result = handler_.detectAndHandle(sm_, "cascade");
    ASSERT_TRUE(result.handled);
    EXPECT_EQ(result.error_code, DiagnosticErrorCode::ROLLBACK_CASCADE_DETECTED);
}

/// UDE-04: Concurrent update collision (7401)
TEST_F(EdgeCaseHandlerTest, UDE_04_ConcurrentUpdateCollision) {
    ASSERT_TRUE(sm_.transition(UpdateState::DOWNLOADING, "1.0.0", "start"));

    auto result = handler_.detectAndHandle(sm_, "concurrent");
    ASSERT_TRUE(result.handled);
    EXPECT_EQ(result.error_code, DiagnosticErrorCode::STATE_ALREADY_IN_PROGRESS);
    EXPECT_TRUE(result.requires_rollback);
}

/// UDE-05: Transition timeout scenario (7403)
TEST_F(EdgeCaseHandlerTest, UDE_05_TransitionTimeout) {
    ASSERT_TRUE(sm_.transition(UpdateState::VERIFYING, "1.0.0", "verify"));

    auto result = handler_.detectAndHandle(sm_, "timeout");
    ASSERT_TRUE(result.handled);
    EXPECT_EQ(result.error_code, DiagnosticErrorCode::STATE_TRANSITION_TIMEOUT);
    EXPECT_TRUE(result.requires_rollback);
}

/// UDE-06: Patch conflict detection (7440)
TEST_F(EdgeCaseHandlerTest, UDE_06_PatchConflictDetected) {
    ASSERT_TRUE(sm_.transition(UpdateState::DOWNLOADING, "1.0.0", "start"));
    ASSERT_TRUE(sm_.transition(UpdateState::VERIFYING, "", "verify"));
    ASSERT_TRUE(sm_.transition(UpdateState::APPLYING, "", "apply"));

    auto result = handler_.detectAndHandle(sm_, "patch-conflict");
    ASSERT_TRUE(result.handled);
    EXPECT_EQ(result.error_code, DiagnosticErrorCode::PATCH_APPLY_FAILED);
    EXPECT_TRUE(result.requires_rollback);
}

/// UDE-07: Manifest version conflict → NETWORK_PARTITION (7460)
TEST_F(EdgeCaseHandlerTest, UDE_07_ManifestVersionConflict) {
    ASSERT_TRUE(sm_.transition(UpdateState::DOWNLOADING, "1.0.0", "start"));

    auto result = handler_.detectAndHandle(sm_, "manifest-conflict");
    ASSERT_TRUE(result.handled);
    EXPECT_EQ(result.error_code, DiagnosticErrorCode::NETWORK_PARTITION);
    EXPECT_FALSE(result.requires_rollback);
}

/// UDE-08: Rollback isolation active (7426)
TEST_F(EdgeCaseHandlerTest, UDE_08_RollbackIsolationActive) {
    auto result = handler_.detectAndHandle(sm_, "isolation");
    ASSERT_TRUE(result.handled);
    EXPECT_EQ(result.error_code, DiagnosticErrorCode::ROLLBACK_ISOLATION_ACTIVE);
}

/// UDE-09: Quota exhaustion → COORDINATION_ORDERING_VIOLATION (7464)
TEST_F(EdgeCaseHandlerTest, UDE_09_QuotaExhaustion) {
    auto result = handler_.detectAndHandle(sm_, "quota");
    ASSERT_TRUE(result.handled);
    EXPECT_EQ(result.error_code, DiagnosticErrorCode::COORDINATION_ORDERING_VIOLATION);
    EXPECT_FALSE(result.requires_rollback);
}

/// UDE-10: Partial success requiring retry (7424)
TEST_F(EdgeCaseHandlerTest, UDE_10_PartialSuccessRequiresRetry) {
    ASSERT_TRUE(sm_.transition(UpdateState::DOWNLOADING, "1.0.0", "start"));
    ASSERT_TRUE(sm_.transition(UpdateState::ROLLING_BACK, "", "rolling"));

    auto result = handler_.detectAndHandle(sm_, "partial-success");
    ASSERT_TRUE(result.handled);
    EXPECT_EQ(result.error_code, DiagnosticErrorCode::ROLLBACK_PARTIAL_SUCCESS);
    EXPECT_TRUE(result.requires_rollback);
}

// ============================================================================
// UDE-11..UDE-15: Helper function coverage
// ============================================================================

/// UDE-11: classifyEdgeCase returns non-empty for all defined codes
TEST(EdgeCaseClassifyTest, UDE_11_ClassifyEdgeCaseNonEmpty) {
    const DiagnosticErrorCode codes[] = {
        DiagnosticErrorCode::STATE_INVALID_TRANSITION,
        DiagnosticErrorCode::STATE_ALREADY_IN_PROGRESS,
        DiagnosticErrorCode::STATE_FAILED_LOCKED,
        DiagnosticErrorCode::STATE_TRANSITION_TIMEOUT,
        DiagnosticErrorCode::ROLLBACK_CASCADE_DETECTED,
        DiagnosticErrorCode::ROLLBACK_PARTIAL_SUCCESS,
        DiagnosticErrorCode::ROLLBACK_ISOLATION_ACTIVE,
        DiagnosticErrorCode::PATCH_APPLY_FAILED,
        DiagnosticErrorCode::NETWORK_PARTITION,
        DiagnosticErrorCode::COORDINATION_ORDERING_VIOLATION,
    };
    for (auto code : codes) {
        auto sv = UpdatesEdgeCaseHandler::classifyEdgeCase(code);
        EXPECT_FALSE(sv.empty()) << "classifyEdgeCase returned empty for code "
                                 << static_cast<uint16_t>(code);
        EXPECT_NE(sv, "Unknown") << "classifyEdgeCase returned Unknown for defined code "
                                 << static_cast<uint16_t>(code);
    }
}

/// UDE-12: isFatal returns true for STATE_FAILED_LOCKED
TEST(EdgeCaseFatalTest, UDE_12_IsFatalForStateLocked) {
    EXPECT_TRUE(UpdatesEdgeCaseHandler::isFatal(DiagnosticErrorCode::STATE_FAILED_LOCKED));
}

/// UDE-13: isFatal returns false for ROLLBACK_PARTIAL_SUCCESS
TEST(EdgeCaseFatalTest, UDE_13_IsFatalFalseForPartialSuccess) {
    EXPECT_FALSE(UpdatesEdgeCaseHandler::isFatal(DiagnosticErrorCode::ROLLBACK_PARTIAL_SUCCESS));
}

/// UDE-14: requiresIsolation returns true for ROLLBACK_CASCADE_DETECTED
TEST(EdgeCaseIsolationTest, UDE_14_RequiresIsolationForCascade) {
    EXPECT_TRUE(UpdatesEdgeCaseHandler::requiresIsolation(
        DiagnosticErrorCode::ROLLBACK_CASCADE_DETECTED));
}

/// UDE-15: requiresIsolation returns false for NETWORK_PARTITION (manifest conflict)
TEST(EdgeCaseIsolationTest, UDE_15_RequiresIsolationFalseForManifestConflict) {
    EXPECT_FALSE(UpdatesEdgeCaseHandler::requiresIsolation(
        DiagnosticErrorCode::NETWORK_PARTITION));
}

// ============================================================================
// UDE-16..UDE-18: Statistics tracking
// ============================================================================

/// UDE-16: stats.total_detected increments on detection
TEST_F(EdgeCaseHandlerTest, UDE_16_StatsIncrementOnDetection) {
    const auto before = handler_.getStats().total_detected;
    ASSERT_TRUE(sm_.transition(UpdateState::DOWNLOADING, "1.0.0", "start"));
    ASSERT_TRUE(sm_.transition(UpdateState::FAILED, "", "error"));

    handler_.detectAndHandle(sm_, "");
    EXPECT_EQ(handler_.getStats().total_detected, before + 1);
}

/// UDE-17: stats.total_fatal increments when fatal edge case detected
TEST_F(EdgeCaseHandlerTest, UDE_17_StatsFatalIncrements) {
    ASSERT_TRUE(sm_.transition(UpdateState::DOWNLOADING, "1.0.0", "start"));
    ASSERT_TRUE(sm_.transition(UpdateState::FAILED, "", "error"));

    const auto before = handler_.getStats().total_fatal;
    handler_.detectAndHandle(sm_, "");
    EXPECT_EQ(handler_.getStats().total_fatal, before + 1)
        << "STATE_FAILED_LOCKED is fatal; total_fatal should have incremented";
}

/// UDE-18: stats.total_rollback_triggered increments when requires_rollback
TEST_F(EdgeCaseHandlerTest, UDE_18_StatsRollbackTriggered) {
    ASSERT_TRUE(sm_.transition(UpdateState::DOWNLOADING, "1.0.0", "start"));

    const auto before = handler_.getStats().total_rollback_triggered;
    auto result = handler_.detectAndHandle(sm_, "concurrent");
    ASSERT_TRUE(result.handled);
    ASSERT_TRUE(result.requires_rollback);
    EXPECT_EQ(handler_.getStats().total_rollback_triggered, before + 1);
}

// ============================================================================
// UDE-19: Thread-safe concurrent detection — no data race
// ============================================================================

/// UDE-19: 2 threads each call detectAndHandle 500 times; stats must be consistent
TEST(EdgeCaseThreadSafetyTest, UDE_19_ConcurrentDetectionNoRace) {
    UpdatesEdgeCaseHandler shared_handler;
    constexpr int kItersPerThread = 500;
    constexpr int kThreads = 2;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&shared_handler]() {
            UpdateStateMachine sm("");
            // Drive to FAILED then detect — guaranteed to trigger.
            ASSERT_TRUE(sm.transition(UpdateState::DOWNLOADING, "1.0.0", "t"));
            ASSERT_TRUE(sm.transition(UpdateState::FAILED, "", "e"));
            for (int i = 0; i < kItersPerThread; ++i) {
                auto r = shared_handler.detectAndHandle(sm, "");
                EXPECT_TRUE(r.handled);
            }
        });
    }

    for (auto& th : threads) {
      th.join();
    }

    const uint64_t expected = static_cast<uint64_t>(kThreads * kItersPerThread);
    EXPECT_EQ(shared_handler.getStats().total_detected, expected);
}

// ============================================================================
// UDE-20: Determinism — same input → same EdgeCaseResult
// ============================================================================

/// UDE-20: Identical state machine snapshot and hint produce identical result
TEST_F(EdgeCaseHandlerTest, UDE_20_DeterministicOutput) {
    ASSERT_TRUE(sm_.transition(UpdateState::DOWNLOADING, "1.0.0", "start"));

    const auto r1 = handler_.detectAndHandle(sm_, "timeout");
    const auto r2 = handler_.detectAndHandle(sm_, "timeout");

    ASSERT_TRUE(r1.handled);
    ASSERT_TRUE(r2.handled);
    EXPECT_EQ(r1.error_code,       r2.error_code);
    EXPECT_EQ(r1.requires_rollback, r2.requires_rollback);
    // descriptions match (both reference same state name)
    EXPECT_EQ(r1.description, r2.description);
}
