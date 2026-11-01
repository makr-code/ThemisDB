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

<!-- Status: current | validated: 2026-10-31 | Primary: src/whisper/ -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Whisper Audio Transcription Plugin

Audio transcription plugin for ThemisDB backed by whisper.cpp.

## Module Purpose

Implements file-based and in-memory audio transcription for ThemisDB. It does not provide
microphone capture or a demonstrated real-time inference path. It provides a language-agnostic
`IAudioBackend` interface with an optional whisper.cpp implementation and a zero-dependency stub
backend when whisper.cpp support is not compiled in. A build compiled with whisper.cpp enabled
requires a valid model; an empty model path does not select stub mode.

## Subsystem Scope

**In scope:** WAV ingestion, PCM decoding (16-bit PCM + IEEE float32), optional FFmpeg decoding,
transcript generation, language detection, and provenance fields.

**Out of scope:** Microphone capture and audio encoding are not provided here. Speaker diarisation
has an API, but the current whisper.cpp implementation returns one segment assigned to one speaker;
it does not distinguish speakers. RAG integration is handled outside this module.

## Relevant Interfaces

- `include/plugins/audio_backend_interface.h` — `IAudioBackend`, `TranscriptionResult`
- `include/whisper/whisper_plugin.h` — `WhisperPlugin` (top-level)
- `include/whisper/whisper_transcriber.h` — `IWhisperTranscriber`, `InMemoryWhisperTranscriber`
- `include/whisper/audio_chunk_reader.h` — `IAudioChunkReader`, `WavAudioChunkReader`
- `include/whisper/whisper_config.h` — `WhisperConfig`

## Current Delivery Status

**Implementation:** plugin version `2.3.0`. The stub is selected when whisper.cpp support is not
compiled in; real inference requires `THEMIS_ENABLE_WHISPER=ON`, a linked `libwhisper`, and a model file. The plugin serializes
transcriber calls. `transcribeStream()` supports callbacks, but the default implementation emits
the completed text as one token; the whisper.cpp transcriber does not override it.

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
│  │   ├── WhisperCppTranscriber │  │     (optional; real model)
│  │   └── WhisperStubTranscriber│  │     (empty result without model)
│  └─────────────────────────────┘  │
└───────────────────────────────────┘
```

## Build

```cmake
# Configure the repository, then build the focused test executable
cmake --build <build-dir> --target test_whisper_plugin
ctest --test-dir <build-dir> -R WhisperPluginFocusedTests --output-on-failure

# Optional real inference; requires libwhisper and a model file
cmake -B build -DTHEMIS_ENABLE_WHISPER=ON
```

The repository root test configuration registers `WhisperPluginFocusedTests` (69 cases).
The 12 registrar cases in `src/whisper/tests/test_whisper_plugin_registrar.cpp` are present in
source but are not registered by the current CMake files. The two test files under
`tests/whisper/` are discovered by that directory's CMake file.

## Installation

Build through the repository CMake flow and enable whisper.cpp only when the optional
runtime dependency is available.

## Usage

- Use `transcribeFile(path)` for file-based transcription.
- Use `transcribe(samples, sample_rate)` for in-memory PCM data.
- Use `detectLanguage(samples, sample_rate)` with optional confidence threshold filtering.

## Test Suite

| CTest name / source | Cases | Registration |
|---|---:|---|
| `WhisperPluginFocusedTests` (`src/whisper/tests/test_whisper_plugin.cpp`) | 69 | Root `tests/CMakeLists.txt` |
| `src/whisper/tests/test_whisper_plugin_registrar.cpp` | 12 | Source present; no current CMake target |
| `tests/whisper/test_whisper_stub_transcribe_bridge.cpp` | 3 | `tests/whisper/CMakeLists.txt` |
| `tests/whisper/test_whisper_highcardinality_stress.cpp` | 3 | `tests/whisper/CMakeLists.txt` |

```bash
ctest -R WhisperPluginFocusedTests --output-on-failure
ctest -R 'test_whisper_.*_whisper_FocusedTests' --output-on-failure
```

The latter pattern selects the two discovered tests in `tests/whisper`; exact generated CTest
names depend on the filename-derived registration in `tests/whisper/CMakeLists.txt`.

## Dependencies

| Dependency | Required | Purpose |
|---|---|---|
| `nlohmann_json` | ✅ | config / stats |
| `whisper.cpp` | ❌ optional | real model inference |
| `ffmpeg` (runtime) | ❌ optional | MP3/OGG/FLAC/M4A/AAC/Opus/WMA/WebM decoding via subprocess |

## Provenance Fields

Every `TranscriptionResult` carries mandatory provenance:

| Field | Value |
|---|---|
| `ingestion_source_type` | `"WHISPER"` |
| `plugin_version` | `"2.3.0"` |
| `generation_timestamp` | Unix epoch milliseconds |

Successful transcriptions receive these fields from `WhisperPlugin`; error results also carry
the source type and plugin version, but do not necessarily have a generation timestamp.

## Troubleshooting

**Build fails with undefined reference to whisper_init_from_file**
→ Ensure `THEMIS_ENABLE_WHISPER=ON` is set and `libwhisper` is available at link time. Without the flag the stub transcriber is used and no whisper.cpp symbols are needed.

**`transcribeFile()` returns empty text**
→ Expected for the default stub. It returns an empty transcript with unknown language and zero confidence; real inference requires the optional backend and a valid model.

**`detectLanguage()` returns `"unknown"` with confidence 0**
→ Audio may be too short or silent, or `language_confidence_threshold` in `WhisperConfig` is set above the raw confidence. Lower the threshold or use longer audio.

**FFmpeg reader fails with "ffmpeg not available"**
→ Install `ffmpeg` and add it to `PATH`. WAV files use `WavAudioChunkReader` directly and bypass FFmpeg.

**Concurrent calls wait for one another**
→ Expected: `WhisperPlugin` serializes calls into its transcriber to protect the shared context. Use separate plugin instances for parallel inference.

## Related Documentation

| Document | Description |
|---|---|
| [`../../include/whisper/README.md`](../../include/whisper/README.md) | Public header surface, per-header API reference, config options, usage snippets |
| [`ARCHITECTURE.md`](ARCHITECTURE.md) | Component diagram, data flows, design principles |
| [`ROADMAP.md`](ROADMAP.md) | Feature roadmap, implementation phases, production readiness checklist |
| [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md) | Gaps in genuine incremental inference and multi-speaker attribution |
| [`SECURITY.md`](SECURITY.md) | Security considerations (model loading, FFmpeg subprocess, path traversal) |
| [`../../docs/de/whisper/README.md`](../../docs/de/whisper/README.md) | German-language module overview |
| [`../../docs/en/whisper/README.md`](../../docs/en/whisper/README.md) | English-language module overview |
