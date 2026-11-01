---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Voice Benchmark Inventory and Evidence Status

This page records benchmark source/registration facts, not measured performance
targets. Historical latency, throughput, memory, availability, and pass-rate
figures previously listed here were not supported by current source or run
artifacts and are intentionally not repeated as guarantees.

## Current benchmark sources

`benchmarks/voice/CMakeLists.txt` returns unless `THEMIS_BUILD_BENCHMARKS` is
enabled, globs `bench_*.cpp`, and registers each source through
`themis_add_standard_benchmark`. `benchmarks/CMakeLists.txt` adds this module
when its CMake file exists.

| Source | Covered workload categories (from benchmark cases) |
|---|---|
| `bench_voice_assistant.cpp` | Session creation/context, commands, audio conversion, wake-word, STT/TTS, storage/meeting/phone |
| `bench_voice_assistant_latency.cpp` | Assistant response/TTS, end-to-end, command routing |
| `bench_voice_audio_preprocessing.cpp` | Validation, preprocessing, wake-word, intent, emotion |
| `bench_voice_dedicated_gates.cpp` | STT/TTS, audio queue, stream-session send |
| `bench_voice_session_lifecycle.cpp` | Create/delete, concurrent creation, scaling and cleanup |
| `bench_voice_streaming_throughput.cpp` | Chunk, latency, buffer, multiple-stream and recovery cases |
| `bench_voice_stt_tts_latency.cpp` | STT/TTS fixture workloads, including duration/noise cases |
| `bench_voice_wake_word_batch.cpp` | Wake-word chunk/window and batch scaling |
| `bench_voice_a8_baselines.cpp` | Validator/liveness construction, chunk/pipeline and multi-session setup |

These are registered benchmark programs and cases, not verified production
baselines. The repository-level roadmap states representative-hardware voice
baseline capture remains open; no current run result is established by these
source files.

## How to report results

For any published measurement, include the exact benchmark target/revision,
build preset and feature options, hardware, fixture/backend, repetitions,
captured metric and raw artifact. Separate mock/fixture results from real
backend results. Do not compare unlike hardware or infer recognition accuracy
from latency benchmarks.

## Test-registration boundary

Voice test executables are registered separately by
[`../../tests/voice/CMakeLists.txt`](../../tests/voice/CMakeLists.txt). Their
presence, CTest registration, or `release_critical` label does not mean they
were built or passed. See the [module roadmap](ROADMAP.md) and
[closure evidence record](WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md) for the current
source-backed status and caveats.

## Canonical references

- [`../../benchmarks/voice/CMakeLists.txt`](../../benchmarks/voice/CMakeLists.txt)
- [`../../benchmarks/CMakeLists.txt`](../../benchmarks/CMakeLists.txt)
- [`../../ROADMAP.md`](../../ROADMAP.md) — aggregate maturity/evidence authority
