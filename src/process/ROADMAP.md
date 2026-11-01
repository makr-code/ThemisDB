---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-11-01
Status: review
Source_Level: level 1
SOT_Domain: module-behavior
Canonical_Sources: DOCUMENTATION_GOVERNANCE.md; include/process/*.h; src/process/*.cpp; tests/process/CMakeLists.txt; benchmarks/process/CMakeLists.txt; ROADMAP.md
Related_Issue: "#7368"
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

# Process Module Roadmap

**AI provenance:** Exact model backend not verifiable from final document text alone.

This roadmap is an evidence-aware planning index, not a release approval. Earlier versions contained fixed test/benchmark totals, pass claims, and readiness assertions that do not follow from the current source tree. No current completion percentage or release status is asserted here.

## Current Status

Current code includes process-model management and format handlers, linking and retrieval components, external orchestration profiles, and additional federation, conflict-resolution, audit, diagnostics, and telemetry surfaces. The public headers and implementation files are the authoritative behavior references; their presence does not establish that planned guarantees are complete or production-ready.

## In Progress

- [~] Reconcile public contracts and module documentation with implementation behavior; keep unsupported guarantees explicitly marked (Target: TBD).

## Planned Features

- [ ] Enforce or remove the declared `ProcessLightRetriever` resource budgets, with tests for timeout, size, and traversal limits (Target: TBD).
- [ ] Define and test recovery behavior for partial orchestration-profile state updates (Target: TBD).
- [ ] Verify audit persistence, delta encoding, and telemetry transport against their public contracts before claiming those guarantees (Target: TBD).
- [ ] Establish reproducible build, test, and benchmark evidence before publishing module readiness or performance status (Target: TBD).

## Implementation Phases

### Phase 1: Design / API Contract
- [x] Check current process API descriptions against the public headers; keep the headers authoritative for signatures and defaults.

### Phase 2: Core Implementation
- [ ] Implement the declared light-retrieval budgets or narrow the API contract to match actual behavior (Target: TBD).

### Phase 3: Error Handling & Edge Cases
- [ ] Specify and verify partial-write and failure recovery for orchestration profile activation and rollback (Target: TBD).

### Phase 4: Tests
- [ ] Add or identify focused tests for retrieval-budget enforcement and partial profile updates; record actual results (Target: TBD).

### Phase 5: Performance / Hardening
- [ ] Map benchmark cases to explicit Process thresholds and capture a reproducible baseline; the four PM-BM source-comment targets are not enforced by the current comparator (Target: TBD).

### Phase 6: Documentation & Acceptance
- [x] Audit the in-scope process Markdown against current source and CMake registration.
- [ ] Obtain maintainer review of the remaining TODOs and any release-readiness statement (Target: TBD).

## Planning and evidence gaps

| Area | Source evidence | Status in this roadmap |
|---|---|---|
| Core API and behavior | [`API.md`](API.md), [`IMPLEMENTATION.md`](IMPLEMENTATION.md), public headers and implementation | Documented; no overall maturity claim |
| Tests | [`tests/process/CMakeLists.txt`](../../tests/process/CMakeLists.txt) registers tests through source globs | Registration verified; build/pass status TODO |
| Benchmarks | [`benchmarks/process/CMakeLists.txt`](../../benchmarks/process/CMakeLists.txt), `src/process/PERFORMANCE_EXPECTATIONS.md`, and `benchmarks/compare_expectations.py` | 12 targets/88 macro declarations; zero Process performance expectation rows; separate release coverage audit 14/14; current performance baseline/pass status not established |
| Federation, audit, telemetry | Contracts and implementation files exist; implementations contain explicit placeholders/simplifications | Feature-by-feature verification and maturity assessment TODO |
| Security and operations | See [security notes](SECURITY.md), [trust boundaries](TRUST.md), and [runbook](RUNBOOK.md) | Deployment-specific controls and release evidence TODO |

## Production Readiness Checklist

- [x] Document current source-visible behavior and known implementation caveats.
- [ ] Record a reproducible focused build/test run and exact configuration; registration is not a pass.
- [ ] Validate benchmark thresholds and baseline provenance against active code and raw output.
- [ ] Verify deployment security, observability, and recovery controls in their owning components.
- [ ] Establish any release/maturity status from the root [`ROADMAP.md`](../../ROADMAP.md) and current release evidence before publishing it here.

## Known Issues & Limitations

- `ProcessLightRetriever::retrieve()` does not enforce the resource budgets declared in its header.
- Profile activation and rollback use multiple writes and provide no transaction guarantee.
- Audit persistence/delta encoding and telemetry export are simplified; their design guarantees are not verified.
- Federation source comments identify alpha/core implementation status; this does not establish production readiness.
- The process benchmark target and macro-registration counts do not define a performance-gate total. The comparator currently has zero Process expectation rows; four numeric PM-BM source comments are not enforced. The release coverage audit's 14/14 result is not a performance pass. No current performance baseline or pass result is established; see [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md).

## Breaking Changes

No public API or runtime behavior changes are part of this documentation update.

## Related documentation

[Module overview](README.md) · [Architecture](ARCHITECTURE.md) · [Future enhancements](FUTURE_ENHANCEMENTS.md) · [Performance expectations](PERFORMANCE_EXPECTATIONS.md) · [Production requirements](PRODUCTION_REQUIREMENTS.md) · [Governance](../../DOCUMENTATION_GOVERNANCE.md)
