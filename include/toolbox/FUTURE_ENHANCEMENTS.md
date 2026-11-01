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

> **Planning note:** The target quarters below are historical proposals, not current commitments. No feature or schedule is promised by this list; reconcile it against current governance before planning work.
>
> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: proposal list | reviewed for stale target dates: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/toolbox/FUTURE_ENHANCEMENTS.md -->

# Toolbox Module — Public Header Future Enhancements

**Module Path:** `include/toolbox/`  
**Canonical implementation enhancements:** [`../../src/toolbox/FUTURE_ENHANCEMENTS.md`](../../src/toolbox/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/toolbox/`. Runtime, algorithm, and benchmark work remains tracked in:

→ [`../../src/toolbox/FUTURE_ENHANCEMENTS.md`](../../src/toolbox/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Public headers must not expose internal implementation types or arena/memory layout details to callers.
- `[x]` Breaking changes to types in `include/toolbox/` require a MAJOR version bump per `VERSIONING.md`.
- `[x]` New entry points should be additive; existing stable APIs remain source-compatible.
- `[x]` Layer association (**ANN/Tensor**) must be preserved in all header expansions.

---

## Required Interfaces (Header Contract)

Current public surfaces cover: shared text-processing, language detection, chunking, normalisation, quality scoring, fingerprinting, and content-toolbox bridging utilities.

Planned extensions follow the same namespace (`themis::toolbox`) and include-guard conventions.

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

- [ ] Module-interface (C++20 modules) wrapper for `include/toolbox/` entry points
- [ ] Header-only usage path for embedding scenarios (where feasible)
- [ ] Extended layer-mapping documentation for ANN/Tensor integration patterns

---

## Alignment with Strategic Plan

These proposals do not establish current strategic-plan alignment. For cross-layer planning, consult the current governance sources before scheduling work:
→ [`../../src/ai/FUTURE_ENHANCEMENTS.md`](../../src/ai/FUTURE_ENHANCEMENTS.md)

> Exact model backend not verifiable from final document text alone.
