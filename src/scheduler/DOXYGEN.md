# SCHEDULER DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\scheduler\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\scheduler\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 30
- Compounds: 100
- Classes/Structs: 49
- Namespaces: 13
- File Compounds: 30

## Namespaces
- benchmark
- std::chrono_literals
- stubs
- testing
- themis
- themis::@262022075307223267355245272301042141240011104262
- themis::@263151073113261221201171362260161316017003340177
- themis::bench
- themis::bench::sch
- themis::query
- themis::scheduler
- themis::scheduler::test
- themis::utils

## Types
### Classes
- SchedulerIntegrationTest
- TaskSchedulerBenchFixture
- ThreadPoolSaturationFixture
- themis::DistributedTaskCoordinator
- themis::EventTrigger
- themis::EventTriggerManager
- themis::HybridRetentionManager
- themis::TaskScheduler
- themis::scheduler::ExternalSchedulerAdapter
- themis::scheduler::TaskAnomalyDetector
- themis::scheduler::TaskAuditManager
- themis::scheduler::TaskResultStore
- themis::scheduler::test::SchedulerConcurrencyTest
- themis::scheduler::test::SchedulerCoordinationFailureTest
- themis::scheduler::test::SchedulerStressRetentionTest

### Structs
- stubs::DistributedCoordinator
- stubs::TaskRegistry
- stubs::TaskScheduler
- stubs::TriggerEngine
- themis::CDCTriggerConfig
- themis::DistributedTaskCoordinator::Config
- themis::DistributedTaskCoordinator::Stats
- themis::EventTrigger::CircuitBreakerConfig
- themis::EventTrigger::ParsedClause
- themis::EventTrigger::Stats
- themis::HybridRetentionConfig
- themis::HybridRetentionConfig::Stage1Config
- themis::HybridRetentionConfig::Stage2Config
- themis::HybridRetentionConfig::Stage3Config
- themis::HybridRetentionStats
- themis::ScheduledTask
- themis::ScheduledTask::CDCTrigger
- themis::ScheduledTask::RetryPolicy
- themis::ScheduledTask::SloRetryConfig
- themis::TaskScheduler::Config
- themis::TaskScheduler::DagExecutionResult
- themis::TaskScheduler::RequestContext
- themis::TaskScheduler::Stats
- themis::scheduler::AirflowDagConfig
- themis::scheduler::AnomalyDetectorConfig
- themis::scheduler::AnomalyMetrics
- themis::scheduler::AuditQueryParams
- themis::scheduler::KubernetesCronJobConfig
- themis::scheduler::TaskAuditConfig
- themis::scheduler::TaskAuditEvent
- themis::scheduler::TaskExecutionResult
- themis::scheduler::TaskResourceUsage
- themis::scheduler::TaskSecurityEvent
- themis::scheduler::TaskStatistics

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 347

### SchedulerIntegrationTest

#### `void SetUp() override`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:39
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:57
- Brief: n/a
- Parameters: none

#### `void makeScheduler(bool persist=false)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:71
- Brief: n/a
- Parameters:
  - `persist` (bool): n/a

#### `std::string registerCountingTask(std::atomic< int > &counter, const std::string &name="count_task")`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:86
- Brief: n/a
- Parameters:
  - `counter` (std::atomic< int > &): n/a
  - `name` (const std::string &): n/a

### TaskSchedulerBenchFixture

#### `void SetUp(const ::benchmark::State &) override`
- Source: `benchmarks/scheduler/bench_task_scheduler.cpp`:37
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &) override`
- Source: `benchmarks/scheduler/bench_task_scheduler.cpp`:64
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

### ThreadPoolSaturationFixture

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/scheduler/bench_thread_pool_saturation.cpp`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/scheduler/bench_thread_pool_saturation.cpp`:56
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

### bench_scheduler_dedicated_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/scheduler/bench_scheduler_dedicated_gates.cpp`:157
- Brief: n/a
- Parameters: none

#### `void BM_SC_BM_01_RegisterP95(benchmark::State &state)`
- Source: `benchmarks/scheduler/bench_scheduler_dedicated_gates.cpp`:73
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_SC_BM_02_ExecuteP95(benchmark::State &state)`
- Source: `benchmarks/scheduler/bench_scheduler_dedicated_gates.cpp`:95
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_SC_BM_03_ListP95(benchmark::State &state)`
- Source: `benchmarks/scheduler/bench_scheduler_dedicated_gates.cpp`:122
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_SC_BM_04_StatsQueryP95(benchmark::State &state)`
- Source: `benchmarks/scheduler/bench_scheduler_dedicated_gates.cpp`:144
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Threads(1) -> Threads(4) ->UseRealTime()`
- Source: `benchmarks/scheduler/bench_scheduler_dedicated_gates.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (1): n/a

### bench_scheduler_release_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/scheduler/bench_scheduler_release_gates.cpp`:163
- Brief: n/a
- Parameters: none

### bench_task_scheduler.cpp

#### `Arg(10) -> Arg(50) ->Arg(100) ->Unit(benchmark::kMicrosecond) ->Iterations(1000)`
- Source: `benchmarks/scheduler/bench_task_scheduler.cpp`:164
- Brief: n/a
- Parameters:
  - `<unnamed>` (10): n/a

#### `BENCHMARK_DEFINE_F(TaskSchedulerBenchFixture, ConcurrentRegister)(benchmark`
- Source: `benchmarks/scheduler/bench_task_scheduler.cpp`:190
- Brief: n/a
- Parameters:
  - `<unnamed>` (TaskSchedulerBenchFixture): n/a
  - `<unnamed>` (ConcurrentRegister): n/a

#### `BENCHMARK_DEFINE_F(TaskSchedulerBenchFixture, ExecuteTaskNow)(benchmark`
- Source: `benchmarks/scheduler/bench_task_scheduler.cpp`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (TaskSchedulerBenchFixture): n/a
  - `<unnamed>` (ExecuteTaskNow): n/a

#### `BENCHMARK_DEFINE_F(TaskSchedulerBenchFixture, GetStats)(benchmark`
- Source: `benchmarks/scheduler/bench_task_scheduler.cpp`:174
- Brief: n/a
- Parameters:
  - `<unnamed>` (TaskSchedulerBenchFixture): n/a
  - `<unnamed>` (GetStats): n/a

#### `BENCHMARK_DEFINE_F(TaskSchedulerBenchFixture, ListTasks)(benchmark`
- Source: `benchmarks/scheduler/bench_task_scheduler.cpp`:141
- Brief: n/a
- Parameters:
  - `<unnamed>` (TaskSchedulerBenchFixture): n/a
  - `<unnamed>` (ListTasks): n/a

