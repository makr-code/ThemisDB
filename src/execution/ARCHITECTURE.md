# Execution Module — Architecture

<!-- Status: PRODUCTION_READY | validated: 2026-08-08 -->

## Overview

The execution module provides the runtime execution substrate for ThemisDB query processing, combining deadline-driven query scheduling with adaptive work-stealing thread pooling. The architecture separates concerns between query scheduling (priority, deadlines, SLAs) and execution (worker threads, load balancing, resource utilization).

## Design Principles

1. **Deadline-Driven Scheduling:** Queries carry SLA deadlines; scheduler enforces deadline constraints through priority queuing and timeout mechanisms
2. **Work-Stealing Parallelism:** Idle workers steal work from busy workers to balance load and maximize throughput
3. **Adaptive Resource Management:** Thread pool sizing adapts dynamically to workload; memory and CPU constraints are enforced
4. **Fail-Closed Degradation:** Resource exhaustion triggers structured errors, not undefined behavior
5. **Observable:** All scheduling decisions and resource state changes are logged with diagnostic context

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│  Query Scheduler (SLA-Aware)                                │
│  • Priority Queue: CRITICAL > HIGH > NORMAL > LOW            │
│  • Deadline Tracking: enqueue_time + sla_deadline_ms         │
│  • Backpressure: max_queue_depth enforcement                 │
│  • Timeout Handling: on enqueue/dequeue operations           │
└──────────────────────┬──────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────┐
│  Thread Pool Manager (Adaptive)                             │
│  • Worker Threads: min_threads to max_threads               │
│  • Per-Thread Task Queue: work-stealing enabled             │
│  • Adaptive Spawning: scale workers based on queue depth    │
│  • Graceful Shutdown: complete in-flight work               │
└──────────────────────┬──────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────┐
│  Work Distribution & Execution                              │
│  • FIFO Dispatch: within priority level                     │
│  • Work Stealing: idle workers steal from busy peers        │
│  • Resource Accounting: CPU, memory per query               │
│  • Timeout Enforcement: deadline-based cancellation         │
└─────────────────────────────────────────────────────────────┘
```

## Core Components

### Query Scheduler

**Purpose:** Manage query entry points with SLA deadline enforcement and priority-based dispatch.

**Responsibilities:**
- Enqueue queries with deadline computation (relative to SLA policy)
- Dequeue queries respecting priority order and deadline constraints
- Track queue depth and apply backpressure when limits exceeded
- Report deadline violations and queue saturation events
- Support graceful shutdown with remaining query completion

**Key Contracts:**
- `enqueue(query, sla_deadline_ms) → Result<QueueToken>`
  - Computes absolute deadline = now() + sla_deadline_ms
  - Returns error if queue full or deadline already expired
  - Thread-safe; lock-free for query entry reads
  
- `dequeue() → Result<Query>`
  - Returns highest-priority query within deadline
  - Returns empty if queue empty or all queries expired
  - Atomic state transition to "executing"
  
- `get_queue_depth() → size_t`
  - Returns current queue size
  - No locking required (atomic read)

**Error Codes (E7100–E7199):**
- E7100: Queue depth exceeded
- E7101: Enqueue timeout (deadline expired during wait)
- E7102: Thread spawn failure (max_threads limit)
- E7103: Work steal timeout
- E7104: Shutdown in progress

### Thread Pool Manager

**Purpose:** Manage worker threads with adaptive scaling and work-stealing load balancing.

**Responsibilities:**
- Spawn and terminate worker threads based on queue depth
- Implement per-thread task queues with work-stealing capability
- Balance load across workers via work-stealing algorithm
- Track worker utilization and scale limits
- Support graceful shutdown with task completion

**Key Contracts:**
- `create(min_threads, max_threads) → ThreadPoolManager*`
  - Creates thread pool with adaptive scaling bounds
  - Initially spawns min_threads workers
  
- `schedule(task) → Result<TaskToken>`
  - Submits task for execution
  - Returns token for cancellation/monitoring
  - May spawn new worker if queue depth exceeds threshold
  
- `shutdown(wait_ms) → Result<>`
  - Stops accepting new tasks
  - Completes in-flight work within wait_ms timeout
  - Returns error if timeout exceeded

**Scaling Strategy:**
```
if queue_depth > high_watermark && active_threads < max_threads:
  spawn_new_worker()
if queue_depth < low_watermark && active_threads > min_threads:
  signal_worker_shutdown()
```

## Concurrency Model

### Thread Safety

1. **Query Queue:** Protected by single spinlock for enqueue/dequeue operations
   - Lock held only for queue state update (< 1 µs)
   - Query entries themselves are lock-free (atomic reads)

2. **Thread Pool State:** Atomic flags for shutdown and scaling decisions
   - No mutex for thread count updates (atomic increment/decrement)
   - Worker termination signaled via atomic flag

3. **Per-Thread Task Queues:** Lock-free with compare-and-swap operations
   - Only touched by owning thread (no contention for common case)
   - Work-stealing uses non-blocking dequeue for competing threads

### Synchronization Primitives

- `std::atomic<size_t>` for queue depth (reader-optimal)
- `std::mutex` + `std::condition_variable` for queue notification
- Per-thread `std::atomic<bool>` for shutdown signals

## Performance Characteristics

### Target Latencies (P99)

- **Enqueue:** < 5 ms (99th percentile)
- **Dequeue:** < 100 µs
- **Work-Steal:** < 200 µs
- **Thread Spawn:** < 10 ms per worker
- **Queue Throughput:** ≥ 10k queries/sec at 16 threads

### Scaling Behavior

- **Vertical Scaling:** Nearly linear speedup up to available CPUs
- **Horizontal Scaling:** Supports cross-node coordination via distributed task queue (future)

### Resource Consumption

- **Per-Worker Memory:** ~2 MB (task queue, TLS)
- **Queue Overhead:** O(queue_depth) memory

## SLA Enforcement

### Deadline Computation

```cpp
absolute_deadline = now_ms + sla_deadline_ms
if (absolute_deadline <= now_ms):
  return error(E7101); // Deadline already expired
if (queue_depth >= max_queue_depth):
  apply_backpressure();
```

### Violation Detection

- Scheduler tracks deadline violations for reporting
- Violating queries are prioritized for execution or rejected
- Operator runbook: increase `min_threads` or reduce `max_queue_depth`

## See Also

- [`ROADMAP.md`](ROADMAP.md) — Implementation phases and deliverables
- [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md) — Planned features
- [`../../include/execution/query_scheduler.h`](../../include/execution/query_scheduler.h) — Public API
- [`../../include/execution/thread_pool_manager.h`](../../include/execution/thread_pool_manager.h) — Public API
