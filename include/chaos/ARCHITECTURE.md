> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/chaos/ARCHITECTURE.md -->

# Chaos Engineering Module — Public Header Architecture

**Module Path:** `include/chaos/`  
**Implementation:** `../../src/chaos/`  
**Canonical architecture doc:** [`../../src/chaos/ARCHITECTURE.md`](../../src/chaos/ARCHITECTURE.md)

---

## 1. Overview

`include/chaos/` defines the **public fault injection, chaos experiment orchestration, and resilience testing API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/chaos/ARCHITECTURE.md`](../../src/chaos/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Chaos Framework

| Header | Public Type | Purpose |
|--------|------------|---------|
| `chaos_framework.h` | `ChaosFramework` | Fault injection and chaos experiment orchestration |

---

## 3. Namespace Layout

All public types reside in the `themis::chaos` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/chaos/` expose the **stable public API**; internal types live in `src/chaos/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **LLM/Graph**.