#### `BENCHMARK_DEFINE_F(TaskSchedulerBenchFixture, RegisterUnregister)(benchmark`
- Source: `benchmarks/scheduler/bench_task_scheduler.cpp`:87
- Brief: n/a
- Parameters:
  - `<unnamed>` (TaskSchedulerBenchFixture): n/a
  - `<unnamed>` (RegisterUnregister): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/scheduler/bench_task_scheduler.cpp`:210
- Brief: n/a
- Parameters: none

#### `Unit(benchmark::kMicrosecond) -> Iterations(5000)`
- Source: `benchmarks/scheduler/bench_task_scheduler.cpp`:105
- Brief: n/a
- Parameters:
  - `<unnamed>` (benchmark::kMicrosecond): n/a

### bench_thread_pool_saturation.cpp

#### `Arg(1) -> Arg(2) ->Arg(4) ->Arg(8) ->Arg(16) ->UseRealTime()`
- Source: `benchmarks/scheduler/bench_thread_pool_saturation.cpp`:188
- Brief: n/a
- Parameters:
  - `<unnamed>` (1): n/a

#### `BENCHMARK(BM_ShutdownLatency) -> Arg(0) ->Arg(64) ->Arg(256) ->Arg(1024) ->UseRealTime()`
- Source: `benchmarks/scheduler/bench_thread_pool_saturation.cpp`:230
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ShutdownLatency): n/a

#### `BENCHMARK_F(ThreadPoolSaturationFixture, ConcurrentProducers)(benchmark`
- Source: `benchmarks/scheduler/bench_thread_pool_saturation.cpp`:156
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThreadPoolSaturationFixture): n/a
  - `<unnamed>` (ConcurrentProducers): n/a

#### `BENCHMARK_F(ThreadPoolSaturationFixture, PriorityOrdering_UnderLoad)(benchmark`
- Source: `benchmarks/scheduler/bench_thread_pool_saturation.cpp`:131
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThreadPoolSaturationFixture): n/a
  - `<unnamed>` (PriorityOrdering_UnderLoad): n/a

#### `BENCHMARK_F(ThreadPoolSaturationFixture, SaturatedQueue_DropRate)(benchmark`
- Source: `benchmarks/scheduler/bench_thread_pool_saturation.cpp`:101
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThreadPoolSaturationFixture): n/a
  - `<unnamed>` (SaturatedQueue_DropRate): n/a

#### `BENCHMARK_F(ThreadPoolSaturationFixture, StatisticsQuery)(benchmark`
- Source: `benchmarks/scheduler/bench_thread_pool_saturation.cpp`:193
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThreadPoolSaturationFixture): n/a
  - `<unnamed>` (StatisticsQuery): n/a

#### `BENCHMARK_F(ThreadPoolSaturationFixture, SubmitThroughput_CPU)(benchmark`
- Source: `benchmarks/scheduler/bench_thread_pool_saturation.cpp`:83
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThreadPoolSaturationFixture): n/a
  - `<unnamed>` (SubmitThroughput_CPU): n/a

#### `BENCHMARK_F(ThreadPoolSaturationFixture, SubmitThroughput_IO)(benchmark`
- Source: `benchmarks/scheduler/bench_thread_pool_saturation.cpp`:65
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThreadPoolSaturationFixture): n/a
  - `<unnamed>` (SubmitThroughput_IO): n/a

#### `void BM_ShutdownLatency(benchmark::State &state)`
- Source: `benchmarks/scheduler/bench_thread_pool_saturation.cpp`:203
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `ThreadPoolManager::Config makeSaturatedConfig(size_t max_threads=4, size_t queue_size=64)`
- Source: `benchmarks/scheduler/bench_thread_pool_saturation.cpp`:30
- Brief: n/a
- Parameters:
  - `max_threads` (size_t): n/a
  - `queue_size` (size_t): n/a

### stubs::DistributedCoordinator

#### `bool coordinate(uint64_t node_id, uint64_t task_id)`
- Source: `tests/scheduler/test_scheduler_highcardinality_stress.cpp`:50
- Brief: n/a
- Parameters:
  - `node_id` (uint64_t): n/a
  - `task_id` (uint64_t): n/a

### stubs::TaskRegistry

#### `bool register_task(uint64_t task_id, int priority)`
- Source: `tests/scheduler/test_scheduler_highcardinality_stress.cpp`:28
- Brief: n/a
- Parameters:
  - `task_id` (uint64_t): n/a
  - `priority` (int): n/a

### stubs::TaskScheduler

#### `bool execute_next()`
- Source: `benchmarks/scheduler/bench_scheduler_dedicated_gates.cpp`:43
- Brief: n/a
- Parameters: none

#### `uint64_t list_tasks()`
- Source: `benchmarks/scheduler/bench_scheduler_dedicated_gates.cpp`:51
- Brief: n/a
- Parameters: none

#### `uint64_t query_stats()`
- Source: `benchmarks/scheduler/bench_scheduler_dedicated_gates.cpp`:57
- Brief: n/a
- Parameters: none

#### `bool register_task(uint64_t id)`
- Source: `benchmarks/scheduler/bench_scheduler_dedicated_gates.cpp`:34
- Brief: n/a
- Parameters:
  - `id` (uint64_t): n/a

### stubs::TriggerEngine

#### `bool fire(uint64_t trigger_id)`
- Source: `tests/scheduler/test_scheduler_highcardinality_stress.cpp`:39
- Brief: n/a
- Parameters:
  - `trigger_id` (uint64_t): n/a

### test_scheduler_highcardinality_stress.cpp

#### `TEST(SchedulerHighCardinalityStress, ConcurrentTriggerStress)`
- Source: `tests/scheduler/test_scheduler_highcardinality_stress.cpp`:98
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerHighCardinalityStress): n/a
  - `<unnamed>` (ConcurrentTriggerStress): n/a

#### `TEST(SchedulerHighCardinalityStress, DistributedCoordinationLoad)`
- Source: `tests/scheduler/test_scheduler_highcardinality_stress.cpp`:131
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerHighCardinalityStress): n/a
  - `<unnamed>` (DistributedCoordinationLoad): n/a

#### `TEST(SchedulerHighCardinalityStress, HighCardinalityTaskRegistration)`
- Source: `tests/scheduler/test_scheduler_highcardinality_stress.cpp`:65
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerHighCardinalityStress): n/a
  - `<unnamed>` (HighCardinalityTaskRegistration): n/a

#### `int main(int argc, char **argv)`
- Source: `tests/scheduler/test_scheduler_highcardinality_stress.cpp`:163
- Brief: n/a
- Parameters:
  - `argc` (int): n/a
  - `argv` (char **): n/a

### test_scheduler_integration.cpp

#### `TEST_F(SchedulerIntegrationTest, ConcurrentTasksExecuteInParallel)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:315
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (ConcurrentTasksExecuteInParallel): n/a

#### `TEST_F(SchedulerIntegrationTest, CronTaskIsRegisteredAndHasValidExpression)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:516
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (CronTaskIsRegisteredAndHasValidExpression): n/a

#### `TEST_F(SchedulerIntegrationTest, EnableDisableTask)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:131
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (EnableDisableTask): n/a

#### `TEST_F(SchedulerIntegrationTest, ExportMetricsContainsRequiredMetricNames)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:218
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (ExportMetricsContainsRequiredMetricNames): n/a

#### `TEST_F(SchedulerIntegrationTest, ExportMetricsHasHelpAndTypeLines)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:247
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (ExportMetricsHasHelpAndTypeLines): n/a

#### `TEST_F(SchedulerIntegrationTest, ExportMetricsIsNotEmpty)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:213
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (ExportMetricsIsNotEmpty): n/a

#### `TEST_F(SchedulerIntegrationTest, ExportMetricsPrometheusFormatValid)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:253
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (ExportMetricsPrometheusFormatValid): n/a

#### `TEST_F(SchedulerIntegrationTest, ExportMetricsReflectsRegisteredTaskCount)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:226
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (ExportMetricsReflectsRegisteredTaskCount): n/a

#### `TEST_F(SchedulerIntegrationTest, ExportMetricsShowsDisabledTaskAsZero)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:700
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (ExportMetricsShowsDisabledTaskAsZero): n/a

#### `TEST_F(SchedulerIntegrationTest, ExportMetricsSuccessCounterIncrementsAfterExecution)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:237
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (ExportMetricsSuccessCounterIncrementsAfterExecution): n/a

#### `TEST_F(SchedulerIntegrationTest, FailedManualExecutionUpdatesFailureStats)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (FailedManualExecutionUpdatesFailureStats): n/a

#### `TEST_F(SchedulerIntegrationTest, GetStatsActiveVsRegistered)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:501
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (GetStatsActiveVsRegistered): n/a

#### `TEST_F(SchedulerIntegrationTest, GetStatsReflectsRunningState)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:476
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (GetStatsReflectsRunningState): n/a

#### `TEST_F(SchedulerIntegrationTest, GetStatsTotalExecutionsAggregatesAllTasks)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:749
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (GetStatsTotalExecutionsAggregatesAllTasks): n/a

#### `TEST_F(SchedulerIntegrationTest, IntervalTaskExecutedBySchedulerLoop)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:289
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (IntervalTaskExecutedBySchedulerLoop): n/a

#### `TEST_F(SchedulerIntegrationTest, InvalidCronExpressionThrowsOnRegister)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:536
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (InvalidCronExpressionThrowsOnRegister): n/a

#### `TEST_F(SchedulerIntegrationTest, ListTasksReturnsAllRegistered)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:144
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (ListTasksReturnsAllRegistered): n/a

#### `TEST_F(SchedulerIntegrationTest, ManualExecutionUpdatesStats)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:157
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (ManualExecutionUpdatesStats): n/a

#### `TEST_F(SchedulerIntegrationTest, ManualTriggerTaskNotExecutedBySchedulerLoop)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:671
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (ManualTriggerTaskNotExecutedBySchedulerLoop): n/a

#### `TEST_F(SchedulerIntegrationTest, MultipleManualExecutionsAccumulateStats)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:194
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (MultipleManualExecutionsAccumulateStats): n/a

#### `TEST_F(SchedulerIntegrationTest, OnFailureHookCalledAfterFailedExecution)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:573
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (OnFailureHookCalledAfterFailedExecution): n/a

#### `TEST_F(SchedulerIntegrationTest, OnSuccessHookCalledAfterSuccessfulExecution)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:554
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (OnSuccessHookCalledAfterSuccessfulExecution): n/a

#### `TEST_F(SchedulerIntegrationTest, PersistenceRoundTripRestoresRetryPolicy)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:401
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (PersistenceRoundTripRestoresRetryPolicy): n/a

#### `TEST_F(SchedulerIntegrationTest, PersistenceRoundTripRestoresTask)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:365
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (PersistenceRoundTripRestoresTask): n/a

#### `TEST_F(SchedulerIntegrationTest, RapidStartStopDoesNotCrash)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:719
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (RapidStartStopDoesNotCrash): n/a

#### `TEST_F(SchedulerIntegrationTest, RegisterAndUnregisterTask)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (RegisterAndUnregisterTask): n/a

#### `TEST_F(SchedulerIntegrationTest, SchedulerLoopAppliesRetryPolicy)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:444
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (SchedulerLoopAppliesRetryPolicy): n/a

#### `TEST_F(SchedulerIntegrationTest, StartStopIsIdempotent)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:111
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (StartStopIsIdempotent): n/a

#### `TEST_F(SchedulerIntegrationTest, UnregisteredFunctionReturnsErrorOnExecution)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:641
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (UnregisteredFunctionReturnsErrorOnExecution): n/a

#### `TEST_F(SchedulerIntegrationTest, UpdateTaskChangesDescription)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:597
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (UpdateTaskChangesDescription): n/a

#### `TEST_F(SchedulerIntegrationTest, UpdateTaskPreservesExecutionStats)`
- Source: `tests/scheduler/test_scheduler_integration.cpp`:615
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerIntegrationTest): n/a
  - `<unnamed>` (UpdateTaskPreservesExecutionStats): n/a

### themis

#### `ScheduledTask::ErrorCategory categorizeError(const std::string &error_message)`
- Source: `src/scheduler/task_scheduler.cpp`:422
- Brief: ── Error categorization helper ────────────────────────────────────────────── Classify a failure message into one of the ScheduledTask::ErrorCategory values.
- Parameters:
  - `error_message` (const std::string &): Input parameter.
- Return: Return value.
- Details: error_message Input parameter. Return value. This function is intentionally conservative: when in doubt it returns TRANSIENT so that the retry policy is not prematurely abandoned. Calls: find().

#### `std::string getTriggerTypeString(ScheduledTask::TriggerType type)`
- Source: `src/scheduler/task_scheduler.cpp`:248
- Brief: Helper function to convert trigger type to string.
- Parameters:
  - `type` (ScheduledTask::TriggerType): Input parameter.
- Return: Return value.
- Details: type Input parameter. Return value. Implements getTriggerTypeString without additional internal calls.

#### `void logUnauthorizedPermissionAttempt(const std::shared_ptr< scheduler::TaskAuditManager > &audit_manager, const std::string &task_id, const std::string &task_name, const std::string &required_permission, const std::string &operation, const std::string &reason)`
- Source: `src/scheduler/task_scheduler.cpp`:199
- Brief: Log Unauthorized Permission Attempt.
- Parameters:
  - `audit_manager` (const std::shared_ptr< scheduler::TaskAuditManager > &): Input parameter.
  - `task_id` (const std::string &): Identifier of the task.
  - `task_name` (const std::string &): Name of the task.
  - `required_permission` (const std::string &): Input parameter.
  - `operation` (const std::string &): Input parameter.
  - `reason` (const std::string &): Input parameter.
- Details: audit_manager Input parameter. task_id Identifier of the task. task_name Name of the task. required_permission Input parameter. operation Input parameter. reason Input parameter.

#### `void setDefaultAuditContext(scheduler::TaskAuditEvent &event)`
- Source: `src/scheduler/task_scheduler.cpp`:173
- Brief: Helper function to set audit context from thread-local RequestContext.
- Parameters:
  - `event` (scheduler::TaskAuditEvent &): Input/output parameter.
- Details: event Input/output parameter. Calls: TaskScheduler::currentUserId(), TaskScheduler::currentClientIp(), empty().

#### `void setDefaultAuditContext(scheduler::TaskSecurityEvent &event)`
- Source: `src/scheduler/task_scheduler.cpp`:184
- Brief: Set Default Audit Context.
- Parameters:
  - `event` (scheduler::TaskSecurityEvent &): Input/output parameter.
- Details: event Input/output parameter. Calls: TaskScheduler::currentUserId(), TaskScheduler::currentClientIp(), empty().

### themis::CDCTriggerConfig

#### `std::string getValidationError() const`
- Source: `include/scheduler/event_trigger.h`:39
- Brief: n/a
- Parameters: none

#### `bool isValid() const`
- Source: `include/scheduler/event_trigger.h`:38
- Brief: n/a
- Parameters: none

### themis::DistributedTaskCoordinator

#### `DistributedTaskCoordinator(TaskScheduler *scheduler, sharding::DistributedCoordinator *coordinator)`
- Source: `include/scheduler/distributed_task_coordinator.h`:83
- Brief: Construct a DistributedTaskCoordinator.
- Parameters:
  - `scheduler` (TaskScheduler *): Local single-node task scheduler. Must not be null. The scheduler must NOT already be running when the coordinator is started.
  - `coordinator` (sharding::DistributedCoordinator *): Gossip-based distributed coordinator used for leader election. Must not be null.
- Throws:
  - std::invalid_argument: if either scheduler or coordinator is null.
- Details: scheduler Local single-node task scheduler. Must not be null. The scheduler must NOT already be running when the coordinator is started. coordinator Gossip-based distributed coordinator used for leader election. Must not be null. std::invalid_argument if either scheduler or coordinator is null.

#### `DistributedTaskCoordinator(TaskScheduler *scheduler, sharding::DistributedCoordinator *coordinator, const Config &config)`
- Source: `include/scheduler/distributed_task_coordinator.h`:99
- Brief: Construct a DistributedTaskCoordinator.
- Parameters:
  - `scheduler` (TaskScheduler *): Local single-node task scheduler. Must not be null. The scheduler must NOT already be running when the coordinator is started.
  - `coordinator` (sharding::DistributedCoordinator *): Gossip-based distributed coordinator used for leader election. Must not be null.
  - `config` (const Config &): Optional runtime configuration.
- Throws:
  - std::invalid_argument: if either scheduler or coordinator is null.
- Details: scheduler Local single-node task scheduler. Must not be null. The scheduler must NOT already be running when the coordinator is started. coordinator Gossip-based distributed coordinator used for leader election. Must not be null. config Optional runtime configuration. std::invalid_argument if either scheduler or coordinator is null.

#### `DistributedTaskCoordinator(const DistributedTaskCoordinator &)=delete`
- Source: `include/scheduler/distributed_task_coordinator.h`:107
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DistributedTaskCoordinator &): n/a

#### `bool acquireLeadershipWithTimeout(std::chrono::milliseconds timeout_ms)`
- Source: `include/scheduler/distributed_task_coordinator.h`:287
- Brief: Attempt to acquire leadership with an explicit timeout.
- Parameters:
  - `timeout_ms` (std::chrono::milliseconds): Input parameter.
- Return: true if this node successfully acquired leadership, false otherwise. Note: false does NOT mean "not leader yet" but specifically that the acquisition attempt timed out or the coordinator is unavailable.
- Throws:
  - std::exception: on internal coordinator errors.
- Details: ── Coordination Health and Resilience ────────────────────────────────────── This method performs a synchronous leadership acquisition attempt against the underlying DistributedCoordinator, with a bounded timeout to detect coordination layer unavailability. If the coordinator is unreachable or fails to respond within the timeout, this method fails explicitly rather than blocking indefinitely. Used to detect and fail fast on coordination layer failures in production scenarios where the gossip network may be partitioned or degraded. timeout_ms Maximum milliseconds to wait for leadership acquisition. true if this node successfully acquired leadership, false otherwise. Note: false does NOT mean "not leader yet" but specifically that the acquisition attempt timed out or the coordinator is unavailable. std::exception on internal coordinator errors. timeout_ms Input parameter. True when the operation succeeds.

#### `void activateScheduler()`
- Source: `include/scheduler/distributed_task_coordinator.h`:166
- Brief: Manually activate the local scheduler.
- Parameters: none
- Details: ── Manual scheduler control ────────────────────────────────────────────────── Registers all locally stored tasks with the TaskScheduler and starts it. Normally called automatically when this node becomes leader; exposed here for testing and custom integration scenarios. Idempotent – calling it when the scheduler is already active is a no-op. Calls: exchange(), THEMIS_INFO(), lock(), size(), registerTask(), THEMIS_WARN(), what(), start().

#### `void deactivateScheduler()`
- Source: `include/scheduler/distributed_task_coordinator.h`:177
- Brief: Manually deactivate the local scheduler.
- Parameters: none
- Details: Deactivate Scheduler. Stops the TaskScheduler (waits for in-flight tasks to complete) and unregisters all tasks from it. The tasks remain in the local registry and will be re-registered the next time activateScheduler() is called. Idempotent – calling it when the scheduler is already inactive is a no-op. Calls: exchange(), THEMIS_INFO(), stop(), lock(), unregisterTask().

#### `void disableTask(const std::string &task_id)`
- Source: `include/scheduler/distributed_task_coordinator.h`:217
- Brief: Disable a task so it is not executed until re-enabled.
- Parameters:
  - `task_id` (const std::string &): Identifier of the task.
- Details: Disable Task. Updates the local registry. If this node is the leader, also disables the task in the active TaskScheduler. task_id Identifier of the task. Calls: lock(), find(), end(), load(), THEMIS_WARN(), what().

#### `void enableTask(const std::string &task_id)`
- Source: `include/scheduler/distributed_task_coordinator.h`:209
- Brief: Enable a previously disabled task.
- Parameters:
  - `task_id` (const std::string &): Identifier of the task.
- Details: Enable Task. Updates the local registry. If this node is the leader, also enables the task in the active TaskScheduler. task_id Identifier of the task. Calls: lock(), find(), end(), load(), THEMIS_WARN(), what().

#### `std::string generateId(const ScheduledTask &task)`
- Source: `include/scheduler/distributed_task_coordinator.h`:358
- Brief: Generate Id.
- Parameters:
  - `task` (const ScheduledTask &): Input parameter.
- Return: Return value.
- Details: task Input parameter. Return value. Calls: std::chrono::system_clock::now(), time_since_epoch(), count(), empty(), std::isalnum(), str().

#### `sharding::DistributedCoordinator * getCoordinator() const`
- Source: `include/scheduler/distributed_task_coordinator.h`:236
- Brief: n/a
- Parameters: none
- Return: non-owning pointer to the underlying DistributedCoordinator.
- Details: non-owning pointer to the underlying DistributedCoordinator.

#### `std::optional< std::string > getCurrentLeader() const`
- Source: `include/scheduler/distributed_task_coordinator.h`:148
- Brief: n/a
- Parameters: none
- Return: the node-id of the current cluster leader, or std::nullopt when no leader has been elected yet.
- Details: the node-id of the current cluster leader, or std::nullopt when no leader has been elected yet.

#### `std::string getLocalNodeId() const`
- Source: `include/scheduler/distributed_task_coordinator.h`:153
- Brief: n/a
- Parameters: none
- Return: the local node-id as reported by the DistributedCoordinator.
- Details: the local node-id as reported by the DistributedCoordinator.

#### `TaskScheduler * getScheduler() const`
- Source: `include/scheduler/distributed_task_coordinator.h`:233
- Brief: n/a
- Parameters: none
- Return: non-owning pointer to the underlying TaskScheduler.
- Details: non-owning pointer to the underlying TaskScheduler.

#### `Stats getStats() const`
- Source: `include/scheduler/distributed_task_coordinator.h`:248
- Brief: n/a
- Parameters: none

#### `std::shared_ptr< ScheduledTask > getTask(const std::string &task_id) const`
- Source: `include/scheduler/distributed_task_coordinator.h`:228
- Brief: Return a specific task from the local registry.
- Parameters:
  - `task_id` (const std::string &): n/a
- Return: Shared pointer to the task, or nullptr if not found.
- Details: Shared pointer to the task, or nullptr if not found.

#### `SchedulerError handleSplitBrainDetection()`
- Source: `include/scheduler/distributed_task_coordinator.h`:329
- Brief: Detect and handle split-brain / consensus failure scenarios.
- Parameters: none
- Return: SchedulerError::kSuccess if consensus is healthy, SchedulerError::kCoordinationError if split-brain detected, other SchedulerError codes for other failures.
- Details: Handle Split Brain Detection. Examines the current coordinator state to detect conditions where the distributed consensus mechanism has diverged (e.g., multiple leaders, replicas out of sync). When such a condition is detected: Logs the divergence with node IDs and timestamps Deactivates the local scheduler if this node is a leader Returns an error code indicating the consensus failure This is a fail-closed operation: in case of doubt, the scheduler stops. SchedulerError::kSuccess if consensus is healthy, SchedulerError::kCoordinationError if split-brain detected, other SchedulerError codes for other failures. scheduler_api_contract.h for error taxonomy. Return value.

#### `void heartbeatMonitorThread(std::chrono::milliseconds interval_ms)`
- Source: `include/scheduler/distributed_task_coordinator.h`:361
- Brief: Heartbeat Monitor Thread.
- Parameters:
  - `interval_ms` (std::chrono::milliseconds): Input parameter.
- Details: interval_ms Input parameter.

#### `bool isLeader() const`
- Source: `include/scheduler/distributed_task_coordinator.h`:142
- Brief: n/a
- Parameters: none
- Return: true if this node is currently the cluster leader and is actively scheduling tasks.
- Details: true if this node is currently the cluster leader and is actively scheduling tasks.

#### `bool isRunning() const`
- Source: `include/scheduler/distributed_task_coordinator.h`:134
- Brief: n/a
- Parameters: none
- Return: true if the coordinator has been started and not yet stopped.
- Details: true if the coordinator has been started and not yet stopped.

#### `bool isSchedulerActive() const`
- Source: `include/scheduler/distributed_task_coordinator.h`:180
- Brief: n/a
- Parameters: none
- Return: true if the local TaskScheduler is currently running.
- Details: true if the local TaskScheduler is currently running.

#### `std::vector< ScheduledTask > listTasks() const`
- Source: `include/scheduler/distributed_task_coordinator.h`:222
- Brief: Return all tasks in the local registry.
- Parameters: none

#### `bool maintainHeartbeat(std::chrono::milliseconds heartbeat_interval_ms)`
- Source: `include/scheduler/distributed_task_coordinator.h`:309
- Brief: Start a background heartbeat thread to detect coordinator failure.
- Parameters:
  - `heartbeat_interval_ms` (std::chrono::milliseconds): Input parameter.
- Return: true if heartbeat monitoring was successfully activated, false if it was already running or if activation failed.
- Details: Maintain Heartbeat. This method spawns a background thread that periodically checks the health of the underlying DistributedCoordinator. If the coordinator becomes unresponsive (heartbeat timeout), the method automatically deactivates the local scheduler to prevent split-brain execution. The heartbeat interval is derived from the coordinator's health check period. If this coordinator's running state becomes false, the heartbeat thread exits gracefully. This implements the fail-closed contract: if coordination is lost, the scheduler stops accepting new task executions. heartbeat_interval_ms Interval between heartbeat checks. true if heartbeat monitoring was successfully activated, false if it was already running or if activation failed. heartbeat_interval_ms Input parameter. True when the operation succeeds.

#### `void onLeaderElected(const std::string &leader_id)`
- Source: `include/scheduler/distributed_task_coordinator.h`:264
- Brief: Notify the coordinator that a new leader has been elected.
- Parameters:
  - `leader_id` (const std::string &): Identifier of the leader.
- Details: ── Private helpers ─────────────────────────────────────────────────────────── This method is called automatically via the DistributedCoordinator leadership callback. It is also exposed publicly to allow: Testing without a real gossip network (call directly with a synthetic leader ID) Custom integration in deployments that use alternative leader-election mechanisms leader_id Node-ID of the newly elected leader. leader_id Identifier of the leader. Calls: load(), getLocalShardId(), lock(), THEMIS_INFO(), fetch_add(), activateScheduler(), deactivateScheduler().

#### `DistributedTaskCoordinator & operator=(const DistributedTaskCoordinator &)=delete`
- Source: `include/scheduler/distributed_task_coordinator.h`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DistributedTaskCoordinator &): n/a

#### `std::string registerTask(const ScheduledTask &task)`
- Source: `include/scheduler/distributed_task_coordinator.h`:193
- Brief: Register a task with the coordinator.
- Parameters:
  - `task` (const ScheduledTask &): Input parameter.
- Return: Assigned task ID.
- Details: ── Task management ─────────────────────────────────────────────────────────── The task is stored in the local registry. If this node is the current leader, the task is also registered with the active TaskScheduler. task Task definition. Assigned task ID. task Input parameter. Return value. Calls: empty(), generateId(), lock(), load(), THEMIS_WARN(), what(), THEMIS_DEBUG().

#### `void start()`
- Source: `include/scheduler/distributed_task_coordinator.h`:123
- Brief: Start the coordinator.
- Parameters: none
- Details: ── Lifecycle ──────────────────────────────────────────────────────────────── Registers a leadership callback with the DistributedCoordinator. If this node is already the leader at start time, the TaskScheduler is activated immediately. The underlying DistributedCoordinator must already be running (or will be started externally). This method does NOT call DistributedCoordinator::start() on your behalf. Calls: exchange(), THEMIS_WARN(), setLeaderElectedCallback(), onLeaderElected(), isLeader(), THEMIS_INFO(), activateScheduler().

#### `void stop()`
- Source: `include/scheduler/distributed_task_coordinator.h`:131
- Brief: Stop the coordinator.
- Parameters: none
- Details: Stop. Deactivates the local scheduler (if running) and removes the leadership callback from the DistributedCoordinator. Safe to call multiple times. Calls: exchange(), lock(), notify_all(), joinable(), join(), deactivateScheduler(), setLeaderElectedCallback(), THEMIS_INFO().

#### `void unregisterTask(const std::string &task_id)`
- Source: `include/scheduler/distributed_task_coordinator.h`:201
- Brief: Unregister a task by ID.
- Parameters:
  - `task_id` (const std::string &): Identifier of the task.
- Details: Unregister Task. Removes the task from the local registry and, if this node is the leader, from the active TaskScheduler as well. task_id Identifier of the task. Calls: lock(), erase(), load(), THEMIS_WARN(), what().

#### `~DistributedTaskCoordinator() noexcept`
- Source: `include/scheduler/distributed_task_coordinator.h`:104
- Brief: n/a
- Parameters: none

### themis::EventTrigger

#### `EventTrigger(Changefeed *changefeed, const CDCTriggerConfig &config, TriggerCallback callback)`
- Source: `include/scheduler/event_trigger.h`:72
- Brief: Construct an event trigger.
- Parameters:
  - `changefeed` (Changefeed *): Changefeed instance to listen to (not owned, must remain valid)
  - `config` (const CDCTriggerConfig &): Trigger configuration
  - `callback` (TriggerCallback): Function to call when event matches
- Throws:
  - std::invalid_argument: if changefeed or callback is null, or config is invalid
- Details: changefeed Changefeed instance to listen to (not owned, must remain valid) config Trigger configuration callback Function to call when event matches std::invalid_argument if changefeed or callback is null, or config is invalid

#### `bool circuitAllows()`
- Source: `include/scheduler/event_trigger.h`:147
- Brief: Circuit Allows.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. Calls: lock(), std::chrono::steady_clock::now(), THEMIS_INFO().

#### `void circuitRecordFailure()`
- Source: `include/scheduler/event_trigger.h`:151
- Brief: Circuit Record Failure.
- Parameters: none
- Details: Calls: lock(), std::chrono::steady_clock::now(), THEMIS_WARN(), count().

#### `void circuitRecordSuccess()`
- Source: `include/scheduler/event_trigger.h`:149
- Brief: Circuit Record Success.
- Parameters: none
- Details: Calls: lock(), THEMIS_INFO().

#### `const CDCTriggerConfig & getConfig() const`
- Source: `include/scheduler/event_trigger.h`:84
- Brief: n/a
- Parameters: none

#### `Stats getStats() const`
- Source: `include/scheduler/event_trigger.h`:98
- Brief: n/a
- Parameters: none

#### `bool isRunning() const`
- Source: `include/scheduler/event_trigger.h`:81
- Brief: n/a
- Parameters: none

#### `void listenerLoop()`
- Source: `include/scheduler/event_trigger.h`:154
- Brief: Listener Loop.
- Parameters: none
- Details: Calls: THEMIS_DEBUG(), load(), getLatestSequence(), listEvents(), size(), matchesFilter(), shouldDebounce(), lock().

#### `bool matchesCondition(const Changefeed::ChangeEvent &event) const`
- Source: `include/scheduler/event_trigger.h`:160
- Brief: n/a
- Parameters:
  - `event` (const Changefeed::ChangeEvent &): n/a

#### `bool matchesEventType(Changefeed::ChangeEventType type) const`
- Source: `include/scheduler/event_trigger.h`:159
- Brief: n/a
- Parameters:
  - `type` (Changefeed::ChangeEventType): n/a

#### `bool matchesFilter(const Changefeed::ChangeEvent &event) const`
- Source: `include/scheduler/event_trigger.h`:157
- Brief: n/a
- Parameters:
  - `event` (const Changefeed::ChangeEvent &): n/a

#### `bool matchesKeyPrefix(const std::string &key) const`
- Source: `include/scheduler/event_trigger.h`:158
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `void rebuildConditionCache_() const`
- Source: `include/scheduler/event_trigger.h`:183
- Brief: n/a
- Parameters: none

#### `void setCircuitBreakerConfig(const CircuitBreakerConfig &config)`
- Source: `include/scheduler/event_trigger.h`:106
- Brief: Set Circuit Breaker Config.
- Parameters:
  - `config` (const CircuitBreakerConfig &): Input parameter.
- Details: config Input parameter. Calls: lock().

#### `bool shouldDebounce() const`
- Source: `include/scheduler/event_trigger.h`:163
- Brief: n/a
- Parameters: none

#### `void start()`
- Source: `include/scheduler/event_trigger.h`:79
- Brief: Start.
- Parameters: none
- Details: Calls: lock(), load(), THEMIS_WARN(), store(), std::thread(), THEMIS_INFO(), size().

#### `void stop()`
- Source: `include/scheduler/event_trigger.h`:80
- Brief: Stop.
- Parameters: none
- Details: Calls: lock(), load(), store(), notify_all(), joinable(), join(), THEMIS_INFO().

#### `void updateConfig(const CDCTriggerConfig &config)`
- Source: `include/scheduler/event_trigger.h`:85
- Brief: Update the access control configuration.
- Parameters:
  - `config` (const CDCTriggerConfig &): New access control configuration.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: config New access control configuration. std::invalid_argument if an error occurs. Calls: isValid(), getValidationError(), lock(), clock(), clear(), THEMIS_DEBUG(), size().

#### `bool validateNoCircularDependencies() const`
- Source: `include/scheduler/event_trigger.h`:166
- Brief: n/a
- Parameters: none

#### `~EventTrigger() noexcept`
- Source: `include/scheduler/event_trigger.h`:76
- Brief: n/a
- Parameters: none

### themis::EventTriggerManager

#### `EventTriggerManager(Changefeed *changefeed)`
- Source: `include/scheduler/event_trigger.h`:194
- Brief: n/a
- Parameters:
  - `changefeed` (Changefeed *): n/a

#### `std::optional< EventTrigger::Stats > getTriggerStats(const std::string &id) const`
- Source: `include/scheduler/event_trigger.h`:226
- Brief: Get trigger statistics.
- Parameters:
  - `id` (const std::string &): Trigger identifier
- Return: Statistics or nullopt if trigger not found
- Details: id Trigger identifier Statistics or nullopt if trigger not found

#### `bool registerTrigger(const std::string &id, const CDCTriggerConfig &config, EventTrigger::TriggerCallback callback)`
- Source: `include/scheduler/event_trigger.h`:204
- Brief: Register a new event trigger.
- Parameters:
  - `id` (const std::string &): Input parameter.
  - `config` (const CDCTriggerConfig &): Input parameter.
  - `callback` (EventTrigger::TriggerCallback): Input parameter.
- Return: True if registered successfully
- Details: Register Trigger. id Unique identifier for the trigger config Trigger configuration callback Callback function True if registered successfully id Input parameter. config Input parameter. callback Input parameter. True when the operation succeeds. Calls: lock(), find(), end(), THEMIS_WARN(), std::move(), start(), THEMIS_INFO(), THEMIS_ERROR().

#### `void startAll()`
- Source: `include/scheduler/event_trigger.h`:231
- Brief: Start all triggers.
- Parameters: none
- Details: Start All. Calls: lock(), isRunning(), start(), THEMIS_INFO(), size().

#### `void stopAll()`
- Source: `include/scheduler/event_trigger.h`:236
- Brief: Stop all triggers.
- Parameters: none
- Details: Stop All. Calls: lock(), isRunning(), stop(), THEMIS_INFO(), size().

#### `void unregisterTrigger(const std::string &id)`
- Source: `include/scheduler/event_trigger.h`:212
- Brief: Unregister an event trigger.
- Parameters:
  - `id` (const std::string &): Input parameter.
- Details: Unregister Trigger. id Trigger identifier id Input parameter. Calls: lock(), find(), end(), stop(), erase(), THEMIS_INFO().

#### `void updateTrigger(const std::string &id, const CDCTriggerConfig &config)`
- Source: `include/scheduler/event_trigger.h`:219
- Brief: Update trigger configuration.
- Parameters:
  - `id` (const std::string &): Input parameter.
  - `config` (const CDCTriggerConfig &): Input parameter.
- Details: Update Trigger. id Trigger identifier config New configuration id Input parameter. config Input parameter. Calls: lock(), find(), end(), updateConfig(), THEMIS_INFO().

#### `~EventTriggerManager() noexcept`
- Source: `include/scheduler/event_trigger.h`:195
- Brief: n/a
- Parameters: none

### themis::HybridRetentionManager

#### `HybridRetentionManager(QueryEngine *query_engine, TSStore *tsstore, TaskScheduler *scheduler, const HybridRetentionConfig &config=HybridRetentionConfig{})`
- Source: `include/scheduler/hybrid_retention_manager.h`:144
- Brief: Construct hybrid retention manager.
- Parameters:
  - `query_engine` (QueryEngine *): Query engine for AQL execution
  - `tsstore` (TSStore *): Time-series store for data access
  - `scheduler` (TaskScheduler *): Task scheduler for periodic execution
  - `config` (const HybridRetentionConfig &): Retention configuration
- Details: query_engine Query engine for AQL execution tsstore Time-series store for data access scheduler Task scheduler for periodic execution config Retention configuration

#### `nlohmann::json applyAdaptiveRetention(const nlohmann::json &params)`
- Source: `include/scheduler/hybrid_retention_manager.h`:201
- Brief: Apply Adaptive Retention.
- Parameters:
  - `params` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: params Input parameter. Return value. Calls: Tracer::startSpan(), value(), executeAql(), str(), THEMIS_ERROR(), error(), message(), is_array().

#### `nlohmann::json applyTimeBasedRetention(const nlohmann::json &params)`
- Source: `include/scheduler/hybrid_retention_manager.h`:202
- Brief: Apply Time Based Retention.
- Parameters:
  - `params` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: params Input parameter. Return value. Calls: Tracer::startSpan(), value(), executeAql(), str(), THEMIS_ERROR(), error(), message(), is_array().

#### `nlohmann::json cleanupOriginalData(const nlohmann::json &params)`
- Source: `include/scheduler/hybrid_retention_manager.h`:203
- Brief: Cleanup Original Data.
- Parameters:
  - `params` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: params Input parameter. Return value. Calls: Tracer::startSpan(), value(), count(), executeAql(), str(), THEMIS_ERROR(), error(), message().

#### `nlohmann::json compressWithGorilla(const nlohmann::json &params)`
- Source: `include/scheduler/hybrid_retention_manager.h`:200
- Brief: ===== Stage Implementations =====
- Parameters:
  - `params` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: params Input parameter. Return value. Calls: Tracer::startSpan(), value(), executeAql(), str(), THEMIS_ERROR(), error(), message(), add().

#### `void executeAll()`
- Source: `include/scheduler/hybrid_retention_manager.h`:166
- Brief: Execute All.
- Parameters: none
- Details: Calls: Tracer::startSpan(), THEMIS_INFO(), executeStage1(), executeStage2(), executeStage3(), empty(), executeTaskNow().

#### `void executeStage1()`
- Source: `include/scheduler/hybrid_retention_manager.h`:163
- Brief: ===== Manual Execution =====
- Parameters: none
- Details: Calls: Tracer::startSpan(), THEMIS_INFO(), empty(), executeTaskNow().

#### `void executeStage2()`
- Source: `include/scheduler/hybrid_retention_manager.h`:164
- Brief: Execute Stage2.
- Parameters: none
- Details: Calls: Tracer::startSpan(), THEMIS_INFO(), empty(), executeTaskNow().

#### `void executeStage3()`
- Source: `include/scheduler/hybrid_retention_manager.h`:165
- Brief: Execute Stage3.
- Parameters: none
- Details: Calls: Tracer::startSpan(), THEMIS_INFO(), empty(), executeTaskNow().

#### `HybridRetentionConfig getConfig() const`
- Source: `include/scheduler/hybrid_retention_manager.h`:160
- Brief: n/a
- Parameters: none

#### `HybridRetentionStats getStats() const`
- Source: `include/scheduler/hybrid_retention_manager.h`:169
- Brief: n/a
- Parameters: none

#### `nlohmann::json getStatusReport() const`
- Source: `include/scheduler/hybrid_retention_manager.h`:173
- Brief: n/a
- Parameters: none

#### `bool isRunning() const`
- Source: `include/scheduler/hybrid_retention_manager.h`:156
- Brief: n/a
- Parameters: none

#### `void resetStats()`
- Source: `include/scheduler/hybrid_retention_manager.h`:170
- Brief: Reset Stats.
- Parameters: none
- Details: Calls: lock(), THEMIS_INFO().

#### `void setupCleanupTasks()`
- Source: `include/scheduler/hybrid_retention_manager.h`:197
- Brief: Setup Cleanup Tasks.
- Parameters: none
- Details: Calls: registerFunction(), cleanupOriginalData(), std::chrono::hours(), registerTask(), THEMIS_INFO().

#### `void setupStage1Tasks()`
- Source: `include/scheduler/hybrid_retention_manager.h`:194
- Brief: ===== Stage Setup =====
- Parameters: none
- Details: Calls: registerFunction(), compressWithGorilla(), count(), updateStats(), registerTask(), THEMIS_INFO().

#### `void setupStage2Tasks()`
- Source: `include/scheduler/hybrid_retention_manager.h`:195
- Brief: Setup Stage2 Tasks.
- Parameters: none
- Details: Calls: registerFunction(), applyAdaptiveRetention(), count(), updateStats(), registerTask(), THEMIS_INFO().

#### `void setupStage3Tasks()`
- Source: `include/scheduler/hybrid_retention_manager.h`:196
- Brief: Setup Stage3 Tasks.
- Parameters: none
- Details: Calls: registerFunction(), applyTimeBasedRetention(), count(), updateStats(), registerTask(), THEMIS_INFO().

#### `void start()`
- Source: `include/scheduler/hybrid_retention_manager.h`:154
- Brief: ===== Lifecycle =====
- Parameters: none
- Details: Calls: lock(), THEMIS_WARN(), THEMIS_INFO(), setupStage1Tasks(), setupStage2Tasks(), setupStage3Tasks(), setupCleanupTasks().

#### `void stop()`
- Source: `include/scheduler/hybrid_retention_manager.h`:155
- Brief: Stop.
- Parameters: none
- Details: Calls: lock(), THEMIS_INFO(), empty(), unregisterTask().

#### `void updateConfig(const HybridRetentionConfig &config)`
- Source: `include/scheduler/hybrid_retention_manager.h`:159
- Brief: ===== Configuration =====
- Parameters:
  - `config` (const HybridRetentionConfig &): New access control configuration.
- Details: config New access control configuration. Calls: lock(), THEMIS_INFO().

#### `void updateStats(int stage, bool success, const nlohmann::json &result)`
- Source: `include/scheduler/hybrid_retention_manager.h`:205
- Brief: Update Stats.
- Parameters:
  - `stage` (int): Input parameter.
  - `success` (bool): Input parameter.
  - `result` (const nlohmann::json &): Input parameter.
- Details: stage Input parameter. success Input parameter. result Input parameter. Calls: lock(), contains(), THEMIS_WARN(), std::chrono::system_clock::now().

#### `~HybridRetentionManager() noexcept`
- Source: `include/scheduler/hybrid_retention_manager.h`:151
- Brief: n/a
- Parameters: none

### themis::TaskScheduler

#### `TaskScheduler(QueryEngine *query_engine, const Config &config, Changefeed *changefeed=nullptr, std::shared_ptr< utils::AuditLogger > audit_logger=nullptr, RocksDBWrapper *result_storage=nullptr)`
- Source: `include/scheduler/task_scheduler.h`:426
- Brief: Construct a task scheduler.
- Parameters:
  - `query_engine` (QueryEngine *): Query engine for executing AQL queries
  - `config` (const Config &): Scheduler configuration
  - `changefeed` (Changefeed *): Optional changefeed for CDC event triggers (nullptr = no CDC support)
  - `audit_logger` (std::shared_ptr< utils::AuditLogger >): Optional audit logger for tamper-evident logging (nullptr = basic logging)
  - `result_storage` (RocksDBWrapper *): Optional RocksDB instance used to persist task execution results. Required when config.enable_result_store == true. Must outlive this TaskScheduler instance.
- Details: query_engine Query engine for executing AQL queries config Scheduler configuration changefeed Optional changefeed for CDC event triggers (nullptr = no CDC support) audit_logger Optional audit logger for tamper-evident logging (nullptr = basic logging) result_storage Optional RocksDB instance used to persist task execution results. Required when config.enable_result_store == true. Must outlive this TaskScheduler instance. Note: The optional parameters maintain backward compatibility. Existing code using TaskScheduler(query_engine, config) continues to work. New code can add changefeed for CDC event trigger support, audit_logger for comprehensive auditing, and result_storage for persistent task output storage.

#### `void adjustConcurrencyLimit(size_t pending_count) noexcept`
- Source: `include/scheduler/task_scheduler.h`:782
- Brief: n/a
- Parameters:
  - `pending_count` (size_t): n/a

#### `bool checkRateLimit(const std::string &task_id)`
- Source: `include/scheduler/task_scheduler.h`:766
- Brief: Check whether a user exceeds the current rate limit.
- Parameters:
  - `task_id` (const std::string &): Identifier of the task.
- Return: True when the user remains within the configured limit.
- Details: task_id Identifier of the task. True when the user remains within the configured limit. Calls: lock(), std::chrono::steady_clock::now(), std::chrono::minutes(), empty(), front(), pop_front(), size(), scheduler::generateUUID().

#### `void clearRequestContext() noexcept`
- Source: `include/scheduler/task_scheduler.h`:400
- Brief: Clear the authentication context for the calling thread.
- Parameters: none

#### `std::string currentAuthorizationJustification(const char *fallback="") noexcept`
- Source: `include/scheduler/task_scheduler.h`:409
- Brief: n/a
- Parameters:
  - `fallback` (const char *): n/a

#### `std::string currentClientIp() noexcept`
- Source: `include/scheduler/task_scheduler.h`:406
- Brief: Return the client IP from the thread-local request context (empty if not set).
- Parameters: none

#### `std::string currentUserId(const char *fallback="system") noexcept`
- Source: `include/scheduler/task_scheduler.h`:403
- Brief: Return the user ID from the thread-local request context, or fallback.
- Parameters:
  - `fallback` (const char *): n/a

#### `void disableTask(const std::string &task_id)`
- Source: `include/scheduler/task_scheduler.h`:465
- Brief: Disable a task (will not be executed until re-enabled).
- Parameters:
  - `task_id` (const std::string &): Identifier of the task.
- Details: Disable Task. task_id Task ID to disable task_id Identifier of the task. Calls: lock(), find(), end(), THEMIS_INFO(), logTaskSchedulerEvent(), TaskScheduler::currentUserId(), saveTasks().

#### `void enableTask(const std::string &task_id)`
- Source: `include/scheduler/task_scheduler.h`:459
- Brief: Enable a disabled task.
- Parameters:
  - `task_id` (const std::string &): Identifier of the task.
- Details: Enable Task. task_id Task ID to enable task_id Identifier of the task. Calls: lock(), find(), end(), THEMIS_INFO(), logTaskSchedulerEvent(), TaskScheduler::currentUserId(), saveTasks().

#### `void enforceQueryComplexityLimits(const std::string &aql) const`
- Source: `include/scheduler/task_scheduler.h`:765
- Brief: n/a
- Parameters:
  - `aql` (const std::string &): n/a

#### `nlohmann::json executeAqlQuery(const std::string &aql)`
- Source: `include/scheduler/task_scheduler.h`:734
- Brief: Execute Aql Query.
- Parameters:
  - `aql` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: aql Input parameter. Return value. std::runtime_error if an error occurs. Calls: validateAqlQuery(), enforceQueryComplexityLimits(), Tracer::startSpan(), setAttribute(), executeAql(), error(), message().

#### `DagExecutionResult executeDAG(const std::vector< std::string > &task_ids)`
- Source: `include/scheduler/task_scheduler.h`:518
- Brief: Execute a set of registered tasks respecting their dependency order.
- Parameters:
  - `task_ids` (const std::vector< std::string > &): Input parameter.
- Return: DagExecutionResult with per-task outcomes (succeeded, failed, skipped, condition_skipped).
- Throws:
  - std::invalid_argument: if task_ids contains an unknown task ID.
  - std::runtime_error: if the dependency graph contains a cycle.
- Details: Execute DAG. Tasks are executed in topological order derived from each task's dependencies list. Tasks whose dependencies have all succeeded are dispatched in parallel (up to max_concurrent_tasks). If a task fails, all tasks that (transitively) depend on it are skipped rather than executed. If a task's branch_condition predicate returns false, the task is condition-skipped and its dependents are condition-skipped transitively (reported in DagExecutionResult::condition_skipped). task_ids IDs of the tasks to include in this DAG execution. Tasks not in this set are ignored even if they appear in a dependency list. DagExecutionResult with per-task outcomes (succeeded, failed, skipped, condition_skipped). std::invalid_argument if task_ids contains an unknown task ID. std::runtime_error if the dependency graph contains a cycle. ⚠️ SECURITY: This method MUST be protected by authentication and authorization. task_ids Input parameter. Return value.

#### `nlohmann::json executeFunction(const std::string &name, const nlohmann::json &params)`
- Source: `include/scheduler/task_scheduler.h`:735
- Brief: Execute Function.
- Parameters:
  - `name` (const std::string &): Input parameter.
  - `params` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: name Input parameter. params Input parameter. Return value. std::runtime_error if an error occurs. Calls: Tracer::startSpan(), setAttribute(), find(), end(), sandbox(), launch(), THEMIS_WARN(), lastError().

#### `void executeTask(std::shared_ptr< ScheduledTask > task)`
- Source: `include/scheduler/task_scheduler.h`:733
- Brief: Execute Task.
- Parameters:
  - `task` (std::shared_ptr< ScheduledTask >): Input parameter.
- Details: task Input parameter. Calls: Tracer::startSpan(), setAttribute(), getTaskExecutionLock(), exec_lock(), std::chrono::steady_clock::now(), getCurrentTimeMs(), scheduler::generateUUID(), std::chrono::system_clock::now().

#### `nlohmann::json executeTaskNow(const std::string &task_id)`
- Source: `include/scheduler/task_scheduler.h`:485
- Brief: Execute a task immediately (out-of-schedule).
- Parameters:
  - `task_id` (const std::string &): Identifier of the task.
- Return: Execution result as JSON
- Details: ===== Manual Execution ===== task_id Task ID to execute Execution result as JSON ⚠️ SECURITY: This method MUST be protected by authentication and authorization. Can be abused for DoS attacks or unauthorized data access. task_id Identifier of the task. Return value. Calls: hasPermission(), logUnauthorizedPermissionAttempt(), THEMIS_INFO(), checkRateLimit(), THEMIS_WARN(), Tracer::startSpan(), setAttribute(), lock().

#### `std::string exportMetrics() const`
- Source: `include/scheduler/task_scheduler.h`:573
- Brief: Export current scheduler metrics in Prometheus text format.
- Parameters: none
- Return: Prometheus text exposition string (never empty)
- Details: Returns a string in Prometheus exposition format (text/plain; version=0.0.4) suitable for scraping by a Prometheus server or any compatible monitoring tool. Metrics exported: themis_scheduler_tasks_registered (gauge) themis_scheduler_tasks_active (gauge) themis_scheduler_tasks_running (gauge) themis_scheduler_executions_total (counter, label: status=success\|failure) themis_scheduler_task_executions_total (counter, per task, labels: task_id, task_name, status) themis_scheduler_task_execution_duration_ms (gauge, per task) themis_scheduler_task_last_run_timestamp (gauge, per task, unix seconds) Prometheus text exposition string (never empty)

#### `void fireTaskFailureAlert(const ScheduledTask &task, const std::string &error)`
- Source: `include/scheduler/task_scheduler.h`:776
- Brief: Fire Task Failure Alert.
- Parameters:
  - `task` (const ScheduledTask &): Input parameter.
  - `error` (const std::string &): Input parameter.
- Details: task Input parameter. error Input parameter. Calls: lock(), makeTaskAlertId(), std::to_string(), sendAlert(), THEMIS_WARN(), THEMIS_ERROR(), error(), message().

#### `void fireTaskSlaBreachAlert(const ScheduledTask &task, double elapsed_ms)`
- Source: `include/scheduler/task_scheduler.h`:777
- Brief: Fire Task Sla Breach Alert.
- Parameters:
  - `task` (const ScheduledTask &): Input parameter.
  - `elapsed_ms` (double): Input parameter.
- Details: task Input parameter. elapsed_ms Input parameter. Calls: has_value(), lock(), makeTaskAlertId(), count(), str(), std::to_string(), sendAlert(), THEMIS_WARN().

#### `std::string generateTaskId(const ScheduledTask &task) const`
- Source: `include/scheduler/task_scheduler.h`:757
- Brief: n/a
- Parameters:
  - `task` (const ScheduledTask &): n/a

#### `std::shared_ptr< observability::Alertmanager > getAlertmanager() const`
- Source: `include/scheduler/task_scheduler.h`:651
- Brief: Get the currently configured alertmanager (may be nullptr).
- Parameters: none

#### `std::shared_ptr< scheduler::TaskAuditManager > getAuditManager() const`
- Source: `include/scheduler/task_scheduler.h`:591
- Brief: Get audit manager for querying audit events.
- Parameters: none
- Return: Shared pointer to audit manager (may be nullptr if audit logging disabled)
- Details: Shared pointer to audit manager (may be nullptr if audit logging disabled)

#### `std::shared_ptr< CronExpression > getCronExpression(const std::string &task_id)`
- Source: `include/scheduler/task_scheduler.h`:748
- Brief: Get Cron Expression.
- Parameters:
  - `task_id` (const std::string &): Identifier of the task.
- Return: Return value.
- Details: task_id Identifier of the task. Return value. Calls: find(), end().

#### `int64_t getCurrentTimeMs() const`
- Source: `include/scheduler/task_scheduler.h`:756
- Brief: n/a
- Parameters: none

#### `size_t getDynamicConcurrencyLimit() const noexcept`
- Source: `include/scheduler/task_scheduler.h`:670
- Brief: Get the current effective max-concurrent-tasks limit.
- Parameters: none
- Details: When enable_dynamic_scaling is false this equals Config::max_concurrent_tasks. When scaling is enabled it reflects the dynamically adjusted value in the range [min_concurrent_tasks, max_concurrent_tasks_ceil].

#### `std::vector< scheduler::TaskAuditEvent > getExecutionHistory(const std::string &task_id="", size_t limit=100, size_t offset=0) const`
- Source: `include/scheduler/task_scheduler.h`:608
- Brief: Get execution history for a specific task (or all tasks).
- Parameters:
  - `task_id` (const std::string &): Task ID to filter on (empty string = all tasks)
  - `limit` (size_t): Maximum number of results to return (default 100)
  - `offset` (size_t): Pagination offset (default 0)
- Return: Vector of audit events ordered by timestamp descending, or empty vector if audit logging is disabled
- Details: Convenience wrapper around TaskAuditManager::queryAuditEvents() that pre-populates the task_id filter and sensible defaults for browsing the searchable audit log. task_id Task ID to filter on (empty string = all tasks) limit Maximum number of results to return (default 100) offset Pagination offset (default 0) Vector of audit events ordered by timestamp descending, or empty vector if audit logging is disabled

#### `std::optional< scheduler::TaskExecutionResult > getLatestTaskResult(const std::string &task_id) const`
- Source: `include/scheduler/task_scheduler.h`:632
- Brief: Retrieve the most-recent execution result for a task.
- Parameters:
  - `task_id` (const std::string &): Task identifier.
- Details: Returns std::nullopt if result storage is disabled or no results exist. task_id Task identifier.

#### `size_t getQueueDepth() const noexcept`
- Source: `include/scheduler/task_scheduler.h`:660
- Brief: Get the number of tasks that were ready to run on the last scheduler tick but could not be dispatched because the concurrency limit was reached.
- Parameters: none
- Details: Always returns 0 when enable_dynamic_scaling is false.

#### `Stats getStats() const`
- Source: `include/scheduler/task_scheduler.h`:554
- Brief: Get scheduler statistics.
- Parameters: none

#### `std::shared_ptr< ScheduledTask > getTask(const std::string &task_id) const`
- Source: `include/scheduler/task_scheduler.h`:585
- Brief: Get details of a specific task.
- Parameters:
  - `task_id` (const std::string &): Task ID
- Return: Task details or nullptr if not found
- Details: task_id Task ID Task details or nullptr if not found

#### `std::mutex & getTaskExecutionLock(const std::string &task_id)`
- Source: `include/scheduler/task_scheduler.h`:786
- Brief: ===== Task Execution Serialization (Phase 3 Hardening) =====
- Parameters:
  - `task_id` (const std::string &): Identifier of the task.
- Return: Return value.
- Details: task_id Identifier of the task. Return value. Calls: lock(), find(), end().

#### `std::vector< scheduler::TaskExecutionResult > getTaskResults(const std::string &task_id, size_t limit=10) const`
- Source: `include/scheduler/task_scheduler.h`:622
- Brief: Retrieve recent execution results for a task from the result store.
- Parameters:
  - `task_id` (const std::string &): Task identifier.
  - `limit` (size_t): Maximum number of records to return (default: 10).
- Details: Returns up to limit results, newest first. Returns an empty vector if result storage is disabled or no results exist. task_id Task identifier. limit Maximum number of records to return (default: 10).

#### `bool hasPermission(const std::string &permission) noexcept`
- Source: `include/scheduler/task_scheduler.h`:407
- Brief: n/a
- Parameters:
  - `permission` (const std::string &): n/a

#### `bool hasRole(const std::string &role) noexcept`
- Source: `include/scheduler/task_scheduler.h`:408
- Brief: n/a
- Parameters:
  - `role` (const std::string &): n/a

#### `bool isRunning() const`
- Source: `include/scheduler/task_scheduler.h`:436
- Brief: n/a
- Parameters: none

#### `std::vector< ScheduledTask > listTasks() const`
- Source: `include/scheduler/task_scheduler.h`:578
- Brief: List all registered tasks.
- Parameters: none

#### `void loadTasks()`
- Source: `include/scheduler/task_scheduler.h`:753
- Brief: Load Tasks.
- Parameters: none
- Details: Calls: file(), good(), THEMIS_DEBUG(), value(), nlohmann::json::object(), std::chrono::milliseconds(), contains(), insert().

#### `std::string makeTaskAlertId(const std::string &task_id, const std::string &alert_type)`
- Source: `include/scheduler/task_scheduler.h`:779
- Brief: n/a
- Parameters:
  - `task_id` (const std::string &): n/a
  - `alert_type` (const std::string &): n/a

#### `void onCDCEvent(std::shared_ptr< ScheduledTask > task, const Changefeed::ChangeEvent &event)`
- Source: `include/scheduler/task_scheduler.h`:745
- Brief: On CDCEvent.
- Parameters:
  - `task` (std::shared_ptr< ScheduledTask >): Input parameter.
  - `event` (const Changefeed::ChangeEvent &): Input parameter.
- Details: task Input parameter. event Input parameter. Calls: THEMIS_DEBUG(), logTaskSchedulerEvent(), TaskScheduler::currentUserId(), load(), lock(), task_thread(), executeTask(), erase().

#### `void registerFunction(const std::string &name, TaskFunction func)`
- Source: `include/scheduler/task_scheduler.h`:532
- Brief: Register a custom function that can be called by tasks.
- Parameters:
  - `name` (const std::string &): Input parameter.
  - `func` (TaskFunction): Input parameter.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: ===== Function Registration ===== name Function name func Function implementation ⚠️ SECURITY CRITICAL: This allows arbitrary code execution. Only allow registration by system administrators. Functions should be sandboxed and resource-limited. name Input parameter. func Input parameter. std::runtime_error if an error occurs. Calls: hasPermission(), hasRole(), logUnauthorizedPermissionAttempt(), THEMIS_INFO(), lock().

#### `std::string registerTask(const ScheduledTask &task)`
- Source: `include/scheduler/task_scheduler.h`:447
- Brief: Register a new scheduled task.
- Parameters:
  - `task` (const ScheduledTask &): Input parameter.
- Return: Task ID
- Throws:
  - std::runtime_error: if an error occurs.
  - std::invalid_argument: if an error occurs.
- Details: ===== Task Management ===== task Task configuration Task ID ⚠️ SECURITY: This method MUST be protected by authentication and authorization. Validate and sanitize all task inputs before registration. task Input parameter. Return value. std::runtime_error if an error occurs. std::invalid_argument if an error occurs. Calls: hasPermission(), logUnauthorizedPermissionAttempt(), validateAqlQuery(), validateResourceLimits(), validateCronExpression(), validateCDCTrigger(), sanitizeTask(), lock().

#### `void removeEventTrigger(const std::string &task_id)`
- Source: `include/scheduler/task_scheduler.h`:744
- Brief: Remove Event Trigger.
- Parameters:
  - `task_id` (const std::string &): Identifier of the task.
- Details: task_id Identifier of the task. Calls: unregisterTrigger().

#### `void resolveTaskFailureAlert(const std::string &task_id)`
- Source: `include/scheduler/task_scheduler.h`:778
- Brief: Resolve Task Failure Alert.
- Parameters:
  - `task_id` (const std::string &): Identifier of the task.
- Details: task_id Identifier of the task. Calls: lock(), find(), end(), erase(), resolveAlert(), THEMIS_INFO(), THEMIS_WARN(), error().

#### `ScheduledTask sanitizeTask(const ScheduledTask &task) const`
- Source: `include/scheduler/task_scheduler.h`:764
- Brief: n/a
- Parameters:
  - `task` (const ScheduledTask &): n/a

#### `void saveTasks()`
- Source: `include/scheduler/task_scheduler.h`:752
- Brief: ===== Persistence =====
- Parameters: none
- Throws:
  - std::runtime_error: if an error occurs.
- Details: std::runtime_error if an error occurs. Calls: nlohmann::json::array(), count(), push_back(), file(), good(), dump(), close(), chmod().

#### `void schedulerLoop()`
- Source: `include/scheduler/task_scheduler.h`:730
- Brief: ===== Scheduler Loop =====
- Parameters: none
- Details: Calls: THEMIS_INFO(), load(), Tracer::startSpan(), std::chrono::system_clock::now(), lock(), shouldExecute(), THEMIS_DEBUG(), logTaskSchedulerEvent().

#### `void setAlertmanager(std::shared_ptr< observability::Alertmanager > alertmanager)`
- Source: `include/scheduler/task_scheduler.h`:646
- Brief: Set the alertmanager for dispatching task failure and SLA breach alerts.
- Parameters:
  - `alertmanager` (std::shared_ptr< observability::Alertmanager >): Input parameter.
- Details: ===== Alertmanager Integration ===== When set, the scheduler fires an alert via the alertmanager whenever: A task fails all execution attempts (TaskFailure alert). A task execution exceeds its configured sla_deadline (TaskSlaBreached alert). A previously-fired failure alert is automatically resolved when the same task subsequently succeeds. Pass nullptr to disable alertmanager integration (default behaviour). alertmanager Input parameter. Calls: lock(), std::move().

#### `void setRequestContext(const RequestContext &ctx) noexcept`
- Source: `include/scheduler/task_scheduler.h`:397
- Brief: n/a
- Parameters:
  - `ctx` (const RequestContext &): n/a
- Details: Set the authentication context for the calling thread. Must be called before any scheduler method that performs audit logging. Thread-safe (each thread owns its own context slot).

#### `void setupEventTrigger(std::shared_ptr< ScheduledTask > task)`
- Source: `include/scheduler/task_scheduler.h`:743
- Brief: Setup Event Trigger.
- Parameters:
  - `task` (std::shared_ptr< ScheduledTask >): Input parameter.
- Details: task Input parameter. Calls: THEMIS_ERROR(), insert(), lock(), find(), end(), THEMIS_DEBUG(), onCDCEvent(), registerTrigger().

#### `bool shouldExecute(const ScheduledTask &task, const std::chrono::system_clock::time_point &now) const`
- Source: `include/scheduler/task_scheduler.h`:738
- Brief: n/a
- Parameters:
  - `task` (const ScheduledTask &): n/a
  - `now` (const std::chrono::system_clock::time_point &): n/a

#### `bool shouldExecuteCron(const ScheduledTask &task, const std::chrono::system_clock::time_point &now) const`
- Source: `include/scheduler/task_scheduler.h`:740
- Brief: n/a
- Parameters:
  - `task` (const ScheduledTask &): n/a
  - `now` (const std::chrono::system_clock::time_point &): n/a

#### `void start()`
- Source: `include/scheduler/task_scheduler.h`:434
- Brief: ===== Lifecycle =====
- Parameters: none
- Details: Calls: lock(), load(), THEMIS_WARN(), store(), std::thread(), size(), startAll(), THEMIS_INFO().

#### `void stop()`
- Source: `include/scheduler/task_scheduler.h`:435
- Brief: Stop.
- Parameters: none
- Details: Calls: lock(), load(), store(), notify_all(), joinable(), join(), std::chrono::seconds(), std::chrono::steady_clock::now().

#### `std::vector< std::string > topologicalSort(const std::vector< std::string > &task_ids, const std::map< std::string, std::vector< std::string > > &adj) const`
- Source: `include/scheduler/task_scheduler.h`:771
- Brief: n/a
- Parameters:
  - `task_ids` (const std::vector< std::string > &): n/a
  - `adj` (const std::map< std::string, std::vector< std::string > > &): n/a

#### `void unregisterFunction(const std::string &name)`
- Source: `include/scheduler/task_scheduler.h`:538
- Brief: Unregister a custom function.
- Parameters:
  - `name` (const std::string &): Input parameter.
- Details: Unregister Function. name Function name name Input parameter. Calls: lock(), erase(), THEMIS_INFO().

#### `void unregisterTask(const std::string &task_id)`
- Source: `include/scheduler/task_scheduler.h`:453
- Brief: Unregister a task.
- Parameters:
  - `task_id` (const std::string &): Identifier of the task.
- Details: Unregister Task. task_id Task ID to remove task_id Identifier of the task. Calls: lock(), find(), end(), removeEventTrigger(), erase(), THEMIS_INFO(), logTaskSchedulerEvent(), TaskScheduler::currentUserId().

#### `void updateCronExpression(const std::string &task_id, const std::string &expression)`
- Source: `include/scheduler/task_scheduler.h`:749
- Brief: Update Cron Expression.
- Parameters:
  - `task_id` (const std::string &): Identifier of the task.
  - `expression` (const std::string &): Input parameter.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: task_id Identifier of the task. expression Input parameter. std::invalid_argument if an error occurs. Calls: CronExpression::parse().

#### `void updateNextRun(ScheduledTask &task)`
- Source: `include/scheduler/task_scheduler.h`:739
- Brief: Update Next Run.
- Parameters:
  - `task` (ScheduledTask &): Input/output parameter.
- Details: task Input/output parameter. Calls: find(), end(), getNextExecution(), std::chrono::system_clock::now().

#### `void updateTask(const ScheduledTask &task)`
- Source: `include/scheduler/task_scheduler.h`:474
- Brief: Update an existing task.
- Parameters:
  - `task` (const ScheduledTask &): Input parameter.
- Details: Update Task. task Updated task configuration (ID must match existing task) ⚠️ SECURITY: This method MUST be protected by authentication and authorization. Verify user has permission to modify the specified task. task Input parameter. Calls: lock(), find(), end(), THEMIS_INFO(), saveTasks().

#### `void validateAqlQuery(const std::string &aql) const`
- Source: `include/scheduler/task_scheduler.h`:760
- Brief: n/a
- Parameters:
  - `aql` (const std::string &): n/a

#### `void validateCDCTrigger(const ScheduledTask::CDCTrigger &trigger) const`
- Source: `include/scheduler/task_scheduler.h`:763
- Brief: n/a
- Parameters:
  - `trigger` (const ScheduledTask::CDCTrigger &): n/a

#### `void validateCronExpression(const std::string &expression) const`
- Source: `include/scheduler/task_scheduler.h`:762
- Brief: n/a
- Parameters:
  - `expression` (const std::string &): n/a

#### `void validateResourceLimits(const ScheduledTask &task) const`
- Source: `include/scheduler/task_scheduler.h`:761
- Brief: n/a
- Parameters:
  - `task` (const ScheduledTask &): n/a

#### `~TaskScheduler()`
- Source: `include/scheduler/task_scheduler.h`:431
- Brief: n/a
- Parameters: none

### themis::bench::sch

#### `void BM_SCH01_ErrorEnumCast(benchmark::State &state)`
- Source: `benchmarks/scheduler/bench_scheduler_release_gates.cpp`:63
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_SCH02_SwitchDispatch(benchmark::State &state)`
- Source: `benchmarks/scheduler/bench_scheduler_release_gates.cpp`:80
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_SCH03_StructAlloc(benchmark::State &state)`
- Source: `benchmarks/scheduler/bench_scheduler_release_gates.cpp`:119
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_SCH04_BatchCast(benchmark::State &state)`
- Source: `benchmarks/scheduler/bench_scheduler_release_gates.cpp`:137
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Repetitions(kRepetitions) -> ReportAggregatesOnly(true)`
- Source: `benchmarks/scheduler/bench_scheduler_release_gates.cpp`:73
- Brief: n/a
- Parameters:
  - `<unnamed>` (kRepetitions): n/a

### themis::scheduler

#### `std::string generateUUID()`
- Source: `src/scheduler/task_audit_event.cpp`:27
- Brief: Generate UUID v4 for event identification.
- Parameters: none
- Return: UUID string in standard format (xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx)
- Details: Generate UUID. UUID string in standard format (xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx) Return value. Calls: rd(), std::setfill(), std::setw(), str().

#### `bool isSchedulerFailClosed(SchedulerError e) noexcept`
- Source: `include/scheduler/scheduler_api_contract.h`:89
- Brief: n/a
- Parameters:
  - `e` (SchedulerError): n/a

#### `std::string maskSensitiveData(const std::string &data, const std::string &mask_type="partial")`
- Source: `src/scheduler/task_audit_event.cpp`:58
- Brief: Mask sensitive data for GDPR compliance.
- Parameters:
  - `data` (const std::string &): Input parameter.
  - `mask_type` (const std::string &): Input parameter.
- Return: Masked data
- Details: Data masking for GDPR compliance. data Original data mask_type Type of masking ("full", "partial", "hash") Masked data data Input parameter. mask_type Input parameter. Return value. Calls: empty(), SHA256(), c_str(), length(), std::setw(), std::setfill(), str(), substr().

#### `TaskEventType taskEventTypeFromString(const std::string &s)`
- Source: `src/scheduler/task_audit_event.cpp`:142
- Brief: Parse event type from string (reverse of taskEventTypeToString).
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Corresponding TaskEventType, or TASK_COMPLETED as default for unknown values
- Details: Task Event Type From String. s String representation of the event type Corresponding TaskEventType, or TASK_COMPLETED as default for unknown values s Input parameter. Return value. Implements taskEventTypeFromString without additional internal calls.

#### `std::string taskEventTypeToString(TaskEventType type)`
- Source: `src/scheduler/task_audit_event.cpp`:91
- Brief: Convert event type to string.
- Parameters:
  - `type` (TaskEventType): Input parameter.
- Return: Return value.
- Details: Task Event Type To String. type Input parameter. Return value. Implements taskEventTypeToString without additional internal calls.

#### `std::string taskSecurityEventTypeToString(TaskSecurityEventType type)`
- Source: `src/scheduler/task_audit_event.cpp`:120
- Brief: Convert security event type to string.
- Parameters:
  - `type` (TaskSecurityEventType): Input parameter.
- Return: Return value.
- Details: Task Security Event Type To String. type Input parameter. Return value. Implements taskSecurityEventTypeToString without additional internal calls.

### themis::scheduler::AnomalyMetrics

#### `nlohmann::json toJson() const`
- Source: `include/scheduler/task_audit_event.h`:98
- Brief: n/a
- Parameters: none

### themis::scheduler::ExternalSchedulerAdapter

#### `ExternalSchedulerAdapter()=default`
- Source: `include/scheduler/external_scheduler_adapter.h`:110
- Brief: n/a
- Parameters: none

#### `SchedulerError classifyAndMapExternalError(const std::string &error_msg, int http_status, const std::string &task_id, ExternalSchedulerType scheduler_type)`
- Source: `include/scheduler/external_scheduler_adapter.h`:314
- Brief: Classify and map external scheduler errors to SchedulerError codes.
- Parameters:
  - `error_msg` (const std::string &): Input parameter.
  - `http_status` (int): Input parameter.
  - `task_id` (const std::string &): Identifier of the task.
  - `scheduler_type` (ExternalSchedulerType): Input parameter.
- Return: Mapped SchedulerError code.
- Details: Classify And Map External Error. Examines error information from an external scheduler backend (HTTP status, error message, exception type) and maps it to a standard SchedulerError code. This enables consistent error handling across different external scheduler implementations and makes diagnostics observable and traceable. Error classification rules: Network/timeout errors (connection refused, deadline exceeded) → kCoordinationError Configuration errors (invalid URL, missing auth) → kInternalError Permanent task errors (not found, permission denied) → kExecutionFailed Transient errors (service temporarily unavailable) → kCoordinationError All classified errors are logged with context (task ID, scheduler type, error details) to support production incident diagnostics. error_msg Error message or exception description. http_status HTTP response status code (if applicable; 0 if not HTTP-based). task_id Task ID for diagnostic logging. scheduler_type Target scheduler for contextual logging. Mapped SchedulerError code. scheduler_api_contract.h for error taxonomy. error_msg Input parameter. http_status Input parameter. task_id Identifier of the task. scheduler_type Input parameter. Return value.

#### `SchedulerError dispatchTaskToExternal(const ScheduledTask &task, ExternalSchedulerType scheduler_type, const nlohmann::json &external_config)`
- Source: `include/scheduler/external_scheduler_adapter.h`:248
- Brief: Dispatch a ThemisDB task to an external scheduler backend.
- Parameters:
  - `task` (const ScheduledTask &): Input parameter.
  - `scheduler_type` (ExternalSchedulerType): Input parameter.
  - `external_config` (const nlohmann::json &): Input parameter.
- Return: SchedulerError::kSuccess if dispatch successful, SchedulerError::kCoordinationError if backend unavailable, other SchedulerError codes for validation/format errors.
- Throws:
  - std::exception: on internal serialization or network errors.
- Details: ───────────────────────────────────────────────────────────────────────────── External Scheduler Integration and Status Sync ───────────────────────────────────────────────────────────────────────────── This method translates a ThemisDB ScheduledTask into the target external scheduler's format and sends it to the external scheduler for registration. The task remains registered locally even if dispatch fails, allowing for retry logic and diagnostic introspection. When the external scheduler is unavailable or unresponsive: The method fails explicitly with kCoordinationError The task remains in the local registry for retry No partial state is left on the external backend This is a fail-closed operation: if dispatch fails, the task is NOT lost, but it will not execute on the external scheduler until a successful retry. task ThemisDB task to dispatch. scheduler_type Target scheduler platform (Kubernetes, Airflow, etc). external_config Backend-specific configuration (URL, auth, etc). SchedulerError::kSuccess if dispatch successful, SchedulerError::kCoordinationError if backend unavailable, other SchedulerError codes for validation/format errors. std::exception on internal serialization or network errors. scheduler_api_contract.h for error taxonomy. task Input parameter. scheduler_type Input parameter. external_config Input parameter. Return value.

#### `std::string effectiveCronSchedule(const ScheduledTask &task)`
- Source: `include/scheduler/external_scheduler_adapter.h`:205
- Brief: Return the effective cron schedule for a task.
- Parameters:
  - `task` (const ScheduledTask &): Input parameter.
- Return: Cron expression string.
- Details: Effective Cron Schedule. Returns task.cron_expression when TriggerType is CRON and the expression is non-empty. Otherwise converts task.interval using intervalToCron(). task ThemisDB scheduled task. Cron expression string. task Input parameter. Return value. Calls: empty(), intervalToCron().

#### `ScheduledTask fromKubernetesCronJobJson(const nlohmann::json &manifest) const`
- Source: `include/scheduler/external_scheduler_adapter.h`:159
- Brief: Parse a Kubernetes CronJob manifest (JSON) and create a matching ThemisDB ScheduledTask.
- Parameters:
  - `manifest` (const nlohmann::json &): JSON object of the Kubernetes CronJob resource.
- Return: ThemisDB ScheduledTask mirroring the CronJob schedule.
- Throws:
  - std::invalid_argument: if the manifest is missing required fields.
- Details: The resulting task uses TriggerType::CRON with the cron expression taken from spec.schedule. The task id is derived from metadata.name; the name and description are also mapped. manifest JSON object of the Kubernetes CronJob resource. ThemisDB ScheduledTask mirroring the CronJob schedule. std::invalid_argument if the manifest is missing required fields.

#### `std::string intervalToCron(std::chrono::milliseconds interval)`
- Source: `include/scheduler/external_scheduler_adapter.h`:194
- Brief: Convert a fixed millisecond interval to a best-effort cron expression.
- Parameters:
  - `interval` (std::chrono::milliseconds): Input parameter.
- Return: 5-field cron expression string.
- Details: Interval To Cron. Intervals shorter than 1 minute are rounded up to 1 minute. Intervals shorter than 1 hour are expressed as *\/N * * * *. Intervals shorter than 24 hours are expressed as 0 *\/N * * *. Daily or longer intervals are expressed as 0 0 *\/D * *. interval Duration to convert. 5-field cron expression string. interval Input parameter. Return value. Calls: minutes(), count(), std::to_string().

#### `std::string jsonToYaml(const nlohmann::json &j, int indent=0)`
- Source: `include/scheduler/external_scheduler_adapter.h`:322
- Brief: Serialise a JSON object as minimal YAML (enough for Kubernetes manifests).
- Parameters:
  - `j` (const nlohmann::json &): Input parameter.
  - `indent` (int): Input parameter.
- Return: Return value.
- Details: ───────────────────────────────────────────────────────────────────────────── YAML serialiser (subset sufficient for Kubernetes manifests) ───────────────────────────────────────────────────────────────────────────── j Input parameter. indent Input parameter. Return value. Calls: pad(), child_pad(), is_object(), begin(), end(), key(), value(), is_array().

#### `SchedulerError pollExternalStatus(const std::string &task_id, const std::string &external_task_id, ExternalSchedulerType scheduler_type, const nlohmann::json &external_config, TaskResultStore *result_store, int max_retries=3, std::chrono::milliseconds initial_backoff_ms=std::chrono::milliseconds{100})`
- Source: `include/scheduler/external_scheduler_adapter.h`:280
- Brief: Poll an external scheduler for task status and sync results.
- Parameters:
  - `task_id` (const std::string &): Identifier of the task.
  - `external_task_id` (const std::string &): Identifier of the external task.
  - `scheduler_type` (ExternalSchedulerType): Input parameter.
  - `external_config` (const nlohmann::json &): Input parameter.
  - `result_store` (TaskResultStore *): Input/output parameter.
  - `max_retries` (int): Input parameter.
  - `initial_backoff_ms` (std::chrono::milliseconds): Input parameter.
- Return: SchedulerError::kSuccess if poll succeeded and status was synced, SchedulerError::kCoordinationError if backend unavailable (after retries), other SchedulerError codes for other failures.
- Details: Poll External Status. Queries the external scheduler backend for the current execution status of a previously dispatched task, and if the status has changed (e.g., from SCHEDULED to COMPLETED), syncs the result back to the local result store. Uses exponential backoff on transient failures (network errors, timeouts) to avoid overwhelming the external backend. Permanent errors (invalid task ID, authentication failure) fail fast without retry. The local result store is the source of truth; external results are synced only if they indicate task completion (success or failure). task_id ThemisDB task identifier. external_task_id External scheduler's task identifier (e.g., K8s pod name). scheduler_type Target scheduler platform. external_config Backend-specific configuration. result_store Local result store for syncing outcomes. max_retries Maximum retry attempts on transient errors (default 3). initial_backoff_ms Initial backoff in milliseconds (default 100). SchedulerError::kSuccess if poll succeeded and status was synced, SchedulerError::kCoordinationError if backend unavailable (after retries), other SchedulerError codes for other failures. scheduler_api_contract.h for error taxonomy. task_id Identifier of the task. external_task_id Identifier of the external task. scheduler_type Input parameter. external_config Input parameter. result_store Input/output parameter. max_retries Input parameter. initial_backoff_ms Input parameter. Return value.

#### `std::string pyStringEscape(const std::string &s)`
- Source: `include/scheduler/external_scheduler_adapter.h`:325
- Brief: Escape a string for embedding in a Python string literal.
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Return value.
- Details: Py String Escape. s Input parameter. Return value. Calls: reserve(), size().

#### `std::string toAirflowDagPython(const std::vector< ScheduledTask > &tasks, const AirflowDagConfig &config) const`
- Source: `include/scheduler/external_scheduler_adapter.h`:178
- Brief: Generate an Airflow DAG Python file for one or more ThemisDB tasks.
- Parameters:
  - `tasks` (const std::vector< ScheduledTask > &): List of ThemisDB tasks to export. Must not be empty.
  - `config` (const AirflowDagConfig &): Airflow DAG generation options.
- Return: Python source code of the Airflow DAG file.
- Throws:
  - std::invalid_argument: if tasks is empty.
- Details: The generated file can be dropped directly into an Airflow dags/ directory. Each task becomes a SimpleHttpOperator (Airflow provider: apache-airflow-providers-http) that POSTs to ThemisDB's task-execute endpoint. Task dependencies are translated into Airflow operator dependencies using the >> syntax. tasks List of ThemisDB tasks to export. Must not be empty. config Airflow DAG generation options. Python source code of the Airflow DAG file. std::invalid_argument if tasks is empty.

#### `std::string toK8sName(const std::string &name)`
- Source: `include/scheduler/external_scheduler_adapter.h`:217
- Brief: Sanitise a string for use as a Kubernetes resource name.
- Parameters:
  - `name` (const std::string &): Input parameter.
- Return: DNS-label-safe name.
- Details: To K8s Name. Converts to lowercase, replaces non-alphanumeric characters with '-', and trims leading/trailing hyphens. Truncates to 52 characters to leave room for a suffix. name Raw name string (e.g. task.id or task.name). DNS-label-safe name. name Input parameter. Return value. Calls: reserve(), size(), std::isalnum(), std::tolower(), find_first_not_of(), substr(), find_last_not_of(), empty().

#### `nlohmann::json toKubernetesCronJobJson(const ScheduledTask &task, const KubernetesCronJobConfig &config) const`
- Source: `include/scheduler/external_scheduler_adapter.h`:128
- Brief: Generate a Kubernetes CronJob manifest in JSON format for a single task.
- Parameters:
  - `task` (const ScheduledTask &): ThemisDB scheduled task to export.
  - `config` (const KubernetesCronJobConfig &): Kubernetes generation options (namespace, image, auth, …).
- Return: JSON object representing the Kubernetes CronJob manifest.
- Throws:
  - std::invalid_argument: if task.id is empty or if the task has no cron expression and no finite interval to convert to a cron schedule.
- Details: The manifest represents a batch/v1 CronJob resource that, on each trigger, runs a pod which calls POST {themisdb_base_url}/api/v1/scheduler/tasks/{task.id}/execute task ThemisDB scheduled task to export. config Kubernetes generation options (namespace, image, auth, …). JSON object representing the Kubernetes CronJob manifest. std::invalid_argument if task.id is empty or if the task has no cron expression and no finite interval to convert to a cron schedule.

#### `std::string toKubernetesCronJobYaml(const ScheduledTask &task, const KubernetesCronJobConfig &config) const`
- Source: `include/scheduler/external_scheduler_adapter.h`:143
- Brief: Generate a Kubernetes CronJob manifest in YAML format for a single task.
- Parameters:
  - `task` (const ScheduledTask &): ThemisDB scheduled task to export.
  - `config` (const KubernetesCronJobConfig &): Kubernetes generation options.
- Return: YAML string ready to apply with kubectl apply -f.
- Throws:
  - std::invalid_argument: (see toKubernetesCronJobJson).
- Details: Convenience wrapper around toKubernetesCronJobJson() that serialises the JSON as YAML text. task ThemisDB scheduled task to export. config Kubernetes generation options. YAML string ready to apply with kubectl apply -f. std::invalid_argument (see toKubernetesCronJobJson).

### themis::scheduler::TaskAnomalyDetector

#### `TaskAnomalyDetector(const AnomalyDetectorConfig &config=AnomalyDetectorConfig())`
- Source: `include/scheduler/task_anomaly_detector.h`:119
- Brief: n/a
- Parameters:
  - `config` (const AnomalyDetectorConfig &): n/a

#### `void anomalyCallbackWorker()`
- Source: `include/scheduler/task_anomaly_detector.h`:216
- Brief: Background worker thread for async anomaly callbacks.
- Parameters: none
- Details: Calls: THEMIS_DEBUG(), load(), lock(), wait_for(), std::chrono::milliseconds(), empty(), std::move(), front().

#### `double calculateMean(const std::deque< double > &values) const`
- Source: `include/scheduler/task_anomaly_detector.h`:233
- Brief: n/a
- Parameters:
  - `values` (const std::deque< double > &): n/a

#### `double calculatePercentile(const std::deque< double > &values, double percentile) const`
- Source: `include/scheduler/task_anomaly_detector.h`:235
- Brief: n/a
- Parameters:
  - `values` (const std::deque< double > &): n/a
  - `percentile` (double): n/a

#### `double calculateStdDev(const std::deque< double > &values, double mean) const`
- Source: `include/scheduler/task_anomaly_detector.h`:234
- Brief: n/a
- Parameters:
  - `values` (const std::deque< double > &): n/a
  - `mean` (double): n/a

#### `AnomalyMetrics checkAnomaly(const std::string &task_id) const`
- Source: `include/scheduler/task_anomaly_detector.h`:171
- Brief: Check anomaly status without recording an execution.
- Parameters:
  - `task_id` (const std::string &): Task identifier
- Return: Anomaly metrics (advisory, non-blocking)
- Details: GAP 1 FIX: On-demand anomaly detection for external invocation task_id Task identifier Anomaly metrics (advisory, non-blocking)

#### `void cleanupOldData(TaskStatistics &stats)`
- Source: `include/scheduler/task_anomaly_detector.h`:238
- Brief: Cleanup Old Data.
- Parameters:
  - `stats` (TaskStatistics &): Input/output parameter.
- Details: stats Input/output parameter. Calls: empty(), front(), pop_front().

#### `double detectFailureRateAnomaly(const std::string &task_id, bool success) const`
- Source: `include/scheduler/task_anomaly_detector.h`:228
- Brief: n/a
- Parameters:
  - `task_id` (const std::string &): n/a
  - `success` (bool): n/a

#### `double detectFrequencyAnomaly(const std::string &task_id, const std::chrono::system_clock::time_point &now) const`
- Source: `include/scheduler/task_anomaly_detector.h`:219
- Brief: n/a
- Parameters:
  - `task_id` (const std::string &): n/a
  - `now` (const std::chrono::system_clock::time_point &): n/a

#### `double detectPatternAnomaly(const std::string &task_id, const std::chrono::system_clock::time_point &now) const`
- Source: `include/scheduler/task_anomaly_detector.h`:222
- Brief: n/a
- Parameters:
  - `task_id` (const std::string &): n/a
  - `now` (const std::chrono::system_clock::time_point &): n/a

#### `double detectResourceAnomaly(const std::string &task_id, const TaskResourceUsage &resource_usage) const`
- Source: `include/scheduler/task_anomaly_detector.h`:225
- Brief: n/a
- Parameters:
  - `task_id` (const std::string &): n/a
  - `resource_usage` (const TaskResourceUsage &): n/a

#### `nlohmann::json exportStatistics() const`
- Source: `include/scheduler/task_anomaly_detector.h`:194
- Brief: Export statistics to JSON (for persistence/analysis).
- Parameters: none

#### `std::map< std::string, TaskStatistics > getAllStatistics() const`
- Source: `include/scheduler/task_anomaly_detector.h`:145
- Brief: Get all task statistics.
- Parameters: none
- Return: Map of task_id -> statistics
- Details: Map of task_id -> statistics

#### `AnomalyDetectorConfig getConfig() const`
- Source: `include/scheduler/task_anomaly_detector.h`:184
- Brief: Get current configuration.
- Parameters: none

#### `std::optional< TaskStatistics > getTaskStatistics(const std::string &task_id) const`
- Source: `include/scheduler/task_anomaly_detector.h`:139
- Brief: Get statistics for a specific task.
- Parameters:
  - `task_id` (const std::string &): Task identifier
- Return: Task statistics (or empty if task not found)
- Details: task_id Task identifier Task statistics (or empty if task not found)

#### `bool hasBaseline(const std::string &task_id) const`
- Source: `include/scheduler/task_anomaly_detector.h`:163
- Brief: Check if task has sufficient baseline data.
- Parameters:
  - `task_id` (const std::string &): Task identifier
- Return: true if baseline is established
- Details: task_id Task identifier true if baseline is established

#### `void importStatistics(const nlohmann::json &data)`
- Source: `include/scheduler/task_anomaly_detector.h`:199
- Brief: Import statistics from JSON (for restoration).
- Parameters:
  - `data` (const nlohmann::json &): Input parameter.
- Details: Import Statistics. data Input parameter. Calls: lock(), contains(), items(), push_back(), jsonToDequeDouble(), clear(), std::chrono::system_clock::time_point(), std::chrono::milliseconds().

#### `void recalibrateBaseline(const std::string &task_id)`
- Source: `include/scheduler/task_anomaly_detector.h`:179
- Brief: Recalibrate baseline for a task (reset time-window data).
- Parameters:
  - `task_id` (const std::string &): Identifier of the task.
- Details: GAP 3 FIX: Explicit baseline recalibration. GAP 3 FIX: Explicit baseline recalibration task_id Task identifier Preserves existing statistics but resets time-based calculations task_id Identifier of the task. Calls: lock(), find(), end(), THEMIS_WARN(), cleanupOldData(), empty(), calculateMean(), calculateStdDev().

#### `AnomalyMetrics recordExecution(const TaskAuditEvent &event)`
- Source: `include/scheduler/task_anomaly_detector.h`:132
- Brief: Record a task execution event.
- Parameters:
  - `event` (const TaskAuditEvent &): Input parameter.
- Return: Anomaly metrics for this execution
- Details: Record Execution. event Audit event to process Anomaly metrics for this execution event Input parameter. Return value. Calls: lock(), updateStatistics(), hasBaseline(), detectFrequencyAnomaly(), detectPatternAnomaly(), detectResourceAnomaly(), detectFailureRateAnomaly(), push_back().

#### `void resetAllStatistics()`
- Source: `include/scheduler/task_anomaly_detector.h`:156
- Brief: Reset all statistics.
- Parameters: none
- Details: Reset All Statistics. Calls: lock(), clear().

#### `void resetTaskStatistics(const std::string &task_id)`
- Source: `include/scheduler/task_anomaly_detector.h`:151
- Brief: Reset statistics for a specific task.
- Parameters:
  - `task_id` (const std::string &): Identifier of the task.
- Details: Reset Task Statistics. task_id Task identifier task_id Identifier of the task. Calls: lock(), erase().

#### `void start()`
- Source: `include/scheduler/task_anomaly_detector.h`:124
- Brief: GAP 2 FIX: Lifecycle management for background callback thread.
- Parameters: none
- Details: Calls: lock(), load(), store(), std::thread(), THEMIS_INFO().

#### `void stop()`
- Source: `include/scheduler/task_anomaly_detector.h`:125
- Brief: Stop.
- Parameters: none
- Details: Calls: lock(), load(), store(), notify_all(), joinable(), join(), THEMIS_INFO().

#### `void updateConfig(const AnomalyDetectorConfig &config)`
- Source: `include/scheduler/task_anomaly_detector.h`:189
- Brief: Update configuration.
- Parameters:
  - `config` (const AnomalyDetectorConfig &): New access control configuration.
- Details: Update the access control configuration. config New access control configuration. Calls: lock().

#### `void updateStatistics(const std::string &task_id, const TaskAuditEvent &event)`
- Source: `include/scheduler/task_anomaly_detector.h`:232
- Brief: Update Statistics.
- Parameters:
  - `task_id` (const std::string &): Identifier of the task.
  - `event` (const TaskAuditEvent &): Input parameter.
- Details: task_id Identifier of the task. event Input parameter. Calls: push_back(), size(), pop_front(), empty(), calculateMean(), calculateStdDev(), std::min_element(), begin().

#### `~TaskAnomalyDetector()`
- Source: `include/scheduler/task_anomaly_detector.h`:121
- Brief: n/a
- Parameters: none

### themis::scheduler::TaskAuditEvent

#### `std::string toCEF() const`
- Source: `include/scheduler/task_audit_event.h`:172
- Brief: Convert to CEF (Common Event Format) for SIEM integration.
- Parameters: none
- Return: CEF-formatted string
- Details: CEF-formatted string

#### `nlohmann::json toElasticECS() const`
- Source: `include/scheduler/task_audit_event.h`:184
- Brief: Convert to Elastic Common Schema (ECS) format.
- Parameters: none
- Return: ECS-compatible JSON
- Details: ECS-compatible JSON

#### `nlohmann::json toJson(bool gdpr_mode=false) const`
- Source: `include/scheduler/task_audit_event.h`:166
- Brief: Convert audit event to JSON format.
- Parameters:
  - `gdpr_mode` (bool): If true, applies GDPR-compliant masking to sensitive fields
- Return: JSON representation of the event
- Details: gdpr_mode If true, applies GDPR-compliant masking to sensitive fields JSON representation of the event

#### `nlohmann::json toSplunkHEC() const`
- Source: `include/scheduler/task_audit_event.h`:178
- Brief: Convert to Splunk HEC format.
- Parameters: none
- Return: Splunk-compatible JSON
- Details: Splunk-compatible JSON

### themis::scheduler::TaskAuditManager

#### `TaskAuditManager(std::shared_ptr< utils::AuditLogger > audit_logger, const TaskAuditConfig &config=TaskAuditConfig())`
- Source: `include/scheduler/task_audit_manager.h`:126
- Brief: Construct audit manager.
- Parameters:
  - `audit_logger` (std::shared_ptr< utils::AuditLogger >): Shared audit logger instance (for tamper-evident logging)
  - `config` (const TaskAuditConfig &): Audit manager configuration
- Details: audit_logger Shared audit logger instance (for tamper-evident logging) config Audit manager configuration

#### `void cacheAuditEvent(const TaskAuditEvent &event)`
- Source: `include/scheduler/task_audit_manager.h`:276
- Brief: Cache Audit Event.
- Parameters:
  - `event` (const TaskAuditEvent &): Input parameter.
- Details: event Input parameter. Calls: lock(), push_back(), size(), pop_front().

#### `void cacheSecurityEvent(const TaskSecurityEvent &event)`
- Source: `include/scheduler/task_audit_manager.h`:277
- Brief: Cache Security Event.
- Parameters:
  - `event` (const TaskSecurityEvent &): Input parameter.
- Details: event Input parameter. Calls: lock(), push_back(), size(), pop_front().

#### `size_t detectAndRecoverCorruption()`
- Source: `include/scheduler/task_audit_manager.h`:261
- Brief: Scan audit log for corruption and attempt recovery.
- Parameters: none
- Return: Number of corrupted entries detected
- Details: GAP 3 FIX: Corruption detection and recovery. GAP 3 FIX: Corruption detection and recovery Number of corrupted entries detected Return value. Calls: lock(), ifs(), is_open(), std::getline(), empty(), nlohmann::json::parse(), contains(), THEMIS_WARN().

#### `size_t enforceRetentionPolicy()`
- Source: `include/scheduler/task_audit_manager.h`:254
- Brief: Enforce audit log retention policies.
- Parameters: none
- Return: Number of entries archived/deleted
- Details: GAP 2 FIX: Retention policy enforcement. GAP 2 FIX: Retention policy enforcement Deletes entries older than configured retention period Number of entries archived/deleted Return value. Calls: lock(), count(), std::chrono::system_clock::now(), ifs(), is_open(), std::getline(), empty(), nlohmann::json::parse().

#### `nlohmann::json exportAnomalyStatistics() const`
- Source: `include/scheduler/task_audit_manager.h`:224
- Brief: Export anomaly detector statistics to JSON for persistence.
- Parameters: none
- Return: JSON object with all task statistics and configuration
- Details: JSON object with all task statistics and configuration

#### `size_t exportAuditEvents(const AuditQueryParams &params, const std::string &output_path, ExportFormat format) const`
- Source: `include/scheduler/task_audit_manager.h`:165
- Brief: Export audit events to file.
- Parameters:
  - `params` (const AuditQueryParams &): Query parameters for filtering
  - `output_path` (const std::string &): Output file path
  - `format` (ExportFormat): Export format
- Return: Number of events exported
- Details: params Query parameters for filtering output_path Output file path format Export format Number of events exported

#### `void flush()`
- Source: `include/scheduler/task_audit_manager.h`:208
- Brief: Flush all logs to disk.
- Parameters: none
- Details: Flush. Implements flush without additional internal calls.

#### `std::string generateAuditEntryHMAC(const TaskAuditEvent &event) const`
- Source: `include/scheduler/task_audit_manager.h`:238
- Brief: Generate HMAC for an audit entry (tamper detection).
- Parameters:
  - `event` (const TaskAuditEvent &): Audit event
- Return: HMAC-256 hex string
- Details: GAP 1 FIX: Immutable audit log enforcement with HMAC event Audit event HMAC-256 hex string

#### `std::map< std::string, TaskStatistics > getAllStatistics() const`
- Source: `include/scheduler/task_audit_manager.h`:180
- Brief: Get all task statistics.
- Parameters: none
- Return: Map of task_id -> statistics
- Details: Map of task_id -> statistics

#### `std::string getAuditLogPath() const`
- Source: `include/scheduler/task_audit_manager.h`:213
- Brief: Get audit log file path.
- Parameters: none

#### `TaskAuditConfig getConfig() const`
- Source: `include/scheduler/task_audit_manager.h`:198
- Brief: Get current configuration.
- Parameters: none

#### `std::string getSecurityLogPath() const`
- Source: `include/scheduler/task_audit_manager.h`:218
- Brief: Get security log file path.
- Parameters: none

#### `std::optional< TaskStatistics > getTaskStatistics(const std::string &task_id) const`
- Source: `include/scheduler/task_audit_manager.h`:174
- Brief: Get task execution statistics.
- Parameters:
  - `task_id` (const std::string &): Task identifier
- Return: Task statistics (if available)
- Details: task_id Task identifier Task statistics (if available)

#### `bool hasAnomalies(const std::string &task_id) const`
- Source: `include/scheduler/task_audit_manager.h`:187
- Brief: Check if a task has anomalous behavior.
- Parameters:
  - `task_id` (const std::string &): Task identifier
- Return: true if recent executions show anomalies
- Details: task_id Task identifier true if recent executions show anomalies

#### `void importAnomalyStatistics(const nlohmann::json &data)`
- Source: `include/scheduler/task_audit_manager.h`:230
- Brief: Import anomaly detector statistics from JSON (restored after restart).
- Parameters:
  - `data` (const nlohmann::json &): Input parameter.
- Details: Import Anomaly Statistics. data Previously exported statistics JSON data Input parameter. Calls: lock(), importStatistics().

#### `std::vector< TaskAuditEvent > loadEventsFromFile(const std::string &file_path, const AuditQueryParams &params) const`
- Source: `include/scheduler/task_audit_manager.h`:281
- Brief: n/a
- Parameters:
  - `file_path` (const std::string &): n/a
  - `params` (const AuditQueryParams &): n/a

#### `std::vector< TaskSecurityEvent > loadSecurityEventsFromFile(const std::string &file_path, const AuditQueryParams &params) const`
- Source: `include/scheduler/task_audit_manager.h`:283
- Brief: n/a
- Parameters:
  - `file_path` (const std::string &): n/a
  - `params` (const AuditQueryParams &): n/a

#### `AnomalyMetrics logAuditEvent(const TaskAuditEvent &event)`
- Source: `include/scheduler/task_audit_manager.h`:136
- Brief: Log a task audit event.
- Parameters:
  - `event` (const TaskAuditEvent &): Input parameter.
- Return: Anomaly metrics (if anomaly detection enabled)
- Details: Log Audit Event. event Audit event to log Anomaly metrics (if anomaly detection enabled) event Input parameter. Return value. Calls: recordExecution(), on_anomaly_detected(), writeToAuditLog(), cacheAuditEvent(), on_audit_event().

#### `void logSecurityEvent(const TaskSecurityEvent &event)`
- Source: `include/scheduler/task_audit_manager.h`:142
- Brief: Log a security violation event.
- Parameters:
  - `event` (const TaskSecurityEvent &): Input parameter.
- Details: Log a security event. event Security event to log event Input parameter. Calls: writeToSecurityLog(), cacheSecurityEvent(), on_security_event(), THEMIS_WARN(), taskSecurityEventTypeToString().

#### `bool matchesQuery(const TaskAuditEvent &event, const AuditQueryParams &params) const`
- Source: `include/scheduler/task_audit_manager.h`:280
- Brief: n/a
- Parameters:
  - `event` (const TaskAuditEvent &): n/a
  - `params` (const AuditQueryParams &): n/a

#### `std::vector< TaskAuditEvent > queryAuditEvents(const AuditQueryParams &params) const`
- Source: `include/scheduler/task_audit_manager.h`:149
- Brief: Query audit events with filters.
- Parameters:
  - `params` (const AuditQueryParams &): Query parameters
- Return: Vector of matching audit events
- Details: params Query parameters Vector of matching audit events

#### `std::vector< TaskSecurityEvent > querySecurityEvents(const AuditQueryParams &params) const`
- Source: `include/scheduler/task_audit_manager.h`:156
- Brief: Query security events with filters.
- Parameters:
  - `params` (const AuditQueryParams &): Query parameters (similar structure)
- Return: Vector of matching security events
- Details: params Query parameters (similar structure) Vector of matching security events

#### `void resetTaskStatistics(const std::string &task_id)`
- Source: `include/scheduler/task_audit_manager.h`:193
- Brief: Reset statistics for a task.
- Parameters:
  - `task_id` (const std::string &): Identifier of the task.
- Details: Reset Task Statistics. task_id Task identifier task_id Identifier of the task. Implements resetTaskStatistics without additional internal calls.

#### `void updateConfig(const TaskAuditConfig &config)`
- Source: `include/scheduler/task_audit_manager.h`:203
- Brief: Update configuration.
- Parameters:
  - `config` (const TaskAuditConfig &): New access control configuration.
- Details: Update the access control configuration. config New access control configuration. Calls: lock().

#### `bool verifyAuditEntryIntegrity(const TaskAuditEvent &event, const std::string &stored_hmac) const`
- Source: `include/scheduler/task_audit_manager.h`:246
- Brief: n/a
- Parameters:
  - `event` (const TaskAuditEvent &): Audit event
  - `stored_hmac` (const std::string &): Previously stored HMAC
- Return: true if entry has not been tampered with
- Details: GAP 1 FIX: Verify integrity of audit entry event Audit event stored_hmac Previously stored HMAC true if entry has not been tampered with

#### `void writeToAuditLog(const TaskAuditEvent &event)`
- Source: `include/scheduler/task_audit_manager.h`:278
- Brief: Write To Audit Log.
- Parameters:
  - `event` (const TaskAuditEvent &): Input parameter.
- Details: event Input parameter. Calls: lock(), ofs(), is_open(), toJson(), dump(), THEMIS_ERROR(), what(), logEvent().

#### `void writeToSecurityLog(const TaskSecurityEvent &event)`
- Source: `include/scheduler/task_audit_manager.h`:279
- Brief: Write To Security Log.
- Parameters:
  - `event` (const TaskSecurityEvent &): Input parameter.
- Details: event Input parameter. Calls: lock(), ofs(), is_open(), toJson(), dump(), THEMIS_ERROR(), what(), logSecurityEvent().

### themis::scheduler::TaskExecutionResult

#### `TaskExecutionResult fromJson(const nlohmann::json &j)`
- Source: `include/scheduler/task_result_store.h`:45
- Brief: Deserialize from JSON (used when loading from RocksDB).
- Parameters:
  - `j` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: From Json. j Input parameter. Return value. Calls: contains(), at().

#### `nlohmann::json toJson() const`
- Source: `include/scheduler/task_result_store.h`:42
- Brief: Serialize to JSON (for storage / API responses).
- Parameters: none

### themis::scheduler::TaskResourceUsage

#### `nlohmann::json toJson() const`
- Source: `include/scheduler/task_audit_event.h`:73
- Brief: n/a
- Parameters: none

### themis::scheduler::TaskResultStore

#### `TaskResultStore(RocksDBWrapper &storage, size_t max_per_task=100)`
- Source: `include/scheduler/task_result_store.h`:65
- Brief: Construct a result store backed by the supplied RocksDB instance.
- Parameters:
  - `storage` (RocksDBWrapper &): Open RocksDB wrapper. Must outlive this object.
  - `max_per_task` (size_t): Maximum number of results to retain per task. Oldest records are pruned when the cap is exceeded.
- Details: storage Open RocksDB wrapper. Must outlive this object. max_per_task Maximum number of results to retain per task. Oldest records are pruned when the cap is exceeded.

#### `std::optional< TaskExecutionResult > getLatestResult(const std::string &task_id) const`
- Source: `include/scheduler/task_result_store.h`:102
- Brief: Return the most-recent execution result for a task, if any.
- Parameters:
  - `task_id` (const std::string &): Task identifier.
- Return: Most-recent result, or std::nullopt if no results are stored.
- Details: task_id Task identifier. Most-recent result, or std::nullopt if no results are stored.

#### `std::vector< TaskExecutionResult > getResults(const std::string &task_id, size_t limit=10) const`
- Source: `include/scheduler/task_result_store.h`:93
- Brief: Retrieve the most-recent execution results for a task.
- Parameters:
  - `task_id` (const std::string &): Task identifier to query.
  - `limit` (size_t): Maximum number of records to return (default: 10).
- Return: Vector of results (may be empty if none are stored).
- Details: Results are returned newest-first. task_id Task identifier to query. limit Maximum number of records to return (default: 10). Vector of results (may be empty if none are stored).

#### `std::string makeKey(const std::string &task_id, int64_t timestamp_ms)`
- Source: `include/scheduler/task_result_store.h`:114
- Brief: Build the full RocksDB key for a result record.
- Parameters:
  - `task_id` (const std::string &): Identifier of the task.
  - `timestamp_ms` (int64_t): Input parameter.
- Return: Return value.
- Details: Build a zero-padded 20-digit decimal timestamp so keys sort chronologically. task_id Identifier of the task. timestamp_ms Input parameter. Return value. Calls: std::setw(), std::setfill(), str().

#### `std::string makeTaskPrefix(const std::string &task_id)`
- Source: `include/scheduler/task_result_store.h`:117
- Brief: Build the prefix used to scan all results for a task.
- Parameters:
  - `task_id` (const std::string &): Identifier of the task.
- Return: Return value.
- Details: Make Task Prefix. task_id Identifier of the task. Return value. Calls: std::string().

#### `SchedulerError store(const TaskExecutionResult &result)`
- Source: `include/scheduler/task_result_store.h`:82
- Brief: Append an execution result for a task.
- Parameters:
  - `result` (const TaskExecutionResult &): Input parameter.
- Return: kSuccess on successful storage, kRetentionLimitExceeded if at capacity, kInternalError on storage failure.
- Details: Store. Enforces retention limits BEFORE writing the new result. If the store is at capacity (max_per_task entries), returns kRetentionLimitExceeded without storing the result. On success, stores the result and performs FIFO pruning to maintain the retention limit (oldest entries deleted first). result Execution record to persist. kSuccess on successful storage, kRetentionLimitExceeded if at capacity, kInternalError on storage failure. result Input parameter. Return value. Calls: lk(), makeTaskPrefix(), scanPrefix(), emplace_back(), size(), THEMIS_WARN(), empty(), front().

### themis::scheduler::TaskSecurityEvent

#### `std::string toCEF() const`
- Source: `include/scheduler/task_audit_event.h`:235
- Brief: Convert to CEF format with security extensions.
- Parameters: none
- Return: CEF-formatted string
- Details: CEF-formatted string

#### `nlohmann::json toJson() const`
- Source: `include/scheduler/task_audit_event.h`:229
- Brief: Convert security event to JSON format.
- Parameters: none
- Return: JSON representation of the event
- Details: JSON representation of the event

### themis::scheduler::test

#### `TEST(SchedulerContractHardening, ErrorCodeRange)`
- Source: `tests/scheduler/test_scheduler_contract_hardening_focused.cpp`:34
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerContractHardening): n/a
  - `<unnamed>` (ErrorCodeRange): n/a

#### `TEST(SchedulerContractHardening, ErrorCodeUniqueness)`
- Source: `tests/scheduler/test_scheduler_contract_hardening_focused.cpp`:15
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerContractHardening): n/a
  - `<unnamed>` (ErrorCodeUniqueness): n/a

#### `TEST_F(SchedulerConcurrencyTest, SCE01_ConcurrentRegistration)`
- Source: `tests/scheduler/test_scheduler_concurrency_edge_cases.cpp`:45
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerConcurrencyTest): n/a
  - `<unnamed>` (SCE01_ConcurrentRegistration): n/a

