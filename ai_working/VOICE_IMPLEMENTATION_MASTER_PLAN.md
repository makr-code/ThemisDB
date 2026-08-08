# Voice Module Implementation Master Plan

**Status**: Phase 0 Initiation  
**Started**: 2026-08-08  
**Target Completion**: 2027-04-01 (GA)  
**Total Duration**: ~29 weeks  

## Executive Summary

This document tracks the complete implementation of the ThemisDB voice module across 6 phases using coordinated sub-agents. The module currently has 11.483 LOC with zero stubs (100% production code), 603 tests, and requires hardening of session management, security controls, operational observability, and performance validation.

### Key Statistics
- **Current LOC**: 11,483
- **Test Coverage**: 603 tests
- **Components**: 20+ .cpp implementations
- **Stub Density**: 0% (verified production code)
- **Phase Target**: GA 2027-04-01

---

## Phase 0: Scope & Planning (Week 1: Aug 8-15, 2026)

### Sub-Task 0.1: Dependency & Symbol Audit
- **Agent**: explore
- **Status**: 🟡 RUNNING
- **Target**: 2026-08-10
- **Deliverable**: `VOICE_SCOPE_AUDIT_PHASE0.md`
  - Header audit (all public APIs in `include/voice/*.h`)
  - Dependency graph (voice → llm, security, content, storage)
  - Error code ranges audit
  - Test infrastructure mapping

### Sub-Task 0.2: Implementation Status Analysis
- **Agent**: general-purpose
- **Status**: 🟡 RUNNING
- **Target**: 2026-08-12
- **Deliverable**: `VOICE_IMPLEMENTATION_STATUS_PHASE0.md`
  - Coverage of all 20+ .cpp implementations
  - Test distribution and gaps
  - Stub/simulation code inventory
  - FUTURE_ENHANCEMENTS extraction
  - ROADMAP status alignment

### Sub-Task 0.3: Risk & Dependency Review
- **Agent**: (pending results from 0.1, 0.2)
- **Target**: 2026-08-15
- **Deliverable**: `VOICE_PHASE0_RISK_ASSESSMENT.md`
  - Blocking dependencies
  - Cross-module impact analysis
  - Build/test infrastructure gaps
  - Sub-agent workload distribution

---

## Phase 1: Session & Streaming Hardening (Q3 2026)

### 1.1 Session-Lifecycle Invariants (Sep 1-30, 2026)
- **Agent**: themisdb-implementer
- **Target**: 2026-09-30
- **Components**: `voice_session_manager.{cpp,h}`
- **Tasks**:
  - Formalize state machine (CREATED→ACTIVE→CLOSING→CLOSED)
  - Atomic transitions with locking
  - Fail-closed error handling
  - Bounded chunk validation
- **Tests**: VSM-01..08 (lifecycle, transitions, concurrency)
- **Build**: `module_voice_test_session_manager_focused`

### 1.2 Stream Teardown & Retry (Oct 1-15, 2026)
- **Agent**: themisdb-implementer
- **Target**: 2026-10-15
- **Components**: `voice_browser_streaming.cpp`, `voice_telephony.cpp`
- **Tasks**:
  - Deterministic connect/disconnect/error semantics
  - Retry-fallback standardization
  - Timeout & partial-input handling
  - Diagnostics-emitter for teardown causes
- **Tests**: VBS-01..08, VTP-01..08
- **Build**: `module_voice_test_browser_streaming_focused`, `module_voice_test_telephony_focused`

### 1.3 Error Handling & Edge Cases (Oct 16-30, 2026)
- **Agent**: themisdb-reviewer (parallel)
- **Target**: 2026-10-30
- **Tasks**:
  - Verify error-code range [8000-8099]
  - Fail-closed enforcement review
  - Bounded-memory semantics verification
  - Malformed-payload handling tests
- **Tests**: Extended `test_voice_production.cpp`
- **Build**: `module_voice_test_production_focused`

---

## Phase 2: Security & Auth Hardening (Q4 2026)

