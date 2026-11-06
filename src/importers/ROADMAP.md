---
Author: "ThemisDB Team"
Created: "2026-08-02"
Last Updated: "2026-11-06"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "ROADMAP.md; include/importers/importers_api_contract.h; tests/importers/CMakeLists.txt; benchmarks/importers/CMakeLists.txt"
Related_Reference: "makr-code/ThemisDB#7426"
---

> Creation-date note: Original creation date is not recorded in the available checkout; the front-matter `Created` date is a metadata placeholder, not independently verified.

> Exact model backend not verifiable from final document text alone.

# Importers Module Roadmap

## Current source/configuration snapshot (2026-11-06)

The importer sources and public headers are present in this checkout, but this
page does not certify module maturity or release readiness. The local focused
test registration discovers 12 test sources and excludes three; two benchmark
targets are declared when benchmark building is enabled. The focused mock test
passed directly (16/16); standalone benchmark sources ran, but the benchmark
library was built as DEBUG. No CTest execution, live connector/runtime test, or
release-profile benchmark was completed. See [`BUILD_STATUS.md`](./BUILD_STATUS.md).

The assessment and versioned waves below are the active module plan. Historical
Q3 2026 delivery notes later in this file are retained as history, not as current
test, benchmark, or release-readiness evidence. The repository currently targets
v2.4.0-alpha; v2.5.0 is the next planned MINOR target and v3.0.0 is a strategic
MAJOR target, subject to root release governance and human approval.

## Current Status

### Evidence-based maturity assessment

**Assessment: broad connector and helper implementation, but integration and
operational evidence are not yet sufficient to call the complete module
production-validated.** Relational, document, streaming, file, object, CDC,
Wikipedia, and specialized ingestion paths exist. Their option support,
checkpoint persistence, failure recovery, and external-I/O behavior vary by
connector; shared interfaces must not be read as proof of parity.

- **Performance and scale:** two benchmark executables are registered when
  benchmark builds are enabled. `bench_importers_release_gates.cpp` has six
  in-memory cases; its p99 labels are not computed measurements or enforced
  gates. `bench_importer_throughput.cpp` scans generated data rather than
  exercising all production connectors. The available exploratory run used a
  DEBUG benchmark library; no representative Release-profile or end-to-end
  connector baseline is recorded.
- **Use cases and usability:** flat-file, SQL/database, Kafka/CDC, S3, Wikipedia,
  and plugin-oriented ingestion are represented in code. `ARROW_ENABLED` gates
  Parquet. The current HuggingFace path normalizes caller-provided rows into
  in-memory state; it is not a live service client or durable database writer.
  Connector capability and supported options need to be discoverable before job
  configuration.
- **Correctness, security, and resilience:** the headers describe ordering,
  idempotency, schema evolution, quotas, cancellation, and errors. The versioned
  contract is not consumed by an import coordinator in `src/importers/`; its
  self-contained test therefore does not prove live connector behavior or
  destination atomicity. `permission_check` is optional, checkpoint/quarantine
  storage is caller-managed, and cancellation is not proof of rollback.
- **Test and governance maturity:** 12 test sources are discovered and three
  suites are skipped by `tests/importers/CMakeLists.txt`; nine are registered
  there. The most recent recorded focused contract test passed 16/16 on
  2026-10-31, but no CTest execution, live-connector integration test, or full
  module build is recorded. Release decisions must use current retained evidence,
  not the historical checklist below.

### Research basis and implementation gaps

1. Kafka and Debezium documentation distinguish default at-least-once delivery
   (possible duplicate delivery) from exactly-once behavior that requires a
   supported transactional processing boundary. This supports an explicit
   at-least-once plus idempotent-apply default, and end-to-end replay tests; it
   does not justify promising exactly-once for arbitrary external sinks.
2. The Apache Beam model documents event-time watermarks and late data. For
   importer CDC/stream paths, checkpoint position, ordering scope, late/out-of-
   order events, and replay boundary therefore need explicit per-connector
   semantics rather than a blanket ordering claim.
3. The 2026 survey of stream-processing benchmarks compares workload type,
   ingestion, system under test, and tracked metrics. The importer benchmark
   plan accordingly adds a named workload matrix and end-to-end throughput,
   tail-latency, resource, and failure/recovery measurements; a parser
   microbenchmark alone cannot establish connector scalability.
