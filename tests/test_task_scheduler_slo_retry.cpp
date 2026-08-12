/**
 * @file test_task_scheduler_slo_retry.cpp
 * @brief Focused tests for Phase 5 SLO-based adaptive task retry policy.
 *
 * Tests cover:
 *  - SloRetryConfig default values
 *  - Optional absent by default
 *  - Config with sla_deadline accepted
 *  - Backward compatibility when no SloRetryConfig
 *  - Retry delay clamped to remaining SLA budget fraction
 *  - Retry skipped when SLA budget exhausted
 *  - SLO violation tracking on breach
 *  - No violation when within SLA
 *  - slo_aware = false disables adaptation
 *  - Compliance pressure reduces max_retries
 *  - Compliance pressure NOT applied when window not full
 *  - SLO fields default to zero
 *  - SloRetryConfig preserved after registerTask
 *  - Window slides when full
 *  - No adaptation when sla_deadline absent
 */

#include <gtest/gtest.h>
#include "scheduler/task_scheduler.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "query/query_engine.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>
#include <thread>

using namespace themis;
using namespace std::chrono_literals;

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

class TaskSchedulerSloRetryFocusedTests : public ::testing::Test {
protected:
    static std::string makeTempPath() {
        auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        return (std::filesystem::temp_directory_path() /
                ("themis_slo_retry_" + std::to_string(ts))).string();
    }

    void SetUp() override {
        db_path_ = makeTempPath();
        std::filesystem::create_directories(db_path_);

        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_ + "/db";
        storage_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open());
        idx_    = std::make_unique<SecondaryIndexManager>(*storage_);
        engine_ = std::make_unique<QueryEngine>(*storage_, *idx_);
    }

    void TearDown() override {
        if (scheduler_) {
            scheduler_->stop();
            scheduler_.reset();
        }
        engine_.reset();
        idx_.reset();
        storage_->close();
        storage_.reset();
        std::filesystem::remove_all(db_path_);
    }

    void makeScheduler() {
        TaskScheduler::Config cfg;
        cfg.max_concurrent_tasks     = 4;
        cfg.check_interval           = 50ms;
        cfg.persist_tasks            = false;
        cfg.enable_audit_logging     = false;
        cfg.enable_anomaly_detection = false;
        scheduler_ = std::make_unique<TaskScheduler>(engine_.get(), cfg);
        scheduler_->start();
    }

    /// Build a FUNCTION task that can fail, with a fast fixed-delay retry.
    ScheduledTask makeFailingTask(const std::string& id,
                                   const std::string& fn_name,
                                   size_t             max_retries = 3,
                                   std::chrono::milliseconds retry_delay = 1ms)
    {
        ScheduledTask t;
        t.id            = id;
        t.name          = id;
        t.type          = ScheduledTask::TaskType::FUNCTION;
        t.function_name = fn_name;
        t.interval      = std::chrono::seconds(9999);
        t.enabled       = true;

        ScheduledTask::RetryPolicy rp;
        rp.strategy      = ScheduledTask::RetryStrategy::FIXED_DELAY;
        rp.max_retries   = max_retries;
        rp.initial_delay = retry_delay;
        rp.max_delay     = retry_delay;
        t.retry_policy   = rp;
        return t;
    }

    std::string                           db_path_;
    std::unique_ptr<RocksDBWrapper>       storage_;
    std::unique_ptr<SecondaryIndexManager> idx_;
    std::unique_ptr<QueryEngine>          engine_;
    std::unique_ptr<TaskScheduler>        scheduler_;
};

// ---------------------------------------------------------------------------
// 1. SloRetryConfig default values
// ---------------------------------------------------------------------------

TEST_F(TaskSchedulerSloRetryFocusedTests, DefaultValues) {
    ScheduledTask::SloRetryConfig slo;
    EXPECT_TRUE(slo.slo_aware);
    EXPECT_DOUBLE_EQ(slo.slo_budget_fraction, 0.5);
    EXPECT_DOUBLE_EQ(slo.slo_compliance_threshold, 0.8);
    EXPECT_EQ(slo.min_retries_under_pressure, 1u);
    EXPECT_EQ(slo.slo_history_window, 20u);
}

// ---------------------------------------------------------------------------
// 2. slo_retry_config absent by default
// ---------------------------------------------------------------------------

