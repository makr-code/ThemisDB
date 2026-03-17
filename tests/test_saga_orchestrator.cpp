/**
 * @file test_saga_orchestrator.cpp
 * @brief Unit tests for SAGAOrchestrator — v1.8.0
 *
 * Acceptance criteria covered:
 *  AC-1  Parallel step execution (DAG-based)
 *  AC-2  Conditional branching (steps with/without compensation)
 *  AC-3  Retry policies per step
 *  AC-4  Timeout management
 *  AC-5  SAGA templates (registerTemplate / getTemplate)
 *  AC-6  Automatic dependency resolution
 *  AC-7  Compensation runs in reverse execution order (LIFO)
 *  AC-8  validate() rejects empty name
 *  AC-9  validate() rejects empty steps
 *  AC-10 validate() rejects duplicate step names
 *  AC-11 validate() rejects unknown depends_on reference
 *  AC-12 validate() detects dependency cycles
 *  AC-13 Single-step SAGA success path
 *  AC-14 Multi-step sequential SAGA success path
 *  AC-15 Multi-step parallel SAGA success path (DAG wave execution)
 *  AC-16 Step failure triggers compensation of preceding steps
 *  AC-17 Step without compensate is skipped during compensation
 *  AC-18 Retry retries up to max_retries times before failing
 *  AC-19 Step timeout fires and returns error
 *  AC-20 Metrics: sagas_started / sagas_completed / sagas_failed / sagas_compensated
 *  AC-21 Metrics: total_step_executions, total_retries, total_timeout_aborts
 *  AC-22 getStatus() returns per-step states after execution
 *  AC-23 getStatus() returns empty struct for unknown saga_id
 *  AC-24 Concurrent independent SAGAs all complete successfully
 *  AC-25 Performance: parallel SAGA is faster than sequential equivalent
 */

#include <gtest/gtest.h>
#include "transaction/saga_orchestrator.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace themis;
using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static SAGAOrchestrator::Step makeStep(
    std::string                    name,
    std::function<void()>          forward,
    std::function<void()>          compensate = {},
    std::set<std::string>          deps       = {},
    std::chrono::milliseconds      timeout    = 2000ms,
    size_t                         max_retries = 0,
    std::chrono::milliseconds      retry_delay = 1ms
) {
    SAGAOrchestrator::Step s;
    s.name        = std::move(name);
    s.forward     = std::move(forward);
    s.compensate  = std::move(compensate);
    s.depends_on  = std::move(deps);
    s.timeout     = timeout;
    s.max_retries = max_retries;
    s.retry_delay = retry_delay;
    return s;
}

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

class SAGAOrchestratorTest : public ::testing::Test {
protected:
    SAGAOrchestrator orch;
};

// ─────────────────────────────────────────────────────────────────────────────
// AC-8..12  Validation tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SAGAOrchestratorTest, Validate_EmptyName) {
    SAGAOrchestrator::SAGADefinition def;
    def.name = "";
    def.steps.push_back(makeStep("s1", []{}));
    auto st = orch.validate(def);
    EXPECT_FALSE(st.ok);
    EXPECT_NE(st.message.find("name"), std::string::npos);
}

TEST_F(SAGAOrchestratorTest, Validate_NoSteps) {
    SAGAOrchestrator::SAGADefinition def;
    def.name = "empty";
    auto st = orch.validate(def);
    EXPECT_FALSE(st.ok);
    EXPECT_NE(st.message.find("step"), std::string::npos);
}

TEST_F(SAGAOrchestratorTest, Validate_DuplicateStepName) {
    SAGAOrchestrator::SAGADefinition def;
    def.name = "dup";
    def.steps.push_back(makeStep("s1", []{}));
    def.steps.push_back(makeStep("s1", []{}));
    auto st = orch.validate(def);
    EXPECT_FALSE(st.ok);
    EXPECT_NE(st.message.find("duplicate"), std::string::npos);
}

TEST_F(SAGAOrchestratorTest, Validate_UnknownDependency) {
    SAGAOrchestrator::SAGADefinition def;
    def.name = "bad_dep";
    def.steps.push_back(makeStep("s1", []{}, {}, {"ghost"}));
    auto st = orch.validate(def);
    EXPECT_FALSE(st.ok);
    EXPECT_NE(st.message.find("ghost"), std::string::npos);
}

