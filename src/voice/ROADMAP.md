> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Voice Module Roadmap

## Current Status
v1.1.0 – Production-ready voice assistant system. VoiceAssistant orchestrator with Whisper-based STT, llama.cpp TTS/LLM integration, session management, phone call transcription, meeting protocol generation, real-time browser WebSocket streaming, voice biometric authentication, and telephony bridge (SIP/WebRTC) are all implemented.

## Completed ✅
- [x] VoiceAssistant – central coordinator for all voice interaction
- [x] STT processing via Whisper AI (speaker diarization, timestamps)
- [x] LLM integration via EmbeddedLLM / LlamaWrapper (intent recognition, query generation, response generation)
- [x] TTS synthesis with audio format output
- [x] Voice command processing pipeline (audio → STT → LLM → TTS → audio)
- [x] Session state and conversation history management
- [x] Context-aware conversational AI
- [x] Phone call recording and transcription
- [x] Meeting protocol generation
- [x] Voice-based database query interface
- [x] Storage and retrieval of voice session data
- [x] Key point and summary extraction
- [x] Real-time streaming STT (word-by-word transcription as audio arrives) (Issue: #2496)
- [x] Wake-word detection for hands-free activation (Issue: #2365)
- [x] Voice biometric authentication (speaker verification) (Issue: #2494)
- [x] Multi-speaker diarization improvements (Issue: #2497)
- [x] WebSocket audio streaming endpoint for browser clients (Issue: #2350)
- [x] Integration with telephony systems — SIP call sessions, WebRTC peer connections, IVR engine, TelephonyBridge coordinator (Issue: #2495)

## In Progress 🚧
- (none)

## Planned Features 📋

### Long-term (6-12 months)
- [ ] Federated learning for on-device voice model personalisation (Target: Q3 2026)
- [ ] GPU-accelerated noise suppression and codec processing (Target: Q4 2026)

## Implementation Phases

### Phase 1: Voice Pipeline & Session Management (Status: Completed ✅)
- [x] `VoiceAssistant` – central coordinator for all voice interaction
- [x] STT processing via Whisper AI (speaker diarization, timestamps)
- [x] LLM integration via `EmbeddedLLM` / `LlamaWrapper` (intent recognition, query generation, response generation)
- [x] TTS synthesis with audio format output
- [x] Voice command processing pipeline (audio → STT → LLM → TTS → audio)
- [x] Session state and conversation history management
- [x] Context-aware conversational AI
- [x] Phone call recording and transcription
- [x] Meeting protocol generation
- [x] Voice-based database query interface
- [x] Storage and retrieval of voice session data
- [x] Key point and summary extraction

### Phase 2: Streaming STT & Wake-Word Detection (Status: Completed ✅)
- [x] Real-time streaming STT (word-by-word transcription as audio arrives)
- [x] Wake-word detection for hands-free activation
- [x] Multi-speaker diarization improvements

### Phase 3: Voice Macros & Browser Streaming (Status: Completed ✅)
- [x] Voice command macros (user-defined shortcuts to AQL queries)
- [x] Language detection and automatic locale switching
- [x] Noise suppression preprocessing (RNNoise integration)
- [x] WebSocket audio streaming endpoint for browser clients (Issue: #2350)
- [x] Voice session playback and search in stored transcripts

### Phase 4: Multi-Language TTS & Biometric Authentication (Status: Completed ✅)
- [x] Multi-language TTS (German, French, Spanish voices)
- [x] Emotion / sentiment detection from voice tone
- [x] Voice biometric authentication (speaker verification)
- [x] Real-time meeting transcription with action-item extraction (Target: Q1 2026)
- [x] Integration with telephony systems (SIP / WebRTC) (Issue: #2495)

## Production Readiness Checklist
- [x] Unit tests coverage > 80% (Issue: #2355) — `test_voice_assistant.cpp`, `test_voice_coverage.cpp`, `test_voice_production.cpp` (496+ tests); focused targets: `VoiceProductionFocusedTests`, `VoiceCoverageFocusedTests`
- [x] Integration tests (full pipeline: audio in → transcription → AQL → audio out) (Issue: #2356) — `VoiceProductionFocusedTests`
- [x] Performance benchmarks (STT latency, TTS generation speed) (Issue: #2357) — `benchmarks/bench_voice_assistant.cpp`
- [I] Security audit (audio data storage, transcription PII handling) (Issue: #2358)
- [I] Documentation complete (Issue: #2359)
- [x] API stability guaranteed (Issue: #2360) — VoiceAssistant session API stable from v1.x; new v1.1.0 APIs (telephony, biometric, browser streaming) marked stable
- [x] Standalone focused test targets registered in `tests/CMakeLists.txt`: `VoiceProductionFocusedTests`, `VoiceCoverageFocusedTests`, `VoiceAssistantFocusedTests` (LLM-gated), `VoiceBrowserStreamingFocusedTests`, `VoiceTelephonyFocusedTests`
- [x] CI workflow registered — `.github/workflows/voice-module-ci.yml` (VoiceProductionFocusedTests, VoiceCoverageFocusedTests, VoiceBrowserStreamingFocusedTests, VoiceTelephonyFocusedTests)

## Known Issues & Limitations
- Streaming STT operates in sliding-window mode (3 s window, 1 s step); true sample-by-sample streaming requires Whisper.cpp `THEMIS_ENABLE_WHISPER` build flag.
- Wake-word detection uses energy-based VAD gating and acoustic feature scoring
  (density, spectral centroid, crest factor). A neural wake-word model backend
  (e.g. Porcupine, openWakeWord) can be plugged in via `WakeWordDetector::scorePhrase()`
  without API changes.
- Multi-speaker diarization uses k-means++ clustering on sub-band acoustic features (RMS + ZCR). Accuracy degrades with more than 4 simultaneous speakers; a neural embedding backend (e.g., pyannote-style x-vector) can be substituted via `diarizeSegments()` without API changes.
- TTS voice quality depends on the llama.cpp model in use.
- Voice biometric authentication uses acoustic sub-band features (no external model required). A neural i-vector/x-vector backend can be plugged in via `VoiceBiometricAuthenticator`'s internal `extractFeatures()` without changing the public API. Liveness detection is heuristic-based (crest factor, spectral flatness, ZCR variability); a neural anti-spoofing model is recommended for production.

## Breaking Changes
- VoiceAssistant session API is stable from v1.x.
- Audio format configuration (sample rate, encoding) may gain new options in v1.5.0; backward-compatible.
