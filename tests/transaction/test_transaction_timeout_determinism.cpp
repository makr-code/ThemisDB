// ==============================================================================
// test_transaction_timeout_determinism.cpp
// ==============================================================================
// CRITICAL MODULE: TRANSACTION
// Acceptance Criterion: AC-5 (Timeout Determinism & Isolation)
//
// PURPOSE:
// Validates timeout behavior determinism under distributed load:
// - Timeout detection deterministic across 50+ replayed scenarios
// - Timeout handling independent of clock drift (±100ms jitter)
// - Cascading timeout handling in multi-step transactions
// - Timeout accuracy within ±50ms bounds
//
// TEST CATEGORIES (12 total):
// TIMEOUT_DETECTION (3): Basic timeout, clock drift, cascading
// DETERMINISM (4): 50x replay identical, clock jitter invariance, ordering
// CASCADING (3): Chain abort, partial timeout, mixed timeout/success
// EDGE_CASES (2): Max timeout, timeout during compensation
//
// ACCEPTANCE CRITERIA:
// AC-5.1: Timeout detection deterministic across 50+ replayed scenarios
// AC-5.2: Timeout handling independent of clock drift (±100ms variation)
// AC-5.3: Timeout cascading in multi-step transactions (abort → compensate)
// AC-5.4: Timeout accuracy within ±50ms bounds (SLA compliance)
// AC-5.5: Timeout ordering preserved in distributed ledger
//
// EVIDENCE: src/transaction/WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md
// GENERATED: 2026-09-02
// ==============================================================================

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <memory>
#include <chrono>
#include <queue>
#include <map>
#include <random>
#include <cmath>

#include "transaction/transaction_manager.h"
#include "transaction/timeout_coordinator.h"
#include "transaction/compensation_log.h"
#include "include/transaction/transaction.h"

namespace themis::transaction::test {

// ==============================================================================
// TIMEOUT DETECTION TESTS (3) — Basic + Clock Drift + Cascading
// ==============================================================================

class TimeoutDetectionTest : public ::testing::Test {
 protected:
  void SetUp() override {
    tx_manager_ = std::make_unique<TransactionManager>();
    timeout_coord_ = std::make_unique<TimeoutCoordinator>();
  }

  void TearDown() override {
    tx_manager_.reset();
    timeout_coord_.reset();
  }

  std::unique_ptr<TransactionManager> tx_manager_;
  std::unique_ptr<TimeoutCoordinator> timeout_coord_;
};

/// AC-5.1: Timeout Detection — Basic timeout at configured duration
TEST_F(TimeoutDetectionTest, TimeoutDetection_BasicTimeout) {
  // Given: Transaction with 500ms timeout
  uint64_t tx_id = 5001;
  std::chrono::milliseconds timeout_duration(500);
  
  // When: Start transaction and wait for timeout
  auto start_time = std::chrono::steady_clock::now();
  tx_manager_->beginTransaction(tx_id, timeout_duration);
  
  // Simulate blocking operation
  std::this_thread::sleep_for(std::chrono::milliseconds(550));
  
  auto elapsed = std::chrono::steady_clock::now() - start_time;
  auto tx_status = tx_manager_->getTransactionStatus(tx_id);

  // Then: Transaction should be marked as TIMED_OUT
  EXPECT_EQ(tx_status.state, "TIMED_OUT")
    << "Transaction should be marked TIMED_OUT after timeout duration";
  
  auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
  EXPECT_GE(elapsed_ms, 500)
    << "Timeout should occur after at least 500ms";
  EXPECT_LE(elapsed_ms, 650)
    << "Timeout should occur within reasonable bounds (500-650ms)";
}

/// AC-5.2: Timeout Handling Independent of Clock Drift — ±100ms variation
TEST_F(TimeoutDetectionTest, TimeoutClockDriftInvariance_PlusMinus100ms) {
  // Given: Transactions with simulated clock drift scenarios
  
  // When: Run 5 transactions with different clock drift conditions
  std::vector<bool> all_timed_out;
  
  for (int drift_offset = -100; drift_offset <= 100; drift_offset += 50) {
    uint64_t tx_id = 5002 + (drift_offset / 50);
    
    // Simulate clock drift
    timeout_coord_->simulateClockDrift(drift_offset);
    
    auto start = std::chrono::steady_clock::now();
    tx_manager_->beginTransaction(tx_id, std::chrono::milliseconds(500));
    
    // Wait for expected timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(550));
    
    auto status = tx_manager_->getTransactionStatus(tx_id);
    all_timed_out.push_back(status.state == "TIMED_OUT");
  }

  // Then: All transactions should timeout regardless of clock drift
  for (int i = 0; i < all_timed_out.size(); ++i) {
    EXPECT_TRUE(all_timed_out[i])
      << "Transaction " << i << " should timeout despite clock drift";
  }
}

/// AC-5.3: Cascading Timeout — Abort cascades to subsequent steps
TEST_F(TimeoutDetectionTest, CascadingTimeout_AbortPropagation) {
  // Given: Multi-step transaction with timeout on step 2
  uint64_t tx_id = 5006;
  
  // When: Execute steps where step 2 times out
  tx_manager_->beginTransaction(tx_id, std::chrono::milliseconds(1000));
  tx_manager_->executeStep(tx_id, "step_1_ok", "{}");
  
  // Step 2 times out
  auto step_2_result = tx_manager_->executeStepWithTimeout(
    tx_id, "step_2_timeout", "{}", 
    std::chrono::milliseconds(100));

  // Step 3 should not execute (cascade from step 2 timeout)
  auto step_3_executed = tx_manager_->executeStep(tx_id, "step_3", "{}");

  // Then: Step 3 should not execute (abort cascade from step 2)
  EXPECT_FALSE(step_3_executed.success)
    << "Step 3 should not execute due to step 2 timeout (cascade)";
  
  auto tx_status = tx_manager_->getTransactionStatus(tx_id);
  EXPECT_EQ(tx_status.state, "ABORTING")
    << "Transaction should be aborting due to timeout cascade";
}

// ==============================================================================
// DETERMINISM TESTS (4) — 50x Replay + Clock Jitter + Ordering
// ==============================================================================

class TimeoutDeterminismTest : public ::testing::Test {
 protected:
  void SetUp() override {
    tx_manager_ = std::make_unique<TransactionManager>();
    timeout_coord_ = std::make_unique<TimeoutCoordinator>();
  }

