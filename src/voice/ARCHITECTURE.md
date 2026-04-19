> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Voice Module — Architecture Guide

**Version:** 1.1
**Last Updated:** 2026-04-06
**Module Path:** `src/voice/`

---

## 1. Overview

The Voice module provides ThemisDB's voice and audio interface: speech-to-text
transcription (OpenAI Whisper via `src/content/`), voice-to-AQL query generation, voice
assistant with conversational AI, voice authentication, wake word detection, batch audio
processing, meeting support (phone call transcription, protocol generation), real-time
WebSocket audio streaming for browser clients, emotion/sentiment analysis from voice
tone, user-defined voice macros, and accessibility features.

The module is production-ready (v1.1.0); Whisper integration requires `THEMIS_ENABLE_WHISPER` for full accuracy.

---

## 2. Design Principles

- **Whisper-First STT** – speech-to-text uses the Whisper model (via `content::STTProcessor`)
  for high-accuracy multilingual transcription.
- **LLM-Powered NLU** – the transcribed text is passed to `EmbeddedLLM` for intent
  recognition and AQL query generation.
- **Session Continuity** – `voice_session_manager.cpp` maintains conversation context
  across multiple utterances.
- **Security by Default** – `voice_authenticator.cpp` verifies speaker identity before
  processing privileged commands; `voice_security.cpp` validates audio inputs.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `voice_assistant.cpp` | Main orchestrator: STT → NLU → AQL → execute → respond |
| `voice_assistant_llm.cpp` | LLM integration for intent parsing and response generation |
| `audio_preprocessing.cpp` | Audio normalization, noise reduction, VAD |
| `voice_session_manager.cpp` | Session state and conversation history management |
| `voice_intent_detector.cpp` | Intent classification from transcribed text |
| `voice_error_handler.cpp` | Structured error handling and user feedback |
| `voice_model_cache.cpp` | Whisper model caching and warm-up |
| `voice_audio_storage.cpp` | Audio recording storage and retrieval |
| `voice_meeting_support.cpp` | Meeting transcription and protocol generation |
| `voice_batch_processor.cpp` | Batch audio file processing |
| `voice_authenticator.cpp` | Speaker identity verification |
| `voice_security.cpp` | Audio input validation and safety checks |
| `voice_accessibility.cpp` | Accessibility: slower speech, simplified output |
| `voice_tts_customizer.cpp` | TTS voice customization settings |
| `wake_word_detector.cpp` | Wake word detection ("Hey Themis") |
| `emotion_analyzer.cpp` | Emotion and sentiment detection from voice tone |
| `voice_browser_streaming.cpp` | WebSocket bidirectional audio streaming for browser clients (Issue #2350) |
| `voice_macro_manager.cpp` | User-defined voice command macros mapped to AQL queries |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                    Audio Input                                   │
│   microphone stream / uploaded audio file / phone call          │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                  AudioPreprocessing                              │
│   noise reduction → VAD → normalize → chunk                     │
└──────────────────────────┬──────────────────────────────────────┘
                           │ audio chunks
┌──────────────────────────▼──────────────────────────────────────┐
│              content::STTProcessor (Whisper)                    │
│   speech → transcript + speaker diarization + timestamps        │
└──────────────────────────┬──────────────────────────────────────┘
                           │ text
┌──────────────────────────▼──────────────────────────────────────┐
│                  VoiceIntentDetector                             │
│   intent: "QUERY", "ADMIN", "CHITCHAT", "EXIT"                  │
└──────────────────────────┬──────────────────────────────────────┘
                           │ intent + text
┌──────────────────────────▼──────────────────────────────────────┐
│              VoiceAssistantLLM (EmbeddedLLM)                    │
│   NL → AQL translation / conversational response               │
└──────────────────────────┬──────────────────────────────────────┘
                           │ AQL or response text
┌──────────────────────────▼──────────────────────────────────────┐
│              QueryEngine / TTS response                          │
└──────────────────────────────────────────────────────────────────┘
```

---

## 4. Data Flow

### 4.1 Voice Query

```
User says: "Show me all users older than 30"
    │
    ├─ wake_word_detector: "Hey Themis" heard
    ├─ audio_preprocessing: VAD → chunk boundary detected
    ├─ STTProcessor: Whisper → "Show me all users older than 30"
    ├─ voice_authenticator: verify speaker identity (optional)
    ├─ voice_intent_detector: intent = QUERY
    ├─ voice_assistant_llm: NL → AQL:
    │       "FOR u IN users FILTER u.age > 30 RETURN u"
    ├─ query_engine.execute(aql) → results
    ├─ voice_assistant_llm: format results as spoken response
    └─ TTS: speak response
```

### 4.2 Meeting Transcription

```
audio_file = "meeting_2026-02-24.wav"
    │
    ├─ voice_batch_processor.process(audio_file)
    ├─ STTProcessor: full transcription + speaker diarization
    ├─ voice_meeting_support.generateProtocol(transcript):
    │       → LLM: extract action items, decisions, attendees
    │       → return structured meeting protocol
    └─ voice_audio_storage.store(audio_file, transcript, protocol)
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Uses** | `src/content/` | STT (Whisper) and audio processing |
| **Uses** | `src/llm/` | NL-to-AQL via EmbeddedLLM |
| **Uses** | `src/query/` | AQL query execution |
| **Provides to** | `src/server/` | Voice API endpoints |
| **Uses** | `src/security/` | Voice authentication and security checks |

---

## 6. Threading & Concurrency Model

- Audio preprocessing and Whisper inference run on a dedicated audio thread.
- `VoiceSessionManager` handles concurrent sessions; each session is isolated.
- `WakeWordDetector` runs on a low-priority background thread.
- Batch processing uses the shared thread pool (`src/utils/thread_pool_manager.cpp`).

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| Voice activity detection | Avoids processing silence; reduces latency |
| Model warm-up | Whisper model loaded at startup into `voice_model_cache.cpp` |
| Streaming transcription | Whisper processes audio in overlapping chunks |
| Batch processing | Multiple audio files processed concurrently |

---

## 8. Security Considerations

- `voice_authenticator.cpp` verifies speaker identity before privileged commands.
- `voice_security.cpp` validates audio inputs to prevent adversarial audio attacks.
- Transcripts containing PII are processed through `pii_detector` before storage.
- Audio storage uses the same field-level encryption as document storage.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `voice.whisper.model` | "base" | Whisper model size (tiny/base/small/medium/large) |
| `voice.wake_word.enabled` | false | Enable wake word detection |
| `voice.authentication.enabled` | false | Enable speaker authentication |
| `voice.tts.enabled` | false | Enable text-to-speech response |
| `voice.session.timeout_s` | 300 | Session idle timeout |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| STT transcription failure | Return error to caller; log |
| NL-to-AQL failure | Respond "I didn't understand that, please rephrase" |
| AQL execution failure | Speak error description |
| Authentication failure | Reject command; log security event |
| Whisper model not loaded | Return 503 |

---

## 11. Known Limitations & Future Work

- Voice module core is production-ready (v1.1.0); advanced hardening work remains for anti-spoofing and model-quality tuning.
- TTS quality and latency depend on configured runtime model and deployment hardware.
- Wake-word detection is implemented with heuristic scoring; optional neural wake-word backends remain future enhancements.
- Real-time streaming transcription latency depends on Whisper model size and hardware.

---

## 12. References

- `src/voice/README.md` — module overview
- `docs/voice/` — voice feature documentation
- `src/content/README.md` — STT implementation details
- `ARCHITECTURE.md` (root) — full system architecture
