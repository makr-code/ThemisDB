---
Author: "ThemisDB Team"
Created: "2026-10-18"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Whisper in ThemisDB

## Overview

The module implements `audio::IAudioBackend` through `WhisperPlugin`. It accepts mono float32
PCM directly or reads audio files through a WAV/FFmpeg reader chain. Real inference is conditional
on `THEMIS_ENABLE_WHISPER`, `libwhisper`, and a model; otherwise the default backend is an empty
stub. It does not provide microphone capture or demonstrate incremental real-model inference.

## Implemented surfaces

- `WhisperConfig` serializes JSON settings, including optional model SHA-256 verification and
  language-confidence filtering.
- `WavAudioChunkReader` decodes 16-bit PCM and IEEE float32 RIFF/WAV; `FfmpegAudioChunkReader`
  invokes the `ffmpeg` executable for supported filename extensions.
- `WhisperPlugin` provides file/PCM transcription, language detection, callback-based
  `transcribeStream()`, optional RMS VAD on that streaming method, and provenance/statistics.
- `IWhisperTranscriber` separates inference from plugin orchestration; stub and in-memory
  implementations support model-free tests.
- Diarisation types and orchestration are present, but the current whisper.cpp backend returns
  one whole-transcript segment for one speaker and does not separate speakers.

`quality_threshold` is accepted and serialized, but the current transcriber does not apply it.
`n_threads` and `beam_size` are lower-bounded at 1 during JSON parsing; no upper clamp is present.

## Build and test references

The root `tests/CMakeLists.txt` registers `WhisperPluginFocusedTests` (69 cases). The CMake file
under `tests/whisper/` discovers the stub bridge and synthetic high-cardinality tests. The
registrar test source is not registered by current CMake. See [README.md](./README.md) for commands
and [ROADMAP.md](./ROADMAP.md) for implementation gaps.
