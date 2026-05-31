# Voice Module - Architecture Guide

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

Version: 1.0
Last Updated: 2026-05-31
Module Path: src/voice/

## 1. Overview

The Voice module implements voice input processing, session control, assistant orchestration, and streaming/telephony voice interfaces.

## 2. Architecture Surfaces

| Surface | Source files |
|---|---|
| Assistant and orchestration | src/voice/voice_assistant.cpp, src/voice/voice_assistant_llm.cpp |
| Audio preprocessing and detection | src/voice/audio_preprocessing.cpp, src/voice/wake_word_detector.cpp, src/voice/emotion_analyzer.cpp |
| Session and command handling | src/voice/voice_session_manager.cpp, src/voice/voice_intent_detector.cpp, src/voice/voice_macro_manager.cpp |
| Security and authentication | src/voice/voice_authenticator.cpp, src/voice/voice_security.cpp |
| Streaming and telephony | src/voice/voice_browser_streaming.cpp, src/voice/voice_telephony.cpp |
| Storage and batch processing | src/voice/voice_audio_storage.cpp, src/voice/voice_batch_processor.cpp |
| Accessibility and customization | src/voice/voice_accessibility.cpp, src/voice/voice_tts_customizer.cpp |

## 3. Runtime Control Flow

1. Voice input enters preprocessing and detection paths.
2. Session and intent handlers classify request intent and context.
3. Assistant orchestration routes to command, response, or integration path.
4. Streaming/telephony outputs are emitted with session-state updates.
5. Security and metrics hooks record diagnostics and outcomes.

## 4. Integration Boundaries

| Direction | Integration |
|---|---|
| Used by | API and runtime handlers needing voice interaction |
| Uses | llm/content/security modules and optional backend services |
| Exposes | voice session APIs, command flows, and streaming interfaces |

## 5. Concurrency Model

- voice sessions operate under concurrent request load
- shared caches/session registries are coordinated by module components
- streaming paths enforce bounded chunk/session behavior

## 6. Known Limits

- latency and quality envelopes depend on backend model and hardware profile
- telephony and browser paths are environment-dependent
- some deployment combinations require additional benchmark evidence

## 7. Sourcecode Verification (Module: voice/architecture)

- Verified files:
  - src/voice/voice_assistant.cpp
  - src/voice/audio_preprocessing.cpp
  - src/voice/voice_session_manager.cpp
  - src/voice/voice_authenticator.cpp
  - src/voice/voice_browser_streaming.cpp
  - src/voice/voice_telephony.cpp
  - src/voice/voice_batch_processor.cpp
  - src/voice/wake_word_detector.cpp
- Verified interfaces and behavior:
  - assistant/session orchestration
  - streaming and telephony control flow
  - detection, preprocessing, and auth surfaces
