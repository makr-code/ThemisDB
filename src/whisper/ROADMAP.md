> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# Whisper Plugin Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Roadmap-Status: current | validated: 2026-08-09 | Primary: src/whisper/ -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

v2.3.0 — Thread-safe. MP3/OGG input via FFmpeg adapter. Streaming, VAD and optional speaker diarisation available.

## Completed ✅

- [x] `IAudioBackend` interface + `THEMIS_AUDIO_PLUGIN()` export macro
- [x] `WavAudioChunkReader` — RIFF/WAV parser (16-bit PCM, IEEE float32)
- [x] `FfmpegAudioChunkReader` — MP3/OGG/FLAC/M4A via `ffmpeg` subprocess
- [x] `CompositeAudioChunkReader` — chains multiple readers by extension
- [x] `IWhisperTranscriber` strategy interface
- [x] `WhisperCppTranscriber` (production, optional compile)
- [x] `WhisperStubTranscriber` (CI / no model file)
- [x] `InMemoryWhisperTranscriber` test double
- [x] `WhisperPlugin` — provenance stamps, error counting, DL entry points
- [x] `WhisperConfig::fromJson` / `toJson` with validation and clamping
- [x] 44 unit tests (`WhisperPluginFocusedTests`, groups A–N)
- [x] Plugin manifest (`plugins/whisper/plugin.json.in`)
- [x] CMake registration (plugin + tests)
- [x] `WhisperConfig.language_confidence_threshold` — filters low-confidence `detectLanguage()` results
- [x] Thread-safety: `transcribe_mutex_` now also guards `detectLanguage()` + threshold filter
- [x] **`WhisperPluginAdapter` + `WhisperPluginRegistrar`** — `IThemisPlugin` adapter wrapping `WhisperPlugin`; `createPlugin`, `createAdapter`, `defaultReloadCallback`, `enableHotPlug`, `disableHotPlug`; 12 unit tests (`WhisperPluginRegistrarTests`, groups A–D) (2026-04-16)

## In Progress

*(none)*

## Planned Features

- [x] Streaming token output during transcription (Target: Q3 2026)
- [x] VAD pre-filter to skip silent segments (Target: Q3 2026)
- [x] Speaker diarisation — multi-speaker attribution (Target: Q4 2026)
- [x] Language-detection confidence threshold config (Target: Q3 2026)

## Implementation Phases

### Phase 1 — Design / API Contract ✅
- [x] `IAudioBackend`, `TranscriptionResult`, `WhisperConfig` defined
- [x] Strategy interface (`IWhisperTranscriber`) separating backend from lifecycle

### Phase 2 — Core Implementation ✅
- [x] `WavAudioChunkReader` — PCM parsing without libsndfile dependency
- [x] `FfmpegAudioChunkReader` — MP3/OGG/FLAC decoder via subprocess
- [x] `CompositeAudioChunkReader` — extension-based reader dispatch
- [x] `WhisperPlugin` wiring config → reader → transcriber → result

### Phase 3 — Error Handling & Edge Cases ✅
- [x] WAV format validation (magic, chunk size, sample rate bounds)
- [x] File-not-found, empty file, truncated data → `success=false` + `error_message`
- [x] Transcriber exception catching in `WhisperPlugin::transcribe()`
- [x] ffmpeg not available → `runtime_error("ffmpeg not available")`
- [x] Shell-escaped path in ffmpeg subprocess (NUL-byte guard, single-quote wrapping)
- [x] Max-output guard (500 MB) in `FfmpegAudioChunkReader`

### Phase 4 — Tests ✅
- [x] 44 unit tests across groups A–N
- [x] Group K: thread-safety (concurrent transcribe, atomic error/success counters, detectLanguage)
- [x] Group L: FfmpegAudioChunkReader canRead, graceful degradation, composite routing
- [x] Group O: streaming transcription — single-token fallback, multi-token, callback exception, uninit guard, provenance (WST-01..05)
- [x] Group P: EnergyThresholdVad — all-silence, all-speech, mixed (VAD-01..03)
- [x] Group Q: WhisperPlugin VAD integration — silent skip, speech pass-through, null VAD no-op (VAD-04..06)

### Phase 5 — Performance / Hardening ✅
- [x] Thread-safety audit of `WhisperPlugin` for concurrent `transcribe()` calls
- [x] Benchmark wired (`bench_whisper_transcription.cpp`, 9 scenarios)
- [x] `transcribeStream()` with incremental token callback; callback-exception safety (Q3 2026)
- [x] `EnergyThresholdVad` + `IVoiceActivityDetector` strategy; `WhisperPlugin::setVoiceActivityDetector()` (Q3 2026)
- [ ] Benchmark against whisper.cpp CLI on real model (Target: Q3 2026)

