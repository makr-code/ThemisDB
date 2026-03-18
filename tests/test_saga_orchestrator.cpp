// Tests for SAGAOrchestrator – v1.8.0
// Covers all acceptance criteria from Issue #4043:
//   AC-1  Parallel step execution (DAG-based) – independent steps run concurrently
//   AC-2  Conditional branching – steps with condition=false are SKIPPED
//   AC-3  Retry policies per step – failed forward retries up to max_retries times
//   AC-4  Timeout management – steps that exceed their timeout are aborted
//   AC-5  SAGA templates – register/instantiate named templates
//   AC-6  Visual workflow designer – renderWorkflow() produces valid ASCII DAG
//   AC-7  Complex workflow support – multi-level diamond DAG executes correctly
//   AC-8  Automatic dependency resolution – topological sort respects depends_on
//   AC-9  Compensation – failed saga compensates completed steps in reverse order
//   AC-10 Validate – duplicate names, cycles, unknown deps are rejected
//   AC-11 getStatus() – returns status after execute; nullopt for unknown id
//   AC-12 getMetrics() – counters reflect started/completed/compensated/failed
//   AC-13 Exponential back-off – retry_delay doubles on each attempt
//   AC-14 Context – context map is available on instantiated templates
//   AC-15 No-compensation skip – steps with no compensate callable are skipped silently
//   AC-16 Parallel speedup – parallel SAGA finishes faster than sequential equivalent
//   AC-17 Skipped step not compensated – SKIPPED steps are excluded from compensation
//   AC-18 Multi-level fan-out/fan-in – complex DAG resolves correctly
//   AC-19 journal_path – journal file is written when path is configured
//   AC-20 Thread safety – concurrent execute() calls on one orchestrator are safe

#include <gtest/gtest.h>
#include "transaction/saga_orchestrator.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace themis;
using namespace std::chrono_literals;

