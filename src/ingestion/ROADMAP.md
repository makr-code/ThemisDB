# Ingestion Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current planning view; code and evidence checked 2026-11-06 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · RESEARCH.md -->

**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot
**Human_Review:** false
**Human_Author:** ThemisDB Team

> Exact model backend not verifiable from final document text alone.

## Current Status

Connector, manager, coordinator, validation, workflow, and sink interfaces and
implementations exist. This is not a production-readiness certification:
deployment security, persistence handoff, connector coverage, and operational
limits are caller- and path-specific.

> **Evidence rule (2026-10-31):** Dates and completion marks below are retained
> planning history unless explicitly revalidated. Current headers,
> implementations, tests, and CMake configuration override older scanner/gap
> reports. The reindex decision helper and canary helper exist; an integrated
> index migration, dual-read query path, persistent manifest lifecycle, and
> traffic routing are not established by those helpers alone.

### Maturity assessment (2026-11-06)

**Assessment: integration hardening; not a production-readiness certification.**
The module contains source adapters for filesystem, API, database, Kafka, CDC,
object storage, web crawling, and dataset workflows, plus orchestration,
validation, checkpoint, quality, and workflow code. This breadth supports
multiple ingestion scenarios, but does not establish uniform capabilities or
easy production setup: configuration, credentials, limits, and sink wiring
remain connector- and deployment-specific. Focused tests and several benchmark
families exist, but the evidence does not establish durable cross-host
recovery, end-to-end sink delivery for every execution path, production
throughput, or release-gate enforcement. In particular:

- The configured filesystem workflow branch returns assembled entities but the
  manager does not deliver them to a sink; a processed count is not evidence of
  persistence (`src/ingestion/ingestion_manager.cpp`,
  `src/ingestion/IMPLEMENTATION.md`).
- Default coordinator checkpoint and leader-election implementations are
  in-memory; durable recovery requires injected implementations
  (`include/ingestion/ingestion_coordinator.h`,
  `src/ingestion/ARCHITECTURE.md`).
- Many connector regressions inject mocked transports. The test registration
  skips `test_ingestion_error_contract_focused.cpp`, so its source is not
  passing CMake/CTest evidence (`tests/ingestion/CMakeLists.txt`).
- The release-gate benchmarks measure in-memory/mock operations. Their
  thresholds are not production capacity results or evidence of CI enforcement
  (`benchmarks/ingestion/README.md`,
  `src/ingestion/PERFORMANCE_EXPECTATIONS.md`).

The maturity label is a planning assessment, not a repository-wide formal
rating. Root `ROADMAP.md` and release governance remain authoritative for release
dates and readiness.

## v2.5.0 / v3.0.0 Research and Wave Plan

The release labels below are planning targets, not a committed release date.
The estimates are engineering person-weeks (five person-days per week); they
exclude cross-team queue time and must be re-estimated after Wave 1. Waves are
ordered by dependency. v2.5.0 prioritizes closing correctness, bounded-input,
and evidence gaps without an intentional breaking API change. v3.0.0 is the
strategic contract and distributed-operation step; any breaking change needs a
versioned migration plan. See [`RESEARCH.md`](./RESEARCH.md) for external
findings and the source-to-gap analysis. These module waves are distinct from
the program-level Wave A–D contribution model below and do not change its
release status.

### v2.5.0 — correctness and operational evidence

Planning estimate: 8–13 engineer-weeks across Waves 1–4, excluding shared-team
queue time and contingency.

#### Wave 1 — Close contract and workflow correctness gaps (P0; estimate: 2–3 engineer-weeks)
- **Goal:** A successful filesystem workflow result reaches the configured sink,
  or the run reports failure; incompatible public error declarations are
  reconciled and covered by the active test suite.
- **Tasks:** Specify the acknowledgement/commit point against the sink contract;
  wire the result to that sink; propagate sink errors and partial outcomes;
  reconcile the `IngestionErrorCode` declarations in
  `ingestion_api_contract.h`, `ingestion_error_contract.h`, and
  `ingestion_manager.h`; restore or replace the CMake-skipped
  `test_ingestion_error_contract_focused.cpp` with tests for the canonical
  contract.
- **Dependencies:** `WorkflowEngine`, `IngestionManager`, and the selected
  document/graph/vector sink API; API-owner decision on error-code
  compatibility.
- **Risk / trade-off:** Retrying after a sink accepted a write but returned an
  ambiguous error can duplicate output; preserve document identity and define
  idempotency before enabling retries. Changing a published error enum may
  affect downstream callers, so preserve or explicitly migrate its contract.
