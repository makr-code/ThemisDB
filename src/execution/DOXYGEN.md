# EXECUTION DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\execution\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\execution\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 6
- Compounds: 33
- Classes/Structs: 15
- Namespaces: 4
- File Compounds: 6

## Namespaces
- testing
- themis
- themis::execution
- themis::resource

## Types
### Classes
- BoundedQueue
- ExecutionHighCardinalityStress
- StealPool
- StubBoundedQueue
- StubWorkStealPool
- themis::execution::QueryScheduler
- themis::resource::WorkStealingThreadPool

### Structs
- themis::execution::QueryEntry
- themis::execution::QueryScheduler::Config
- themis::execution::QueryScheduler::EarliestDeadlineFirst
- themis::execution::QueryScheduler::Metrics
- themis::resource::WorkItem
- themis::resource::WorkStealingThreadPool::Config
- themis::resource::WorkStealingThreadPool::Statistics
- themis::resource::WorkStealingThreadPool::ThreadQueue

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 61

### BoundedQueue

#### `BoundedQueue(std::size_t capacity)`
- Source: `benchmarks/execution/bench_execution_dedicated_gates.cpp`:43
- Brief: n/a
- Parameters:
  - `capacity` (std::size_t): n/a

#### `bool dequeue(uint64_t &out)`
- Source: `benchmarks/execution/bench_execution_dedicated_gates.cpp`:52
- Brief: n/a
- Parameters:
  - `out` (uint64_t &): n/a

#### `bool enqueue(uint64_t item)`
- Source: `benchmarks/execution/bench_execution_dedicated_gates.cpp`:45
- Brief: n/a
- Parameters:
  - `item` (uint64_t): n/a

#### `std::size_t size() const`
- Source: `benchmarks/execution/bench_execution_dedicated_gates.cpp`:60
- Brief: n/a
- Parameters: none

### StealPool

#### `StealPool(unsigned workers)`
- Source: `benchmarks/execution/bench_execution_dedicated_gates.cpp`:76
- Brief: n/a
- Parameters:
  - `workers` (unsigned): n/a

#### `bool steal(uint64_t &out)`
- Source: `benchmarks/execution/bench_execution_dedicated_gates.cpp`:85
- Brief: n/a
- Parameters:
  - `out` (uint64_t &): n/a

#### `void submit(uint64_t task_id)`
- Source: `benchmarks/execution/bench_execution_dedicated_gates.cpp`:79
- Brief: n/a
- Parameters:
  - `task_id` (uint64_t): n/a

### StubBoundedQueue

#### `StubBoundedQueue(std::size_t capacity)`
- Source: `tests/execution/test_execution_highcardinality_stress.cpp`:43
- Brief: n/a
- Parameters:
  - `capacity` (std::size_t): n/a

#### `bool dequeue(uint64_t &out)`
- Source: `tests/execution/test_execution_highcardinality_stress.cpp`:53
- Brief: n/a
- Parameters:
  - `out` (uint64_t &): n/a

#### `uint64_t dequeued() const`
- Source: `tests/execution/test_execution_highcardinality_stress.cpp`:68
- Brief: n/a
- Parameters: none

#### `bool enqueue(uint64_t item)`
- Source: `tests/execution/test_execution_highcardinality_stress.cpp`:45
- Brief: n/a
- Parameters:
  - `item` (uint64_t): n/a

#### `uint64_t enqueued() const`
- Source: `tests/execution/test_execution_highcardinality_stress.cpp`:67
- Brief: n/a
- Parameters: none

#### `uint64_t overflow() const`
- Source: `tests/execution/test_execution_highcardinality_stress.cpp`:69
- Brief: n/a
- Parameters: none

#### `std::size_t size() const`
- Source: `tests/execution/test_execution_highcardinality_stress.cpp`:62
- Brief: n/a
- Parameters: none

### StubWorkStealPool

#### `StubWorkStealPool(unsigned workers)`
- Source: `tests/execution/test_execution_highcardinality_stress.cpp`:85
- Brief: n/a
- Parameters:
  - `workers` (unsigned): n/a

#### `void drainStep()`
- Source: `tests/execution/test_execution_highcardinality_stress.cpp`:96
- Brief: n/a
- Parameters: none

