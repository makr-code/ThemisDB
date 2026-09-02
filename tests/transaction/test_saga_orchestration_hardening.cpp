// ==============================================================================
// test_saga_orchestration_hardening.cpp
// ==============================================================================
// CRITICAL MODULE: TRANSACTION
// Acceptance Criteria: AC-9 (SAGA Orchestration) + AC-10 (Retry Storm Handling)
//
// PURPOSE:
// Validates SAGA orchestration robustness under failure scenarios:
// - Circuit breaker activation after 5 consecutive remote failures
// - Compensation idempotency under 10 concurrent retries
// - Partial failure handling (some steps succeed, some fail)
// - Retry storm suppression with exponential backoff
//
// TEST CATEGORIES (20 total):
// CIRCUIT_BREAKER (5): Threshold, recovery, half-open state, metrics, edge cases
// COMPENSATION (7): Idempotency, concurrent retries, ordering, rollback chains
// PARTIAL_FAILURE (5): Mixed success/failure, cascading, selective compensation
// RETRY_STORM (3): Backoff validation, jitter, bounded retries
//
// ACCEPTANCE CRITERIA:
// AC-9.1: Circuit breaker activates after 5 consecutive remote failures
// AC-9.2: Partial failure scenarios (some steps OK, some fail)
// AC-9.3: Compensation ordering is deterministic (reverse sequence)
// AC-10.1: Compensation idempotency under 10 concurrent retries (same outcome)
// AC-10.2: Retry storm suppression with exponential backoff (base 100ms, factor 2x)
// AC-10.3: Error codes consistent across retries (no silent transitions)
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

#include "transaction/saga_orchestrator.h"
#include "transaction/distributed_saga.h"
#include "transaction/compensation_log.h"
#include "include/transaction/transaction.h"

namespace themis::transaction::test {

// ==============================================================================
// CIRCUIT BREAKER TESTS (5) — Threshold + Recovery Validation
// ==============================================================================

class CircuitBreakerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Initialize SAGA orchestrator with default settings
    saga_orchestrator_ = std::make_unique<SAGAOrchestrator>();
  }

  void TearDown() override {
    saga_orchestrator_.reset();
  }

  std::unique_ptr<SAGAOrchestrator> saga_orchestrator_;
};

/// AC-9.1: Circuit Breaker Activation — Threshold 5 consecutive failures
TEST_F(CircuitBreakerTest, CircuitBreakerActivatesAtThreshold_5Failures) {
  // Given: SAGA orchestrator with remote step
  uint64_t saga_id = 1001;
  
  // When: Simulate 5 consecutive remote failures
  std::vector<bool> failure_sequence = {false, false, false, false, false};  // 5 failures
  
  for (int i = 0; i < 5; ++i) {
    // Attempt to execute remote step (simulate failure)
    auto result = saga_orchestrator_->executeRemoteStep(saga_id, "remote_step_1",
                                                         "{\"action\": \"debit\"}");
    EXPECT_FALSE(result.success) << "Step " << i << " should fail";
  }

  // Then: Circuit breaker should be open (reject subsequent calls)
  auto cb_state = saga_orchestrator_->getCircuitBreakerState("remote_step_1");
  EXPECT_EQ(cb_state, "OPEN") 
    << "Circuit breaker should be OPEN after 5 consecutive failures";
}

/// AC-9.1: Circuit Breaker Recovery — Half-open state after timeout
TEST_F(CircuitBreakerTest, CircuitBreakerRecovery_HalfOpenAfterTimeout) {
  // Given: Circuit breaker in OPEN state (from previous test)
  uint64_t saga_id = 1002;
  
  // Trigger 5 failures to open circuit
  for (int i = 0; i < 5; ++i) {
    saga_orchestrator_->executeRemoteStep(saga_id, "remote_step_2", "{}");
  }
  EXPECT_EQ(saga_orchestrator_->getCircuitBreakerState("remote_step_2"), "OPEN");

  // When: Wait for recovery timeout (typically 30s, simulate via mock)
  saga_orchestrator_->simulateCircuitBreakerTimeout("remote_step_2");

  // Then: Circuit breaker should transition to HALF_OPEN
  auto state_after = saga_orchestrator_->getCircuitBreakerState("remote_step_2");
  EXPECT_EQ(state_after, "HALF_OPEN")
    << "Circuit breaker should be HALF_OPEN after timeout";
}