- **Success metric:** 100% of successful deterministic workflow fixtures are
  visible in the selected sink; rejected or failed writes are reported as
  failures, with no silent processed-only success; all declared error-code
  headers can be included together and the canonical focused tests are
  registered and passing.
- **Validation:** Focused contract, success, sink-failure, partial-batch, and
  replay tests; one integration test through the actual sink adapter; verify
  every result and failure count.

#### Wave 2 — Bound untrusted source intake (P1; estimate: 2–3 engineer-weeks)
- **Goal:** Remote and filesystem inputs have explicit resource and network
  boundaries before parsing or extraction.
- **Tasks:** Audit connector-specific byte, document, page, time, and redirect
  limits; provide finite production profiles; validate crawler URL schemes and
  resolved IPv4/IPv6 destinations, including after redirects; prevent private,
  loopback, link-local, and metadata-service destinations unless explicitly
  allowed by a reviewed deployment policy.
- **Dependencies:** Wave 1 error/reporting semantics; connector-specific HTTP
  transports and configuration; deployment network policy.
- **Risk / trade-off:** Tight limits can reject legitimate large documents or
  private enterprise sources; retain explicit per-source overrides rather than
  silently changing all zero/unlimited semantics.
- **Success metric:** 100% of the documented malicious URL/redirect and
  oversized-input regression corpus is rejected before fetch/parse; every
  production example selects finite limits; no secret/token appears in
  diagnostics for the tested credential-failure cases.
- **Validation:** Unit tests with injected DNS/HTTP responses (no real network),
  parser boundary tests, config compatibility tests, and a manual deployment
  policy review against the OWASP SSRF guidance cited in `RESEARCH.md`.

#### Wave 3 — Durable restart and replay behavior (P1; estimate: 2–4 engineer-weeks)
- **Goal:** Checkpoint and recovery behavior is explicit for each supported
  source/sink combination; in-memory defaults are not presented as durable.
- **Tasks:** Select a durable checkpoint implementation and record its
  ownership; define source position, replay window, and sink idempotency
  behavior; inject failures before and after checkpoint and sink commit.
- **Dependencies:** Wave 1 acknowledgement point and idempotency decision; the
  chosen source offset APIs and storage/sink durability guarantees.
- **Risk / trade-off:** Cross-system atomic commit is not available for every
  connector; use replay plus sink idempotency where a distributed transaction
  is impossible and state the delivery guarantee per connector.
- **Success metric:** For a deterministic 10,000-record fixture, all injected
  restart points produce the expected final record set with zero missing
  records and zero duplicate visible records; recovery resumes from the
  documented checkpoint boundary.
- **Validation:** Restart/fault-injection integration tests using the selected
  durable backend, plus negative tests for unavailable/corrupt checkpoints.

#### Wave 4 — Production-shaped performance and operations baseline (P1;
estimate: 2–3 engineer-weeks)
- **Goal:** Replace source-declared microbenchmark thresholds as the only
performance evidence with reproducible connector-to-sink and soak results.
- **Tasks:** Publish workload, hardware, connector, sink, concurrency, corpus,
and build metadata; measure throughput, p50/p95/p99 latency, error rate, queue
depth, and peak/steady-state memory; map alerts and recovery steps in the
runbook.
- **Dependencies:** Waves 1–3; representative source credentials and a
production-shaped sink; benchmark runner availability.
- **Risk / trade-off:** A live-source baseline is environment-specific and
cannot be compared directly to mock/in-memory cases; preserve separate
benchmark classes and do not claim portable capacity from one host.
- **Success metric:** Every release-critical path has at least three
reproducible runs on the same recorded hardware class and a checked-in or
artifact-linked baseline; matched microbenchmarks regress by no more than 10%
at p99; a 12-hour mixed-source soak has no unbounded queue/memory growth, lost
records, or unexplained failures. Treat the 10% limit as a proposed gate until
the first baseline is reviewed.
- **Validation:** Run the mapped Google Benchmark cases and connector-to-sink
integration workload; inspect artifacts and compare identical cases/builds;
exercise runbook alerts and restart recovery.

### v3.0.0 — versioned contracts and distributed operation

Planning estimate: 15–24 engineer-weeks across Waves 5–9, excluding shared-team
queue time and contingency.

