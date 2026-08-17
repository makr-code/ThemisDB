> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks ueberfuehren. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Server Module Roadmap

## Current Status
Production-ready server stack with HTTP/1.1, HTTP/2, HTTP/3, WebSocket, MQTT, PostgreSQL wire protocol, gRPC, GraphQL, and MCP integration. Core API gateway, auth middleware, validation, and observability paths are available in production deployments.

## Recently Completed
- [x] Phase 5 Server Hardening — P5-S01 Wire-Protocol Retry + P5-S02 HTTP Timeout/Shutdown — Completed Q3 2026 (Validated 2026-07-20)
  - P5-S01: Exponential-backoff retry gate with configurable max_retries, base_delay, budget cap, and optional jitter
  - P5-S01: Retry eligibility gating (kTransient only; kFatal/kInvalidArg fail-fast)
  - P5-S01: Per-request retry-count tracking with thread-safe reset; concurrent sessions validated (2 threads × 8 retries)
  - P5-S01: 16 deterministic WSR test cases; all pass (test_server_phase5_hardening)
  - P5-S02: In-process server stub with per-request deadline enforcement (kTimedOut on overrun)
  - P5-S02: Graceful-shutdown drain logic (ServerState kRunning → kDraining → kStopped)
  - P5-S02: Idle-connection and keepalive-timeout recycling semantics
  - P5-S02: 12 deterministic HST test cases; all pass (test_server_phase5_hardening)
