# Whisper Plugin — Architecture Guide

<!-- Status: current | validated: 2026-04-16 | Primary: src/whisper/ -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

**Version:** 1.0
**Last Updated:** 2026-04-07
**Module Path:** `src/whisper/`

---

## 1. Overview

The Whisper plugin integrates [whisper.cpp](https://github.com/ggerganov/whisper.cpp) into
ThemisDB's plugin system as an `IAudioBackend`. It converts raw WAV audio into text
transcriptions enriched with mandatory provenance metadata. The plugin is structured around
three orthogonal concerns:

1. **Audio ingestion** — `WavAudioChunkReader` parses RIFF/WAV containers without external libraries.
2. **Inference** — `IWhisperTranscriber` (strategy) decouples model execution from plugin lifecycle.
3. **Provenance** — `WhisperPlugin` applies stamps (`ingestion_source_type`, `plugin_version`,
   `generation_timestamp`) unconditionally before returning `TranscriptionResult`.

---

## 2. Design Principles

- **Stub-first** — `WhisperStubTranscriber` is always available; whisper.cpp is optional.
  CI environments without a model file receive a valid `TranscriptionResult` with `success=true`.
- **Injection-friendly** — `WhisperPlugin(std::unique_ptr<IWhisperTranscriber>)` constructor
  enables full unit testing without touching the file system or model files.
- **Provenance unconditional** — timestamps and source-type stamps are applied in `WhisperPlugin`,
  never delegated to the transcriber, ensuring no transcriber implementation can omit them.
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
                │  │ WavAudioChunkRdr  │   │  ← RIFF/WAV parser
                │  └───────────────────┘   │
                └──────────────────────────┘
```

---

## 4. Key Data Flows

### 4.1 transcribe(path, config)

```
WhisperPlugin::transcribe(path, cfg)
  ├─ WavAudioChunkReader::read(path)     → AudioChunk (PCM samples)
  ├─ IWhisperTranscriber::transcribe(chunk, cfg)  → raw text / language
  ├─ apply provenance stamps
  └─ return TranscriptionResult
```

### 4.2 Error Paths

| Condition | Behaviour |
|-----------|-----------|
| Plugin not initialised | `success=false`, `error_message="not initialized"` |
| File not found | `success=false`, `error_message=<filesystem error>` |
| Invalid WAV format | `success=false`, `error_message=<parse error>` |
| Transcriber throws | `success=false`, `error_message=<exception.what()>` |
| All success paths | `success=true`, provenance stamps set |

---

## 5. Configuration (`WhisperConfig`)

| Field | Default | Constraint |
|---|---|---|
| `model_path` | `""` | path to `.bin` model |
| `language` | `"auto"` | ISO 639-1 or `"auto"` |
| `threads` | `4` | clamped to `[1, 64]` |
| `n_past` | `0` | context tokens to keep |
| `use_gpu` | `false` | enables CUDA/Metal |
| `beam_size` | `5` | `[1, 20]` |

---

## 6. Plugin Manifest

`plugins/whisper/plugin.json.in` is processed by CMake into `plugin.json` at build time.
The manifest declares `type: AUDIO_PROCESSING`, `api_version: 1.0`, and optional dependency
on `whisper_cpp >= 1.5.0`.

---

## 7. Thread Safety

`WhisperPlugin` is thread-safe for concurrent `transcribe()`, `transcribeFile()`, and
`detectLanguage()` calls via `transcriber_mutex_` plus atomic counters.

---

## 8. Testing Strategy

| Type | Files | Count |
|---|---|---|
| Unit (stub mode) | `src/whisper/tests/test_whisper_plugin.cpp` | 44 |

All 44 tests run without a whisper.cpp model file. `InMemoryWhisperTranscriber` is injected
via the DI constructor for multiple groups (including E–N).