#### `TEST_F(SchedulerConcurrencyTest, SCE02_ConcurrentExecuteAndRegister)`
- Source: `tests/scheduler/test_scheduler_concurrency_edge_cases.cpp`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerConcurrencyTest): n/a
  - `<unnamed>` (SCE02_ConcurrentExecuteAndRegister): n/a

#### `TEST_F(SchedulerConcurrencyTest, SCE03_UnregisterWhileRunning)`
- Source: `tests/scheduler/test_scheduler_concurrency_edge_cases.cpp`:130
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerConcurrencyTest): n/a
  - `<unnamed>` (SCE03_UnregisterWhileRunning): n/a

#### `TEST_F(SchedulerConcurrencyTest, SCE04_ListDuringConcurrentModification)`
- Source: `tests/scheduler/test_scheduler_concurrency_edge_cases.cpp`:177
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerConcurrencyTest): n/a
  - `<unnamed>` (SCE04_ListDuringConcurrentModification): n/a

#### `TEST_F(SchedulerConcurrencyTest, SCE05_StatsDuringExecution)`
- Source: `tests/scheduler/test_scheduler_concurrency_edge_cases.cpp`:226
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerConcurrencyTest): n/a
  - `<unnamed>` (SCE05_StatsDuringExecution): n/a

#### `TEST_F(SchedulerConcurrencyTest, SCE06_TriggerEvaluationConcurrent)`
- Source: `tests/scheduler/test_scheduler_concurrency_edge_cases.cpp`:272
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerConcurrencyTest): n/a
  - `<unnamed>` (SCE06_TriggerEvaluationConcurrent): n/a

