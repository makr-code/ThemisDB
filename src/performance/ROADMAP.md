---
Author: "ThemisDB Team"
Created: "2026-05-31"
Last Updated: "2026-11-07"
Status: "active"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_Sources:
  - "include/performance/performance_api_contract.h"
  - "src/performance/IMPLEMENTATION.md"
  - "src/performance/PERFORMANCE_EXPECTATIONS.md"
  - "src/performance/RESEARCH.md"
  - "src/performance/OPERATIONS.md"
  - "src/performance/TRUST.md"
  - "src/performance/PRODUCTION_REQUIREMENTS.md"
  - "tests/performance/README.md"
  - "tests/performance/CMakeLists.txt"
  - "benchmarks/performance/README.md"
  - "benchmarks/performance/CMakeLists.txt"
  - "benchmarks/performance/bench_performance_release_gates.cpp"
  - "benchmarks/performance/bench_random_access_prefetch.cpp"
  - "ROADMAP.md"
Related_Issue: "#7439"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Performance Module Roadmap

## Current Status

The performance module has implemented measurement and export, workload
prediction and adaptation, cache and NUMA facilities, hardware dispatch, Phase 2
algorithms, Phase 3 feedback and vector-index components, and Phase 4 io_uring,
PMEM, and PMU components. The shared error/result vocabulary is declared in
[`performance_api_contract.h`](../../include/performance/performance_api_contract.h).
Implementation presence, test registration, and benchmark source are not proof
of passing tests, production readiness, or improved performance.

**Maturity assessment:** The module has broad implementation coverage but
remains evidence-incomplete for release-level performance and operational
readiness. Source-level architecture and capability boundaries are documented;
runtime quality and performance confidence are low until registered tests run
and representative, repeatable benchmark evidence exists.

Evidence and gaps at this update:

- `tests/performance/README.md` documents root tests and recursive Phase 3/4
  tests. An isolated CMake registration check found 31 entries, but did not
  compile or execute them. The root `test_performance_*.cpp` sources currently
  have both `_focused` and `_autofocused` registrations; whether to deduplicate
  remains unresolved.
- `benchmarks/performance/CMakeLists.txt` registers two executables.
  `bench_performance_release_gates.cpp` has six deterministic microbenchmark
  cases and five repetitions, but no threshold, baseline, or pass/fail decision.
  `bench_random_access_prefetch.cpp` measures paired prefetch on/off point,
  batch, and scan cases and other range/scaling cases; source comments are not
  acceptance criteria.
- The random-access benchmark disables WAL for faster setup. Before using those
  results for release claims, assess and document how that setup differs from
  production; it does not by itself represent production behavior.
- No performance-module baseline or defensible numerical regression threshold
  exists. A release claim requires repeated runs on declared representative,
  host-matched hardware, a complete run manifest, and an agreed decision rule.
  Keep p95/p99 refresh, mixed/distributed scenarios, and long-duration workload
  shifts open; none is established as complete here.
- Forecasts and cost calibration are estimates, not proof of improvement.
  Evaluate adaptive outcomes against a comparable control. Capability-dependent
  paths require build, runtime, operating-system, and host support; report
  explicit failure or fallback rather than claiming unavailable acceleration.
- Operational telemetry can reveal workload and resource-use patterns. Apply
  masking, bounded cardinality, access control, and retention controls at the
  relevant collection and service boundaries.

The root [`ROADMAP.md`](../../ROADMAP.md) is authoritative for project release
status and release promotion. This module roadmap plans work for v2.5.0 and
v3.0.0; it does not claim either release is approved or that the root roadmap's
project-wide gates are satisfied. Wave estimates below are planning estimates
in person-weeks, not commitments or measured durations.

## Planned Features

### v2.5.0 — correctness, baseline, and representative single-node evidence

v2.5.0 focuses on trustworthy measurement, contract and edge-case reliability,
and bounded single-node evaluation of workload adaptation. Its performance
decision is evidence-gathering, not a promised speedup.

