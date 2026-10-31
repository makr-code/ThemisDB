> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: planning/backlog; implementation not implied | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/ingestion/FUTURE_ENHANCEMENTS.md -->

# Ingestion Module — Public Header Future Enhancements

**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot
**Human_Review:** false
**Human_Author:** ThemisDB Team

> Exact model backend not verifiable from final document text alone.

**Module Path:** `include/ingestion/`  
**Canonical implementation enhancements:** [`../../src/ingestion/FUTURE_ENHANCEMENTS.md`](../../src/ingestion/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/ingestion/`. Runtime, algorithm, and benchmark work remains tracked in:

→ [`../../src/ingestion/FUTURE_ENHANCEMENTS.md`](../../src/ingestion/FUTURE_ENHANCEMENTS.md)

> Items and target quarters below are planning notes, not statements about
> current implementation or release commitment. Verify current declarations and
> build configuration before treating a proposal as an accepted contract.

---

## Design Constraints

- `[x]` Public headers must not expose internal implementation types or arena/memory layout details to callers.
- `[x]` Breaking changes to types in `include/ingestion/` require a MAJOR version bump per `VERSIONING.md`.
- `[x]` New entry points should be additive; existing stable APIs remain source-compatible.
- `[ ]` TODO: establish the module's layer association from a current canonical architecture source; **ANN Frontdoor** is an unverified historical label.

---

## Required Interfaces (Header Contract)

Current public surfaces cover: multi-connector ingestion pipeline (API, CDC, database, filesystem, HuggingFace, Kafka, S3, web crawler), entity assembly, semantic validation, and workflow orchestration.

Planned extensions follow the same namespace (`themis::ingestion`) and include-guard conventions.

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

- [ ] Module-interface (C++20 modules) wrapper for `include/ingestion/` entry points
- [ ] Header-only usage path for embedding scenarios (where feasible)
- [ ] TODO: document a layer-mapping only after the module's current strategic association is verified; do not assume the former ANN Frontdoor label.

---

## Alignment with Strategic Plan

The alignment with `FUTURE_PLAN.md` and the former **ANN Frontdoor** label is
unverified; confirm the current strategic source before using it. For cross-layer
planning, see:
→ [`../../src/ai/FUTURE_ENHANCEMENTS.md`](../../src/ai/FUTURE_ENHANCEMENTS.md)
