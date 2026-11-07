# Temporal Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | source review: 2026-10-31 | historical validation: TEMPORAL_MODULE_STATUS_VALIDATION.md -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · TEMPORAL_MODULE_STATUS_VALIDATION.md -->

Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-11-07
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors

Exact model backend not verifiable from final document text alone.

## Current Status

The source tree contains temporal runtime APIs and implementations for the
subsystems listed below. The root [`ROADMAP.md`](../../ROADMAP.md) currently
classifies temporal as `PRODUCTION_CANDIDATE` and remains authoritative for
maturity, blockers, and release readiness. Source/test presence is not evidence
that a build, benchmark, or release gate passed.

This roadmap is a module plan, not a release-readiness declaration. The current
product line is v2.4.0-alpha; [`VERSIONING.md`](../../VERSIONING.md) identifies
v2.5.0-alpha1 as the next MINOR feature lane. All work below is planned, not
completed. v2.5.0 is scoped to compatibility-preserving validation and
integration; v3.0.0 is a strategic lane for changes that would require approved
breaking-change and migration governance.

### Evidence and research gap analysis

- **Temporal semantics:** [`temporal_types.h`](../../include/temporal/temporal_types.h)
  defines `themisdb::temporal::Timestamp` as milliseconds since Unix epoch and
  `TimeRange` as half-open `[start, end)`. The runtime has system-time and
  valid-time APIs, but these declarations alone do not prove consistency across
  all operations or SQL/AQL language integration; see
  [`ARCHITECTURE.md`](ARCHITECTURE.md) and
  [`temporal_query_engine.h`](../../include/temporal/temporal_query_engine.h).
- **Test evidence:** [`tests/temporal/CMakeLists.txt`](../../tests/temporal/CMakeLists.txt)
  glob-registers focused test sources and has a second autogen registration
  loop. Registration is not execution evidence. In particular,
  `test_temporal_highcardinality_stress.cpp` models behaviors with local stubs,
  not temporal runtime classes. Runtime-backed edge/property tests and recorded
  execution evidence are therefore explicit priorities.
- **Performance evidence:** the runtime benchmark sources are
  [`bench_temporal_queries.cpp`](../../benchmarks/temporal/bench_temporal_queries.cpp)
  and [`bench_interval_tree_erase.cpp`](../../benchmarks/temporal/bench_interval_tree_erase.cpp).
  The dedicated/release gate sources use stubs or mocks, benchmarks are
  optional, and no current run or baseline is recorded
  ([benchmark inventory](../../benchmarks/temporal/README.md),
  [performance evidence](PERFORMANCE_EXPECTATIONS.md)). Do not infer an SLA or
  runtime gate pass from source names or embedded thresholds.
- **Boundaries:** snapshots and the default CDC ring are in-memory;
  `CDCPersistentLog` is a filesystem WAL, and retention can physically remove
  versions. Temporal APIs do not themselves authenticate or authorize callers.
  These limits are documented in
  [`SECURITY.md`](SECURITY.md) and [`OPERATIONS.md`](OPERATIONS.md); they must
  remain visible in service integration, recovery, retention, and audit plans.
