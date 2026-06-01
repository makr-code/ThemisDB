# Importers Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production importer runtime exists across relational/document/stream/file/object ingestion paths, schema/conflict/quality handling, and auditable post-processing support.

## In Progress

- [~] hardening connector parity and fallback determinism across mixed runtime capability profiles (Target: Q3 2026)
- [~] benchmark stabilization for importer throughput and conflict-resolution hot paths (Target: Q3 2026)
- [~] diagnostics consistency for schema/conflict/connector denial incidents (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic behavior under high-volume multi-connector ingestion loads (Target: Q4 2026)
- [ ] extend stress coverage for mixed schema drift and conflict strategy scenarios (Target: Q4 2026)
- [ ] improve operator-facing diagnostics for connector and validation failure incidents (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for parser/import/conflict pathways (Target: Q1 2027)
- [ ] broaden benchmark depth for CDC, stream, and quality/audit-intensive workflows (Target: Q1 2027)
- [ ] harden long-running reliability under sustained ingest pressure (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [ ] freeze connector/schema/conflict/audit contracts for active major line (Target: Q3 2026)
- [ ] define explicit error taxonomy for validation, conflict, and connector capability classes (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] complete hardening for connector import and schema/validation internals (Target: Q4 2026)
- [ ] align conflict/quality/audit behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-safe behavior for unsupported connector and malformed schema scenarios (Target: Q4 2026)
- [ ] unify diagnostics across schema, conflict, and capability failure incidents (Target: Q4 2026)

### Phase 4: Tests
- [ ] expand focused regressions for mixed connector/schema/conflict edge scenarios (Target: Q4 2026)
- [ ] extend deterministic stress fixtures for high-throughput import workloads (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [ ] lock benchmark-backed release gates for importer hot paths (Target: Q4 2026)
- [ ] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core importers module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core importer surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [ ] remaining hardening tasks closed for connector/validation/conflict edge paths
- [ ] release benchmark stabilization complete

## Known Issues and Limitations

- runtime behavior depends on connector availability and build/runtime feature flags.
- selected connector and conflict edge scenarios need continued hardening.
- benchmark breadth should continue expanding for advanced ingest workflows.

## Breaking Changes

No breaking importer contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.