/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_distributed_saga.cpp                          ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-13 04:38:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     632                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • ffeccdf868  2026-03-01  feat(transaction): implement DistributedSagaCoordinator f... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_distributed_saga.cpp
 * @brief Unit tests for DistributedSagaCoordinator
 *
 * Covers:
 *  - Validation (empty saga_id, duplicate names, unknown dependency, cycle)
 *  - Sequential execution (success path, partial failure + compensation)
 *  - DAG-based parallel execution
 *  - Retry on transient failures
 *  - Step timeouts
 *  - Compensation ordering (LIFO)
 *  - Steps without compensating actions (no-op skip)
 *  - Concurrent independent SAGAs
 *  - Metrics accumulation
 *  - getReport()
 */

#include <gtest/gtest.h>
#include "transaction/distributed_saga.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

using namespace themis;
using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static DistributedSagaStep makeStep(
    std::string name,
    std::function<DistributedSagaStatus()> forward,
    std::function<DistributedSagaStatus()> compensate = {},
    std::set<std::string> deps = {}
) {
    DistributedSagaStep s;
    s.name         = std::move(name);
    s.node_id      = "node-0";
    s.forward      = std::move(forward);
    s.compensate   = std::move(compensate);
    s.depends_on   = std::move(deps);
    s.max_retries  = 0;
    s.retry_backoff = 1ms;
    s.forward_timeout = 2000ms;
    s.compensate_timeout = 2000ms;
    return s;
}

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

class DistributedSagaTest : public ::testing::Test {
protected:
    DistributedSagaCoordinator coord;   // default config
};

// ─────────────────────────────────────────────────────────────────────────────
// Validation tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedSagaTest, ValidateEmptySagaId) {
    DistributedSagaDefinition def;
    def.saga_id = "";
    def.steps.push_back(makeStep("s1", []{ return DistributedSagaStatus::OK(); }));
    auto st = coord.validate(def);
    EXPECT_FALSE(st.ok);
    EXPECT_NE(st.message.find("saga_id"), std::string::npos);
}

TEST_F(DistributedSagaTest, ValidateNoSteps) {
    DistributedSagaDefinition def;
    def.saga_id = "test";
    auto st = coord.validate(def);
    EXPECT_FALSE(st.ok);
}

TEST_F(DistributedSagaTest, ValidateDuplicateStepName) {
    DistributedSagaDefinition def;
    def.saga_id = "test";
    def.steps.push_back(makeStep("dup", []{ return DistributedSagaStatus::OK(); }));
    def.steps.push_back(makeStep("dup", []{ return DistributedSagaStatus::OK(); }));
    auto st = coord.validate(def);
    EXPECT_FALSE(st.ok);
    EXPECT_NE(st.message.find("duplicate"), std::string::npos);
}

TEST_F(DistributedSagaTest, ValidateUnknownDependency) {
    DistributedSagaDefinition def;
    def.saga_id = "test";
    def.steps.push_back(makeStep("s1", []{ return DistributedSagaStatus::OK(); },
                                 {}, {"nonexistent"}));
    auto st = coord.validate(def);
    EXPECT_FALSE(st.ok);
    EXPECT_NE(st.message.find("unknown"), std::string::npos);
}

TEST_F(DistributedSagaTest, ValidateCycleDetected) {
    DistributedSagaDefinition def;
    def.saga_id = "cycle-test";
    // A → B → A
    def.steps.push_back(makeStep("A", []{ return DistributedSagaStatus::OK(); }, {}, {"B"}));
    def.steps.push_back(makeStep("B", []{ return DistributedSagaStatus::OK(); }, {}, {"A"}));
    auto st = coord.validate(def);
    EXPECT_FALSE(st.ok);
    EXPECT_NE(st.message.find("cycle"), std::string::npos);
}

TEST_F(DistributedSagaTest, ValidateNoForwardAction) {
    DistributedSagaDefinition def;
    def.saga_id = "test";
    DistributedSagaStep s;
    s.name = "no_forward";
    // forward intentionally left empty
    def.steps.push_back(s);
    auto st = coord.validate(def);
    EXPECT_FALSE(st.ok);
}

