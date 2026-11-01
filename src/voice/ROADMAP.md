# Voice Module Roadmap

<!-- Last source review: 2026-10-31. Source registrations are not test or benchmark execution evidence. -->

## Current source posture

The tree contains assistant/session, preprocessing, streaming, telephony,
biometric/security helper, storage, and utility implementations. This is an
inventory statement, not a production-readiness judgment. Root
[`../../ROADMAP.md`](../../ROADMAP.md) is authoritative for aggregate project
maturity and release blockers; its current voice-related notes describe
representative-hardware baselines as outstanding.

## Implementation and evidence still needed

- [ ] Refresh representative-hardware p95/p99 voice baselines and publish the
  run artifacts; CMake benchmark registration alone is insufficient.
- [ ] Preserve test/CI execution records alongside source registration. The
  root roadmap reports prior voice CI evidence, but this file update did not
  run or independently verify those jobs.
- [ ] Reconcile the remaining historical Wave A snapshot claims with current
  artifacts before using that snapshot as closure evidence.

## Build and test registration

- Voice sources and compile options are wired in `cmake/CMakeLists.txt`;
  `THEMIS_ENABLE_VOICE_ASSISTANT` defaults to `OFF`.
- `tests/CMakeLists.txt` auto-discovers module CMake files, including
  `tests/voice/CMakeLists.txt`. That file globs `test_*.cpp` and registers
  focused unit targets. It adds `wave_a` and `release_critical` labels only to
  the patterns explicitly listed in its label condition; labels do not imply
  test execution or a passing result.
- `benchmarks/CMakeLists.txt` conditionally adds `benchmarks/voice`.
  `benchmarks/voice/CMakeLists.txt` globs `bench_*.cpp` when
  `THEMIS_BUILD_BENCHMARKS` is enabled.

See [`WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md`](WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md)
for the historical snapshot caveat and
[`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md) for the benchmark
inventory.

## Scope and compatibility

The public interfaces are declared in [`../../include/voice/`](../../include/voice/).
Do not treat [`../../include/voice/VOICE_SESSION_CONTRACT.md`](../../include/voice/VOICE_SESSION_CONTRACT.md)
as a source of current behavior where it conflicts with the current header or
implementation; it is an older, non-normative snapshot.
