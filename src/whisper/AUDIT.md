---
Author: "ThemisDB Team"
Created: "2026-04-19"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Whisper Module Documentation and Build Audit

**Verification date:** 2026-10-31. **Method:** Source/CMake inspection; no build or test run is
claimed by this document.

## Current source surface

| Area | Source | Verified behavior |
|---|---|---|
| Configuration | `whisper_config.cpp` | JSON round-trip; `n_threads` and `beam_size` lower-bound at 1; language-confidence threshold clamped to `[0, 1]`. |
| Audio input | `audio_chunk_reader.cpp` | WAV PCM16/float32 decoding; optional FFmpeg subprocess with 16 kHz mono output cap of 500 MiB; composite extension-based routing. |
| Transcription | `whisper_transcriber.cpp` | Optional whisper.cpp backend, empty-result stub, and in-memory test double. |
| Plugin | `whisper_plugin.cpp` | File/PCM/stream/language/diarisation entry points, provenance, counters, exception-to-result conversion, serialized transcriber calls. |
| Registration | `whisper_plugin_registrar.cpp` | Adapter, factory helpers, and PluginManager hot-plug helpers. |

## Build and test registration

- Root `tests/CMakeLists.txt` builds `test_whisper_plugin` and registers CTest name
  `WhisperPluginFocusedTests`. The source currently contains 69 test cases.
- `tests/whisper/CMakeLists.txt` discovers two files and registers filename-derived focused tests:
  three stub-bridge cases and three synthetic stress-fixture cases.
- `src/whisper/tests/test_whisper_plugin_registrar.cpp` contains 12 cases, but current CMake does
  not register or build that source.
- The benchmark source contains stub and conditional real-model cases. Its CMake `EXISTS` guard
  checks for a missing root-level source, while the checked-in file is under `benchmarks/whisper/`;
  target creation is therefore not established.

## Validation gaps

- The registered unit tests do not establish real-model inference behavior.
- `WhisperCppTranscriber` inherits the default `transcribeStream()` implementation, which emits
  one completed-text token. This is not incremental inference.
- The current whisper.cpp diarisation method returns one transcript segment for one speaker; it
  does not separate speakers.
- `quality_threshold` is serialized but not applied in the transcriber.
- Module source exposes basic statistics counters, but does not implement the metrics/exporter,
  log patterns, or operational commands described by the external whisper runbook.

See [`ROADMAP.md`](ROADMAP.md), [`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md), and
[`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md) for tracked follow-up work.
