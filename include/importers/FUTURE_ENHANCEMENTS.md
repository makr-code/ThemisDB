---
Author: "ThemisDB Team"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "api-contract"
Canonical_References: "include/importers/*.h; include/importers/*.hpp; VERSIONING.md; src/importers/FUTURE_ENHANCEMENTS.md"
Related_Reference: "makr-code/ThemisDB#7353"
---

> Creation-date note: Original creation date is not recorded in the available checkout; the front-matter `Created` date is a metadata placeholder, not independently verified.

> Exact model backend not verifiable from final document text alone.

<!-- Planning notes only; not a committed delivery schedule. -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/importers/FUTURE_ENHANCEMENTS.md -->

# Importers Module — Public Header Future Enhancements

**Module Path:** `include/importers/`  
**Canonical implementation enhancements:** [`../../src/importers/FUTURE_ENHANCEMENTS.md`](../../src/importers/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/importers/`. Runtime, algorithm, and benchmark work remains tracked in:

→ [`../../src/importers/FUTURE_ENHANCEMENTS.md`](../../src/importers/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Public headers must not expose internal implementation types or arena/memory layout details to callers.
- `[x]` Breaking changes to types in `include/importers/` require a MAJOR version bump per `VERSIONING.md`.
- `[x]` New entry points should be additive; existing stable APIs remain source-compatible.

---

## Required Interfaces (Header Contract)

Current declarations span source connectors, shared import options, schema and
conflict helpers, plugins, and specialized import paths. Consult the current
headers for the actual surface; header presence does not establish runtime
availability.

Planned extensions follow the same namespace (`themis::importers`) and include-guard conventions.

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

- [ ] Module-interface (C++20 modules) wrapper for `include/importers/` entry points
- [ ] Header-only usage path for embedding scenarios (where feasible)
- [ ] Extended layer-mapping documentation for ANN/Tensor integration patterns

---

## Alignment with Strategic Plan

TODO: review proposed additions against current headers, module roadmap, and
root release policy before treating them as planned commitments. For related
AI work see [`../../src/ai/FUTURE_ENHANCEMENTS.md`](../../src/ai/FUTURE_ENHANCEMENTS.md).
