> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/voice/ARCHITECTURE.md -->

# VOICE Module — Public Header Architecture

**Module Path:** `include/voice/`
**Implementation:** `../../src/voice/`
**Canonical architecture doc:** [`../../src/voice/ARCHITECTURE.md`](../../src/voice/ARCHITECTURE.md)

---

## 1. Overview

The `include/voice/` directory contains the **public C++ header contract** for ThemisDB's voice input processing, session control, assistant orchestration, streaming/telephony interfaces, and safety controls. Headers define types, interfaces, and configuration structures consumed by internal implementation files and embedders.

All headers are `#pragma once` guarded and contain no implementation code.

For full architectural details — data flow diagrams, threading model, integration point map — see the canonical document:

→ [`../../src/voice/ARCHITECTURE.md`](../../src/voice/ARCHITECTURE.md)

---

## 2. Namespace

All public types live under `themis::voice`.

---

## 3. Header Surface Map

| Execution Plane | Key Headers |
|---|---|
| `Assistant and orchestration` | `voice_assistant.h`, `voice_intent_detector.h`, `voice_macro.h` |
| `Audio preprocessing and detection` | `audio_preprocessing.h`, `wake_word_detector.h`, `emotion_analyzer.h` |
| `Session and streaming` | `voice_session_manager.h`, `voice_browser_streaming.h`, `voice_telephony.h`... |
| `Security and lifecycle` | `voice_auth.h`, `voice_security.h`, `voice_error_handler.h`... |

Full header list: see [`README.md`](README.md).

---

## 4. Build Conditionals

| CMake Symbol | Headers Affected | Required Dependency |
|---|---|---|
| `THEMIS_ENABLE_TELEPHONY` | voice_telephony.h | Telephony backend integration |
| `THEMIS_ENABLE_BROWSER_STREAMING` | voice_browser_streaming.h | Browser WebRTC/streaming transport |

---

## 5. Compatibility and Stability

- **ABI stability:** Public types follow semantic versioning; breaking changes trigger a major version bump.
- **No implementation code:** Headers contain only declarations and `constexpr`/template helpers.
- **`[[nodiscard]]`:** Factory functions and error-returning methods use `[[nodiscard]]`.

---

## 6. References

- Full architecture: [`../../src/voice/ARCHITECTURE.md`](../../src/voice/ARCHITECTURE.md)
- Module overview: [`../../src/voice/README.md`](../../src/voice/README.md)
- Roadmap: [`../../src/voice/ROADMAP.md`](../../src/voice/ROADMAP.md)
- Future enhancements: [`../../src/voice/FUTURE_ENHANCEMENTS.md`](../../src/voice/FUTURE_ENHANCEMENTS.md)
- Public header overview: [`README.md`](README.md)
