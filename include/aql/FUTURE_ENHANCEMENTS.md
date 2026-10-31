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
---

Exact model backend not verifiable from final document text alone.

<!-- Historical planning list; proposed items and target dates are not approved or current commitments. -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/aql/FUTURE_ENHANCEMENTS.md -->

# AQL (AI Query Language) Module — Public Header Future Enhancements

**Module Path:** `include/aql/`  
**Canonical implementation enhancements:** [`../../src/aql/FUTURE_ENHANCEMENTS.md`](../../src/aql/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/aql/`. Runtime, algorithm, and benchmark work remains tracked in:

→ [`../../src/aql/FUTURE_ENHANCEMENTS.md`](../../src/aql/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Public headers must not expose internal implementation types or arena/memory layout details to callers.
- `[x]` Breaking changes to types in `include/aql/` require a MAJOR version bump per `VERSIONING.md`.
- `[x]` New entry points should be additive; existing stable APIs remain source-compatible.
- `[x]` Layer association (**LLM/Graph**) must be preserved in all header expansions.

---

## Required Interfaces (Header Contract)

Current public surfaces cover: AQL query building, validation, autocomplete, LoRA fine-tuning, LLM routing, and multi-modal inference bridges.

Planned extensions follow the same namespace (`themis::aql`) and include-guard conventions.

---

## Planned Enhancements

The items below are unverified proposals from the original planning snapshot;
they are not committed API work. **TODO:** Reconcile with
[`../../src/aql/ROADMAP.md`](../../src/aql/ROADMAP.md) and current API review
before assigning delivery status or dates.

### Short-Term (Q3 2026)

- [ ] Add `[[deprecated]]` annotations to any legacy entry points with migration notes
- [ ] Add explicit `noexcept` specifications to query/read paths where safe
- [ ] Add structured diagnostic types for all thrown exceptions

### Medium-Term (Q4 2026)

- [ ] Expand public surface with convenience factory functions for common usage patterns
- [ ] Add Concepts (C++20) constraints to template parameters in public headers
- [ ] Introduce explicit ABI stability markers (`THEMIS_STABLE_ABI`) to long-term interfaces

### Long-Term (2027+)

- [ ] Module-interface (C++20 modules) wrapper for `include/aql/` entry points
- [ ] Header-only usage path for embedding scenarios (where feasible)
- [ ] Extended layer-mapping documentation for LLM/Graph integration patterns

---

## Alignment with Strategic Plan

For current module-level implementation planning, see
[`../../src/aql/FUTURE_ENHANCEMENTS.md`](../../src/aql/FUTURE_ENHANCEMENTS.md)
and [`../../src/aql/ROADMAP.md`](../../src/aql/ROADMAP.md).