/// AC-9.1: Successful Recovery — Transition HALF_OPEN → CLOSED
TEST_F(CircuitBreakerTest, CircuitBreakerRecovery_SuccessfulTransition) {
  // Given: Circuit breaker in HALF_OPEN state
  uint64_t saga_id = 1003;
  
  // Trigger failures then timeout to reach HALF_OPEN
  for (int i = 0; i < 5; ++i) {
    saga_orchestrator_->executeRemoteStep(saga_id, "remote_step_3", "{}");
  }
  saga_orchestrator_->simulateCircuitBreakerTimeout("remote_step_3");
  EXPECT_EQ(saga_orchestrator_->getCircuitBreakerState("remote_step_3"), "HALF_OPEN");

  // When: Execute a successful request in HALF_OPEN state
  auto result = saga_orchestrator_->executeRemoteStep(saga_id, "remote_step_3",
                                                       "{\"action\": \"debit\", \"amount\": 100}",
                                                       true);  // true = success

  // Then: Circuit breaker should close
  auto state_closed = saga_orchestrator_->getCircuitBreakerState("remote_step_3");
  EXPECT_EQ(state_closed, "CLOSED")
    << "Circuit breaker should transition to CLOSED after successful request";
  EXPECT_TRUE(result.success);
}

/// AC-9.1: Metrics Collection — Failure count, state transitions
TEST_F(CircuitBreakerTest, CircuitBreakerMetrics_CountingAndTracking) {
  // Given: SAGA with tracked circuit breaker metrics
  uint64_t saga_id = 1004;
  
  // When: Execute requests and track state transitions
  for (int i = 0; i < 3; ++i) {
    saga_orchestrator_->executeRemoteStep(saga_id, "remote_step_4", "{}", false);
  }

  // Then: Metrics should show 3 failures recorded
  auto metrics = saga_orchestrator_->getCircuitBreakerMetrics("remote_step_4");
  EXPECT_EQ(metrics.consecutive_failures, 3)
    << "Failure counter should track 3 failures";
  EXPECT_GT(metrics.last_transition_time_ms, 0)
    << "Transition time should be recorded";
}

/// AC-9.1: Edge Case — Single failure doesn't trigger circuit breaker
TEST_F(CircuitBreakerTest, CircuitBreakerEdgeCase_NoTriggerBeforeThreshold) {
  // Given: Circuit breaker threshold is 5
  uint64_t saga_id = 1005;
  
  // When: Execute only 4 failed requests
  for (int i = 0; i < 4; ++i) {
    saga_orchestrator_->executeRemoteStep(saga_id, "remote_step_5", "{}", false);
  }

  // Then: Circuit breaker should still be CLOSED (threshold not reached)
  auto state = saga_orchestrator_->getCircuitBreakerState("remote_step_5");
  EXPECT_EQ(state, "CLOSED")
    << "Circuit breaker should not open before threshold (4 < 5)";
}

// ==============================================================================
// COMPENSATION IDEMPOTENCY TESTS (7) — Concurrent Retries + Ordering
// ==============================================================================

class CompensationIdempotencyTest : public ::testing::Test {
 protected:
  void SetUp() override {
    saga_orchestrator_ = std::make_unique<SAGAOrchestrator>();
    compensation_log_ = std::make_unique<CompensationLog>();
  }

  void TearDown() override {
    saga_orchestrator_.reset();
    compensation_log_.reset();
  }

  std::unique_ptr<SAGAOrchestrator> saga_orchestrator_;
  std::unique_ptr<CompensationLog> compensation_log_;
};

