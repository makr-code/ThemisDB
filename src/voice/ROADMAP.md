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
