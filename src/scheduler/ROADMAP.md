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
Canonical references: include/scheduler/*.h; src/scheduler/*.cpp; tests/scheduler/CMakeLists.txt; benchmarks/scheduler/CMakeLists.txt
Related issue: #7376; DOC-WEEKLY-2026-44
---

Exact model backend not verifiable from final document text alone.

# Scheduler module roadmap

This is a source-informed engineering follow-up list, not a release-readiness
statement. Completion claims require implementation evidence and tests that
exercise the production behavior.

## Current surfaces

The source contains task scheduling, coordination, external-adapter, retention,
changefeed-trigger, audit, result-store, and anomaly-detection components. Their
interfaces and implementation map is summarized in [README.md](README.md).

## Open work

- [x] Replace the `executeTaskNow` coordination placeholder with a shared,
  coordinator-owned execution gate checked by manual, scheduled, and DAG
  dispatch. The gate is false until healthy leadership is established and is
  closed on leadership loss, heartbeat failure, and coordinator shutdown.
- [ ] Replace skipped coordination failure scenarios and simulated
  burst/retention cases with tests that exercise the actual implementation.
- [x] Reject malformed, unknown-field, and unknown-operator CDC predicates;
  configuration validation and runtime evaluation both fail closed.
- [x] Deny security-sensitive scheduler operations when `RequestContext` is
  absent. HTTP routes and trusted coordinator/persistence paths now set scoped,
  operation-limited contexts.
- [ ] Define a numeric benchmark release workflow, statistic, threshold, and
  representative-hardware baseline. The production scheduler benchmark has a
  CTest smoke registration, but its timing is informational until an owner,
  stable hardware, baseline, and threshold are agreed.
- [ ] Obtain security/deployment-owner review for production requirements and
  configuration defaults; see [PRODUCTION_REQUIREMENTS.md](PRODUCTION_REQUIREMENTS.md).

## Evidence inventory

- Focused test CMake registration globs `tests/scheduler/test_scheduler_*.cpp`.
  The existing coordination-failure placeholder suite remains skipped; the
  production distributed coordinator integration tests cover the execution
  gate. Some stress tests still have simulation or placeholder logic. See
  [AUDIT.md](AUDIT.md).
- Benchmark CMake registration globs `benchmarks/scheduler/bench_*.cpp` when
  `THEMIS_BUILD_BENCHMARKS` is enabled. See
  [PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md) for what each
  benchmark measures.
- No assertion is made here that the open work above is scheduled for a
  particular release or milestone beyond this documentation refresh.

## Contract changes

Review changes to public declarations in `include/scheduler/` against
`include/scheduler/scheduler_api_contract.h` and the relevant implementation
before documenting a behavior as guaranteed. Keep historical release notes in
[CHANGELOG.md](CHANGELOG.md).
