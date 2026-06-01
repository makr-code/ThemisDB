> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/llama_cpp/ARCHITECTURE.md -->

# Llama.cpp Integration Module — Public Header Architecture

**Module Path:** `include/llama_cpp/`  
**Implementation:** `../../src/llama_cpp/`  
**Canonical architecture doc:** [`../../src/llama_cpp/ARCHITECTURE.md`](../../src/llama_cpp/ARCHITECTURE.md)

---

## 1. Overview

`include/llama_cpp/` defines the **public llama.cpp plugin registration and runtime integration API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/llama_cpp/ARCHITECTURE.md`](../../src/llama_cpp/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Plugin Integration

| Header | Public Type | Purpose |
|--------|------------|---------|
| `llama_cpp_plugin.h` | `LlamaCppPlugin` | llama.cpp engine plugin contract |
| `llama_cpp_registrar.h` | `LlamaCppRegistrar` | Plugin self-registration helper |

---

## 3. Namespace Layout

All public types reside in the `themis::llama_cpp` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/llama_cpp/` expose the **stable public API**; internal types live in `src/llama_cpp/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **LLM**.