4. NIST SP 800-92 treats log management as an operational process spanning
   infrastructure and retention. Importer audit, diagnostic, checkpoint, and
   quarantine records therefore need ownership, access, retention, and
   redaction rules. This is a design input, not a compliance certification.

| Observed gap | Source evidence | v2.5.0 response | v3.0.0 direction |
|---|---|---|---|
| Versioned replay/visibility rules have no live coordinator consumer | `include/importers/importers_api_contract.h`; `src/importers/` coordinator boundary described in `RESEARCH.md` | Implement and test only the supported coordinator/sink guarantees; state connector exceptions | Versioned capability and coordinator contract with explicit sink transaction boundary |
| Test coverage does not demonstrate live connector parity or recovery | `tests/importers/CMakeLists.txt`; three skipped sources; `BUILD_STATUS.md` | Restore or replace skipped suites and add opt-in connector integration/restart tests | Continuous compatibility and recovery matrix across supported connector versions |
| Existing benchmark targets are synthetic and do not compute/enforce p99 | `benchmarks/importers/bench_importers_release_gates.cpp`; `PERFORMANCE_EXPECTATIONS.md` | Add reproducible Release-profile connector benchmarks and retained baselines | Scale-out, sustained-load, and fault-recovery benchmark suite |
| Optional shared features are not uniformly implemented or persisted | `include/importers/importer_interface.h`; connector implementations; `OPERATIONS.md` | Publish an implementation/capability matrix; test advertised behavior | Versioned capability negotiation and stable operational contracts |
| Diagnostic and recovery artifacts have sensitive-data and ownership boundaries | `SECURITY.md`, `TRUST.md`, `OPERATIONS.md` | Define and test redaction, access, retention, and recovery expectations | Governed durable lineage/audit metadata with tenant and retention boundaries |

## Prioritized Release Measures

### v2.5.0 — verification and bounded hardening

The next MINOR release should close evidence and implementation gaps without
claiming a new universal delivery guarantee. Preserve the current public contract
unless a documented additive extension is necessary.

1. **P0 — live contract and restart correctness:** select supported connectors,
   connect the versioned coordinator contract to a real implementation where
   feasible, and verify ordering scope, replay/idempotent apply, commit
   visibility, timeout, cancellation, and schema-change behavior against a
   destination. Unsupported semantics must be returned explicitly.
2. **P0 — recover focused test coverage:** repair the three skipped suites or
   replace them with focused equivalents; register and run all maintained
   importer tests. Add local-service integration coverage for the supported
   Kafka/CDC and database paths, including restart around checkpoint/commit
   boundaries.
3. **P1 — bounds, security, and diagnostics:** document capability differences;
   verify configured row/byte/batch limits, permission denial, credential
   redaction, and retention boundaries for logs/checkpoints/quarantine; make
   failure and cancellation outcomes observable.
4. **P1 — evidence-grade performance and operations:** add end-to-end cases for
   representative file, database, CDC/stream, and object paths; record
   throughput, p50/p95/p99, CPU, peak RSS, errors, and recovery lag alongside
   environment and workload metadata; link operator recovery steps.

### v3.0.0 — strategic importer platform

The MAJOR release may revise public contracts only after an ADR, compatibility
inventory, migration guide, and release-governance review.

1. **P0 — capability-driven API and coordinator:** replace assumptions of
   connector parity with explicit, versioned capabilities and delivery,
   ordering, schema, transaction, checkpoint, and retry semantics.
2. **P0 — durable replay and recovery model:** design durable source positions,
   checkpoint/commit coordination, idempotent destination application, replay,
   and reconciliation. Exactly-once is an optional end-to-end property only
   where source, coordinator, and sink can demonstrate the necessary atomic
   boundary; otherwise document at-least-once and idempotent effects.
3. **P1 — scalable streaming execution:** investigate bounded queues,
   backpressure, partition-aware concurrency, and schema-change handling while
   preserving declared ordering and resource limits.
4. **P1 — governance and plugin isolation:** define tenant/source permissions,
   secret handling, redacted structured audit events, retention/deletion
   boundaries, plugin ABI/capability versioning, and supply-chain evidence.
