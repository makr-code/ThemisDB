---
Author: "ThemisDB Team"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "api-contract"
Canonical_References: "include/exporters/*.h; src/exporters/FUTURE_ENHANCEMENTS.md"
Related: "Issue #7346; DOC-WEEKLY-2026-44"
---

Exact model backend not verifiable from final document text alone.

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: review | source checked 2026-10-31; planning dates require maintainer review -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/exporters/FUTURE_ENHANCEMENTS.md -->

# Exporters Module — Public Header Future Enhancements

**Module Path:** `include/exporters/`  
**Canonical implementation enhancements:** [`../../src/exporters/FUTURE_ENHANCEMENTS.md`](../../src/exporters/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/exporters/`. Runtime, algorithm, and benchmark work remains tracked in:

→ [`../../src/exporters/FUTURE_ENHANCEMENTS.md`](../../src/exporters/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Public headers must not expose internal implementation types or arena/memory layout details to callers.
- `[x]` Breaking changes to types in `include/exporters/` require a MAJOR version bump per `VERSIONING.md`.
- `[x]` New entry points should be additive; existing stable APIs remain source-compatible.

---

## Required Interfaces (Header Contract)

Current public surfaces cover: data export in Parquet, Arrow IPC, JSONL/LLM, HuggingFace, streaming, incremental, and encrypted formats.

Planned extensions follow the same namespace (`themis::exporters`) and include-guard conventions.

---

## Planned Enhancements

### Short-Term (Q3 2026)

TODO: Re-plan these target dates; Q3 2026 has passed and no completion claim is
made by this planning document.

- [ ] Add `[[deprecated]]` annotations to any legacy entry points with migration notes
- [ ] Add explicit `noexcept` specifications to query/read paths where safe
- [ ] Add structured diagnostic types for all thrown exceptions

### Medium-Term (Q4 2026)

- [ ] Expand public surface with convenience factory functions for common usage patterns
- [ ] Add Concepts (C++20) constraints to template parameters in public headers
- [ ] Introduce explicit ABI stability markers (`THEMIS_STABLE_ABI`) to long-term interfaces

### Long-Term (2027+)

- [ ] Module-interface (C++20 modules) wrapper for `include/exporters/` entry points
- [ ] Header-only usage path for embedding scenarios (where feasible)
- [ ] TODO: Confirm whether any cross-module layer mapping is required before documenting one.

---

## Alignment with Strategic Plan

TODO: No canonical source in the exporter headers identifies a Tensor/Graph
layer association. Keep cross-module planning in the owning module's canonical
roadmap until that relationship is verified.