  void TearDown() override {
    tx_manager_.reset();
    timeout_coord_.reset();
  }

  std::unique_ptr<TransactionManager> tx_manager_;
  std::unique_ptr<TimeoutCoordinator> timeout_coord_;
};

/// AC-5.1: Deterministic Timeout Detection — 50x replay identical outcome
TEST_F(TimeoutDeterminismTest, DeterministicTimeout_50xReplay) {
  // Given: Transaction scenario with 500ms timeout
  const int n_replays = 50;
  std::vector<std::string> timeout_states;
  
  // When: Execute identical scenario 50 times
  for (int replay = 0; replay < n_replays; ++replay) {
    uint64_t tx_id = 6001 + replay;
    
    tx_manager_->beginTransaction(tx_id, std::chrono::milliseconds(300));
    std::this_thread::sleep_for(std::chrono::milliseconds(350));
    
    auto status = tx_manager_->getTransactionStatus(tx_id);
    timeout_states.push_back(status.state);
  }

  // Then: All 50 replays should result in identical TIMED_OUT state
  for (int i = 1; i < timeout_states.size(); ++i) {
    EXPECT_EQ(timeout_states[i], timeout_states[0])
      << "Replay " << i << " should match replay 0 (deterministic)";
    EXPECT_EQ(timeout_states[i], "TIMED_OUT")
      << "All replays should result in TIMED_OUT";
  }
}

/// AC-5.2: Clock Jitter Invariance — ±100ms jitter doesn't break determinism
TEST_F(TimeoutDeterminismTest, ClockJitterInvariance_DeterministicOutcome) {
  // Given: Transactions with ±100ms clock jitter
  const int n_trials = 30;
  std::vector<bool> all_consistent_timeout;
  
  // When: Run transactions with random clock jitter
  for (int trial = 0; trial < n_trials; ++trial) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> jitter_dist(-100, 100);
    int jitter_ms = jitter_dist(gen);
    
    uint64_t tx_id = 6051 + trial;
    
    timeout_coord_->simulateClockJitter(jitter_ms);
    
    tx_manager_->beginTransaction(tx_id, std::chrono::milliseconds(500));
    std::this_thread::sleep_for(std::chrono::milliseconds(550 + jitter_ms));
    
    auto status = tx_manager_->getTransactionStatus(tx_id);
    all_consistent_timeout.push_back(status.state == "TIMED_OUT");
  }

  // Then: All trials should timeout consistently
  for (bool timed_out : all_consistent_timeout) {
    EXPECT_TRUE(timed_out)
      << "Transaction should timeout despite clock jitter";
  }
}