#### `uint64_t processed() const`
- Source: `tests/execution/test_execution_highcardinality_stress.cpp`:107
- Brief: n/a
- Parameters: none

#### `uint64_t remaining() const`
- Source: `tests/execution/test_execution_highcardinality_stress.cpp`:109
- Brief: n/a
- Parameters: none

#### `void submit(uint64_t task_id)`
- Source: `tests/execution/test_execution_highcardinality_stress.cpp`:88
- Brief: n/a
- Parameters:
  - `task_id` (uint64_t): n/a

#### `uint64_t submitted() const`
- Source: `tests/execution/test_execution_highcardinality_stress.cpp`:106
- Brief: n/a
- Parameters: none

### bench_execution_dedicated_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/execution/bench_execution_dedicated_gates.cpp`:234
- Brief: n/a
- Parameters: none

#### `void BM_EX_BM_01_EnqueueP95(benchmark::State &state)`
- Source: `benchmarks/execution/bench_execution_dedicated_gates.cpp`:110
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_EX_BM_02_DequeueP95(benchmark::State &state)`
- Source: `benchmarks/execution/bench_execution_dedicated_gates.cpp`:135
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_EX_BM_03_WorkStealThroughput(benchmark::State &state)`
- Source: `benchmarks/execution/bench_execution_dedicated_gates.cpp`:165
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_EX_BM_04_ConcurrentDispatchThroughput(benchmark::State &state)`
- Source: `benchmarks/execution/bench_execution_dedicated_gates.cpp`:197
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Name("EX-BM-01/EnqueueP95") -> Repetitions(5) ->ReportAggregatesOnly(true) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/execution/bench_execution_dedicated_gates.cpp`:125
- Brief: n/a
- Parameters:
  - `<unnamed>` ("EX-BM-01/EnqueueP95"): n/a

#### `Name("EX-BM-02/DequeueP95") -> Repetitions(5) ->ReportAggregatesOnly(true) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/execution/bench_execution_dedicated_gates.cpp`:154
- Brief: n/a
- Parameters:
  - `<unnamed>` ("EX-BM-02/DequeueP95"): n/a

#### `Name("EX-BM-03/WorkStealThroughput") -> Arg(10000) ->Repetitions(5) ->ReportAggregatesOnly(true) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/execution/bench_execution_dedicated_gates.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` ("EX-BM-03/WorkStealThroughput"): n/a

#### `Name("EX-BM-04/ConcurrentDispatchThroughput") -> Arg(5000) ->Repetitions(3) ->ReportAggregatesOnly(true) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/execution/bench_execution_dedicated_gates.cpp`:228
- Brief: n/a
- Parameters:
  - `<unnamed>` ("EX-BM-04/ConcurrentDispatchThroughput"): n/a

### test_execution_highcardinality_stress.cpp

#### `TEST_F(ExecutionHighCardinalityStress, ConcurrentQueueStress)`
- Source: `tests/execution/test_execution_highcardinality_stress.cpp`:180
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExecutionHighCardinalityStress): n/a
  - `<unnamed>` (ConcurrentQueueStress): n/a

#### `TEST_F(ExecutionHighCardinalityStress, HighCardinalityTaskDispatch)`
- Source: `tests/execution/test_execution_highcardinality_stress.cpp`:142
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExecutionHighCardinalityStress): n/a
  - `<unnamed>` (HighCardinalityTaskDispatch): n/a

#### `TEST_F(ExecutionHighCardinalityStress, WorkStealingUnderLoad)`
- Source: `tests/execution/test_execution_highcardinality_stress.cpp`:237
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExecutionHighCardinalityStress): n/a
  - `<unnamed>` (WorkStealingUnderLoad): n/a

### themis::execution::QueryScheduler

#### `QueryScheduler()`
- Source: `include/execution/query_scheduler.h`:100
- Brief: n/a
- Parameters: none

#### `QueryScheduler(const Config &cfg)`
- Source: `include/execution/query_scheduler.h`:107
- Brief: Query Scheduler.
- Parameters:
  - `cfg` (const Config &): Input parameter.
- Return: Return value.
- Details: cfg Input parameter. Return value.

#### `QueryScheduler(const QueryScheduler &)=delete`
- Source: `include/execution/query_scheduler.h`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (const QueryScheduler &): n/a

