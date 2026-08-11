# Config Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production config runtime exists for path resolution, schema validation, config observability, watcher signaling, and encrypted-store integration surfaces.

## In Progress

- [x] hardening resolver/validator edge-case consistency under complex config sets (Target: Q3 2026)
- [x] benchmark stabilization for config resolution and update-serialization hot paths (Target: Q3 2026)
- [x] diagnostics consistency improvements across audit/watcher/encrypted-store failures (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic behavior for path mapping and validation race-edge scenarios (Target: Q4 2026)
- [ ] expand regressions for file-watcher churn and resolver fallback permutations (Target: Q4 2026)
- [ ] improve operator diagnostics for config validation and secure-store incidents (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline config p95/p99 envelopes for release-profile resolver/validator pathways (Target: Q1 2027)
- [ ] add dedicated benchmark coverage beyond current resolver-focused measurements (Target: Q1 2027)
- [ ] harden long-running watcher and encrypted-store rotation behavior under sustained load (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze resolver/validator/secure-store contracts for active major line (Target: Q3 2026)
  - Delivered: `include/config/config_contract.h` — frozen v1.x contract (size constraints, temporal bounds,
    failure classes, fail-closed semantics, resolver/validator contract, watcher contract, encrypted-store contract)
- [x] define explicit error taxonomy for mapping/validation/watcher/store failure classes (Target: Q3 2026)
  - Delivered: failure classes registered in `config_contract.h` with isFailClosedClass() predicate

### Phase 2: Core Implementation
- [x] complete hardening for resolution, validation, and watcher internals (Target: Q4 2026)
- [x] align encrypted-store and metrics/audit behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [x] standardize fail-closed behavior for malformed config and invalid mapping states (Target: Q4 2026)
- [x] unify diagnostics across resolver/validator/watcher/store failure paths (Target: Q4 2026)

### Phase 4: Tests
- [x] expand focused regressions for schema, resolver fallback, and watcher race scenarios (Target: Q4 2026)
  - Delivered: `tests/config/test_config_hardening_resolver_validator.cpp`
    CFG-01..08 (resolver edge cases: oversized paths, missing paths, circular fallbacks, special characters)
  - Delivered: `tests/config/test_config_hardening_resolver_validator.cpp`
    CFG-09..16 (validator edge cases: oversized schemas, circular refs, nesting depth, external $ref prevention)
- [x] extend deterministic fixture coverage for secure-store and metrics/audit permutations (Target: Q4 2026)
  - Delivered: `tests/config/test_config_hardening_watcher_store.cpp`
    CFG-17..24 (watcher edge cases: polling bounds, modification detection, file deletion, operation timeout)
  - Delivered: `tests/config/test_config_hardening_watcher_store.cpp`
    CFG-25..32 (encrypted-store edge cases: algorithm verification, auth-tag length, key rotation, metadata validation)

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for config hot paths (Target: Q4 2026)
  - Delivered: `benchmarks/config/bench_config_release_gates.cpp` — GATE-CFG-01..06
    (resolve p99 ≤ 1 µs, validate p99 ≤ 500 µs, encrypted-store get/put p99 ≤ 1 ms,
     watcher poll within bounds, metrics < 5% overhead)
- [x] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)
  - Gate thresholds documented in benchmark file and config_contract.h

### Phase 6: Documentation and Acceptance
- [x] core config module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] frozen contract header with all runtime semantics documented
  - `include/config/config_contract.h` § 1-8 with size constraints, temporal bounds, failure classes, 
    fail-closed contracts, validator bounds, watcher availability, encrypted-store consistency,
    audit/observability contracts

## Production Readiness Checklist

- [x] core config surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] remaining hardening tasks closed for resolver/validator/watcher/store edges
  - Phase 1-6 complete with frozen contract, comprehensive tests, and release gates
- [x] release benchmark stabilization complete
  - GATE-CFG-01..06 documented and measured

## Known Issues and Limitations

- benchmark coverage remains narrow and resolver-centric in current mapping.
- selected watcher and secure-store long-running edge profiles require ongoing hardening.
- behavior remains partially capability-dependent on enabled runtime integrations.

## Breaking Changes

No breaking config-module contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `config`
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
