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

# Voice Implementation Guide

> Exact model backend not verifiable from final document text alone.

**Audience:** Contributors changing `src/voice` or integrating internal voice components. Public declarations live under [`include/voice`](../../include/voice); implementation ownership remains in this directory.

## Runtime path

1. `VoiceAssistant` owns the orchestration between STT, intent/LLM response, TTS, session data, optional wake-word/auth, security, macros, and audio storage.
2. `AudioPreprocessingPipeline` and `WakeWordDetector` accept PCM-oriented input; format validation and transport decoding must be handled at the boundary that owns those formats.
3. `VoiceIntentDetector` consumes transcript text; it returns an `IntentResult` with category, confidence, entities, and optional context hints.
4. `VoiceSessionManager` maintains session state and optionally delegates persistence to `ISessionPersistenceBackend`. Its default backend is in-memory; `InMemorySessionBackend` loses data on process termination.
5. Browser streaming uses `VoiceStreamingSession` / `VoiceStreamingManager`; telephony integration has its own `VoiceTelephonyManager` surface. Do not assume one path's payload limits apply to the others.

## Limits are surface-specific

| Surface | Source-defined constraint |
|---|---|
| `VoiceStreamValidator` | Non-empty chunks, maximum 16 MiB per chunk, known sample rates and bit depths, 1–8 channels, sequential chunk numbers, maximum stream timestamp of one hour |
| `VoiceStreamingSession` | Maximum frame size defaults to 65,536 bytes and cannot exceed that; default max duration is 600 seconds and may be configured up to 3,600 seconds; internal accumulated-audio bound is 50 MiB |
| `VoiceSessionManager` | Default idle timeout 5 minutes, maximum session duration 30 minutes, cleanup interval 30 seconds; documented manager limit 1,000 concurrent sessions |
| `VoiceStreamingManager` | Default active-session limit 200; creation returns an empty ID when at capacity |

These limits belong to distinct classes and are not interchangeable. The frozen [`VOICE_SESSION_CONTRACT.md`](../../include/voice/VOICE_SESSION_CONTRACT.md) describes the shared lifecycle and failure expectations, while implementation-specific limits are in the headers and sources linked above.

## Failure and concurrency invariants

- Reject invalid, empty, oversized, malformed, or out-of-sequence input at the owning boundary; do not continue downstream with partially validated bytes.
- A completed stream rejects additional chunks. Session close/timeout must release buffers and prevent reuse of terminal sessions.
- Intent and wake-word timeout defaults are non-positive decisions (`UNKNOWN` / not detected). Authentication and liveness failures must not be converted into success by fallback.
- Use manager APIs for session synchronization; do not access internal session maps or add cross-session shared mutable state.
- Browser streaming callbacks are invoked directly from the streaming code paths in this implementation. Keep callbacks bounded and non-blocking, and protect any shared state they update.

## Backend integration caveat

The browser stream implementation contains an always-active placeholder transcript when no partial STT callback is injected; finalization also constructs placeholder transcript text rather than invoking a final transcription callback. It must not be mistaken for production speech recognition. Integrators must wire a real supported backend and verify partial and final transcript behavior before enabling that path for users. This is an existing implementation limitation, not a new fallback to extend.

RNNoise is also build-conditional: when `THEMIS_ENABLE_RNNOISE` is absent, preprocessing uses a simpler spectral-gate implementation. `VoiceAssistant::makeFFmpegAudioConvertFn()` returns an empty function when FFmpeg is unavailable; the current conversion path can then pass through original bytes. Confirm the compiled feature set and validate output format at integration boundaries.

## Change and test workflow

For a change to a contract, update the corresponding header comments and this guide in the same patch. Extend the nearest tests under [`tests/voice`](../../tests/voice), including boundary, invalid-input, timeout/unavailable-backend, and teardown cases. For performance-sensitive changes, use the voice benchmarks under [`benchmarks/voice`](../../benchmarks/voice); a registered benchmark is not evidence that a representative-hardware baseline has passed.

See [`API.md`](API.md), [`MODULE_GUIDE.md`](MODULE_GUIDE.md), and [`ARCHITECTURE.md`](ARCHITECTURE.md) for the adjacent contract and flow descriptions.
