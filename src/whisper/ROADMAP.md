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

> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# Whisper Plugin Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Roadmap-Status: current | validated: 2026-10-31 | Primary: src/whisper/ -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Plugin version `2.3.0`. Optional whisper.cpp inference, stub mode, WAV/FFmpeg readers, streaming
callback API, RMS VAD, and a diarisation API are present. The streaming API's default and real
whisper.cpp paths emit the completed transcript as one token; current whisper.cpp diarisation
returns one speaker segment and does not distinguish speakers. Model-backed behavior has not
been verified by the registered unit tests.

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
- [x] 69 unit tests (`WhisperPluginFocusedTests`, groups A–U; registered by root `tests/CMakeLists.txt`)
- [x] Plugin manifest (`plugins/whisper/plugin.json.in`)
- [x] CMake registration (plugin + tests)
- [x] `WhisperConfig.language_confidence_threshold` — filters low-confidence `detectLanguage()` results
- [x] Thread-safety: `transcribe_mutex_` now also guards `detectLanguage()` + threshold filter
- [x] **`WhisperPluginAdapter` + `WhisperPluginRegistrar`** — `IThemisPlugin` adapter wrapping `WhisperPlugin`; `createPlugin`, `createAdapter`, `defaultReloadCallback`, `enableHotPlug`, `disableHotPlug`; 12 unit tests (`WhisperPluginRegistrarTests`, groups A–D) (2026-04-16)

## In Progress

Real-model integration validation, genuine incremental streaming, and speaker separation remain open;
see Planned Features and Known Issues.

## Planned Features

- [x] Streaming callback API with completed-text fallback (not incremental whisper.cpp inference)
- [x] RMS VAD pre-filter on `transcribeStream()` when explicitly installed
- [~] Multi-speaker diarisation API exists; current whisper.cpp path labels one whole-transcript segment only
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
- [x] 69 registered unit-test cases across groups A–U (earlier milestone: 44 cases across groups A–N)
- [x] Group K: thread-safety (concurrent transcribe, atomic error/success counters, detectLanguage)
- [x] Group L: FfmpegAudioChunkReader canRead, graceful degradation, composite routing
- [x] Group O: streaming API — single-token fallback, in-memory multi-token path, callback exception, uninit guard, provenance (WST-01..05)
- [x] Group P: EnergyThresholdVad — all-silence, all-speech, mixed (VAD-01..03)
- [x] Group Q: WhisperPlugin VAD integration — silent skip, speech pass-through, null VAD no-op (VAD-04..06)

### Phase 5 — Performance / Hardening ✅
- [x] Thread-safety audit of `WhisperPlugin` for concurrent `transcribe()` calls
- [x] Benchmark source contains stub and conditional real-model scenarios; target is not currently registered because the CMake existence guard points at a missing path.
- [x] `transcribeStream()` callback API with callback-exception handling; true incremental model output remains open
- [x] `EnergyThresholdVad` + `IVoiceActivityDetector` strategy; `WhisperPlugin::setVoiceActivityDetector()` (Q3 2026)
- [ ] Run the real-model benchmark against a pinned whisper.cpp model and CLI baseline (Target: TODO; benchmark CMake guard currently prevents target registration)

### Phase 5 note — Q1 2027 hardware-baseline items
- [~] Real whisper.cpp end-to-end validation remains pending; requires a model file and a registered model-backed test

### Phase 6 — Documentation & Acceptance ✅
- [x] README, CHANGELOG, ROADMAP, ARCHITECTURE, FUTURE_ENHANCEMENTS, AUDIT, SECURITY

The unused-symbol section below is a dated audit snapshot from 2026-04-20; its caller findings
have not been revalidated as part of this documentation update.

### Phase 7 — Diarisation API (v2.3.0)
- [x] `DiarisationResult` + `DiarisationConfig` added to `IWhisperTranscriber` (optional default implementation)
- [x] `WhisperPlugin::transcribeWithDiarisation()` invokes the transcriber and applies provenance fields
- [x] Stub + in-memory transcriber fixtures and five API/configuration tests
- [ ] Add actual multi-speaker inference and measure quality on a defined evaluation set (Target: TODO)

## Production Readiness Checklist

- [x] Unit tests present (69 tests)
- [x] Stub mode for CI without model file
- [x] Injection constructor for test doubles
- [x] Plugin-owned source/version provenance fields; generation timestamps are set on successful results and may be absent on errors
- [x] Thread-safety tests cover concurrent transcription/language detection and VAD replacement/streaming paths
- [!] Benchmark source exists, but current CMake guard checks for a root-level source file that is absent; target registration is not established
- [x] Adapter and hot-plug helper APIs exist; registrar test source is not registered by current CMake
- [x] `transcribeStream()` callback API with exception handling; whisper.cpp still uses the one-token fallback
- [x] `EnergyThresholdVad` + `IVoiceActivityDetector` strategy injected via `setVoiceActivityDetector()` (v2.2.0)
- [x] 69 unit tests (groups A–U, including DSR-01..05 + SEC-01 guard tests)
- [ ] Real whisper.cpp integration validated end-to-end (requires model file and a registered real-model test)
- [x] SHA-256 model integrity validation path implemented (`WhisperConfig.model_sha256`)

### Phase 8 — PluginManager Hot-Plug Integration ✅ (v2.1.0)
- [x] `WhisperPluginAdapter : IThemisPlugin` — wraps `WhisperPlugin`, implements `initialize(config_json)`, `shutdown()`, `getType()`, `getCapabilities()`, `getInstance()`; `PluginType::AUDIO_PROCESSING`
- [x] `WhisperPluginRegistrar` — `createPlugin()`, `createAdapter()`, `defaultReloadCallback()`, `enableHotPlug()`, `disableHotPlug()`
- [x] 12 unit tests (`WhisperPluginRegistrarTests`, groups A–D) in `src/whisper/tests/test_whisper_plugin_registrar.cpp`

## Known Issues & Limitations

- `WhisperCppTranscriber` requires `THEMIS_ENABLE_WHISPER`, `libwhisper`, and a model; registered tests use stub/in-memory paths.
- `WhisperCppTranscriber::diarize()` returns one segment with one speaker ID; it is not speaker separation.
- `WhisperCppTranscriber` inherits the default `transcribeStream()` implementation, which emits the full transcript as one token.
- `FfmpegAudioChunkReader` requires `ffmpeg` on `PATH`; reads in supported non-WAV formats fail when it is unavailable.
- Benchmark target registration is blocked by a CMake existence-check path mismatch; performance results are not established here.
- The registrar test source has 12 cases but no CMake registration in the current tree.

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
- [x] Add a high-cardinality stress fixture (Target: Q1 2027) — evidence: `tests/whisper/test_whisper_highcardinality_stress.cpp`; this uses local stub classes, not `WhisperPlugin` or a model.
- [x] Add transcription/load/audio-chunk soak fixtures (Target: Q1 2027) — evidence: `tests/integration/test_whisper_transcription_soak.cpp`; these use in-process substitutes, not live model inference.
- [!] The runbook exists at `docs/operability/RUNBOOK_WHISPER_TRANSCRIPTION.md`, but its metrics/log patterns and operational commands are not implemented in this module's source; treat those instructions as unverified assumptions.

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [!] No distributed/acceleration path is exposed by this module's current public interface.
- [ ] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: TODO; register benchmark target and define model/hardware)
- [!] Basic `getStatistics()` counters exist; runbook alerts/metrics are not demonstrated by module source.
