# Chimera Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production adapter runtime exists for the current ThemisDB adapter implementation, including simulation-mode behavior and conditional engine-dispatch integration surfaces.

## In Progress

- [~] hardening parity between simulation-mode and engine-backed dispatch paths (Target: Q3 2026)
- [~] benchmark stabilization for adapter request/response compatibility pathways (Target: Q3 2026)
- [~] diagnostics consistency improvements for capability and dispatch failure classes (Target: Q3 2026)
- [~] v1.1.0: Production ThemisDB Adapter Integration with connection pooling (Target: Q3 2026)
- [~] v1.1.0: Transaction Management with ACID properties and savepoints (Target: Q3 2026)
- [~] v1.1.0: Error Recovery with exponential backoff retry strategy (Target: Q3 2026)
- [~] v1.1.0: Batch Operation Optimization for throughput (Target: Q3 2026)
- [~] v1.2.0: MongoDB/Qdrant/Neo4j Real Driver Integration (Target: Q4 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic behavior across adapter dispatch edge permutations (Target: Q4 2026)
- [ ] expand regression coverage for engine-availability and fallback behavior (Target: Q4 2026)
- [ ] improve operator diagnostics for adapter contract and runtime mismatch incidents (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline adapter p95/p99 envelopes for release-profile compatibility paths (Target: Q1 2027)
- [ ] add dedicated chimera-native benchmark coverage for adapter operations (Target: Q1 2027)
- [ ] evaluate modular multi-vendor adapter expansion strategy within src/chimera (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [ ] freeze adapter contract semantics for active major line (Target: Q3 2026)
- [ ] define explicit error taxonomy for connection/capability/dispatch failures (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] complete hardening for lifecycle and dispatch internals (Target: Q4 2026)
- [ ] align simulation and engine-backed behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-closed behavior for invalid connection/dispatch states (Target: Q4 2026)
- [ ] unify diagnostics across capability mismatch and unsupported-path classes (Target: Q4 2026)

### Phase 4: Tests
- [ ] expand focused regressions for adapter lifecycle and dispatch edge scenarios (Target: Q4 2026)
- [ ] extend deterministic fixture coverage for engine-injection permutations (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [ ] lock benchmark-backed release gates for adapter compatibility hot paths (Target: Q4 2026)
- [ ] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core chimera module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core chimera surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [ ] remaining hardening tasks closed for dispatch parity and edge cases
- [ ] release benchmark stabilization complete

## Known Issues and Limitations

- current source layout contains the ThemisDB adapter implementation only.
- behavior parity for all engine-backed dispatch paths requires continued hardening.
- benchmark depth remains limited for chimera-native adapter pathways.

## Breaking Changes

No breaking chimera-module contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.