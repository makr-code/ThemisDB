<!-- Status: current | validated: 2026-03-22 -->

# Voice — Security

## Scope
Security considerations for the public headers in `include/voice/`. This covers voice authentication, anti-spoofing, audio data handling, browser/telephony streaming, session management, and privacy of biometric and audio data.

## Threat Model

| Threat | Impact | Mitigation |
|---|---|---|
| Voice spoofing / replay attack | Unauthorized voice authentication | `voice_security.h` liveness detection; ML anti-spoofing planned Q2 2026 |
| SSML injection via TTS input | Unexpected audio output, DoS | `voice_tts_customizer.h` must sanitize SSML; tracked in ROADMAP |
| Audio buffer retained post-VAD | Privacy violation (always-on mic) | `wake_word_detector.h` must discard audio outside activation window |
| Biometric template exfiltration | Speaker identity exposure | `voice_auth.h` templates stored encrypted; audit planned Q4 2026 |
| WebRTC/WS origin bypass | Unauthorized audio stream injection | `voice_browser_streaming.h` origin allowlist (planned Q2 2026) |
| Path traversal in model cache | Arbitrary file read | `voice_model_cache.h` model names must be allowlisted |
| Session token prediction | Session hijacking | `voice_session_manager.h` uses CSPRNG-backed UUID (fixed v1.1.0) |
| DTMF tone interception | Credential capture via telephony | `voice_telephony.h` must isolate DTMF handling per call leg |
| Meeting transcript PII leakage | Privacy / regulatory breach | `voice_meeting_support.h` transcript export requires PII redaction |
| Macro privilege escalation | Unauthorized command replay | `voice_macro.h` macro replay must respect caller privilege level |

## Security Controls

- **Liveness detection:** `VoiceSecurity` provides configurable anti-spoofing; ML-based upgrade planned.
- **Session tokens:** CSPRNG-backed UUID v4 in `VoiceSessionManager` (since v1.1.0).
- **Audio retention policy:** `WakeWordDetector` must not retain audio outside activation window; enforced by design.
- **Encrypted audio storage:** `VoiceAudioStorage` targets AES-256-GCM (planned Q2 2026).
- **PII in transcripts:** Meeting transcript export integrates with `../../include/utils/pii_pseudonymizer.h`.
- **Error messages:** `VoiceErrorHandler` must not include internal file paths, model paths, or user credentials.

## Known Limitations

- **LIMITATION-VOICE-01 (Medium):** Anti-spoofing is currently threshold-based, not ML-based. Susceptible to sophisticated replay. ML upgrade planned Q2 2026.
- **LIMITATION-VOICE-02 (Medium):** SSML input sanitization not yet enforced in `voice_tts_customizer.h`. Callers must sanitize. Fix planned Q2 2026.
- **LIMITATION-VOICE-03 (Medium):** `voice_browser_streaming.h` origin validation not yet enforced at API level. Planned Q2 2026.
- **LIMITATION-VOICE-04 (Low):** `voice_model_cache.h` model name allowlist not yet in public API. Path traversal risk in implementation. Fix planned Q2 2026.
- **LIMITATION-VOICE-05 (Info):** Biometric template storage has not undergone formal security audit. Planned Q4 2026.