/// AC-10.1: Compensation Idempotency — Same compensation called 10x → same outcome
TEST_F(CompensationIdempotencyTest, CompensationIdempotency_10ConcurrentRetries) {
  // Given: A SAGA transaction with compensation step
  uint64_t saga_id = 2001;
  const int n_retries = 10;
  
  // Initial state: debit account by 100
  std::map<std::string, int> account_state;
  account_state["balance"] = 1000;
  
  // When: Execute forward step
  saga_orchestrator_->executeStep(saga_id, "debit", 100);
  account_state["balance"] -= 100;  // Now: 900
  
  // And: Trigger compensation concurrently N times (simulate retries)
  std::vector<std::map<std::string, int>> compensation_results;
  std::mutex result_lock;
  
  auto run_compensation = [&](int retry_num) {
    auto result = saga_orchestrator_->compensateStep(saga_id, "debit", 100);
    std::lock_guard<std::mutex> lock(result_lock);
    compensation_results.push_back(result);
  };
  
  std::vector<std::thread> compensation_threads;
  for (int i = 0; i < n_retries; ++i) {
    compensation_threads.emplace_back(run_compensation, i);
  }
  
  for (auto& t : compensation_threads) {
    t.join();
  }

  // Then: All compensations should yield identical result (idempotent)
  EXPECT_EQ(compensation_results.size(), n_retries);
  
  for (int i = 1; i < compensation_results.size(); ++i) {
    EXPECT_EQ(compensation_results[i], compensation_results[0])
      << "Compensation result " << i << " should match result 0 (idempotent)";
  }
  EXPECT_EQ(compensation_results[0]["balance"], 1000)
    << "Balance should be restored to original value";
}

/// AC-10.1: Compensation Ordering — Reverse sequence (LIFO)
TEST_F(CompensationIdempotencyTest, CompensationOrdering_ReverseSequence) {
  // Given: Multi-step SAGA (debit → credit → log)
  uint64_t saga_id = 2002;
  std::vector<std::string> execution_log;
  std::mutex log_lock;

  // When: Execute steps in order
  auto step_1 = saga_orchestrator_->executeStep(saga_id, "step_1_debit", 100);
  auto step_2 = saga_orchestrator_->executeStep(saga_id, "step_2_credit", 100);
  auto step_3 = saga_orchestrator_->executeStep(saga_id, "step_3_audit", "OK");

  // And: Trigger abort (should compensate in reverse order)
  std::vector<std::string> compensation_order;
  saga_orchestrator_->setCompensationLogger([&](const std::string& step_name) {
    std::lock_guard<std::mutex> lock(log_lock);
    compensation_order.push_back(step_name);
  });

  saga_orchestrator_->abortTransaction(saga_id);

  // Then: Compensation order should be LIFO (3 → 2 → 1)
  EXPECT_EQ(compensation_order.size(), 3);
  EXPECT_EQ(compensation_order[0], "step_3_audit")
    << "First compensation should be step_3 (LIFO)";
  EXPECT_EQ(compensation_order[1], "step_2_credit")
    << "Second compensation should be step_2";
  EXPECT_EQ(compensation_order[2], "step_1_debit")
    << "Third compensation should be step_1";
}

/// AC-10.1: Rollback Chain — Cascading compensation under cascading failures
TEST_F(CompensationIdempotencyTest, RollbackChain_CascadingCompensation) {
  // Given: Complex SAGA with 5 dependent steps
  uint64_t saga_id = 2003;
  
  // When: Execute steps 1-3 successfully, step 4 fails
  saga_orchestrator_->executeStep(saga_id, "step_1", 100);
  saga_orchestrator_->executeStep(saga_id, "step_2", 100);
  saga_orchestrator_->executeStep(saga_id, "step_3", 100);
  
  // Step 4 fails → should trigger compensation of 1-3 in reverse
  auto step_4_result = saga_orchestrator_->executeStep(saga_id, "step_4_fails", 100, false);

  // Then: Compensation chain should complete without cascading failure
  int compensation_count = saga_orchestrator_->getCompensationStepCount(saga_id);
  EXPECT_EQ(compensation_count, 3)
    << "Should compensate 3 successful steps (1, 2, 3)";
  
  // Verify final state is consistent
  auto final_state = saga_orchestrator_->getTransactionState(saga_id);
  EXPECT_EQ(final_state, "ABORTED")
    << "Transaction should be in ABORTED state";
}

