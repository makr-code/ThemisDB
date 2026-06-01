> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/temporal/ARCHITECTURE.md -->

# TEMPORAL Module — Public Header Architecture

**Module Path:** `include/temporal/`
**Implementation:** `../../src/temporal/`
**Canonical architecture doc:** [`../../src/temporal/ARCHITECTURE.md`](../../src/temporal/ARCHITECTURE.md)

---

## 1. Overview

The `include/temporal/` directory contains the **public C++ header contract** for ThemisDB's temporal query execution, bitemporal version semantics, snapshot/retention lifecycle, temporal indexing and aggregation, and conflict/CDC/compression behavior. Headers define types, interfaces, and configuration structures consumed by internal implementation files and embedders.

All headers are `#pragma once` guarded and contain no implementation code.

For full architectural details — data flow diagrams, threading model, integration point map — see the canonical document:

→ [`../../src/temporal/ARCHITECTURE.md`](../../src/temporal/ARCHITECTURE.md)

---

## 2. Namespace

All public types live under `themis::temporal`.

---

## 3. Header Surface Map

| Execution Plane | Key Headers |
|---|---|
| `Query and version semantics` | `temporal_query_engine.h`, `bi_temporal.h`, `bitemporal_join.h`... |
| `Lifecycle and consistency` | `snapshot_manager.h`, `retention_manager.h`, `temporal_conflict_resolver.h`... |
| `Indexing and throughput` | `temporal_index.h`, `interval_tree_index.h`, `temporal_aggregator.h`... |

Full header list: see [`README.md`](README.md).

---

## 4. Build Conditionals

_No optional compile-time dependencies — all headers are unconditionally available._

---

## 5. Compatibility and Stability

- **ABI stability:** Public types follow semantic versioning; breaking changes trigger a major version bump.
- **No implementation code:** Headers contain only declarations and `constexpr`/template helpers.
- **`[[nodiscard]]`:** Factory functions and error-returning methods use `[[nodiscard]]`.

---

## 6. References

- Full architecture: [`../../src/temporal/ARCHITECTURE.md`](../../src/temporal/ARCHITECTURE.md)
- Module overview: [`../../src/temporal/README.md`](../../src/temporal/README.md)
- Roadmap: [`../../src/temporal/ROADMAP.md`](../../src/temporal/ROADMAP.md)
- Future enhancements: [`../../src/temporal/FUTURE_ENHANCEMENTS.md`](../../src/temporal/FUTURE_ENHANCEMENTS.md)
- Public header overview: [`README.md`](README.md)