5. **P2 — operator and ecosystem usability:** expose preflight validation,
   capability discovery, restart/replay diagnostics, and supported connector
   compatibility information.

## Planned Features

- [ ] Complete v2.5.0 verification and bounded hardening through Waves 1–4,
  retaining connector-level correctness, security, and performance evidence
  (Target: v2.5.0).
- [ ] Deliver the v3.0.0 capability-driven contract and durable/scalable
  coordination only after API, migration, architecture, and release review
  (Target: v3.0.0).

## Implementation Phases

### Phase 1: Design / API Contract

- [ ] Complete the per-connector capability matrix and select the v2.5.0
  contract/coordinator boundary (Target: v2.5.0, Wave 1).
- [ ] Approve the v3.0.0 capability, delivery, ordering, and migration ADR before
  changing public API or ABI behavior (Target: v3.0.0, Wave 5).

### Phase 2: Core Implementation

- [ ] Implement only the selected v2.5.0 coordinator and connector behaviors
  supported by the reviewed compatibility matrix (Target: v2.5.0, Waves 1–2).
- [ ] Prototype durable checkpoint/commit coordination and bounded concurrency
  behind the approved v3.0.0 contract (Target: v3.0.0, Wave 6).

### Phase 3: Error Handling and Edge Cases

- [ ] Specify and test unsupported capabilities, bad rows, schema drift,
  permissions, quotas, timeout, cancellation, partial progress, and recovery
  outcomes per connector (Target: v2.5.0, Waves 2–3).
- [ ] Exercise duplicate, late/out-of-order, and crash/replay cases without
  overstating exactly-once behavior (Target: v3.0.0, Wave 6).

### Phase 4: Tests

- [ ] Repair or disposition the three excluded focused suites, run every
  maintained importer CTest target, and add service-backed restart tests
  (Target: v2.5.0, Wave 2).
- [ ] Add connector-version, compatibility, and failure-injection matrix tests
  for the redesigned major-version contract (Target: v3.0.0, Waves 5–7).

### Phase 5: Performance / Hardening

- [ ] Establish Release-profile end-to-end baselines and resource/recovery
  measurements; retain five independent runs per workload (Target: v2.5.0,
  Waves 3–4).
- [ ] Measure 1/2/4-worker throughput and tail latency with configured
  backpressure and memory limits; preserve correctness and ordering
  (Target: v3.0.0, Waves 6–7).

### Phase 6: Documentation and Acceptance

- [ ] Update capability tables, operator recovery/runbook guidance, and
  security/retention evidence from tested behavior (Target: v2.5.0, Waves 1–4).
- [ ] Publish approved API/ABI migration notes and verify root release gates
  before any breaking change is accepted (Target: v3.0.0, Waves 5–7).

## Wave Plan

The following module Waves are release-targeted engineering batches, not the
project-wide Wave A–D gates. Each estimate is an engineering-week range for
implementation plus focused validation, excluding external connector-service
provisioning and release-manager wait time.

### Wave 1 — v2.5.0 contract and capability baseline (2–3 engineering weeks)

- **Goal:** define exactly what the current connectors implement before
  extending shared guarantees.
- **Tasks:**
  - [ ] Map `IImporter`, `importers_api_contract.h`, and each selected connector to
    ordering, commit/visibility, replay, schema, cancellation, timeout, quota,
    checkpoint, and permission behavior; publish the matrix (Target: v2.5.0).
  - [ ] Decide which existing coordinator/sink can consume the versioned
    idempotency and visibility contract; specify a non-breaking implementation
    boundary and explicit unsupported outcomes (Target: v2.5.0).
  - [ ] Define source identity and retry rules for at-least-once connectors;
    do not label a path exactly-once unless restart tests prove the full source-
    to-sink boundary (Target: v2.5.0).
- **Dependencies:** current headers and implementation inventory; agreement on
  the initial supported connector set.
- **Risk:** current promises may exceed connector behavior; narrowing or
  clarifying semantics can expose compatibility differences.
- **Success metric:** 100% of selected connectors have a reviewed
  capability/contract entry; every unsupported guarantee has an explicit
  deterministic result and documented limitation.

### Wave 2 — v2.5.0 connector correctness and restart tests (3–5 engineering weeks)

- **Goal:** make live behavior measurable for the highest-use file, relational,
  and CDC/stream paths.