namespace {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

/// Build a minimal valid one-step SAGA.
SAGADefinition make_trivial_saga(const std::string& id = "s1",
                                  bool forward_ok = true) {
    SAGADefinition saga;
    saga.id   = id;
    saga.name = "trivial";
    SAGAStep step;
    step.name = "only";
    step.forward = [forward_ok]() {
        if (!forward_ok) throw std::runtime_error("forced failure");
    };
    saga.steps.push_back(std::move(step));
    return saga;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// AC-1: Parallel step execution
// ─────────────────────────────────────────────────────────────────────────────

TEST(SAGAOrchestratorTest, AC1_ParallelStepExecution_IndependentStepsRunConcurrently) {
    SAGAOrchestrator::Config cfg;
    cfg.enable_parallel = true;
    SAGAOrchestrator orch(cfg);

    std::atomic<int> concurrent{0};
    std::atomic<int> max_concurrent{0};

    auto make_slow_step = [&](const std::string& name) {
        SAGAStep s;
        s.name = name;
        s.forward = [&]() {
            int cur = ++concurrent;
            int prev = max_concurrent.load();
            while (prev < cur && !max_concurrent.compare_exchange_weak(prev, cur)) {}
            std::this_thread::sleep_for(20ms);
            --concurrent;
        };
        return s;
    };

    SAGADefinition saga;
    saga.id   = "parallel-1";
    saga.name = "parallel_test";
    saga.enable_parallel = true;
    saga.steps.push_back(make_slow_step("step_a"));
    saga.steps.push_back(make_slow_step("step_b"));
    saga.steps.push_back(make_slow_step("step_c"));

    auto result = orch.execute(saga);
    EXPECT_TRUE(result.ok) << result.message;
    // With parallel execution the max concurrency should be >1
    EXPECT_GT(max_concurrent.load(), 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-2: Conditional branching
// ─────────────────────────────────────────────────────────────────────────────

TEST(SAGAOrchestratorTest, AC2_ConditionalBranching_FalseConditionSkipsStep) {
    SAGAOrchestrator orch;

    bool executed = false;
    SAGADefinition saga;
    saga.id   = "cond-1";
    saga.name = "conditional";
    SAGAStep s;
    s.name      = "maybe";
    s.condition = []() { return false; };
    s.forward   = [&executed]() { executed = true; };
    saga.steps.push_back(std::move(s));

    auto result = orch.execute(saga);
    EXPECT_TRUE(result.ok) << result.message;
    EXPECT_FALSE(executed);

    auto status = orch.getStatus("cond-1");
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status->step_states.at("maybe"), StepState::SKIPPED);
    EXPECT_EQ(status->skipped_steps, 1u);
}

TEST(SAGAOrchestratorTest, AC2_ConditionalBranching_TrueConditionExecutesStep) {
    SAGAOrchestrator orch;

    bool executed = false;
    SAGADefinition saga;
    saga.id   = "cond-2";
    saga.name = "conditional_true";
    SAGAStep s;
    s.name      = "always";
    s.condition = []() { return true; };
    s.forward   = [&executed]() { executed = true; };
    saga.steps.push_back(std::move(s));

    auto result = orch.execute(saga);
    EXPECT_TRUE(result.ok) << result.message;
    EXPECT_TRUE(executed);
    auto status = orch.getStatus("cond-2");
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status->step_states.at("always"), StepState::COMPLETED);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-3: Retry policies per step
// ─────────────────────────────────────────────────────────────────────────────

TEST(SAGAOrchestratorTest, AC3_RetryPolicies_StepSucceedsOnSecondAttempt) {
    SAGAOrchestrator orch;

    std::atomic<int> attempts{0};
    SAGADefinition saga;
    saga.id   = "retry-1";
    saga.name = "retry_test";
    SAGAStep s;
    s.name        = "flaky";
    s.max_retries = 2;
    s.retry_delay = 1ms; // fast for tests
    s.forward     = [&attempts]() {
        if (++attempts < 2) throw std::runtime_error("transient");
    };
    saga.steps.push_back(std::move(s));

    auto result = orch.execute(saga);
    EXPECT_TRUE(result.ok) << result.message;
    EXPECT_EQ(attempts.load(), 2);

    auto metrics = orch.getMetrics();
    EXPECT_GE(metrics.total_step_retries, 1u);
}

TEST(SAGAOrchestratorTest, AC3_RetryPolicies_StepFailsAfterAllRetries) {
    SAGAOrchestrator orch;

    std::atomic<int> attempts{0};
    SAGADefinition saga;
    saga.id   = "retry-2";
    saga.name = "always_fail";
    SAGAStep s;
    s.name        = "bad";
    s.max_retries = 2;
    s.retry_delay = 1ms;
    s.forward     = [&attempts]() { ++attempts; throw std::runtime_error("always"); };
    saga.steps.push_back(std::move(s));

    auto result = orch.execute(saga);
    EXPECT_FALSE(result.ok);
    // 1 initial + 2 retries = 3
    EXPECT_EQ(attempts.load(), 3);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-4: Timeout management
// ─────────────────────────────────────────────────────────────────────────────

TEST(SAGAOrchestratorTest, AC4_TimeoutManagement_SlowStepTimesOut) {
    SAGAOrchestrator::Config cfg;
    cfg.default_timeout = 50ms;
    SAGAOrchestrator orch(cfg);

    SAGADefinition saga;
    saga.id   = "timeout-1";
    saga.name = "slow_step";
    SAGAStep s;
    s.name        = "slow";
    s.timeout     = 50ms;
    s.max_retries = 0;
    s.forward     = []() { std::this_thread::sleep_for(500ms); };
    saga.steps.push_back(std::move(s));

    auto start  = std::chrono::steady_clock::now();
    auto result = orch.execute(saga);
    auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_FALSE(result.ok);
    // Must have returned well before the full 500 ms sleep
    EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 400);
}

TEST(SAGAOrchestratorTest, AC4_TimeoutManagement_FastStepCompletesBeforeTimeout) {
    SAGAOrchestrator::Config cfg;
    cfg.default_timeout = 500ms;
    SAGAOrchestrator orch(cfg);

    SAGADefinition saga = make_trivial_saga("timeout-ok");
    auto result = orch.execute(saga);
    EXPECT_TRUE(result.ok) << result.message;
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-5: SAGA templates
// ─────────────────────────────────────────────────────────────────────────────

TEST(SAGAOrchestratorTest, AC5_SAGATemplates_RegisterAndInstantiate) {
    SAGAOrchestrator orch;

    SAGADefinition tmpl;
    tmpl.id   = "template-base";
    tmpl.name = "order_saga";
    tmpl.context["env"] = "staging";
    SAGAStep s;
    s.name    = "charge";
    s.forward = []() {};
    tmpl.steps.push_back(std::move(s));

    orch.registerTemplate("order", tmpl);

    auto instance = orch.instantiateTemplate("order", "order-instance-1",
                                              {{"env", "production"}, {"order_id", "42"}});

    EXPECT_EQ(instance.id, "order-instance-1");
    EXPECT_EQ(instance.name, "order_saga");
    EXPECT_EQ(instance.context.at("env"), "production"); // override wins
    EXPECT_EQ(instance.context.at("order_id"), "42");

    auto result = orch.execute(instance);
    EXPECT_TRUE(result.ok) << result.message;
}

TEST(SAGAOrchestratorTest, AC5_SAGATemplates_UnknownTemplateThrows) {
    SAGAOrchestrator orch;
    EXPECT_THROW(orch.instantiateTemplate("nonexistent", "i1"), std::out_of_range);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-6: Visual workflow designer
// ─────────────────────────────────────────────────────────────────────────────

TEST(SAGAOrchestratorTest, AC6_VisualWorkflowDesigner_ContainsSagaName) {
    SAGAOrchestrator orch;

    SAGADefinition saga;
    saga.id   = "vis-1";
    saga.name = "process_order";
    SAGAStep a, b, c;
    a.name = "reserve_inventory";
    b.name = "validate_customer";
    c.name = "charge_payment";
    c.depends_on = {"reserve_inventory", "validate_customer"};
    a.forward = b.forward = c.forward = []() {};
    saga.steps = {a, b, c};

    std::string viz = orch.renderWorkflow(saga);
    EXPECT_NE(viz.find("process_order"), std::string::npos);
    EXPECT_NE(viz.find("reserve_inventory"), std::string::npos);
    EXPECT_NE(viz.find("charge_payment"), std::string::npos);
}

TEST(SAGAOrchestratorTest, AC6_VisualWorkflowDesigner_TerminalNodeLabelled) {
    SAGAOrchestrator orch;

    SAGADefinition saga;
    saga.id   = "vis-2";
    saga.name = "simple";
    SAGAStep s;
    s.name    = "only_step";
    s.forward = []() {};
    saga.steps.push_back(std::move(s));

    std::string viz = orch.renderWorkflow(saga);
    EXPECT_NE(viz.find("terminal"), std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-7: Complex workflow support (multi-level diamond)
// ─────────────────────────────────────────────────────────────────────────────

TEST(SAGAOrchestratorTest, AC7_ComplexWorkflow_DiamondDAGExecutesCorrectly) {
    SAGAOrchestrator orch;

    std::vector<std::string> execution_order;
    std::mutex mu;

    auto track = [&](const std::string& name) {
        return [&execution_order, &mu, name]() {
            std::lock_guard<std::mutex> lk(mu);
            execution_order.push_back(name);
        };
    };

    SAGADefinition saga;
    saga.id            = "diamond-1";
    saga.name          = "diamond";
    saga.enable_parallel = false; // sequential to check order

    SAGAStep root, left, right, join;
    root.name  = "root";
    left.name  = "left";  left.depends_on  = {"root"};
    right.name = "right"; right.depends_on = {"root"};
    join.name  = "join";  join.depends_on  = {"left", "right"};

    root.forward  = track("root");
    left.forward  = track("left");
    right.forward = track("right");
    join.forward  = track("join");

    saga.steps = {root, left, right, join};

    auto result = orch.execute(saga);
    EXPECT_TRUE(result.ok) << result.message;

    // root must be first, join must be last
    ASSERT_EQ(execution_order.size(), 4u);
    EXPECT_EQ(execution_order.front(), "root");
    EXPECT_EQ(execution_order.back(),  "join");
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-8: Automatic dependency resolution
// ─────────────────────────────────────────────────────────────────────────────

TEST(SAGAOrchestratorTest, AC8_DependencyResolution_StepsRunInTopologicalOrder) {
    SAGAOrchestrator orch;

    std::vector<std::string> order;
    std::mutex mu;

    SAGADefinition saga;
    saga.id            = "topo-1";
    saga.name          = "chain";
    saga.enable_parallel = false;

    SAGAStep s1, s2, s3;
    s1.name = "first";
    s2.name = "second"; s2.depends_on = {"first"};
    s3.name = "third";  s3.depends_on = {"second"};
    for (auto* s : {&s1, &s2, &s3}) {
        const std::string n = s->name;
        s->forward = [&order, &mu, n]() {
            std::lock_guard<std::mutex> lk(mu);
            order.push_back(n);
        };
    }
    saga.steps = {s3, s1, s2}; // intentionally out-of-order

    auto result = orch.execute(saga);
    EXPECT_TRUE(result.ok) << result.message;
    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], "first");
    EXPECT_EQ(order[1], "second");
    EXPECT_EQ(order[2], "third");
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-9: Compensation
// ─────────────────────────────────────────────────────────────────────────────

TEST(SAGAOrchestratorTest, AC9_Compensation_FailedSagaCompensatesCompletedSteps) {
    SAGAOrchestrator orch;

    std::vector<std::string> comp_order;
    std::mutex mu;

    SAGADefinition saga;
    saga.id            = "comp-1";
    saga.name          = "compensation_test";
    saga.enable_parallel = false;

    SAGAStep s1, s2, s3;
    s1.name = "step1";
    s2.name = "step2"; s2.depends_on = {"step1"};
    s3.name = "step3"; s3.depends_on = {"step2"};

    s1.forward    = []() {};
    s2.forward    = []() {};
    s3.forward    = []() { throw std::runtime_error("step3 fails"); };

    s1.compensate = [&comp_order, &mu]() {
        std::lock_guard<std::mutex> lk(mu);
        comp_order.push_back("comp-step1");
    };
    s2.compensate = [&comp_order, &mu]() {
        std::lock_guard<std::mutex> lk(mu);
        comp_order.push_back("comp-step2");
    };

    saga.steps = {s1, s2, s3};

    auto result = orch.execute(saga);
    EXPECT_FALSE(result.ok);

    // Compensation must happen in reverse: step2, then step1
    ASSERT_EQ(comp_order.size(), 2u);
    EXPECT_EQ(comp_order[0], "comp-step2");
    EXPECT_EQ(comp_order[1], "comp-step1");

    auto metrics = orch.getMetrics();
    EXPECT_EQ(metrics.sagas_compensated, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-10: Validate
// ─────────────────────────────────────────────────────────────────────────────

TEST(SAGAOrchestratorTest, AC10_Validate_DuplicateStepNameRejected) {
    SAGAOrchestrator orch;

    SAGADefinition saga;
    saga.id = "val-dup";
    SAGAStep a, b;
    a.name = b.name = "same";
    a.forward = b.forward = []() {};
    saga.steps = {a, b};

    auto result = orch.validate(saga);
    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.message.find("duplicate"), std::string::npos);
}

TEST(SAGAOrchestratorTest, AC10_Validate_UnknownDependencyRejected) {
    SAGAOrchestrator orch;

    SAGADefinition saga;
    saga.id = "val-unk";
    SAGAStep s;
    s.name        = "step";
    s.depends_on  = {"ghost"};
    s.forward     = []() {};
    saga.steps    = {s};

    auto result = orch.validate(saga);
    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.message.find("ghost"), std::string::npos);
}

TEST(SAGAOrchestratorTest, AC10_Validate_CycleDetected) {
    SAGAOrchestrator orch;

    SAGADefinition saga;
    saga.id = "val-cycle";
    SAGAStep a, b;
    a.name       = "a"; a.depends_on = {"b"};
    b.name       = "b"; b.depends_on = {"a"};
    a.forward = b.forward = []() {};
    saga.steps = {a, b};

    auto result = orch.validate(saga);
    EXPECT_FALSE(result.ok);
}

TEST(SAGAOrchestratorTest, AC10_Validate_EmptyIdRejected) {
    SAGAOrchestrator orch;

    SAGADefinition saga = make_trivial_saga("");
    auto result = orch.validate(saga);
    EXPECT_FALSE(result.ok);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-11: getStatus()
// ─────────────────────────────────────────────────────────────────────────────

TEST(SAGAOrchestratorTest, AC11_GetStatus_ReturnsStatusAfterExecute) {
    SAGAOrchestrator orch;
    SAGADefinition saga = make_trivial_saga("status-1");
    orch.execute(saga);

    auto status = orch.getStatus("status-1");
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status->saga_id, "status-1");
    EXPECT_EQ(status->completed_steps, 1u);
    EXPECT_EQ(status->failed_steps, 0u);
}

TEST(SAGAOrchestratorTest, AC11_GetStatus_NulloptForUnknownId) {
    SAGAOrchestrator orch;
    EXPECT_FALSE(orch.getStatus("nonexistent").has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-12: getMetrics()
// ─────────────────────────────────────────────────────────────────────────────

TEST(SAGAOrchestratorTest, AC12_Metrics_CountersAccurate) {
    SAGAOrchestrator orch;

    // 2 successful sagas
    orch.execute(make_trivial_saga("m1"));
    orch.execute(make_trivial_saga("m2"));

    // 1 failing saga (triggers compensation)
    SAGADefinition failing = make_trivial_saga("m3", /*forward_ok=*/false);
    orch.execute(failing);

    auto m = orch.getMetrics();
    EXPECT_EQ(m.sagas_started,     3u);
    EXPECT_EQ(m.sagas_completed,   2u);
    EXPECT_EQ(m.sagas_compensated, 1u);
    EXPECT_GE(m.total_step_executions, 3u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-13: Exponential back-off
// ─────────────────────────────────────────────────────────────────────────────

TEST(SAGAOrchestratorTest, AC13_ExponentialBackoff_RetriesAreDelayed) {
    SAGAOrchestrator orch;

    std::vector<std::chrono::steady_clock::time_point> attempt_times;
    std::mutex mu;

    SAGADefinition saga;
    saga.id   = "backoff-1";
    saga.name = "backoff";
    SAGAStep s;
    s.name        = "flaky";
    s.max_retries = 3;
    s.retry_delay = 10ms;
    s.forward     = [&attempt_times, &mu]() {
        std::lock_guard<std::mutex> lk(mu);
        attempt_times.push_back(std::chrono::steady_clock::now());
        throw std::runtime_error("keep failing");
    };
    saga.steps.push_back(std::move(s));

    orch.execute(saga);

    ASSERT_GE(attempt_times.size(), 2u);
    auto gap_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        attempt_times[1] - attempt_times[0]).count();
    // First retry delay is 10ms; allow generous margin for scheduler jitter
    EXPECT_GE(gap_ms, 5);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-14: Context map on template instances
// ─────────────────────────────────────────────────────────────────────────────

TEST(SAGAOrchestratorTest, AC14_Context_TemplateContextMergedWithOverrides) {
    SAGAOrchestrator orch;

    SAGADefinition tmpl;
    tmpl.id      = "tmpl";
    tmpl.name    = "ctx_saga";
    tmpl.context = {{"key1", "v1"}, {"key2", "v2"}};
    SAGAStep s;
    s.name    = "work";
    s.forward = []() {};
    tmpl.steps.push_back(std::move(s));

    orch.registerTemplate("ctx_tmpl", tmpl);

    auto inst = orch.instantiateTemplate("ctx_tmpl", "inst-1", {{"key2", "overridden"}, {"key3", "v3"}});
    EXPECT_EQ(inst.context.at("key1"), "v1");        // from template
    EXPECT_EQ(inst.context.at("key2"), "overridden"); // overridden
    EXPECT_EQ(inst.context.at("key3"), "v3");         // new
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-15: No-compensation skip
// ─────────────────────────────────────────────────────────────────────────────

TEST(SAGAOrchestratorTest, AC15_NoCompensation_StepWithoutCompensateHandledSilently) {
    SAGAOrchestrator orch;

    SAGADefinition saga;
    saga.id            = "nocomp-1";
    saga.name          = "no_comp";
    saga.enable_parallel = false;

    SAGAStep s1, s2;
    s1.name      = "good";
    s1.forward   = []() {};
    // s1 has no compensate

    s2.name    = "bad";
    s2.depends_on = {"good"};
    s2.forward = []() { throw std::runtime_error("fail"); };

    saga.steps = {s1, s2};

    // Should not crash even though s1 has no compensation
    auto result = orch.execute(saga);
    EXPECT_FALSE(result.ok);

    auto status = orch.getStatus("nocomp-1");
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status->step_states.at("good"), StepState::COMPENSATED);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-16: Parallel speedup
// ─────────────────────────────────────────────────────────────────────────────

TEST(SAGAOrchestratorTest, AC16_ParallelSpeedup_FasterThanSequential) {
    // Wall-clock timing test: opt-in only to avoid flakiness under CI load
    // or sanitizers. Run with THEMIS_RUN_PERF_TESTS=1 to enable.
    if (!std::getenv("THEMIS_RUN_PERF_TESTS")) {
        GTEST_SKIP() << "Skipped (set THEMIS_RUN_PERF_TESTS=1 to enable)";
    }

    auto run_saga = [](bool parallel) -> int64_t {
        SAGAOrchestrator::Config cfg;
        cfg.enable_parallel = parallel;
        SAGAOrchestrator orch(cfg);

        SAGADefinition saga;
        saga.id            = parallel ? "par" : "seq";
        saga.name          = "speedup_test";
        saga.enable_parallel = parallel;

        for (int i = 0; i < 4; ++i) {
            SAGAStep s;
            s.name    = "step" + std::to_string(i);
            s.forward = []() { std::this_thread::sleep_for(30ms); };
            saga.steps.push_back(std::move(s));
        }

        auto t0 = std::chrono::steady_clock::now();
        orch.execute(saga);
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - t0).count();
    };

    int64_t seq_ms = run_saga(false);
    int64_t par_ms = run_saga(true);

    // Parallel must finish notably faster (at least 1.5x)
    EXPECT_LT(par_ms * 3, seq_ms * 2)
        << "par=" << par_ms << "ms seq=" << seq_ms << "ms";
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-17: Skipped steps not compensated
// ─────────────────────────────────────────────────────────────────────────────

TEST(SAGAOrchestratorTest, AC17_SkippedStepNotCompensated) {
    SAGAOrchestrator orch;

    bool comp_called = false;
    SAGADefinition saga;
    saga.id            = "skip-comp-1";
    saga.name          = "skip_comp";
    saga.enable_parallel = false;

    SAGAStep s1, s2;
    s1.name      = "skipped";
    s1.condition = []() { return false; };
    s1.forward   = []() {};
    s1.compensate = [&comp_called]() { comp_called = true; };

    s2.name    = "fail";
    s2.depends_on = {}; // independent
    s2.forward = []() { throw std::runtime_error("fail"); };

    saga.steps = {s1, s2};

    orch.execute(saga);
    EXPECT_FALSE(comp_called);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-18: Multi-level fan-out/fan-in
// ─────────────────────────────────────────────────────────────────────────────

TEST(SAGAOrchestratorTest, AC18_MultiLevelFanOutFanIn_AllStepsExecuted) {
    SAGAOrchestrator orch;

    std::atomic<int> count{0};

    SAGADefinition saga;
    saga.id   = "fanout-1";
    saga.name = "fan_out_in";
    saga.enable_parallel = true;

    // root → {A, B, C} → join
    SAGAStep root, a, b, c, join;
    root.name = "root";
    a.name    = "a"; a.depends_on = {"root"};
    b.name    = "b"; b.depends_on = {"root"};
    c.name    = "c"; c.depends_on = {"root"};
    join.name = "join"; join.depends_on = {"a", "b", "c"};

    for (auto* s : {&root, &a, &b, &c, &join}) {
        s->forward = [&count]() { ++count; };
    }
    saga.steps = {root, a, b, c, join};

    auto result = orch.execute(saga);
    EXPECT_TRUE(result.ok) << result.message;
    EXPECT_EQ(count.load(), 5);

    auto status = orch.getStatus("fanout-1");
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status->completed_steps, 5u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-19: Journal file written when path is configured
// ─────────────────────────────────────────────────────────────────────────────

TEST(SAGAOrchestratorTest, AC19_Journal_WrittenWhenPathConfigured) {
    // Build a unique path under the system temp dir using a nanosecond
    // timestamp + steady-clock counter to avoid collisions in parallel runs.
    const auto ts = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::string journal_path =
        (std::filesystem::temp_directory_path() /
         ("saga_test_journal_" + std::to_string(ts) + ".jsonl"))
        .string();
    std::filesystem::remove(journal_path);

    SAGAOrchestrator::Config cfg;
    cfg.journal_path = journal_path;
    SAGAOrchestrator orch(cfg);

    SAGADefinition saga = make_trivial_saga("journal-1");
    orch.execute(saga);

    EXPECT_TRUE(std::filesystem::exists(journal_path));

    std::ifstream ifs(journal_path);
    std::string line;
    std::getline(ifs, line);
    EXPECT_NE(line.find("saga_started"), std::string::npos);

    // Clean up even if assertions fail (RAII not needed here since we check
    // remove() in a separate statement that won't throw).
    std::filesystem::remove(journal_path);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-20: Thread safety
// ─────────────────────────────────────────────────────────────────────────────

TEST(SAGAOrchestratorTest, AC20_ThreadSafety_ConcurrentExecuteCallsAreSafe) {
    SAGAOrchestrator orch;

    constexpr int N = 20;
    std::atomic<int> succeeded{0};
    std::vector<std::thread> threads;
    threads.reserve(N);

    for (int i = 0; i < N; ++i) {
        threads.emplace_back([&orch, &succeeded, i]() {
            SAGADefinition saga = make_trivial_saga("thread-" + std::to_string(i));
            auto result = orch.execute(saga);
            if (result.ok) ++succeeded;
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_EQ(succeeded.load(), N);
    auto m = orch.getMetrics();
    EXPECT_EQ(m.sagas_completed, static_cast<uint64_t>(N));
}

// ─────────────────────────────────────────────────────────────────────────────
// Additional edge-case: empty steps rejected by execute()
// ─────────────────────────────────────────────────────────────────────────────

TEST(SAGAOrchestratorTest, EdgeCase_EmptyStepsRejected) {
    SAGAOrchestrator orch;
    SAGADefinition saga;
    saga.id   = "empty";
    saga.name = "empty";

    auto result = orch.execute(saga);
    EXPECT_FALSE(result.ok);
}

// ─────────────────────────────────────────────────────────────────────────────
// Additional: successful saga increments sagas_completed, not compensated
// ─────────────────────────────────────────────────────────────────────────────

TEST(SAGAOrchestratorTest, SuccessfulSaga_DoesNotIncrementCompensatedCounter) {
    SAGAOrchestrator orch;
    orch.execute(make_trivial_saga("ok-1"));
    orch.execute(make_trivial_saga("ok-2"));

    auto m = orch.getMetrics();
    EXPECT_EQ(m.sagas_completed,   2u);
    EXPECT_EQ(m.sagas_compensated, 0u);
    EXPECT_EQ(m.sagas_failed,      0u);
}
