---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-10-31 -->
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
| `whisper_transcriber.h` | `IWhisperTranscriber`, `WhisperCppTranscriber` (conditional), `WhisperStubTranscriber`, `InMemoryWhisperTranscriber`, diarisation types | Inference strategy, fallback, test double, and optional diarisation API |
| `audio_chunk_reader.h` | `IAudioChunkReader`, `WavAudioChunkReader`, `FfmpegAudioChunkReader`, `CompositeAudioChunkReader` | Whole-file audio decoding to PCM |
| `voice_activity_detector.h` | `IVoiceActivityDetector`, `EnergyThresholdVad`, `VadConfig`, `SpeechSegment` | RMS-threshold speech segment detection |

---

## 3. Namespace Layout

Public whisper types reside in `themis::whisper`; shared result and callback types such as
`TranscriptionResult`, `TranscriptionToken`, and `StreamCallback` reside in `themis::audio`.

---

## 4. Contract Notes

- Headers in `include/whisper/` declare the public API surface; implementations are under `src/whisper/`.
- The existence of a declaration does not imply a complete real-model implementation. In particular,
  whisper.cpp inherits the default `transcribeStream()` implementation (one completed-text token),
  and its diarisation implementation returns one whole-transcript segment for one speaker.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **LLM/Voice**.