/// AC-10.1: Concurrent Compensation Calls — Race condition handling
TEST_F(CompensationIdempotencyTest, ConcurrentCompensationCalls_NoRaceConditions) {
  // Given: SAGA with single compensation step
  uint64_t saga_id = 2004;
  saga_orchestrator_->executeStep(saga_id, "transfer_funds", 100);

  // When: Trigger compensation from multiple threads simultaneously
  const int n_threads = 10;
  std::vector<bool> compensation_results(n_threads);
  std::vector<std::thread> threads;

  for (int i = 0; i < n_threads; ++i) {
    threads.emplace_back([&](int idx) {
      auto result = saga_orchestrator_->compensateStep(saga_id, "transfer_funds", 100);
      compensation_results[idx] = result.success;
    }, i);
  }

  for (auto& t : threads) {
    t.join();
  }

  // Then: All compensations should succeed (no race condition)
  for (int i = 0; i < n_threads; ++i) {
    EXPECT_TRUE(compensation_results[i])
      << "Compensation " << i << " should succeed (no race condition)";
  }
}

/// AC-10.1: Compensation Timeout — Partial compensation with timeout
TEST_F(CompensationIdempotencyTest, CompensationTimeout_PartialWithTimeout) {
  // Given: Slow compensation step (simulates timeout scenario)
  uint64_t saga_id = 2005;
  
  // When: Execute step then trigger slow compensation
  saga_orchestrator_->executeStep(saga_id, "slow_step", 100);
  
  auto start = std::chrono::steady_clock::now();
  auto result = saga_orchestrator_->compensateStepWithTimeout(saga_id, "slow_step", 100, 
                                                               std::chrono::milliseconds(500));
  auto elapsed = std::chrono::steady_clock::now() - start;

  // Then: Compensation should timeout gracefully
  EXPECT_FALSE(result.success)
    << "Slow compensation should timeout";
  EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 1000)
    << "Timeout should be enforced (≤1s for 500ms timeout)";
}

/// AC-10.1: Error Propagation — Errors consistent across retries
TEST_F(CompensationIdempotencyTest, ErrorPropagation_ConsistentAcrossRetries) {
  // Given: Failing compensation step
  uint64_t saga_id = 2006;
  
  // When: Retry same compensation 5 times
  std::vector<std::string> error_codes;
  for (int i = 0; i < 5; ++i) {
    auto result = saga_orchestrator_->compensateStep(saga_id, "failing_step", 100);
    error_codes.push_back(result.error_code);
  }

  // Then: All retries should report identical error code
  for (int i = 1; i < error_codes.size(); ++i) {
    EXPECT_EQ(error_codes[i], error_codes[0])
      << "Error code " << i << " should match error code 0 (consistent)";
  }
}

// ==============================================================================
// PARTIAL FAILURE TESTS (5) — Mixed Success/Failure Scenarios
// ==============================================================================

class PartialFailureTest : public ::testing::Test {
 protected:
  void SetUp() override {
    saga_orchestrator_ = std::make_unique<SAGAOrchestrator>();
  }

  void TearDown() override {
    saga_orchestrator_.reset();
  }

  std::unique_ptr<SAGAOrchestrator> saga_orchestrator_;
};

/// AC-9.2: Partial Failure — Some steps OK, some fail
TEST_F(PartialFailureTest, PartialFailure_MixedSuccessAndFailure) {
  // Given: SAGA with 4 remote steps
  uint64_t saga_id = 3001;
  
  // When: Execute steps with mixed outcomes
  auto step_1 = saga_orchestrator_->executeRemoteStep(saga_id, "step_1_ok", "{}", true);
  auto step_2 = saga_orchestrator_->executeRemoteStep(saga_id, "step_2_ok", "{}", true);
  auto step_3 = saga_orchestrator_->executeRemoteStep(saga_id, "step_3_fail", "{}", false);
  auto step_4 = saga_orchestrator_->executeRemoteStep(saga_id, "step_4_ok", "{}", true);

  // Then: Transaction should abort with partial completion
  auto state = saga_orchestrator_->getTransactionState(saga_id);
  EXPECT_EQ(state, "ABORTING")
    << "Transaction should be aborting after partial failure";
  
  // And: Compensate only steps 1-2 (reverse order from completion)
  int compensated = saga_orchestrator_->getCompensationStepCount(saga_id);
  EXPECT_EQ(compensated, 2)
    << "Should compensate 2 successful steps (1, 2)";
}

