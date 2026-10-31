---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source level: "level1"
SOT domain: "api-contract"
Canonical source references: "src/cdc/ROADMAP.md; include/cdc/README.md; include/cdc/cdc_delivery_contract.h"
Related milestone: "DOC-WEEKLY-2026-44"
---

# CDC public API roadmap

The public header inventory and contract notes are maintained in
[`README.md`](./README.md) and
[`cdc_delivery_contract.h`](./cdc_delivery_contract.h). This page does not
duplicate runtime implementation milestones.

## Current Status

The public CDC headers are indexed in [`README.md`](./README.md). Their
declarations do not establish that an optional backend is enabled or that
runtime release gates have passed.

## Planned Features

- [ ] Keep public header descriptions and build-guard notes aligned with the declarations when the API changes (Target: ongoing)

## Implementation Phases

### Phase 1: Design / API Contract
- [ ] Check proposed public signatures and behavior against `cdc_delivery_contract.h` before implementation (Target: each CDC API change)

### Phase 2: Core Implementation
- [ ] Update the owning implementation and public declaration together for an approved API change (Target: each CDC API change)

### Phase 3: Error Handling and Edge Cases
- [ ] Document feature guards, invalid-input behavior, and failure semantics from the implementation (Target: each CDC API change)

### Phase 4: Tests
- [ ] Add or update a focused test for each changed public contract (Target: each CDC API change)

### Phase 5: Performance and Hardening
- [ ] Validate performance impact when an API change affects a measured hot path (Target: each performance-sensitive change)

### Phase 6: Documentation and Acceptance
- [ ] Update this header index and linked API documentation after public surface changes (Target: each CDC API change)

## Production Readiness Checklist

- [x] Public header inventory and core build guards are documented
- [ ] Runtime readiness and release-gate evidence are maintained in the canonical implementation roadmap

## Known Issues & Limitations

- Header declarations and documentation do not prove that a conditional transport is enabled in a configured build.
- Runtime status and open implementation work belong to [`src/cdc/ROADMAP.md`](../../src/cdc/ROADMAP.md).

## Breaking Changes

Breaking public API changes require migration notes and a changelog entry under the repository versioning policy.

For current module status, implementation phases, open hardening work, and
production-readiness tracking, see the canonical
[`src/cdc/ROADMAP.md`](../../src/cdc/ROADMAP.md). Do not interpret this
header-level page as evidence that the implementation roadmap's open work or
release gates are complete.

**Exact model backend not verifiable from final document text alone.**
