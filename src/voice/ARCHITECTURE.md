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

> Exact model backend not verifiable from final document text alone.

# Voice Module — Implementation Architecture

This source-backed map describes the current in-tree voice components. It does
not assert production readiness, external service availability, or benchmark
performance. Public contracts are in [`../../include/voice/`](../../include/voice/).

## Components and ownership

| Implementation | Public interface / responsibility |
|---|---|
| `voice_assistant.cpp`, `voice_assistant_llm.cpp` | `VoiceAssistant` orchestration; command, response, optional auth/wake-word and configured processing callbacks |
| `audio_preprocessing.cpp` | `AudioPreprocessingPipeline`; optional RNNoise path and spectral-gate fallback |
| `wake_word_detector.cpp` | PCM wake-word decision path; inspect the header for format/configuration |
| `voice_intent_detector.cpp` | Transcript-text classification and result/context construction |
| `voice_session_manager.cpp` | Session lifecycle, timeout/cleanup operations and optional `ISessionPersistenceBackend` |
| `voice_stream_validator.cpp` | `VoiceStreamValidator` chunk validation |
| `voice_browser_streaming.cpp` | `VoiceStreamingSession`/`VoiceStreamingManager`; callback/backend injection points |
| `voice_telephony.cpp` | In-tree telephony session/bridge and IVR interfaces; not proof of deployed SIP/WebRTC connectivity |
| `voice_authenticator.cpp`, `voice_liveness_checker.cpp`, `voice_liveness_detector.cpp`, `voice_anti_spoof_engine.cpp` | Speaker feature comparison and separate liveness/anti-spoof decision components |
| `voice_security.cpp`, `voice_audit_logger.cpp` | In-process security helper and audit logger behavior |
| `voice_audio_storage.cpp` | Audio records/storage helper; the interface does not itself establish encryption or durable storage |
| Other `src/voice/*.cpp` | Batch processing, accessibility, emotion, macro, meeting, model cache, error handling and TTS customization; see the matching public headers |

## Dataflow boundaries

```text
Caller / service
  ├─ assistant path: audio or text → VoiceAssistant → configured callbacks /
  │  in-tree intent, response and optional synthesis/auth helpers
  ├─ browser path: frames → VoiceStreamingSession → validation and callbacks
  ├─ telephony path: call/session records → telephony bridge / IVR interfaces
  └─ direct components: preprocessing, wake-word, intent, biometric, storage

Application owns: caller authorization, consent, transport decoding/origin policy,
backend provisioning, durable persistence, retention and operational telemetry.
```

The paths are separate interfaces, not a single guaranteed pipeline. The
assistant's configured callbacks and integrations determine which backend work
is actually performed. `VoiceStreamingSession` uses placeholder transcript text
when no transcription callback is provided; the default TTS callback echoes
transcript bytes. Do not treat those defaults as recognition or synthesized
audio. Source: [`voice_browser_streaming.cpp`](voice_browser_streaming.cpp).

## Configuration and build

- `THEMIS_ENABLE_VOICE_ASSISTANT` is declared in `cmake/CMakeLists.txt` and
  defaults to `OFF`; it gates the server voice API handler. The voice
  implementation sources in `cmake/CMakeLists.txt` are gated by
  `THEMIS_ENABLE_LLM` instead. Do not treat these flags as interchangeable.
- `THEMIS_ENABLE_RNNOISE` defaults to `OFF`. The preprocessing implementation
  uses its non-RNNoise fallback when it is not compiled in.
- `VoiceAssistant::Config` defaults `enable_voice_auth` and `enable_wake_word`
  to `false`.
- `VoiceSessionManager` defaults are 5-minute idle timeout, 30-minute maximum
  duration, and 30-second cleanup interval, as declared in
  [`voice_session_manager.h`](../../include/voice/voice_session_manager.h).
- Browser stream defaults and accepted bounds are declared separately in
  [`voice_browser_streaming.h`](../../include/voice/voice_browser_streaming.h);
  validator chunk limits are in
  [`voice_stream_validator.h`](../../include/voice/voice_stream_validator.h).

## Test and benchmark discovery

- `tests/CMakeLists.txt` discovers `tests/voice/CMakeLists.txt` as a module
  directory. The voice file globs `test_*.cpp`, constructs focused unit-test
  targets, and assigns `wave_a`/`release_critical` labels only to the named
  patterns in its label condition. Presence/registration does not show a pass.
- `benchmarks/CMakeLists.txt` conditionally adds `benchmarks/voice`; the module
  globs `bench_*.cpp` and registers them with the standard benchmark helper
  when `THEMIS_BUILD_BENCHMARKS` is enabled (default `OFF` in the root options).
- Current registration details are canonical in
  [`../../tests/voice/CMakeLists.txt`](../../tests/voice/CMakeLists.txt) and
  [`../../benchmarks/voice/CMakeLists.txt`](../../benchmarks/voice/CMakeLists.txt).
  Representative-hardware results and release-gate execution are not inferred
  from CMake registration.

## Limits of this description

There is no source basis here for a universal thread-safety guarantee, a frozen
cross-component state machine, a 100+ stream throughput claim, a complete
security/compliance guarantee, or a particular external STT/TTS provider.
Consult the public headers and implementation for the individual contracts;
record integration-specific claims at the owning service layer.
