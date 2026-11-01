---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_Sources: "include/llama_cpp/llama_cpp_plugin.h; include/llama_cpp/llama_cpp_registrar.h; src/llama_cpp/llama_cpp_plugin.cpp; src/llama_cpp/llama_cpp_registrar.cpp"
Related_Issue: "#7356"
Related_Milestone: "DOC-WEEKLY-2026-44"
---

> Exact model backend not verifiable from final document text alone.

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-10-31 -->
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
| `llama_cpp_registrar.h` | `LlamaCppPluginRegistrar` | Plugin factory, adapter, and registration helpers |

---

## 3. Namespace Layout

The public plugin types reside in `themis::llamacpp`. The plugin implements
`themis::llm::ILLMPlugin`; its registrar is `LlamaCppPluginRegistrar`.

---

## 4. Contract Notes

- Headers in `include/llama_cpp/` expose the public integration declarations; internal implementation lives in `src/llama_cpp/`. This description does not assert ABI stability.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **LLM**.