/// AC-9.2: Selective Compensation — Don't compensate failed steps
TEST_F(PartialFailureTest, SelectiveCompensation_SkipFailedSteps) {
  // Given: SAGA where step 2 fails
  uint64_t saga_id = 3002;
  
  saga_orchestrator_->executeRemoteStep(saga_id, "step_1", "{}", true);
  saga_orchestrator_->executeRemoteStep(saga_id, "step_2_fails", "{}", false);
  saga_orchestrator_->executeRemoteStep(saga_id, "step_3", "{}", true);

  // When: Get compensation steps (should skip step_2_fails)
  auto compensation_steps = saga_orchestrator_->getCompensationSteps(saga_id);

  // Then: Only steps 1 and 3 should be compensated
  EXPECT_EQ(compensation_steps.size(), 2)
    << "Should compensate 2 steps (skip the failed step_2)";
  
  bool has_failed_compensation = false;
  for (const auto& step : compensation_steps) {
    if (step == "step_2_fails") {
      has_failed_compensation = true;
    }
  }
  EXPECT_FALSE(has_failed_compensation)
    << "Failed step should not be compensated";
}

/// AC-9.2: Cascading Partial Failure — Failure in step 2 blocks step 3
TEST_F(PartialFailureTest, CascadingPartialFailure_StepBlockage) {
  // Given: Steps where success of step 2 is required for step 3
  uint64_t saga_id = 3003;
  
  // When: Step 1 succeeds, step 2 fails
  saga_orchestrator_->executeRemoteStep(saga_id, "step_1", "{}", true);
  auto step_2 = saga_orchestrator_->executeRemoteStep(saga_id, "step_2_fails", "{}", false);

  // Then: Step 3 should not be executed
  auto step_3_executed = saga_orchestrator_->isStepExecuted(saga_id, "step_3");
  EXPECT_FALSE(step_3_executed)
    << "Step 3 should not execute if step 2 failed (dependency)";
}

/// AC-9.2: Partial Failure with Timeout — Handle mixed timeouts + failures
TEST_F(PartialFailureTest, PartialFailureWithTimeout_MixedScenarios) {
  // Given: SAGA with timeout and failure scenarios
  uint64_t saga_id = 3004;
  
  saga_orchestrator_->executeRemoteStep(saga_id, "step_1", "{}", true);
  saga_orchestrator_->executeRemoteStepWithTimeout(saga_id, "step_2_timeout", "{}", 
                                                    std::chrono::milliseconds(100), false);
  saga_orchestrator_->executeRemoteStep(saga_id, "step_3", "{}", true);

  // Then: Transaction should handle mixed failure modes gracefully
  auto state = saga_orchestrator_->getTransactionState(saga_id);
  EXPECT_EQ(state, "ABORTING");
}

/// AC-9.2: Partial Failure Recovery — Allow retry from specific checkpoint
TEST_F(PartialFailureTest, PartialFailureRecovery_CheckpointRetry) {
  // Given: SAGA with checkpoint after step 2
  uint64_t saga_id = 3005;
  
  saga_orchestrator_->executeRemoteStep(saga_id, "step_1", "{}", true);
  saga_orchestrator_->executeRemoteStep(saga_id, "step_2", "{}", true);
  saga_orchestrator_->createCheckpoint(saga_id, "after_step_2");
  saga_orchestrator_->executeRemoteStep(saga_id, "step_3_fails", "{}", false);

  // When: Retry from checkpoint
  auto checkpoint_state = saga_orchestrator_->getCheckpointState(saga_id, "after_step_2");
  auto retry_result = saga_orchestrator_->retryFromCheckpoint(saga_id, "after_step_2");

  // Then: Should resume from checkpoint successfully
  EXPECT_TRUE(retry_result.success)
    << "Retry from checkpoint should succeed";
}

