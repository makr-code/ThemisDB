> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Status: current | validated: 2026-08-17 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog - Server Module

All notable changes to the server module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Changed
- Documentation governance sync: README, ARCHITECTURE, SECURITY, CHANGELOG, and PERFORMANCE_EXPECTATIONS aligned to source-verifiable server behavior.
- Performance expectations updated from mixed network/security proxy mapping to explicit benchmark symbols from bench_api_endpoints.cpp and bench_stream_protocol.cpp.

## [2.0.0] - 2026-08

### Fixed
- **noexcept build-blocker cleanup (2026-08-17)** — A-5 ThreadSanitizer cleanup pass resolved all remaining noexcept-related build blockers on `develop`.

### Added
- **Phase 5-S01: Wire-protocol retry with exponential backoff (Q3 2026)** — configurable `max_retries`, `base_delay`, global budget cap, and optional jitter; retry eligibility gating (kTransient only; kFatal/kInvalidArg fail-fast); per-request retry-count tracking with thread-safe reset; concurrent sessions validated (2 threads × 8 retries). 16 deterministic WSR test cases in `tests/server/test_server_phase5_hardening.cpp`. Canonical source: `src/server/ROADMAP.md` §Phase 5.
- **Phase 5-S02: HTTP timeout + graceful-shutdown drain (Q3 2026)** — per-request deadline enforcement (kTimedOut on overrun); graceful-shutdown state machine (kRunning → kDraining → kStopped); idle-connection and keepalive-timeout recycling semantics. 12 deterministic HST test cases in `tests/server/test_server_phase5_hardening.cpp`. Canonical source: `src/server/ROADMAP.md` §Phase 5.
- **Phase 1 Security/Auth Hardening (Q2 2026)** — route-by-route auth gate audit complete; API contract frozen at `include/server/server_api_contract.h` (§2 Auth Gate Contract, §6 Error Taxonomy, §8 Threading Guarantees); all 12+ error classes documented with fail-closed semantics. Canonical source: `src/server/ROADMAP.md` §Phase 1.
- **Phase 4 contract hardening test suite (Q2 2026)** — SCH-01..SCH-20: 20 deterministic GTest cases in `tests/server/test_server_contract_hardening_focused.cpp` covering auth, retry, timeout, rate-limit, and protocol contracts (including SCH-15 distributed backend fail-closed, SCH-17..SCH-20 protocol/quorum fault injection). Canonical source: `src/server/ROADMAP.md` §Phase 4.
- **39 focused SRV-01..SRV-39 tests** — retry exhaustion/backoff (SRV-01..08), timeout edge cases (SRV-09..16), graceful-shutdown ordering (SRV-17..24), fault-recovery (SRV-25..31), chaos/failure injection (SRV-32..39); all registered `release_critical;server;phase1`.
- **8 benchmark release gates SVR-01..SVR-08** — `benchmarks/server/bench_server_hotpaths.cpp`; Wave-7 latency baselines (read p99≤200µs, write ≥80k ops/s) confirmed with no regressions from retry/timeout logic.

## [1.9.x] - 2026

### Added
- broader server runtime hardening across request lifecycle, protocol sessions, and reliability paths.

## [1.5.x] - 2026

### Added
- HTTP/3, MCP, extended rate-limiting, and expanded server runtime surfaces.

## [1.0.x] - 2024-2025

### Added
- foundational HTTP server, middleware, and API endpoint runtime infrastructure.