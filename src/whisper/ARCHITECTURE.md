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

# Whisper Plugin — Architecture Guide

<!-- Status: current | validated: 2026-10-31 | Primary: src/whisper/ -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

**Version:** 1.0
**Last Updated:** 2026-09-09
**Module Path:** `src/whisper/`

---

## 1. Overview

The Whisper plugin integrates [whisper.cpp](https://github.com/ggerganov/whisper.cpp) into
ThemisDB's plugin system as an `IAudioBackend`. It converts raw WAV audio into text
transcriptions enriched with mandatory provenance metadata. The plugin is structured around
three orthogonal concerns:

1. **Audio ingestion** — `WavAudioChunkReader` reads RIFF/WAV without an external audio library;
   `FfmpegAudioChunkReader` invokes the `ffmpeg` executable for supported extensions;
   the default `CompositeAudioChunkReader` tries WAV before FFmpeg.
2. **Inference** — `IWhisperTranscriber` (strategy) decouples model execution from plugin lifecycle.
3. **Provenance** — successful `WhisperPlugin` results receive source, plugin-version, and
   generation-time stamps. Error results carry source/version fields; some early error paths do
   not set a generation timestamp.

---

## 2. Design Principles

- **Stub-first** — `WhisperStubTranscriber` is available when whisper.cpp is not enabled.
  It returns an empty transcript (`language="unknown"`, confidence `0`); the result type defaults
  to `success=true`.
- **Injection-friendly** — the `WhisperPlugin` constructor accepts injected transcriber and reader
  dependencies, enabling unit testing without model files.
- **Plugin-owned provenance** — `WhisperPlugin` applies source/version fields to results and sets
  generation timestamps on successful results; some error results do not have a timestamp.
- **Zero external dependencies for ingestion** — `WavAudioChunkReader` requires only `<fstream>`
  and `<cstring>`.

---

## 3. Component Diagram

```
┌────────────────────────────────────────────────────────────┐
│  IAudioBackend  (include/plugins/audio_backend_interface.h) │
└───────────────────────────┬────────────────────────────────┘
                            │ implements
                ┌───────────▼──────────────┐
                │      WhisperPlugin       │
                │  ┌───────────────────┐   │
                │  │ IWhisperTranscriber│   │  ← strategy
                │  │  ┌─────────────┐  │   │
                │  │  │ CppTranscrib│  │   │  (whisper.cpp, optional)
                │  │  ├─────────────┤  │   │
                │  │  │ StubTranscr │  │   │  (always available)
                │  │  ├─────────────┤  │   │
                │  │  │ InMemoryTr. │  │   │  (test double)
                │  │  └─────────────┘  │   │
                │  └───────────────────┘   │
                │  ┌───────────────────┐   │
                │  │CompositeChunkRdr  │   │  ← dispatches by extension
                │  │  ┌─────────────┐  │   │
                │  │  │WavChunkRdr  │  │   │  (RIFF/WAV, no deps)
                │  │  ├─────────────┤  │   │
                │  │  │FfmpegChkRdr │  │   │  (MP3/OGG/FLAC, ffmpeg)
                │  │  └─────────────┘  │   │
                │  └───────────────────┘   │
                └──────────────────────────┘
                           │ registered via
         ┌─────────────────▼──────────────────┐
         │ WhisperPluginAdapter : IThemisPlugin │  ← PluginManager integration
         │ WhisperPluginRegistrar              │  ← hot-plug lifecycle
         └─────────────────────────────────────┘
```

---

## 4. Key Data Flows

### 4.1 `transcribeFile(path)`

```
WhisperPlugin::transcribeFile(path)
  ├─ CompositeAudioChunkReader::readFile(path) → mono float32 PCM + sample rate
  ├─ IWhisperTranscriber::transcribe(pcm, sample_rate) → text / language
  ├─ apply provenance stamps
  └─ return TranscriptionResult
```

For in-memory input, `transcribe(pcm, sample_rate)` skips the reader. `transcribeStream()` applies
VAD when configured and calls the transcriber's streaming interface; the default implementation
emits one token containing the completed transcript, and `WhisperCppTranscriber` does not override
it.

`transcribeWithDiarisation()` calls the optional transcriber method. The current whisper.cpp
implementation wraps the full transcript in one segment with one speaker ID; it does not perform
speaker separation. The base interface default returns an empty successful result.

### 4.2 Error Paths

| Condition | Behaviour |
|-----------|-----------|
| Plugin not initialised | `success=false`, `error_message="not initialized"` |
| File not found | `success=false`, `error_message=<filesystem error>` |
| Invalid WAV format | `success=false`, `error_message=<parse error>` |
| Transcriber throws `std::exception` | `success=false`, `error_message=<exception.what()>`; non-standard exceptions may propagate |
| All success paths | `success=true`, provenance stamps set |

---

## 5. Configuration (`WhisperConfig`)

| Field | Default | Constraint |
|---|---|---|
| `model_path` | `""` | path to `.bin` model |
| `language` | `"auto"` | passed through to whisper.cpp; no language-code validation in `WhisperConfig` |
| `n_threads` | `4` | values below `1` are clamped to `1`; no upper clamp is implemented |
| `beam_size` | `5` | values below `1` are clamped to `1`; no upper clamp is implemented |
| `model_sha256` | empty | optional expected SHA-256; checked by the whisper.cpp backend before model load |
| `quality_threshold` | `0.0` | serialized, but no filtering behavior is implemented in the current transcriber |
| `language_confidence_threshold` | `0.0` | clamped to `[0, 1]`; lower-confidence language results become `"unknown"` |

---

## 6. Plugin Manifest

`plugins/whisper/plugin.json.in` is processed by `src/whisper/CMakeLists.txt` into a build-tree
`plugin.json`.
The manifest declares `type: AUDIO_PROCESSING`, `api_version: 1.0`, and optional dependency
on `whisper_cpp >= 1.5.0`.

---

## 7. Thread Safety

Transcriber operations invoked through `WhisperPlugin` are serialized by `transcriber_mutex_`;
the initialized flag and counters are atomic. VAD replacement and use are guarded separately.

---

## 8. Testing Strategy

| Type | Files | Count |
|---|---|---|
| Registered focused unit test | `src/whisper/tests/test_whisper_plugin.cpp` | 69 |
| Registrar test source | `src/whisper/tests/test_whisper_plugin_registrar.cpp` | 12; not registered by current CMake |
| Additional whisper tests | `tests/whisper/test_whisper_*.cpp` | 6 across two CMake-discovered targets |

The 69 registered plugin tests exercise stub/in-memory paths and do not require a model. The two
additional tests under `tests/whisper/` exercise an injected stub bridge and a synthetic
high-cardinality stress fixture; neither is real-model validation.

## Module Dependencies

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| whisper.cpp | `whisper.h` (external library) | Optional ASR inference runtime when `THEMIS_ENABLE_WHISPER` is enabled and `libwhisper` is found |
| FFmpeg executable | subprocess | Optional format decoding/resampling; no FFmpeg library binding is used |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes |
|--------|-----|-------|
| PluginManager | `WhisperPluginAdapter` / `WhisperPluginRegistrar` | Adapter and hot-plug helper APIs exist; an end-to-end server caller is not established by this module's CMake/source alone. |

## Integration Points

### Optional whisper.cpp binding
`WhisperCppTranscriber` is compiled only when `THEMIS_ENABLE_WHISPER` is defined by the build.
It loads the configured model at initialization; `WhisperPlugin` serializes calls into its
transcriber instance. Model-dependent behavior is not covered by the registered unit tests.
