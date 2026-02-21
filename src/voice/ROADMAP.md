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

## In Progress 🚧
- [~] Real-time streaming STT (word-by-word transcription as audio arrives) (Target: Q2 2026)
- [~] Wake-word detection for hands-free activation (Target: Q2 2026)
- [~] Multi-speaker diarization improvements (Target: Q3 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] Voice command macros (user-defined shortcuts to AQL queries) (Issue: #1981)
- [ ] Language detection and automatic locale switching
- [ ] Noise suppression preprocessing (RNNoise integration)
- [ ] WebSocket audio streaming endpoint for browser clients
- [ ] Voice session playback and search in stored transcripts

### Long-term (6-12 months)
- [ ] Multi-language TTS (German, French, Spanish voices)
- [ ] Emotion / sentiment detection from voice tone
- [ ] Voice biometric authentication (speaker verification)
- [ ] Real-time meeting transcription with action-item extraction
- [ ] Integration with telephony systems (SIP / WebRTC)

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

### Phase 2: Streaming STT & Wake-Word Detection (Status: In Progress 🚧)
- [~] Real-time streaming STT (word-by-word transcription as audio arrives)
- [~] Wake-word detection for hands-free activation
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
- [ ] Voice biometric authentication (speaker verification)
- [ ] Real-time meeting transcription with action-item extraction
- [ ] Integration with telephony systems (SIP / WebRTC)

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [ ] Integration tests (full pipeline: audio in → transcription → AQL → audio out)
- [ ] Performance benchmarks (STT latency, TTS generation speed)
- [ ] Security audit (audio data storage, transcription PII handling)
- [ ] Documentation complete
- [ ] API stability guaranteed

## Known Issues & Limitations
- Streaming (real-time word-by-word) STT is not yet implemented; batch mode only.
- Wake-word detection requires an additional lightweight model; not yet integrated.
- Multi-speaker diarization accuracy degrades with more than 4 simultaneous speakers.
- TTS voice quality depends on the llama.cpp model in use.

## Breaking Changes
- VoiceAssistant session API is stable from v1.x.
- Audio format configuration (sample rate, encoding) may gain new options in v1.5.0; backward-compatible.