### 2.1 Anti-Spoof & Replay-Resistance (Nov 1-15, 2026)
- **Agent**: themisdb-implementer (security-focused)
- **Target**: 2026-11-15
- **Components**: `voice_authenticator.cpp`, `voice_security.cpp`
- **Tasks**:
  - Replay-detection implementation
  - Liveness-check against adversarial inputs
  - Session-binding for auth-tokens
  - Auth-deny diagnostics
- **Tests**: VSC-01..08 (security, anti-spoof)
- **Build**: `module_voice_test_security_features_focused`

### 2.2 Security Diagnostics & Envelopes (Nov 16-30, 2026)
- **Agent**: themisdb-reviewer (parallel)
- **Target**: 2026-11-30
- **Tasks**:
  - Deny-path diagnostics mapping
  - Degraded-backend handling
  - Risk-classification (High/Medium/Low)
- **Build**: Security regression suite

---

## Phase 3: Operational Hardening & Observability (Q4 2026)

### 3.1 Wake-Word & Intent Observability (Nov 1-15, 2026)
- **Agent**: themisdb-implementer
- **Target**: 2026-11-15
- **Components**: `wake_word_detector.cpp`, `voice_intent_detector.cpp`
- **Tasks**:
  - Emitter-pattern for wake-word events
  - Intent-classification diagnostics
  - State-transition logging
- **Tests**: WW-01..08, VID-01..08
- **Build**: `module_voice_test_*_focused`

### 3.2 Browser/Telephony Reliability (Nov 16-Dec 1, 2026)
- **Agent**: themisdb-implementer (parallel)
- **Target**: 2026-12-01
- **Components**: Streaming components + session cleanup
- **Tasks**:
  - Streaming-error classification
  - Telephony-fallback paths
  - Long-running resource cleanup
  - Leak-prevention hardening
- **Tests**: Endurance & stress tests (100+ sessions)

---

## Phase 4: Performance Baseline & Benchmarking (Q1 2027)

### 4.1 STT/TTS Latency & Throughput Gates (Jan-31, 2027)
- **Agent**: task
- **Target**: 2027-01-31
- **Location**: `benchmarks/voice/`
- **Tasks**:
  - STT p95/p99 latency (Target: ≤500ms per-frame)
  - TTS throughput (Target: ≤1000ms per-sentence)
  - Wake-word latency (Target: ≤100ms on-device)
  - Preprocessing overhead (Target: ≤50ms)
- **Build**: `ctest --label voice;gates`

### 4.2 Multi-Session Concurrency & Resource-Bounding (Feb 1-15, 2027)
- **Agent**: themisdb-implementer
- **Target**: 2027-02-15
- **Components**: `voice_model_cache.cpp`, session registry
- **Tasks**:
  - Cache-eviction policies
  - Queue-growth prevention
  - Session-resource tracking
  - Endurance tests (100+ concurrent sessions)
- **Build**: Concurrency benchmark suite

---

## Phase 5: Documentation Sync & Acceptance (Q1 2027)

### 5.1 Doxygen API Docs (Feb 1-28, 2027)
- **Agent**: themisdb-doc-orchestrator
- **Target**: 2027-02-28
- **Location**: `include/voice/*.h`
- **Tasks**:
  - Update all public APIs (@brief, @param, @return, @throws)
  - Document preconditions, postconditions, error-semantics
  - Include session-lifecycle & failure-contracts
- **Verify**: `doxygen Doxyfile` (zero warnings)

### 5.2 ROADMAP & Changelog Sync (Mar 1-5, 2027)
- **Agent**: themisdb-doc-orchestrator
- **Target**: 2027-03-05
- **Tasks**:
  - Move completed items → CHANGELOG
  - Update FUTURE_ENHANCEMENTS for Q2 2027+
  - Verify production-readiness checklist
- **Location**: `src/voice/{ROADMAP.md, CHANGELOG.md, FUTURE_ENHANCEMENTS.md}`

### 5.3 Known Limits Documentation (Mar 6-10, 2027)
- **Agent**: themisdb-reviewer
- **Target**: 2027-03-10
- **Tasks**:
  - Document deployment-combination limitations
  - Hardware requirements (CPU, Memory, GPU)
  - Backend-availability fallback strategies
