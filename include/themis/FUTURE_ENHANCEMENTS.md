---
Author: "ThemisDB Contributors"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Model_Attribution: "Exact model backend not verifiable from final document text alone."
AI_Contributor: "themisdb-doc-orchestrator"
Human_Review: false
Human_Author: "unknown"
---

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/themis/FUTURE_ENHANCEMENTS.md -->

# Themis Core Identity Module — Public Header Future Enhancements

**Module Path:** `include/themis/`  
**Canonical implementation enhancements:** [`../../src/themis/FUTURE_ENHANCEMENTS.md`](../../src/themis/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/themis/`. Runtime, algorithm, and benchmark work remains tracked in:

→ [`../../src/themis/FUTURE_ENHANCEMENTS.md`](../../src/themis/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Public headers must not expose internal implementation types or arena/memory layout details to callers.
- `[x]` Breaking changes to types in `include/themis/` require a MAJOR version bump per `VERSIONING.md`.
- Public API compatibility must be assessed against `VERSIONING.md`; header presence alone does not establish source or ABI stability.
- A canonical layer association for `include/themis/` was not established by this review; do not treat the former "Graph" label as verified.

---

## Required Interfaces (Header Contract)

Current public surfaces cover: build metadata, edition management, license gating, module signature verification, and export macros.

Planned extensions follow the same namespace (`themis`) and include-guard conventions.

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

- [ ] Module-interface (C++20 modules) wrapper for `include/themis/` entry points
- [ ] Header-only usage path for embedding scenarios (where feasible)
- [ ] Document layer mapping after a canonical classification is established

---

## Alignment with Strategic Plan

These are proposals, not committed delivery dates or current implementation
claims. Cross-module architectural policy belongs to the repository's
architecture governance documents; no `FUTURE_PLAN.md` source was found at the
previously implied path.
