---
Author: "ThemisDB Team"
Created: "2026-10-18"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Voice Feature Map

This index names in-tree surfaces; it does not imply a deployed service,
external model, protocol conformance, or customer outcome.

| Capability area | Current source surface |
|---|---|
| Assistant command orchestration | `voice_assistant.cpp`, `voice_assistant_llm.cpp` |
| Audio preprocessing and wake-word decisions | `audio_preprocessing.cpp`, `wake_word_detector.cpp` |
| Transcript intent classification | `voice_intent_detector.cpp` |
| Session state and persistence interface | `voice_session_manager.cpp` |
| Browser-style chunk streaming | `voice_browser_streaming.cpp` |
| Telephony session/bridge interfaces | `voice_telephony.cpp` |
| Speaker verification and liveness helpers | `voice_authenticator.cpp`, liveness/anti-spoof sources |
| Storage, audit, and security helpers | `voice_audio_storage.cpp`, `voice_audit_logger.cpp`, `voice_security.cpp` |

## Boundaries

Public types are declared under [`../../include/voice/`](../../include/voice/).
The caller/service owns authorization, consent, transport decoding, durable
storage and deployment policy. The default browser transcript and TTS paths are
placeholders without injected callbacks. See [`ARCHITECTURE.md`](ARCHITECTURE.md)
and [`API.md`](API.md) for verified integration caveats.
