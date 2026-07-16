> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/acceleration/FUTURE_ENHANCEMENTS.md -->

# Acceleration Module — Public Header Future Enhancements

**Module Path:** `include/acceleration/`  
**Canonical implementation enhancements:** [`../../src/acceleration/FUTURE_ENHANCEMENTS.md`](../../src/acceleration/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/acceleration/`. Runtime, algorithm, and benchmark work remains tracked in:

→ [`../../src/acceleration/FUTURE_ENHANCEMENTS.md`](../../src/acceleration/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Public headers must not expose internal implementation types or arena/memory layout details to callers.
- `[x]` Breaking changes to types in `include/acceleration/` require a MAJOR version bump per `VERSIONING.md`.
- `[x]` New entry points should be additive; existing stable APIs remain source-compatible.
- `[x]` Layer association (**ANN/Tensor**) must be preserved in all header expansions.

---

## Required Interfaces (Header Contract)

Current public surfaces cover: compute-backend dispatch, GPU/CPU/Vulkan/OpenCL acceleration, FAISS GPU KNN, tensor-core matmul, multi-GPU coordination, and kernel fallback.

Planned extensions follow the same namespace (`themis::acceleration`) and include-guard conventions.

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

- [ ] Module-interface (C++20 modules) wrapper for `include/acceleration/` entry points
- [ ] Header-only usage path for embedding scenarios (where feasible)
- [ ] Extended layer-mapping documentation for ANN/Tensor integration patterns

---

## Alignment with Strategic Plan

Enhancements align with `FUTURE_PLAN.md` layer **ANN/Tensor**. For cross-layer integration planning see:
→ [`../../src/ai/FUTURE_ENHANCEMENTS.md`](../../src/ai/FUTURE_ENHANCEMENTS.md)
