---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Updates Module Roadmap

## Current Status

This roadmap tracks source-visible work and evidence gaps. It is not a
production-readiness approval. The root maturity aggregate currently classifies
`updates` as **HARDENING** in [`../../ROADMAP.md`](../../ROADMAP.md); that
aggregate takes precedence over older module-local GA/readiness statements.

### Current source state (2026-10-31)

- Public interfaces, implementation sources, focused tests, and benchmark
  sources are present under `include/updates/`, `src/updates/`,
  `tests/updates/`, and `benchmarks/updates/`.
- `tests/updates/CMakeLists.txt` registers `test_updates_*.cpp` focused targets
  (with the diagnostics source excluded from the prefix glob; see test
  registration and module test sources for the actual target inventory).
- `benchmarks/updates/CMakeLists.txt` registers five benchmark sources when
  `THEMIS_BUILD_BENCHMARKS` is enabled. The cluster-stress benchmark source is
  present but not registered.
- Source/test/benchmark presence does not establish passing CI, representative
  hardware results, or a release gate. **TODO:** attach current CI and benchmark
  run evidence before changing the module maturity claim.

## Planned Features

All items below are evidence or documentation tasks; no runtime implementation or
release readiness is implied. Target timing requires maintainer approval.

- [ ] Record the configured test targets, CTest names, and latest run evidence from `tests/updates/CMakeLists.txt` (Target: next release-evidence cycle).
- [ ] Define reviewed benchmark baselines and acceptance thresholds in `PERFORMANCE_EXPECTATIONS.md` (Target: next release-evidence cycle).
- [ ] Register or explicitly retire `bench_updates_cluster_stress_q4.cpp` after owner review (Target: next benchmark review).
- [ ] Reconcile historical Phase/GA sign-offs with the root maturity aggregate (Target: next roadmap review).

## Implementation Phases

### Phase 1 — Design / API contract
- [ ] Confirm the reviewed public lifecycle, patch, migration, rollout, and diagnostics contracts against `include/updates/` and focused test sources (Target: next release-evidence cycle).

### Phase 2 — Core implementation
- [ ] Decide whether the unregistered cluster-stress benchmark is intended to ship; register it only after benchmark-owner approval (Target: next benchmark review).

### Phase 3 — Error handling and edge cases
- [ ] Map failure and rollback cases to focused tests and record any uncovered public contract path (Target: next release-evidence cycle).

### Phase 4 — Tests
- [ ] Capture the configured update test target and CTest inventory, then record the exact test run result (Target: next release-evidence cycle).

### Phase 5 — Performance and hardening
- [ ] Establish reviewed benchmark baselines, thresholds, and a reproducible run manifest (Target: next benchmark review).

### Phase 6 — Documentation and acceptance
- [ ] Reconcile historical module evidence and readiness assertions with current root governance and verified run artifacts (Target: next roadmap review).

## Production Readiness Checklist

- [ ] Current focused test results recorded for a named build configuration.
- [ ] Representative benchmark results and approved thresholds recorded.
- [ ] Operational and deployment-specific controls verified by their owners.
- [ ] Root maturity aggregate and module-local readiness statements reconciled.

## Known Issues & Limitations

- Test and benchmark source presence does not establish passing runs.
- `bench_updates_cluster_stress_q4.cpp` is present but is not registered by the benchmark CMake file.
- Current performance thresholds and representative-hardware results are not established by this documentation audit.
- Deployment-wide authentication, authorization, topology, and recovery policy are outside the module's standalone C++ contract.

## Compatibility and change control

Public API compatibility follows the repository versioning policy. A public
contract change should update the applicable header comments, `API.md`, focused
tests, and the relevant operational documentation. Do not infer a future
breaking-change plan from this roadmap.

## Breaking Changes

No public API change was made or proposed in this documentation update. A future
breaking change must follow the repository versioning policy and be documented
in the affected header and API contract.

## Historical status notes

Older sections in prior copies of this file described v1.1.0/v2.4.0 readiness,
UPDP gate passes, and future phase assignments. Their run evidence and approval
state could not be verified against the current CMake registration and current
release evidence during this documentation audit; those claims are not active
status.
