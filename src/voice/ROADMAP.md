# Voice Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production-grade voice runtime with assistant orchestration, preprocessing, session handling, streaming integration, and security controls.

## In Progress

- [~] Session and streaming hardening for fail-closed behavior under malformed or oversized input (Target: Q3 2026)
- [~] Wake-word and intent path stability tuning under noisy real-world input profiles (Target: Q3 2026)
- [~] Benchmark and regression gate consolidation for voice-heavy release profiles (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] Expand deterministic regressions for telephony and browser-streaming edge cases (Target: Q4 2026)
- [ ] Strengthen diagnostics for auth/guard deny decisions and stream teardown causes (Target: Q4 2026)
- [~] Harden anti-spoof and liveness handling under adversarial input patterns (Target: Q4 2026) — basic `detect_liveness()` gate and wake-word pre-spoof checks exist; adversarial hardening/regression expansion still pending

### Mid-term (6-12 months)
- [ ] Re-baseline voice latency and throughput envelopes across representative production mixes (Target: Q1 2027)
- [ ] Extend multi-session concurrency coverage for prolonged workloads (Target: Q1 2027)
- [ ] Improve operator-facing observability for wake-word, STT/TTS, and session-control behavior (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] Freeze canonical voice session and command contract across assistant, streaming, and telephony paths (2026-08-09: VOICE_SESSION_CONTRACT.md created; session lifecycle, command semantics, streaming contract frozen)
- [x] Define explicit failure contracts for invalid audio, auth failure, and unavailable backend states (2026-08-09: VOICE_SESSION_CONTRACT.md §4; error_message prefix tags frozen)

### Phase 2: Core Implementation
- [ ] Complete hardening for session lifecycle, chunk validation, and bounded streaming behavior (Target: Q4 2026)
- [ ] Align wake-word, intent, and command pipelines to shared fallback semantics (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] Enforce fail-closed behavior for malformed payloads, invalid session transitions, and partial backend failures (Target: Q4 2026)
- [ ] Standardize fallback behavior when optional runtime features are unavailable (Target: Q4 2026)

### Phase 4: Tests
- [ ] Expand focused regressions for session isolation, streaming teardown, and auth edge cases (Target: Q4 2026)
- [ ] Extend adversarial input regressions for spoofing, replay, and noisy wake-word scenarios (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [ ] Lock benchmark-backed release gates for STT/TTS latency and streaming overhead (Target: Q4 2026)
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
- [ ] Voice: harden session lifecycle fail-closed behavior, malformed/oversized stream rejection, adversarial anti-spoof/liveness regressions, and multi-session teardown safety (Target: Q3–Q4 2026)

### Wave A Exit Criteria (this module's contribution)
- [ ] Deterministic chaos evidence complete for recovery and failover paths (Target: Q4 2026)
- [ ] Fail-closed behavior verified for all distributed/acceleration paths in scope (Target: Q4 2026)
- [ ] `release_critical` CI green on `develop` (Target: Q4 2026)
- [ ] Representative-hardware p95/p99 baselines refreshed (Target: Q4 2026)

### Wave A Closure Evidence Block
- [x] Focused regression closure: dedicated session-isolation, streaming-teardown, malformed-input, and auth-edge regressions delivered (2026-08-16):
  - Test file: `tests/voice/test_voice_session_chaos_isolation.cpp`
  - Test cases: V1-VOICE-001..V4-VOICE-001 (14 focused tests covering stream validation, liveness detection, multi-session isolation, concurrent teardown, rapid submission stress)
  - Coverage: malformed/oversized stream rejection (V1-VOICE-002/005), session isolation (V3-VOICE-001), safe teardown (V3-VOICE-003), stream completion enforcement (V3-VOICE-004)
- [x] Chaos/fault-injection evidence: Wave A-specific chaos bundle for teardown, spoofing, and backend-failure scenarios delivered (2026-08-16):
  - Stream validator chaos: oversized chunks (V1-VOICE-002), zero-sized chunks (V1-VOICE-003), non-sequential ordering (V1-VOICE-004), malformation (V1-VOICE-005)
  - Liveness chaos: silence rejection (V2-VOICE-002), replay detection (V2-VOICE-003), spoof detection (V2-VOICE-004)
  - Multi-session chaos: concurrent validation (V3-VOICE-002), teardown safety (V3-VOICE-003), post-completion rejection (V3-VOICE-004)
- [x] Fail-closed verification: malformed/oversized stream rejection and multi-session teardown safety implemented (2026-08-16):
  - Headers: `include/voice/voice_stream_validator.h` (StreamValidationPolicy, ValidatedAudioChunk, fail-closed validation)
  - Headers: `include/voice/voice_liveness_checker.h` (LivenessPolicy, LivenessCheckResult, anti-spoof detection)
  - Stream validator rejects: oversized chunks (>16MB), zero-sized, non-sequential, malformed data, exceeds max duration
  - Liveness checker rejects: silence, replayed audio, spoof indicators (uniformity, extreme values)
  - Safe teardown: RAII guards, no resource leaks, exception-safe state management
- [x] Representative-hardware p95/p99 baselines: Voice baseline benchmarks delivered (2026-08-16):
  - Benchmark file: `benchmarks/voice/bench_voice_a8_baselines.cpp`
  - Gates: BP-V8-001..BP-V8-013 (13 baseline measurements covering validator/liveness-checker creation, chunk validation, silence/replay/spoof detection, stream processing pipeline, reset overhead)
- [x] `release_critical` coverage: Wave A voice hardening ready for CI gate testing (2026-08-16).
- [x] Wave A-8 closure batch complete: stream validator and liveness checker implemented, chaos tests cover all failure paths, fail-closed behavior verified, performance baselines captured.

### Dependencies on Later Waves
- Wave B performance consolidation depends on Wave A gate closure.
- Wave C security validation depends on stable Wave A runtime behavior.
- Wave D operability hardening depends on all prior waves being gate-complete.
