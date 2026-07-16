# ThemisDB Voice Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The Voice module provides speech-driven interaction, session orchestration, and voice-runtime utilities for ThemisDB.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| voice_assistant.cpp | voice interaction orchestrator |
| voice_assistant_llm.cpp | intent and response integration with LLM runtime |
| audio_preprocessing.cpp | audio preprocessing and normalization pipeline |
| voice_intent_detector.cpp | intent detection from transcript text |
| voice_session_manager.cpp | session lifecycle and context handling |
| wake_word_detector.cpp | wake-word and chunk-level detection flow |
| voice_authenticator.cpp | speaker verification and liveness-related checks |
| voice_browser_streaming.cpp | browser streaming session handling |
| voice_telephony.cpp | telephony session bridge (SIP and WebRTC) |
| voice_batch_processor.cpp | batch-oriented voice workload processing |

## Scope

In scope:
- speech command orchestration and voice sessions
- wake-word, audio preprocessing, and intent detection
- browser/telephony streaming integration
- authentication and security-related voice controls
- voice-specific observability and processing utilities

Out of scope:
- external STT/TTS model lifecycle internals
- non-voice transport stack bootstrap logic
- storage engine internals outside voice integration surfaces

## Known Limitations

- runtime behavior varies with configured backend and deployment profile
- low-latency performance depends on model size and hardware availability
- some advanced anti-spoofing and hardening paths require continuous tuning

## Sourcecode Verification (Module: voice/readme)

- Verified files:
  - src/voice/voice_assistant.cpp
  - src/voice/voice_assistant_llm.cpp
  - src/voice/audio_preprocessing.cpp
  - src/voice/voice_intent_detector.cpp
  - src/voice/voice_session_manager.cpp
  - src/voice/wake_word_detector.cpp
  - src/voice/voice_authenticator.cpp
  - src/voice/voice_browser_streaming.cpp
  - src/voice/voice_telephony.cpp
  - src/voice/voice_batch_processor.cpp
- Verified behavior surfaces:
  - voice request/session orchestration
  - preprocessing and wake-word paths
  - streaming, telephony, and auth integration
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - implementation history remains in CHANGELOG.md
