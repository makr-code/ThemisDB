# Whisper Plugin Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Roadmap-Status: current | validated: 2026-04-07 | Primary: src/whisper/ -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

v2.0.0 — Core pipeline operational. Stub mode fully functional. whisper.cpp integration
compiled when `THEMIS_ENABLE_WHISPER=ON`.

## Completed ✅

- [x] `IAudioBackend` interface + `THEMIS_AUDIO_PLUGIN()` export macro
- [x] `WavAudioChunkReader` — RIFF/WAV parser (16-bit PCM, IEEE float32)
- [x] `IWhisperTranscriber` strategy interface
- [x] `WhisperCppTranscriber` (production, optional compile)
- [x] `WhisperStubTranscriber` (CI / no model file)
- [x] `InMemoryWhisperTranscriber` test double
- [x] `WhisperPlugin` — provenance stamps, error counting, DL entry points
- [x] `WhisperConfig::fromJson` / `toJson` with validation and clamping
- [x] 30 unit tests (`WhisperPluginFocusedTests`)
- [x] Plugin manifest (`plugins/whisper/plugin.json.in`)
- [x] CMake registration (plugin + tests)

## In Progress

- [~] Integration with `PluginManager` hot-plug monitor

## Planned Features

- [ ] Streaming token output during transcription (Target: Q3 2026)
- [ ] Speaker diarisation — multi-speaker attribution (Target: Q4 2026)
- [ ] VAD pre-filter to skip silent segments (Target: Q3 2026)
- [ ] MP3/OGG input support via FFmpeg adapter (Target: Q4 2026)
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
- [x] 30 unit tests across groups A–J

### Phase 5 — Performance / Hardening
- [ ] Thread-safety audit of `WhisperPlugin` for concurrent `transcribe()` calls (Target: Q3 2026)
- [ ] Benchmark against whisper.cpp CLI (Target: Q3 2026)

### Phase 6 — Documentation & Acceptance ✅
- [x] README, CHANGELOG, ROADMAP, ARCHITECTURE, FUTURE_ENHANCEMENTS, AUDIT, SECURITY

## Production Readiness Checklist

- [x] Unit tests present (30 tests)
- [x] Stub mode for CI without model file
- [x] Injection constructor for test doubles
- [x] Provenance stamps on every result
- [ ] Thread-safety verified for concurrent access
- [ ] Performance benchmarked vs. whisper.cpp CLI
- [ ] Real whisper.cpp integration validated end-to-end

## Known Issues & Limitations

- `WhisperCppTranscriber` is compiled but not exercised in CI without a model file.
- MP3 and OGG inputs are not yet supported — callers must convert to WAV first.
- Speaker diarisation is not implemented.

## Breaking Changes

None yet (v2.0.0 is the initial release).