- **Tasks:**
  - [ ] Repair or replace the three skipped importer test suites and register
    them; run every registered importer CTest target in the intended
    test-enabled configuration (Target: v2.5.0).
  - [ ] Add opt-in integration tests for representative flat-file, relational,
    and Kafka/CDC connectors with controlled test services and retained service
    versions (Target: v2.5.0).
  - [ ] Inject process/service interruption before and after checkpoint and
    destination commit; verify replay, ordering scope, counters, and final sink
    state (Target: v2.5.0).
  - [ ] Test additive and breaking schema changes, malformed/oversized rows,
    denied permissions, timeout, cancellation, and recovery per connector
    capability (Target: v2.5.0).
- **Dependencies:** Wave 1 contract matrix; available service-backed CI or
  documented local integration profile.
- **Risk:** integration environments can be flaky or expensive; keep unit tests
  deterministic and make service tests explicit rather than silently skipped.
- **Success metric:** all maintained importer CTest targets pass; zero missing
  or duplicate applied records in the defined replay matrix; no order violation
  within a connector's declared ordering scope.

### Wave 3 — v2.5.0 bounded safety and operability (2–4 engineering weeks)

- **Goal:** bound resource use and make failures/recovery safe to operate.
- **Tasks:**
  - [ ] Test byte, row, batch, and timeout limits at boundary and over-limit
    values, including malformed input and schema growth (Target: v2.5.0).
  - [ ] Verify cancellation outcome separately from rollback/commit outcome;
    ensure callers can distinguish partial progress, retryability, and final
    visibility (Target: v2.5.0).
  - [ ] Add tests for optional `permission_check` behavior, secret/credential
    redaction, and restricted checkpoint/quarantine/diagnostic data handling
    (Target: v2.5.0).
  - [ ] Document preflight, safe retry, checkpoint retention, and recovery steps
    for selected connectors in `OPERATIONS.md` and `RUNBOOK.md` (Target: v2.5.0).
- **Dependencies:** Waves 1–2 semantics and test services.
- **Risk:** changing defaults or converting silent fallback into explicit errors
  may affect callers; prefer additive validation and migration notes.
- **Success metric:** all supported limit and permission cases have passing
  focused tests; no test fixture credential or raw sensitive row appears in
  captured diagnostic output; every retryable failure has a tested recovery
  expectation.

### Wave 4 — v2.5.0 benchmark and release evidence (2–3 engineering weeks)

- **Goal:** replace synthetic-only performance claims with repeatable,
  representative connector evidence.
- **Tasks:**
  - [ ] Add Release-profile end-to-end workloads for file parse/commit,
    relational bulk import, CDC/stream catch-up, and conflict/schema-heavy jobs
    (Target: v2.5.0).
  - [ ] Record workload size/distribution, connector/service versions, hardware,
    build flags, seed, throughput, p50/p95/p99, CPU, peak RSS, errors, and
    recovery lag; retain machine-readable run output (Target: v2.5.0).
  - [ ] Keep IMRG-01..06 labeled as microbenchmarks until threshold assertions
    and a suitable measurement method exist; establish numeric end-to-end
    thresholds only from a named baseline environment (Target: v2.5.0).
  - [ ] Add importer focused tests to the required `release_critical` path only
    after the tests are reliable and their required external services are
    available or isolated behind a separate integration gate (Target: v2.5.0).
- **Dependencies:** Waves 1–3; representative hardware and reproducible
  connector service configuration.
- **Risk:** shared-runner noise and external-service limits can make tail
  latency unstable; report environment and variance instead of treating noisy
  data as a pass.
- **Success metric:** at least five independent Release-profile runs per
  benchmark workload with retained output; report p50/p95/p99 and peak RSS; no
  accepted release comparison exceeds a 10% p95 regression against its
  same-environment baseline without an approved explanation. Do not infer p99
  from source labels or Google Benchmark aggregate output that does not measure
  it.

### Wave 5 — v3.0.0 versioned capability and API design (3–5 engineering weeks)

- **Goal:** design a coherent major-version contract without forcing false
  connector equivalence.
