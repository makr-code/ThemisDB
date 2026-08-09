> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/stable_diffusion/ARCHITECTURE.md -->

# Stable Diffusion Module — Public Header Architecture

**Module Path:** `include/stable_diffusion/`  
**Implementation:** `../../src/stable_diffusion/`  
**Canonical architecture doc:** [`../../src/stable_diffusion/ARCHITECTURE.md`](../../src/stable_diffusion/ARCHITECTURE.md)

---

## 1. Overview

`include/stable_diffusion/` defines the **public Stable Diffusion image generation plugin, configuration, prompt sanitisation, and plugin registration API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/stable_diffusion/ARCHITECTURE.md`](../../src/stable_diffusion/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Stable Diffusion Plugin

| Header | Public Type | Purpose |
|--------|------------|---------|
| `sd_generator.h` | `ISDGenerator`, `SDStubGenerator`, `InMemorySDGenerator`, `SDCppGenerator` | Stable Diffusion generation backends |
| `sd_config.h` | `SDConfig` | Stable Diffusion configuration |
| `sd_prompt_sanitizer.h` | `SDPromptSanitizer` | Prompt safety and sanitisation for SD |
| `sd_plugin.h` | `SDPlugin` | SD plugin contract |
| `sd_plugin_registrar.h` | `SDPluginRegistrar` | Plugin self-registration helper |

---

## 3. Namespace Layout

All public types reside in the `themis::imggen` namespace.

---

## 4. Contract Notes

- Headers in `include/stable_diffusion/` expose the **stable public API**; internal types live in `src/stable_diffusion/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **LLM**.
