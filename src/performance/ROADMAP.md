---
Author: "ThemisDB Team"
Created: "2026-05-31"
Last Updated: "2026-10-31"
Status: "active"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_Sources:
  - "src/performance/*.cpp"
  - "include/performance/*.h"
  - "tests/performance/CMakeLists.txt"
  - "benchmarks/performance/CMakeLists.txt"
Related_Issue: "#7366"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Performance Module Roadmap

## Current source state

The source tree contains measurement/export components, workload and query-cost
feedback, cache/memory and hardware interfaces, and Phase 2–4 components. The
presence of code or tests is not by itself evidence of production readiness,
passing tests, or measured performance.

Evidence currently present in source:

- `performance_api_contract.h` declares the `PerfError` enum and shared result
  types `PoolAcquireResult`, `CacheStats`, and `CostEstimate`.
- `test_performance_contract_hardening_focused.cpp` contains test IDs
  `PFM-01` through `PFM-16`.
- `bench_performance_release_gates.cpp` defines six named microbenchmark cases
  (`GATE-PFM-01` through `GATE-PFM-06`) with repetitions and deterministic
  random seeds. The source does not encode acceptance thresholds or report
  baseline pass/fail status.

## Build and coverage gaps

- [ ] Register or explicitly exclude the 12 phase3/phase4 test sources. The
  current `tests/performance/CMakeLists.txt` globs only root-level
  `  test_*.cpp`; it does not add nested test files. (Target: DOC-WEEKLY-2026-44)
- [ ] Decide whether root-level `test_performance_*.cpp` files should be
  registered twice: the current module CMake creates both `_focused` and
  `_autofocused` targets for those sources. (Target: DOC-WEEKLY-2026-44)
- [ ] Document a reproducible release baseline and explicit benchmark
  acceptance thresholds if they are intended to be enforced. The current
  performance benchmark CMake registration includes only
  `bench_performance_release_gates`; it does not include
  `bench_random_access_prefetch.cpp`. (Target: DOC-WEEKLY-2026-44)
- [ ] Expand representative workload and sustained workload-shift evaluation
  before making broad module-wide performance claims. (Target: Q1 2027)

## Planned work

- [ ] Refresh p95/p99 baselines on representative hardware and record the
  workload, build flags, and run manifest. (Target: Q1 2027)
- [ ] Broaden mixed and distributed performance scenario coverage where the
  owning benchmark modules provide canonical workloads. (Target: Q1 2027)
- [ ] Continue long-duration reliability evaluation for adaptive workload
  shifts. (Target: Q1 2027)
- [ ] Evaluate distributed tracing and NUMA-aware adaptive memory-pool resizing
  only with an implementation proposal and measurable acceptance criteria.
  (Target: Q2 2027)

## Verification status

This roadmap was checked against current headers, implementations, and the
module-local test/benchmark CMake files. No tests or benchmarks were executed
for this documentation update; passing status and measured thresholds remain
unverified. Historical milestone completion claims should be checked against
their original run evidence before being used as current acceptance evidence.

Exact model backend not verifiable from final document text alone.
