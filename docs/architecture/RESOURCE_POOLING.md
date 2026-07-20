# Resource Pooling Architecture — P3-03

**Version:** 1.0.0  
**Status:** Implemented  
**Phase:** Phase 3 — Block B  
**Maturity Score:** 7/10

---

## Overview

P3-03 delivers three coordinated resource-pool components for ThemisDB:

| Component | Header | Source | Purpose |
|---|---|---|---|
| Slab Allocator | `include/base/buffer_pool.h` | `src/base/buffer_pool.cpp` | Fixed-size buffer reuse |
| Adaptive Connection Pool | `include/base/resource_pool_manager.h` | `src/base/resource_pool_manager.cpp` | Bounded connection management |
| Work-Stealing Thread Pool | `include/execution/thread_pool_manager.h` | `src/execution/thread_pool_manager.cpp` | Low-latency task execution |

---

## Component Design

### BufferPool — Slab Allocator

**Design constraint:** avoid heap fragmentation for hot I/O paths.

Six slab size classes (bytes): 128, 256, 512, 1024, 2048, 4096.  
Oversize requests fall through to OS allocation and are returned as `SlabClass::kOversized` handles.

```
acquire(n bytes)
  → pick SlabClass by size
  → try free-list (lock-guarded per slab)
  → on miss: OS malloc, mark as fresh
  → return RAII BufferHandle (auto-releases on destruction)
```

**Statistics** (`BufferPool::Stats`): total_acquired, total_released, current_in_use, free_list_hits, os_fallbacks, pre_allocated — all updated under per-slab lock.

**RAII contract:** `BufferHandle` holds a raw pointer + slab class enum + back-reference to the owning `BufferPool`. Destructor calls `release()`. Handles are move-only.

**Pre-allocation:** `Config::preallocate_count` (default 32) slots per slab are allocated at construction time to warm the free lists.

### AdaptiveConnectionPool

Models connection slots as monotonically-incrementing integer tokens (production implementations wrap real sockets). Manages min/max/step:

```
acquire(timeout_ms)
  → if slot available: return slot_id
  → if at max: wait (cv) up to timeout
  → if scale window crossed: trigger forceScaleUp()
  → on timeout: return -1 (EAGAIN)
```

Scale-up rule: avg_wait_ms over last `Config::scale_window` acquisitions exceeds `Config::scale_up_threshold_ms`.  
Scale-down rule: utilization < `Config::idle_shrink_pct` (default 20%) for `Config::idle_shrink_periods` consecutive `release()` calls.

### ResourcePoolManager

Unified orchestrator: wraps one `AdaptiveConnectionPool` and one `BufferPool`. Exposes `acquireConnection()`, `acquireBuffer()`, and aggregate `getStatus()` (connection utilization + buffer stats). Not movable after construction.

### WorkStealingThreadPool

EDF-adjacent: workers try to find work from multiple sources in order:

1. Central dispatch queue (protected by `dispatch_mutex_`)
2. Peer threads' per-thread deques (work stealing via `ThreadQueue::trySteal()`)
3. Sleep on `dispatch_cv_` if nothing found

**Backpressure:** `submit()` blocks (up to `Config::submit_timeout_ms`) if the dispatch queue reaches `Config::max_queue_depth` (default 4096). Returns false on timeout.

**Latency histogram:** each worker records its dequeue → completion round-trip time in microseconds into a lock-free histogram for monitoring.

**Graceful shutdown:** `shutdown()` drains inflight tasks, then joins workers. `forceShutdown()` cancels pending and joins immediately.

---

## Data Flow

```
[Client code]
    │
    ├─ acquireBuffer(n)  →  BufferPool → free-list or OS malloc
    │                             ↓
    │                     BufferHandle (RAII)
    │
    ├─ acquireConnection() →  AdaptiveConnectionPool → slot_id
    │                                   ↑
    │                           adaptive scaling
    │
    └─ submit(task)       →  WorkStealingThreadPool
                                  ↓
                          dispatch_queue → worker deques → steal
```

---

## Configuration Reference

### BufferPool::Config
| Field | Default | Description |
|---|---|---|
| preallocate_count | 32 | Free-list warm-up per slab |
| max_pool_size | 10000 | Upper cap on pooled buffers |

### AdaptiveConnectionPool::Config
| Field | Default | Description |
|---|---|---|
| min_connections | 5 | Minimum always-live slots |
| max_connections | 50 | Hard upper bound |
| scale_step | 5 | Slots to add per scale-up event |
| scale_up_threshold_ms | 10 | Avg wait threshold to trigger scale-up |
| scale_window | 5 | Acquisition window for avg wait |
| acquire_timeout_ms | 5000 | Max wait for a free slot |
| idle_shrink_pct | 20 | Utilization % below which scale-down triggers |
| idle_shrink_periods | 3 | Required consecutive idle releases |

### WorkStealingThreadPool::Config
| Field | Default | Description |
|---|---|---|
| num_threads | hardware_concurrency() | Worker thread count |
| max_queue_depth | 4096 | Dispatch queue depth cap (backpressure) |
| submit_timeout_ms | 100 | submit() max wait before backpressure return |

---

## Test Coverage

Integration test: `tests/integration/test_resource_pooling.cpp`  
36 test cases, CTest labels: `integration;phase3;p3-03;resource_pooling`

Categories:
- **P3-03-A** BufferPool (9): basic alloc/free, RAII, slab class selection, oversize, double-free safety, concurrent stress, stats tracking, pre-alloc count, shutdown behavior
- **P3-03-B** AdaptiveConnectionPool (9): min/max enforcement, timeout return, scale-up trigger, scale-down trigger, concurrent contention, utilization tracking, config validation
- **P3-03-C** WorkStealingThreadPool (9): submit/execute, backpressure, work stealing, latency tracking, graceful shutdown, force shutdown, worker count, queue depth
- **P3-03-D** ResourcePoolManager (9): orchestration, unified status, connection + buffer independence, shutdown coordination

---

## Known Limitations

- Connection slots are integer tokens; wrap real connection type for production socket pools.
- Scale-down uses a release-count heuristic; production deployments should back this with real utilization telemetry.
- Work-stealing steal target is round-robin offset; randomized target reduces contention under skewed workloads.

---

## See Also

- `docs/architecture/QUERY_SCHEDULING.md` — P3-04 query scheduling design
- `ai_working/PHASE3_OPTIMIZATION_DETAILED_PLAN.md` — acceptance criteria
- `src/distributed_tensor/ROADMAP.md` — parent roadmap