#### `bool dequeue(QueryEntry &out, std::chrono::milliseconds timeout=std::chrono::seconds(5))`
- Source: `include/execution/query_scheduler.h`:122
- Brief: Dequeue.
- Parameters:
  - `out` (QueryEntry &): Input/output parameter.
  - `timeout` (std::chrono::milliseconds): Input parameter.
- Return: True when the operation succeeds.
- Details: out Input/output parameter. timeout Input parameter. True when the operation succeeds. Calls: std::chrono::steady_clock::now(), lk(), wait_until(), empty(), load(), std::move(), top(), pop().

#### `std::uint64_t enqueue(QueryEntry::ExecuteFn execute, SLAPriority priority=SLAPriority::MEDIUM, long sla_ms=50, std::string name={}, std::chrono::milliseconds timeout=std::chrono::seconds(5))`
- Source: `include/execution/query_scheduler.h`:115
- Brief: Enqueue.
- Parameters:
  - `execute` (QueryEntry::ExecuteFn): Input parameter.
  - `priority` (SLAPriority): Input parameter.
  - `sla_ms` (long): Input parameter.
  - `name` (std::string): Input parameter.
  - `timeout` (std::chrono::milliseconds): Input parameter.
- Return: Return value.
- Details: execute Input parameter. priority Input parameter. sla_ms Input parameter. name Input parameter. timeout Input parameter. Return value. Calls: load(), std::chrono::steady_clock::now(), std::chrono::milliseconds(), lk(), wait_until(), size(), fetch_add(), std::move().

#### `bool is_shutdown() const noexcept`
- Source: `include/execution/query_scheduler.h`:140
- Brief: n/a
- Parameters: none

#### `Metrics metrics() const noexcept`
- Source: `include/execution/query_scheduler.h`:130
- Brief: n/a
- Parameters: none

#### `QueryScheduler & operator=(const QueryScheduler &)=delete`
- Source: `include/execution/query_scheduler.h`:113
- Brief: n/a
- Parameters:
  - `<unnamed>` (const QueryScheduler &): n/a

#### `void reportCompletion(std::uint64_t query_id, std::chrono::steady_clock::time_point completion_time=std::chrono::steady_clock::now())`
- Source: `include/execution/query_scheduler.h`:125
- Brief: Report Completion.
- Parameters:
  - `query_id` (std::uint64_t): Identifier of the query.
  - `completion_time` (std::chrono::steady_clock::time_point): Input parameter.
- Details: query_id Identifier of the query. completion_time Input parameter. Calls: lk(), find(), end(), erase().

#### `void shutdown() noexcept`
- Source: `include/execution/query_scheduler.h`:138
- Brief: Shutdown.
- Parameters: none
- Details: Exception safety: noexcept.

#### `std::size_t size() const noexcept`
- Source: `include/execution/query_scheduler.h`:132
- Brief: n/a
- Parameters: none

#### `~QueryScheduler()`
- Source: `include/execution/query_scheduler.h`:109
- Brief: n/a
- Parameters: none

### themis::execution::QueryScheduler::EarliestDeadlineFirst

#### `bool operator()(const QueryEntry &a, const QueryEntry &b) const`
- Source: `include/execution/query_scheduler.h`:147
- Brief: n/a
- Parameters:
  - `a` (const QueryEntry &): n/a
  - `b` (const QueryEntry &): n/a

### themis::resource::WorkItem

#### `WorkItem(Fn f, std::string n={})`
- Source: `include/execution/thread_pool_manager.h`:48
- Brief: n/a
- Parameters:
  - `f` (Fn): n/a
  - `n` (std::string): n/a

### themis::resource::WorkStealingThreadPool

#### `WorkStealingThreadPool()`
- Source: `include/execution/thread_pool_manager.h`:74
- Brief: n/a
- Parameters: none

#### `WorkStealingThreadPool(const Config &cfg)`
- Source: `include/execution/thread_pool_manager.h`:81
- Brief: Work Stealing Thread Pool.
- Parameters:
  - `cfg` (const Config &): Input parameter.
- Return: Return value.
- Details: cfg Input parameter. Return value.

#### `WorkStealingThreadPool(const WorkStealingThreadPool &)=delete`
- Source: `include/execution/thread_pool_manager.h`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (const WorkStealingThreadPool &): n/a

