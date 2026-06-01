> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/distributed_knowledge/FUTURE_ENHANCEMENTS.md -->

# Distributed Knowledge Module — Public Header Future Enhancements

**Module Path:** `include/distributed_knowledge/`  
**Canonical implementation enhancements:** [`../../src/distributed_knowledge/FUTURE_ENHANCEMENTS.md`](../../src/distributed_knowledge/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/distributed_knowledge/`. Runtime, algorithm, and benchmark work remains tracked in:

→ [`../../src/distributed_knowledge/FUTURE_ENHANCEMENTS.md`](../../src/distributed_knowledge/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Public headers must not expose internal implementation types or arena/memory layout details to callers.
- `[x]` Breaking changes to types in `include/distributed_knowledge/` require a MAJOR version bump per `VERSIONING.md`.
- `[x]` New entry points should be additive; existing stable APIs remain source-compatible.
- `[x]` Layer association (**LLM/Tensor**) must be preserved in all header expansions.

---

## Required Interfaces (Header Contract)

Current public surfaces cover: federated RAG merging, LoRA federation, federated distillation, cross-shard feedback sync, and adapter capability announcements.

Planned extensions follow the same namespace (`themis::distributed_knowledge`) and include-guard conventions.

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

- [ ] Module-interface (C++20 modules) wrapper for `include/distributed_knowledge/` entry points
- [ ] Header-only usage path for embedding scenarios (where feasible)
- [ ] Extended layer-mapping documentation for LLM/Tensor integration patterns

---

## Alignment with Strategic Plan

Enhancements align with `FUTURE_PLAN.md` layer **LLM/Tensor**. For cross-layer integration planning see:
→ [`../../src/ai/FUTURE_ENHANCEMENTS.md`](../../src/ai/FUTURE_ENHANCEMENTS.md)