TEST_F(SAGAOrchestratorTest, Validate_CycleDetected) {
    SAGAOrchestrator::SAGADefinition def;
    def.name = "cycle";
    def.steps.push_back(makeStep("a", []{}, {}, {"b"}));
    def.steps.push_back(makeStep("b", []{}, {}, {"a"}));
    auto st = orch.validate(def);
    EXPECT_FALSE(st.ok);
    EXPECT_NE(st.message.find("cycle"), std::string::npos);
}

TEST_F(SAGAOrchestratorTest, Validate_StepWithNoForwardAction) {
    SAGAOrchestrator::SAGADefinition def;
    def.name = "no_fwd";
    SAGAOrchestrator::Step s;
    s.name    = "step1";
    // forward left empty (null)
    def.steps.push_back(s);
    auto st = orch.validate(def);
    EXPECT_FALSE(st.ok);
    EXPECT_NE(st.message.find("forward"), std::string::npos);
}

TEST_F(SAGAOrchestratorTest, Validate_ValidDefinitionReturnsOk) {
    SAGAOrchestrator::SAGADefinition def;
    def.name = "valid";
    def.steps.push_back(makeStep("s1", []{}));
    def.steps.push_back(makeStep("s2", []{}, {}, {"s1"}));
    auto st = orch.validate(def);
    EXPECT_TRUE(st.ok);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-13  Single-step SAGA success
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SAGAOrchestratorTest, SingleStep_Success) {
    bool ran = false;
    SAGAOrchestrator::SAGADefinition def;
    def.name = "single";
    def.steps.push_back(makeStep("s1", [&ran]{ ran = true; }));

    auto st = orch.execute(def);
    EXPECT_TRUE(st.ok);
    EXPECT_TRUE(ran);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-14  Multi-step sequential SAGA success
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SAGAOrchestratorTest, MultiStep_Sequential_Success) {
    std::vector<int> order;
    std::mutex mu;

    SAGAOrchestrator::SAGADefinition def;
    def.name           = "sequential";
    def.enable_parallel = false;

    for (int i = 0; i < 3; ++i) {
        def.steps.push_back(makeStep("s" + std::to_string(i),
            [i, &order, &mu]{ std::lock_guard<std::mutex> lk(mu); order.push_back(i); }));
    }

    auto st = orch.execute(def);
    EXPECT_TRUE(st.ok);
    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], 0);
    EXPECT_EQ(order[1], 1);
    EXPECT_EQ(order[2], 2);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-15  Parallel SAGA success — independent steps run in parallel
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SAGAOrchestratorTest, Parallel_IndependentSteps_AllComplete) {
    std::atomic<int> completed{0};

    SAGAOrchestrator::SAGADefinition def;
    def.name           = "parallel";
    def.enable_parallel = true;

    for (int i = 0; i < 4; ++i) {
        def.steps.push_back(makeStep("s" + std::to_string(i),
            [&completed]{ ++completed; }));
    }

    auto st = orch.execute(def);
    EXPECT_TRUE(st.ok);
    EXPECT_EQ(completed.load(), 4);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-1  DAG-based parallel execution — dependency ordering enforced
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SAGAOrchestratorTest, DAG_DependencyOrder_Enforced) {
    // reserve_inventory and validate_customer run first (wave 0)
    // charge_payment runs after both complete (wave 1)
    std::vector<std::string> execution_order;
    std::mutex mu;

    SAGAOrchestrator::SAGADefinition def;
    def.name           = "dag_order";
    def.enable_parallel = true;

    def.steps.push_back(makeStep("reserve_inventory",
        [&]{ std::lock_guard<std::mutex> lk(mu); execution_order.push_back("reserve_inventory"); }));
    def.steps.push_back(makeStep("validate_customer",
        [&]{ std::lock_guard<std::mutex> lk(mu); execution_order.push_back("validate_customer"); }));
    def.steps.push_back(makeStep("charge_payment",
        [&]{ std::lock_guard<std::mutex> lk(mu); execution_order.push_back("charge_payment"); },
        {}, {"reserve_inventory", "validate_customer"}));

    auto st = orch.execute(def);
    EXPECT_TRUE(st.ok);
    ASSERT_EQ(execution_order.size(), 3u);
    // charge_payment must be last
    EXPECT_EQ(execution_order.back(), "charge_payment");
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-16  Step failure → compensation of preceding steps
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SAGAOrchestratorTest, StepFailure_TriggersCompensation) {
    std::vector<std::string> comp_log;
    std::mutex mu;

    SAGAOrchestrator::SAGADefinition def;
    def.name           = "compensate_test";
    def.enable_parallel = false;

    def.steps.push_back(makeStep("s1",
        []{},
        [&]{ std::lock_guard<std::mutex> lk(mu); comp_log.push_back("comp_s1"); }));
    def.steps.push_back(makeStep("s2",
        []{},
        [&]{ std::lock_guard<std::mutex> lk(mu); comp_log.push_back("comp_s2"); }));
    def.steps.push_back(makeStep("s3",
        []{ throw std::runtime_error("step 3 failed"); },
        [&]{ std::lock_guard<std::mutex> lk(mu); comp_log.push_back("comp_s3"); }));

    auto st = orch.execute(def);
    EXPECT_FALSE(st.ok);
    EXPECT_NE(st.message.find("step 3 failed"), std::string::npos);

    // s1 and s2 completed → compensated in reverse (s2 then s1)
    ASSERT_EQ(comp_log.size(), 2u);
    EXPECT_EQ(comp_log[0], "comp_s2");
    EXPECT_EQ(comp_log[1], "comp_s1");
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-7  Compensation runs in reverse execution order (LIFO)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SAGAOrchestratorTest, Compensation_ReverseOrder) {
    std::vector<int> comp_order;
    std::mutex mu;

    SAGAOrchestrator::SAGADefinition def;
    def.name           = "reverse_comp";
    def.enable_parallel = false;

    for (int i = 0; i < 4; ++i) {
        def.steps.push_back(makeStep(
            "s" + std::to_string(i),
            []{},
            [i, &comp_order, &mu]{ std::lock_guard<std::mutex> lk(mu); comp_order.push_back(i); }));
    }
    // Last step fails
    def.steps.push_back(makeStep("fail",
        []{ throw std::runtime_error("forced failure"); }));

    auto st = orch.execute(def);
    EXPECT_FALSE(st.ok);

    ASSERT_EQ(comp_order.size(), 4u);
    EXPECT_EQ(comp_order[0], 3);
    EXPECT_EQ(comp_order[1], 2);
    EXPECT_EQ(comp_order[2], 1);
    EXPECT_EQ(comp_order[3], 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-17  Step without compensate is skipped during compensation
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SAGAOrchestratorTest, Compensation_NoCompensateFn_IsSkipped) {
    std::atomic<int> comp_calls{0};

    SAGAOrchestrator::SAGADefinition def;
    def.name           = "no_comp";
    def.enable_parallel = false;

    // Step with compensate
    def.steps.push_back(makeStep("s1", []{},
        [&comp_calls]{ ++comp_calls; }));
    // Step without compensate
    def.steps.push_back(makeStep("s2", []{}, {}));
    // Failing step
    def.steps.push_back(makeStep("s3", []{ throw std::runtime_error("fail"); }));

    auto st = orch.execute(def);
    EXPECT_FALSE(st.ok);
    // Only s1 has a compensate function
    EXPECT_EQ(comp_calls.load(), 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-3  Retry policies per step
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SAGAOrchestratorTest, RetryPolicy_SucceedsAfterRetries) {
    std::atomic<int> calls{0};

    SAGAOrchestrator::SAGADefinition def;
    def.name = "retry_success";
    def.steps.push_back(makeStep("flaky", [&calls]{
        int n = ++calls;
        if (n < 3) throw std::runtime_error("transient");
    }, {}, {}, 2000ms, /*max_retries=*/3, /*retry_delay=*/1ms));

    auto st = orch.execute(def);
    EXPECT_TRUE(st.ok);
    EXPECT_EQ(calls.load(), 3); // failed twice, succeeded on 3rd
}

TEST_F(SAGAOrchestratorTest, RetryPolicy_ExhaustsRetriesAndFails) {
    std::atomic<int> calls{0};

    SAGAOrchestrator::SAGADefinition def;
    def.name = "retry_fail";
    def.steps.push_back(makeStep("always_fail", [&calls]{
        ++calls;
        throw std::runtime_error("always bad");
    }, {}, {}, 2000ms, /*max_retries=*/2, /*retry_delay=*/1ms));

    auto st = orch.execute(def);
    EXPECT_FALSE(st.ok);
    EXPECT_NE(st.message.find("always bad"), std::string::npos);
    EXPECT_EQ(calls.load(), 3); // 1 initial + 2 retries
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-4  Timeout management
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SAGAOrchestratorTest, Timeout_StepExceedsDeadline_Fails) {
    SAGAOrchestrator::SAGADefinition def;
    def.name = "timeout_test";
    def.steps.push_back(makeStep("slow", []{
        std::this_thread::sleep_for(500ms);
    }, {}, {}, /*timeout=*/50ms, /*max_retries=*/0));

    auto st = orch.execute(def);
    EXPECT_FALSE(st.ok);
    EXPECT_NE(st.message.find("timed out"), std::string::npos);
}

TEST_F(SAGAOrchestratorTest, Timeout_StepWithinDeadline_Succeeds) {
    SAGAOrchestrator::SAGADefinition def;
    def.name = "within_timeout";
    def.steps.push_back(makeStep("fast", []{
        std::this_thread::sleep_for(10ms);
    }, {}, {}, /*timeout=*/2000ms, /*max_retries=*/0));

    auto st = orch.execute(def);
    EXPECT_TRUE(st.ok);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-5  SAGA templates
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SAGAOrchestratorTest, Template_RegisterAndRetrieve) {
    SAGAOrchestrator::SAGADefinition templ;
    templ.name = "order_template";
    templ.steps.push_back(makeStep("step1", []{}));
    templ.steps.push_back(makeStep("step2", []{}, {}, {"step1"}));

    orch.registerTemplate(templ);

    auto retrieved = orch.getTemplate("order_template");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->name, "order_template");
    EXPECT_EQ(retrieved->steps.size(), 2u);
}

TEST_F(SAGAOrchestratorTest, Template_UnknownName_ReturnsNullopt) {
    auto t = orch.getTemplate("nonexistent");
    EXPECT_FALSE(t.has_value());
}

TEST_F(SAGAOrchestratorTest, Template_ExecuteFromTemplate) {
    std::atomic<int> ran{0};

    SAGAOrchestrator::SAGADefinition templ;
    templ.name = "reusable";
    templ.steps.push_back(makeStep("a", [&ran]{ ++ran; }));
    templ.steps.push_back(makeStep("b", [&ran]{ ++ran; }, {}, {"a"}));
    orch.registerTemplate(templ);

    // Retrieve and execute the template
    auto def = orch.getTemplate("reusable").value();
    auto st  = orch.execute(def);
    EXPECT_TRUE(st.ok);
    EXPECT_EQ(ran.load(), 2);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-6  Automatic dependency resolution — 3-level DAG
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SAGAOrchestratorTest, DAG_ThreeLevelDiamond_CorrectOrder) {
    // A → B, A → C, B → D, C → D  (diamond)
    std::vector<std::string> exec_order;
    std::mutex mu;

    auto record = [&](std::string name) -> std::function<void()> {
        return [&mu, &exec_order, name = std::move(name)]{
            std::lock_guard<std::mutex> lk(mu);
            exec_order.push_back(name);
        };
    };

    SAGAOrchestrator::SAGADefinition def;
    def.name           = "diamond";
    def.enable_parallel = true;

    def.steps.push_back(makeStep("A", record("A")));
    def.steps.push_back(makeStep("B", record("B"), {}, {"A"}));
    def.steps.push_back(makeStep("C", record("C"), {}, {"A"}));
    def.steps.push_back(makeStep("D", record("D"), {}, {"B", "C"}));

    auto st = orch.execute(def);
    EXPECT_TRUE(st.ok);
    ASSERT_EQ(exec_order.size(), 4u);

    // A must come first, D must come last
    EXPECT_EQ(exec_order.front(), "A");
    EXPECT_EQ(exec_order.back(), "D");
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-22  getStatus() returns correct per-step states after success
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SAGAOrchestratorTest, GetStatus_AfterSuccess_AllCompleted) {
    SAGAOrchestrator::SAGADefinition def;
    def.name = "status_check";
    def.steps.push_back(makeStep("s1", []{}));
    def.steps.push_back(makeStep("s2", []{}, {}, {"s1"}));

    auto st = orch.execute(def);
    EXPECT_TRUE(st.ok);

    auto status = orch.getStatus("status_check");
    EXPECT_EQ(status.saga_name, "status_check");
    EXPECT_EQ(status.completed_steps, 2u);
    EXPECT_EQ(status.failed_steps, 0u);
    EXPECT_EQ(status.step_states.at("s1"), StepState::COMPLETED);
    EXPECT_EQ(status.step_states.at("s2"), StepState::COMPLETED);
}

TEST_F(SAGAOrchestratorTest, GetStatus_AfterFailure_ShowsFailedAndCompensated) {
    SAGAOrchestrator::SAGADefinition def;
    def.name           = "status_fail";
    def.enable_parallel = false;
    def.steps.push_back(makeStep("s1", []{},
        []{ /* compensate */ }));
    def.steps.push_back(makeStep("s2",
        []{ throw std::runtime_error("forced"); }));

    auto st = orch.execute(def);
    EXPECT_FALSE(st.ok);

    auto status = orch.getStatus("status_fail");
    EXPECT_EQ(status.failed_steps, 1u);
    EXPECT_EQ(status.step_states.at("s1"), StepState::COMPENSATED);
    EXPECT_EQ(status.step_states.at("s2"), StepState::FAILED);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-23  getStatus() for unknown saga_id returns empty struct
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SAGAOrchestratorTest, GetStatus_UnknownId_ReturnsEmpty) {
    auto status = orch.getStatus("does_not_exist");
    EXPECT_TRUE(status.saga_name.empty());
    EXPECT_EQ(status.completed_steps, 0u);
    EXPECT_EQ(status.failed_steps, 0u);
    EXPECT_TRUE(status.step_states.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-20/21  Metrics accumulation
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SAGAOrchestratorTest, Metrics_SuccessfulSagaIncrementsCounters) {
    SAGAOrchestrator::SAGADefinition def;
    def.name = "metrics_ok";
    def.steps.push_back(makeStep("s1", []{}));

    orch.execute(def);

    auto m = orch.getMetrics();
    EXPECT_GE(m.sagas_started, 1u);
    EXPECT_GE(m.sagas_completed, 1u);
    EXPECT_GE(m.total_step_executions, 1u);
}

TEST_F(SAGAOrchestratorTest, Metrics_FailedSagaIncrementsFailureCounter) {
    SAGAOrchestrator::SAGADefinition def;
    def.name = "metrics_fail";
    def.steps.push_back(makeStep("s1", []{ throw std::runtime_error("x"); }));

    orch.execute(def);

    auto m = orch.getMetrics();
    EXPECT_GE(m.sagas_failed, 1u);
}

TEST_F(SAGAOrchestratorTest, Metrics_CompensatedSagaCountedSeparately) {
    SAGAOrchestrator::SAGADefinition def;
    def.name           = "metrics_comp";
    def.enable_parallel = false;
    def.steps.push_back(makeStep("s1", []{}, []{}));
    def.steps.push_back(makeStep("s2", []{ throw std::runtime_error("x"); }));

    orch.execute(def);

    auto m = orch.getMetrics();
    EXPECT_GE(m.sagas_compensated, 1u);
    EXPECT_GE(m.total_compensations, 1u);
}

TEST_F(SAGAOrchestratorTest, Metrics_Retries_Counted) {
    std::atomic<int> n{0};

    SAGAOrchestrator::SAGADefinition def;
    def.name = "metrics_retry";
    def.steps.push_back(makeStep("flaky", [&n]{
        if (++n < 3) throw std::runtime_error("transient");
    }, {}, {}, 2000ms, /*max_retries=*/3, /*retry_delay=*/1ms));

    orch.execute(def);

    auto m = orch.getMetrics();
    EXPECT_GE(m.total_retries, 2u);
}

TEST_F(SAGAOrchestratorTest, Metrics_TimeoutAborts_Counted) {
    SAGAOrchestrator::SAGADefinition def;
    def.name = "metrics_timeout";
    def.steps.push_back(makeStep("slow", []{
        std::this_thread::sleep_for(500ms);
    }, {}, {}, /*timeout=*/50ms));

    orch.execute(def);

    auto m = orch.getMetrics();
    EXPECT_GE(m.total_timeout_aborts, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-24  Concurrent independent SAGAs
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SAGAOrchestratorTest, ConcurrentSAGAs_AllComplete) {
    constexpr int kSAGAs = 8;
    std::atomic<int> total_steps{0};
    std::vector<std::thread> threads;
    std::atomic<int> successes{0};

    for (int i = 0; i < kSAGAs; ++i) {
        threads.emplace_back([this, i, &total_steps, &successes] {
            SAGAOrchestrator::SAGADefinition def;
            def.name = "concurrent_" + std::to_string(i);
            def.steps.push_back(makeStep("s1", [&total_steps]{ ++total_steps; }));
            def.steps.push_back(makeStep("s2", [&total_steps]{ ++total_steps; }, {}, {"s1"}));

            auto st = orch.execute(def);
            if (st.ok) ++successes;
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_EQ(successes.load(), kSAGAs);
    EXPECT_EQ(total_steps.load(), kSAGAs * 2);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-25  Performance: parallel SAGA faster than sequential equivalent
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SAGAOrchestratorTest, Performance_ParallelFasterThanSequential) {
    // 4 independent steps, each sleeping 50 ms.
    // Sequential ≈ 200 ms; parallel ≈ 50 ms.  Threshold: 150 ms.
    constexpr int kSteps       = 4;
    constexpr int kSleepMs     = 50;
    constexpr int kThresholdMs = 150;

    auto buildDef = [&](bool parallel, const std::string& name) {
        SAGAOrchestrator::SAGADefinition def;
        def.name           = name;
        def.enable_parallel = parallel;
        for (int i = 0; i < kSteps; ++i) {
            def.steps.push_back(makeStep("s" + std::to_string(i), [kSleepMs]{
                std::this_thread::sleep_for(std::chrono::milliseconds(kSleepMs));
            }));
        }
        return def;
    };

    // ── Parallel ──
    SAGAOrchestrator o1;
    auto t0 = std::chrono::steady_clock::now();
    o1.execute(buildDef(true, "perf_par"));
    auto par_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();

    // ── Sequential ──
    SAGAOrchestrator o2;
    auto t1 = std::chrono::steady_clock::now();
    o2.execute(buildDef(false, "perf_seq"));
    auto seq_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t1).count();

    EXPECT_LT(par_ms, kThresholdMs)
        << "Parallel SAGA took " << par_ms << " ms (expected < " << kThresholdMs << " ms)";
    EXPECT_GT(seq_ms, par_ms)
        << "Sequential (" << seq_ms << " ms) should be slower than parallel (" << par_ms << " ms)";
}

// ─────────────────────────────────────────────────────────────────────────────
// Validate_ValidDefinitionWithChain — chain of 3 serial dependencies
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SAGAOrchestratorTest, ChainedDependencies_Success) {
    std::vector<int> order;
    std::mutex mu;

    SAGAOrchestrator::SAGADefinition def;
    def.name           = "chain";
    def.enable_parallel = true; // parallel enabled, but steps form a chain

    def.steps.push_back(makeStep("s1",
        [&]{ std::lock_guard<std::mutex> lk(mu); order.push_back(1); }));
    def.steps.push_back(makeStep("s2",
        [&]{ std::lock_guard<std::mutex> lk(mu); order.push_back(2); },
        {}, {"s1"}));
    def.steps.push_back(makeStep("s3",
        [&]{ std::lock_guard<std::mutex> lk(mu); order.push_back(3); },
        {}, {"s2"}));

    auto st = orch.execute(def);
    EXPECT_TRUE(st.ok);
    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 3);
}

// ─────────────────────────────────────────────────────────────────────────────
// Compensation exception does not abort the orchestrator
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SAGAOrchestratorTest, Compensation_ExceptionDuringComp_DoesNotCrash) {
    SAGAOrchestrator::SAGADefinition def;
    def.name           = "comp_throw";
    def.enable_parallel = false;

    def.steps.push_back(makeStep("s1", []{},
        []{ throw std::runtime_error("compensation kaboom"); }));
    def.steps.push_back(makeStep("s2",
        []{ throw std::runtime_error("step 2 failure"); }));

    // Should not propagate compensation exception
    EXPECT_NO_THROW({
        auto st = orch.execute(def);
        EXPECT_FALSE(st.ok); // SAGA failed due to s2
    });
}
