---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: ThemisDB documentation orchestration specialist
Human_Review: false
Human_Author: ThemisDB Contributors
Source level: level1
SOT domain: api-contract
Canonical source references:
  - ../../VERSIONING.md
  - ../../src/graph/FUTURE_ENHANCEMENTS.md
  - ./
Related milestone: DOC-WEEKLY-2026-44
AI_Model_Note: Exact model backend not verifiable from final document text alone.
---

> **Build:** this is a planning document; use [`tests/graph/README.md`](../../tests/graph/README.md)
> for test-enabled configure/build commands.

<!-- Planning snapshot dated 2026-06-01; target dates below are not current commitments. -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/graph/FUTURE_ENHANCEMENTS.md -->

# Graph Module — Public Header Future Enhancements

> The dated checkboxes and compatibility statements below are historical proposals. They are
> not approved API/ABI commitments. The inspected graph headers and root
> [`VERSIONING.md`](../../VERSIONING.md) define current declarations and release policy.

**Module Path:** `include/graph/`  
**Canonical implementation enhancements:** [`../../src/graph/FUTURE_ENHANCEMENTS.md`](../../src/graph/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/graph/`. Runtime, algorithm, and benchmark work remains tracked in:

→ [`../../src/graph/FUTURE_ENHANCEMENTS.md`](../../src/graph/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Public headers must not expose internal implementation types or arena/memory layout details to callers.
- `[x]` Compatibility changes must follow repository `VERSIONING.md`; do not infer a major-bump rule from this historic checklist alone.
- `[x]` Additive entry points are a design preference, not a guarantee that every existing header has a stable compatibility contract.
- `[x]` Layer association (**Graph Truth Layer**) must be preserved in all header expansions.

---

## Required Interfaces (Header Contract)

Current public surfaces cover: graph traversal, embedding, knowledge-graph reasoning, ontology management, tensor deduplication, and query optimisation.

Planned extensions follow the same namespace (`themis::graph`) and include-guard conventions.

---

## Planned Enhancements

### Short-Term (Q3 2026)

- [ ] Add `[[deprecated]]` annotations to any legacy entry points with migration notes
- [ ] Add explicit `noexcept` specifications to query/read paths where safe
- [ ] Add structured diagnostic types for all thrown exceptions

### Medium-Term (Q4 2026)

- [ ] Expand public surface with convenience factory functions for common usage patterns
- [ ] Add Concepts (C++20) constraints to template parameters in public headers
- [ ] Introduce explicit ABI stability markers (`THEMIS_STABLE_ABI`) to long-term interfaces

### Long-Term (2027+)

- [ ] Module-interface (C++20 modules) wrapper for `include/graph/` entry points
- [ ] Header-only usage path for embedding scenarios (where feasible)
- [ ] Extended layer-mapping documentation for Graph Truth Layer integration patterns

---

## Alignment with Strategic Plan

Enhancements should be reviewed against the current
[`src/graph/FUTURE_ENHANCEMENTS.md`](../../src/graph/FUTURE_ENHANCEMENTS.md) and root roadmap.
No current approval or milestone is implied by the historic dates above.
