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

# Whisper Benchmarks

`bench_whisper_transcription.cpp` measures plugin/stub dispatch, PCM buffer-size cases,
language detection, statistics lookup, and a missing-file fast-fail path. A conditional
`BM_WhisperRealModel_1min` case is included when whisper.cpp and a model path are configured.
Without that backend/model it does not measure recognition quality or real inference latency.

`benchmarks/CMakeLists.txt` contains an `add_executable` stanza for this source, but its target
guard checks for `benchmarks/bench_whisper_transcription.cpp`; the checked-in source is
`benchmarks/whisper/bench_whisper_transcription.cpp`. The benchmark target is therefore not
registered by the current guard. See [`../../src/whisper/PERFORMANCE_EXPECTATIONS.md`](../../src/whisper/PERFORMANCE_EXPECTATIONS.md)
for the verified workload inventory and follow-up.
