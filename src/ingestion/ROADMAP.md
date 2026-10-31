# Ingestion Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current planning view; code state checked 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

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
- selected connector and workflow edge scenarios need continued hardening.
- benchmark breadth should continue expanding for advanced ingestion scenarios.

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
