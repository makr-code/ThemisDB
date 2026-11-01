---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone. Original creation dates
> could not be verified from available history; `Created` records this AI-assisted revision.

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/stable_diffusion/FUTURE_ENHANCEMENTS.md -->

# Stable Diffusion Module — Public Header Future Enhancements

**Module Path:** `include/stable_diffusion/`  
**Canonical implementation enhancements:** [`../../src/stable_diffusion/FUTURE_ENHANCEMENTS.md`](../../src/stable_diffusion/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/stable_diffusion/`. Runtime, algorithm, and benchmark work remains tracked in:

→ [`../../src/stable_diffusion/FUTURE_ENHANCEMENTS.md`](../../src/stable_diffusion/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Public headers must not expose internal implementation types or arena/memory layout details to callers.
- `[x]` Breaking changes to types in `include/stable_diffusion/` require a MAJOR version bump per `VERSIONING.md`.
- `[x]` New entry points should be additive; existing stable APIs remain source-compatible.
- `[x]` Layer association (**LLM**) must be preserved in all header expansions.

---

## Required Interfaces (Header Contract)

Current public surfaces cover: Stable Diffusion image generation plugin, configuration, prompt sanitisation, and plugin registration.

Planned extensions follow the same namespace (`themis::imggen`) and include-guard conventions.

---

## Planned Enhancements

### Short-Term (Target Q3 2026 elapsed; replan needed)

**TODO:** Re-evaluate the past target dates; no replacement milestone is inferred here.

- [ ] Add `[[deprecated]]` annotations to any legacy entry points with migration notes
- [ ] Add explicit `noexcept` specifications to query/read paths where safe
- [ ] Add structured diagnostic types for all thrown exceptions

### Medium-Term (Q4 2026)

- [ ] Expand public surface with convenience factory functions for common usage patterns
- [ ] Add Concepts (C++20) constraints to template parameters in public headers
- [ ] Introduce explicit ABI stability markers (`THEMIS_STABLE_ABI`) to long-term interfaces

### Long-Term (2027+)

- [ ] Module-interface (C++20 modules) wrapper for `include/stable_diffusion/` entry points
- [ ] Header-only usage path for embedding scenarios (where feasible)
- [ ] Extended layer-mapping documentation for LLM integration patterns

---

## Alignment with Strategic Plan

For cross-layer planning see [`../../src/ai/FUTURE_ENHANCEMENTS.md`](../../src/ai/FUTURE_ENHANCEMENTS.md).
The previously cited `FUTURE_PLAN.md` is absent from the repository; **TODO:** verify the intended
strategic layer mapping before asserting that this module belongs to layer **LLM**.