TEST_F(DistributedSagaTest, ValidateValidDefinition) {
    DistributedSagaDefinition def;
    def.saga_id = "valid";
    def.steps.push_back(makeStep("s1", []{ return DistributedSagaStatus::OK(); }));
    def.steps.push_back(makeStep("s2", []{ return DistributedSagaStatus::OK(); },
                                 {}, {"s1"}));
    auto st = coord.validate(def);
    EXPECT_TRUE(st.ok);
}

// ─────────────────────────────────────────────────────────────────────────────
// Sequential success path
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedSagaTest, SequentialAllStepsExecuted) {
    std::vector<int> order;
    std::mutex mtx;

    DistributedSagaDefinition def;
    def.saga_id = "seq-success";
    for (int i = 0; i < 5; ++i) {
        def.steps.push_back(makeStep(
            "s" + std::to_string(i),
            [i, &order, &mtx]() -> DistributedSagaStatus {
                std::lock_guard lk(mtx);
                order.push_back(i);
                return DistributedSagaStatus::OK();
            }
        ));
    }
    // Chain: s0 ← s1 ← s2 ← s3 ← s4
    for (int i = 1; i < 5; ++i) {
        def.steps[i].depends_on.insert("s" + std::to_string(i - 1));
    }

    auto report = coord.execute(def);

    EXPECT_TRUE(report.succeeded());
    EXPECT_EQ(report.state, SagaExecutionState::COMPLETED);
    EXPECT_EQ(order.size(), 5u);
    // steps must be ordered 0,1,2,3,4
    for (int i = 0; i < 5; ++i) EXPECT_EQ(order[i], i);
}

TEST_F(DistributedSagaTest, SingleStepSuccess) {
    std::atomic<int> executed{0};
    DistributedSagaDefinition def;
    def.saga_id = "single-step";
    def.steps.push_back(makeStep("only",
        [&executed]() -> DistributedSagaStatus {
            ++executed;
            return DistributedSagaStatus::OK();
        }
    ));

    auto report = coord.execute(def);

    EXPECT_TRUE(report.succeeded());
    EXPECT_EQ(executed.load(), 1);
    ASSERT_EQ(report.step_records.size(), 1u);
    EXPECT_EQ(report.step_records[0].phase, StepRecord::Phase::DONE);
}

// ─────────────────────────────────────────────────────────────────────────────
// Failure + compensation (LIFO order)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedSagaTest, CompensationRunsInReverseOrder) {
    std::vector<std::string> comp_order;
    std::mutex mtx;

    DistributedSagaDefinition def;
    def.saga_id = "comp-order";

    // s0 → s1 → s2 (s2 fails)
    def.steps.push_back(makeStep("s0",
        []{ return DistributedSagaStatus::OK(); },
        [&comp_order, &mtx]() -> DistributedSagaStatus {
            std::lock_guard lk(mtx);
            comp_order.push_back("comp_s0");
            return DistributedSagaStatus::OK();
        }
    ));
    def.steps.push_back(makeStep("s1",
        []{ return DistributedSagaStatus::OK(); },
        [&comp_order, &mtx]() -> DistributedSagaStatus {
            std::lock_guard lk(mtx);
            comp_order.push_back("comp_s1");
            return DistributedSagaStatus::OK();
        },
        {"s0"}
    ));
    def.steps.push_back(makeStep("s2",
        []{ return DistributedSagaStatus::Error("boom"); },
        [&comp_order, &mtx]() -> DistributedSagaStatus {
            std::lock_guard lk(mtx);
            comp_order.push_back("comp_s2"); // should NOT run (s2 never succeeded)
            return DistributedSagaStatus::OK();
        },
        {"s1"}
    ));

    auto report = coord.execute(def);

    EXPECT_FALSE(report.succeeded());
    EXPECT_EQ(report.state, SagaExecutionState::COMPENSATED);

    // Only s0 and s1 completed; s2 failed before finishing → compensate s1 then s0
    ASSERT_EQ(comp_order.size(), 2u);
    EXPECT_EQ(comp_order[0], "comp_s1");
    EXPECT_EQ(comp_order[1], "comp_s0");
}

