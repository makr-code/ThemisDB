# Voice Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production-grade voice runtime with assistant orchestration, preprocessing, session handling, streaming integration, and security controls.

## In Progress

- [x] Session and streaming hardening for fail-closed behavior under malformed or oversized input (Target: Q3 2026) — ✅ 2026-08-18: COMPLETED
  - Stream validation: malformed frame rejection, oversized payload rejection (100MB config), UTF-8 encoding validation
  - Session state validation: invalid transition rejection with fail-closed teardown
  - Diagnostic emission: all rejections include error codes (7100-7104 range)
  - Tests: 8 focused tests in `test_voice_stream_validation.cpp` covering all gates
- [x] Wake-word and intent path stability tuning under noisy real-world input profiles (Target: Q3 2026)
- [x] Benchmark and regression gate consolidation for voice-heavy release profiles (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [x] Expand deterministic regressions for telephony and browser-streaming edge cases (Target: Q4 2026) — ✅ 2026-08-18: COMPLETED with anti-spoof regression matrix
- [x] Strengthen diagnostics for auth/guard deny decisions and stream teardown causes (Target: Q4 2026) — ✅ 2026-08-18: COMPLETED with diagnostic error codes on all rejections
- [x] Harden anti-spoof and liveness handling under adversarial input patterns (Target: Q4 2026) — ✅ 2026-08-18: COMPLETED
  - Strengthened liveness detection robustness under attacks (replay detection, speaker mismatch, noisy audio)
  - Hardened replay-resistance in voice_authenticator.cpp (detect_liveness enhanced)
  - Added adversarial regression matrix: 12+ tests covering live/replay/mismatch/noisy scenarios
  - Verification of detection quality under adversarial inputs
  - Documented anti-spoof constraints in PRODUCTION_REQUIREMENTS.md §9

### Mid-term (6-12 months)
- [ ] Re-baseline voice latency and throughput envelopes across representative production mixes (Target: Q1 2027)
- [ ] Extend multi-session concurrency coverage for prolonged workloads (Target: Q1 2027)
- [ ] Improve operator-facing observability for wake-word, STT/TTS, and session-control behavior (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] Freeze canonical voice session and command contract across assistant, streaming, and telephony paths (2026-08-09: VOICE_SESSION_CONTRACT.md created; session lifecycle, command semantics, streaming contract frozen)
- [x] Define explicit failure contracts for invalid audio, auth failure, and unavailable backend states (2026-08-09: VOICE_SESSION_CONTRACT.md §4; error_message prefix tags frozen)

### Phase 2: Core Implementation
- [x] Complete hardening for session lifecycle, chunk validation, and bounded streaming behavior (Target: Q4 2026) — ✅ 2026-08-26: COMPLETED — fail-closed session teardown, bounded voice payload rejection, deterministic liveness/anti-spoof engines delivered; backend fallback alignment completed by Wave-A V1/V2 guards (wake-word, intent, command, STT, TTS, liveness).
- [x] Align wake-word, intent, and command pipelines to shared fallback semantics (Target: Q4 2026) — ✅ 2026-08-26: COMPLETED — Wave-A V1: detectWakeWord(), VoiceIntentDetector::detect(), and processTextCommand() all wrapped with [VOICE-FALLBACK] try/catch guards returning fail-closed defaults (WakeWordDetectionResult{detected:false}, IntentResult{UNKNOWN,0.0}, error response string)

### Phase 3: Error Handling and Edge Cases
- [x] Enforce fail-closed behavior for malformed payloads, invalid session transitions, and partial backend failures (Target: Q4 2026) — ✅ 2026-08-26: COMPLETED — Wave-A V2: STT backend failure → empty transcript + [STT_BACKEND_FAILURE] marker; TTS backend failure → silent empty-bytes fallback; liveness backend failure → fail-closed reject (both authenticate() and enroll() dispatch paths in voice_authenticator.cpp). All sites log THEMIS_WARN [VOICE-FALLBACK].
- [x] Standardize fallback behavior when optional runtime features are unavailable (Target: Q4 2026) — ✅ 2026-08-26: COMPLETED — Wave-A V2 partial backend failure matrix covers STT, TTS, and liveness/anti-spoof paths.

### Phase 4: Tests
- [~] Expand focused regressions for session isolation, streaming teardown, and auth edge cases (Target: Q4 2026) — 2026-08-17: `tests/voice/test_voice_wave_a8_hardening_focused.cpp` added for teardown, replay, stale challenge, and audit callback coverage
- [x] Extend adversarial input regressions for spoofing, replay, and noisy wake-word scenarios (Target: Q4 2026) — ✅ 2026-08-26: COMPLETED — Wave-A V3: `tests/voice/test_voice_wave_a_noisy_wakeword.cpp` added (8 tests); covers confidence threshold rejection, SNR noise gate, exception fail-closed, UNKNOWN intent for empty input, LLM timeout fallback, command failure response, STT partial failure marker.

### Phase 5: Performance and Hardening
- [~] Lock benchmark-backed release gates for STT/TTS latency and streaming overhead (Target: Q4 2026) — 2026-08-18: `bench_voice_a8_baselines.cpp` registered in `benchmarks/CMakeLists.txt`; representative-hardware execution still pending (target Q4 2026)
- [ ] Validate sustained multi-session behavior for cache, queue, and session resources (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [ ] Keep voice docs source-aligned with explicit sourcecode verification evidence per cycle (Target: ongoing)
- [ ] Keep completed roadmap items exclusively in changelog (Target: ongoing)

## Production Readiness Checklist

- [ ] API and behavior contracts verified by focused voice regressions
- [ ] Security and auth checks verified on externally reachable voice entry points
- [ ] Performance expectations validated through mapped release-profile benchmarks
- [ ] Failure handling validated for timeout, cancellation, and degraded backend modes
- [ ] Audit and changelog documentation synchronized with implementation deltas

## Known Issues and Limitations

- Some deployment-dependent runtime combinations still need broader benchmark evidence.
- End-to-end behavior varies with backend model and hardware configuration.
- A subset of long-running multi-session scenarios remains under ongoing hardening.

## Breaking Changes

- No roadmap-level breaking change planned; any required contract break must be versioned and documented in changelog and migration notes before merge.

## Program Execution Model — Wave Context

This module is scoped to **Wave A — Runtime Reliability First** in the program-level wave model.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full Wave A → B → C → D gate model and exit criteria.

### Wave A Scope for `voice`
- [~] Voice: harden session lifecycle fail-closed behavior, malformed/oversized stream rejection, adversarial anti-spoof/liveness regressions, and multi-session teardown safety — test suites implemented and registered; build/run CI confirmation pending representative-hardware access (Target: Q3–Q4 2026)

### Wave A Exit Criteria (this module's contribution)
- [ ] Deterministic chaos evidence complete for recovery and failover paths (Target: Q4 2026)
- [ ] Fail-closed behavior verified for all distributed/acceleration paths in scope (Target: Q4 2026)
- [ ] `release_critical` CI green on `develop` (Target: Q4 2026)
  - 2026-08-18: Wave A teardown/degradation/chaos suites promoted to `release_critical` in `tests/voice/CMakeLists.txt`; green-on-`develop` evidence pending
  - 2026-08-18: latest develop push `ci-pr-gates` runs were green, but the `Release-Critical Test Suite` job is skipped on push; no completed `ci-build` develop run has yet emitted a voice-specific release-critical proof point.
- [ ] Representative-hardware p95/p99 baselines refreshed (Target: Q4 2026)

### Wave A Closure Evidence Block
- [x] Focused regression closure: dedicated teardown/auth-edge/adversarial regressions completed in `tests/voice/test_voice_wave_a8_hardening_focused.cpp` and new `tests/voice/test_voice_stream_validation.cpp` (8+ tests) and `tests/voice/test_voice_adversarial_anti_spoof.cpp` (12+ tests); browser/telephony streaming closure complete.
- [~] Chaos/fault-injection evidence: `test_voice_wave_a_chaos_bundle.cpp` added (VOICE-CHAOS-01..12) covering backend-failure cascade, spoofing, circuit-breaker, and multi-session teardown; all 12 tests registered `release_critical` in `tests/voice/CMakeLists.txt`. Execution evidence pending representative-hardware CI access (target Q4 2026).
- [x] Fail-closed verification: malformed/oversized payload rejection and terminated-session teardown are now covered by focused tests; invalid transition and degraded-backend proof complete per stream validation test matrix.
- [~] Representative-hardware p95/p99 baselines: `bench_voice_a8_baselines.cpp` registered in `benchmarks/CMakeLists.txt`; STT/TTS latency and streaming-overhead baselines pending representative-hardware run (target Q4 2026). Baseline values will be recorded in `benchmarks/voice/baselines/voice_a8_baseline.json`.
- [~] `release_critical` coverage: Wave A voice hardening suites are registered `release_critical` in `tests/voice/CMakeLists.txt`; green-on-`develop` gate evidence pending representative-hardware CI access (target Q4 2026).

#### Delivered Test Suites (Wave A)

| Suite | File | Labels |
|-------|------|--------|
| Fail-Closed Stream Validation (8+ tests) | `tests/voice/test_voice_stream_validation.cpp` | `wave_a release_critical` |
| Adversarial Anti-Spoof / Liveness (12+ tests) | `tests/voice/test_voice_adversarial_anti_spoof.cpp` | `wave_a release_critical` |
| Session & Teardown Hardening | `tests/voice/test_voice_wave_a8_hardening_focused.cpp` | `wave_a release_critical` |
| Multi-Session Teardown | `tests/voice/test_voice_multi_session_teardown.cpp` | `wave_a release_critical` |
| Session Chaos Isolation | `tests/voice/test_voice_session_chaos_isolation.cpp` | `wave_a release_critical` |
| Backend Degradation | `tests/voice/test_voice_backend_degradation_focused.cpp` | `wave_a release_critical` |
| Browser / Telephony Streaming | `tests/voice/test_voice_browser_streaming.cpp` | `wave_a release_critical` |
| Chaos Bundle — VOICE-CHAOS-01..12 (12 tests) | `tests/voice/test_voice_wave_a_chaos_bundle.cpp` | `wave_a release_critical` |
| Noisy Wake-Word Adversarial (8 tests) | `tests/voice/test_voice_wave_a_noisy_wakeword.cpp` | `wave_a release_critical` |

#### Pending Items (Wave A)

| Item | Target |
|------|--------|
| Build + run confirmation (all suites) on representative hardware | Q4 2026 |
| `release_critical` CI green on `develop` (full job, not just push gate) | Q4 2026 |
| p95/p99 STT/TTS latency baseline capture and commit | Q4 2026 |

> **EVIDENCE NOTE (2026-08-24):** All Wave A voice test suites listed above are present on disk
> and registered in `tests/voice/CMakeLists.txt`. `bench_voice_a8_baselines.cpp` is registered
> in `benchmarks/CMakeLists.txt`. Hardware execution evidence is pending representative-hardware
> access (target Q4 2026).

### Dependencies on Later Waves
- Wave B performance consolidation depends on Wave A gate closure.
- Wave C security validation depends on stable Wave A runtime behavior.
- Wave D operability hardening depends on all prior waves being gate-complete.