#### Wave 5 — Versioned ingestion job and record contract (P1; estimate: 2–3
engineer-weeks)
- **Goal:** Define one explicit, versioned contract for record identity,
ordering, schema evolution, retryability, partial success, and acknowledgement.
- **Tasks:** Specify delivery guarantees and commit visibility; add versioned
job/record metadata including source position and provenance; define
compatibility rules and migration guidance for connector and sink adapters.
- **Dependencies:** Wave 1 sink semantics and Wave 3 replay evidence; API-owner
review.
- **Risk / trade-off:** A unified contract can obscure connector-specific
capabilities; expose capability negotiation and do not promise stronger
semantics than the selected source and sink support.
- **Success metric:** Contract tests cover every supported capability and
failure outcome; all registered connectors declare their guarantees; old
v2.x-compatible behavior remains covered by compatibility tests.
- **Validation:** Contract conformance suite for connector/sink pairs,
schema-evolution tests, generated API documentation review, and explicit
version migration tests.

#### Wave 6 — Shared durable checkpoint and replay coordination (P1; estimate:
4–6 engineer-weeks)
- **Goal:** Enable restart-safe coordination across processes for connectors
that advertise the required capabilities.
- **Tasks:** Implement shared checkpoint/lease storage, fencing against stale
workers, bounded lease recovery, duplicate suppression, and observable
checkpoint lag; keep unsupported connectors on an explicit non-distributed
path.
- **Dependencies:** Wave 3 proven persistence and Wave 5 versioned capabilities;
storage and coordination service contracts.
- **Risk / trade-off:** Shared coordination adds latency and operational
dependencies; availability and consistency settings must be chosen against
the declared delivery guarantee.
- **Success metric:** In a three-worker, 10,000-record fault-injection test,
worker loss and lease expiry result in zero missing/duplicate visible records;
recovery time and checkpoint lag meet an SLO published before implementation
sign-off.
- **Validation:** Multi-process integration and chaos tests against the selected
shared store; test split-brain fencing, lease expiry, store outage, and
checkpoint corruption.

#### Wave 7 — Tenant governance, lineage, and operational controls (P2;
estimate: 3–5 engineer-weeks)
- **Goal:** Make ingestion operationally auditable and resource-isolated for
multiple callers without treating the in-process admin API as authorization.
- **Tasks:** Define caller/service authorization boundaries; enforce
tenant-scoped quotas and source allowlists at the owning service; specify
lineage, checkpoint, quarantine, and diagnostic retention/access policies;
emit stable outcome and lag metrics with credential/PII redaction.
- **Dependencies:** Wave 5 contract metadata; owning server/auth/observability
teams and applicable compliance policy.
- **Risk / trade-off:** Auditability increases retained sensitive data and
storage cost; default to data minimization and configurable retention.
- **Success metric:** Every end-to-end job is traceable by stable job/source
identifiers; unauthorized control requests are rejected at the service
boundary; quota tests isolate tenants; secret/PII test fixtures are absent
from logs and metrics.
- **Validation:** Server-level authorization tests, tenant-isolation stress
tests, retention/redaction checks, and operational runbook review.

#### Wave 8 — Complete embedding and index-version lifecycle (P1; estimate: 3–5
engineer-weeks)
- **Goal:** Turn the existing reindex decision helper into an explicitly
verified version-migration lifecycle, or defer it from v3.0.0 with a published
capability boundary.
- **Tasks:** Decide ownership for durable manifest persistence; connect model,
dimension, chunk-profile, and schema-version changes to a reindex/migration
plan; implement dual-read comparison and traffic routing only if required by
the approved design; otherwise remove unsupported completion claims from
release-facing docs.
- **Dependencies:** Wave 5 versioned metadata contract; index/RAG owners and
durable manifest-store contract.
- **Risk / trade-off:** Dual-read and canary routing can increase storage,
query, and operational cost; the current decision/canary helpers alone do not
establish migration or live routing.
- **Success metric:** Every supported version-change trigger has a durable
manifest and tested state transition; no document with mismatched
`chunking_profile` or `embedding_model_id` is silently mixed into the same
index; rollback restores the prior serving version.
- **Validation:** Restart/durability tests for manifests, index/RAG integration
tests for dual-read parity and rollback, and a canary test with injected
promotion failure. If dual-read/routing is out of scope, assert the documented
unsupported state instead.

