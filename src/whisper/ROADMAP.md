# Whisper Plugin Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Roadmap-Status: current | validated: 2026-04-07 | Primary: src/whisper/ -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

v2.1.0 — Thread-safe plugin with `transcriber_mutex_` + `std::atomic` counters; `FfmpegAudioChunkReader` (popen-based ffmpeg, shell-escaped path, 500 MB cap) and `CompositeAudioChunkReader` added. 36 unit tests (groups A–L).

## Completed ✅

- [x] `IAudioBackend` interface + `THEMIS_AUDIO_PLUGIN()` export macro
- [x] `WavAudioChunkReader` — RIFF/WAV parser (16-bit PCM, IEEE float32)
- [x] `IWhisperTranscriber` strategy interface
- [x] `WhisperCppTranscriber` (production, optional compile)
- [x] `WhisperStubTranscriber` (CI / no model file)
- [x] `InMemoryWhisperTranscriber` test double
- [x] `WhisperPlugin` — provenance stamps, error counting, DL entry points
- [x] `WhisperConfig::fromJson` / `toJson` with validation and clamping
- [x] 30 unit tests (`WhisperPluginFocusedTests`, groups A–J) — v2.0.0
- [x] Plugin manifest (`plugins/whisper/plugin.json.in`)
- [x] CMake registration (plugin + tests)
- [x] **Thread-safety** — `transcriber_mutex_` (`std::mutex`) + `std::atomic` counters for concurrent `transcribe()` calls (Issue: #4591) (2026-04-12)
- [x] **`FfmpegAudioChunkReader`** — popen-based FFmpeg adapter; shell-escaped path; 500 MB output cap (Issue: #4591) (2026-04-12)
- [x] **`CompositeAudioChunkReader`** — chains multiple `IAudioChunkReader` implementations for unified pipeline (Issue: #4591) (2026-04-12)
- [x] **36 unit tests** (`WhisperPluginFocusedTests`, groups A–L) — 6 new tests groups K–L for thread-safety + FFmpeg/Composite readers (Issue: #4591) (2026-04-12)

## In Progress

- [~] Integration with `PluginManager` hot-plug monitor

## Planned Features

- [ ] Streaming token output during transcription (Target: Q3 2026)
- [ ] Speaker diarisation — multi-speaker attribution (Target: Q4 2026)
- [ ] VAD pre-filter to skip silent segments (Target: Q3 2026)
- [ ] MP3/OGG input support via FFmpeg adapter (Target: Q4 2026) — FFmpeg adapter (`FfmpegAudioChunkReader`) shipped in v2.1.0 (Issue: #4591); MP3/OGG format pass-through pending
- [ ] Language-detection confidence threshold config (Target: Q3 2026)

## Implementation Phases

### Phase 1 — Design / API Contract ✅
- [x] `IAudioBackend`, `TranscriptionResult`, `WhisperConfig` defined
- [x] Strategy interface (`IWhisperTranscriber`) separating backend from lifecycle

### Phase 2 — Core Implementation ✅
- [x] `WavAudioChunkReader` — PCM parsing without libsndfile dependency
- [x] `WhisperPlugin` wiring config → reader → transcriber → result

### Phase 3 — Error Handling & Edge Cases ✅
- [x] WAV format validation (magic, chunk size, sample rate bounds)
- [x] File-not-found, empty file, truncated data → `success=false` + `error_message`
- [x] Transcriber exception catching in `WhisperPlugin::transcribe()`

### Phase 4 — Tests ✅
- [x] 30 unit tests across groups A–J (v2.0.0)
- [x] 6 additional tests groups K–L (v2.1.0): thread-safety + FFmpeg/Composite readers (Issue: #4591)

### Phase 5 — Performance / Hardening
- [x] Thread-safety audit of `WhisperPlugin` for concurrent `transcribe()` calls — `transcriber_mutex_` + `std::atomic` counters shipped v2.1.0 (Issue: #4591)
- [x] `FfmpegAudioChunkReader` + `CompositeAudioChunkReader` shipped v2.1.0 (Issue: #4591)
- [ ] Benchmark against whisper.cpp CLI (Target: Q3 2026)

### Phase 6 — Documentation & Acceptance ✅
- [x] README, CHANGELOG, ROADMAP, ARCHITECTURE, FUTURE_ENHANCEMENTS, AUDIT, SECURITY

## Production Readiness Checklist

- [x] Unit tests present (36 tests, groups A–L)
- [x] Stub mode for CI without model file
- [x] Injection constructor for test doubles
- [x] Provenance stamps on every result
- [x] Thread-safety verified — `transcriber_mutex_` + `std::atomic` counters (v2.1.0, Issue: #4591)
- [ ] Performance benchmarked vs. whisper.cpp CLI
- [ ] Real whisper.cpp integration validated end-to-end

## Known Issues & Limitations

- `WhisperCppTranscriber` is compiled but not exercised in CI without a model file.
- MP3 and OGG inputs are not yet supported — callers must convert to WAV first.
- Speaker diarisation is not implemented.

## Breaking Changes

None yet (v2.1.0 adds thread-safety and FFmpeg reader; no API breakage).