- **Tasks:**
  - [ ] Write an ADR for capability negotiation, delivery semantics, ordering
    scope, atomic visibility, schema evolution, checkpoint ownership, retry
    classification, and plugin ABI/version policy (Target: v3.0.0).
  - [ ] Publish compatibility and migration matrices for the existing
    `IImporter`, plugin API, and versioned contract; identify deprecated
    aliases before removal (Target: v3.0.0).
  - [ ] Specify observable behavior and errors for unsupported features and
    partial progress (Target: v3.0.0).
- **Dependencies:** v2.5.0 conformance matrix, integrations, and maintainers'
  approval of any breaking changes.
- **Risk:** a universal interface may constrain connector-specific features or
  create excess abstraction; retain extension points only where concrete
  connectors need them.
- **Success metric:** every public contract change has API review, migration
  notes, compatibility tests, and an approved ADR before implementation begins.

### Wave 6 — v3.0.0 durable coordination and scalable execution (4–8 engineering weeks)

- **Goal:** prototype and validate durable replay semantics and bounded
  multi-source throughput.
- **Tasks:**
  - [ ] Implement a coordinator prototype with durable source positions,
    idempotent sink application, and crash-safe checkpoint/commit ordering
    (Target: v3.0.0).
  - [ ] Add failure-injection tests around source acknowledgement, checkpoint
    persistence, sink commit, process restart, and duplicate/out-of-order
    events (Target: v3.0.0).
  - [ ] Prototype bounded buffering, backpressure, and partition-aware
    parallelism; preserve ordering where the declared contract requires it
    (Target: v3.0.0).
  - [ ] Compare at-least-once/idempotent execution with transactional
    exactly-once only for source/sink combinations that can demonstrate a
    common atomic boundary (Target: v3.0.0).
- **Dependencies:** Wave 5 approved contract; storage and transaction interfaces;
  cross-module architecture review.
- **Risk:** durable coordination and cross-system transactions add latency,
  state, and failure modes; exactly-once claims can be invalidated by an
  external sink outside the transaction boundary.
- **Success metric:** zero loss and zero duplicate *effects* in the documented
  crash/replay matrix; all memory/queue limits remain bounded at configured
  values; benchmark throughput and tail latency are reported for 1-, 2-, and
  4-worker configurations, with ordering tests enabled.

### Wave 7 — v3.0.0 governance, ecosystem, and release validation (3–5 engineering weeks)

- **Goal:** make the platform secure, maintainable, and supportable across
  connector versions and operating environments.
- **Tasks:**
  - [ ] Add connector/plugin compatibility manifests and CI checks for the
    supported version matrix (Target: v3.0.0).
  - [ ] Define least-privilege source/destination access, secret injection,
    tenant separation, audit redaction, retention/deletion, and provenance
    controls for durable import artifacts (Target: v3.0.0).
  - [ ] Add sustained-load, overload/backpressure, connector outage, recovery,
    and resource-exhaustion tests with operator dashboards/runbooks
    (Target: v3.0.0).
  - [ ] Complete ABI/API migration documentation and release-readiness review
    before enabling breaking changes (Target: v3.0.0).
- **Dependencies:** Waves 5–6; deployment, security, and operations owners.
- **Risk:** connector-version coverage and audit retention increase
  maintenance/storage costs; private or credentialed integrations must not
  enter Community/Minimal release lanes.
- **Success metric:** every connector advertised as supported passes its
  compatibility and security regression matrix; all release-critical importer
  suites pass on `develop`; operational artifacts cover restart, replay,
  quarantine, and credential rotation.

## Effort, risks, and open assumptions

- **Planning estimate:** v2.5.0 Waves 1–4 total 9–15 engineering weeks;
  v3.0.0 Waves 5–7 total 10–18 engineering weeks. These are engineering effort
  ranges, not calendar commitments; service/runner provisioning, review, and
  cross-module changes can extend elapsed time.
- **Trade-offs:** stronger commit visibility and durability can reduce
  throughput; parallel ingestion can conflict with source ordering; deeper
  diagnostics aid recovery but raise privacy/retention risk; broad connector
  matrices increase CI cost. Use per-connector capabilities and explicit
  opt-in integration profiles to bound these costs.
- **Open assumptions / knowledge gaps:** the supported connector/version list,
  representative production volumes and hardware, acceptable CDC lag, durable
  coordinator ownership, existing sink transaction capabilities, service-backed
  CI availability, and compliance/retention obligations have not been established
  by the checked-in evidence. Maintainers must decide these before converting
  proposed targets into release gates.
