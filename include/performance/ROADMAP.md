---
Author: "ThemisDB Team"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "active"
Source_Level: "level1"
SOT_Domain: "api-contract"
Canonical_Sources:
  - "include/performance/*.h"
  - "src/performance/ROADMAP.md"
Related_Issue: "#7366"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Performance Module — Public Header Roadmap

**Module Path:** `include/performance/`
**Canonical implementation roadmap:** [`../../src/performance/ROADMAP.md`](../../src/performance/ROADMAP.md)

---

## Overview

Tracks public performance API contract stability, optimization/header coverage, and planned public entry points. Runtime implementation work remains in:

→ [`../../src/performance/ROADMAP.md`](../../src/performance/ROADMAP.md)

---

## Current Status

The current header tree contains 44 public headers covering measurement,
adaptive optimization, cache/NUMA tuning, concurrency primitives, accelerator
dispatch, and phase-specific components. This inventory is a point-in-time
count, not a stability or implementation-maturity claim.

---

## Completed ✅

- [x] measurement/runtime-control headers — `cycle_metrics.h`, `runtime_config.h`, `feature_flags.h`, `phase2_feature_flags.h`
- [x] adaptive optimization headers — `adaptive_query_compiler.h`, `workload_predictor.h`, `workload_adaptive_optimizer.h`, `phase3/adaptive_batch_tuner.h`, `phase3/bao.h`
- [x] memory/cache/concurrency headers — `advanced_cache_manager.h`, `numa_memory_manager.h`, `rcu.h`, `lockfree_metrics_buffer.h`
- [x] advanced engine headers — `hardware_accelerator.h`, `wisckey.h`, `dostoevsky.h`, `cicada.h`, `phase3/*`, `phase4/*`

---

## In Progress

- [ ] Clarify stability expectations for phase-3/phase-4 headers versus shared performance contracts; no common stability marker was verified across these headers (Target: 2026-Q4)
- [ ] Keep unsupported-hardware and degraded-feature behavior explicit in accelerator-facing declarations; verify each path against implementation and host capability (Target: 2026-Q4)

---

## Planned

- [ ] Consider `performance_capability_profile.h` — proposed shared runtime capability summary; file not present in the current include tree (Target: 2026-Q4)
- [ ] Consider `optimizer_incident.h` — proposed diagnostic type; file not present in the current include tree (Target: 2026-Q4)
- [ ] Document benchmark-backed compatibility notes for core measurement and adaptive hot paths (Target: 2026-Q4)

---

## Breaking Change History

No breaking-change history is recorded in this header roadmap. The active
contract statement and compatibility scope should be read from
[`performance_api_contract.h`](performance_api_contract.h) and the project
release/versioning documents.

Exact model backend not verifiable from final document text alone.
