> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/ai/ARCHITECTURE.md -->

# AI Plugin Generator Module — Public Header Architecture

**Module Path:** `include/ai/`  
**Implementation:** `../../src/ai/`  
**Canonical architecture doc:** [`../../src/ai/ARCHITECTURE.md`](../../src/ai/ARCHITECTURE.md)

---

## 1. Overview

`include/ai/` defines the **public AI-assisted plugin scaffolding and code generation API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/ai/ARCHITECTURE.md`](../../src/ai/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Plugin Generator

| Header | Public Type | Purpose |
|--------|------------|---------|
| `ai_plugin_generator.h` | `AIPluginGenerator` | LLM-assisted plugin scaffolding and code generation |

---

## 3. Namespace Layout

All public types reside in the `themis::ai` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/ai/` expose the **stable public API**; internal types live in `src/ai/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **LLM**.