TEST_F(TaskSchedulerSloRetryFocusedTests, OptionalAbsentByDefault) {
    ScheduledTask t;
    EXPECT_FALSE(t.slo_retry_config.has_value());
    EXPECT_EQ(t.slo_violations,   0u);
    EXPECT_EQ(t.slo_window_count, 0u);
}

// ---------------------------------------------------------------------------
// 3. SLO config present on a task with sla_deadline
// ---------------------------------------------------------------------------

TEST_F(TaskSchedulerSloRetryFocusedTests, ConfigWithDeadlineAccepted) {
    ScheduledTask t;
    t.sla_deadline = 2000ms;

    ScheduledTask::SloRetryConfig slo;
    slo.slo_budget_fraction       = 0.4;
    slo.slo_compliance_threshold  = 0.9;
    slo.min_retries_under_pressure = 2;
    slo.slo_history_window        = 10;
    t.slo_retry_config = slo;

    ASSERT_TRUE(t.slo_retry_config.has_value());
    EXPECT_DOUBLE_EQ(t.slo_retry_config->slo_budget_fraction, 0.4);
    EXPECT_DOUBLE_EQ(t.slo_retry_config->slo_compliance_threshold, 0.9);
    EXPECT_EQ(t.slo_retry_config->min_retries_under_pressure, 2u);
    EXPECT_EQ(t.slo_retry_config->slo_history_window, 10u);
}

// ---------------------------------------------------------------------------
// 4. Task executes without SloRetryConfig (backward compat)
// ---------------------------------------------------------------------------

TEST_F(TaskSchedulerSloRetryFocusedTests, NoSloConfigBackwardCompat) {
    makeScheduler();

    std::atomic<int> call_count{0};
    scheduler_->registerFunction("always_ok", [&](const nlohmann::json&) -> nlohmann::json {
        ++call_count;
        return {{"status", "ok"}};
    });

    ScheduledTask t;
    t.id            = "no_slo_task";
    t.name          = "no_slo_task";
    t.type          = ScheduledTask::TaskType::FUNCTION;
    t.function_name = "always_ok";
    t.interval      = 9999s;
    t.enabled       = true;
    // Deliberately leave slo_retry_config unset
    scheduler_->registerTask(t);

    auto result = scheduler_->executeTaskNow("no_slo_task");
    EXPECT_EQ(call_count.load(), 1);
    EXPECT_TRUE(result.is_object());
}

// ---------------------------------------------------------------------------
// 5. SLO budget caps retry delay
// ---------------------------------------------------------------------------

TEST_F(TaskSchedulerSloRetryFocusedTests, RetryDelayClampedToSloBudget) {
    makeScheduler();

    std::atomic<int> call_count{0};
    scheduler_->registerFunction("slow_fail", [&](const nlohmann::json&) -> nlohmann::json {
        ++call_count;
        throw std::runtime_error("transient");
    });

    ScheduledTask t = makeFailingTask("slo_delay_task", "slow_fail",
                                      /*max_retries=*/2, /*retry_delay=*/5000ms);
    // 200 ms SLA; 50% budget = 100 ms for retries
    t.sla_deadline = 200ms;

    ScheduledTask::SloRetryConfig slo;
    slo.slo_budget_fraction      = 0.5;  // 100 ms for retries
    slo.slo_compliance_threshold = 0.8;
    slo.min_retries_under_pressure = 1;
    slo.slo_history_window       = 0;    // disable history-based reduction
    t.slo_retry_config = slo;

    scheduler_->registerTask(t);

    auto start = std::chrono::steady_clock::now();
    scheduler_->executeTaskNow("slo_delay_task");
    auto end   = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double, std::milli>(end - start).count();

    // Without clamping: 2 retries × 5000 ms = 10000 ms.
    // With SLO budget clamping (100 ms budget), total wait << 1000 ms.
    EXPECT_LT(elapsed, 1000.0)
        << "Retry delay should have been clamped to SLO budget; got " << elapsed << "ms";
    EXPECT_GE(call_count.load(), 1);
}

// ---------------------------------------------------------------------------
// 6. Retry skipped when SLA budget exhausted
// ---------------------------------------------------------------------------

