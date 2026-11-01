---
Author: "ThemisDB Team"
Created: "2026-06-13"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Production Requirements — Whisper Plugin

No root-level `PRODUCTION_REQUIREMENTS.md` exists in this checkout. This page records module
implementation facts and open validation requirements; it does not certify production readiness.

## Module scope

- `WhisperPlugin`: `audio::IAudioBackend` implementation and orchestration.
- `WhisperConfig`: JSON configuration, optional model digest, language-confidence threshold.
- `WavAudioChunkReader`, `FfmpegAudioChunkReader`, and `CompositeAudioChunkReader`: whole-file
  decoding to float32 PCM.
- `IWhisperTranscriber`: model interface; `WhisperCppTranscriber` is conditional on
  `THEMIS_ENABLE_WHISPER`, `libwhisper`, and a model.
- `EnergyThresholdVad`: optional RMS detector applied by `transcribeStream()` when installed.

## Verified implementation behavior

| Area | Current behavior | Limitation / evidence gap |
|---|---|---|
| Language/build | The module target uses C++17. `THEMIS_ENABLE_WHISPER` defaults off; `libwhisper` is optional. | Real-model behavior requires separate model-backed validation. |
| Stub | Returns empty text, language `"unknown"`, and confidence `0`; result type defaults to success. | It is not speech recognition and must not be used as evidence of inference quality. |
| File input | WAV supports 16-bit PCM and float32; FFmpeg reader handles listed extensions and emits 16 kHz mono float32. | WAV is read wholly into memory without a module size limit. FFmpeg is an optional runtime executable. |
| Model digest | Non-empty `model_sha256` is checked before whisper.cpp loads the model. | Integrity check is optional and does not prove publisher authenticity. |
| Streaming | Callback API exists; default transcriber emits one completed-text token. | `WhisperCppTranscriber` does not provide incremental token output. |
| Diarisation | API and orchestration exist. | Current whisper.cpp implementation returns one whole-transcript segment and one speaker ID; it does not distinguish speakers. |
| VAD | RMS threshold detector and streaming integration exist. | `transcribe()` does not apply VAD; accuracy/performance has no measured production baseline. |
| Statistics | `getStatistics()` returns plugin/model/initialized fields, transcription/error counts, and last error. | No module metrics exporter or duration histogram is implemented here. |
| Concurrency | Calls into one transcriber instance are serialized; state/counters use atomics or mutexes. | This does not establish concurrent throughput; requests on one instance wait in the mutex. |

## Build, tests, and benchmark

- Root `tests/CMakeLists.txt` registers `WhisperPluginFocusedTests` from
  `src/whisper/tests/test_whisper_plugin.cpp` (69 source test cases).
- `tests/whisper/CMakeLists.txt` discovers the stub bridge and synthetic high-cardinality test
  sources (three cases each).
- `src/whisper/tests/test_whisper_plugin_registrar.cpp` contains 12 tests but is not registered
  by current CMake.
- `benchmarks/whisper/bench_whisper_transcription.cpp` contains stub and conditional real-model
  cases. Current benchmark CMake target guard checks a missing root-level file, so target
  registration is not established.
- TODO: configure and run the real backend with a pinned model, record the digest and hardware,
  and capture success/failure evidence before claiming inference readiness.

## Deployment responsibilities

- Require callers to authorize file paths and set request/file limits; the module has no WAV size
  cap, request rate limiter, or transcription timeout.
- Install `ffmpeg` only when non-WAV format support is required.
- If model integrity is required, provide the expected digest through a trusted distribution path.
- Do not treat the runbook's operational metric names, alert names, or CLI commands as implemented
  by this module; verify them against the deployed server integration.

## Open acceptance items

- [ ] Run and record real-model transcription validation on pinned model/hardware (Target: TODO).
- [ ] Implement/validate actual incremental whisper.cpp callbacks or document the API as completed-text fallback (Target: TODO).
- [ ] Define and validate a real multi-speaker diarisation backend and quality metric (Target: TODO).
- [ ] Register the benchmark target and publish reproducible performance percentiles (Target: TODO).

Format: THEMIS_PRODUCTION_REQUIREMENTS_V1