- **Research context:** Kulkarni and Michels describe SQL:2011 periods,
  system/application time, bitemporal semantics, and related SQL features
  ([SIGMOD Record 41(3), 2012](https://doi.org/10.1145/2380776.2380786)); this
  is a semantics reference, not evidence that ThemisDB's parser or APIs are
  integrated. Mostafa et al. argue that parallelism, batch ingestion,
  regularity, multivariate data, mixed workloads, and system metrics expose
  time-series bottlenecks hidden by single-axis tests
  ([arXiv:2608.01459, 2026](https://arxiv.org/abs/2608.01459)); use these as
  benchmark dimensions to evaluate, not as measured ThemisDB results. Simatis
  et al. introduce AITTI as a temporal-indexing research alternative
  ([Proc. ACM Manag. Data 4(4), 2026](https://doi.org/10.1145/3837093)); compare
  it with current indexes before considering adoption. NIST SP 800-92 gives
  log-management program and process guidance
  ([publication](https://csrc.nist.gov/pubs/sp/800/92/final)); it does not
  establish temporal-database-specific compliance or a compliance guarantee.
  [`research/papers/temporal_sql2011_2012.md`](../../research/papers/temporal_sql2011_2012.md)
  is background only; its broader adoption statements are not used as evidence
  for current implementation.

#### Open assumptions and knowledge gaps

- Which AQL/SQL temporal clauses, if any, are required for an end-to-end
  supported product contract, and which service owns parsing, authorization,
  and execution integration?
- What temporal test profiles, sanitizer jobs, representative hardware, and
  benchmark-run artifacts are canonical for release decisions? The inspected
  source inventory does not answer this.
- What are the required recovery point/time objectives and legal retention
  obligations for consuming deployments? No module-level default should be
  treated as an approved deployment policy.
- Which CDC consumers require durable replay, and what are the intended
  guarantees for WAL corruption, callback failure, and replay checkpoints?
- Which workload distributions and data scales should drive index selection?
  No single benchmark shape or published research result establishes that
  answer for ThemisDB.

### Prioritized release plans

Priority order is correctness and reproducible evidence first, then operational
and security boundaries, then broader language/API integration. Estimates are
preliminary person-weeks, exclude cross-team scheduling, and must be refined
with maintainers. KPIs below are proposed acceptance evidence, not current
results. Performance comparisons must use a recorded, workload-matched baseline
on identified hardware; consider a regression gate such as no more than 10%
regression only after the baseline and configuration exist and are approved.

#### v2.5.0 — compatibility-preserving MINOR lane

##### Wave 1 — Runtime correctness and test realism (Priority 1)

- **Goal:** establish reproducible runtime evidence for time semantics,
  lifecycle boundaries, and failure behavior without changing public contracts.
- **Tasks:**
  - [ ] **Target: v2.5.0** Add runtime-backed boundary/property tests for
    half-open ranges, adjacent/overlapping intervals, system/valid-time
    transitions, timestamp extremes, and malformed inputs; tie each case to
    the relevant runtime API.
  - [ ] **Target: v2.5.0** Replace or clearly separate stub-only
    high-cardinality examples with stress cases that instantiate temporal
    runtime classes; assert invariants and deterministic cleanup, not only
    modeled outcomes.
  - [ ] **Target: v2.5.0** Record the exact CMake/CTest registrations, enabled
    configuration, executed test names, and result artifacts; resolve duplicate
    registration or target-link ambiguity where reproduced.
- **Preliminary estimate:** 4–7 person-weeks.
- **Dependencies:** temporal runtime owners; focused-test CI configuration and
  an agreed supported build profile.
- **Risk / trade-off:** runtime stress tests can be slow or flaky; bound test
  duration and keep deterministic unit/property coverage separate from soak
  tests. Do not equate registered tests with executed tests.
- **KPIs:** all selected semantic invariants have runtime-backed cases; each
  listed case has a traceable test and recorded pass/fail run; repeated seeded
  stress runs report zero invariant violations; no test is counted solely from
  source presence.
- **Validation:** run the focused temporal CTest set in the declared profile,
  verify the test inventory against CMake registration, run sanitizer-enabled
  cases where supported, and attach logs/artifact references to the owning
  validation record.

##### Wave 2 — Workload-sensitive benchmark baseline (Priority 2)

- **Goal:** produce comparable runtime measurements and decide which
  workload-specific regressions merit a gate.
- **Tasks:**
  - [ ] **Target: v2.5.0** Define a versioned benchmark manifest containing
    runtime/build revision, compiler and flags, hardware, dataset/history
    shape, seed, concurrency, warm-up, repetitions, and measurement procedure.
  - [ ] **Target: v2.5.0** Extend runtime benchmarks to cover batch and
    single-row ingestion, history depth, interval overlap/regularity,
    concurrent readers/writers, mixed read/write and retention workloads,
    and report throughput plus latency distributions and system resource
    metrics.
  - [ ] **Target: v2.5.0** Separate runtime benchmark results from mock/stub
    gate outputs; record baseline artifacts before proposing any
    baseline-relative regression threshold.
- **Preliminary estimate:** 4–6 person-weeks.
- **Dependencies:** Wave 1 runtime correctness; benchmark-enabled build;
  representative, identified hardware and storage configuration.
- **Risk / trade-off:** more dimensions improve diagnostic value but increase
  run time and result variance; publish a small repeatable core matrix and keep
  exploratory matrices non-gating.
- **KPIs:** every published result is reproducible from a complete manifest;
  the agreed core workload matrix has results for throughput, latency
  distributions, and resource use; changes are compared only with a
  configuration-matched baseline. Any proposed ≤10% regression bound is
  provisional until baseline evidence and gate ownership are approved.
- **Validation:** execute both runtime benchmark binaries; verify that output
  calls runtime implementations; repeat runs to characterize variance; review
  manifests and results with performance owners before enabling a gate.

##### Wave 3 — Security, resilience, lifecycle, and operability (Priority 3)

- **Goal:** make temporal data-lifecycle and recovery limitations actionable
  for embedding services without implying temporal APIs provide authorization
  or compliance.
- **Tasks:**
  - [ ] **Target: v2.5.0** Define and test failure/restart behavior for CDC
    ring overflow, filesystem WAL write/read errors, callback exceptions, and
    retention interruption; document what is lost, replayable, or recoverable
    for each boundary.
  - [ ] **Target: v2.5.0** Specify consuming-service controls for caller
    authentication/authorization, event payload minimization, WAL directory
    permissions, backup/deletion, and retention/legal-hold policy ownership;
    do not label module behavior as a compliance guarantee.
  - [ ] **Target: v2.5.0** Validate operator procedures for snapshot bounds,
    CDC overflow, WAL capacity/recovery, retention effects, and escalation
    against the owning deployment; add observable service-side signals only
    where the owning service can supply them.
- **Preliminary estimate:** 4–7 person-weeks.
- **Dependencies:** service/security/storage owners; Wave 1 failure cases;
  deployment-specific runbook and recovery environment.
- **Risk / trade-off:** durable replay and stronger guarantees may add I/O,
  storage, and operational burden; keep guarantees explicit and avoid claiming
  durability for in-memory components.
- **KPIs:** each documented failure boundary has a reproducible test or an
  explicit deployment-owner validation; recovery steps distinguish durable
  WAL from in-memory state; retention dry-run/review records identify affected
  data and irreversible deletion; security review records the integration
  owner and unresolved controls.
- **Validation:** fault-injection/restart tests for WAL and callbacks; CDC
  overflow and retention tests; security review of data-flow and access
  boundaries; operator walkthrough with captured outcomes. NIST SP 800-92 may
  inform log-management process design, not serve as a product compliance
  attestation.

##### Wave 4 — Compatibility contract and integration inventory (Priority 4)

- **Goal:** document supported runtime contracts and expose integration gaps
  before introducing new syntax or behavior.
- **Tasks:**
  - [ ] **Target: v2.5.0** Build a contract matrix for runtime APIs, units,
    range boundaries, serialization, errors, persistence, and retention;
    reconcile it with `include/temporal/` declarations and module docs.
  - [ ] **Target: v2.5.0** Trace structured temporal query APIs through AQL/SQL
    parsing, planning, execution, authorization, and serialization; mark each
    integration point implemented, absent, or unverified rather than infer
    parser support from query helpers.
  - [ ] **Target: v2.5.0** Publish compatibility tests and examples for
    existing callers; route additive adapters through existing interfaces and
    avoid changing timestamp units, range inclusivity, or existing signatures
    in this MINOR lane.
- **Preliminary estimate:** 3–5 person-weeks.
- **Dependencies:** query-language/API owners; Wave 1 contract tests; release
  owners for compatibility review.
- **Risk / trade-off:** integration inventory may reveal incomplete or
  inconsistent APIs; defer incompatible normalization to v3 rather than
  silently widening v2.5 scope.
- **KPIs:** every public temporal contract used by the selected integration
  path has a cited declaration and an executable compatibility test; every
  parser/planner/authentication boundary has an owner and a verified status;
  no unapproved public breaking change is included.
- **Validation:** API/architecture review against runtime headers and
  implementations; parser-to-runtime integration tests for any claimed
  supported syntax; release-note and compatibility review under
  [`VERSIONING.md`](../../VERSIONING.md).

#### v3.0.0 — strategic, breaking-change-controlled lane

##### Wave 5 — Unified temporal architecture and language/API contracts

- **Goal:** decide and, if approved, deliver a coherent contract spanning
  runtime APIs, query language, and service authorization boundaries.
- **Tasks:**
  - [ ] **Target: v3.0.0** Resolve open semantics for timestamp units/time
    zones, period boundaries, system versus application time, error behavior,
    serialization, and cross-interface consistency; publish normative
    compatibility decisions before implementation.
  - [ ] **Target: v3.0.0** Design an explicit parser/planner/executor adapter
    for selected temporal clauses and document authentication/authorization
    integration with the owning service; do not make module API presence a
    proxy for end-to-end support.
  - [ ] **Target: v3.0.0** If incompatible API or wire behavior is approved,
    open/link the breaking-change issue, document deprecations, and provide
    migration guidance as required by version policy before removing old
    behavior.
- **Preliminary estimate:** 6–10 person-weeks.
- **Dependencies:** Wave 4 integration inventory; query/API/security owners;
  explicit release-manager and maintainer approval for any contract break.
- **Risk / trade-off:** unified semantics can reduce ambiguity but may break
  callers or constrain existing module APIs; preserve adapters where feasible
  and do not promise language coverage outside tested syntax.
- **KPIs:** each selected temporal clause has a specification, parser-to-runtime
  test, authorization boundary, and error mapping; every breaking decision has
  an approved issue and migration mapping; unsupported syntax remains
  explicitly rejected or documented.
- **Validation:** contract conformance suite across direct API and language
  paths; authorization-negative tests in the consuming service; compatibility
  review and documented migration rehearsal before release-candidate scope.

##### Wave 6 — Durability, retention, and migration architecture

- **Goal:** determine which lifecycle guarantees should become durable and
  define safe migration/recovery for any approved architectural change.
- **Tasks:**
  - [ ] **Target: v3.0.0** Specify durability and replay guarantees separately
    for snapshots, CDC, WAL, retention archives, and conflict history; include
    crash consistency, corruption handling, backup/restore, and deletion
    semantics.
  - [ ] **Target: v3.0.0** Evaluate durable snapshot/archive or CDC designs
    against current in-memory and filesystem behavior; select a design only
    after failure-model, resource, and security review.
  - [ ] **Target: v3.0.0** Define a versioned migration plan with preflight,
    idempotency, interruption/restart behavior, reconciliation checks,
    rollback limits, and treatment of data already removed by retention.
- **Preliminary estimate:** 6–12 person-weeks.
- **Dependencies:** storage/WAL owners; Wave 3 failure inventory; Wave 5
  contract decisions; deployment backup and recovery requirements.
- **Risk / trade-off:** stronger durability can increase write latency, disk
  use, and operational complexity; irreversible retention deletion cannot be
  undone by a code rollback.
- **KPIs:** every persistence path has a stated durability/recovery contract;
  migration rehearsal reports source/destination counts and consistency
  outcomes; injected interruption leaves a documented recoverable or
  fail-closed state; retention policy checks detect prohibited deletion before
  execution where the product contract permits.
- **Validation:** crash/restart and corruption fault-injection; backup/restore
  and migration rehearsal on representative history; security review of WAL,
  backups, deletion and access controls; operational sign-off by storage and
  data owners.

##### Wave 7 — Index strategy and comparative performance

- **Goal:** make temporal index evolution evidence-led and workload-specific,
  retaining current structures unless comparative results justify change.
- **Tasks:**
  - [ ] **Target: v3.0.0** Compare current interval-index behavior with
    alternatives, including AITTI as research context, across overlap
    density, range lengths, update/delete mix, concurrency, memory, and
    recovery requirements.
  - [ ] **Target: v3.0.0** Select an index or hybrid design only when runtime
    benchmark results show a repeatable advantage on agreed workloads and
    semantic tests establish equivalent range behavior.
  - [ ] **Target: v3.0.0** Define index rebuild, format/versioning, rollback,
    and migration procedures before changing persistent or externally visible
    index contracts.
- **Preliminary estimate:** 5–9 person-weeks.
- **Dependencies:** Wave 1 correctness suite; Wave 2 benchmark manifest and
  baseline; Wave 6 persistence/migration decisions if index state is durable.
- **Risk / trade-off:** a research alternative may improve selected workloads
  while worsening memory, mutation, or other distributions; avoid
  single-workload adoption or claims based on publication results alone.
- **KPIs:** comparison covers the agreed workload dimensions and resource
  metrics; semantic suite passes for every candidate; adoption requires
  repeatable, baseline-relative improvement for the targeted workload with
  regressions and trade-offs reported; absent evidence leaves the current
  implementation unchanged.
- **Validation:** repeatable benchmark matrix on identified hardware; run
  correctness/property tests and concurrency checks; review raw result
  manifests, confidence/variance, memory use, and migration/rollback rehearsal.

##### Wave 8 — Release governance, security assurance, and operational acceptance

- **Goal:** integrate module evidence into project release decisions without
  claiming module-level evidence satisfies product-wide readiness or
  regulatory requirements.
- **Tasks:**
  - [ ] **Target: v3.0.0** Assemble traceable evidence for correctness,
    benchmark baselines, security boundaries, recovery, migration,
    observability ownership, and operator procedures; link artifacts to their
    producing test/configuration/revision.
  - [ ] **Target: v3.0.0** Map temporal data and log controls to applicable
    project and deployment requirements; record control owners, gaps, and
    approvals. Do not imply NIST SP 800-92 or temporal functionality alone
    establishes compliance.
  - [ ] **Target: v3.0.0** Reconcile module evidence and any residual blockers
    with root [`ROADMAP.md`](../../ROADMAP.md), release policy, changelog, and
    migration documentation; leave readiness determination to those canonical
    release sources.
- **Preliminary estimate:** 3–6 person-weeks.
- **Dependencies:** Waves 5–7; security, operations, compliance, and release
  owners; current root release gates.
- **Risk / trade-off:** evidence assembly and external control ownership can
  extend schedules; module completion cannot bypass product-wide release
  blockers or human sign-off.
- **KPIs:** each claimed gate has a current artifact, revision/configuration,
  owner, and pass/fail result; all open gaps have owners and disposition;
  temporal readiness status remains consistent with the root roadmap.
- **Validation:** governance review of evidence links and version scope;
  security/operations review of recovery and runbooks; root-roadmap and
  release-document consistency review before any release claim.

## In Progress

- [!] Temporal-specific CI, sanitizer, and representative-hardware evidence was not established by this source review. **TODO:** record canonical run/artifact links before making module release-readiness claims.
- [!] Reconcile the historical diagnostics-hardening items below against current test/run evidence before marking them complete.
- [!] Whether the temporal benchmark prototypes are active release gates is unverified; the module benchmark sources do not provide recorded runtime results or a gate configuration.

## Planned Features

### Short-term (3-6 months)
- [!] `tests/temporal/test_temporal_highcardinality_stress.cpp` contains stub-based versioned-write, conflict-resolution, and CDC examples (TSTR-01..03); it does not exercise retention or the temporal runtime. **TODO:** add runtime-backed stress coverage and attach execution evidence before treating these behaviors as validated.
- [!] The runbook referenced for operator diagnostics is outside this module's Markdown scope; verify its commands and deployment-specific metrics against the owning service before claiming operational validation.

### Mid-term (6-12 months)
- [!] Re-baseline p95/p99 for runtime query/history workloads (Target: Q1 2027). **TODO:** identify representative hardware, baseline artifact, and current results.
- [!] `bench_temporal_dedicated_gates.cpp` contains four local stub benchmarks, not workload coverage of the temporal runtime; it does not close this item.
- [!] Integration soak-test source exists at `tests/integration/test_temporal_store_soak.cpp`; no execution result is recorded here (Target: Q1 2027).

## Implementation Phases

### Phase 1: Design / API Contract
- [x] A distinct `temporal_api_contract.h` declares query/version statements for `themis::temporal`; it is not a unified contract for the `themisdb::temporal` runtime.
- [x] The same separate header declares `TemporalErrorCode`; runtime API failures remain header-specific.

### Phase 2: Core Implementation
- [x] Runtime implementations and public declarations are present for bitemporal/query and lifecycle paths; presence does not assert release validation.
- [x] CDC, compression, and tiering implementations are present; stress/benchmark outcomes remain separately unverified.

### Phase 3: Error Handling and Edge Cases
- [!] Cross-interface failure and recovery behavior requires current test evidence; return values and exceptions differ by API.
- [!] Deployment-level alert/diagnostic mappings must be verified in the service that owns them; no temporal exporter or alert configuration is defined by these headers.

### Phase 4: Tests
- [!] Focused regression source and TCH markers are present; execution results are not established here.
- [!] Stress-test sources are present; deterministic runtime results are not established here.

### Phase 5: Performance and Hardening
- [x] Benchmark source files exist for runtime operations and mock gate examples; `bench_temporal_release_gates.cpp` and `bench_temporal_dedicated_gates.cpp` use mock/stub workloads.
- [!] Validate p95/p99 and throughput against a release baseline. **TODO:** provide runtime benchmark configuration and recorded results; the source files alone do not establish this.

### Phase 6: Documentation and Acceptance
- [~] Core temporal module docs are being aligned to source-verifiable behavior; this update awaits human review.
- [x] roadmap/future planning separated from historical changelog entries
- [x] temporal_api_contract.h frozen contract header published (Completed 2026-07-29)

## Production Readiness Checklist

- [~] Core temporal surfaces are documented; this documentation update is awaiting human review.
- [~] Security and failure boundaries are documented in this module; this does not establish a security audit or production approval.
- [x] Benchmark source mapping is documented; results and active release-gate wiring are not established.
- [!] `temporal_api_contract.h` exists as a distinct `themis::temporal` header. Its written contract must not be conflated with the `themisdb::temporal` runtime semantics.
- [!] `test_temporal_contract_hardening_focused.cpp` and TCH markers are present; no test execution result is asserted.
- [!] `bench_temporal_release_gates.cpp` and TRG markers are present, but the source uses mock helpers; no runtime gate pass is asserted.
- [!] Remaining lifecycle hardening and release benchmark stabilization require current test/run evidence before closure.

## Known Issues and Limitations

- runtime behavior depends on temporal workload shape and retention/snapshot configuration.
- **TODO:** confirm whether lifecycle and conflict edge scenarios remain open using current test/run evidence; the older planning claims were not independently verified here.
- benchmark depth should continue expanding for advanced temporal workloads.

## Breaking Changes

The inspected source does not establish a current breaking-change plan. Any proposed contract break must be confirmed by the owning maintainers and recorded with migration notes and a changelog entry.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `temporal`
- [ ] Verify applicable tracing/exporter requirements and evidence (Target: Q1 2027); the temporal APIs do not themselves define an exporter.
- [ ] Verify long-duration soak-test execution for the temporal paths (Target: Q1 2027); source presence is not a passing result.
- [ ] Verify operator runbook commands, metrics, and alert names against the owning deployment (Target: Q1 2027).

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [!] No distributed/acceleration temporal path or fail-closed release criterion is substantiated by the inspected module API/source set.
- [!] Benchmark-backed p95/p99 results on representative hardware are not established here (Target: Q1 2027). **TODO:** record the baseline and run artifact.
- [!] Deployment-specific diagnostics, alerts, and runbooks require verification against the owning service (Target: Q1 2027).