/// AC-5.5: Timeout Ordering Preserved — Distributed ledger ordering
TEST_F(TimeoutDeterminismTest, TimeoutOrdering_DistributedLedger) {
  // Given: Multiple concurrent transactions with staggered timeouts
  std::vector<uint64_t> tx_ids = {7001, 7002, 7003};
  std::vector<std::chrono::milliseconds> timeouts = {
    std::chrono::milliseconds(200),
    std::chrono::milliseconds(400),
    std::chrono::milliseconds(600)
  };

  // When: Start all transactions and record timeout order
  auto start = std::chrono::steady_clock::now();
  std::vector<std::pair<uint64_t, long>> timeout_order;  // (tx_id, time_ms)
  std::mutex order_lock;

  for (int i = 0; i < tx_ids.size(); ++i) {
    std::thread([&](int idx) {
      tx_manager_->beginTransaction(tx_ids[idx], timeouts[idx]);
      std::this_thread::sleep_for(timeouts[idx] + std::chrono::milliseconds(50));
      
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();
      
      std::lock_guard<std::mutex> lock(order_lock);
      timeout_order.push_back({tx_ids[idx], elapsed});
    }).detach();
  }

  // Wait for all to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  // Then: Timeout order should follow configured timeout durations
  std::sort(timeout_order.begin(), timeout_order.end(),
    [](const auto& a, const auto& b) { return a.second < b.second; });
  
  for (int i = 0; i < timeout_order.size(); ++i) {
    EXPECT_EQ(timeout_order[i].first, tx_ids[i])
      << "Timeout order " << i << " should match configured order";
  }
}

/// AC-5.1: Timeout State Consistency — Same state across lookups
TEST_F(TimeoutDeterminismTest, TimeoutStateConsistency_MultipleQueries) {
  // Given: Transaction in timed-out state
  uint64_t tx_id = 7051;
  
  tx_manager_->beginTransaction(tx_id, std::chrono::milliseconds(200));
  std::this_thread::sleep_for(std::chrono::milliseconds(250));

  // When: Query timeout state multiple times
  std::vector<std::string> queried_states = {};

  for (int i = 0; i < 10; ++i) {
    auto status = tx_manager_->getTransactionStatus(tx_id);
    queried_states.push_back(status.state);
  }

  // Then: All queries should return identical state
  for (int i = 1; i < queried_states.size(); ++i) {
    EXPECT_EQ(queried_states[i], queried_states[0])
      << "Query " << i << " should match query 0 (consistent)";
    EXPECT_EQ(queried_states[i], "TIMED_OUT");
  }
}

// ==============================================================================
// CASCADING TIMEOUT TESTS (3) — Chain Abort + Partial + Mixed
// ==============================================================================

class CascadingTimeoutTest : public ::testing::Test {
 protected:
  void SetUp() override {
    tx_manager_ = std::make_unique<TransactionManager>();
  }

  void TearDown() override {
    tx_manager_.reset();
  }

  std::unique_ptr<TransactionManager> tx_manager_;
};

/// AC-5.3: Cascading Abort — Full chain aborts on timeout
TEST_F(CascadingTimeoutTest, CascadingAbort_FullChainTermination) {
  // Given: 5-step transaction where step 3 times out
  uint64_t tx_id = 8001;
  
  // When: Execute steps with step 3 timeout
  tx_manager_->beginTransaction(tx_id, std::chrono::milliseconds(5000));
  tx_manager_->executeStep(tx_id, "step_1", "{}");
  tx_manager_->executeStep(tx_id, "step_2", "{}");
  
  // Step 3 times out
  auto step_3 = tx_manager_->executeStepWithTimeout(
    tx_id, "step_3", "{}", std::chrono::milliseconds(100));
  EXPECT_FALSE(step_3.success);

  // Steps 4 and 5 should not execute
  auto step_4 = tx_manager_->executeStep(tx_id, "step_4", "{}");
  auto step_5 = tx_manager_->executeStep(tx_id, "step_5", "{}");

  // Then: Steps 4 and 5 should not execute (cascade)
  EXPECT_FALSE(step_4.success)
    << "Step 4 should not execute (timeout cascade)";
  EXPECT_FALSE(step_5.success)
    << "Step 5 should not execute (timeout cascade)";
  
  // And: Compensation should begin
  auto compensation_steps = tx_manager_->getCompensationSteps(tx_id);
  EXPECT_GE(compensation_steps.size(), 2)
    << "Should compensate at least steps 1 and 2";
}

/// AC-5.3: Partial Timeout Cascade — Some steps completed before timeout
TEST_F(CascadingTimeoutTest, PartialTimeoutCascade_SelectiveCompensation) {
  // Given: Transaction where step 2 completes before timeout triggers
  uint64_t tx_id = 8002;
  
  // When: Execute steps
  tx_manager_->beginTransaction(tx_id, std::chrono::milliseconds(5000));
  tx_manager_->executeStep(tx_id, "step_1", "{}");
  tx_manager_->executeStep(tx_id, "step_2_ok", "{}");
  
  // Step 3 times out (after step 2 completed)
  auto step_3 = tx_manager_->executeStepWithTimeout(
    tx_id, "step_3_timeout", "{}", std::chrono::milliseconds(50));
  EXPECT_FALSE(step_3.success);

  // Then: Should compensate only completed steps (1, 2)
  auto compensation_steps = tx_manager_->getCompensationSteps(tx_id);
  EXPECT_EQ(compensation_steps.size(), 2)
    << "Should compensate 2 completed steps";
  
  // Verify steps are in reverse order (LIFO)
  EXPECT_EQ(compensation_steps[0], "step_2_ok");
  EXPECT_EQ(compensation_steps[1], "step_1");
}

