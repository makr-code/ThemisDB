---
Author: "ThemisDB Team"
Created: "2026-09-21"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Execution Module Roadmap

## Current Status

The execution module has a bounded deadline scheduler and a bounded fixed-worker
pool. No implementation drift was reported by the issue. The seven governance
documents are present and their stale status claims have been corrected. The
checked-in Doxygen XML is stale relative to the current headers, and no
configured test build is available in this checkout; validation and maintainer
acceptance evidence remain pending. The next implementation work is true work
stealing, elastic worker scaling, and other explicitly planned features.

- [x] Live implementation drift in `src/execution/` and `include/execution/` reviewed against current source and tests (Target: Q3 2026)
- [x] Governance documentation files present and aligned to current behavior (Target: Q4 2026)
- [ ] Regenerate and validate Doxygen XML against the current public headers (Target: Q4 2026)
- [ ] Elastic worker scaling and true steal-path activation implemented (Target: Q1 2027)
- [ ] Request-path server integration advanced beyond construction/teardown wiring (Target: Q1 2027)

## Planned Features

- [x] Restore missing governance docs: `AUDIT.md`, `CHANGELOG.md`, `FUTURE_ENHANCEMENTS.md`, `MODULE_GAPS.md`, `PERFORMANCE_EXPECTATIONS.md`, `PRODUCTION_REQUIREMENTS.md`, `SECURITY.md` (Target: Q3 2026)
- [x] Replace stale module claims about urgent buckets, fairness reservations, dynamic thread retirement, and live work stealing with source-verified descriptions (Target: Q3 2026)
- [x] Public-header comments describe the current implementation and reserved configuration fields (Target: Q3 2026)
- [ ] Activate per-worker queue population and actual cross-worker steal behavior (Target: Q1 2027)
- [ ] Implement elastic worker growth/shrink semantics or narrow the public naming/contract accordingly (Target: Q1 2027)
- [ ] Add queue-expiry / cancellation policy for stale scheduler entries where required by upstream consumers (Target: Q1 2027)
- [ ] Add structured observability/export hooks for scheduler and thread-pool metrics (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] Freeze `QueryScheduler` admission/dequeue/result-reporting API in `include/execution/query_scheduler.h` (Target: Q3 2026)
- [x] Freeze `WorkStealingThreadPool` submission/statistics/shutdown API in `include/execution/thread_pool_manager.h` (Target: Q3 2026)
- [x] Document reserved-but-currently-unused configuration fields (`urgent_window_ms`, `default_sla_ms`) and runtime-semantics gaps for `max_threads` / `idle_timeout_ms` (Target: Q3 2026)

### Phase 2: Core Implementation
- [x] Implement deadline-ordered admission queue with timeout-based backpressure and low-priority shedding in `src/execution/query_scheduler.cpp` (Target: Q3 2026)
- [x] Implement bounded central-dispatch worker pool with graceful shutdown and exception accounting in `src/execution/thread_pool_manager.cpp` (Target: Q3 2026)
- [ ] Activate true work-stealing dispatch from reserved per-worker queues (Target: Q1 2027)

### Phase 3: Error Handling & Edge Cases
- [x] Reject scheduler and thread-pool submissions after shutdown begins (Target: Q3 2026)
- [x] Return explicit failure (`0` / `false`) on queue-capacity timeout and low-priority shedding (Target: Q3 2026)
- [ ] Decide and implement stale-query eviction/cancellation behavior instead of retaining expired entries until dequeue (Target: Q1 2027)

### Phase 4: Tests
- [x] Keep scheduler behavior and metrics coverage in `tests/execution/` (Target: Q3 2026)
- [x] Keep thread-pool behavior coverage in `tests/integration/test_resource_pooling.cpp` (Target: Q3 2026)
- [x] Retain the high-cardinality local-model simulation with its non-production scope explicit (Target: Q4 2026)
- [ ] Add production-runtime capacity stress tests for `QueryScheduler` and `WorkStealingThreadPool` (Target: Q1 2027)
- [ ] Add focused tests for any future true-steal and elastic-scaling implementation before enabling those behaviors (Target: Q1 2027)

### Phase 5: Performance/Hardening
- [x] Maintain dedicated execution benchmark coverage in `benchmarks/execution/bench_execution_dedicated_gates.cpp` (Target: Q3 2026)
- [ ] Establish production-runtime queue and dispatch baselines on representative release hardware; current benchmark artifacts use standalone queue models (Target: Q1 2027)
- [ ] Add benchmark-backed guardrails for any future elastic-scaling or cancellation behavior (Target: Q1 2027)

### Phase 6: Documentation & Acceptance
- [x] Keep the full governance document set for the execution module aligned with source (Target: Q4 2026)
- [x] Align README, architecture, roadmap, and feature narratives to the live implementation (Target: Q4 2026)
- [ ] Regenerate Doxygen XML and resolve any confirmed public-API findings (Target: Q4 2026)
- [ ] Attach a successful focused CTest run and maintainer sign-off for the module-specific acceptance gate (Target: Q4 2026)

## Production Readiness Checklist

- [x] Public API declarations exist for scheduler and thread pool
- [x] Core implementation exists for bounded scheduler and fixed worker pool
- [x] Focused regression coverage exists for scheduler/thread-pool paths
- [x] A dedicated execution benchmark harness exists; it does not measure the production scheduler or pool
- [x] Governance documentation set restored and aligned
- [x] Public headers document current implementation behavior
- [ ] Current Doxygen XML evidence validated against the public headers
- [ ] Focused CTest validation evidence attached; current checkout has no configured test build
- [ ] Maintainer acceptance completed for the current documentation issue

## Known Issues & Limitations

- The scheduler orders by computed deadline; `SLAPriority` is not an independent bucket scheduler.
- `urgent_window_ms` and `default_sla_ms` are compatibility fields only in the current implementation.
- The thread pool does not currently push work into per-thread deques or perform active stealing.
- `max_threads` and `idle_timeout_ms` do not yet provide elastic scaling behavior.
- No built-in cancellation, stale-query eviction, or external metrics exporter exists.

## Breaking Changes

None currently planned. Any future narrowing of the thread-pool contract or removal of unused config fields must be announced here before release.
