# Evaluation Module Roadmap

> **Update provenance (2026-11-06):** AI_Used: true; AI_Models: unknown;
> AI_Contributor: OpenAI ChatGPT; Human_Review: false; Human_Author: ThemisDB
> Contributors. Exact model backend not verifiable from final document text alone.

<!-- Source level: level1 · SOT domain: module-behavior -->
<!-- Last Updated: 2026-11-06 · Issue: #7417 · Milestone: DOC-WEEKLY-2026-45 -->

## Current Status

This roadmap is a research-backed plan, not a release acceptance record. The
source/CMake evidence audit for issue #7344 remains distinct from this forward
plan; its historical status and limitations are retained below. No proposed
future task is marked complete; the checked boxes are historical issue #7344
inventory only. Source presence or target registration does not establish
build, test, runtime integration, scientific validity, or release readiness.

The current evidence maturity is **M1 — contracts/source identified; execution
and end-to-end evidence unverified**. This is a planning rating, not a product
quality score. The module defines metric, benchmark bookkeeping, hardware
profile, ablation, planner, approximation and lifecycle interfaces. The
`themis_evaluation` target lists five implementation sources;
`query_planner`, `approximation_rules`, and artifact lifecycle also have
separately conditional library declarations. Five local test sources are
registered through `tests/evaluation/CMakeLists.txt`; high-cardinality stress
coverage uses stubs, and focused tests are contract tests rather than
end-to-end search-quality evidence. None of these declarations proves that a
target ran in a configured environment.

`retrieval_metrics.h` defines retrieval, evidence, provenance, compression,
LLM-answer, distributed and tensor-graph metric structures. The
`computeRetrievalQuality` contract takes binary relevant-item IDs, not graded
relevance labels. `query_planner.h` defines five typed paths and fallback
reasons; `selectPath()` selects a path, it does not execute retrieval. The
public `include/evaluation/evaluation.h` currently has no component includes.
The existing planner contract's exact fallback behavior, advisory-only tensor
artifacts, and CPU-only Category C invariants remain requirements; this roadmap
does not relax them.

### Source and execution evidence

- `src/evaluation/CMakeLists.txt` lists five sources in `themis_evaluation`.
  Planner and approximation are separate conditional libraries; artifact
  lifecycle also has a separately conditional library declaration.
- The five local test sources are listed/globbed by
  `tests/evaluation/CMakeLists.txt`. Registrations are not execution results.
  The high-cardinality stress harness uses stubs; focused tests establish
  component contracts, not end-to-end search quality.
- `benchmarks/evaluation/bench_evaluation_dedicated_gates.cpp` benchmarks stub
  functions. It is not production-path performance evidence. `BASELINES.md`
  contains no captured measurements.
- Historical `benchmarks/epic2_evaluation/` planner cases call in-process
  `selectPath()` and are separate from production retrieval; synthetic matrices
  are not production performance evidence. The artifact-staleness benchmark
  source is not registered. These facts do not establish benchmark execution.
- `src/evaluation/SECURITY.md` records that callers provide labels and
  measurement inputs: the module does not validate truth, authenticate or
  authorize, establish provenance, enforce caller decisions, or persist/rebuild
  artifacts. Those are explicit boundary limitations, not assumed capabilities.
- `StorageStrategyAssessor` is owned by `distributed_tensor`, not this module.
  Keep that external component outside all evaluation-module feature and
  performance claims.

### Root aggregate status and evidence

Root `ROADMAP.md` is the product maturity aggregate, not this module roadmap.
At this planning update, root `VERSIONING.md` defines SemVer, targets feature
work at the next MINOR (`v2.5.0-alpha1`), and reserves MAJOR increments for
incompatible API or wire changes; the root product roadmap reports `v2.4.0-alpha`.
The module roadmap is authoritative for module behavior, not release readiness.

Root `ROADMAP.md` links item 2.7 to sub-issue #5443 and merged implementation PR
#5606. The storage assessor and focused-test registration are owned by
`distributed_tensor`; the EPIC 2 benchmark is a separate synthetic placement
harness. The CI run attached to PR #5606 failed during configure, so build and
test steps were skipped. No captured benchmark run/result was found.
Consequently, neither 220+ passing tests nor completed benchmark runs are
verified acceptance evidence. This update does not close or reclassify issue
#5643.

