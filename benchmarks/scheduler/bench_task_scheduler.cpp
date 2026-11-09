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
/// This benchmark exercises production TaskScheduler APIs. Numeric release
/// gates remain undefined until an owner, stable hardware, baseline, and
/// threshold are agreed.

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
        static std::atomic<int> fixture_id{0};
        db_path_ = "./data/bench_task_scheduler_tmp_" +
                   std::to_string(fixture_id.fetch_add(1, std::memory_order_relaxed));
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
        TaskScheduler::RequestContext benchmark_context;
        benchmark_context.user_id = "scheduler-benchmark";
        benchmark_context.granted_permissions = {
            "task:register", "task:unregister", "task:update", "task:execute",
            "task:register_function"};
        benchmark_context.roles.insert("system_admin");
        TaskScheduler::setRequestContext(benchmark_context);
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
        TaskScheduler::clearRequestContext();
    }

    std::string                             db_path_;
    std::unique_ptr<RocksDBWrapper>         storage_;
    std::unique_ptr<SecondaryIndexManager>  idx_;
    std::unique_ptr<QueryEngine>            engine_;
    std::unique_ptr<TaskScheduler>          scheduler_;
};

struct ConcurrentTaskSchedulerBenchEnv {
    const std::string db_path = "./data/bench_task_scheduler_concurrent";
    std::unique_ptr<RocksDBWrapper> storage;
    std::unique_ptr<SecondaryIndexManager> index;
    std::unique_ptr<QueryEngine> engine;
    std::unique_ptr<TaskScheduler> scheduler;

    ConcurrentTaskSchedulerBenchEnv() {
        std::error_code cleanup_error;
        std::filesystem::remove_all(db_path, cleanup_error);
        std::filesystem::create_directories(db_path);

        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path + "/db";
        cfg.enable_blobdb = false;
        storage = std::make_unique<RocksDBWrapper>(cfg);
        if (!storage->open()) {
            throw std::runtime_error("Failed to open concurrent scheduler benchmark DB");
        }

        index = std::make_unique<SecondaryIndexManager>(*storage);
        engine = std::make_unique<QueryEngine>(*storage, *index);

        TaskScheduler::Config config;
        config.max_concurrent_tasks = 4;
        config.check_interval = 1h;
        config.persist_tasks = false;
        config.enable_audit_logging = false;
        config.enable_anomaly_detection = false;
        scheduler = std::make_unique<TaskScheduler>(engine.get(), config);
    }

    ~ConcurrentTaskSchedulerBenchEnv() {
        scheduler.reset();
        engine.reset();
        index.reset();
        if (storage) {
            storage->close();
        }
        storage.reset();
        std::error_code cleanup_error;
        std::filesystem::remove_all(db_path, cleanup_error);
    }
};

ConcurrentTaskSchedulerBenchEnv& concurrentTaskSchedulerBenchEnv() {
    static ConcurrentTaskSchedulerBenchEnv env;
    return env;
}

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

    constexpr int kTaskCount = 10;
    static std::atomic<int> run_counter{0};
    const int run_id = run_counter.fetch_add(1, std::memory_order_relaxed);
    std::vector<std::string> task_ids;
    task_ids.reserve(kTaskCount);
    for (int i = 0; i < kTaskCount; ++i) {
        ScheduledTask task;
        task.name          = "bench_exec_task_" + std::to_string(run_id) + "_" + std::to_string(i);
        task.type          = ScheduledTask::TaskType::FUNCTION;
        task.function_name = "bench_noop";
        task.trigger_type  = ScheduledTask::TriggerType::MANUAL;
        task_ids.push_back(scheduler_->registerTask(task));
    }

    size_t task_index = 0;
    for (auto _ : state) {
        const auto& task_id = task_ids[task_index++ % task_ids.size()];
        auto result = scheduler_->executeTaskNow(task_id);
        if (result.contains("error")) {
            state.SkipWithError(
                "TaskScheduler::executeTaskNow returned an error; refusing to time a failed call");
            break;
        }
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(TaskSchedulerBenchFixture, ExecuteTaskNow)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(10);

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
        try {
            scheduler_->registerTask(task);
        } catch (const std::exception& ex) {
            state.SkipWithError(ex.what());
            return;
        }
    }

    for (auto _ : state) {
        auto tasks = scheduler_->listTasks();
        const auto task_count = tasks.size();
        benchmark::DoNotOptimize(tasks.data());
        benchmark::DoNotOptimize(task_count);
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

static void BM_ConcurrentRegister(benchmark::State& state) {
    TaskScheduler::RequestContext benchmark_context;
    benchmark_context.user_id = "scheduler-concurrent-benchmark";
    benchmark_context.granted_permissions = {"task:register"};
    TaskScheduler::ScopedRequestContext request_context(benchmark_context);

    ConcurrentTaskSchedulerBenchEnv* env = nullptr;
    try {
        env = &concurrentTaskSchedulerBenchEnv();
    } catch (const std::exception& ex) {
        state.SkipWithError(ex.what());
        return;
    }

    static std::atomic<int> counter{0};
    for (auto _ : state) {
        int id = counter.fetch_add(1, std::memory_order_relaxed);
        ScheduledTask task;
        task.name          = "concurrent_task_" + std::to_string(id);
        task.type          = ScheduledTask::TaskType::FUNCTION;
        task.function_name = "noop";
        task.trigger_type  = ScheduledTask::TriggerType::MANUAL;
        auto tid = env->scheduler->registerTask(task);
        benchmark::DoNotOptimize(tid);
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_ConcurrentRegister)
    ->Unit(benchmark::kMicrosecond)
    ->Threads(2)
    ->Iterations(2000);

BENCHMARK_MAIN();
