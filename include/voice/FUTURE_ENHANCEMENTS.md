> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/voice/FUTURE_ENHANCEMENTS.md -->

# Voice Module — Public Header Future Enhancements

**Module Path:** `include/voice/`
**Canonical implementation enhancements:** [`../../src/voice/FUTURE_ENHANCEMENTS.md`](../../src/voice/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/voice/`. Runtime STT/TTS engine routing, telephony signalling, wake-word inference, and benchmark work remain tracked in:

→ [`../../src/voice/FUTURE_ENHANCEMENTS.md`](../../src/voice/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Authentication and security headers must fail closed for detected deepfake or spoofing signals.
- `[x]` Security headers must scrub PII before any downstream delivery or storage.
- `[x]` Session and assistant headers must not expose STT/TTS engine selection or pipeline routing internals.
- `[x]` Telephony and streaming headers must define stable signalling contracts independent of transport-layer details.

---

## Required Interfaces (Header Contract)

| Interface | Declared In | Consumer | Status |
|-----------|------------|----------|--------|
| `VoiceAssistant` process / stream | `voice_assistant.h` | API gateway, meeting integration | ✅ Stable |
| `VoiceAuth` authenticate API | `voice_auth.h` | Authentication middleware | ✅ Stable |
| `VoiceIntentDetector` detect API | `voice_intent_detector.h` | Conversational AI pipeline | ✅ Stable |
| `VoiceBrowserStreaming` stream API | `voice_browser_streaming.h` | Web front-ends | ✅ Stable |
| `VoiceErrorHandler` structured errors | `voice_error_handler.h` | All pipeline consumers | ✅ Stable |

---

## Planned Enhancements

### Short-Term (Q3 2026)

- Document PII-scrubbing coverage and deepfake-detection fail-closed semantics uniformly in `voice_security.h`.
- Clarify speaker-diarisation segment-labelling guarantees and confidence bounds in `voice_meeting_support.h`.
- Add WebRTC/SIP protocol conformance notes to `voice_browser_streaming.h` and `voice_telephony.h`.

### Medium-Term (Q4 2026)

- Introduce `voice_policy.h` to provide per-session audio processing, recording retention, and privacy policy contract.
- Expose benchmark-reference latency targets for wake-word detection, intent extraction, and TTS synthesis hot paths.
- Add deprecation guidance for any legacy audio-format assumptions and document migration to WebM/Opus pipelines.

### Long-Term

- Unify emotion, intent, and speaker-ID result types behind a shared voice-context envelope for pipeline consumers.
- Add extension hooks for embedders to inject custom STT/TTS backends via a stable engine-adapter interface.
- Provide voice accessibility output variants (captions, signing-language annotations) via a shared accessibility contract.
