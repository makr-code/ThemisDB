---
Author: "ThemisDB Team"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/whisper/FUTURE_ENHANCEMENTS.md -->

# Whisper ASR Module — Public Header Future Enhancements

**Module Path:** `include/whisper/`  
**Canonical implementation enhancements:** [`../../src/whisper/FUTURE_ENHANCEMENTS.md`](../../src/whisper/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/whisper/`. Runtime, algorithm, and benchmark work remains tracked in:

→ [`../../src/whisper/FUTURE_ENHANCEMENTS.md`](../../src/whisper/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Public headers must not expose internal implementation types or arena/memory layout details to callers.
- `[x]` Breaking changes to types in `include/whisper/` require a MAJOR version bump per `VERSIONING.md`.
- `[x]` New entry points should be additive; existing stable APIs remain source-compatible.
- `[x]` Layer association (**LLM/Voice**) must be preserved in all header expansions.

---

## Required Interfaces (Header Contract)

Current public surfaces cover: OpenAI Whisper ASR integration plugin, audio chunking, voice activity detection, transcription, configuration, and plugin registration.

Planned extensions follow the same namespace (`themis::whisper`) and include-guard conventions.

The current module target is C++17 (`src/whisper/CMakeLists.txt`). Any planned C++20 constraints
or module interface must first define the compatibility/build-standard transition; they are not
part of the current API.

---

## Planned Enhancements

### Short-Term (reschedule; previous Q3 2026 target passed)

- [ ] Identify an obsolete public entry point before adding `[[deprecated]]`; document its replacement and migration notes (Target: TODO)
- [ ] Add explicit `noexcept` only where exception behavior is proven and documented (Target: TODO)
- [ ] Define structured diagnostics only if they preserve current error behavior and have a concrete consumer (Target: TODO)

### Medium-Term (Q4 2026)

- [ ] Evaluate convenience factories against existing `WhisperPluginRegistrar` helpers (Target: TODO)
- [ ] Consider Concepts (C++20) only after a repository-wide C++ standard/compatibility decision (Target: TODO)
- [ ] Define ABI-stability policy before adding stability-marker macros (Target: TODO)

### Long-Term (2027+)

- [ ] Module-interface (C++20 modules) wrapper for `include/whisper/` entry points, gated on repository support (Target: TODO)
- [ ] Header-only usage path for embedding scenarios (where feasible)
- [ ] Extended layer-mapping documentation for LLM/Voice integration patterns

---

## Alignment with Strategic Plan

Enhancements align with `FUTURE_PLAN.md` layer **LLM/Voice**. For cross-layer integration planning see:
→ [`../../src/ai/FUTURE_ENHANCEMENTS.md`](../../src/ai/FUTURE_ENHANCEMENTS.md)
