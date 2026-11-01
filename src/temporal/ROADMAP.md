# Temporal Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | source review: 2026-10-31 | historical validation: TEMPORAL_MODULE_STATUS_VALIDATION.md -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · TEMPORAL_MODULE_STATUS_VALIDATION.md -->

Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
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
