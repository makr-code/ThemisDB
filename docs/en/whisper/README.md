[docs](../../README.md) > [en](../README.md) > [whisper](./README.md) > [README](./README.md)
**Datum:** 2026-04-16
**Status:** review
**Primary (Quelle der Wahrheit):**
- `src/whisper/README.md`
- `src/whisper/ARCHITECTURE.md`
- `src/whisper/ROADMAP.md`
- `src/whisper/FUTURE_ENHANCEMENTS.md`
- `include/whisper/README.md`
- `include/whisper/ROADMAP.md`

**Bezug / Reference:**
- Issue: `[MODULE] whisper`
- Context: Module-level reality check and secondary documentation migration (Primary → Secondary)

---

# Whisper Module

## TL;DR

The `whisper` module is implemented with `WhisperPlugin`, WAV reader, FFmpeg adapter,
composite reader, thread safety (`transcriber_mutex_`), and 44 unit tests (A–N).

## Installation

Build through the existing CMake integration:

```bash
cmake -B build
cmake --build build --target test_whisper_plugin
```

Optional production backend:

```bash
cmake -B build -DTHEMIS_ENABLE_WHISPER=ON
```

## Usage

- File transcription via `WhisperPlugin::transcribeFile(path)`
- PCM transcription via `WhisperPlugin::transcribe(samples, sample_rate)`
- Language detection via `WhisperPlugin::detectLanguage(samples, sample_rate)`

## Task 1 — Reality Check (Code vs Primary Docs)

Validated implementation paths:
- `src/whisper/whisper_plugin.cpp`
- `src/whisper/audio_chunk_reader.cpp`
- `src/whisper/whisper_config.cpp`
- `src/whisper/tests/test_whisper_plugin.cpp`

Drift found and corrected in this migration:
- `src/whisper/README.md`: maturity/test-count/quickstart snippet was outdated.
- `src/whisper/ARCHITECTURE.md`: thread-safety section was outdated.
- `src/whisper/AUDIT.md` and `src/whisper/ROADMAP.md`: test count was 36 instead of 44.
- `include/whisper/ROADMAP.md`: test count was 36 instead of 44.

## Task 2 — ROADMAP / FUTURE_ENHANCEMENTS Verification

- `src/whisper/ROADMAP.md`: Phases 1–6 are backed by implementation evidence; open items remain
  streaming, diarization, VAD, and real-model CI.
- `src/whisper/FUTURE_ENHANCEMENTS.md`: actionable and implementation-ready structure
  (inputs/outputs/constraints/tests/targets).
- `include/whisper/FUTURE_ENHANCEMENTS.md`: structurally valid; performance target is intentionally
  high-level and should be made measurable when implementation starts.

## Task 3 — Research Notes / Constraints

- FFmpeg support remains subprocess-based (`FfmpegAudioChunkReader`) with shell escaping and
  a 500 MB output guard.
- Stub vs production behavior is compile-time gated via `THEMIS_ENABLE_WHISPER`.
- Model integrity verification is still a documented security follow-up.

## Related Secondary Docs

- [PRIMARY_SOURCES.md](./PRIMARY_SOURCES.md)