#### Wave 1 — close test and measurement foundations

- **Target / priority:** v2.5.0; P0 release evidence prerequisite.
- **Goal:** Make the active test inventory and benchmark evidence reproducible
  before using either to assess a release.
- **Concrete tasks:** Resolve or explicitly document the duplicate
  `_focused`/`_autofocused` registration policy; record configured test
  discovery and compile/run outcomes for the registered performance tests;
  document benchmark host, revision, build flags, commands, workload, raw output,
  and WAL/setup differences in a run manifest; review whether the random-access
  setup is representative enough for each intended claim.
- **Dependencies:** Configured build with required test/benchmark dependencies;
  representative host and workload owner availability.
- **Effort:** Planning estimate: 2–3 person-weeks.
- **Risks / trade-offs:** Registration cleanup can alter test discovery; setup
  fidelity and host availability constrain representativeness. A registration
  result alone cannot close this wave.
- **KPI / acceptance:** Duplicate registration has a documented disposition;
  actual build and execution status is recorded for the intended test set; every
  claimed benchmark run has a complete manifest and states its setup limits.
  No numerical performance target is assumed.
- **Validation:** Configure, build, and run registered tests in a declared
  environment; inspect CTest discovery and retain logs. Run benchmarks only in
  a declared profile and review manifests. Check metric masking/cardinality and
  follow security and production requirements; registration-only evidence is
  not a pass.

#### Wave 2 — establish the single-node baseline and decision method

- **Target / priority:** v2.5.0; P0 for any performance acceptance claim.
- **Goal:** Create repeatable, host-matched baseline evidence for representative
  single-node workloads, including latency tails.
- **Concrete tasks:** Define workload shape, data distribution, dataset size,
  host, build configuration, feature flags, warm-up, sampling, and control path;
  collect repeated runs and p50/p95/p99 latency, throughput, resource use, and
  relevant result-quality measures; preserve raw results and revision/config
  manifest; characterize variability and derive a proposed threshold/decision
  rule from those runs with the owning release stakeholders. Refresh p95/p99
  evidence on the same declared host where possible.
- **Dependencies:** Wave 1; representative host access; an agreed workload and
  comparison control. No external paper supplies ThemisDB's threshold.
- **Effort:** Planning estimate: 3–5 person-weeks.
- **Risks / trade-offs:** Host noise, dataset mismatch, and generator behavior
  can invalidate comparisons; a threshold chosen before repeatability is
  understood creates false confidence. Do not select a speedup or latency
  number in advance without local evidence.
- **KPI / acceptance:** A reproducible host/workload manifest and repeated raw
  runs exist; variability is reported; release owners accept a documented
  baseline and decision rule, or explicitly leave the numerical gate unset
  pending more evidence. No claim is accepted from a single run.
- **Validation:** Compare repeated same-host runs with identical workload and
  build settings; verify latency sampling and generator behavior; retain raw
  output and record deviations. Review with benchmark owners and release
  governance before promoting any threshold.

#### Wave 3 — correctness, failure, and adaptive single-node evaluation

- **Target / priority:** v2.5.0; P1, after test inventory and baseline protocol.
- **Goal:** Establish correctness and bounded adaptive-policy behavior against a
  non-adaptive control under representative single-node workload changes.
- **Concrete tasks:** Exercise invalid bounds, resource exhaustion, stale state,
  missing statistics, exporter failures, and optional capability fallback;
  verify result/error distinctions at API boundaries; compare adaptive and
  control runs over representative workload phases and sustained workload
  shifts; record forecast confidence, selected strategy, observed outcome,
  latency tails, throughput, and resource overhead without interpreting a
  prediction as realized gain.
- **Dependencies:** Waves 1–2; workload/control definitions; feature and host
  capability inventory.
