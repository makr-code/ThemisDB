> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/training/FUTURE_ENHANCEMENTS.md -->

# TRAINING Module — Public Header Future Enhancements

**Module Path:** `include/training/`
**Canonical implementation enhancements:** [`../../src/training/FUTURE_ENHANCEMENTS.md`](../../src/training/FUTURE_ENHANCEMENTS.md)

---

## Scope

This document covers planned enhancements to the **public header contract** in `include/training/` — new types, interface additions, deprecation removals, and header-level API improvements. Enhancements that touch both headers and implementation are tracked primarily in the canonical source-level document:

→ [`../../src/training/FUTURE_ENHANCEMENTS.md`](../../src/training/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Headers must remain backward-compatible within a major version; new capabilities are added via new methods or versioned types.
- `[x]` `#pragma once` guard required on every header; no include-guard macros.
- `[x]` No implementation code in headers (exception: `constexpr` helpers, template bodies, and header-only utilities explicitly documented as such).
- `[x]` All factory functions and error-returning methods must be `[[nodiscard]]`.
- `[x]` Build-conditional headers must not be included unconditionally by other headers.

---

## Execution Plane Surface

- **Dataset preparation plane:** `auto_labeler.h`, `database_domain_auto_labeler.h`, `modality_parser.h`, `knowledge_graph_enricher.h`, `lora_data_selection.h`
- **Adapter training lifecycle plane:** `lora_adapter.h`, `ada_lora_adapter.h`, `adalora_tt_bridge.h`, `incremental_lora_trainer.h`, `lora_adapter_merger.h`, `lora_checkpoint_manager.h`
- **Governance and orchestration plane:** `provenance_tracker.h`, `training_interfaces.h`, `training_pipeline.h`, `adapter_serving.h`

For the authoritative interface inventory and stability classification see [`../../src/training/FUTURE_ENHANCEMENTS.md`](../../src/training/FUTURE_ENHANCEMENTS.md).

---

## Planned Header Enhancements

### 1. `[[nodiscard]]` Audit

**Priority:** Medium
**Target Version:** v2.1.0

Audit all public headers for factory functions and error-returning methods that are missing `[[nodiscard]]`. Apply missing annotations and add a CI compile-time check to prevent regressions.

---

### 2. Deprecated Symbol Cleanup

**Priority:** Low
**Target Version:** v2.1.0

Identify symbols that have been superseded in `v1.x` and annotate them with `[[deprecated("use X instead")]]`. Track removal in a subsequent major version.

---

### 3. Header Isolation Verification

**Priority:** Low
**Target Version:** v2.1.0

Verify that every header in `include/training/` compiles in isolation (without implicit transitive includes). Add a CMake `check_headers` target for automated CI enforcement.

---

## Test Strategy

| Test Type | Target | Notes |
|---|---|---|
| Compile-time | All headers compile in isolation | CMake `check_headers` target (planned) |
| Unit | Key interface implementations | Tracked in module test suite |
| ABI | No unexpected virtual table changes between patch releases | ABI checker in CI |

---

## Security / Reliability

- `[x]` `[[nodiscard]]` applied to factory and error-returning methods.
- `[x]` No implementation code in public headers.
- `[x]` Build-conditional guards documented in `ARCHITECTURE.md`.

---

## References

- Canonical implementation enhancements: [`../../src/training/FUTURE_ENHANCEMENTS.md`](../../src/training/FUTURE_ENHANCEMENTS.md)
- Architecture: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- Roadmap: [`ROADMAP.md`](ROADMAP.md)
- Module overview: [`README.md`](README.md)