## In Progress

- [ ] Record build/test/benchmark execution results only after running the
  applicable targets in a configured environment. Keep registration separate
  from execution evidence.
- [x] Identify the storage-strategy implementation and focused-test registration
  as `distributed_tensor`-owned; do not describe them as evaluation-module
  targets.
- [ ] Verify downstream workflow integration only against actual consumers and
  accepted module gates.
- [ ] Resolve the open dataset, workload, hardware, API-consumer, data-handling,
  and maintainer-priority assumptions before setting final acceptance gates.

## Planned Features

### Priority for v2.5.0 — next MINOR (proposed)

`v2.5.0` work is intended to be backward-compatible under the root SemVer policy.
Prioritize evidence quality and safe contracts before adding new metric
semantics:

1. Establish owned datasets/workloads, a reproducible evaluation protocol, and
   an evidence artifact schema; do not set absolute retrieval-quality targets
   until corpus and ground truth are agreed.
2. Test existing metric and planner invariants, invalid inputs, and fail-closed
   outcomes against implementation APIs and real consumer paths where available.
3. Replace stub-only performance claims with production-path benchmarks, an
   explicitly recorded environment, repetitions, retained raw output and
   artifact provenance. Keep any stub benchmark clearly labeled as a harness
   self-check.
4. Define the public API/umbrella-header and consumer obligations before
   exposing or promising a unified entry point. Preserve fallback reason
   visibility and caller responsibility.
5. Treat all checklist items as proposed planning work. No implementation,
   acceptance, or release readiness is asserted here.

### Priority for v3.0.0 — strategic MAJOR (conditional)

These are strategic proposals, not a commitment or an assertion that a breaking
change is required. Any incompatible API, configuration, or wire change needs
explicit compatibility design, issue tracking, deprecation/migration planning,
and root release-governance alignment.

1. If dataset ownership and repeated labeled observations are available, add
   statistically supported uncertainty reporting for AI-facing evaluation;
   distinguish item-level from system-level variation and disclose sampling
   assumptions.
2. Consider graded relevance and richer evaluation protocols only through an
   agreed contract, with compatibility/migration analysis for the existing
   binary-ID metric API.
3. Define versioned, privacy-reviewed evidence/run manifests and decide whether
   persistence belongs here or in a consuming service. Do not silently add
   persistence, authentication, authorization, truth validation, or rebuild
   responsibilities to this library.
4. Consider an integrated evaluation API only after consumers, target ownership,
   dependency/linkage and stable public-header responsibilities are known.
5. Establish release gates from measured data and agreed corpus/workloads;
   statistical outputs and benchmark tool features alone are not quality
   evidence.

### Testable runtime requirements versus source evidence

The following are proposed runtime acceptance requirements, to be verified by
tests on executed code and, where necessary, actual consumers. They are not
claims about current execution.

| Requirement | Testable acceptance intent | Evidence boundary |
|---|---|---|
| Explicit, safe planning | Exercise all five `ExecutionPath` cases, each applicable `FallbackReason`, stale/missing/rebuilding artifacts, forced exact/CPU, missing manifests, and failure inputs; verify exact fallback is explicit. | Header enums and CMake registration alone do not prove behavior or consumer enforcement. |
| Exactness and CPU-only Category C | Runtime tests reject/escalate truth-bearing approximate/bounded paths and reject GPU dispatch for Category C; test bypass/policy-version cases. | `approximation_rules.h` defines the contract; callers still enforce returned decisions. |
| Metric input integrity | Test documented invalid-input `MetricErrorKind` cases, numeric range/finite behavior, duplicates, empty/missing truth, and summary-first failures. Validate supplied labels separately at the dataset boundary. | Calculations cannot prove label truth, representativeness, or provenance. |
| Summary-first safety | On a declared test corpus with known misses, require **0 unrecovered summary-first false negatives** in the tested scenarios; every miss must trigger exact fallback or explicit failure. | Proposed invariant for evaluated scenarios, not a universal guarantee; exact fallback is caller/consumer behavior. |
| Repeatable measurement | Run production-path benchmarks with environment metadata and repetitions, preserve raw output, and report distributions rather than a single favorable run. | Existing stub or synthetic harness output is not production evidence. |
| Evidence provenance | Require 100% completion of an agreed mandatory run-manifest field set before a result is eligible as a baseline. | Proposed artifact gate; no current provenance verifier is claimed. |