- **Release dependency:** importer waves do not override the project-wide
  `release_critical` gate, Wave 7 baseline, top-risk module sign-off,
  resilience/security/operations evidence, or human release approval in root
  `ROADMAP.md`, `RELEASE_STRATEGY.md`, and `VERSIONING.md`.

## Production Readiness Checklist

- [ ] All maintained importer tests build and pass in a test-enabled
  configuration; the three currently excluded suites are repaired/replaced or
  explicitly dispositioned with evidence (Target: v2.5.0).
- [ ] Supported live connectors pass restart/replay, ordering, schema, bounds,
  permission, cancellation, and recovery tests (Target: v2.5.0).
- [ ] Release-profile end-to-end benchmarks have retained environment/workload
  metadata and measured p50/p95/p99, throughput, CPU, peak RSS, and recovery
  lag (Target: v2.5.0).
- [ ] Operator guidance covers preflight, credentials, checkpoint/quarantine
  retention, retry, and recovery; diagnostic redaction is tested (Target: v2.5.0).
- [ ] Any v3.0.0 API/ABI break has an approved ADR, compatibility inventory,
  migration guide, and passing migration/conformance tests (Target: v3.0.0).
- [ ] Module evidence is green on the project-required `release_critical` path
  and does not substitute for the root release gates or human sign-off
  (Target: each release).

## Known Issues & Limitations

- The current checked-in test evidence is primarily unit/mock-based; no live
  connector runtime, full importer CTest run, or Release-profile end-to-end
  benchmark is recorded in `BUILD_STATUS.md`.
- Three of the twelve discovered focused test sources are excluded from local
  test target creation; the reasons are described in `tests/importers/CMakeLists.txt`.
- The versioned import contract does not currently have a coordinator consumer
  in `src/importers/`, so the mock contract tests do not establish live
  idempotency, atomic visibility, or replay.
- Connector support for checkpoints, permissions, cancellation, schema
  validation, and other shared options varies. The contract/header alone does
  not establish runtime parity.
- The HuggingFace importer currently operates on caller-provided data and
  in-memory state; do not treat it as a live external client or durable sink.
- Existing benchmark labels are targets, not measured p99 thresholds or passing
  release gates; the latest recorded standalone run used a DEBUG benchmark
  library and synthetic/in-memory work.

## Breaking Changes

No breaking importer contract is proposed for v2.5.0. The v3.0.0 capability,
coordinator, and plugin-ABI work may require breaking changes; each such change
requires an approved ADR, migration notes, compatibility tests, and alignment
with `VERSIONING.md` before implementation or release.

## Research and evidence references

- Kafka, **Message Delivery Semantics**:
  <https://docs.confluent.io/kafka/design/delivery-semantics.html>
- Debezium, **Exactly once delivery** (default at-least-once; exactly-once
  depends on Kafka Connect support and configuration):
  <https://debezium.io/documentation/reference/stable/configuration/eos.html>
- Apache Beam, **Programming Guide: Watermarks and Late Data**:
  <https://beam.apache.org/documentation/programming-guide/#watermarks-and-late-data>
- Yue Wang, Martin Boissier, Tilmann Rabl, **A Survey of Stream Processing
  System Benchmarks**, *Performance Evaluation and Benchmarking*, 2026,
  DOI: <https://doi.org/10.1007/978-3-031-93858-0_2>
- NIST SP 800-92, **Guide to Computer Security Log Management**:
  <https://csrc.nist.gov/pubs/sp/800/92/final>
- Local evidence: [`BUILD_STATUS.md`](./BUILD_STATUS.md),
  [`RESEARCH.md`](./RESEARCH.md), [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md),
  [`SECURITY.md`](./SECURITY.md), [`OPERATIONS.md`](./OPERATIONS.md),
  [`importer_interface.h`](../../include/importers/importer_interface.h),
  [`importers_api_contract.h`](../../include/importers/importers_api_contract.h),
  [`tests/importers/CMakeLists.txt`](../../tests/importers/CMakeLists.txt), and
  [`benchmarks/importers/CMakeLists.txt`](../../benchmarks/importers/CMakeLists.txt).

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Historical status metadata from 2026-08-02; superseded by the dated note above. -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · BUILD_STATUS.md -->

