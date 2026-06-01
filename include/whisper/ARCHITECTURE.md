> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/whisper/ARCHITECTURE.md -->

# Whisper ASR Module — Public Header Architecture

**Module Path:** `include/whisper/`  
**Implementation:** `../../src/whisper/`  
**Canonical architecture doc:** [`../../src/whisper/ARCHITECTURE.md`](../../src/whisper/ARCHITECTURE.md)

---

## 1. Overview

`include/whisper/` defines the **public OpenAI Whisper ASR integration plugin, audio chunking, voice activity detection, transcription, configuration, and plugin registration API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/whisper/ARCHITECTURE.md`](../../src/whisper/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Whisper Plugin

| Header | Public Type | Purpose |
|--------|------------|---------|
| `whisper_plugin.h` | `WhisperPlugin` | Whisper ASR engine plugin contract |
| `whisper_plugin_registrar.h` | `WhisperPluginRegistrar` | Plugin self-registration helper |
| `whisper_config.h` | `WhisperConfig` | Whisper model and inference configuration |
| `whisper_transcriber.h` | `WhisperTranscriber` | Audio transcription entry point |
| `audio_chunk_reader.h` | `AudioChunkReader` | Chunked audio stream reader |
| `voice_activity_detector.h` | `VoiceActivityDetector` | Voice activity detection for pre-filtering |

---

## 3. Namespace Layout

All public types reside in the `themis::whisper` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/whisper/` expose the **stable public API**; internal types live in `src/whisper/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **LLM/Voice**.