TEST_F(DistributedSagaTest, CompensationSkippedForUncompletedStep) {
    std::atomic<int> comp_calls{0};

    DistributedSagaDefinition def;
    def.saga_id = "partial-comp";
    def.steps.push_back(makeStep("s0",
        []{ return DistributedSagaStatus::Error("immediate fail"); },
        [&comp_calls]() -> DistributedSagaStatus {
            ++comp_calls;
            return DistributedSagaStatus::OK();
        }
    ));

    auto report = coord.execute(def);

    EXPECT_FALSE(report.succeeded());
    // s0 never completed its forward action, so no compensation needed
    EXPECT_EQ(comp_calls.load(), 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Steps without compensation (no-op skip)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedSagaTest, StepWithoutCompensationSkipped) {
    std::atomic<int> s1_comp{0};

    DistributedSagaDefinition def;
    def.saga_id = "no-comp";

    // s0 has no compensation
    def.steps.push_back(makeStep("s0",
        []{ return DistributedSagaStatus::OK(); }
        // no compensate
    ));
    // s1 has compensation and fails
    DistributedSagaStep s1;
    s1.name     = "s1";
    s1.node_id  = "node-0";
    s1.forward  = []{ return DistributedSagaStatus::Error("fail"); };
    s1.compensate = [&s1_comp]() -> DistributedSagaStatus {
        ++s1_comp;
        return DistributedSagaStatus::OK();
    };
    s1.depends_on  = {"s0"};
    s1.max_retries = 0;
    s1.retry_backoff = 1ms;
    s1.forward_timeout = 2000ms;
    s1.compensate_timeout = 2000ms;
    def.steps.push_back(s1);

    auto report = coord.execute(def);

    EXPECT_FALSE(report.succeeded());
    EXPECT_EQ(report.state, SagaExecutionState::COMPENSATED);
    // s1 never completed → s1 comp not called; s0 has no comp → skipped
    EXPECT_EQ(s1_comp.load(), 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// DAG parallel execution
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedSagaTest, IndependentStepsRunInParallel) {
    // Two independent steps that each sleep 100ms.
    // If they run sequentially total > 200ms; in parallel total ~100ms.
    DistributedSagaCoordinator::Config cfg;
    cfg.enable_parallel = true;
    DistributedSagaCoordinator par_coord(cfg);

    DistributedSagaDefinition def;
    def.saga_id = "parallel-test";
    for (int i = 0; i < 4; ++i) {
        def.steps.push_back(makeStep("s" + std::to_string(i),
            []() -> DistributedSagaStatus {
                std::this_thread::sleep_for(50ms);
                return DistributedSagaStatus::OK();
            }
        ));
    }
    // s0, s1, s2, s3 are all independent (no dependencies)

    auto t0     = std::chrono::steady_clock::now();
    auto report = par_coord.execute(def);
    auto elapsed = std::chrono::steady_clock::now() - t0;

    EXPECT_TRUE(report.succeeded());
    // Should finish well under 4×50ms = 200ms if parallelised (allow 3× margin)
    EXPECT_LT(elapsed, 150ms);
}

TEST_F(DistributedSagaTest, DiamondDependencyCorrectOrder) {
    //  A → B → D
    //  A → C → D
    std::vector<std::string> exec_order;
    std::mutex mtx;

    DistributedSagaDefinition def;
    def.saga_id = "diamond";

    auto step = [&](std::string name, std::set<std::string> deps)
    {
        return makeStep(name,
            [name, &exec_order, &mtx]() -> DistributedSagaStatus {
                std::lock_guard lk(mtx);
                exec_order.push_back(name);
                return DistributedSagaStatus::OK();
            },
            {},
            deps
        );
    };

    def.steps.push_back(step("A", {}));
    def.steps.push_back(step("B", {"A"}));
    def.steps.push_back(step("C", {"A"}));
    def.steps.push_back(step("D", {"B", "C"}));

    auto report = coord.execute(def);

    EXPECT_TRUE(report.succeeded());
    // A must be first, D must be last
    ASSERT_GE(exec_order.size(), 4u);
    EXPECT_EQ(exec_order.front(), "A");
    EXPECT_EQ(exec_order.back(),  "D");
}

// ─────────────────────────────────────────────────────────────────────────────
// Retry on transient failures
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedSagaTest, RetryTransientFailureSucceeds) {
    std::atomic<int> attempt_count{0};

    DistributedSagaStep step;
    step.name     = "flaky";
    step.node_id  = "node-0";
    step.forward  = [&attempt_count]() -> DistributedSagaStatus {
        int c = ++attempt_count;
        if (c < 3) return DistributedSagaStatus::Error("transient");
        return DistributedSagaStatus::OK();
    };
    step.max_retries     = 4;
    step.retry_backoff   = 1ms;
    step.forward_timeout = 2000ms;
    step.compensate_timeout = 2000ms;

    DistributedSagaDefinition def;
    def.saga_id = "retry-transient";
    def.steps.push_back(step);

    auto report = coord.execute(def);

    EXPECT_TRUE(report.succeeded());
    EXPECT_GE(attempt_count.load(), 3);
}

TEST_F(DistributedSagaTest, RetryExhaustedLeadsToCompensation) {
    std::atomic<int> attempt_count{0};

    DistributedSagaStep step;
    step.name     = "always-fail";
    step.node_id  = "node-0";
    step.forward  = [&attempt_count]() -> DistributedSagaStatus {
        ++attempt_count;
        return DistributedSagaStatus::Error("permanent");
    };
    step.max_retries     = 2;
    step.retry_backoff   = 1ms;
    step.forward_timeout = 2000ms;
    step.compensate_timeout = 2000ms;

    DistributedSagaDefinition def;
    def.saga_id = "retry-exhausted";
    def.steps.push_back(step);

    auto report = coord.execute(def);

    EXPECT_FALSE(report.succeeded());
    // 1 original + 2 retries = 3 total attempts
    EXPECT_EQ(attempt_count.load(), 3);
}

// ─────────────────────────────────────────────────────────────────────────────
// Timeout handling
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedSagaTest, StepTimeoutLeadsToFailure) {
    DistributedSagaStep step;
    step.name     = "slow";
    step.node_id  = "node-0";
    step.forward  = []() -> DistributedSagaStatus {
        std::this_thread::sleep_for(500ms); // long sleep
        return DistributedSagaStatus::OK();
    };
    step.max_retries     = 0;
    step.forward_timeout = 50ms; // very short
    step.compensate_timeout = 2000ms;

    DistributedSagaDefinition def;
    def.saga_id = "timeout-test";
    def.steps.push_back(step);

    auto report = coord.execute(def);

    EXPECT_FALSE(report.succeeded());
    EXPECT_EQ(report.state, SagaExecutionState::COMPENSATED);

    auto m = coord.getMetrics();
    EXPECT_GE(m.total_timeout_aborts, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Compensation failure → FAILED state
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedSagaTest, CompensationFailureLeadsToFailedState) {
    DistributedSagaDefinition def;
    def.saga_id = "comp-failure";

    // s0 succeeds but its compensation always throws
    def.steps.push_back(makeStep("s0",
        []{ return DistributedSagaStatus::OK(); },
        []() -> DistributedSagaStatus {
            return DistributedSagaStatus::Error("comp failed permanently");
        }
    ));
    // s1 fails, triggering compensation of s0
    def.steps.push_back(makeStep("s1",
        []{ return DistributedSagaStatus::Error("trigger"); },
        {},
        {"s0"}
    ));

    auto report = coord.execute(def);

    EXPECT_FALSE(report.succeeded());
    EXPECT_EQ(report.state, SagaExecutionState::FAILED);
}

// ─────────────────────────────────────────────────────────────────────────────
// getReport()
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedSagaTest, GetReportUnknownSagaReturnsNullopt) {
    auto r = coord.getReport("does-not-exist");
    EXPECT_FALSE(r.has_value());
}