## Historical delivery plan and status (not revalidated)

Production importer runtime exists across relational/document/stream/file/object ingestion paths, schema/conflict/quality handling, and auditable post-processing support.

### Historical In Progress

- [x] benchmark stabilization for importer throughput and conflict-resolution hot paths (Delivered: Q3 2026)
- [x] final integration testing for Phase 2-3 hardening delivery (Delivered: Q3 2026)

### Historical Planned Features

#### Short-term (3-6 months)
- [ ] tighten deterministic behavior under high-volume multi-connector ingestion loads (Target: Q4 2026)
- [ ] extend stress coverage for mixed schema drift and conflict strategy scenarios (Target: Q4 2026)
- [ ] improve operator-facing diagnostics for connector and validation failure incidents (Target: Q4 2026)

#### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for parser/import/conflict pathways (Target: Q1 2027)
- [ ] broaden benchmark depth for CDC, stream, and quality/audit-intensive workflows (Target: Q1 2027)
- [ ] harden long-running reliability under sustained ingest pressure (Target: Q1 2027)

### Historical Implementation Phases

#### Phase 1: Design / API Contract
- [x] freeze connector/schema/conflict/audit contracts for active major line (Delivered: Q3 2026)
- [x] define explicit error taxonomy for validation, conflict, and connector capability classes (Delivered: Q3 2026)
  - Contract header: `include/importers/importers_api_contract.h`
  - Error codes: IMPORT_SCHEMA_MISMATCH, IMPORT_ROW_INVALID, IMPORT_DUPLICATE_KEY,
    IMPORT_FILE_NOT_FOUND, IMPORT_QUOTA_EXCEEDED, IMPORT_DUPLICATE_ID, IMPORT_TIMEOUT,
    IMPORT_CONNECTOR_UNAVAILABLE, IMPORT_ROLLBACK, INTERNAL_ERROR

#### Phase 2: Core Implementation
- [x] complete hardening for connector import and schema/validation internals (Delivered: Q3 2026)
  - T2.1 Part 1: PostgreSQL connector hardening (commit 01e331e7)
  - T2.1 Part 2: MySQL/Oracle/SQLite connector hardening (commit 5646694c)
  - T2.1 Part 3/4: MongoDB/Kafka/S3 connector hardening (Q3 2026)
  - T2.2: Schema inference and validation hardening (commit 6d5e118c)
- [x] align conflict/quality/audit behavior to bounded runtime contracts (Delivered: Q3 2026)
  - T2.3: Conflict resolution determinism, quality scoring bounds, audit trail integration (commit fe2f4cfd)

#### Phase 3: Error Handling and Edge Cases
- [x] standardize fail-safe behavior for unsupported connector and malformed schema scenarios (Delivered: Q3 2026)
  - T3.1.1: Connector capability fallback chain
  - T3.1.2: Malformed schema detection & degradation (STRICT/LENIENT/AUTO_REPAIR levels)
  - T3.1.3: Rollback & recovery audit trail with deterministic replay
- [x] unify diagnostics across schema, conflict, and capability failure incidents (Delivered: Q3 2026)
  - T3.2.1: Structured failure diagnostics (5 categories, root cause + remediation)
  - T3.2.2: Diagnostic aggregation & reporting with top 5 root causes
  - Commit: 8e36c2e6fe

#### Phase 4: Tests
- [x] expand focused regressions for mixed connector/schema/conflict edge scenarios (Delivered: Q3 2026)
- [x] extend deterministic stress fixtures for high-throughput import workloads (Delivered: Q3 2026)
  - Test file: `tests/importers/test_importers_contract_hardening_focused.cpp`
  - Test cases: IMCH-01..IMCH-16 (idempotency, schema evolution, error handling, large import)
  - kImportersContractSeed = 42; all tests self-contained, no external I/O

#### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for importer hot paths (Delivered: Q3 2026)
- [x] validate p95/p99 and throughput behavior against release baselines (Delivered: Q3 2026)
  - Benchmark file: `benchmarks/importers/bench_importers_release_gates.cpp`
  - Gates: IMRG-01..IMRG-06 (CSV parse ≥5M/s, schema ≤50µs, dedup ≤100µs,
    commit ≤5ms, quota ≤50µs, schema-evolution ≤200µs)
  - kImportersCanonicalSeed = 42; Repetitions(5)

