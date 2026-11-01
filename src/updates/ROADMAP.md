---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-11-01"
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

### Current source state (2026-11-01)

- Public interfaces, implementation sources, focused tests, and benchmark
  sources are present under `include/updates/`, `src/updates/`,
  `tests/updates/`, and `benchmarks/updates/`.
- `tests/updates/CMakeLists.txt` registers `test_updates_*.cpp` focused targets
  (with the diagnostics source excluded from the prefix glob; see test
  registration and module test sources for the actual target inventory).
- `benchmarks/updates/CMakeLists.txt` registers five benchmark sources when
  `THEMIS_BUILD_BENCHMARKS` is enabled. The cluster-stress benchmark is
  separately registered by `benchmarks/CMakeLists.txt`, with a conditional
  CTest smoke entry.
- Diagnostic CMake configuration succeeded and exposed the focused updates
  test targets. The focused build did not complete, so no passing test or
  benchmark run is claimed. Source/test/benchmark presence does not establish
  passing CI, representative hardware results, or an approved release gate.

## Planned Features

All items below are evidence or documentation tasks; no runtime implementation or
release readiness is implied. Target timing requires maintainer approval.

- [x] Record the configured focused test targets and CTest names from `tests/updates/CMakeLists.txt` (Target: 2026-11-01).
- [ ] Complete the focused updates test build and record the CTest results (Target: next release-evidence cycle).
- [ ] Define reviewed benchmark baselines and acceptance thresholds in `PERFORMANCE_EXPECTATIONS.md` (Target: next release-evidence cycle).
- [x] Confirm `bench_updates_cluster_stress_q4` and its CTest smoke entry are registered by the repository-level benchmark CMake file (Target: 2026-11-01).
- [ ] Reconcile historical Phase/GA sign-offs with the root maturity aggregate (Target: next roadmap review).

## Implementation Phases

### Phase 1 — Design / API contract
- [ ] Confirm the reviewed public lifecycle, patch, migration, rollout, and diagnostics contracts against `include/updates/` and focused test sources (Target: next release-evidence cycle).

### Phase 2 — Core implementation
- [x] Confirm the cluster-stress benchmark target and CTest smoke entry are registered at repository level; avoid duplicate registration in the module CMake file (Target: 2026-11-01).

### Phase 3 — Error handling and edge cases
- [ ] Map failure and rollback cases to focused tests and record any uncovered public contract path (Target: next release-evidence cycle).

### Phase 4 — Tests
- [x] Capture the configured updates test target and CTest inventory (Target: 2026-11-01).
- [ ] Build and execute the focused updates tests; record exact results (Target: next release-evidence cycle).

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
- The cluster-stress target is registered at repository level, but no successful benchmark run is established here.
- Throughput values in the benchmark source are candidate expectations only; approved thresholds and representative-hardware results remain open.
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
