# Voice Module — Frozen Session & Command Contract

**Version:** 1.0.0 (Frozen)  
**Status:** FROZEN — Q3 2026 gate delivery  
**Last Validated:** 2026-08-09  
**Frozen By:** Copilot agent (roadmap gate: Phase 1 Design / API Contract)

---

## 1. Scope

This document freezes the **canonical voice session and command contract**
across assistant, streaming, and telephony paths, and defines **explicit
failure contracts** for invalid audio, auth failure, and unavailable backend
states.

---

## 2. Session Contract

### 2.1 Session Lifecycle

A `VoiceSession` transitions through the following states:

```
IDLE → LISTENING → PROCESSING → RESPONDING → IDLE
                                           └→ ERROR (terminal, session closed)
```

| State | Description |
|---|---|
| IDLE | Session is open but not actively processing |
| LISTENING | Audio input is being accepted (streaming or chunk) |
| PROCESSING | STT / intent detection in progress |
| RESPONDING | TTS generation or response streaming in progress |
| ERROR | Terminal failure state; the session will be closed |

**Session invariants (frozen):**
- Every session has a unique, immutable session ID assigned at creation.
- A session in ERROR state MUST NOT accept further audio or command input.
- Session tear-down (close / timeout) MUST be safe to call from any state.
- Concurrent operations on the same session ID are serialised internally;
  callers need not hold locks.

### 2.2 Session Timeout Contract

- Sessions idle for longer than `VoiceAssistantConfig::session_timeout_seconds`
  MUST be automatically closed.
- Timeout close MUST NOT lose in-flight audio that arrived before the timeout.
- A timeout-closed session MUST NOT be reusable; callers must create a new session.

---

## 3. Command Contract

### 3.1 Intent and Command Semantics

| Phase | Contract |
|---|---|
| Wake-word detection | Returns `true` exactly when the configured wake-word phrase is detected above threshold; no side effects |
| Intent detection | Returns the highest-confidence `IntentCategory`; returns `UNKNOWN` rather than throwing when confidence is below threshold |
| Command dispatch | Executes at most one command per invocation; returns a `CommandResult` regardless of execution outcome |

### 3.2 Streaming Command Contract

- `transcribeStream()` delivers incremental tokens via callback before the
  final result is available.
- Partial (streaming) tokens MUST NOT be forwarded to intent/command dispatch —
  only the final transcript triggers dispatch.
- If the streaming callback throws, the exception is swallowed and logged;
  the transcription continues to completion.

---

## 4. Failure Contracts

### 4.1 Invalid Audio

| Condition | Behaviour |
|---|---|
| Empty audio buffer (0 bytes) | Return empty `TranscriptionResult` with `success=false`, `error_message` containing `"[INVALID_AUDIO] empty buffer"` |
| Audio format not recognised (not WAV/MP3/OGG) | Return `success=false`, `error_message` containing `"[INVALID_AUDIO] unsupported format"` |
| Truncated WAV (missing header fields) | Return `success=false`, `error_message` containing `"[INVALID_AUDIO] truncated WAV"` |
| Audio exceeds max size (500 MB gate) | Return `success=false`, `error_message` containing `"[INVALID_AUDIO] exceeds max size"` |

**Invariant:** Invalid audio MUST NEVER cause the session to enter the ERROR
terminal state — the session remains usable after an invalid-audio failure.

### 4.2 Auth Failure

| Condition | Behaviour |
|---|---|
| User not authenticated | Session open request rejected; `VoiceSession` is NOT created |
| Rate limit exceeded | Auth failure recorded; `recordAuthFailure()` returns `true` when the user is locked out; session open rejected |
| PII exposure check fails | Session open rejected; error logged with event type `pii_exposure` |

**Invariant:** Auth failures are **non-retryable** within the same request.
The caller must obtain valid credentials before retrying.

### 4.3 Backend Unavailable

| Backend | Unavailability Behaviour |
|---|---|
| STT backend (Whisper) | Transcription returns `success=false`, `error_message` containing `"[BACKEND_UNAVAILABLE] STT"` |
| TTS backend | Response streaming falls back to text-only response if available; otherwise session enters ERROR |
| Intent engine | Falls back to `IntentCategory::UNKNOWN` with a `confidence=0.0` result; does NOT throw |
| Wake-word detector | Falls back to always-listening mode (no wake-word required) with a WARN log |

**Invariant:** Backend unavailability for STT/TTS MUST be logged at WARN level
with the session ID and backend type. The session MUST NOT be silently abandoned.

---

## 5. Telephony / Browser-Streaming Paths

- Telephony sessions follow the same session lifecycle as § 2.1.
- Browser-streaming sessions (`VoiceBrowserStreaming`) use chunked audio
  delivery; each chunk follows the invalid-audio contract in § 4.1.
- Both paths MUST emit a `session_end` event when the session is closed,
  regardless of cause (timeout, error, or explicit close).

---

## 6. Backward Compatibility

- Session lifecycle states and command semantics are frozen for the v1.x line.
- Failure contract `error_message` prefix tags (`[INVALID_AUDIO]`, etc.) are
  frozen and may be used by callers for structured parsing.
- New backend types may be added without a major version bump.
- Changes to existing state transitions or failure message prefixes require
  a v2.x version bump.