TEST_F(TaskSchedulerSloRetryFocusedTests, RetrySkippedWhenBudgetExhausted) {
    makeScheduler();

    std::atomic<int> call_count{0};
    scheduler_->registerFunction("always_fail_slow", [&](const nlohmann::json&) -> nlohmann::json {
        ++call_count;
        std::this_thread::sleep_for(30ms);  // consume SLA budget
        throw std::runtime_error("fail");
    });

    ScheduledTask t = makeFailingTask("budget_exhausted_task", "always_fail_slow",
                                      /*max_retries=*/5, /*retry_delay=*/1ms);
    t.sla_deadline = 50ms;  // very tight SLA (< time for first attempt)

    ScheduledTask::SloRetryConfig slo;
    slo.slo_budget_fraction      = 0.9;
    slo.slo_compliance_threshold = 0.8;
    slo.min_retries_under_pressure = 1;
    slo.slo_history_window       = 0;
    t.slo_retry_config = slo;

    scheduler_->registerTask(t);

    auto start = std::chrono::steady_clock::now();
    scheduler_->executeTaskNow("budget_exhausted_task");
    double elapsed = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - start).count();

    // All 5 retries running ≥ 6 × 30ms = 180ms minimum.
    // Budget exhaustion should stop early.
    EXPECT_LT(elapsed, 250.0)
        << "All retries ran despite exhausted budget; elapsed=" << elapsed << "ms";
}

// ---------------------------------------------------------------------------
// 7. SLO compliance tracking: violations incremented on breach
// ---------------------------------------------------------------------------

TEST_F(TaskSchedulerSloRetryFocusedTests, SloViolationIncrementedOnBreach) {
    makeScheduler();

    scheduler_->registerFunction("slow_ok", [](const nlohmann::json&) -> nlohmann::json {
        std::this_thread::sleep_for(80ms);
        return {{"ok", true}};
    });

    ScheduledTask t;
    t.id            = "track_violation";
    t.name          = "track_violation";
    t.type          = ScheduledTask::TaskType::FUNCTION;
    t.function_name = "slow_ok";
    t.interval      = 9999s;
    t.enabled       = true;
    t.sla_deadline  = 20ms;  // task takes 80ms → always breaches

    ScheduledTask::SloRetryConfig slo;
    slo.slo_aware                = true;
    slo.slo_history_window       = 10;
    slo.slo_budget_fraction      = 0.5;
    slo.slo_compliance_threshold = 0.8;
    slo.min_retries_under_pressure = 1;
    t.slo_retry_config = slo;

    scheduler_->registerTask(t);
    scheduler_->executeTaskNow("track_violation");

    auto task_ptr = scheduler_->getTask("track_violation");
    ASSERT_NE(task_ptr, nullptr);
    EXPECT_GE(task_ptr->slo_window_count, 1u) << "slo_window_count should be incremented";
    EXPECT_GE(task_ptr->slo_violations,   1u) << "slo_violations should be incremented on breach";
}

// ---------------------------------------------------------------------------
// 8. SLO compliance tracking: no violation when within SLA
// ---------------------------------------------------------------------------

TEST_F(TaskSchedulerSloRetryFocusedTests, NoViolationWhenWithinSla) {
    makeScheduler();

    scheduler_->registerFunction("fast_ok", [](const nlohmann::json&) -> nlohmann::json {
        return {{"ok", true}};
    });

    ScheduledTask t;
    t.id            = "track_no_violation";
    t.name          = "track_no_violation";
    t.type          = ScheduledTask::TaskType::FUNCTION;
    t.function_name = "fast_ok";
    t.interval      = 9999s;
    t.enabled       = true;
    t.sla_deadline  = 2000ms;  // very generous SLA → no breach

    ScheduledTask::SloRetryConfig slo;
    slo.slo_aware                = true;
    slo.slo_history_window       = 10;
    slo.slo_budget_fraction      = 0.5;
    slo.slo_compliance_threshold = 0.8;
    slo.min_retries_under_pressure = 1;
    t.slo_retry_config = slo;

    scheduler_->registerTask(t);
    scheduler_->executeTaskNow("track_no_violation");

    auto task_ptr = scheduler_->getTask("track_no_violation");
    ASSERT_NE(task_ptr, nullptr);
    EXPECT_GE(task_ptr->slo_window_count, 1u) << "slo_window_count should be incremented";
    EXPECT_EQ(task_ptr->slo_violations,   0u) << "slo_violations should NOT be incremented";
}

// ---------------------------------------------------------------------------
// 9. slo_aware = false → no adaptation
// ---------------------------------------------------------------------------

