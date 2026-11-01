---
Author: ThemisDB Team
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
Source level: 1
SOT domain: module-behavior
Canonical references: benchmarks/scheduler/CMakeLists.txt; benchmarks/scheduler/*.cpp; src/scheduler/PERFORMANCE_EXPECTATIONS.md
Related issue: #7376; DOC-WEEKLY-2026-44
---

Exact model backend not verifiable from final document text alone.

# Scheduler benchmark sources

The local CMake file discovers `bench_*.cpp` when
`THEMIS_BUILD_BENCHMARKS` is enabled. Current sources:

- `bench_task_scheduler.cpp` — calls production `TaskScheduler` APIs.
- `bench_scheduler_dedicated_gates.cpp` — measures a local stub scheduler.
- `bench_scheduler_release_gates.cpp` — measures enum operations and a local
  descriptor, not scheduler lifecycle behavior.
- `bench_thread_pool_saturation.cpp` — measures generic `ThreadPoolManager`
  behavior.

See [`../../src/scheduler/PERFORMANCE_EXPECTATIONS.md`](../../src/scheduler/PERFORMANCE_EXPECTATIONS.md)
for the case mapping and limits on interpreting benchmark names/labels.