#### Wave 9 — Scale and v3.0 release qualification (P1; estimate: 3–5 engineer-weeks)
- **Goal:** Qualify declared connector/sink combinations under representative
load and close remaining release evidence gaps.
- **Tasks:** Run 1/2/4-worker scaling workloads over the supported source
matrix; measure end-to-end freshness, latency, throughput, failure rate, and
resource bounds; publish support matrix, SLOs, rollback procedure, and
deprecation/migration notes.
- **Dependencies:** Waves 5–8; production-shaped environments and agreed
service objectives.
- **Risk / trade-off:** External service quotas and variable source data can
dominate results; document exclusions and avoid extrapolating one connector's
performance to another.
- **Success metric:** At four workers, aggregate throughput is at least 2.0x
the one-worker result on the same workload and hardware class, with no loss or
duplicate visible records; each supported tier has a published p95/p99
freshness/latency SLO and passes a 24-hour soak with bounded memory. The
scaling threshold is a proposed target to ratify against Wave 4 evidence.
- **Validation:** Reproducible live-source-to-sink benchmark, multi-worker
fault injection, 24-hour soak, security/governance checklist, and release
sign-off against root `ROADMAP.md`.

#### Cross-wave assumptions and open questions

- The root roadmap does not commit a v2.5.0/v3.0.0 calendar or ingestion
staffing; milestone labels and person-week estimates are provisional.
- The target sink, durable checkpoint store, supported connector/capability
matrix, and production deployment network policy must be selected by owners.
- Baselines are absent for production-shaped connector-to-sink workloads; the
proposed performance gates must be reviewed after measurement, not represented
as current guarantees.
- Compliance obligations, retention periods, and tenant authorization policy
are deployment/owner decisions and must not be inferred from this module alone.

## In Progress

- [ ] TODO: revalidate connector parity and fallback behavior with current connector tests (original target: Q3 2026).
- [ ] TODO: revalidate benchmark baselines and release enforcement; benchmark source labels are not run evidence (original target: Q3 2026).
- [ ] TODO: confirm diagnostics coverage for each connector, validation, and workflow failure class (original target: Q3 2026).
  - [x] Canonical retry taxonomy bridge added in `ingestion_api_contract.h` (`toRetryTimeoutSource`, `toRetryExhaustionReason`) with focused assertions in `test_ingestion_contract_hardening_focused.cpp` (Delivered: 2026-08-09)
- [x] filesystem sources can enter the `WorkflowEngine` path when configured/eligible; that manager path does not currently deliver its result to a sink.
  - [~] extend parity path: `workflow_engine_mode=filesystem` now enables workflow-based execution for plugin/object-storage sources that provide filesystem-backed locations (Target: Q4 2026)
  - [x] source-integrity remediation for workflow routing core completed: compile-breaking token corruption in `workflow_engine.cpp` and malformed method scoping/call tokens in `ingestion_manager.cpp` removed; workflow dispatch path is source-validated again (Delivered: 2026-09-07)
- [~] **Phase D: Embedding & Index Version Governance** (planning; implementation is partial, target Q4 2026)
  - **Specification Document**: `src/ingestion/EMBEDDING_VERSION_GOVERNANCE.md` (2026-09-24 CREATED)
  - **Core Ingestion Changes**:
    - [x] Reindex decision helper detects model ID, dimension, chunking profile ID, or schema-major changes; focused tests exist.
    - [ ] TODO: dual-read query-time comparison and live traffic routing are not verified in the ingestion implementation.
    - [ ] TODO: safe migration without full re-ingestion is a proposal, not a behavior established by the helper.
  - [x] Schema file exists at `src/index/INDEX_MANIFEST_V1_SCHEMA.json`; ingestion-side persistence/loading and durability tests are not established here.
  - **Acceptance Criteria**:
    - [x] Decision helper and focused tests cover all 4 conditions (model ID, embedding dimension, chunking profile ID, schema major version); this is not automatic migration.
    - [ ] TODO: dual-read comparison and the stated bit-identical result criterion require an implemented integration and test evidence.
    - [ ] TODO: manifest persistence/durability acceptance remains unverified.
  - **CMake Feature Gate:** `THEMIS_EMBEDDING_VERSION_GOVERNANCE` was not found in current CMake configuration; do not describe it as an active switch without new evidence.
  - **CI Gate:** the workflow file exists, but its existence alone does not establish that it validates all ingestion version-governance behavior; inspect its current triggers/checks.

## Planned Features

### Short-term (3-6 months)
- [x] tighten deterministic behavior under high-volume mixed-source ingestion workloads (Delivered: Wave D 2026-Q4)
- [x] extend stress coverage for checkpoint/quarantine/retry edge scenarios (Delivered: Wave D — `tests/ingestion/test_ingestion_highcardinality_stress.cpp`)
- [x] improve operator-facing diagnostics for connector/validation/quality incidents (Delivered: Wave D — `docs/operability/RUNBOOK_INGESTION_PIPELINE.md`)

