# Utils Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production-usable shared utility behavior exists for observability, privacy processing, key helpers, compression, concurrency, tracing, and general reusable support code used across ThemisDB.

## In Progress

- [~] hardening privacy and audit helper behavior for edge-case and overload scenarios (Target: Q3 2026)
- [~] tightening diagnostics consistency across shared helper failures and degradation paths (Target: Q3 2026)
- [~] aligning benchmark-backed release expectations to the broad utils hot-path surface (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] expand targeted regressions for privacy, audit, and runtime helper edge cases (Target: Q4 2026)
- [ ] improve operator-visible diagnostics for shared helper overload and fallback conditions (Target: Q4 2026)
- [ ] tighten lifecycle and documentation around crypto and key-management helper contracts (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] broaden benchmark depth for additional utility hot paths where justified by release risk (Target: Q1 2027)
- [ ] extend shared-helper resilience validation under sustained concurrent load (Target: Q1 2027)
- [ ] refine compatibility guarantees for widely consumed utility APIs (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze widely consumed helper contracts for current major line consumers (Target: Q3 2026)
- [x] define explicit error taxonomy for audit, privacy, and runtime utility failure classes (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] complete remaining hardening for shared utility hotspots with broad module fan-out (Target: Q4 2026)
- [ ] align degradation paths to predictable, module-safe contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-safe behavior across privacy, crypto, compression, and observability helpers (Target: Q4 2026)
- [ ] unify diagnostics and incident categorization for shared-helper failures (Target: Q4 2026)

### Phase 4: Tests
- [x] expand focused regressions for benchmark-mapped utility hotspots and edge scenarios (Target: Q4 2026)
- [ ] extend concurrency and stress validation for shared helper fan-out (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for mapped utility hotspots (Target: Q4 2026)
- [x] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core utils module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core shared helper surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [ ] remaining hardening tasks closed for high-fan-out helpers
- [ ] broader release benchmark stabilization complete

## Known Issues and Limitations

- the utils module covers a broad surface and requires continued curation of benchmark depth.
- some helpers depend on host libraries or external services and therefore need explicit degradation coverage.
- shared helper misuse can amplify impact across consumers if failure contracts drift.

## Breaking Changes

No breaking shared-helper contract change planned. Any consumer-visible API change requires migration notes and changelog entry before merge.