---
Author: "ThemisDB Team"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Planning snapshot last source-checked: 2026-06-01; short-term Q3 2026 targets have elapsed. -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/index/FUTURE_ENHANCEMENTS.md -->

# Index Module — Public Header Future Enhancements

**Module Path:** `include/index/`  
**Canonical implementation enhancements:** [`../../src/index/FUTURE_ENHANCEMENTS.md`](../../src/index/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/index/`. Runtime, algorithm, and benchmark work remains tracked in:

→ [`../../src/index/FUTURE_ENHANCEMENTS.md`](../../src/index/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Public headers must not expose internal implementation types or arena/memory layout details to callers.
- `[x]` Breaking changes to types in `include/index/` require a MAJOR version bump per `VERSIONING.md`.
- `[x]` New entry points should be additive; existing stable APIs remain source-compatible.
- `[x]` Layer association (**ANN Frontdoor**) must be preserved in all header expansions.

---

## Required Interfaces (Header Contract)

Current public surfaces cover: vector indexes (HNSW, ANN, GPU), graph indexes, secondary/spatial/temporal indexes, product quantisation, learned indexes, and multi-GPU support.

Planned extensions follow the same namespace (`themis::index`) and include-guard conventions.

---

## Planned Enhancements

### Short-Term (Q3 2026)

> These milestones have elapsed and have not been rebaselined; the bullets are
> historical planning items, not current delivery commitments.

- [ ] Add `[[deprecated]]` annotations to any legacy entry points with migration notes
- [ ] Add explicit `noexcept` specifications to query/read paths where safe
- [ ] Add structured diagnostic types for all thrown exceptions

### Medium-Term (Q4 2026)

- [ ] Expand public surface with convenience factory functions for common usage patterns
- [ ] Add Concepts (C++20) constraints to template parameters in public headers
- [ ] Introduce explicit ABI stability markers (`THEMIS_STABLE_ABI`) to long-term interfaces

### Long-Term (2027+)

- [ ] Module-interface (C++20 modules) wrapper for `include/index/` entry points
- [ ] Header-only usage path for embedding scenarios (where feasible)
- [ ] Extended layer-mapping documentation for ANN Frontdoor integration patterns

---

## Alignment with Module Planning

Runtime and algorithm enhancements are tracked in
[`../../src/index/FUTURE_ENHANCEMENTS.md`](../../src/index/FUTURE_ENHANCEMENTS.md)
and [`../../src/index/ROADMAP.md`](../../src/index/ROADMAP.md). No separate
`FUTURE_PLAN.md` source is referenced by this module.
