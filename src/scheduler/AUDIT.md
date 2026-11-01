---
Author: ThemisDB Team
Created: 2026-10-31
Last Updated: 2026-11-01
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
Source level: 1
SOT domain: module-behavior
Canonical references: src/scheduler/*.cpp; include/scheduler/*.h; tests/scheduler/CMakeLists.txt; benchmarks/scheduler/CMakeLists.txt; cmake/CMakeLists.txt; cmake/ModularBuild.cmake
Related issue: #7376; DOC-WEEKLY-2026-44
---

Exact model backend not verifiable from final document text alone.

# Scheduler documentation/source audit

## Review scope and result

Targeted source inspection covered the scoped scheduler Markdown, relevant
public declarations and implementation paths, scheduler test/benchmark sources,
and their CMake registration.
The initial documentation review did not run build, lint, or tests. Subsequent
scheduler hardening added source-level behavior and regression coverage; focused
build/test execution remains blocked when required system dependencies are absent.
This is not a code-quality or security certification.

| Check | Finding |
|---|---|
| Implementation inventory | Ten `.cpp` files are present, including `task_execution_result.cpp`. |
| Focused test registration | `tests/scheduler/CMakeLists.txt` globs `test_scheduler_*.cpp`; six scoped test sources currently match. Registration does not prove execution or pass status. |
| Benchmark registration | `benchmarks/scheduler/CMakeLists.txt` globs `bench_*.cpp`; four scoped benchmark sources currently match when benchmark building is enabled. |
| CMake source inclusion | Scheduler sources are conditionally registered with `THEMIS_ENABLE_HTTP_SERVER`; `task_execution_result.cpp` is separately listed in the modular build. |
| Documentation locations | No Markdown files currently exist directly under `tests/scheduler/`. |

## Open source/documentation gaps

1. The coordination-failure placeholder fixture still calls `GTEST_SKIP()` in
   setup. A production `DistributedTaskCoordinator` test now verifies that
   immediate, scheduled, and DAG paths do not execute without healthy
   leadership, but the broader failover scenarios remain unimplemented.
2. The
   burst/retention test includes simulated or placeholder scenarios. These
   sources do not substantiate all the behavior their names/comments describe.
3. The dedicated “p95” benchmark cases use a local stub scheduler; the release
   gate benchmark measures enum operations and a local descriptor. No scoped
   CMake registration or benchmark source establishes a CI-enforced scheduler
   release threshold or hardware baseline.
4. CDC condition configuration and runtime evaluation now reject malformed
   predicates, unknown fields, and unsupported operators.
5. Permission and role helpers deny without a request context. HTTP and trusted
   in-process scheduler callers use scoped contexts with narrow permissions;
   deployments must still verify their identity-provider mappings.

## Disposition

The current docs reflect the implemented fail-closed deltas. Broader coordinator
failure testing and a benchmark owner/hardware/baseline/threshold agreement
remain open; timing is not a release gate.
