> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/chimera/ARCHITECTURE.md -->

# Chimera Adapter Module — Public Header Architecture

**Module Path:** `include/chimera/`  
**Implementation:** `../../src/chimera/`  
**Canonical architecture doc:** [`../../src/chimera/ARCHITECTURE.md`](../../src/chimera/ARCHITECTURE.md)

---

## 1. Overview

`include/chimera/` defines the **public database adapter abstraction layer for multi-engine interoperability API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/chimera/ARCHITECTURE.md`](../../src/chimera/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Database Adapters

| Header | Public Type | Purpose |
|--------|------------|---------|
| `database_adapter.hpp` | `DatabaseAdapter` | Polymorphic database adapter interface |
| `themisdb_adapter.hpp` | `ThemisDBAdapter` | ThemisDB-native chimera adapter |

---

## 3. Namespace Layout

All public types reside in the `themis::chimera` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/chimera/` expose the **stable public API**; internal types live in `src/chimera/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **Graph**.
