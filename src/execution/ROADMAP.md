**Author:** ThemisDB Contributors  
**Created:** 2026-09-21  
**Last Updated:** 2026-09-21  
**Status:** active

# Execution Module Roadmap

## Current Status

The execution module has a working bounded scheduler and a working bounded fixed-size thread pool, plus focused tests and dedicated benchmarks. The remaining drift for this module is documentation and contract alignment: earlier docs described dynamic reprioritisation, active work stealing, and elastic scaling that are **not** present in the live implementation.

- [x] Live implementation drift in `src/execution/` and `include/execution/` reviewed against current source and tests (Target: Q3 2026)
- [~] Governance documentation restored and aligned to current behavior (Target: Q3 2026)
- [~] Source-level Doxygen refreshed to match the live public API contract (Target: Q3 2026)
- [ ] Elastic worker scaling and true steal-path activation implemented (Target: Q1 2027)
- [ ] Request-path server integration advanced beyond construction/teardown wiring (Target: Q1 2027)

## In Progress

- [~] Restore missing governance docs: `AUDIT.md`, `CHANGELOG.md`, `FUTURE_ENHANCEMENTS.md`, `MODULE_GAPS.md`, `PERFORMANCE_EXPECTATIONS.md`, `PRODUCTION_REQUIREMENTS.md`, `SECURITY.md` (Target: Q3 2026)
- [~] Replace stale module claims about urgent buckets, fairness reservations, dynamic thread retirement, and live work stealing with source-verified descriptions (Target: Q3 2026)
- [~] Refresh Doxygen comments on `include/execution/query_scheduler.h` and `include/execution/thread_pool_manager.h` so the public contract matches the implementation (Target: Q3 2026)

## Planned Features

- [ ] Activate per-worker queue population and actual cross-worker steal behavior (Target: Q1 2027)
- [ ] Implement elastic worker growth/shrink semantics or narrow the public naming/contract accordingly (Target: Q1 2027)
- [ ] Add queue-expiry / cancellation policy for stale scheduler entries where required by upstream consumers (Target: Q1 2027)
- [ ] Add structured observability/export hooks for scheduler and thread-pool metrics (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] Freeze `QueryScheduler` admission/dequeue/result-reporting API in `include/execution/query_scheduler.h` (Target: Q3 2026)
- [x] Freeze `WorkStealingThreadPool` submission/statistics/shutdown API in `include/execution/thread_pool_manager.h` (Target: Q3 2026)
- [~] Document reserved-but-currently-unused configuration fields (`urgent_window_ms`, `default_sla_ms`) and runtime-semantics gaps for `max_threads` / `idle_timeout_ms` (Target: Q3 2026)

### Phase 2: Core Implementation
- [x] Implement deadline-ordered admission queue with timeout-based backpressure and low-priority shedding in `src/execution/query_scheduler.cpp` (Target: Q3 2026)
- [x] Implement bounded central-dispatch worker pool with graceful shutdown and exception accounting in `src/execution/thread_pool_manager.cpp` (Target: Q3 2026)
- [ ] Activate true work-stealing dispatch from reserved per-worker queues (Target: Q1 2027)

### Phase 3: Error Handling & Edge Cases
- [x] Reject scheduler and thread-pool submissions after shutdown begins (Target: Q3 2026)
- [x] Return explicit failure (`0` / `false`) on queue-capacity timeout and low-priority shedding (Target: Q3 2026)
- [ ] Decide and implement stale-query eviction/cancellation behavior instead of retaining expired entries until dequeue (Target: Q1 2027)

### Phase 4: Tests
- [x] Keep focused scheduler behavior coverage in `tests/integration/test_load_balancing.cpp` (Target: Q3 2026)
- [x] Keep focused thread-pool coverage in `tests/integration/test_resource_pooling.cpp` (Target: Q3 2026)
- [x] Keep high-cardinality stress coverage in `tests/execution/test_execution_highcardinality_stress.cpp` (Target: Q3 2026)
- [ ] Add focused tests for any future true-steal and elastic-scaling implementation before enabling those behaviors (Target: Q1 2027)

### Phase 5: Performance/Hardening
- [x] Maintain dedicated execution benchmark coverage in `benchmarks/execution/bench_execution_dedicated_gates.cpp` (Target: Q3 2026)
- [~] Re-baseline queue and dispatch latency targets against representative release hardware (Target: Q4 2026)
- [ ] Add benchmark-backed guardrails for any future elastic-scaling or cancellation behavior (Target: Q1 2027)

### Phase 6: Documentation & Acceptance
- [~] Restore the full governance document set for the execution module (Target: Q3 2026)
- [~] Align README, architecture, roadmap, and Doxygen narratives to the live implementation (Target: Q3 2026)
- [ ] Attach focused validation evidence for the module-specific acceptance gate and maintainer sign-off (Target: Q3 2026)

## Production Readiness Checklist

- [x] Public API declarations exist for scheduler and thread pool
- [x] Core implementation exists for bounded scheduler and fixed worker pool
- [x] Focused regression coverage exists for scheduler/thread-pool paths
- [x] Dedicated execution benchmark coverage exists
- [~] Governance documentation set restored and aligned
- [~] Doxygen contract refreshed for current implementation behavior
- [ ] Maintainer validation evidence attached for the current documentation-restoration issue

## Known Issues & Limitations

- The scheduler orders by computed deadline; `SLAPriority` is not an independent bucket scheduler.
- `urgent_window_ms` and `default_sla_ms` are compatibility fields only in the current implementation.
- The thread pool does not currently push work into per-thread deques or perform active stealing.
- `max_threads` and `idle_timeout_ms` do not yet provide elastic scaling behavior.
- No built-in cancellation, stale-query eviction, or external metrics exporter exists.

## Breaking Changes

None currently planned. Any future narrowing of the thread-pool contract or removal of unused config fields must be announced here before release.
