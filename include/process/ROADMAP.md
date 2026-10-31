---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
Source_Level: level 1
SOT_Domain: api-contract
Canonical_Sources: include/process/*.h; src/process/ROADMAP.md; src/process/API.md; DOCUMENTATION_GOVERNANCE.md
Related_Issue: "#7368"
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

# Process Public API Roadmap

**AI provenance:** Exact model backend not verifiable from final document text alone.

This page is a planning index for public-contract work. It does not claim that every header is stable, complete, compatible across platforms, or production-ready. The current public surface is defined by headers in this directory; the matching implementation and tests must be checked for behavior.

## Current Status

The public headers expose process model, format, retrieval, linking, diagnostics, federation, audit, and telemetry interfaces. Header presence is not evidence that every declared guarantee is implemented or validated.

## In Progress

- [~] Keep public API descriptions aligned with declarations and implementation behavior (Target: TBD).

## Planned Features

- [ ] Reconcile the `ProcessLightRetriever` resource-budget declarations with its current implementation (Target: TBD).
- [ ] Review compatibility and deprecation policy against the current [`VERSIONING.md`](../../VERSIONING.md) before proposing a public API change (Target: TBD).
- [ ] Add examples only after compiling them against current headers and checking failure handling (Target: TBD).
- [ ] Establish public-header compile coverage from active CI configuration; current platform coverage is not verified here (Target: TBD).

## Implementation Phases

### Phase 1: Design / API Contract
- [x] Review the public interface map against current process headers.

### Phase 2: Core Implementation
- [ ] Reconcile declarations for retrieval budgets, audit, telemetry, and federation with their actual implementations (Target: TBD).

### Phase 3: Error Handling & Edge Cases
- [ ] Document failure and partial-update behavior for public process APIs (Target: TBD).

### Phase 4: Tests
- [ ] Verify focused tests and public-header compile checks for changed contracts (Target: TBD).

### Phase 5: Performance / Hardening
- [ ] Publish only performance or compatibility claims backed by current source and reproducible evidence (Target: TBD).

### Phase 6: Documentation & Acceptance
- [x] Preserve links to the module API guide and implementation roadmap.
- [ ] Obtain maintainer review before treating candidate items as commitments (Target: TBD).

## Production Readiness Checklist

- [x] Public API docs identify headers as authoritative.
- [ ] Verify runtime behavior and test results for each claimed public contract.
- [ ] Verify ABI and platform compatibility through current CI evidence.

## Known Issues & Limitations

- The light-retriever resource budgets are declared in the public header but are not enforced by the current `retrieve()` implementation.
- Orchestration profile activation and rollback are not transactional.
- Federation, audit, and telemetry interfaces do not establish production-ready implementations.

## Breaking Changes

No public API changes are part of this documentation update. Evaluate compatibility under the repository [`VERSIONING.md`](../../VERSIONING.md) before changing declarations.

## Canonical references

- [Public-header architecture](ARCHITECTURE.md) · [API index](README.md)
- [Implementation roadmap](../../src/process/ROADMAP.md) · [API guide](../../src/process/API.md)
- [Versioning policy](../../VERSIONING.md)
