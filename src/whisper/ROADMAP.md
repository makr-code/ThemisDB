---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-11-07"
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
<!-- Roadmap-Status: current | validated: 2026-11-07 | Primary: src/whisper/ -->
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

## Research-Based Assessment

**Maturity:** feature-complete at the plugin/API layer, but not yet performance- or quality-validated
as a real-model ASR service. The default/stub and injected paths have registered tests; registered
tests do not load a real whisper.cpp model. The checked-in benchmark source also is not registered
by the current root CMake guard. The public streaming and diarisation methods therefore overstate
the current model-backed capability unless their documented fallbacks are understood.

**Code evidence:** `WhisperCppTranscriber::transcribe()` calls `whisper_full()` synchronously and
concatenates finished segments; its inherited stream method emits completed text as one callback.
`diarize()` wraps that text in one speaker segment. Calls are serialized per `WhisperPlugin`.
WAV is read wholly into memory without a module size limit; FFmpeg output is capped, while the
subprocess uses a shell-quoted input path. The model digest check is optional. See
[`ARCHITECTURE.md`](ARCHITECTURE.md), [`SECURITY.md`](SECURITY.md),
[`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md), and the source/test/benchmark paths
listed in the waves below.

**Research implications:** Whisper's broad multilingual and zero-shot results support an offline,
multilingual use case, but the original study also evaluates long-form behavior and decoding
strategies; results for the paper's model/training setup do not establish quality for this optional
whisper.cpp build or a particular model file [R1]. A separate study found hallucinated phrases in
about 1% of the tested Whisper service transcriptions, especially motivating silence and
low-speech validation; that rate is not transferable as a measurement of this plugin [R2]. ASR
quality and efficiency should be evaluated separately: use normalized WER (and CER as a secondary
metric) against fixed references, and report real-time factor plus runtime/memory resources [R3].
Speaker diarisation is a distinct segmentation/embedding/clustering problem, not a property
provided by ASR transcription alone; evaluate it with DER and state the scoring protocol [R4].
Upstream whisper.cpp documents quantization and hardware-specific execution options, but these
options and their trade-offs must be verified against the actual dependency revision and target
hardware before adoption [R5].

**Overall finding:** suitable as an optional transcription plugin and development integration;
not yet evidenced for production accuracy, real-time streaming, diarisation, concurrent throughput,
or bounded resource consumption. Benchmarks using synthetic PCM or the stub establish dispatch
cost only. The generated developer wiki was last refreshed 2026-11-02 and has no whisper-specific
curated page; source code and this module's active documentation take precedence.

## Release Roadmap: v2.5.0 and v3.0.0

Estimates are engineering effort, not calendar commitments. Proposed numeric gates below are
acceptance targets to validate during baseline capture; published results must include model
revision/digest, audio corpus/license, hardware, build flags, thread count, and scoring protocol.
Do not present stub, synthetic-audio, or unpinned-model results as production ASR evidence.

### v2.5.0 — Evidence and bounded operation

#### Wave 1 — Reproducible model-backed baseline (P0; 2–3 engineer-weeks)

- [ ] Register `benchmarks/whisper/bench_whisper_transcription.cpp` by correcting the root CMake
  source existence guard; keep stub and real-model scenarios distinguishable (Target: v2.5.0-alpha1).
- [ ] Add an opt-in model-backed integration test using a pinned, distributable model and licensed,
  fixed audio/reference fixtures; cover speech, silence, noisy speech, language detection, and a
  long-form sample (Target: v2.5.0-alpha1).
- [ ] Capture normalized WER/CER, real-time factor, p50/p95 latency, peak RSS, cold model-load time,
  and model SHA-256 on a named CPU profile; report stub dispatch separately (Target: v2.5.0-beta1).

**Dependencies:** none for benchmark registration; model, corpus licensing, and a reproducible
hardware runner are required for real-model evidence. **Risk/trade-off:** model downloads and
hardware-dependent results make ordinary CI brittle; keep fast stub tests in `release_critical`
and gate model tests on an explicitly provisioned job/artifact. **Success metrics:** the benchmark
target is build- and CTest-discoverable; all five proposed evaluation classes have attributable
results; the
provisional existing target of less than 2x real-time for 30 seconds of audio on a 4-core CPU is
measured, reported as pass/fail, and revised only with recorded evidence. **Validation:** focused
unit tests, CTest discovery, optional model-backed integration test, and benchmark artifact review.

#### Wave 2 — Input/resource and inference hardening (P0; 2–4 engineer-weeks)

- [ ] Define and enforce configurable maximum WAV bytes, decoded PCM samples/duration, and model
  load/input failure behavior before allocation/inference (Target: v2.5.0-beta1).
- [ ] Bound FFmpeg execution time and decoded output; verify content and decoder exit status, and
  use a platform-safe argument-vector process interface or document an approved security review
  for retaining shell execution (Target: v2.5.0-beta1).
- [ ] Exercise malformed/truncated WAV, oversized input/output, invalid sample rate, missing
  decoder, decoder timeout/failure, empty/silent audio, and digest mismatch paths (Target: v2.5.0-rc1).
- [ ] Define failure behavior for empty results and low-confidence language detection; do not treat
  the current constant whisper.cpp confidence value as a calibrated quality score (Target: v2.5.0-rc1).

**Dependencies:** Wave 1 identifies representative model/audio sizes and gives the baseline needed
to choose defaults. **Risk/trade-off:** limits can reject legitimate long recordings; expose
documented configuration and test boundary values, while retaining fail-closed behavior on
timeouts/invalid input. **Success metrics:** oversized inputs fail before model inference/allocation
past the configured limit; subprocess timeout and failure are deterministic; all listed malformed
input cases return an error without crash, hang, or unbounded output; ASR WER does not regress by
more than 1 absolute percentage point versus the Wave 1 pinned baseline on the same fixtures.
**Validation:** boundary/fault-injection tests, subprocess tests, sanitizer run where available,
and repeated model-backed evaluation.

#### Wave 3 — v2.5.0 release and operational acceptance (P0; 1–2 engineer-weeks)

- [ ] Register all existing whisper test sources, including the plugin registrar tests, and ensure
  focused stub tests remain model-independent and release-critical (Target: v2.5.0-rc1).
- [ ] Publish a versioned benchmark/evaluation manifest with dependency revision, model hash,
  fixture provenance, WER/CER, RTF, p50/p95, RSS, and known limitations (Target: v2.5.0-rc1).
- [ ] Update module security/operations guidance from verified behavior only; document optional
  whisper.cpp/FFmpeg prerequisites and supported resource limits (Target: v2.5.0).

**Dependencies:** Waves 1–2 and green `release_critical` CI on `develop`. **Risk/trade-off:**
model-backed validation may be unavailable on shared CI; stable release criteria must distinguish
passed deterministic tests from pending hardware/model evidence. **Success metrics:** all module
test sources are registered and pass; `release_critical` is green; every claimed accuracy or
performance number links to a reproducible artifact; no model-backed capability is claimed when
its gate is pending. **Validation:** focused CTest, release-critical CI, evidence-manifest
consistency and documentation-link review.

### v3.0.0 — Capability and scale evolution

#### Wave 4 — Genuine incremental transcription (P1; 3–5 engineer-weeks)

- [ ] Implement a model-backed segment/token callback path using supported whisper.cpp APIs;
  document ordering, partial-result retention, callback exceptions, and cancellation behavior
  without claiming a background worker unless one is managed (Target: v3.0.0-alpha1).
- [ ] Add tests proving multiple callbacks arrive before final completion, callback failure is
  surfaced deterministically, and the final transcript agrees with batch transcription under the
  declared normalization policy (Target: v3.0.0-alpha2).

**Dependencies:** Wave 1 model-backed test fixture and confirmation of callback/API support in the
selected whisper.cpp revision. **Risk/trade-off:** the upstream callback lifecycle and shared
context serialization may constrain parallel work; prioritize bounded, correct callbacks over
throughput. **Success metrics:** at least two incremental callbacks are observed for 95% of the
speech fixtures; first-callback p95 and end-to-end RTF are recorded on the pinned model/hardware;
final-text WER is within 1 absolute percentage point of the batch baseline. **Validation:** real
model callback integration tests, callback error/cancellation cases, and first-token benchmark.

#### Wave 5 — Measured speaker diarisation (P1; 4–8 engineer-weeks)

- [ ] Select an optional speaker segmentation/embedding/clustering backend only after license,
  model provenance, supported platforms, resource footprint, and dependency ownership are approved
  (Target: v3.0.0-alpha1).
- [ ] Replace the one-speaker placeholder with timestamped speaker turns and explicit unavailable
  / failed-backend behavior that preserves the ASR transcript where possible (Target: v3.0.0-beta1).
- [ ] Add licensed multi-speaker, overlap, and single-speaker fixtures with a recorded DER scoring
  protocol and speaker-label mapping (Target: v3.0.0-beta1).

**Dependencies:** Wave 1 dataset/evaluation harness; human approval of backend/model and licensing.
**Risk/trade-off:** an additional model increases storage, memory, startup latency, supply-chain and
licensing burden; keep diarisation optional and separately reported from ASR. **Success metrics:**
publish DER (miss, false alarm, and speaker-confusion components) for every fixture; achieve the
review-approved DER threshold on the selected held-out corpus without worsening WER by more than
1 absolute percentage point. Threshold, collar, overlap handling, and corpus must be fixed before
implementation acceptance. **Validation:** backend availability/failure tests, DER scoring
reproducibility, model-integrity checks, and resource benchmark.

#### Wave 6 — Bounded throughput and backend strategy (P2; 4–6 engineer-weeks)

- [ ] Benchmark serialized single-context inference against a bounded multi-context worker model
  on representative CPU profiles; compare throughput, p95 queue/latency, and peak RSS before
  exposing concurrency or accelerator configuration (Target: v3.0.0-beta1).
- [ ] Evaluate supported whisper.cpp quantization/backend variants with paired WER/CER, RTF, and
  memory results; adopt only configurations that satisfy the Wave 1 quality gate and packaging
  policy (Target: v3.0.0-beta1).
- [ ] Add repeatable load/soak and recovery tests against the actual plugin/transcriber; keep the
  existing synthetic high-cardinality fixture labelled as simulation evidence (Target: v3.0.0-rc1).

**Dependencies:** Waves 1–3; Wave 4 if streaming and concurrency share contexts; approved dependency
and packaging choices. **Risk/trade-off:** independent model contexts may multiply memory, while
quantization/hardware acceleration may change recognition quality and reproducibility.
**Success metrics:** on the named host, two bounded workers deliver at least 1.5x the single-worker
throughput, p95 queue latency and peak RSS are published, and WER remains within 1 absolute
percentage point of the pinned baseline; every adopted quantized/backend configuration reports
the same metrics. **Validation:** real-model benchmark matrix, stress/soak, sanitizer, unload/reload
and failure-recovery tests; verify `release_critical` remains green on `develop`.

### Cross-wave assumptions, governance, and stop conditions

- [ ] Keep the existing public C++17 contract additive through v2.5.0; any required breaking API
  change belongs in the v3.0.0 design review and requires migration notes (Target: v2.5.0 / v3.0.0).
- [ ] Keep model files and private/customer audio out of the repository; record artifact
  provenance, licensing, digest source, retention, and access controls before using evaluation
  fixtures (Target: v2.5.0–v3.0.0).
- [ ] Treat model/hardware/corpus unavailability, unapproved diarisation licensing, or an unmet
  quality/resource threshold as an explicit blocked gate, not a passing substitute-path test
  (Target: v2.5.0–v3.0.0).

The numeric WER/RTF/throughput thresholds are proposed gates, not measured current performance.
Open assumptions: an approved public/licensed evaluation corpus and model can be provisioned;
release CI has a hardware runner for optional real-model tests; the selected whisper.cpp revision
supports the required callbacks; and a diarisation dependency can meet distribution/license rules.

### Research References

- **[R1]** Radford et al., “Robust Speech Recognition via Large-Scale Weak Supervision,” arXiv:2212.04356 (2022). <https://arxiv.org/abs/2212.04356>
- **[R2]** Koenecke et al., “Careless Whisper: Speech-to-Text Hallucination Harms,” FAccT 2024. <https://doi.org/10.1145/3630106.3658996>
- **[R3]** NIST, “OpenASR21 Challenge Evaluation Plan,” §3 (WER and runtime/resource reporting), 2021. <https://www.nist.gov/document/openasr21-challenge-evaluation-plan>
- **[R4]** Bredin, “pyannote.audio 2.1 speaker diarization pipeline: principle, benchmark, and recipe,” Interspeech 2023. <https://doi.org/10.21437/Interspeech.2023-105>
- **[R5]** ggml-org, whisper.cpp README (upstream capability and quantization overview; rolling source, not a ThemisDB dependency pin). <https://github.com/ggml-org/whisper.cpp/blob/master/README.md>

## Planned Features

- [x] Streaming callback API with completed-text fallback (not incremental whisper.cpp inference)
- [x] RMS VAD pre-filter on `transcribeStream()` when explicitly installed
- [~] Multi-speaker diarisation API exists; current whisper.cpp path labels one whole-transcript segment only (Target: v3.0.0-beta1; Wave 5)
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
- [ ] Run the real-model benchmark against a pinned whisper.cpp model and CLI baseline (Target: v2.5.0-beta1; Wave 1; benchmark CMake guard currently prevents target registration)

### Phase 5 note — v2.5.0 evidence gates
- [~] Real whisper.cpp end-to-end validation remains pending; requires a pinned model, licensed fixtures, and registered model-backed tests (Target: v2.5.0-rc1; Wave 1)

### Phase 6 — Documentation & Acceptance ✅
- [x] README, CHANGELOG, ROADMAP, ARCHITECTURE, FUTURE_ENHANCEMENTS, AUDIT, SECURITY

The unused-symbol section below is a dated audit snapshot from 2026-04-20; its caller findings
have not been revalidated as part of this documentation update.

### Phase 7 — Diarisation API (v2.3.0)
- [x] `DiarisationResult` + `DiarisationConfig` added to `IWhisperTranscriber` (optional default implementation)
- [x] `WhisperPlugin::transcribeWithDiarisation()` invokes the transcriber and applies provenance fields
- [x] Stub + in-memory transcriber fixtures and five API/configuration tests
- [ ] Add actual multi-speaker inference and measure quality on a defined evaluation set (Target: v3.0.0-beta1; Wave 5)

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
- [ ] Real whisper.cpp integration validated end-to-end (requires pinned model, licensed fixtures, and registered test; Target: v2.5.0-rc1; Wave 1)
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
- [ ] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: v2.5.0-beta1; Wave 1 registers the benchmark and pins model/hardware)
- [!] Basic `getStatistics()` counters exist; runbook alerts/metrics are not demonstrated by module source.
