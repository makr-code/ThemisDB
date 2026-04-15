<!-- Status: current | validated: 2026-04-07 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Whisper Plugin

All notable changes to the Whisper audio transcription plugin are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

- `WhisperConfig.language_confidence_threshold` — `detectLanguage()` now returns `"unknown"` when
  detected confidence is below the configured threshold (0 = disabled, default)
- Real whisper.cpp inference integration (requires `THEMIS_ENABLE_WHISPER=ON`)
- Streaming token output during transcription
- Speaker diarisation (multi-speaker attribution)
- VAD (voice activity detection) pre-filter

## [2.0.0] — 2026-04-07

### Added
- `WhisperPlugin` — top-level `IAudioBackend` implementation with provenance stamps
  (`ingestion_source_type="WHISPER"`, `plugin_version`, `generation_timestamp`)
- `WavAudioChunkReader` — zero-dependency RIFF/WAV parser supporting 16-bit PCM and
  IEEE float32 samples; validates magic bytes, format chunk, and sample-rate range
- `IWhisperTranscriber` — strategy interface separating model inference from plugin lifecycle
- `WhisperCppTranscriber` — production backend, compiled only when `THEMIS_ENABLE_WHISPER=ON`
- `WhisperStubTranscriber` — always-available fallback returning a fixed transcript
- `InMemoryWhisperTranscriber` — test double with `setNextResult` / `setNextLanguage` / `setFailNext`
- `WhisperConfig` — runtime config with `fromJson` / `toJson` round-trip; validates model path,
  language, thread count, and context window size
- 30 unit tests (`WhisperPluginFocusedTests`, groups A–J)
- `plugins/whisper/plugin.json.in` — plugin manifest for dynamic loading
- `src/whisper/CMakeLists.txt` — build target with optional `whisper.cpp` support
- `tests/CMakeLists.txt` — `WhisperPluginFocusedTests` target registered
- `plugins/CMakeLists.txt` — `THEMIS_PLUGIN_WHISPER` option added
