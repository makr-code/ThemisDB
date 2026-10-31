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
Canonical_Sources: "src/llama_cpp/FUTURE_ENHANCEMENTS.md; src/llama_cpp/ROADMAP.md; include/llama_cpp/llama_cpp_plugin.h; include/llama_cpp/llama_cpp_registrar.h"
Related_Issue: "#7356"
Related_Milestone: "DOC-WEEKLY-2026-44"
---

> Exact model backend not verifiable from final document text alone.

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/llama_cpp/FUTURE_ENHANCEMENTS.md -->

# Llama.cpp Integration Module — Public Header Future Enhancements

**Module Path:** `include/llama_cpp/`  
**Canonical implementation enhancements:** [`../../src/llama_cpp/FUTURE_ENHANCEMENTS.md`](../../src/llama_cpp/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/llama_cpp/`. Runtime, algorithm, and benchmark work remains tracked in:

→ [`../../src/llama_cpp/FUTURE_ENHANCEMENTS.md`](../../src/llama_cpp/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Public headers must not expose internal implementation types or arena/memory layout details to callers.
- `[x]` Breaking changes to types in `include/llama_cpp/` require a MAJOR version bump per `VERSIONING.md`.
- `[x]` New entry points should be additive; existing stable APIs remain source-compatible.
- `[x]` Layer association (**LLM**) must be preserved in all header expansions.

---

## Required Interfaces (Header Contract)

The current public declarations cover plugin lifecycle/inference and registrar
integration. They are in the `themis::llamacpp` namespace; see the public
headers linked above.

---

## Planned Enhancements

No header-specific enhancements are currently approved. New proposals must
first be recorded in the canonical
[`src/llama_cpp/FUTURE_ENHANCEMENTS.md`](../../src/llama_cpp/FUTURE_ENHANCEMENTS.md)
with acceptance criteria and validation requirements.

---

## Alignment with Strategic Plan

Module implementation status and planning remain in
[`../../src/llama_cpp/ROADMAP.md`](../../src/llama_cpp/ROADMAP.md).
