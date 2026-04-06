<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · ../../src/voice/ -->

# Voice — Public Header Architecture

## Overview
`include/voice/` exposes the public C++ headers for ThemisDB's voice subsystem. The module covers wake-word detection, voice authentication, emotion analysis, TTS customization, browser/telephony streaming, meeting support, batch processing, accessibility, and session management. Implementation lives in `../../src/voice/`.

## Design Principles
1. **Pipeline composability** — headers define discrete stages (preprocessing → detection → analysis → response) that compose without tight coupling.
2. **Security by design** — authentication (`voice_auth.h`) and security policy (`voice_security.h`) are separate, independently auditable concerns.
3. **Multimodal output** — TTS customization (`voice_tts_customizer.h`) and accessibility (`voice_accessibility.h`) are first-class concerns.
4. **Transport abstraction** — browser streaming and telephony use separate headers, isolating transport-specific logic.
5. **Observability** — error handling (`voice_error_handler.h`) and session management (`voice_session_manager.h`) provide hooks for monitoring.

## Interface Inventory

| Header | Classes / Interfaces | Purpose |
|---|---|---|
| `audio_preprocessing.h` | `AudioPreprocessor` | Noise reduction, normalization, VAD pre-processing |
| `emotion_analyzer.h` | `EmotionAnalyzer` | Emotion classification from audio features |
| `voice_accessibility.h` | `VoiceAccessibility` | Accessibility features (captions, slow-TTS, amplification) |
| `voice_assistant.h` | `VoiceAssistant` | Top-level assistant orchestrator |
| `voice_audio_storage.h` | `VoiceAudioStorage` | Encrypted audio sample storage |
| `voice_auth.h` | `VoiceAuth` | Speaker verification and voice biometric auth |
| `voice_batch_processor.h` | `VoiceBatchProcessor` | Batch audio transcription/analysis |
| `voice_browser_streaming.h` | `VoiceBrowserStreaming` | WebRTC/WebSocket browser audio streaming |
| `voice_error_handler.h` | `VoiceErrorHandler` | Structured voice pipeline error handling |
| `voice_intent_detector.h` | `VoiceIntentDetector` | NLU intent classification from transcription |
| `voice_macro.h` | `VoiceMacro` | Macro/command recording and playback |
| `voice_meeting_support.h` | `VoiceMeetingSupport` | Multi-speaker meeting transcription, diarization |
| `voice_model_cache.h` | `VoiceModelCache` | Model loading/caching (ASR, TTS, emotion) |
| `voice_security.h` | `VoiceSecurity` | Anti-spoofing, liveness detection, policy enforcement |
| `voice_session_manager.h` | `VoiceSessionManager` | Voice session lifecycle (create/resume/terminate) |
| `voice_telephony.h` | `VoiceTelephony` | SIP/PSTN telephony integration |
| `voice_tts_customizer.h` | `VoiceTtsCustomizer` | TTS voice style, rate, pitch, SSML customization |
| `wake_word_detector.h` | `WakeWordDetector` | Low-latency always-on wake-word detection |

## Pipeline Overview
```
Microphone / Telephony / Browser
        ↓
  AudioPreprocessor
        ↓
  WakeWordDetector ──► VoiceAssistant
        ↓
  VoiceAuth / VoiceSecurity
        ↓
  VoiceIntentDetector / EmotionAnalyzer
        ↓
  VoiceTtsCustomizer / VoiceAccessibility
        ↓
  VoiceSessionManager → VoiceAudioStorage
```

Implementation in `../../src/voice/`
