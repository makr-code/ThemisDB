> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog - Base Module

All notable changes to the base module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Added — Wave D delivery (2026-09-16)
- **Distributed tracing**: `include/themis/base/trace_context.h` integrated into
  `HotReloadManager`; `reloadModule()` and `rollback()` now emit `ScopedSpan`
  events. Injectable `SpanEmitter` added via `setSpanEmitter()` / `spanEmitter()`.
- **Exporter reliability counters**: `RemoteRegistryClient::RequestStats` now
  accumulates `retry_exhausted_count` and `timeout_count` at runtime;
  `ObservabilityHook` is fired on each event (retry-exhausted and timeout).
- **High-cardinality stress and fail-closed tests**: `tests/base/test_base_wave_d_tracing.cpp` —
  200-concurrent reload stress, mixed concurrent operations, fail-closed
  verification, tracing integration, all BASE_LOADER_*/BASE_SANDBOX_* code coverage.
- **Long-duration soak tests**: `tests/base/test_base_soak.cpp` —
  reload/rollback/re-register cycle (THEMIS_SOAK_ITERATIONS), stats monotonicity,
  and concurrent soak; registered under `ctest -L soak`.
- **Benchmark baseline file**: `benchmarks/baselines/base/wave_d_baselines.json` —
  GATE-BASE-01..12 threshold definitions for CI gate enforcement.
- **Operator runbook**: `src/base/RUNBOOK.md` — full operator guide covering all
  BASE_* error-code groups, diagnostics, remediation steps, and Prometheus alerts.

### Changed
- Documentation governance sync: README, ARCHITECTURE, SECURITY, ROADMAP, FUTURE_ENHANCEMENTS, AUDIT, and PERFORMANCE_EXPECTATIONS aligned to source-verifiable module behavior.

### Changed — Documentation audit (2026-10-31)
- Reconciled module documentation with current headers, implementations, and build definitions; see [AUDIT.md](AUDIT.md).
- Clarified that modular CMake creates `themis_base` and exposes `themis_core` as an interface target; the benchmark directory contains sources of differing scopes.
- `tests/CMakeLists.txt` auto-discovers `tests/base/CMakeLists.txt`; its focused unit tests and focused two-hour soak target are registered with module/tier/kind/focus labels (plus `base` and `soak`). Registration does not assert that the targets build or tests pass.
- Runbook metric and management-command examples are illustrative, not built-in contracts; benchmark gate thresholds are definitions, not measured results. See [RUNBOOK.md](RUNBOOK.md) and [the benchmark notes](../../benchmarks/base/README.md).

## [1.8.0] - 2026-03-22

### Added
- wasm runtime and sandbox hardening infrastructure enhancements.

## [1.7.0] - 2026-03-09

### Added
- plugin watchdog and runtime reliability enhancements.

## [1.6.0] - 2026-02-10

### Added
- sandboxing, dependency, remote registry, and hot-reload expansion.

## [1.0.0] - 2024-01-01

### Added
- initial secure module-loading and lifecycle foundations.