// ==============================================================================
// RETRY STORM SUPPRESSION TESTS (3) — Backoff + Jitter Validation
// ==============================================================================

class RetryStormTest : public ::testing::Test {
 protected:
  void SetUp() override {
    saga_orchestrator_ = std::make_unique<SAGAOrchestrator>();
  }

  void TearDown() override {
    saga_orchestrator_.reset();
  }

  std::unique_ptr<SAGAOrchestrator> saga_orchestrator_;
};

/// AC-10.2: Exponential Backoff — Base 100ms, factor 2x
TEST_F(RetryStormTest, ExponentialBackoff_BaseAndFactor) {
  // Given: Retry mechanism with exponential backoff (base=100ms, factor=2)
  
  // When: Measure retry delays
  std::vector<int> retry_delays_ms;
  for (int retry = 0; retry < 4; ++retry) {
    auto delay_ms = saga_orchestrator_->calculateRetryBackoffMs(retry, 100, 2.0);
    retry_delays_ms.push_back(delay_ms);
  }

  // Then: Delays should follow exponential pattern (100ms, 200ms, 400ms, 800ms)
  EXPECT_EQ(retry_delays_ms[0], 100)
    << "Retry 0 should have base delay (100ms)";
  EXPECT_EQ(retry_delays_ms[1], 200)
    << "Retry 1 should be 2x base (200ms)";
  EXPECT_EQ(retry_delays_ms[2], 400)
    << "Retry 2 should be 4x base (400ms)";
  EXPECT_EQ(retry_delays_ms[3], 800)
    << "Retry 3 should be 8x base (800ms)";
}

/// AC-10.2: Jitter Bounds — ±20% jitter on backoff
TEST_F(RetryStormTest, JitterBounds_PlusMinus20Percent) {
  // Given: Backoff with jitter (±20%)
  const int base_delay_ms = 100;
  const double jitter_factor = 0.2;  // ±20%
  
  // When: Apply jitter to delays multiple times
  std::vector<int> jittered_delays;
  for (int i = 0; i < 100; ++i) {
    auto delay = saga_orchestrator_->applyJitterToBackoff(base_delay_ms, jitter_factor);
    jittered_delays.push_back(delay);
  }

  // Then: All delays should be within ±20% of base
  for (int delay : jittered_delays) {
    int min_bound = static_cast<int>(base_delay_ms * (1.0 - jitter_factor));
    int max_bound = static_cast<int>(base_delay_ms * (1.0 + jitter_factor));
    
    EXPECT_GE(delay, min_bound)
      << "Jittered delay should be >= " << min_bound;
    EXPECT_LE(delay, max_bound)
      << "Jittered delay should be <= " << max_bound;
  }
}

/// AC-10.2: Bounded Retries — Max 3 retries (no infinite loops)
TEST_F(RetryStormTest, BoundedRetries_MaximumThreeRetries) {
  // Given: Retry limit of 3
  uint64_t saga_id = 4001;
  const int max_retries = 3;
  
  // When: Attempt retries beyond limit
  int retry_count = 0;
  for (int attempt = 0; attempt < 10; ++attempt) {
    auto result = saga_orchestrator_->retryWithBoundedAttempts(saga_id, "failing_step",
                                                                max_retries);
    if (result.success || !result.can_retry) {
      break;
    }
    retry_count++;
  }

  // Then: Retry count should not exceed maximum
  EXPECT_LE(retry_count, max_retries)
    << "Retry count should not exceed " << max_retries;
  
  // And: Should return error code indicating retry exhaustion
  auto final_result = saga_orchestrator_->getLastRetryResult(saga_id);
  EXPECT_TRUE(final_result.retries_exhausted)
    << "Should indicate retries exhausted";
}

}  // namespace themis::transaction::test