- **Effort:** Planning estimate: 3–5 person-weeks.
- **Risks / trade-offs:** Adaptation can oscillate or regress under distribution
  shifts; overhead and tail latency can outweigh average throughput. Keep
  optional strategies disabled or retain the safe path when evidence is
  inconclusive.
- **KPI / acceptance:** Relevant tests build and pass in the declared
  configuration; every exercised unavailable path yields explicit error or
  documented fallback; adaptive results are compared with a control across
  declared phases and duration, with no unsupported claim of improvement.
  Any numeric benefit/regression criterion must come from Wave 2 evidence and
  be approved before it is used.
- **Validation:** Focused correctness and fault-path tests; repeated paired
  adaptive/control benchmarks on the declared host; report tails and resource
  cost. Review telemetry masking, access and retention controls, resource
  limits, and fail-closed configuration requirements.

### v3.0.0 — broader scale and operational gates, contingent on evidence

v3.0.0 work starts only after v2.5.0 evidence is reviewed. It extends validated
methods to mixed and distributed settings; single-node results do not imply
distributed readiness or scale.

#### Wave 4 — mixed and distributed workload evidence

- **Target / priority:** v3.0.0; P1, conditional on v2.5.0 baseline acceptance.
- **Goal:** Determine whether selected module behavior remains correct and
  operationally useful across mixed, contention-heavy, and distributed
  scenarios.
- **Concrete tasks:** Select owner-approved mixed and distributed workloads;
  identify the canonical workload driver and control path for each; measure
  throughput, latency distributions, resource consumption, failure/fallback
  behavior, and relevant quality under concurrency and load changes; document
  topology, network, storage, and workload limits.
- **Dependencies:** Waves 1–3; distributed workload owners and stable
  multi-node test environment; root release governance approval for gates.
- **Effort:** Planning estimate: 4–6 person-weeks.
- **Risks / trade-offs:** Distributed effects and workload differences make
  single-node thresholds non-transferable; infrastructure noise or an
  unrepresentative topology can produce misleading conclusions.
- **KPI / acceptance:** Each claimed scenario has a repeatable manifest,
  host/topology-matched control, repeated results including p95/p99, and
  documented limits. Numerical targets are derived only after observed
  variability and workload requirements are reviewed; otherwise the result is
  an evidence report, not a pass.
- **Validation:** Repeated multi-node benchmark runs and scenario-specific
  correctness/failure tests; compare with the selected control; audit telemetry
  exposure, access, retention, and fallback operation. Do not extrapolate beyond
  tested topology.

#### Wave 5 — sustained operations and capability-path readiness

- **Target / priority:** v3.0.0; P1, conditional on Waves 3–4 findings.
- **Goal:** Validate long-duration workload shifts, recovery/rollback, and
  optional hardware or I/O paths under deployment-relevant constraints.
- **Concrete tasks:** Run sustained workloads with controlled workload changes;
  inspect adaptation stability, metric loss/overhead, resource bounds, and
  recovery behavior; test compile-time and runtime capability combinations and
  explicit fallbacks on supported/unsupported hosts; define operator
  observability, masking, access, retention, and rollback evidence.
- **Dependencies:** Stable candidate behavior from v2.5.0; supported host/kernel
  matrix and deployment/operator participation.
- **Effort:** Planning estimate: 3–5 person-weeks.
- **Risks / trade-offs:** Long runs are costly and host-specific; optional
  accelerators, PMEM, NUMA, or io_uring may not be available in all supported
  environments. A requested path is not proof it was used.
- **KPI / acceptance:** Declared duration, workload transitions, capability
  matrix, and recovery checks are completed with retained results; unsupported
  paths report explicit failure or documented fallback; no sensitive telemetry
  is emitted outside approved controls. Any numerical stability or regression
  limit is established from representative runs and approved by owners.
- **Validation:** Long-duration test and benchmark runs, supported/unsupported
  host checks, operator rollback exercise, and review against
  `TRUST.md`, `OPERATIONS.md`, and `PRODUCTION_REQUIREMENTS.md`.

