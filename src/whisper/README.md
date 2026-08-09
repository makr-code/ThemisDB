> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

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

**Maturity:** 🟢 Production-ready (v2.3.0 implementation state) — Core pipeline operational in
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
| `WhisperPluginFocusedTests` | 69 | `plugins;whisper;audio;v2.3.0` |
| `WhisperPluginRegistrarTests` | 12 | `plugins;whisper;registrar` |

```bash
ctest -R WhisperPluginFocusedTests --output-on-failure
ctest -R WhisperPluginRegistrarTests --output-on-failure
```

## Dependencies

| Dependency | Required | Purpose |
|---|---|---|
| `nlohmann_json` | ✅ | config / stats |
| `whisper.cpp` | ❌ optional | real model inference |
| `ffmpeg` (runtime) | ❌ optional | MP3/OGG/FLAC decoding via subprocess |

## Provenance Fields

Every `TranscriptionResult` carries mandatory provenance:

| Field | Value |
|---|---|
| `ingestion_source_type` | `"WHISPER"` |
| `plugin_version` | `"2.3.0"` |
| `generation_timestamp` | Unix epoch milliseconds |

## Troubleshooting

**Build fails with undefined reference to whisper_init_from_file**
→ Ensure `THEMIS_ENABLE_WHISPER=ON` is set and `libwhisper` is available at link time. Without the flag the stub transcriber is used and no whisper.cpp symbols are needed.

**`transcribeFile()` returns empty text in CI**
→ Expected for stub mode. The `WhisperStubTranscriber` always returns empty text unless a real model is loaded (`THEMIS_ENABLE_WHISPER=ON` + valid `model_path`).

**`detectLanguage()` returns `"unknown"` with confidence 0**
→ Audio may be too short or silent, or `language_confidence_threshold` in `WhisperConfig` is set above the raw confidence. Lower the threshold or use longer audio.

**FFmpeg reader fails with "ffmpeg not available"**
→ Install `ffmpeg` and add it to `PATH`. WAV files use `WavAudioChunkReader` directly and bypass FFmpeg.

**Tests hang or timeout**
→ Run with `-j 1` or reduce `n_threads` in `WhisperConfig`. Concurrent whisper.cpp calls require one `whisper_context*` per thread.

## Related Documentation

| Document | Description |
|---|---|
| [`../../include/whisper/README.md`](../../include/whisper/README.md) | Public header surface, per-header API reference, config options, usage snippets |
| [`ARCHITECTURE.md`](ARCHITECTURE.md) | Component diagram, data flows, design principles |
| [`ROADMAP.md`](ROADMAP.md) | Feature roadmap, implementation phases, production readiness checklist |
| [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md) | Planned enhancements (streaming, diarisation, VAD) with design constraints |
| [`SECURITY.md`](SECURITY.md) | Security considerations (model loading, FFmpeg subprocess, path traversal) |
| [`../../docs/de/whisper/README.md`](../../docs/de/whisper/README.md) | German-language module overview |
| [`../../docs/en/whisper/README.md`](../../docs/en/whisper/README.md) | English-language module overview |
