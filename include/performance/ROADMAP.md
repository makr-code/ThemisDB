> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/performance/ROADMAP.md -->

# Performance Module — Public Header Roadmap

**Module Path:** `include/performance/`
**Canonical implementation roadmap:** [`../../src/performance/ROADMAP.md`](../../src/performance/ROADMAP.md)

---

## Overview

Tracks public performance API contract stability, optimization/header coverage, and planned public entry points. Runtime implementation work remains in:

→ [`../../src/performance/ROADMAP.md`](../../src/performance/ROADMAP.md)

---

## Current Status

All 43 performance headers are present and cover measurement, adaptive optimization, cache/NUMA tuning, lock-free metrics primitives, accelerator dispatch, and phase-3/phase-4 advanced optimization surfaces.

---

## Completed ✅

- [x] measurement/runtime-control headers — `cycle_metrics.h`, `runtime_config.h`, `feature_flags.h`, `phase2_feature_flags.h`
- [x] adaptive optimization headers — `adaptive_query_compiler.h`, `workload_predictor.h`, `workload_adaptive_optimizer.h`, `phase3/adaptive_batch_tuner.h`, `phase3/bao.h`
- [x] memory/cache/concurrency headers — `advanced_cache_manager.h`, `numa_memory_manager.h`, `rcu.h`, `lockfree_metrics_buffer.h`
- [x] advanced engine headers — `hardware_accelerator.h`, `wisckey.h`, `dostoevsky.h`, `cicada.h`, `phase3/*`, `phase4/*`

---

## In Progress

- [ ] Clarify stability expectations for experimental phase-3/phase-4 headers versus core performance contracts (Target: 2026-Q3)
- [ ] Add stronger unsupported-hardware and degraded-feature guidance to accelerator-facing headers (Target: 2026-Q3)

---

## Planned

- [ ] `performance_capability_profile.h` — shared runtime capability summary for tuning decisions (Target: 2026-Q4)
- [ ] `optimizer_incident.h` — common incident/diagnostic DTO for fallback and tuning failures (Target: 2026-Q4)
- [ ] Document benchmark-backed compatibility notes for core measurement and adaptive hot paths (Target: 2026-Q4)

---

## Breaking Change History

None in v1.x. Public performance headers must remain backward compatible within the active major line; contract changes require migration notes and changelog updates.