- [x] Voice API Bearer-Token JWT/OIDC Validation (#302) — Completed Q2 2026 (Validated 2026-07-19)
  - JWT signature validation using JWTValidator from JWKS
  - Token expiry (exp claim) checking
  - Issuer (iss claim) validation
  - Audience (aud claim) validation ("themis-voice-api")
  - Token revocation (JTI blacklist) support
  - Fail-closed rejection on any validation failure
  - Comprehensive test coverage for all validation scenarios

## In Progress
- [~] P0 security/code-quality remediation wave for server paths (Target: Q2 2026)
  - Status: 2,172 verified gaps identified and categorized (2026-06-25); 654 actionable (Critical + High severity)
  - [ ] Finish remaining true-positive triage from gap scan and remove residual high-risk findings from active code paths (Target: Q2 2026)
  - [ ] Consolidate auth enforcement checks for all routing-layer special cases and keep regression tests green (Target: Q2 2026)
- [x] Phase 5-S kickoff: wire-protocol retry/idempotency hardening batch (Target: Q3 2026 → delivered Q3 2026)
  - [x] Idempotency cache lookup now serves thread-local snapshots and `lookupSnapshot()` exposes by-value reads without exposing unlocked internal storage
  - [x] Zero-window idempotency configuration fails safe by disabling retention rather than growing unbounded state
  - [x] P5-S01: wire-protocol retry with exponential backoff (16 WSR tests PASS) and P5-S02: HTTP timeout + graceful-shutdown (12 HST tests PASS) — `tests/server/test_server_phase5_hardening.cpp`
- [x] GA Sign-off evidence bundling for delivered Phase-5 hardening (Target: Q3 2026 → delivered 2026-08-04)
  - [x] Residual-risk register for retry/timeout/shutdown release-critical paths documented in `docs/governance/GA_PROMOTION_SIGN_OFF.md`
  - [x] `release_critical` regression proof on `develop` confirmed via `.github/workflows/09-pr-gates_release-critical-tests.yml`
  - [x] Failure/recovery sign-off evidence linked into root gate board docs and `FINAL_GA_READINESS_CHECKLIST.md`

## Planned Features

### Short-term (3-6 months)
- [ ] Plugin-based server adapter loading with signature validation and rollback guardrails (Target: Q4 2026)
- [ ] Cluster-wide distributed rate-limit state hardening for mixed-node latency profiles (Target: Q4 2026)
- [ ] GraphQL federation and schema governance hardening for multi-service deployments (Target: Q4 2026)
- [ ] HTTP/3 congestion-control and connection migration tuning under production-like packet loss (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] Passwordless WebAuthn/FIDO2 auth integration for admin and API scopes (Target: Q1 2027)
- [ ] CPU- and memory-governed WASM execution hardening with stricter runtime policy envelopes (Target: Q1 2027)
- [ ] Service-mesh policy sync hardening and failover behavior validation under partition scenarios (Target: Q1 2027)

## Implementation Phases

### Phase 1: Security and Access Hardening
- [x] Complete route-by-route auth gate audit for privileged server endpoints — frozen API contract: `include/server/server_api_contract.h` (§2 Auth Gate Contract, §6 Error Taxonomy, §8 Threading Guarantees) (Target: Q2 2026)
- [x] Close remaining scanner-confirmed high-severity auth/logging findings with regression tests — `include/server/server_api_contract.h` documents all 12+ error classes and fail-closed semantics; SCH-01..SCH-20 regression tests in `tests/server/test_server_contract_hardening_focused.cpp` (Target: Q2 2026)

### Phase 2: Protocol and Gateway Hardening
- [ ] Improve HTTP/3 production behavior under migration/retransmit stress (Target: Q4 2026)
- [ ] Extend gateway resilience tests for quorum loss and split-brain protection paths (Target: Q4 2026)

### Phase 3: Validation and Contract Governance
- [ ] Strengthen OpenAPI/JSON-Schema drift detection for handler registration changes (Target: Q4 2026)
- [ ] Add stricter backward-compat checks for gRPC and REST versioning contracts (Target: Q4 2026)

### Phase 4: Tests and Reliability Gates
- [x] Expand integration and soak coverage for mixed protocol traffic (HTTP/gRPC/WebSocket/MQTT) — 20 deterministic GTest cases SCH-01..SCH-20 in `tests/server/test_server_contract_hardening_focused.cpp` covering auth, retry, timeout, rate-limit, and protocol contracts (Target: Q4 2026)
- [x] Add deterministic fault-injection tests for distributed rate-limit and fallback behavior — SCH-15 (distributed backend fail-closed), SCH-17..SCH-20 (protocol/quorum fault injection) in `tests/server/test_server_contract_hardening_focused.cpp` (Target: Q4 2026)

### Phase 5: Performance and Operational Hardening
- [x] P5-S01: Wire-protocol retry with exponential backoff (2-3 retries + budget cap + jitter) — Completed Q3 2026
- [x] P5-S02: HTTP timeout patterns + graceful shutdown drain semantics — Completed Q3 2026
- [x] Re-baseline server latency/throughput gates with production-like payload mixes — 8 release-gate benchmarks SVR-01..SVR-08 delivered in `benchmarks/server/bench_server_hotpaths.cpp` (Target: Q1 2027)
- [ ] Add adaptive tuning recommendations for queue/backpressure settings by deployment profile (Target: Q1 2027)

### Phase 1: Top-Risk Module Hardening (Retry/Timeout/Graceful-Shutdown/Recovery)
- [x] Implemented Consistent Retry Semantics (Target: Q3 2026)
  - SRV-01..08: Retry exhaustion & backoff scenarios (8 tests) ✓
  - SRV-01: Retry exhaustion when max_retries exceeded
  - SRV-02: Immediate success (no backoff)
  - SRV-03: Recovery on second attempt
  - SRV-04: Exponential backoff validation
  - SRV-05: Global budget timeout enforcement
  - SRV-06: Zero-latency success path
  - SRV-07: Fatal error fails fast (no retry)
  - SRV-08: Transient→Fatal mixed error codes
- [x] Implemented Graceful Shutdown & In-Flight Cleanup (Target: Q3 2026)
  - SRV-09..16: Timeout edge cases (pre/at/post deadline) (8 tests) ✓
  - SRV-17..24: Graceful shutdown ordering (drain, timeout, health checks) (8 tests) ✓
  - SRV-09: Pre-deadline completion
  - SRV-10: Exact deadline boundary
  - SRV-11: Post-deadline timeout detection
  - SRV-12: Zero-budget immediate fail
  - SRV-13: Large timeout remote future
  - SRV-14: Retry with cumulative budget
  - SRV-15: Cancellation early return
  - SRV-16: Timer-driven context deadline
  - SRV-17: Phase ordering (Idle→Draining)
  - SRV-18: Phase ordering (Draining→Complete)
  - SRV-19: Phase ordering (Complete→Done)
  - SRV-20: Clean drain (no active requests)
  - SRV-21: Drain with pending requests
  - SRV-22: Forced close on timeout
  - SRV-23: Pre-shutdown health checks
  - SRV-24: Shutdown phase transition logging
- [x] Wave-7 Regression Validation (Target: Q3 2026)
  - Verified latency gates hold (read p99≤200µs, write≥80k ops/s)
  - No performance regressions from retry/timeout logic
- [x] Created 39 Focused Tests (Target: Q3 2026)
  - SRV-01..08: Retry exhaustion & backoff (8 tests)
  - SRV-09..16: Timeout edge cases (8 tests)
  - SRV-17..24: Graceful shutdown ordering (8 tests)
  - SRV-25..31: Fault-recovery scenarios (7 tests) ✓
  - SRV-32..39: Chaos/failure injection (8 tests) ✓
  - SRV-25: Transient error recovery
  - SRV-26: Permanent error no recovery
  - SRV-27: Circuit breaker open
  - SRV-28: Circuit breaker half-open probe
  - SRV-29: Connection pool reset after recovery
  - SRV-30: Request timeout then recovery
  - SRV-31: Idempotent recovery retry
  - SRV-32: Connection failure injection
  - SRV-33: Latency injection (request slowdown)
  - SRV-34: Connection pool exhaustion
  - SRV-35: Request cancellation under chaos
  - SRV-36: Timeout under high load
  - SRV-37: Partial message loss
  - SRV-38: Quiescent shutdown under chaos
  - SRV-39: Recovery stabilization (eventual consistency)
  - All tests: Use themis_register_module_focused_test(), tier unit, timeout 120s
  - Registered with label: `release_critical;server;phase1`
- [x] Phase 1 Exit Criteria (2026-08-31)
  - 0 new CRITICAL findings in CodeQL
  - 39 focused tests created and passing
  - Wave-7 gates remain PASS (no regressions)
  - Retry/timeout exception-safety audits complete with documented contracts
  - Module-level ROADMAP.md updated with closure status

### Phase 6: Documentation and Release Readiness
- [x] Keep server developer docs aligned with source and routing behavior after each hardening wave — `include/server/server_api_contract.h` freezes all handler registration, auth gate, retry/timeout/backpressure, error taxonomy, lifecycle/ownership, and threading contracts for v1.x (Target: Q2 2026)
- [x] Ensure completed roadmap items are moved only to CHANGELOG and not retained in roadmap history blocks — server ROADMAP Phase 1, Phase 4, Phase 5 checkboxes updated with evidence references (Target: ongoing)

## Production Readiness Checklist
- Status: Tracking in progress (last validated 2026-08-17)
- Nachweise: Integration tests, focused protocol tests, and security regression suites
- Hinweis: Abgeschlossene Arbeit wird ausschliesslich in CHANGELOG dokumentiert.
- Validation Summary: Issue #5622 module evidence validation complete; 9 test cases (100% pass rate) in module_server_test_server_activation_profile_focused
- [x] API contracts frozen and documented for all HTTP/gRPC/WebSocket/MQTT entry points — `include/server/server_api_contract.h`
- [x] Phase 1 Security/Auth Hardening complete — frozen API contract (`include/server/server_api_contract.h` §2 Auth Gate Contract, §6 Error Taxonomy, §8 Threading Guarantees); all 12+ error classes documented with fail-closed semantics
- [x] SCH-01..SCH-20 regression test suite passing — `tests/server/test_server_contract_hardening_focused.cpp`; covers auth, retry, timeout, rate-limit, and protocol contracts
- [x] Phase 4 contract hardening test suite complete — 20 deterministic GTest cases (auth, retry, timeout, rate-limit, protocol fault injection) in `tests/server/test_server_contract_hardening_focused.cpp`
- [x] Phase 5 wire-protocol retry complete (P5-S01) — exponential-backoff retry with configurable budget/jitter; 16 WSR tests pass in `tests/server/test_server_phase5_hardening.cpp`
- [x] Phase 5 HTTP timeout and graceful-shutdown complete (P5-S02) — deadline enforcement, kRunning→kDraining→kStopped drain semantics; 12 HST tests pass in `tests/server/test_server_phase5_hardening.cpp`
- [x] 39 focused SRV-01..SRV-39 tests complete and registered as `release_critical;server;phase1`
- [x] 8 benchmark release gates SVR-01..SVR-08 delivered — `benchmarks/server/bench_server_hotpaths.cpp`; Wave-7 latency baselines (read p99≤200µs, write≥80k ops/s) hold with no regressions from retry/timeout logic
- [x] Voice API Bearer-Token JWT/OIDC validation complete — JWT signature, expiry, issuer, audience, and JTI revocation; fail-closed on any validation failure
- [x] GA evidence bundling and sign-off complete (Batch C) — retry/timeout/shutdown release-critical paths documented in `docs/governance/GA_PROMOTION_SIGN_OFF.md`
- [x] Phase 6 documentation aligned — `include/server/server_api_contract.h` freezes all handler registration, auth gate, retry/timeout/backpressure, error taxonomy, lifecycle/ownership, and threading contracts
- [x] noexcept build-blocker cleanup complete (2026-08-17) — A-5 ThreadSanitizer cleanup pass resolved all remaining noexcept-related build blockers

## Known Issues and Limitations
- Plugin-based adapter loading still requires roadmap delivery.
- Some advanced protocol features require additional soak/fault-injection validation before hard SLA commitments.
- Cross-node consistency for globally distributed rate limits needs further hardening evidence.

## Breaking Changes
- REST versioning remains path-based and backward-compatible for v1 clients.
- gRPC schema evolution remains additive-only for active major lines.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `server`
- [ ] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027)
- [ ] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027)
- [ ] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027)

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [ ] This module's distributed/acceleration paths fail closed (Target: Q1 2027)
- [ ] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027)
- [ ] Operator-critical paths have diagnostics, alerts, and runbooks (Target: Q1 2027)
