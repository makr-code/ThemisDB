---
Author: "ThemisDB Team"
Created: "2026-08-19"
Last Updated: "2026-10-31"
Status: "archived"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Voice Module — Wave A Evidence Record

<!-- Historical snapshot created 2026-08-19 and updated 2026-08-24. The source inventory below was rechecked 2026-10-31; the old execution assertions are not current CI evidence. -->

## Current static inventory (2026-10-31)

The following paths exist in the current tree and are included by the module
test/benchmark discovery rules:

- Tests include `test_voice_stream_validation.cpp`,
  `test_voice_adversarial_anti_spoof.cpp`,
  `test_voice_wave_a_chaos_bundle.cpp`,
  `test_voice_backend_degradation_focused.cpp`,
  `test_voice_session_chaos_isolation.cpp`,
  `test_voice_wave_a_noisy_wakeword.cpp`, and other `test_*.cpp` files.
- `tests/voice/CMakeLists.txt` adds `wave_a` and `release_critical` labels only
  for its explicit regex: chaos bundle, stream validation, backend degradation,
  session chaos isolation, and noisy wake-word tests. Other tests listed in
  older versions of this record are not labeled `release_critical` by that
  condition.
- `bench_voice_a8_baselines.cpp` exists and is picked up by
  `benchmarks/voice/CMakeLists.txt` when `THEMIS_BUILD_BENCHMARKS` is enabled.

These are source and registration facts only. No test/benchmark execution or
green CI result is established by this document update. The root
[`../../ROADMAP.md`](../../ROADMAP.md) remains authoritative for aggregate
project status and says representative-hardware voice baselines remain open.

## Historical-record caveat

The original 2026-08 snapshot reported individual test counts, passing
execution, CI closure, error-code ranges, and planned artifact paths. Those
claims have not been independently revalidated here and must not be used as
current proof. In particular, do not infer test pass status from test source
presence or CMake labels. Consult current run artifacts and maintainer
approval before claiming Wave A closure.

## Canonical source references

- [`../../tests/voice/CMakeLists.txt`](../../tests/voice/CMakeLists.txt)
- [`../../benchmarks/voice/CMakeLists.txt`](../../benchmarks/voice/CMakeLists.txt)
- [`ROADMAP.md`](ROADMAP.md)
- [`../../ROADMAP.md`](../../ROADMAP.md)
