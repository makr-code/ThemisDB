---
Author: ThemisDB Team
Created: 2026-06-01
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

Exact model backend not verifiable from final document text alone.

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | header inventory reviewed: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/ethics_ai/FUTURE_ENHANCEMENTS.md -->

# Ethics AI Module — Public Header Future Enhancements

**Module Path:** `include/ethics_ai/`  
**Canonical implementation enhancements:** [`../../src/ethics_ai/FUTURE_ENHANCEMENTS.md`](../../src/ethics_ai/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/ethics_ai/`. Runtime, algorithm, and benchmark work remains tracked in:

→ [`../../src/ethics_ai/FUTURE_ENHANCEMENTS.md`](../../src/ethics_ai/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Public headers must not expose internal implementation types or arena/memory layout details to callers.
- `[x]` Breaking changes to types in `include/ethics_ai/` require a MAJOR version bump per `VERSIONING.md`.
- `[x]` New entry points should be additive; existing stable APIs remain source-compatible.
- `[x]` Layer association (**LLM**) must be preserved in all header expansions.

---

## Required Interfaces (Header Contract)

Current public headers also declare LDM helper APIs (`DiscourseOrchestrator`, `MetaVerdictBuilder`, and `MirrorSchoolHandler`). Their declaration does not mean they are wired to production inference; see `../../src/ethics_ai/ARCHITECTURE.md`.

Planned extensions follow the namespace used by current headers (`themis::plugins::ethics`) and the existing include-guard conventions.

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

- [ ] Module-interface (C++20 modules) wrapper for `include/ethics_ai/` entry points
- [ ] Header-only usage path for embedding scenarios (where feasible)
- [ ] Extended layer-mapping documentation for LLM integration patterns

---

## Alignment with Strategic Plan

The module is labeled as an LLM-layer interface in its module documentation. The previously referenced `FUTURE_PLAN.md` is not present at the repository root or in this directory. For cross-layer integration planning see:
→ [`../../src/ai/FUTURE_ENHANCEMENTS.md`](../../src/ai/FUTURE_ENHANCEMENTS.md)