#### `TEST_F(SchedulerConcurrencyTest, SCE07_AnomalyDetectionConcurrent)`
- Source: `tests/scheduler/test_scheduler_concurrency_edge_cases.cpp`:310
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerConcurrencyTest): n/a
  - `<unnamed>` (SCE07_AnomalyDetectionConcurrent): n/a

#### `TEST_F(SchedulerConcurrencyTest, SCE08_LockOrderingConsistency)`
- Source: `tests/scheduler/test_scheduler_concurrency_edge_cases.cpp`:349
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerConcurrencyTest): n/a
  - `<unnamed>` (SCE08_LockOrderingConsistency): n/a

#### `TEST_F(SchedulerCoordinationFailureTest, SCF01_ExecuteFailsClosedWithoutCoordinator)`
- Source: `tests/scheduler/test_scheduler_coordination_failures.cpp`:46
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerCoordinationFailureTest): n/a
  - `<unnamed>` (SCF01_ExecuteFailsClosedWithoutCoordinator): n/a

#### `TEST_F(SchedulerCoordinationFailureTest, SCF02_RetryOnTransientFailure)`
- Source: `tests/scheduler/test_scheduler_coordination_failures.cpp`:69
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerCoordinationFailureTest): n/a
  - `<unnamed>` (SCF02_RetryOnTransientFailure): n/a