TEST_F(DistributedSagaTest, GetReportAfterExecutionReturnsReport) {
    DistributedSagaDefinition def;
    def.saga_id = "storable";
    def.steps.push_back(makeStep("s", []{ return DistributedSagaStatus::OK(); }));

    coord.execute(def);

    auto r = coord.getReport("storable");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->state, SagaExecutionState::COMPLETED);
}

// ─────────────────────────────────────────────────────────────────────────────
// Metrics
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedSagaTest, MetricsCountSuccessAndCompensation) {
    // Run 2 successful SAGAs
    for (int i = 0; i < 2; ++i) {
        DistributedSagaDefinition def;
        def.saga_id = "ok-" + std::to_string(i);
        def.steps.push_back(makeStep("s", []{ return DistributedSagaStatus::OK(); }));
        coord.execute(def);
    }

    // Run 1 compensating SAGA
    {
        DistributedSagaDefinition def;
        def.saga_id = "fail-1";
        def.steps.push_back(makeStep("s0", []{ return DistributedSagaStatus::OK(); },
                                     []{ return DistributedSagaStatus::OK(); }));
        def.steps.push_back(makeStep("s1", []{ return DistributedSagaStatus::Error("x"); },
                                     {}, {"s0"}));
        coord.execute(def);
    }

    auto m = coord.getMetrics();
    EXPECT_EQ(m.sagas_started,     3u);
    EXPECT_EQ(m.sagas_completed,   2u);
    EXPECT_EQ(m.sagas_compensated, 1u);
    EXPECT_EQ(m.sagas_failed,      0u);
    EXPECT_GE(m.total_compensations, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Concurrent independent SAGAs
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedSagaTest, ConcurrentSagasAllSucceed) {
    constexpr int N = 20;
    std::atomic<int> success_count{0};
    std::vector<std::thread> threads;
    threads.reserve(N);

    for (int i = 0; i < N; ++i) {
        threads.emplace_back([this, i, &success_count]() {
            DistributedSagaDefinition def;
            def.saga_id = "concurrent-" + std::to_string(i);
            def.steps.push_back(makeStep("s",
                []() -> DistributedSagaStatus {
                    std::this_thread::sleep_for(5ms);
                    return DistributedSagaStatus::OK();
                }
            ));
            auto report = coord.execute(def);
            if (report.succeeded()) ++success_count;
        });
    }

    for (auto& t : threads) t.join();

    EXPECT_EQ(success_count.load(), N);
}

// ─────────────────────────────────────────────────────────────────────────────
// Step record fields are populated
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedSagaTest, StepRecordFieldsPopulated) {
    DistributedSagaDefinition def;
    def.saga_id = "step-record";
    def.steps.push_back(makeStep("work", []{ return DistributedSagaStatus::OK(); }));

    auto report = coord.execute(def);

    ASSERT_EQ(report.step_records.size(), 1u);
    const auto& rec = report.step_records[0];
    EXPECT_EQ(rec.name, "work");
    EXPECT_EQ(rec.phase, StepRecord::Phase::DONE);
    EXPECT_EQ(rec.attempts, 1u);
    EXPECT_TRUE(rec.error_message.empty());
    EXPECT_GE(report.total_duration_ms, 0);
}

TEST_F(DistributedSagaTest, FailedStepRecordHasErrorMessage) {
    DistributedSagaDefinition def;
    def.saga_id = "failed-record";
    def.steps.push_back(makeStep("bad",
        []{ return DistributedSagaStatus::Error("test-error"); }
    ));

    auto report = coord.execute(def);

    ASSERT_EQ(report.step_records.size(), 1u);
    const auto& rec = report.step_records[0];
    EXPECT_EQ(rec.phase, StepRecord::Phase::FAILED);
    EXPECT_FALSE(rec.error_message.empty());
    EXPECT_NE(rec.error_message.find("test-error"), std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// Execute returns FAILED state on validation error
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedSagaTest, ExecuteInvalidSagaReturnsFailedState) {
    DistributedSagaDefinition def;
    def.saga_id = "";  // invalid
    def.steps.push_back(makeStep("s", []{ return DistributedSagaStatus::OK(); }));

    auto report = coord.execute(def);

    EXPECT_FALSE(report.succeeded());
    EXPECT_EQ(report.state, SagaExecutionState::FAILED);
}