#### `bool is_shutdown() const noexcept`
- Source: `include/execution/thread_pool_manager.h`:101
- Brief: n/a
- Parameters: none

#### `WorkStealingThreadPool & operator=(const WorkStealingThreadPool &)=delete`
- Source: `include/execution/thread_pool_manager.h`:87
- Brief: n/a
- Parameters:
  - `<unnamed>` (const WorkStealingThreadPool &): n/a

#### `void shutdown(std::chrono::milliseconds drain_timeout=std::chrono::seconds(30))`
- Source: `include/execution/thread_pool_manager.h`:99
- Brief: Shutdown.
- Parameters:
  - `drain_timeout` (std::chrono::milliseconds): Input parameter.
- Details: drain_timeout Input parameter. Calls: exchange(), waitAll(), notify_all(), lk(), joinable(), join(), clear().

#### `Statistics statistics() const noexcept`
- Source: `include/execution/thread_pool_manager.h`:97
- Brief: n/a
- Parameters: none

#### `bool submit(WorkItem item, std::chrono::milliseconds timeout=std::chrono::seconds(5))`
- Source: `include/execution/thread_pool_manager.h`:89
- Brief: Submit.
- Parameters:
  - `item` (WorkItem): Input parameter.
  - `timeout` (std::chrono::milliseconds): Input parameter.
- Return: True when the operation succeeds.
- Details: item Input parameter. timeout Input parameter. True when the operation succeeds. Calls: load(), std::chrono::steady_clock::now(), lk(), wait_until(), push_back(), std::move(), fetch_add(), unlock().

#### `bool submit(std::function< void()> fn, std::string name={}, std::chrono::milliseconds timeout=std::chrono::seconds(5))`
- Source: `include/execution/thread_pool_manager.h`:92
- Brief: n/a
- Parameters:
  - `fn` (std::function< void()>): n/a
  - `name` (std::string): n/a
  - `timeout` (std::chrono::milliseconds): n/a

#### `std::size_t thread_count() const noexcept`
- Source: `include/execution/thread_pool_manager.h`:105
- Brief: n/a
- Parameters: none

#### `bool tryGetWork(std::size_t own_idx, WorkItem &out)`
- Source: `include/execution/thread_pool_manager.h`:144
- Brief: Try Get Work.
- Parameters:
  - `own_idx` (std::size_t): Input parameter.
  - `out` (WorkItem &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: ------------------------------------------------------------------------ tryGetWork — try dispatch queue first, then steal from peers ------------------------------------------------------------------------ own_idx Input parameter. out Input/output parameter. True when the operation succeeds. size_t Input parameter. out Input/output parameter. True when the operation succeeds. Calls: lk(), empty(), std::move(), front(), pop_front(), fetch_sub(), notify_one().

#### `bool waitAll(std::chrono::milliseconds timeout=std::chrono::seconds(30))`
- Source: `include/execution/thread_pool_manager.h`:95
- Brief: Wait All.
- Parameters:
  - `timeout` (std::chrono::milliseconds): Input parameter.
- Return: True when the operation succeeds.
- Details: timeout Input parameter. True when the operation succeeds. Calls: std::chrono::steady_clock::now(), lk(), empty(), load(), std::this_thread::sleep_for(), std::chrono::milliseconds().

#### `void workerLoop(std::size_t thread_idx)`
- Source: `include/execution/thread_pool_manager.h`:137
- Brief: Worker Loop.
- Parameters:
  - `thread_idx` (std::size_t): Input parameter.
- Details: thread_idx Input parameter. thread_idx Input parameter. Calls: std::chrono::milliseconds(), load(), tryGetWork(), lk(), wait_for(), empty(), std::chrono::steady_clock::now(), fn().

#### `~WorkStealingThreadPool()`
- Source: `include/execution/thread_pool_manager.h`:83
- Brief: n/a
- Parameters: none

### themis::resource::WorkStealingThreadPool::ThreadQueue

#### `bool trySteal(WorkItem &out)`
- Source: `include/execution/thread_pool_manager.h`:122
- Brief: Try to steal one item from the back.
- Parameters:
  - `out` (WorkItem &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: out Input/output parameter. True when the operation succeeds. Calls: lk(), empty(), std::move(), back(), pop_back().

