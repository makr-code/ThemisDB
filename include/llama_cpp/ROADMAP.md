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
Canonical_Sources: "src/llama_cpp/ROADMAP.md; src/llama_cpp/FUTURE_ENHANCEMENTS.md; include/llama_cpp/llama_cpp_plugin.h; include/llama_cpp/llama_cpp_registrar.h"
Related_Issue: "#7356"
Related_Milestone: "DOC-WEEKLY-2026-44"
---

> Exact model backend not verifiable from final document text alone.

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/llama_cpp/ROADMAP.md -->

# Llama.cpp Integration Module — Public Header Roadmap

**Module Path:** `include/llama_cpp/`  
**Canonical implementation roadmap:** [`../../src/llama_cpp/ROADMAP.md`](../../src/llama_cpp/ROADMAP.md)

---

## Overview

Tracks public `llama_cpp` API contract stability, header-level coverage work, and future public entry points. Runtime implementation work remains in:

→ [`../../src/llama_cpp/ROADMAP.md`](../../src/llama_cpp/ROADMAP.md)

---

## Current Status

The two public headers expose `LlamaCppPlugin` and `LlamaCppPluginRegistrar`
in `themis::llamacpp`. Runtime delivery and acceptance are tracked by the
canonical [`src/llama_cpp/ROADMAP.md`](../../src/llama_cpp/ROADMAP.md);
the public-header scope has no separate implementation status.

---

## Completed ✅

- [x] Public header surfaces documented in `ARCHITECTURE.md`
- [x] Canonical cross-links to `src/llama_cpp/` implementation docs established
- [x] LLM association documented from `LlamaCppPlugin : llm::ILLMPlugin`
- [x] Namespace layout (`themis::llamacpp`) documented

---

## In Progress

None.

---

## Planned

No public-header work is currently approved. See
[`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md).

---

## Production Readiness Checklist

- [x] Public header surfaces documented
- [x] Canonical links to implementation docs set
- [x] Runtime readiness and deployment gates link to
  [`PRODUCTION_REQUIREMENTS.md`](../../src/llama_cpp/PRODUCTION_REQUIREMENTS.md)
