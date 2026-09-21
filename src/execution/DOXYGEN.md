# EXECUTION DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-21
Last Updated: 2026-09-21
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `/tmp/module-doxygen-execution/execution/xml/index.xml`
- Warnings log: `/tmp/module-doxygen-execution/execution/doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 6
- Compounds: 27
- Classes/Structs: 10
- Namespaces: 3
- File Compounds: 6

## Namespaces
- themis
- themis::execution
- themis::resource

## Types
### Classes
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
- Functions extracted: 29

### themis::execution::QueryScheduler

#### `QueryScheduler()`
- Source: `include/execution/query_scheduler.h`:96
- Brief: Construct a scheduler with default configuration.
- Parameters: none

#### `QueryScheduler(const Config &cfg)`
- Source: `include/execution/query_scheduler.h`:102
- Brief: Construct a scheduler with an explicit configuration snapshot.
- Parameters:
  - `cfg` (const Config &): Runtime limits and reserved compatibility settings.
- Details: cfg Runtime limits and reserved compatibility settings.

#### `QueryScheduler(const QueryScheduler &other)=delete`
- Source: `include/execution/query_scheduler.h`:113
- Brief: Copying is disabled because the scheduler owns synchronization state.
- Parameters:
  - `other` (const QueryScheduler &): Unused source scheduler instance.
- Details: other Unused source scheduler instance.

#### `bool dequeue(QueryEntry &out, std::chrono::milliseconds timeout=std::chrono::seconds(5))`
- Source: `include/execution/query_scheduler.h`:146
- Brief: Dequeue the next query ordered by absolute deadline.
- Parameters:
  - `out` (QueryEntry &): Receives the dequeued entry on success.
  - `timeout` (std::chrono::milliseconds): Maximum time to wait for queued work.
- Return: true when an entry was dequeued, otherwise false if the wait timed out or the scheduler reached shutdown with no remaining work.
- Details: out Receives the dequeued entry on success. timeout Maximum time to wait for an entry before returning false. true when an entry was dequeued, otherwise false if the wait timed out or the scheduler reached shutdown with no remaining work. out Receives the dequeued entry on success. timeout Maximum time to wait for queued work. true when an entry was dequeued, otherwise false.

#### `std::uint64_t enqueue(QueryEntry::ExecuteFn execute, SLAPriority priority=SLAPriority::MEDIUM, long sla_ms=50, std::string name={}, std::chrono::milliseconds timeout=std::chrono::seconds(5))`
- Source: `include/execution/query_scheduler.h`:132
- Brief: Enqueue a query for later execution.
- Parameters:
  - `execute` (QueryEntry::ExecuteFn): Callable stored in the queued entry.
  - `priority` (SLAPriority): SLA label used for metrics and shedding decisions.
  - `sla_ms` (long): Relative deadline budget in milliseconds.
  - `name` (std::string): Optional diagnostic name.
  - `timeout` (std::chrono::milliseconds): Maximum time to wait for queue capacity.
- Return: A non-zero query id on success, or 0 if shutdown is in progress, the wait for capacity times out, or a LOW-priority entry is shed.
- Details: Enqueue one execution request after waiting for bounded capacity. execute Callable owned by the queued entry. priority Caller-provided SLA label used for metrics and shedding. sla_ms Relative SLA budget in milliseconds; converted to an absolute deadline at enqueue time. name Optional diagnostic name stored with the queued entry. timeout Maximum time to wait for queue capacity before rejecting the submission. A non-zero query id on success, or 0 if shutdown is in progress, the wait for capacity times out, or a LOW-priority entry is shed. execute Callable stored in the queued entry. priority SLA label used for metrics and shedding decisions. sla_ms Relative deadline budget in milliseconds. name Optional diagnostic name. timeout Maximum time to wait for queue capacity. A non-zero query id on success, or 0 if the queue stays full, the scheduler is shutting down, or a LOW-priority item is shed.

#### `bool is_shutdown() const noexcept`
- Source: `include/execution/query_scheduler.h`:183
- Brief: Report whether shutdown has started.
- Parameters: none
- Return: true once shutdown has been requested.
- Details: true once shutdown has been requested.

#### `Metrics metrics() const noexcept`
- Source: `include/execution/query_scheduler.h`:166
- Brief: Return a consistent snapshot of the current scheduler metrics.
- Parameters: none
- Return: Scheduler metrics copied under the internal mutex.
- Details: Scheduler metrics copied under the internal mutex.

#### `QueryScheduler & operator=(const QueryScheduler &other)=delete`
- Source: `include/execution/query_scheduler.h`:120
- Brief: Copy assignment is disabled because the scheduler owns synchronization state.
- Parameters:
  - `other` (const QueryScheduler &): Unused source scheduler instance.
- Return: This scheduler instance; the operator is deleted and cannot be used.
- Details: other Unused source scheduler instance. This scheduler instance; the operator is deleted and cannot be used.

#### `void reportCompletion(std::uint64_t query_id, std::chrono::steady_clock::time_point completion_time=std::chrono::steady_clock::now())`
- Source: `include/execution/query_scheduler.h`:157
- Brief: Report completion status for a previously enqueued query id.
- Parameters:
  - `query_id` (std::uint64_t): Identifier returned by enqueue().
  - `completion_time` (std::chrono::steady_clock::time_point): Completion timestamp used for SLA accounting.
- Details: Record the observed completion time for a previously queued query. query_id Identifier returned by enqueue(). completion_time Completion timestamp used for SLA accounting. Missing ids are ignored. Callers that rely on Metrics::completed_total and Metrics::sla_compliance_pct must invoke this method explicitly. query_id Identifier returned by enqueue(). completion_time Completion timestamp used for SLA accounting.

#### `void shutdown() noexcept`
- Source: `include/execution/query_scheduler.h`:177
- Brief: Stop accepting new entries and wake blocked waiters.
- Parameters: none

#### `std::size_t size() const noexcept`
- Source: `include/execution/query_scheduler.h`:172
- Brief: Return the current queue depth.
- Parameters: none
- Return: Number of entries currently stored in the scheduler queue.
- Details: Number of entries currently stored in the scheduler queue.

#### `~QueryScheduler()`
- Source: `include/execution/query_scheduler.h`:107
- Brief: Destroy the scheduler after initiating shutdown.
- Parameters: none

### themis::execution::QueryScheduler::EarliestDeadlineFirst

#### `bool operator()(const QueryEntry &a, const QueryEntry &b) const`
- Source: `include/execution/query_scheduler.h`:198
- Brief: Order two queued entries for the internal priority queue.
- Parameters:
  - `a` (const QueryEntry &): Left-hand entry.
  - `b` (const QueryEntry &): Right-hand entry.
- Return: true when a should be ordered behind b.
- Details: a Left-hand entry. b Right-hand entry. true when a should be ordered behind b.

### themis::resource::WorkItem

#### `WorkItem(Fn f, std::string n={})`
- Source: `include/execution/thread_pool_manager.h`:44
- Brief: Construct a work item.
- Parameters:
  - `f` (Fn): Callable to run on a worker thread.
  - `n` (std::string): Optional diagnostic name.
- Details: f Callable to run on a worker thread. n Optional diagnostic name.

### themis::resource::WorkStealingThreadPool

#### `WorkStealingThreadPool()`
- Source: `include/execution/thread_pool_manager.h`:82
- Brief: Construct a pool with default configuration.
- Parameters: none

#### `WorkStealingThreadPool(const Config &cfg)`
- Source: `include/execution/thread_pool_manager.h`:88
- Brief: Construct a pool with an explicit configuration.
- Parameters:
  - `cfg` (const Config &): Worker-count and queue-capacity settings.
- Details: cfg Worker-count and queue-capacity settings.

#### `WorkStealingThreadPool(const WorkStealingThreadPool &other)=delete`
- Source: `include/execution/thread_pool_manager.h`:99
- Brief: Copying is disabled because the pool owns worker threads and synchronization state.
- Parameters:
  - `other` (const WorkStealingThreadPool &): Unused source pool instance.
- Details: other Unused source pool instance.

#### `bool is_shutdown() const noexcept`
- Source: `include/execution/thread_pool_manager.h`:151
- Brief: Report whether shutdown has started.
- Parameters: none
- Return: true once shutdown has been requested.
- Details: true once shutdown has been requested.

#### `WorkStealingThreadPool & operator=(const WorkStealingThreadPool &other)=delete`
- Source: `include/execution/thread_pool_manager.h`:106
- Brief: Copy assignment is disabled because the pool owns worker threads and synchronization state.
- Parameters:
  - `other` (const WorkStealingThreadPool &): Unused source pool instance.
- Return: This pool instance; the operator is deleted and cannot be used.
- Details: other Unused source pool instance. This pool instance; the operator is deleted and cannot be used.

#### `void shutdown(std::chrono::milliseconds drain_timeout=std::chrono::seconds(30))`
- Source: `include/execution/thread_pool_manager.h`:145
- Brief: Stop accepting work, drain pending items, wake workers, and join them.
- Parameters:
  - `drain_timeout` (std::chrono::milliseconds): Maximum time to wait for pending items to drain.
- Details: Drain the queue, wake all workers, and join them. drain_timeout Maximum time to wait for queue drain before forcing the wake/join sequence. drain_timeout Maximum time to wait for pending items to drain.

#### `Statistics statistics() const noexcept`
- Source: `include/execution/thread_pool_manager.h`:139
- Brief: Return a snapshot of worker, queue, and latency statistics.
- Parameters: none
- Return: Statistics copied from the current pool state.
- Details: Statistics copied from the current pool state.

#### `bool submit(WorkItem item, std::chrono::milliseconds timeout=std::chrono::seconds(5))`
- Source: `include/execution/thread_pool_manager.h`:115
- Brief: Submit a work item to the bounded dispatch queue.
- Parameters:
  - `item` (WorkItem): Work item to enqueue.
  - `timeout` (std::chrono::milliseconds): Maximum time to wait for queue capacity.
- Return: true on success, or false if shutdown is in progress or the queue stays full until timeout elapses.
- Details: Submit one work item into the bounded central dispatch queue. item Work item to move into the pool. timeout Maximum time to wait for queue capacity. true on success, or false if shutdown is in progress or the queue stays full until timeout elapses. item Work item to enqueue. timeout Maximum time to wait for queue capacity. true when the work item was accepted, otherwise false.

#### `bool submit(std::function< void()> fn, std::string name={}, std::chrono::milliseconds timeout=std::chrono::seconds(5))`
- Source: `include/execution/thread_pool_manager.h`:125
- Brief: Submit a callable directly.
- Parameters:
  - `fn` (std::function< void()>): Callable to execute.
  - `name` (std::string): Optional diagnostic label.
  - `timeout` (std::chrono::milliseconds): Maximum time to wait for queue capacity.
- Return: Same result semantics as submit(WorkItem, timeout).
- Details: fn Callable to execute. name Optional diagnostic label. timeout Maximum time to wait for queue capacity. Same result semantics as submit(WorkItem, timeout).

#### `std::size_t thread_count() const noexcept`
- Source: `include/execution/thread_pool_manager.h`:159
- Brief: Return the number of currently active worker threads.
- Parameters: none
- Return: Active worker count.
- Details: Active worker count.

#### `bool tryGetWork(std::size_t own_idx, WorkItem &out)`
- Source: `include/execution/thread_pool_manager.h`:201
- Brief: Try to obtain the next work item.
- Parameters:
  - `own_idx` (std::size_t): Index of the requesting worker (reserved for future steal paths).
  - `out` (WorkItem &): Receives the acquired work item on success.
- Return: true when work was acquired.
- Details: Try to acquire one pending work item for a worker. own_idx Index of the requesting worker. out Receives the next work item on success. true when work was acquired. The current implementation reads only from the shared dispatch queue. own_idx is reserved for future steal-path activation. own_idx Index of the requesting worker (reserved for future steal paths). out Receives the acquired work item on success. true when a work item was acquired from the shared dispatch queue.

#### `bool waitAll(std::chrono::milliseconds timeout=std::chrono::seconds(30))`
- Source: `include/execution/thread_pool_manager.h`:133
- Brief: Wait until the pending queue drains or the timeout elapses.
- Parameters:
  - `timeout` (std::chrono::milliseconds): Maximum time to poll for queue drain completion.
- Return: true when the queue drained before the deadline, otherwise false.
- Details: Wait until the bounded dispatch queue drains or the timeout elapses. timeout Maximum time to poll for an empty queue. true when the queue drained before the deadline, otherwise false. timeout Maximum time to poll for queue drain completion. true when the queue drained before the deadline, otherwise false.

#### `void workerLoop(std::size_t thread_idx)`
- Source: `include/execution/thread_pool_manager.h`:190
- Brief: Worker-thread main loop.
- Parameters:
  - `thread_idx` (std::size_t): Worker index used for reserved per-thread bookkeeping.
- Details: Run the worker loop until shutdown is requested. thread_idx Index of the worker inside the pre-created queue array. thread_idx Worker index used for reserved per-thread bookkeeping.

#### `~WorkStealingThreadPool()`
- Source: `include/execution/thread_pool_manager.h`:93
- Brief: Destroy the pool after initiating shutdown.
- Parameters: none

### themis::resource::WorkStealingThreadPool::ThreadQueue

#### `bool trySteal(WorkItem &out)`
- Source: `include/execution/thread_pool_manager.h`:175
- Brief: Try to steal one item from the back of a reserved per-thread queue.
- Parameters:
  - `out` (WorkItem &): Receives the stolen work item on success.
- Return: true when a work item was available.
- Details: out Receives the stolen work item on success. true when a work item was available.

