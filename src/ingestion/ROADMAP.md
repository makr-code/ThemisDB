# Ingestion Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-18 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production ingestion runtime exists across multi-source connectors, orchestration controls, validation/quality gating, and workflow-driven extraction support.

## In Progress

- [~] hardening connector parity and deterministic fallback across mixed capability environments (Target: Q3 2026)
- [~] benchmark stabilization for ingestion throughput, extraction, and quality-judge hot paths (Target: Q3 2026)
- [~] diagnostics consistency for connector, validation, and workflow incident classes (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic behavior under high-volume mixed-source ingestion workloads (Target: Q4 2026)
- [ ] extend stress coverage for checkpoint/quarantine/retry edge scenarios (Target: Q4 2026)
- [ ] improve operator-facing diagnostics for connector/validation/quality incidents (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for ingestion control-plane operations (Target: Q1 2027)
- [ ] broaden benchmark depth for quality-judge and extraction-intensive workflows (Target: Q1 2027)
- [ ] harden long-running reliability under sustained ingest pressure (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze connector/control/quality/workflow contracts for active major line (Delivered: Q3 2026)
- [x] define explicit error taxonomy for connector, validation, and workflow failure classes (Delivered: Q3 2026)
  - Contract header: `include/ingestion/ingestion_api_contract.h`
  - Error codes: INGESTION_BUFFER_FULL, INGESTION_SCHEMA_INVALID, INGESTION_DUPLICATE_KEY,
    INGESTION_QUOTA_EXCEEDED, INGESTION_TIMEOUT, INGESTION_BATCH_ROLLBACK,
    INGESTION_BACKEND_UNAVAILABLE, INGESTION_CONNECTOR_FAILED, INTERNAL_ERROR

### Phase 2: Core Implementation
- [ ] complete hardening for connector intake and orchestration internals (Target: Q4 2026)
- [ ] align validation/quality/workflow behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-safe behavior for unsupported/degraded connector scenarios (Target: Q4 2026)
- [ ] unify diagnostics across retry/quarantine/validation/quality incidents (Target: Q4 2026)

### Phase 4: Tests
- [x] expand focused regressions for mixed connector and workflow edge scenarios (Delivered: Q3 2026)
- [x] extend deterministic stress fixtures for high-throughput ingestion operations (Delivered: Q3 2026)
  - Test file: `tests/ingestion/test_ingestion_contract_hardening_focused.cpp`
  - Test cases: INCH-01..INCH-16 (write-path, batch atomicity, schema validation, back-pressure)
  - kIngestionContractSeed = 42; all tests self-contained, no external I/O

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for ingestion hot paths (Delivered: Q3 2026)
- [x] validate p95/p99 and throughput behavior against release baselines (Delivered: Q3 2026)
  - Benchmark file: `benchmarks/ingestion/bench_ingestion_release_gates.cpp`
  - Gates: INRG-01..INRG-06 (write ≥500k/s, batch ≤5ms, schema ≤100µs,
    buffer ≤10µs, backpressure ≤50µs, quota ≤50µs)
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
- [ ] remaining hardening tasks closed for connector/control/workflow edge paths
- [ ] release benchmark stabilization complete

## Known Issues and Limitations

- runtime behavior depends on connector capability, runtime flags, and workflow configuration.
- selected connector and workflow edge scenarios need continued hardening.
- benchmark breadth should continue expanding for advanced ingestion scenarios.

## Breaking Changes

No breaking ingestion contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.