#### `TEST_F(SchedulerCoordinationFailureTest, SCF03_LeaderElectionDuringExecution)`
- Source: `tests/scheduler/test_scheduler_coordination_failures.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerCoordinationFailureTest): n/a
  - `<unnamed>` (SCF03_LeaderElectionDuringExecution): n/a

#### `TEST_F(SchedulerCoordinationFailureTest, SCF04_DeactivateOnLeadershipLoss)`
- Source: `tests/scheduler/test_scheduler_coordination_failures.cpp`:101
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerCoordinationFailureTest): n/a
  - `<unnamed>` (SCF04_DeactivateOnLeadershipLoss): n/a

#### `TEST_F(SchedulerCoordinationFailureTest, SCF05_TaskRegistryConsistency)`
- Source: `tests/scheduler/test_scheduler_coordination_failures.cpp`:117
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerCoordinationFailureTest): n/a
  - `<unnamed>` (SCF05_TaskRegistryConsistency): n/a

#### `TEST_F(SchedulerCoordinationFailureTest, SCF06_CascadeFailurePrevention)`
- Source: `tests/scheduler/test_scheduler_coordination_failures.cpp`:132
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerCoordinationFailureTest): n/a
  - `<unnamed>` (SCF06_CascadeFailurePrevention): n/a

#### `TEST_F(SchedulerCoordinationFailureTest, SCF07_DiagnosticsDuringOutage)`
- Source: `tests/scheduler/test_scheduler_coordination_failures.cpp`:147
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerCoordinationFailureTest): n/a
  - `<unnamed>` (SCF07_DiagnosticsDuringOutage): n/a

#### `TEST_F(SchedulerCoordinationFailureTest, SCF08_RecoveryFromProlongedOutage)`
- Source: `tests/scheduler/test_scheduler_coordination_failures.cpp`:162
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerCoordinationFailureTest): n/a
  - `<unnamed>` (SCF08_RecoveryFromProlongedOutage): n/a

#### `TEST_F(SchedulerStressRetentionTest, SSB01_BurstRegistration100)`
- Source: `tests/scheduler/test_scheduler_stress_burst_retention.cpp`:49
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerStressRetentionTest): n/a
  - `<unnamed>` (SSB01_BurstRegistration100): n/a

#### `TEST_F(SchedulerStressRetentionTest, SSB02_SustainedExecutionUnderBurst)`
- Source: `tests/scheduler/test_scheduler_stress_burst_retention.cpp`:90
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerStressRetentionTest): n/a
  - `<unnamed>` (SSB02_SustainedExecutionUnderBurst): n/a

#### `TEST_F(SchedulerStressRetentionTest, SSB03_RetentionLimitEnforcement)`
- Source: `tests/scheduler/test_scheduler_stress_burst_retention.cpp`:118
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerStressRetentionTest): n/a
  - `<unnamed>` (SSB03_RetentionLimitEnforcement): n/a

#### `TEST_F(SchedulerStressRetentionTest, SSB04_RetentionEviction)`
- Source: `tests/scheduler/test_scheduler_stress_burst_retention.cpp`:160
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerStressRetentionTest): n/a
  - `<unnamed>` (SSB04_RetentionEviction): n/a

#### `TEST_F(SchedulerStressRetentionTest, SSB05_TriggerEvaluationSustained)`
- Source: `tests/scheduler/test_scheduler_stress_burst_retention.cpp`:175
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerStressRetentionTest): n/a
  - `<unnamed>` (SSB05_TriggerEvaluationSustained): n/a

#### `TEST_F(SchedulerStressRetentionTest, SSB06_AnomalyDetectionReliability)`
- Source: `tests/scheduler/test_scheduler_stress_burst_retention.cpp`:205
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerStressRetentionTest): n/a
  - `<unnamed>` (SSB06_AnomalyDetectionReliability): n/a

#### `TEST_F(SchedulerStressRetentionTest, SSB07_MemoryStabilityBurst)`
- Source: `tests/scheduler/test_scheduler_stress_burst_retention.cpp`:235
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerStressRetentionTest): n/a
  - `<unnamed>` (SSB07_MemoryStabilityBurst): n/a

#### `TEST_F(SchedulerStressRetentionTest, SSB08_OperationDeterminism)`
- Source: `tests/scheduler/test_scheduler_stress_burst_retention.cpp`:263
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchedulerStressRetentionTest): n/a
  - `<unnamed>` (SSB08_OperationDeterminism): n/a

### themis::scheduler::test::SchedulerCoordinationFailureTest

#### `void SetUp() override`
- Source: `tests/scheduler/test_scheduler_coordination_failures.cpp`:34
- Brief: n/a
- Parameters: none

### themis::scheduler::test::SchedulerStressRetentionTest

#### `void SetUp() override`
- Source: `tests/scheduler/test_scheduler_stress_burst_retention.cpp`:35
- Brief: n/a
- Parameters: none

