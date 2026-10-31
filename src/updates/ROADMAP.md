# Updates Module Roadmap

This roadmap tracks source-visible work and evidence gaps. It is not a
production-readiness approval. The root maturity aggregate currently classifies
`updates` as **HARDENING** in [`../../ROADMAP.md`](../../ROADMAP.md); that
aggregate takes precedence over older module-local GA/readiness statements.

## Current source state (2026-10-31)

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

## Work items

| Item | Source evidence | Status |
|---|---|---|
| Verify public lifecycle, patch, migration, rollout, and diagnostics contracts | Headers in `include/updates/`; focused suites in `tests/updates/` | Present; current execution results not verified here |
| Reconcile update-module test inventory and labels with current CMake | `tests/updates/CMakeLists.txt` | TODO: capture configured target/CTest inventory |
| Establish reviewed performance baselines and thresholds | `benchmarks/updates/` and `PERFORMANCE_EXPECTATIONS.md` | TODO: no current release thresholds established by registration |
| Register or explicitly retire the cluster-stress benchmark source | `benchmarks/updates/bench_updates_cluster_stress_q4.cpp` and `benchmarks/updates/CMakeLists.txt` | TODO |
| Reconcile older Phase/GA sign-offs and archived plans with root maturity | `CHANGELOG.md`, `MODULE_EVIDENCE.md`, `FINAL_IMPLEMENTATION_PLAN.md` | TODO: treat those entries as historical until current evidence is attached |

## Compatibility and change control

Public API compatibility follows the repository versioning policy. A public
contract change should update the applicable header comments, `API.md`, focused
tests, and the relevant operational documentation. Do not infer a future
breaking-change plan from this roadmap.

## Historical status notes

Older sections in prior copies of this file described v1.1.0/v2.4.0 readiness,
UPDP gate passes, and future phase assignments. Their run evidence and approval
state could not be verified against the current CMake registration and current
release evidence during this documentation audit; those claims are not active
status.
