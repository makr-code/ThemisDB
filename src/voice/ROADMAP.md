<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Voice Module Roadmap

## Current Status
v1.x – Functional voice assistant system. VoiceAssistant orchestrator with Whisper-based STT, llama.cpp TTS/LLM integration, session management, phone call transcription, and meeting protocol generation are implemented.

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

## In Progress 🚧
- [I] Multi-speaker diarization improvements (Target: Q3 2026) (Issue: #2497)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] Voice command macros (user-defined shortcuts to AQL queries) (Issue: #1981)
- [I] Language detection and automatic locale switching (Issue: #2492)
- [I] Noise suppression preprocessing (RNNoise integration) (Issue: #2041)
- [I] WebSocket audio streaming endpoint for browser clients (Issue: #2350)
- [I] Voice session playback and search in stored transcripts (Issue: #2077)

### Long-term (6-12 months)
- [I] Multi-language TTS (German, French, Spanish voices) (Issue: #2127)
- [!] Emotion / sentiment detection from voice tone (Issue: #2493)
- [x] Voice biometric authentication (speaker verification) (Issue: #2494)
- [I] Real-time meeting transcription with action-item extraction (Issue: #2353)
- [!] Integration with telephony systems (SIP / WebRTC) (Issue: #2495)

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
- [~] Multi-speaker diarization improvements

### Phase 3: Voice Macros & Browser Streaming (Status: Planned 📋)
- [ ] Voice command macros (user-defined shortcuts to AQL queries)
- [ ] Language detection and automatic locale switching
- [ ] Noise suppression preprocessing (RNNoise integration)
- [ ] WebSocket audio streaming endpoint for browser clients
- [ ] Voice session playback and search in stored transcripts

### Phase 4: Multi-Language TTS & Biometric Authentication (Status: Planned 📋)
- [ ] Multi-language TTS (German, French, Spanish voices)
- [ ] Emotion / sentiment detection from voice tone
- [x] Voice biometric authentication (speaker verification)
- [ ] Real-time meeting transcription with action-item extraction
- [ ] Integration with telephony systems (SIP / WebRTC)

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #2355)
- [I] Integration tests (full pipeline: audio in → transcription → AQL → audio out) (Issue: #2356)
- [I] Performance benchmarks (STT latency, TTS generation speed) (Issue: #2357)
- [I] Security audit (audio data storage, transcription PII handling) (Issue: #2358)
- [I] Documentation complete (Issue: #2359)
- [I] API stability guaranteed (Issue: #2360)

## Known Issues & Limitations
- Streaming STT operates in sliding-window mode (3 s window, 1 s step); true sample-by-sample streaming requires Whisper.cpp `THEMIS_ENABLE_WHISPER` build flag.
- Wake-word detection uses energy-based VAD gating and acoustic feature scoring
  (density, spectral centroid, crest factor). A neural wake-word model backend
  (e.g. Porcupine, openWakeWord) can be plugged in via `WakeWordDetector::scorePhrase()`
  without API changes.
- Multi-speaker diarization accuracy degrades with more than 4 simultaneous speakers.
- TTS voice quality depends on the llama.cpp model in use.

## Breaking Changes
- VoiceAssistant session API is stable from v1.x.
- Audio format configuration (sample rate, encoding) may gain new options in v1.5.0; backward-compatible.
