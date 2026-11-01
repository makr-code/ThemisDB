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

# Whisper Performance Expectations

## Scope and evidence

This page covers `benchmarks/whisper/bench_whisper_transcription.cpp`. No measured performance
results are present in this repository documentation; the values below are workload labels or
source-level budgets, not verified baselines.

## Benchmark cases in source

| Case | Workload | Evidence/limit |
|---|---|---|
| `Transcribe_1s`, `Transcribe_5s`, `Transcribe_30s_CLIParity` | PCM buffers at 16 kHz | Stub mode measures dispatch overhead; a comment sets a real-model budget of less than 60 seconds for 30 seconds of audio on a 4-core CPU. This has not been measured here. |
| `Transcribe_8kHz_1s` | 8 kHz, one-second buffer | API boundary case; no target latency is documented. |
| `DetectLanguage` | One-second, 16 kHz buffer | No target latency is documented. |
| `TranscribeFile_NonExistent` | Missing-file error path | Error-path overhead, not decode/transcription throughput. |
| `StatsQuery` | Statistics read after warm-up | No target latency is documented. |
| `BM_WhisperStub_Direct` | Direct stub transcriber | Dispatch-only path. |
| `BM_Transcribe_BufferSize` | 100 ms through 30 s PCM | Buffer-size sweep; runs with the selected backend. |
| `BM_WhisperRealModel_1min` | One-minute synthetic PCM | Compiled only when whisper.cpp and `THEMIS_BENCH_WHISPER_MODEL_PATH` are configured. |

The benchmark accepts `THEMIS_BENCH_WHISPER_MODEL_PATH` from the environment or compile-time
definition. Without a usable model/backend, the default path is stub mode and does not measure
recognition performance.

## Build registration gap

The source is present, but `benchmarks/CMakeLists.txt` guards target creation using
`EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_whisper_transcription.cpp"`. The checked-in source is
`benchmarks/whisper/bench_whisper_transcription.cpp`, and no root-level file with the guarded name
exists. Therefore target registration is not established by the current CMake condition.

- [ ] Correct the CMake existence guard and run the benchmark on named hardware/model before
  publishing latency or throughput baselines (Target: TODO).
- [ ] Record real-model p50/p95/p99, model digest, CPU/GPU, thread count, and build flags
  (Target: TODO).
