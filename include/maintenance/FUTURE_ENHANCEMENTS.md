> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/maintenance/FUTURE_ENHANCEMENTS.md -->

# Maintenance Module — Public Header Future Enhancements

**Module Path:** `include/maintenance/`  
**Canonical implementation enhancements:** [`../../src/maintenance/FUTURE_ENHANCEMENTS.md`](../../src/maintenance/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/maintenance/`. Runtime, algorithm, and benchmark work remains tracked in:

→ [`../../src/maintenance/FUTURE_ENHANCEMENTS.md`](../../src/maintenance/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Public headers must not expose internal implementation types or arena/memory layout details to callers.
- `[x]` Breaking changes to types in `include/maintenance/` require a MAJOR version bump per `VERSIONING.md`.
- `[x]` New entry points should be additive; existing stable APIs remain source-compatible.
- `[?]` Layer association (**Graph**) is inherited from legacy planning notes; **TODO:** verify it against the current maintenance architecture before treating it as normative.

---

## Required Interfaces (Header Contract)

Current public surfaces cover: database maintenance task scheduling, distributed lock coordination, task handler dispatch, and health reporting.

Planned extensions follow the same namespace (`themis::maintenance`) and include-guard conventions.

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

- [ ] Module-interface (C++20 modules) wrapper for `include/maintenance/` entry points
- [ ] Header-only usage path for embedding scenarios (where feasible)
- [ ] Extended layer-mapping documentation for Graph integration patterns

---

## Alignment with Strategic Plan

> **TODO/Assumption:** The legacy `FUTURE_PLAN.md` layer association and linked
> cross-layer plan have not been verified against current maintenance ownership.
> Use [`../../src/maintenance/ARCHITECTURE.md`](../../src/maintenance/ARCHITECTURE.md)
> for current runtime boundaries.