### Phase 6 — Documentation & Acceptance ✅
- [x] README, CHANGELOG, ROADMAP, ARCHITECTURE, FUTURE_ENHANCEMENTS, AUDIT, SECURITY

### Phase 7 — Speaker Diarisation ✅ (v2.3.0)
- [x] `DiarisationResult` + `DiarisationConfig` added to `IWhisperTranscriber` (optional default implementation)
- [x] `WhisperPlugin::transcribeWithDiarisation()` orchestrates diarisation and applies provenance stamps
- [x] Stub + in-memory transcriber diarisation fixtures for deterministic testing
- [x] DSR-01..05 test coverage for fixture flow, config handling, fallback behavior, clamp, and provenance

## Production Readiness Checklist

- [x] Unit tests present (69 tests)
- [x] Stub mode for CI without model file
- [x] Injection constructor for test doubles
- [x] Provenance stamps on every result
- [x] Thread-safety verified for concurrent access
- [x] Performance benchmarks wired (stub path exercised in CI)
- [x] PluginManager hot-plug integration (`WhisperPluginAdapter` / `WhisperPluginRegistrar`)
- [x] `transcribeStream()` — incremental token callback with exception safety (v2.2.0)
- [x] `EnergyThresholdVad` + `IVoiceActivityDetector` strategy injected via `setVoiceActivityDetector()` (v2.2.0)
- [x] 69 unit tests (groups A–U, including DSR-01..05 + SEC-01 guard tests)
- [~] Real whisper.cpp integration validated end-to-end (requires model file in CI env)
- [x] SHA-256 model integrity validation path implemented (`WhisperConfig.model_sha256`)

### Phase 8 — PluginManager Hot-Plug Integration ✅ (v2.1.0)
- [x] `WhisperPluginAdapter : IThemisPlugin` — wraps `WhisperPlugin`, implements `initialize(config_json)`, `shutdown()`, `getType()`, `getCapabilities()`, `getInstance()`; `PluginType::AUDIO_PROCESSING`
- [x] `WhisperPluginRegistrar` — `createPlugin()`, `createAdapter()`, `defaultReloadCallback()`, `enableHotPlug()`, `disableHotPlug()`
- [x] 12 unit tests (`WhisperPluginRegistrarTests`, groups A–D) in `src/whisper/tests/test_whisper_plugin_registrar.cpp`

## Known Issues & Limitations

- `WhisperCppTranscriber` is compiled but not exercised in CI without a model file.
- Real-model diarisation quality still depends on external diarisation model/runtime availability.
- `FfmpegAudioChunkReader` requires `ffmpeg` on PATH; degrades gracefully when absent.

## Breaking Changes

- v2.1.0: `WhisperPlugin` default constructor now installs a `CompositeAudioChunkReader`
  (WAV first, then FFmpeg) instead of a bare `WavAudioChunkReader`.  Injection-constructor
  callers are unaffected.

## Latente Symbole (Unused-Functions-Audit)

_Stand: 2026-04-20 – Quelle: [`src/UNUSED_FUNCTIONS_REPORT.md`](../UNUSED_FUNCTIONS_REPORT.md)_

### 🧪 NUR_TESTS (implementiert, kein Produktions-Aufrufer)

- `canRead` – Prüft ob Whisper-Plugin einen Audio-Chunk lesen kann
- `addReader` – Registriert einen Audio-Reader für den Whisper-Plugin-Stack
- `WhisperPlugin` – Whisper-ASR-Plugin-Implementierung; Tests + Bench vorhanden
  > **Aktion:** ROADMAP-Ticket für Produktions-Integration ergänzen oder als CANDIDATE_FOR_REMOVAL markieren.

### 🟡 UNGENUTZT (kein Test, kein externer Aufrufer)

- `parseWav` – Parsed WAV-Header und extrahiert Audio-Rohdaten
  > **Aktion:** Für jedes Symbol entscheiden: (1) Verdrahten, (2) Testen oder (3) als CANDIDATE_FOR_REMOVAL einplanen.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `whisper`
- [ ] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027)
- [ ] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027)
- [ ] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027)

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [ ] This module's distributed/acceleration paths fail closed (Target: Q1 2027)
- [ ] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027)
- [ ] Operator-critical paths have diagnostics, alerts, and runbooks (Target: Q1 2027)
