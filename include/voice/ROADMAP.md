> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ../../src/voice/ROADMAP.md -->

# VOICE Module — Public Header Roadmap

**Module Path:** `include/voice/`
**Canonical implementation roadmap:** [`../../src/voice/ROADMAP.md`](../../src/voice/ROADMAP.md)

---

## Overview

This document tracks public API contract stability, planned header additions, and header-level breaking changes for `include/voice/`. For feature roadmap items that affect both implementation and headers see the canonical roadmap:

→ [`../../src/voice/ROADMAP.md`](../../src/voice/ROADMAP.md)

---

## Current Status

production voice runtime with assistant orchestration, preprocessing, session handling, streaming integration, auth, and security controls. All production-required public headers are present and `#pragma once` guarded.

The header API surface is **stable** for all types introduced in v1.x.

---

## Completed ✅

- [x] `voice_assistant.h` — assistant and orchestration contract
- [x] `voice_intent_detector.h` — assistant and orchestration contract
- [x] `voice_macro.h` — assistant and orchestration contract
- [x] `audio_preprocessing.h` — audio preprocessing and detection contract
- [x] `wake_word_detector.h` — audio preprocessing and detection contract
- [x] `emotion_analyzer.h` — audio preprocessing and detection contract
- [x] `voice_session_manager.h` — session and streaming contract
- [x] `voice_browser_streaming.h` — session and streaming contract
- [x] `voice_telephony.h` — session and streaming contract
- [x] `voice_batch_processor.h` — session and streaming contract
- [x] `voice_tts_customizer.h` — session and streaming contract
- [x] `voice_auth.h` — security and lifecycle contract
- [x] `voice_security.h` — security and lifecycle contract
- [x] `voice_error_handler.h` — security and lifecycle contract
- [x] `voice_accessibility.h` — security and lifecycle contract
- [x] `voice_audio_storage.h` — security and lifecycle contract
- [x] `voice_meeting_support.h` — security and lifecycle contract
- [x] `voice_model_cache.h` — security and lifecycle contract

---

## In Progress 🚧

- [I] Header-level unit test coverage for all public interfaces (tracked via module issue backlog)

---

## Planned Features 📋

### Short-term (Next 3–6 months)

- [ ] Audit all headers for missing `[[nodiscard]]` on factory and error-returning methods (Target: Q3 2026)
- [ ] Verify `#pragma once` guard consistency across all headers in a CI step (Target: Q3 2026)

### Medium-term (6–12 months)

- [ ] Align header-level type documentation with OpenAPI spec where applicable (Target: Q4 2026)
- [ ] Consolidate deprecated symbol annotations with `[[deprecated("...")]]` where needed (Target: Q4 2026)

---

## Production Readiness Checklist

- [x] All headers have `#pragma once` guard
- [x] All public factory methods marked `[[nodiscard]]`
- [x] Build conditionals documented in `README.md` and `ARCHITECTURE.md`
- [P] Header-level unit tests (tracked in module issue backlog)

---

## References

- Canonical implementation roadmap: [`../../src/voice/ROADMAP.md`](../../src/voice/ROADMAP.md)
- Architecture: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- Future enhancements: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)
- Module overview: [`README.md`](README.md)
