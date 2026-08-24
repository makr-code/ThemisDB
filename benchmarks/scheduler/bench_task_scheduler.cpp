/// @file bench_task_scheduler.cpp
/// @brief Performance benchmarks for the TaskScheduler.
///
/// Covers the following process lines:
///   - TaskScheduler::registerTask()    – registration overhead
///   - TaskScheduler::unregisterTask()  – removal overhead
///   - TaskScheduler::executeTaskNow()  – synchronous execution latency
///   - TaskScheduler::getStats()        – stats retrieval overhead
///   - TaskScheduler::listTasks()       – list overhead vs. registered count
///   - TaskScheduler::registerFunction() + executeTaskNow() – function call
///
/// Performance targets (src/scheduler/ROADMAP.md):
///   - registerTask:    < 100 µs
///   - unregisterTask:  < 50 µs
///   - executeTaskNow:  < 500 µs (function task, no actual query engine)
///   - listTasks (100): < 200 µs

#include <benchmark/benchmark.h>
#include "scheduler/task_scheduler.h"
#include "query/query_engine.h"
#include "index/secondary_index.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <atomic>
#include <string>
#include <chrono>

using namespace themis;
using namespace std::chrono_literals;

// ============================================================================
// Fixture
// ============================================================================

class TaskSchedulerBenchFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& /*state*/) override {
        db_path_ = "./data/bench_task_scheduler_tmp";
        if (std::filesystem::exists(db_path_)) {
            std::filesystem::remove_all(db_path_);
        }
        std::filesystem::create_directories(db_path_);

        RocksDBWrapper::Config cfg;
        cfg.db_path       = db_path_ + "/db";
        cfg.enable_blobdb = false;
        storage_          = std::make_unique<RocksDBWrapper>(cfg);
        storage_->open();

        idx_    = std::make_unique<SecondaryIndexManager>(*storage_);
        engine_ = std::make_unique<QueryEngine>(*storage_, *idx_);

        TaskScheduler::Config sched_cfg;
        sched_cfg.max_concurrent_tasks    = 4;
        sched_cfg.check_interval          = 1h; // disable auto-tick
        sched_cfg.persist_tasks           = false;
        sched_cfg.enable_audit_logging    = false;
        sched_cfg.enable_anomaly_detection = false;

        scheduler_ = std::make_unique<TaskScheduler>(engine_.get(), sched_cfg);
        scheduler_->start();
    }

    void TearDown(const ::benchmark::State& /*state*/) override {
        scheduler_->stop();
        scheduler_.reset();
        engine_.reset();
        idx_.reset();
        storage_->close();
        storage_.reset();
        if (std::filesystem::exists(db_path_)) {
            std::filesystem::remove_all(db_path_);
        }
    }

    std::string                             db_path_;
    std::unique_ptr<RocksDBWrapper>         storage_;
    std::unique_ptr<SecondaryIndexManager>  idx_;
    std::unique_ptr<QueryEngine>            engine_;
    std::unique_ptr<TaskScheduler>          scheduler_;
};

// ============================================================================
// registerTask / unregisterTask overhead
// ============================================================================

BENCHMARK_DEFINE_F(TaskSchedulerBenchFixture, RegisterUnregister)(benchmark::State& state) {
    std::atomic<int> counter{0};
    for (auto _ : state) {
        int id = counter.fetch_add(1, std::memory_order_relaxed);
        ScheduledTask task;
        task.name     = "bench_task_" + std::to_string(id);
        task.type     = ScheduledTask::TaskType::FUNCTION;
        task.function_name = "noop_fn";
        task.trigger_type  = ScheduledTask::TriggerType::MANUAL;

        std::string tid = scheduler_->registerTask(task);
        scheduler_->unregisterTask(tid);
        benchmark::DoNotOptimize(tid);
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(TaskSchedulerBenchFixture, RegisterUnregister)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(5000);

// ============================================================================
// executeTaskNow – function task
// ============================================================================

BENCHMARK_DEFINE_F(TaskSchedulerBenchFixture, ExecuteTaskNow)(benchmark::State& state) {
    // Register a no-op function
    scheduler_->registerFunction("bench_noop", [](const nlohmann::json&) {
        return nlohmann::json{{"status", "ok"}};
    });

    ScheduledTask task;
    task.name          = "bench_exec_task";
    task.type          = ScheduledTask::TaskType::FUNCTION;
    task.function_name = "bench_noop";
    task.trigger_type  = ScheduledTask::TriggerType::MANUAL;
    std::string tid    = scheduler_->registerTask(task);

    for (auto _ : state) {
        auto result = scheduler_->executeTaskNow(tid);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(TaskSchedulerBenchFixture, ExecuteTaskNow)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(1000);

// ============================================================================
// listTasks overhead vs. number of registered tasks
// ============================================================================

BENCHMARK_DEFINE_F(TaskSchedulerBenchFixture, ListTasks)(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));

    // Pre-register n tasks
    for (int i = 0; i < n; ++i) {
        ScheduledTask task;
        task.name          = "list_task_" + std::to_string(i);
        task.type          = ScheduledTask::TaskType::FUNCTION;
        task.function_name = "noop";
        task.trigger_type  = ScheduledTask::TriggerType::MANUAL;
        scheduler_->registerTask(task);
    }

    for (auto _ : state) {
        auto tasks = scheduler_->listTasks();
        benchmark::DoNotOptimize(tasks);
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.SetLabel("n=" + std::to_string(n));
}

BENCHMARK_REGISTER_F(TaskSchedulerBenchFixture, ListTasks)
    ->Arg(10)
    ->Arg(50)
    ->Arg(100)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(1000);

// ============================================================================
// getStats overhead
// ============================================================================

BENCHMARK_DEFINE_F(TaskSchedulerBenchFixture, GetStats)(benchmark::State& state) {
    for (auto _ : state) {
        auto stats = scheduler_->getStats();
        benchmark::DoNotOptimize(stats);
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(TaskSchedulerBenchFixture, GetStats)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(5000);

// ============================================================================
// Concurrent registerTask from multiple threads
// ============================================================================

BENCHMARK_DEFINE_F(TaskSchedulerBenchFixture, ConcurrentRegister)(benchmark::State& state) {
    static std::atomic<int> counter{0};
    for (auto _ : state) {
        int id = counter.fetch_add(1, std::memory_order_relaxed);
        ScheduledTask task;
        task.name          = "concurrent_task_" + std::to_string(id);
        task.type          = ScheduledTask::TaskType::FUNCTION;
        task.function_name = "noop";
        task.trigger_type  = ScheduledTask::TriggerType::MANUAL;
        auto tid = scheduler_->registerTask(task);
        benchmark::DoNotOptimize(tid);
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(TaskSchedulerBenchFixture, ConcurrentRegister)
    ->Unit(benchmark::kMicrosecond)
    ->Threads(2)
    ->Iterations(2000);

BENCHMARK_MAIN();