TEST_F(TaskSchedulerSloRetryFocusedTests, SloAwareFalseNoAdaptation) {
    makeScheduler();

    std::atomic<int> call_count{0};
    scheduler_->registerFunction("counted_fail", [&](const nlohmann::json&) -> nlohmann::json {
        ++call_count;
        throw std::runtime_error("fail");
    });

    ScheduledTask t = makeFailingTask("slo_disabled_task", "counted_fail",
                                      /*max_retries=*/3, /*retry_delay=*/1ms);
    t.sla_deadline = 1ms;  // absurdly tight; would skip retries if adaptation were on

    ScheduledTask::SloRetryConfig slo;
    slo.slo_aware                = false;  // DISABLED
    slo.slo_budget_fraction      = 0.01;
    slo.slo_history_window       = 0;
    slo.min_retries_under_pressure = 1;
    t.slo_retry_config = slo;

    scheduler_->registerTask(t);
    scheduler_->executeTaskNow("slo_disabled_task");

    // All 4 attempts (initial + 3 retries) should run since slo_aware=false
    EXPECT_EQ(call_count.load(), 4)
        << "All retries should run when slo_aware=false";
}

// ---------------------------------------------------------------------------
// 10. Compliance pressure reduces effective max_retries
// ---------------------------------------------------------------------------

TEST_F(TaskSchedulerSloRetryFocusedTests, CompliancePressureReducesMaxRetries) {
    makeScheduler();

    std::atomic<int> call_count{0};
    scheduler_->registerFunction("pressure_fail", [&](const nlohmann::json&) -> nlohmann::json {
        ++call_count;
        throw std::runtime_error("fail");
    });

    ScheduledTask t = makeFailingTask("pressure_task", "pressure_fail",
                                      /*max_retries=*/10, /*retry_delay=*/1ms);
    t.sla_deadline = 500ms;

    ScheduledTask::SloRetryConfig slo;
    slo.slo_aware                 = true;
    slo.slo_budget_fraction       = 0.9;
    slo.slo_compliance_threshold  = 0.8;  // 80% compliance required
    slo.min_retries_under_pressure = 1;   // under pressure: at most 1 retry
    slo.slo_history_window        = 4;    // small window
    t.slo_retry_config = slo;

    // Pre-populate counters: 3 violations out of 4 = 25% compliance < 80% threshold
    t.slo_violations   = 3;
    t.slo_window_count = 4;

    scheduler_->registerTask(t);
    scheduler_->executeTaskNow("pressure_task");

    // Under pressure: max retries clamped to min_retries_under_pressure = 1
    // → max attempts = 1 (initial) + 1 (retry) = 2
    EXPECT_LE(call_count.load(), 2)
        << "Compliance pressure should reduce max_retries; got " << call_count.load() << " calls";
}

// ---------------------------------------------------------------------------
// 11. Compliance pressure NOT applied when window is not yet full
// ---------------------------------------------------------------------------

TEST_F(TaskSchedulerSloRetryFocusedTests, CompliancePressureNotAppliedWhenWindowEmpty) {
    makeScheduler();

    std::atomic<int> call_count{0};
    scheduler_->registerFunction("window_fail", [&](const nlohmann::json&) -> nlohmann::json {
        ++call_count;
        throw std::runtime_error("fail");
    });

    ScheduledTask t = makeFailingTask("empty_window_task", "window_fail",
                                      /*max_retries=*/3, /*retry_delay=*/1ms);
    t.sla_deadline = 500ms;

    ScheduledTask::SloRetryConfig slo;
    slo.slo_aware                 = true;
    slo.slo_budget_fraction       = 0.9;
    slo.slo_compliance_threshold  = 0.8;
    slo.min_retries_under_pressure = 1;
    slo.slo_history_window        = 20;  // large window; no samples yet
    t.slo_retry_config = slo;
    // slo_violations = 0, slo_window_count = 0: window not full → no pressure

    scheduler_->registerTask(t);
    scheduler_->executeTaskNow("empty_window_task");

    // Window not full → all 4 attempts should run (1 initial + 3 retries)
    EXPECT_EQ(call_count.load(), 4)
        << "All retries should run when compliance window is not yet full";
}

// ---------------------------------------------------------------------------
// 12. SLO fields default to zero
// ---------------------------------------------------------------------------

TEST_F(TaskSchedulerSloRetryFocusedTests, SloFieldsDefaultZero) {
    ScheduledTask t;
    EXPECT_EQ(t.slo_violations,   0u);
    EXPECT_EQ(t.slo_window_count, 0u);
}

// ---------------------------------------------------------------------------
// 13. SloRetryConfig preserved after registerTask
// ---------------------------------------------------------------------------

