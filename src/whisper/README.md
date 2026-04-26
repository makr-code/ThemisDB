> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

<!-- Status: current | validated: 2026-04-16 | Primary: src/whisper/ -->
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

**Maturity:** 🟢 Production-ready (v2.1.0 implementation state) — Core pipeline operational in
stub mode and with optional `whisper.cpp`. Thread-safe `transcribe()` / `detectLanguage()` and
multi-format decoding (`WAV` + `FFmpeg` reader chain) are implemented.

## Quick Start

```cpp
#include "whisper/whisper_plugin.h"

// Stub mode (no model required)
auto plugin = std::make_unique<themis::whisper::WhisperPlugin>();
plugin->initialize("", {});

auto result = plugin->transcribeFile("recording.wav");
if (result.success) {
    std::cout << result.text << "\n";
}

// With injected transcriber (tests)
auto transcriber = std::make_unique<themis::whisper::InMemoryWhisperTranscriber>();
themis::audio::TranscriptionResult preset;
preset.text = "Hallo Welt";
transcriber->setNextResult(preset);
auto plugin = std::make_unique<themis::whisper::WhisperPlugin>(
    std::move(transcriber),
    std::make_unique<themis::whisper::WavAudioChunkReader>());
```

## Architecture Overview

```
┌───────────────────────────────────┐
│         WhisperPlugin             │  ← IAudioBackend
│  ┌─────────────────────────────┐  │
│  │   CompositeAudioChunkReader │  │  ← dispatches WAV + FFmpeg reader
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

## Installation

Build through the repository CMake flow and enable whisper.cpp only when the optional
runtime dependency is available.

## Usage

- Use `transcribeFile(path)` for file-based transcription.
- Use `transcribe(samples, sample_rate)` for in-memory PCM data.
- Use `detectLanguage(samples, sample_rate)` with optional confidence threshold filtering.

## Test Suite

| Suite | Count | Labels |
|---|---|---|
| `WhisperPluginFocusedTests` | 44 | `plugins;whisper;audio;v2.1.0` |

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
