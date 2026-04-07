<!-- Status: current | validated: 2026-04-07 | Primary: src/whisper/ -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Whisper Audio Transcription Plugin

Audio transcription plugin for ThemisDB backed by whisper.cpp.

## Module Purpose

Implements real-time and batch audio transcription for ThemisDB. Provides a language-agnostic
`IAudioBackend` interface with a production implementation (`WhisperPlugin`) backed by
[whisper.cpp](https://github.com/ggerganov/whisper.cpp) and a zero-dependency stub mode for
CI environments without a model file.

## Subsystem Scope

**In scope:** WAV audio ingestion, PCM decoding (16-bit + IEEE float32), transcript generation,
automatic language detection, provenance stamp injection, content-policy integration.

**Out of scope:** Audio encoding/re-sampling (external), real-time microphone capture (external),
speaker diarisation (future), integration with the RAG pipeline (handled by `rag` module).

## Relevant Interfaces

- `include/plugins/audio_backend_interface.h` — `IAudioBackend`, `TranscriptionResult`
- `include/whisper/whisper_plugin.h` — `WhisperPlugin` (top-level)
- `include/whisper/whisper_transcriber.h` — `IWhisperTranscriber`, `InMemoryWhisperTranscriber`
- `include/whisper/audio_chunk_reader.h` — `IAudioChunkReader`, `WavAudioChunkReader`
- `include/whisper/whisper_config.h` — `WhisperConfig`

## Current Delivery Status

**Maturity:** 🟡 Beta (v2.0.0) — Core pipeline operational in stub mode. Real whisper.cpp
inference available when `THEMIS_ENABLE_WHISPER=ON` is set and a model file is present.

## Quick Start

```cpp
#include "whisper/whisper_plugin.h"

// Stub mode (no model required)
auto plugin = std::make_unique<themis::audio::WhisperPlugin>();
plugin->initialize("", {});

auto result = plugin->transcribe("recording.wav", {});
if (result.success) {
    std::cout << result.text << "\n";
}

// With injected transcriber (tests)
auto transcriber = std::make_unique<themis::audio::InMemoryWhisperTranscriber>();
transcriber->setNextResult("Hallo Welt");
auto plugin = std::make_unique<themis::audio::WhisperPlugin>(std::move(transcriber));
```

## Architecture Overview

```
┌───────────────────────────────────┐
│         WhisperPlugin             │  ← IAudioBackend
│  ┌─────────────────────────────┐  │
│  │   WavAudioChunkReader       │  │  ← zero-dep WAV parser (RIFF/PCM/float32)
│  └─────────────────────────────┘  │
│  ┌─────────────────────────────┐  │
│  │   IWhisperTranscriber       │  │  ← strategy pattern
│  │   ├── WhisperCppTranscriber │  │     (requires whisper.cpp)
│  │   └── WhisperStubTranscriber│  │     (CI / no model)
│  └─────────────────────────────┘  │
└───────────────────────────────────┘
```

## Build

```cmake
# Stub mode (default – no model file required)
cmake -B build && cmake --build build --target test_whisper_plugin

# Real inference
cmake -B build -DTHEMIS_ENABLE_WHISPER=ON
```

## Test Suite

| Suite | Count | Labels |
|---|---|---|
| `WhisperPluginFocusedTests` | 30 | `plugins;whisper;audio;v2.0.0` |

```bash
ctest -R WhisperPluginFocusedTests --output-on-failure
```

## Dependencies

| Dependency | Required | Purpose |
|---|---|---|
| `nlohmann_json` | ✅ | config / stats |
| `whisper.cpp` | ❌ optional | real model inference |

## Provenance Fields

Every `TranscriptionResult` carries mandatory provenance:

| Field | Value |
|---|---|
| `ingestion_source_type` | `"WHISPER"` |
| `plugin_version` | `"2.0.0"` |
| `generation_timestamp` | Unix epoch milliseconds |