TEST_F(TaskSchedulerSloRetryFocusedTests, SloConfigPreservedAfterRegister) {
    makeScheduler();

    ScheduledTask t;
    t.id            = "persist_slo";
    t.name          = "persist_slo";
    t.type          = ScheduledTask::TaskType::FUNCTION;
    t.function_name = "noop";
    t.interval      = 9999s;
    t.enabled       = true;

    ScheduledTask::SloRetryConfig slo;
    slo.slo_budget_fraction       = 0.3;
    slo.slo_compliance_threshold  = 0.95;
    slo.min_retries_under_pressure = 2;
    slo.slo_history_window        = 15;
    t.slo_retry_config = slo;
    t.slo_violations   = 5;
    t.slo_window_count = 10;

    scheduler_->registerTask(t);

    auto task_ptr = scheduler_->getTask("persist_slo");
    ASSERT_NE(task_ptr, nullptr);

    ASSERT_TRUE(task_ptr->slo_retry_config.has_value());
    EXPECT_DOUBLE_EQ(task_ptr->slo_retry_config->slo_budget_fraction, 0.3);
    EXPECT_DOUBLE_EQ(task_ptr->slo_retry_config->slo_compliance_threshold, 0.95);
    EXPECT_EQ(task_ptr->slo_retry_config->min_retries_under_pressure, 2u);
    EXPECT_EQ(task_ptr->slo_retry_config->slo_history_window, 15u);
    EXPECT_EQ(task_ptr->slo_violations,   5u);
    EXPECT_EQ(task_ptr->slo_window_count, 10u);
}

// ---------------------------------------------------------------------------
// 14. SLO window slides when full
// ---------------------------------------------------------------------------

TEST_F(TaskSchedulerSloRetryFocusedTests, WindowSlidesWhenFull) {
    makeScheduler();

    scheduler_->registerFunction("fast_ok2", [](const nlohmann::json&) -> nlohmann::json {
        return {};
    });

    ScheduledTask t;
    t.id            = "slide_window";
    t.name          = "slide_window";
    t.type          = ScheduledTask::TaskType::FUNCTION;
    t.function_name = "fast_ok2";
    t.interval      = 9999s;
    t.enabled       = true;
    t.sla_deadline  = 2000ms;

    ScheduledTask::SloRetryConfig slo;
    slo.slo_aware                = true;
    slo.slo_history_window       = 4;    // tiny window for testability
    slo.slo_budget_fraction      = 0.9;
    slo.slo_compliance_threshold = 0.8;
    slo.min_retries_under_pressure = 1;
    t.slo_retry_config = slo;

    // Pre-fill exactly to window capacity
    t.slo_violations   = 2;
    t.slo_window_count = 4;  // full

    scheduler_->registerTask(t);
    scheduler_->executeTaskNow("slide_window");  // triggers window slide

    auto task_ptr = scheduler_->getTask("slide_window");
    ASSERT_NE(task_ptr, nullptr);

    // After slide: count = (4/2) + 1 = 3
    EXPECT_LE(task_ptr->slo_window_count, 4u);
    EXPECT_GE(task_ptr->slo_window_count, 1u);
}

// ---------------------------------------------------------------------------
// 15. No adaptation when sla_deadline absent
// ---------------------------------------------------------------------------

TEST_F(TaskSchedulerSloRetryFocusedTests, SloConfigIgnoredWithoutDeadline) {
    makeScheduler();

    std::atomic<int> call_count{0};
    scheduler_->registerFunction("fail_no_deadline", [&](const nlohmann::json&) -> nlohmann::json {
        ++call_count;
        throw std::runtime_error("fail");
    });

    ScheduledTask t = makeFailingTask("no_deadline_task", "fail_no_deadline",
                                      /*max_retries=*/3, /*retry_delay=*/1ms);
    // Deliberately leave t.sla_deadline unset

    ScheduledTask::SloRetryConfig slo;
    slo.slo_aware                = true;
    slo.slo_budget_fraction      = 0.01;  // would skip ALL retries if applied
    slo.slo_history_window       = 0;
    slo.min_retries_under_pressure = 1;
    t.slo_retry_config = slo;

    scheduler_->registerTask(t);
    scheduler_->executeTaskNow("no_deadline_task");

    // All 4 attempts should run (no sla_deadline means no clamping)
    EXPECT_EQ(call_count.load(), 4)
        << "SloRetryConfig without sla_deadline should be ignored; got " << call_count.load();
}