#### Phase 6: Documentation and Acceptance
- [x] core importers module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] Phase 1-6 Wave 3B Category-D closure delivered (Q3 2026)
  - Contract header, 16 focused tests (IMCH-01..16), 6 release-gate benchmarks (IMRG-01..06)

### Historical readiness checklist (not a current readiness assessment)

- [x] core importer surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] remaining hardening tasks closed for connector/validation/conflict edge paths (Delivered: 2026-08-07)
- [x] release benchmark stabilization complete (Delivered: 2026-08-07)

### Historical build and test claims (not revalidated)

### Focused Test Inventory (Verified 2026-08-02)
- Test files: 6 source files in `tests/importers/test_*.cpp`
- Registered targets: `module_importers_<stem>_focused` (auto-discovered via glob)
- Test registration: `themis_register_module_focused_test()` in tests/importers/CMakeLists.txt
- IMCH test cases: 16 focused unit tests (IMCH-01..IMCH-16) in test_importers_contract_hardening_focused.cpp
  - Idempotency contract validation (IMCH-01..04)
  - Schema evolution handling (IMCH-05..08)
  - Error handling and rollback (IMCH-09..12)
  - Large import and edge cases (IMCH-13..16)
- Tier: unit, Timeout: 120s per test, Label: `importers phase4`

### Benchmark Inventory (Verified 2026-08-02)
- Benchmark file: `benchmarks/importers/bench_importers_release_gates.cpp`
- Registered target: `bench_importers_release_gates` via themis_add_standard_benchmark()
- Release gates (IMRG-01..IMRG-06):
  - IMRG-01: CSV parse ≥5M rows/s (GATE-IMRG-01)
  - IMRG-02: Schema validation p99 ≤50µs (GATE-IMRG-02)
  - IMRG-03: Dedup key check p99 ≤100µs (GATE-IMRG-03)
  - IMRG-04: Row commit p99 ≤5ms RT (GATE-IMRG-04)
  - IMRG-05: Import quota p99 ≤50µs (GATE-IMRG-05)
  - IMRG-06: Schema evolution p99 ≤200µs (GATE-IMRG-06)
- Seed: kImportersCanonicalSeed = 42; Repetitions(5), WarmupIterations(200)

### Build Configuration Status
- Preset: `community-release-allow-missing-rocksdb` (vcpkg fallback for Linux system packages)
- Test framework: GoogleTest/GTest (auto-discovery via find_package)
- Benchmark framework: google-benchmark
- Configuration verified: 2026-08-02 (cmake --preset community-release-allow-missing-rocksdb successful)
- Build targets verified: test and benchmark registration confirmed in CMake configuration

### Historical Known Issues and Limitations

- runtime behavior depends on connector availability and build/runtime feature flags.
- Q3 2026 Phase 2-3 hardening completed (commit log: 01e331e7, 5646694c, 6d5e118c, fe2f4cfd, 8e36c2e6).
  - Phase 2 T2.1: Connector pooling/timeout/fallback/error mapping (4 parts)
  - Phase 2 T2.2: Schema bounds/cycles/validation levels
  - Phase 2 T2.3: Conflict determinism/quality scoring/audit schema
  - Phase 3 T3.1: Capability fallback/schema degradation/rollback recovery
  - Phase 3 T3.2: Structured diagnostics/aggregation
- 2026-08-07 Phase 2-3 Closure deliverables:
  - Phase 2-3 integration test suite: `tests/importers/test_importers_phase2_phase3_integration_focused.cpp` (IMPI-01..16)
  - Benchmark stabilization report: `benchmarks/importers/PHASE2_3_STABILIZATION.md`
  - All six release gates (IMRG-01..06) validated and stabilized with <3% variance
  - CI gate thresholds established and reproducible across environments
- benchmark breadth should continue expanding for advanced ingest workflows.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `importers`
- [ ] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027)
- [ ] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027)
- [ ] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027)

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [ ] This module's distributed/acceleration paths fail closed (Target: Q1 2027)
- [ ] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027)
- [ ] Operator-critical paths have diagnostics, alerts, and runbooks (Target: Q1 2027)
