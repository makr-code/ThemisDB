---
Author: "ThemisDB Team"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Voice Public Headers

This directory declares C++ voice interfaces. It does not by itself provide
network protocols, database-backed persistence, deployed STT/TTS services, or
application authorization. The current header map is
[`ARCHITECTURE.md`](ARCHITECTURE.md); inspect the individual header comments for
the exact contract.

## Primary API areas

- Assistant and session: `voice_assistant.h`, `voice_session_manager.h`
- Audio and detection: `audio_preprocessing.h`, `wake_word_detector.h`,
  `voice_intent_detector.h`
- Streaming and telephony: `voice_browser_streaming.h`, `voice_telephony.h`
- Authentication and security helpers: `voice_auth.h`,
  `voice_liveness_checker.h`, `voice_liveness_detector.h`,
  `voice_anti_spoof_engine.h`, `voice_security.h`
- Storage and audit helpers: `voice_audio_storage.h`, `voice_audit_logger.h`
- Other helpers: batch, error handling, accessibility, emotion, macros,
  meeting support, model cache, and TTS customization

## Integration cautions

- `VoiceAssistant::Config` disables voice authentication and wake-word handling
  by default.
- Browser streaming may produce placeholder transcript/TTS output without
  injected callbacks.
- In-memory helper state is not durable persistence or a compliance record.
- A class or feature name does not prove a supported external backend or
  protocol implementation.

See [`../../src/voice/API.md`](../../src/voice/API.md) for a concise API
summary and [`../../src/voice/ARCHITECTURE.md`](../../src/voice/ARCHITECTURE.md)
for current dataflow/build/test boundaries.
