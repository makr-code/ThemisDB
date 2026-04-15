# include whisper roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
- [x] Header API for whisper plugin and helpers is available — v2.1.0 (2026-04-12)
  - Thread-safe `WhisperPlugin` via `transcriber_mutex_` + `std::atomic` counters (Issue: #4591)
  - `FfmpegAudioChunkReader` (popen ffmpeg, shell-escaped path, 500 MB cap) (Issue: #4591)
  - `CompositeAudioChunkReader` chaining multiple `IAudioChunkReader` implementations (Issue: #4591)
  - 36 unit tests groups A–L (Issue: #4591)

## In Progress
- [ ] Add richer contract notes for multilingual/translation defaults (Target: Q3 2026)

## Implementation Phases
### Phase 1: Design / API Contract
- [x] Define plugin/transcriber/config/audio-reader interfaces (Target: Q2 2026)
### Phase 2: Core Implementation
- [x] Source implementation exists in `src/whisper` (Target: Q2 2026)
### Phase 3: Error Handling & Edge Cases
- [x] Stub fallback path for non-whisper builds is represented in API (Target: Q2 2026)
### Phase 4: Tests
- [x] Add include-contract tests for file reader and config parsing edge cases (Target: Q3 2026)
### Phase 5: Performance/Hardening
- [x] Define API limits for file size and sample-rate ranges (`bench_whisper_transcription.cpp`): transcribe 1s/5s/30s, 8 kHz, CLI parity budget, buffer-size sweep (Target: Q3 2026)
### Phase 6: Documentation & Acceptance
- [x] Baseline include module docs created (Target: Q2 2026)

## Production Readiness Checklist
- [x] Public headers map to implementation
- [x] Additional API compatibility tests

## Known Issues & Limitations
- Real transcription quality depends on linked whisper runtime and model assets.

## Breaking Changes
- None currently planned.