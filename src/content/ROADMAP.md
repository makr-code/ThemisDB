# Content Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-29 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production content runtime exists across ingestion orchestration, multi-format extraction, policy/security validation, enrichment (OCR/LLM/embedding), and deduplication/operations surfaces. **Batch 5 Finalization** (CMT-7500–7599): 100 documentation and quality gate items for v2.4.0 GA closure (2026-08-15).

## In Progress

- [~] Batch 5 Finalization: GA promotion sign-off via CMT-7504/7505/7506 documentation/test/governance items (Target: Sept 22, 2026)
- [~] hardening processor dependency edge behavior and failure parity across formats (Target: Q3 2026)
- [~] benchmark stabilization for extraction and concurrent ingestion pathways (Target: Q3 2026)
- [~] diagnostics consistency improvements for validation/security and async-ingestion failures (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic behavior for mixed-format and large-payload edge permutations (Target: Q4 2026)
- [ ] expand regressions for async queue pressure and processor fallback paths (Target: Q4 2026)
- [ ] improve operator diagnostics for ingestion quality and processor degradation incidents (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for extraction and concurrent throughput profiles (Target: Q1 2027)
- [ ] add dedicated benchmark coverage for additional processor families and pipeline stages (Target: Q1 2027)
- [ ] harden long-running ingestion stability under sustained mixed-media workloads (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze ingestion/extraction/validation contracts for active major line (Target: Q3 2026)
- [x] define explicit error taxonomy for policy/security and processor failure classes (Target: Q3 2026)

### Phase 2: Core Implementation
- [~] complete hardening for manager, processor routing, and async worker internals (Target: Q4 2026)
  - 2026-07-29: `ContentManager::searchContentHybrid()` now uses the same canonical fulltext table/column contract as ingestion (`chunk.text`) and applies one shared whitelist path for vector and fulltext lanes.
- [~] align enrichment and deduplication behavior to bounded runtime contracts (Target: Q4 2026)
  - 2026-07-29: `buildChunkWhitelist()` hardened to fail closed for malformed/empty filter constraints and unknown custom keys; scalar + array filter forms and created-at bounds now normalize into one bounded contract path.

### Phase 3: Error Handling and Edge Cases
- [~] standardize fail-closed behavior for invalid payload and policy-violation scenarios (Target: Q4 2026)
  - 2026-07-29: malformed search filters (`category` object, empty/invalid selector sets) are now rejected via fail-closed whitelist evaluation instead of silently widening result sets.
- [~] unify diagnostics across extraction, enrichment, and fallback failure paths (Target: Q4 2026)
  - 2026-07-29: hybrid search no longer uses divergent ad-hoc fulltext filter parsing; filter handling is unified through one bounded whitelist contract for both retrieval paths.

### Phase 4: Tests
- [x] expand focused regressions for format-specific and async-pressure edge scenarios (Target: Q4 2026)
- [x] extend deterministic fixture coverage for processor dependency permutation matrixes (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for content extraction hot paths (Target: Q4 2026)
- [x] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core content module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

### Phase 6B: Batch 5 — GA Documentation & Quality Gates (2026-08-15)
- [ ] CMT-7504: Module Documentation Linkset Synchronization (Consistency)
  - [ ] CMT-7504-01: Update ROADMAP.md with processor inventory (44 files)
  - [ ] CMT-7504-02: Update FUTURE_ENHANCEMENTS.md with deferred features from CMT-7502 TODO scan
  - [ ] CMT-7504-03: Cross-check phase status in all 4 docs (must be consistent)
  - [ ] CMT-7504-04: Add automated linkset validation in CI (broken anchor detection)
- [ ] CMT-7505: Test Coverage Correlation (Production Readiness)
  - [ ] CMT-7505-01: Aggregate all Batch 1-4 remediation items (CRITICAL 48 + HIGH 402)
  - [ ] CMT-7505-02: For each fix, verify corresponding test in tests/content/ exists
  - [ ] CMT-7505-03: Run `ctest --preset community-release -L content` to validate
  - [ ] CMT-7505-04: Generate test coverage report showing gap-to-test mapping
- [ ] CMT-7506: GA Promotion Sign-Off (Final Gate)
  - [ ] Document checklist at docs/governance/GA_PROMOTION_SIGN_OFF.md § Content Module (Batch 5)
  - [ ] Track completion status of pre-requisite items (Batches 1-4, CMT-7500–7503)

## Production Readiness Checklist

- [x] core content surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [~] remaining hardening tasks closed for processor and async edge cases
- [x] release benchmark stabilization complete

## Known Issues and Limitations

- behavior remains partially capability-dependent on enabled optional processors.
- selected async and processor-degradation edges require ongoing hardening.
- benchmark depth should be expanded beyond current extraction-focused mappings.

## Breaking Changes

No breaking content-module contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `content`
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
