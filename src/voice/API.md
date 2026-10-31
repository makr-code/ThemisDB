---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Voice API Reference

> Exact model backend not verifiable from final document text alone.

**Audience:** C++ developers integrating voice sessions, command processing, streaming, or biometric checks. Public declarations in [`include/voice`](../../include/voice) are authoritative; this guide summarizes their current behavior and does not add a new API.

## Primary interfaces

| Header / type | Purpose and contract |
|---|---|
| [`voice_assistant.h`](../../include/voice/voice_assistant.h) — `VoiceAssistant` | Initializes and shuts down the combined voice runtime; processes audio or text commands; exposes optional wake-word, speaker-auth, macro, audio-storage, recording, and synthesis operations. `processVoiceCommand` returns response bytes and rejects empty/oversized input fail-closed. Optional voice authentication and wake-word detection are disabled in the default `Config`. |
| [`voice_session_manager.h`](../../include/voice/voice_session_manager.h) — `VoiceSessionManager` | `createSession(user_id, device_id)` rejects an empty user ID and returns a session with an empty ID on creation failure. Lookups return `optional`; mutation methods return `bool`; termination and cleanup have explicit timeout behavior. Methods are documented thread-safe. |
| [`voice_browser_streaming.h`](../../include/voice/voice_browser_streaming.h) — `VoiceStreamingSession`, `VoiceStreamingManager` | Create/start a stream, register transcript/TTS/error callbacks, send ordered audio chunks, signal utterance end, and close. A frame outside configured bounds or misaligned PCM is rejected and signaled through the error callback; a manager at capacity returns an empty stream ID. |
| [`voice_stream_validator.h`](../../include/voice/voice_stream_validator.h) — `VoiceStreamValidator` | Validates non-empty chunks, accepted format metadata, sequence, timestamp, and malformed content. Constructor rejects invalid session/format settings; `validate_chunk` throws `StreamValidationError` on rejected chunks. |
| [`voice_intent_detector.h`](../../include/voice/voice_intent_detector.h) — `VoiceIntentDetector` | Maps transcript text to intent and named entities. Low confidence or timeout can yield `UNKNOWN`; callers should inspect confidence and `requires_context`. |
| [`voice_auth.h`](../../include/voice/voice_auth.h) — `VoiceBiometricAuthenticator` | Enrolls one profile per user, performs 1:1 verification or candidate-limited 1:N identification, and combines liveness plus verification in `authenticate`. Enrollment and authentication return explicit failure results; biometric checks are not a replacement for application authorization. |
| [`voice_security.h`](../../include/voice/voice_security.h) — `VoiceSecurityManager` | Provides PII redaction, consent records, audit events, rate-limiting helpers, data export/deletion operations, and security statistics. Callers must persist/secure data at the application boundary where required; this class's in-memory state is not a durable compliance ledger. |

## Streaming contract

`VoiceStreamingSession::Config` defaults to 16 kHz mono PCM16, partial results enabled, a 64 KiB maximum frame, and a 600-second maximum duration. `origin_allowlist` is unrestricted when empty; when populated, `checkOrigin` requires an exact, case-sensitive match, so normalize the origin before checking it. PCM16 frames must align to sample/channel boundaries. `sendAudioChunk` returns an empty partial result when no result is available or input is rejected; use the registered error callback to observe rejections.

The implementation's default browser transcript is placeholder text unless a partial transcription backend is injected, and the final transcript path is also placeholder-based. Do not treat either result as recognized speech without a real backend integration. See [`IMPLEMENTATION.md`](IMPLEMENTATION.md).

## Session and result semantics

- The session manager uses `ACTIVE`, `IDLE`, `EXPIRED`, `CLOSING`, and `TERMINATED` states; unknown session IDs report `TERMINATED` via `getSessionState`.
- Manager APIs return snapshots or value results; an empty `session_id`, `nullopt`, `false`, no matches, or `UNKNOWN` represents a failed/not-found/low-confidence outcome according to the method.
- The frozen shared session/command semantics and error-prefix expectations are in [`VOICE_SESSION_CONTRACT.md`](../../include/voice/VOICE_SESSION_CONTRACT.md). When a source implementation differs from the frozen contract, treat the source/header implementation and module roadmap as evidence of current behavior and raise the drift for maintainer resolution rather than silently relying on an example.

## Compatibility and validation

The v1.x session and command contract is described as frozen. Changes to lifecycle transitions or failure prefixes require the versioning treatment in `VOICE_SESSION_CONTRACT.md`. Add tests for both return value and side effects at boundary conditions. Existing focused tests include session manager, stream validation, browser streaming, telephony, and auth security suites in [`tests/voice`](../../tests/voice).

For subsystem boundaries and failure flow, see [`ARCHITECTURE.md`](ARCHITECTURE.md); for integration sequencing and validation, see [`MODULE_GUIDE.md`](MODULE_GUIDE.md).