### Open assumptions and knowledge gaps

- Dataset and gold-label ownership, consent/licensing, versioning, adjudication,
  freshness, and representativeness are not established.
- Expected production workload distributions (query mix, concurrency, corpus
  scale, shard count, data churn, hardware mix) are unknown.
- Reference hardware, operating system/build configuration, isolation policy,
  and acceptable run-to-run variance have not been agreed.
- The stable public API, consuming modules/services, their obligations to
  enforce planner/approximation decisions, and the intended umbrella-header
  exposure remain unresolved.
- Persistence, retention, privacy classification, sensitive query/answer
  handling, and access control for evaluation evidence are undecided.
- Maintainer ownership, priority against other roadmap work, staffing, and
  acceptance authority are not established. All effort figures below are
  planning estimates only.
- Current test execution, production-consumer integration, measured baseline,
  and current runtime false-negative behavior have not been verified by this
  documentation update.

## Research-to-gap/action links

| Research / guidance | Relevance and caveat | Gap and proposed action |
|---|---|---|
| NIST AI 800-3, *Expanding the AI Evaluation Toolbox with Statistical Models* (Feb 2026), [DOI](https://doi.org/10.6028/NIST.AI.800-3) | Calls for explicit statistical models, generalized accuracy estimates, separation of item/system variance, and uncertainty reporting for AI benchmarks. Relevant to future AI-facing metrics only when repeated, labeled data is sufficient; it does not imply these methods exist in this module. | Wave 4: define estimands, sampling/unit-of-analysis, repeated-run design, and uncertainty intervals; report “not estimable” when data is insufficient. |
| NIST AI RMF 1.0, [DOI](https://doi.org/10.6028/NIST.AI.100-1) | Govern/Map/Measure/Manage frames TEVV throughout the AI lifecycle. Use as a governance design lens, not as evidence of compliance or certification. | Wave 1: assign dataset, risk, ownership, review and evidence responsibilities; Wave 5: decide retention/privacy and consumer controls. |
| BenchExec, *Reliable benchmarking: requirements and solutions* (2017), [DOI](https://doi.org/10.1007/s10009-017-0469-y) | Discusses resource isolation and controlled, repeatable benchmark execution. It does not show current ThemisDB benchmark isolation. | Wave 3: specify controlled runner, resource/environment metadata, repeatability checks, and retained outputs. |
| Google Benchmark [user guide](https://google.github.io/benchmark/user_guide.html) | Documents warm-up, repetitions/standard deviation, machine context, and JSON/CSV output options. This is tool guidance, not a measured result; existing evaluation dedicated gates use stubs. | Wave 3: use and record appropriate tool options on production implementations; preserve raw machine-readable output and distinguish harness self-tests. |

## Implementation Phases

The checked boxes in Phases 1–6 below are the historical issue #7344
documentation/source inventory only; they do not mean a release gate passed.
The proposed Waves 1–5 that follow are a separate future plan, with effort
estimates in person-weeks. Estimates exclude unconfirmed data acquisition,
external consumer changes, and release-governance lead time; they are not dates
or release promises.

### Phase 1

**Design and API contract**

- [x] Identify component declarations under `src/evaluation/include/`.
- [ ] Define the intended public umbrella contract for the currently empty
  `include/evaluation/evaluation.h` and align its consumers/documentation.

### Phase 2

**Core implementation**

- [x] Inventory the seven implementation files and their CMake target ownership.
- [x] Identify the separate storage-assessor interface, implementation, library
  target and focused-test registration under `distributed_tensor`.

### Phase 3

**Errors and edge cases**

- [x] Confirm source representations for typed metric errors, planner fallback
  reasons, approximation decisions and lifecycle states.
- [ ] Complete any required correctness/security review; this documentation
  inventory is not an exhaustive audit.

### Phase 4

**Tests**

- [x] Distinguish local focused-test registrations from conditional EPIC 2
  registrations.
- [ ] Establish current build and test execution results in a configured
  environment.

### Phase 5

**Performance**

- [x] Distinguish registered planner/matrix/storage targets, the unregistered
  artifact-staleness source, synthetic simulation cases and source-level output
  capability.
- [ ] Capture measured results from applicable implementation benchmarks and
  retain the run environment; do not treat stub or synthetic simulation output
  as production-path evidence.

### Phase 6

**Documentation and acceptance**

- [x] Synchronize the scoped module documentation for issue #7344 while
  preserving dated acceptance and sign-off records as historical.
- [x] Align the root maturity summary with the cross-module source and CMake
  ownership.
- [ ] Obtain current human-reviewed build, test and benchmark acceptance
  evidence before asserting module readiness.

### Proposed Wave 1 — Evidence contract and study design

- **Goal:** Resolve assumptions and specify what a valid evaluation result means
  before choosing quality thresholds.
- **Prioritized tasks:** Assign dataset/gold-label ownership and versioning;
  document workload classes and sampling; define metric estimands and run
  manifest required fields; document the limits of binary relevance in
  `src/evaluation/include/retrieval_metrics.h`; specify consumer/API obligations
  and review `include/evaluation/evaluation.h` without assuming it is public
  today. Coordinate risk/ownership with `src/evaluation/SECURITY.md`.
- **Dependencies:** Named maintainers, identified consumers, dataset access and
  privacy/licensing review.
- **Risks/trade-offs:** A narrow owned corpus may not represent production;
  broad datasets increase labeling cost and privacy exposure; premature API
  stabilization may constrain future compatibility.
- **Measurable success criteria (proposed):** All required manifest fields have
  an owner and definition; each evaluation metric has a documented input,
  estimand and known limitation; workload and label gaps have explicit owners
  or a recorded blocker. No absolute relevance-quality target is set without
  agreed corpus and ground truth.
- **Validation strategy:** Review API/header contracts and module security
  boundaries; use design review checklist for label provenance, privacy and
  consumer enforcement; validate manifest schema with positive, missing-field
  and malformed-field cases once implemented.
- **Planning estimate:** 2–4 person-weeks.

### Proposed Wave 2 — Runtime invariants and fail-closed behavior (v2.5.0)

- **Goal:** Establish executable evidence that current metric, planner and
  approximation contracts behave safely on defined test cases.
- **Prioritized tasks:** Add/complete focused tests for
  `computeRetrievalQuality`, other public metric functions and `MetricError`;
  exercise all five paths/relevant fallbacks in `query_planner.cc`; exercise
  Category C/true-bearing cases in `approximation_rules.cc`; test
  `TensorGraphSnapshot::isUnrecoveredFalseNegative()` and summary-first metrics;
  verify thread-safety assumptions at the `MetricCollector` and observer
  boundaries. Use `tests/evaluation/` and `tests/epic2_evaluation/` registrations
  as appropriate, but capture actual execution separately.
- **Dependencies:** Wave 1 contract decisions; configured build with optional
  GTest/EPIC 2 targets available; agreed consumer test fixture for decisions
  enforced outside this library.
- **Risks/trade-offs:** Test doubles can prove decision logic but not retrieval
  quality or real caller enforcement; additional cases may expose compatibility
  issues requiring issue-level triage.
- **Measurable success criteria (proposed):** Every enumerated safety invariant
  and defined fail-closed branch has at least one executed positive/negative
  test; summary-first test scenarios have 0 unrecovered false negatives; no
  claim of universal recall is made from those finite scenarios.
- **Validation strategy:** Run actual focused CTest targets and report target,
  configuration and result; test exact fallback, missing/stale state, GPU
  eligibility and malformed metric values; run security/governance review that
  returned decisions are enforced by the consumer fixture. Registration alone
  is insufficient.
- **Planning estimate:** 3–6 person-weeks.

### Proposed Wave 3 — Controlled production-path baselines (v2.5.0)

- **Goal:** Capture repeatable performance evidence for real module
  implementations and any agreed consumer path, without promoting stub results.
- **Prioritized tasks:** Define reference machine/build and resource-isolation
  protocol; add or adapt production-path benchmarks under
  `benchmarks/evaluation/`; use Google Benchmark warm-up/repetition and
  machine-readable output where suitable; record raw output and a manifest
  (revision, compiler, flags, CPU, memory, OS/kernel, workload/data version,
  repetitions, tool version); update `src/evaluation/BASELINES.md` only from
  retained qualifying runs. Keep `bench_evaluation_dedicated_gates.cpp` marked
  as stub-only unless it is replaced with production API calls.
- **Dependencies:** Wave 1 workload/data and manifest decisions; Wave 2 correct
  runtime cases; available reference hardware and benchmark runner.
- **Risks/trade-offs:** Resource isolation and stable hardware cost time; shared
  runners can produce noisy results; optimizing synthetic cases can mislead
  without production representativeness.
- **Measurable success criteria (proposed):** At least 10 independent measured
  repetitions per accepted benchmark configuration (tool warm-up separate from
  measured repetitions); 100% of agreed manifest fields present; publish
  distribution and variability, flagging unstable runs rather than averaging
  away noise. Numerical regression thresholds remain unset until qualifying
  baselines and owners agree them.
- **Validation strategy:** Confirm benchmark calls production implementations;
  run controlled repeatability checks and inspect JSON/CSV plus raw outputs;
  verify artifact completeness automatically; security review data leakage in
  captured arguments/output. Existing stub and synthetic benchmark results do
  not satisfy this wave.
- **Planning estimate:** 3–5 person-weeks.

### Proposed Wave 4 — Statistical AI-evaluation extensions (v3.0.0 strategic)

- **Goal:** Add uncertainty-aware AI-facing evaluation only if labeled data and
  repeated observations support valid inference.
- **Prioritized tasks:** Design an explicit statistical model and estimand,
  separating item/system variation; specify sampling, repeated-run and
  uncertainty-interval methods; prototype metric/API changes and compatibility
  options for binary versus graded relevance; report data insufficiency and
  limitations. Keep model outputs separate from security/compliance claims.
- **Dependencies:** Wave 1 owned, versioned labels and repeated data; Wave 3
  reproducible collection; independent statistical and domain review.
- **Risks/trade-offs:** Small, biased or non-independent samples produce
  misleading intervals; added statistical dependencies and API changes may
  require a major-version compatibility decision; no model should manufacture
  certainty absent adequate data.
- **Measurable success criteria (proposed):** For each supported AI-facing
  estimate, publish an uncertainty interval and method when predeclared data
  sufficiency rules pass; otherwise return/document “not estimable.” Validate
  interval procedure on known synthetic distributions and sensitivity checks.
  Do not set absolute accuracy/recall thresholds absent agreed corpus and gold
  labels.
- **Validation strategy:** Statistical review; simulation-based tests for
  interval behavior and item/system variance; held-out dataset review; API
  compatibility and migration review under `VERSIONING.md`; security/privacy
  review of labels and outputs.
- **Planning estimate:** 5–9 person-weeks, excluding dataset labeling and
  external review.

### Proposed Wave 5 — Evidence lifecycle and consumer integration (v3.0.0 strategic)

- **Goal:** Decide and, if approved, deliver accountable evidence handling and
  consumer integration without confusing evaluation calculations with
  authorization, truth, persistence or rebuild execution.
- **Prioritized tasks:** Decide ownership for evidence persistence/retention and
  privacy; version a run/evidence manifest and define provenance verification;
  integrate planner/approximation outputs with named consumers and test exact
  fallback enforcement end-to-end; assess umbrella API and any breaking schema
  changes; keep storage-strategy work in `distributed_tensor` unless ownership
  is explicitly changed by canonical planning.
- **Dependencies:** Wave 1 ownership and privacy decision; executed Wave 2/3
  evidence; identified consumer maintainers; explicit major-version scope and
  migration approval where compatibility is affected.
- **Risks/trade-offs:** Persistent labels/results can expose sensitive inputs;
  library persistence adds operational coupling; consumer integration may
  expose independent failures; broad scope can exceed a major-release window.
- **Measurable success criteria (proposed):** 100% of accepted benchmark
  artifacts pass manifest validation; all named consumer fail-closed scenarios
  pass end-to-end tests; retention/deletion and privacy obligations have an
  owner and tested policy where persistence is approved. No compliance
  certification or general release readiness follows from these checks alone.
- **Validation strategy:** End-to-end tests with actual consumers, negative
  tests for missing/invalid provenance and failed exact fallback, security and
  privacy threat review, migration/compatibility review and release-governance
  checks. Run builds/tests only as planned implementation validation, not as
  part of this documentation-only update.
- **Planning estimate:** 4–8 person-weeks, excluding consumer-team work and
  release approval.

## Production Readiness Checklist

- [x] Resolve the root roadmap versus module-source ownership discrepancy.
- [ ] Establish current aggregate test and benchmark execution evidence.
- [ ] Establish current build and focused-test results in an available,
  configured environment.
- [ ] Capture and review benchmark results with hardware/build context; do not
  use stub or synthetic simulation results as module baselines.
- [x] Confirm item 2.7 traceability and cross-module implementation ownership
  through EPIC #5436, sub-issue #5443 and merged PR #5606.
- [ ] Verify downstream integration and acceptance gates before describing
  production readiness.
- [ ] Resolve the open assumptions and execute the proposed waves before
  asserting module readiness or release readiness.

## Known Issues & Limitations

- Root `ROADMAP.md` distinguishes cross-module implementation presence from
  aggregate EPIC 2 acceptance; current execution evidence is still missing.
- `include/evaluation/evaluation.h` is an empty umbrella, while component
  declarations are under `src/evaluation/include/`.
- `StorageStrategyAssessor` is owned by `distributed_tensor`, not this module.
  The separately registered EPIC 2 benchmark calls
  `DefaultShardPlacementStrategy` with synthetic nodes and does not establish
  measured `StorageStrategyAssessor` performance.
- Conditional CMake registrations do not prove execution. The local evaluation
  benchmark uses stubs; the EPIC 2 matrix simulations are synthetic, and
  artifact-staleness benchmark source is not registered.
- `computeRetrievalQuality` consumes binary relevant IDs; it does not implement
  statistical uncertainty estimates or validate the truth of caller-provided
  labels.
- Caller-supplied metrics, labels and planner signals are not authenticated,
  authorized, provenance-verified or automatically enforced by this library;
  it does not persist artifacts or rebuild them.

## Breaking Changes

This documentation-only update changes no API or runtime behavior and
introduces no breaking change. Any future contract or source change must be
assessed separately and recorded in the module changelog. Root
`VERSIONING.md` reserves a major bump for incompatible API or wire changes;
the v3.0.0 items above are strategic proposals, not a claim that such changes
are already approved.

## Historical status records

`MODULE_EVIDENCE.md`, `JUSTIFIED_GAP.md`, the Phase 4-6 checklists and the
Phase 6 sign-off template contain dated records associated with issue #5643.
Their recorded dates, blockers, phase marks and sign-off text are preserved as
historical material. They are not updated by this source audit and must not be
read as current build results or current issue state.

## References

- [Module overview](./README.md)
- [Architecture and source map](./ARCHITECTURE.md)
- [Implementation and CMake mapping](./IMPLEMENTATION.md)
- [Build/test/benchmark evidence](./MODULE_EVIDENCE.md)
- [Performance expectations](./PERFORMANCE_EXPECTATIONS.md)
- [Security boundary](./SECURITY.md)
- `ROADMAP.md` (root maturity aggregate; reconciliation required)
- `VERSIONING.md` (SemVer and release targeting)
- `src/evaluation/CMakeLists.txt`
- `src/evaluation/include/retrieval_metrics.h`
- `src/evaluation/include/query_planner.h`
- `src/evaluation/include/approximation_rules.h`
- `include/evaluation/evaluation.h`
- `tests/evaluation/CMakeLists.txt`
- `tests/epic2_evaluation/CMakeLists.txt`
- `benchmarks/evaluation/CMakeLists.txt`
- `benchmarks/evaluation/bench_evaluation_dedicated_gates.cpp`
- `benchmarks/epic2_evaluation/CMakeLists.txt`
- `src/evaluation/BASELINES.md`
- `src/evaluation/SECURITY.md`