#### Wave 6 — release decision and evidence publication

- **Target / priority:** v3.0.0; P0 release decision gate, only after Waves 4–5.
- **Goal:** Make module claims auditable and align their release disposition with
  the root roadmap.
- **Concrete tasks:** Consolidate test outcomes, manifests, workload and host
  scope, agreed thresholds or explicit threshold gaps, known limitations,
  fallback behavior, and security/operational review; have module and release
  owners decide which claims are supported and which gates remain blocked.
- **Dependencies:** Waves 1–5 and root release-governance review.
- **Effort:** Planning estimate: 1–2 person-weeks.
- **Risks / trade-offs:** Missing or stale evidence must block a claim rather
  than be replaced with extrapolation; module evidence cannot override project
  release status.
- **KPI / acceptance:** Every performance release claim points to retained,
  reproducible evidence with scope and limitations; unresolved evidence gaps
  remain explicit; root `ROADMAP.md` continues to govern release readiness.
- **Validation:** Independent review of manifests, tests, benchmark decisions,
  security/operations controls, and source references; verify the final
  disposition is consistent with the root roadmap.

## Research Basis and Evidence Limits

The following work informs evaluation design only; none is a locally reproduced
ThemisDB result or a source for numerical acceptance targets:

- Cooper et al., “Benchmarking Cloud Serving Systems with YCSB,” SoCC 2010,
  [DOI 10.1145/1807128.1807152](https://doi.org/10.1145/1807128.1807152):
  motivates representative workload definitions and characterization of
  throughput/latency as load scales.
- Friedrich, Wingerath & Ritter, “Coordinated Omission in NoSQL Database
  Benchmarking,” BTW 2017,
  [paper](https://wolle.science/publications/wingerath-2017-scdm-coordinated-omissions.pdf):
  describes how request generation can omit latency samples during stalls and
  distort tail measurements. Benchmark methodology must therefore document
  generator behavior and check tail-sampling validity.
- Dean & Barroso, “The Tail at Scale,” CACM 2013,
  [DOI 10.1145/2408776.2408794](https://doi.org/10.1145/2408776.2408794):
  explains why tail-latency variability matters in large distributed systems;
  it does not set a ThemisDB target.

Local evidence is still required: the current benchmark executables do not encode
a cross-run baseline or pass/fail threshold, and the isolated CMake registration
check did not compile or run tests.

## Implementation Phases

1. **Phase 1 — Design / API:** Keep the existing shared error/result contract
   stable. Specify workload, control, manifests, capability assumptions, and
   acceptance method before expanding benchmark claims. Any proposal to change
   the frozen v2.x contract requires explicit major-version coordination.
2. **Phase 2 — Core:** Complete the benchmark/test inventory decision and
   implement only evidence-backed harness or reliability changes needed for the
   v2.5.0 single-node evaluation. Avoid adding optimization paths merely to
   satisfy a performance target that has not been measured.
3. **Phase 3 — Error / edge cases:** Verify bounds, invalid inputs, unavailable
   statistics, resource exhaustion, stale state, exporter failure, and
   capability-dependent error/fallback behavior without converting failures
   into successful-looking results.
4. **Phase 4 — Tests:** Build and execute the intended root and recursive
   Phase 3/4 test set in declared configurations; record discovery, omissions,
   failures, and platform dependencies. Registration alone is not test
   acceptance.
5. **Phase 5 — Performance / hardening:** Establish repeated host-matched
   single-node baseline and adaptive/control evidence for v2.5.0; undertake
   mixed/distributed and sustained operational evidence for v3.0.0 only after
   earlier gates. Derive numerical criteria from measured variability and
   stakeholder requirements rather than inventing speedups.
6. **Phase 6 — Docs / acceptance:** Preserve manifests and limitations, verify
   masking/cardinality/access/retention and production configuration controls,
   publish the supported claim scope, and reconcile release disposition with
   root `ROADMAP.md`.

## Production Readiness Checklist

- [ ] Relevant test targets are configured, built, and executed; the current
  registration-only check is not sufficient.
- [ ] Duplicate focused/autofocused test registration has a documented
  disposition.
- [ ] Representative single-node baseline includes repeated runs, valid
  latency-tail sampling, raw results, and a complete manifest.
- [ ] Any numeric performance threshold is derived from local evidence and
  approved; otherwise the numeric gate remains unset.
- [ ] Adaptive changes are compared with a control across declared workloads and
  duration; forecasts/cost factors are not presented as measured gains.
- [ ] Mixed/distributed and sustained-operation claims have their own
  topology/workload-matched evidence before v3.0.0 release claims.
- [ ] Optional hardware and I/O paths are verified against compile/runtime/host
  support, with explicit failure or fallback behavior.
- [ ] Production configuration and resource limits are validated; optimizer
  convergence guardrails and fail-closed security settings are in place.
- [ ] Monitoring data is masked, bounded in cardinality, access-controlled, and
  retained according to deployment policy.
- [ ] Release evidence and remaining blockers are reviewed against root
  `ROADMAP.md`; no module-only status implies project release approval.

## Known Issues & Limitations

- The isolated CMake check confirmed 31 test registrations only; no compilation
  or execution result follows from it.
- Root `test_performance_*.cpp` files are currently included in both focused and
  autofocused registration paths; the intended deduplication policy remains
  unresolved.
- The six release-gate-named microbench cases have no numerical thresholds,
  cross-run baseline, or executable pass/fail gate. The name “release gates”
  does not establish release acceptance.
- The random-access benchmark's paired results do not calculate an on/off ratio
  or enforce a threshold. Its WAL-disabled setup needs a production-realism
  assessment before release use.
- No representative-host module baseline exists. p95/p99 refresh, mixed and
  distributed scenarios, and long-duration workload-shift evaluation remain
  open work.
- Forecasting, cost calibration, benchmark comments, and published research
  cannot substitute for measured control comparisons on the intended
  ThemisDB workload.
- Hardware and operating-system facilities are capability-dependent; local
  results do not imply universal host support. Telemetry and vector/index data
  require deployment-appropriate protection.

## Breaking Changes

No API breakage is planned for v2.5.0 or v3.0.0 in this roadmap. The shared
performance API contract describes itself as frozen for v2.x; any proposed
breaking change requires a deprecation notice and major-version coordination
under the contract and repository release policy. This roadmap does not authorize
such a change.

## Canonical References

- [Shared API contract](../../include/performance/performance_api_contract.h)
- [Implementation map](IMPLEMENTATION.md)
- [Performance expectations and benchmark evidence](PERFORMANCE_EXPECTATIONS.md)
- [Research and evaluation protocol](RESEARCH.md)
- [Operations](OPERATIONS.md), [trust and privacy](TRUST.md), and
  [production requirements](PRODUCTION_REQUIREMENTS.md)
- [Performance test inventory](../../tests/performance/README.md) and
  [test registration](../../tests/performance/CMakeLists.txt)
- [Benchmark inventory](../../benchmarks/performance/README.md) and
  [benchmark registration](../../benchmarks/performance/CMakeLists.txt)
- Cooper et al. (SoCC 2010),
  [DOI 10.1145/1807128.1807152](https://doi.org/10.1145/1807128.1807152)
- Friedrich, Wingerath & Ritter (BTW 2017),
  [paper](https://wolle.science/publications/wingerath-2017-scdm-coordinated-omissions.pdf)
- Dean & Barroso (CACM 2013),
  [DOI 10.1145/2408776.2408794](https://doi.org/10.1145/2408776.2408794)
- [Root roadmap and release authority](../../ROADMAP.md)

Exact model backend not verifiable from final document text alone.
