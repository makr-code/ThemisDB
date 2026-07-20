# Query Scheduling & Load Balancing Architecture — P3-04

**Version:** 1.0.0  
**Status:** Implemented  
**Phase:** Phase 3 — Block B  
**Maturity Score:** 7/10

---

## Overview

P3-04 delivers two coordinated scheduling components:

| Component | Header | Source | Purpose |
|---|---|---|---|
| ShardLoadBalancer | `include/sharding/shard_load_balancer.h` | `src/sharding/shard_load_balancer.cpp` | Route queries to lowest-cost shard |
| QueryScheduler | `include/execution/query_scheduler.h` | `src/execution/query_scheduler.cpp` | EDF queue with SLA priority + load shedding |

---

## Component Design

### ShardLoadBalancer

Selects the optimal shard for each incoming query using a weighted score formula:

```
score(shard) = cpu_weight * cpu_pct
             + queue_weight * (pending / max_pending * 100)
             + latency_weight * (latency_p99_ms / max_p99_ms * 100)
```

Default weights: cpu=0.30, queue=0.40, latency=0.30 (sum = 1.0).

**Selection algorithm:**
1. Lock shard map.
2. Compute score for all available shards.
3. Return shard with minimum score (ties: insertion order wins, deterministic).
4. Respect sticky sessions if `client_id` hash maps to a shard and score ≤ `Config::sticky_threshold`.

**Sticky sessions:** client_id → shard index via `hash(client_id) % n`. Abandoned if the stickied shard's score exceeds `Config::sticky_threshold` (default 80.0), falling back to lowest-score selection.

**Failure model:**
- `setAvailable(id, false)` excludes shard from selection.
- Failover: if all shards are unavailable, `selectShard()` throws `std::runtime_error`.

**Latency update:** `reportCompletion(shard_id, latency_ms)` updates `p99_latency_ms` via Exponential Moving Average (EMA) with smoothing factor 0.1. CPU and queue stats updated via direct assignment from caller (`updateMetrics()`).

### QueryScheduler

Priority-queue-based dispatcher using **Earliest Deadline First (EDF)**:

```
enqueue(priority, deadline_ms)
  → if queue >= max_capacity: block up to enqueue_timeout_ms
  → if priority == LOW && queue >= shed_threshold: shed (return 0 immediately)
  → push QueryEntry {id, deadline, priority, enqueue_time} into max-heap (min deadline)
  → notify one dequeue waiter

dequeue(out, timeout)
  → pop top from priority_queue (EarliestDeadlineFirst comparator)
  → record dequeue latency
  → return true/false
```

**SLA priority:** `SLAPriority` enum: LOW, MEDIUM, HIGH, CRITICAL. Load shedding applies only to LOW priority queries. MEDIUM/HIGH/CRITICAL always enqueue (subject to capacity limit + backpressure).

**Deadline comparator:** EarliestDeadlineFirst orders by ascending `deadline_ms`. Ties broken by ascending `id` (FIFO within the same deadline window).

**SLA compliance tracking:**
- `reportCompletion(id, actual_latency_ms)` looks up the query's registered deadline in `pending_deadlines_`.
- Increments `sla_met_count_` if `actual_latency_ms ≤ deadline_ms`, else `sla_violated_count_`.
- `slaComplianceRate()` returns met / (met + violated).

**Backpressure vs load shedding:**
| Condition | Action |
|---|---|
| queue_size < max_capacity | Immediate enqueue |
| max_capacity ≤ queue_size < shed_threshold | Block up to `enqueue_timeout_ms` |
| queue_size ≥ shed_threshold AND priority == LOW | Shed immediately (return 0) |
| queue_size ≥ max_capacity AND timeout expires | Return -1 (EAGAIN) |

---

## Data Flow

```
[Query Client]
    │
    ├─ selectShard(client_id)   →  ShardLoadBalancer
    │                                   │
    │                         score all healthy shards
    │                                   ↓
    │                          lowest-score shard_id
    │
    └─ enqueue(priority, deadline_ms)  →  QueryScheduler
                                               │
                                     EDF priority_queue
                                               │
                                     [Worker] dequeue()
                                               │
                                     execute on target shard
                                               │
                              reportCompletion(id, latency_ms)
                                               ↓
                                     SLA compliance tracking
```

---

## Configuration Reference

### ShardLoadBalancer::Config
| Field | Default | Description |
|---|---|---|
| weights.cpu | 0.30 | CPU utilization weight |
| weights.queue | 0.40 | Queue depth weight |
| weights.latency | 0.30 | Latency p99 weight |
| max_pending | 1000 | Queue depth normalization cap |
| max_latency_us | 100000 | Latency normalization cap (µs) |
| sticky_threshold | 80.0 | Score above which stickiness is abandoned |
| ema_alpha | 0.1 | Latency EMA smoothing factor |

### QueryScheduler::Config
| Field | Default | Description |
|---|---|---|
| max_capacity | 10000 | Hard queue capacity cap |
| shed_threshold | 8000 | Low-priority shed threshold |
| enqueue_timeout_ms | 500 | Max wait when queue is at max_capacity |
| urgent_window_ms | 50 | Window used for near-deadline classification |

---

## Test Coverage

Integration test: `tests/integration/test_load_balancing.cpp`  
24 test cases, CTest labels: `integration;phase3;p3-04;load_balancing;scheduling`

Categories:
- **P3-04-A** ShardLoadBalancer basics (6): single shard selection, weighted scoring, healthy/unhealthy exclusion, sticky session routing, sticky failover, empty shard list
- **P3-04-B** ShardLoadBalancer advanced (6): EMA latency update, score formula, multi-shard contention, concurrent routing, reporting round-trip, dynamic shard add
- **P3-04-C** QueryScheduler basics (6): enqueue/dequeue round-trip, EDF ordering, load shedding, SLA compliance rate, shutdown drain, capacity backpressure
- **P3-04-D** QueryScheduler advanced (6): concurrent producers/consumers, deadline miss detection, priority tier ordering, latency stats, CRITICAL never shed, compliance accumulation

---

## Known Limitations

- Score formula uses linear normalization; production deployments should tune `max_pending`/`max_latency_us` to match observed working ranges.
- Load shedding sheds the incoming query on overflow; an alternative design is eviction of the lowest-priority enqueued item.
- SLA tracking uses in-memory `pending_deadlines_` map; long-running systems need periodic purging of stale entries.

---

## See Also

- `docs/architecture/RESOURCE_POOLING.md` — P3-03 resource pool design
- `ai_working/PHASE3_OPTIMIZATION_DETAILED_PLAN.md` — acceptance criteria
- `include/sharding/shard_load_balancer.h` — ShardLoadBalancer API
- `include/execution/query_scheduler.h` — QueryScheduler API