- **Location**: `src/voice/PRODUCTION_REQUIREMENTS.md`

---

## Phase 6: Release-Hardening & Acceptance (Q1 2027)

### 6.1 Wave-6-Style Integration Tests (Mar 1-20, 2027)
- **Agent**: themisdb-implementer
- **Target**: 2027-03-20
- **Location**: `tests/integration/pipeline/voice_*.cpp`
- **Tests**:
  - VE2E-01..08: Complete voice flows (session → command → output)
  - VRJ-01..08: Multi-session reliability journey
  - VSS-01..08: Stress/soak stability
- **Build**: `ctest --label voice;wave6`

### 6.2 Performance-Gate Validation (Mar 21-25, 2027)
- **Agent**: task
- **Target**: 2027-03-25
- **Tasks**:
  - Execute all benchmark gates
  - Validate release-profile baselines
  - Generate regression report & sign-off
- **Build**: `ctest --label voice;gates`

### 6.3 Security Sign-Off (Mar 26-28, 2027)
- **Agent**: themisdb-reviewer
- **Target**: 2027-03-28
- **Tasks**:
  - Penetration-test evidence collection
  - Sanitizer/ASAN evidence for voice paths
  - Auth & anti-spoof sign-off
- **Deliverables**: GA_VOICE_SECURITY_EVIDENCE.md

### 6.4 Production-Readiness Final (Mar 29-31, 2027)
- **Agent**: general-purpose
- **Target**: 2027-03-31
- **Tasks**:
  - Verify all ROADMAP checklist items
  - Finalize CHANGELOG & release notes
  - Create final PR & merge to `develop` (community)
- **Deliverable**: Production GA PR

---

## Success Criteria (Per Milestone)

- ✅ All component tests pass (no timeouts, no failures)
- ✅ Benchmarks show zero regression vs baseline
- ✅ Security & performance gates passed
- ✅ Doxygen documentation complete & updated
- ✅ ROADMAP/Changelog synchronized
- ✅ Code coverage ≥ 85% for critical paths
- ✅ Performance targets met (p95/p99)
- ✅ Zero production-known-issues above gate-threshold

---

## Sub-Agent Assignments

| Agent | Phases | Primary Focus |
|---|---|---|
| **themisdb-implementer** | 1-6 | Production hardening, feature impl, tests |
| **themisdb-reviewer** | 1-6 (parallel) | Security, error handling, validation |
| **themisdb-doc-orchestrator** | 5-6 | Doxygen, ROADMAP, governance sync |
| **task** | 4, 6 | Build, testing, benchmarking, CI gates |
| **general-purpose** | 0, 6 | Coordination, assembly, release sign-off |
| **explore** | 0 | Scope audit, dependency analysis |

---

## Timeline Summary

| Phase | Target | Duration | Status |
|---|---|---|---|
| Phase 0 | 2026-08-15 | 1 week | 🟡 IN PROGRESS |
| Phase 1 | 2026-10-30 | 8 weeks | ⏳ PENDING |
| Phase 2 | 2026-11-30 | 4 weeks | ⏳ PENDING |
| Phase 3 | 2026-12-01 | 4 weeks | ⏳ PENDING |
| Phase 4 | 2027-02-15 | 6 weeks | ⏳ PENDING |
| Phase 5 | 2027-03-10 | 3 weeks | ⏳ PENDING |
| Phase 6 | 2027-03-31 | 3 weeks | ⏳ PENDING |
| **GA** | **2027-04-01** | **~29 weeks** | ⏳ TARGET |

---

## Progress Tracking

Progress is tracked incrementally via:
1. `engine-tools-report_progress` commits after each sub-task
2. Agent completion notifications and handoff reviews
3. Build/test verification for each phase
4. This master plan updated weekly with status

---

## Notes

- User prefers larger remediation batches per commit ("weiter"/"nächster block")
- All sub-agents run in background for parallelism
- Critical-path dependencies: Phase 1 → Phase 2 → Phase 3 → Phase 4 → Phase 5 → Phase 6
- Phase 0 completion is gate for Phase 1 start