### Mid-term (6-12 months)
- [~] re-baseline p95/p99 envelopes for ingestion control-plane operations (Target: Q1 2027)
- [~] broaden benchmark depth for quality-judge and extraction-intensive workflows (Target: Q1 2027)
- [~] harden long-running reliability under sustained ingest pressure (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze connector/control/quality/workflow contracts for active major line (Delivered: Q3 2026)
- [x] define explicit error taxonomy for connector, validation, and workflow failure classes (Delivered: Q3 2026)
  - Contract header: `include/ingestion/ingestion_api_contract.h`
  - Error codes: INGESTION_BUFFER_FULL, INGESTION_SCHEMA_INVALID, INGESTION_DUPLICATE_KEY,
    INGESTION_QUOTA_EXCEEDED, INGESTION_TIMEOUT, INGESTION_BATCH_ROLLBACK,
    INGESTION_BACKEND_UNAVAILABLE, INGESTION_CONNECTOR_FAILED, INTERNAL_ERROR

### Phase 2: Core Implementation
- [~] complete hardening for connector intake and orchestration internals (Target: Q4 2026)
- [~] align validation/quality/workflow behavior to bounded runtime contracts (Target: Q4 2026)
- [x] Manager has an optional/configured filesystem profile workflow path; it does not hand the assembled result to a sink, so end-to-end processing/persistence is not complete.

### Phase 3: Error Handling and Edge Cases
- [~] standardize fail-safe behavior for unsupported/degraded connector scenarios (Target: Q4 2026)
- [~] unify diagnostics across retry/quarantine/validation/quality incidents (Target: Q4 2026)

### Phase 4: Tests
- [x] expand focused regressions for mixed connector and workflow edge scenarios (Delivered: Q3 2026)
- [x] extend deterministic stress fixtures for high-throughput ingestion operations (Delivered: Q3 2026)
  - Test file: `tests/ingestion/test_ingestion_contract_hardening_focused.cpp`
  - Test cases: INCH-01..INCH-16 (write-path, batch atomicity, schema validation, back-pressure)
  - kIngestionContractSeed = 42; all tests self-contained, no external I/O

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for ingestion hot paths (Delivered: Q3 2026)
- [ ] TODO: validate p95/p99 and throughput against a recorded release baseline on representative hardware (original completion mark requires current evidence).
  - Benchmark file: `benchmarks/ingestion/bench_ingestion_release_gates.cpp`
  - INRG-01..INRG-06 have source-declared thresholds. The cases use mock/in-memory operations; this roadmap has no current run/baseline evidence proving production performance or release-gate enforcement.
  - kIngestionCanonicalSeed = 42; Repetitions(5)

### Phase 6: Documentation and Acceptance
- [x] core ingestion module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] Phase 1-6 Wave 3B Category-D closure delivered (Q3 2026)
  - Contract header, 16 focused tests (INCH-01..16), 6 release-gate benchmarks (INRG-01..06)

## Production Readiness Checklist

- [x] core ingestion surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [~] remaining hardening tasks closed for connector/control/workflow edge paths
- [~] release benchmark stabilization complete

## Known Issues and Limitations

- runtime behavior depends on connector capability, runtime flags, and workflow configuration.
- the filesystem workflow path does not currently deliver its result to a sink;
  in-memory coordinator state is not durable restart evidence.
- connector tests and source-declared benchmark thresholds do not establish live
  connector-to-sink capacity or cross-connector parity.
- zero-valued limits are unlimited for some connectors; production profiles
  must configure finite bounds for untrusted or unbounded sources.
- the v2.5.0/v3.0.0 waves depend on sink, checkpoint, deployment security, and
  compliance decisions that are not owned by this module.

## Breaking Changes

No breaking ingestion contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `ingestion`
- [x] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Delivered: 2026-09-16 — `tests/ingestion/test_ingestion_highcardinality_stress.cpp`, IN-BM-01..04 benchmark gates)
- [x] Contribute to or validate long-duration soak test coverage for this module's primary paths (Delivered: 2026-09-16 — `tests/integration/test_ingestion_pipeline_soak.cpp`)
- [x] Ensure runbook coverage for operator-critical scenarios in this module (Delivered: 2026-09-16 — `docs/operability/RUNBOOK_INGESTION_PIPELINE.md`)

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [ ] TODO: verify fail-closed behavior for distributed/acceleration paths against their active implementations and tests.
- [ ] TODO: no representative-hardware IN-BM run/baseline is linked here; benchmark source thresholds are not measured evidence.
- [ ] Runbook source exists, but deployment alerting/diagnostics integration must be verified outside this module.