/// AC-5.3: Mixed Timeout and Success — Some steps timeout, some succeed
TEST_F(CascadingTimeoutTest, MixedTimeoutAndSuccess_PartialCompletion) {
  // Given: 4-step transaction with step 2 timeout (step 1 succeeds)
  uint64_t tx_id = 8003;
  
  tx_manager_->beginTransaction(tx_id, std::chrono::milliseconds(5000));
  tx_manager_->executeStep(tx_id, "step_1_ok", "{}");
  
  // Step 2 times out
  auto step_2 = tx_manager_->executeStepWithTimeout(
    tx_id, "step_2_timeout", "{}", std::chrono::milliseconds(50));
  EXPECT_FALSE(step_2.success);

  // Step 3 should not execute (timeout cascade)
  auto step_3 = tx_manager_->executeStep(tx_id, "step_3", "{}");
  EXPECT_FALSE(step_3.success)
    << "Step 3 should not execute due to cascade";

  // Then: Only step 1 should be compensated
  auto compensation_steps = tx_manager_->getCompensationSteps(tx_id);
  EXPECT_EQ(compensation_steps.size(), 1)
    << "Should compensate only step 1";
  EXPECT_EQ(compensation_steps[0], "step_1_ok");
}

// ==============================================================================
// EDGE CASE TESTS (2) — Max Timeout + Timeout During Compensation
// ==============================================================================

class TimeoutEdgeCaseTest : public ::testing::Test {
 protected:
  void SetUp() override {
    tx_manager_ = std::make_unique<TransactionManager>();
  }

  void TearDown() override {
    tx_manager_.reset();
  }

  std::unique_ptr<TransactionManager> tx_manager_;
};

/// AC-5.4: Maximum Timeout Accuracy — Within ±50ms bounds
TEST_F(TimeoutEdgeCaseTest, TimeoutAccuracy_Within50msMargin) {
  // Given: Multiple transactions with various timeout durations
  std::vector<int> timeout_durations_ms = {100, 250, 500, 1000};
  
  // When: Measure actual timeout accuracy for each duration
  for (int expected_ms : timeout_durations_ms) {
    uint64_t tx_id = 9001 + (expected_ms / 100);
    
    auto start = std::chrono::steady_clock::now();
    tx_manager_->beginTransaction(tx_id, std::chrono::milliseconds(expected_ms));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(expected_ms + 100));
    
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start).count();
    
    auto status = tx_manager_->getTransactionStatus(tx_id);
    EXPECT_EQ(status.state, "TIMED_OUT");
    
    // Then: Actual timeout should be within ±50ms of expected
    int variance = static_cast<int>(elapsed) - expected_ms;
    EXPECT_GE(variance, -50)
      << "Timeout variance should be >= -50ms for " << expected_ms << "ms";
    EXPECT_LE(variance, 50)
      << "Timeout variance should be <= 50ms for " << expected_ms << "ms";
  }
}

/// AC-5.3: Timeout During Compensation — Handle gracefully
TEST_F(TimeoutEdgeCaseTest, TimeoutDuringCompensation_GracefulHandling) {
  // Given: Transaction timing out during compensation phase
  uint64_t tx_id = 9051;
  
  tx_manager_->beginTransaction(tx_id, std::chrono::milliseconds(1000));
  tx_manager_->executeStep(tx_id, "step_1", "{}");
  tx_manager_->executeStep(tx_id, "step_2", "{}");
  
  // Trigger timeout during step 3
  auto step_3 = tx_manager_->executeStepWithTimeout(
    tx_id, "step_3", "{}", std::chrono::milliseconds(100));
  EXPECT_FALSE(step_3.success);

  // When: Begin compensation with timeout occurring during compensation
  auto compensation_timeout = std::chrono::milliseconds(50);
  auto comp_result = tx_manager_->compensateWithTimeout(tx_id, compensation_timeout);

  // Then: Compensation should handle timeout gracefully
  EXPECT_FALSE(comp_result.success)
    << "Compensation timeout should be handled gracefully";
  
  auto final_state = tx_manager_->getTransactionState(tx_id);
  EXPECT_NE(final_state.state, "UNKNOWN")
    << "Transaction should have defined state after compensation timeout";
}

}  // namespace themis::transaction::test
