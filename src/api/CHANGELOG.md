---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Status: review | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog - API Module

All notable changes to the API module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Documentation
- Refreshed API module documentation against current headers, implementation, tests, and benchmark sources; marked the generated Doxygen snapshot stale.
- `PERFORMANCE_EXPECTATIONS.md` records that several API benchmark target labels do not match their mapped benchmark cases; mapping correction remains open.

### Added (Wave D — Q1 2027 operability, 2026-08-19 to 2026-09-16)
- `tests/api/test_api_wave_d_stress.cpp`: Wave D stress, soak, and operator-hint coverage tests — high-cardinality tracing stress, exporter retry/failure resilience, 100k-request soak simulation, concurrent mixed-load soak, operator ERR_-prefix diagnostics.
- `docs/API_TRANSPORT_RUNBOOK.md`: operator runbook covering GraphQL/gRPC/WebSocket/OTLP failure scenarios, ERR_-prefixed remediation steps, Prometheus alert rule examples, escalation path, and SLA targets.
- `benchmarks/server/bench_api_endpoints.cpp`: server endpoint load benchmarks for transport hot paths.
- `benchmarks/api/bench_api_release_gates.cpp`: release-gate benchmark cases GATE-API-01..06 with documented per-case target budgets.

### Changed (Wave A/B/C documentation finalization, 2026-09-16)
- `src/api/MODULE_GAPS.md`: Wave A, B, C status updated from `[~]` to `[x]` with per-wave closure evidence; Sign-off Gate State extended with Wave A/B/C/D and AUDIT/CHANGELOG sync gates.
- `src/api/AUDIT.md`: open findings API-AUD-01, API-AUD-02, API-AUD-03 closed with detailed resolution evidence; summary updated to reflect 61-test baseline and no open hardening findings.
- `src/api/CHANGELOG.md`: synchronized to Wave D and Wave A/B/C closure baseline; all Phase 1–5 and Q4 2026 entries backfilled.

### Added (Wave B/C HIGH closure — 2026-08-19)
- `src/api/api_transport_policy.cpp`: `applyPolicy()` annotated with per-rule non-retryable rationale; scanner suppression comments added for all 8 `no_retry_logic` findings.
- `src/api/themisdb_grpc_service.cpp`: `setPrometheusRegistry()` refactored — prometheus counter construction moved outside `grpc_metrics_mutex_`; `recordRpcStatus()` refactored to two-phase lookup/creation avoiding lock held across `prometheus::Family::Add()`.
- `src/api/graphql.cpp`: `ThemisSchemaBuilder::build()` wrapped with RAII-safe `try/catch`; partial schema destroyed on any throw; callers receive typed `std::runtime_error`.
- `src/api/ws_handler.cpp`: structured `AuditLogBuilder(AuthorizationFailure)` event added for `cdc:subscribe` rejections.

### Added (Phase 4 + Q4 2026 degraded-mode — 2026-07-18 to 2026-08-19)
- `tests/api/test_api_phase4_concurrency.cpp`: 32×50 concurrency matrix, payload/path boundary, method×version×payload combination matrix, taxonomy correctness, and policy config normalization.
- `tests/api/test_api_degraded_mode.cpp`: capability unavailability, non-gated path success, transient degradation, policy+degraded composition, concurrent mixed traffic, and diagnostic messages.
- `benchmarks/api/bench_api_release_gates.cpp` (14 registered cases in current source): GATE-API-01..06 and sustained/mixed request loops.
- `include/api/api_transport_contracts.h`: `ITransportContract`, `TransportContractValidator`, `TransportCapability`, `TransportFailureClass` (9 classes), `kSupportedApiVersions`, `kMaxPayloadBytes`, `kMaxPathBytes`.
- `include/api/api_error_taxonomy.h`: `ApiErrorTaxonomy` — `toErrorCode()`, `toHttpStatus()`, `toMessage()` (ERR_TRANSPORT_* prefixes), `isClientError()` covering all 9 `TransportFailureClass` values.
- `include/api/api_transport_policy.h` + `src/api/api_transport_policy.cpp`: `TransportPolicyMiddleware` — 5-rule fail-closed `IHttpHandler` + `ITransportContract` decorator; thread-safe, immutable after construction.

## [2.1.0] - 2026-07-18

### Added (Q3 2026 hardening — Phase 1–3 + test baseline)
- `tests/api/test_api_transport_hardening.cpp`: fail-closed behavior, payload/size limits, version negotiation, Content-Type enforcement, correlation ID propagation, and concurrency safety.
- `tests/api/test_api_error_handling.cpp`: error condition coverage, degraded mode, error recovery, and message quality.
- `tests/api/test_api_observability.cpp`: request-lifecycle metrics, queue depth and session count tracking, bounded resource enforcement, and concurrency safety.
- `benchmarks/api/bench_api_transport.cpp` (13 registered cases): mock-handler request handling, validation, response/header construction, correlation-ID, and sequential-request measurements.
- `src/api/graphql_ws_handler.cpp`: fail-closed handling for invalid JSON frames, pre-init messages, missing `id` on subscribe/complete, unknown message types.
- `src/api/graphql_aql_resolver.cpp`: bounded recursion depth (`kMaxComplexityScoringDepth`), overflow-checked complexity accumulation (`checkedAdd`), typed exception handling.
- `src/api/grpc_server.cpp` (critical fix): `lock.lock()` replaced with `lock.try_lock_for(std::chrono::seconds(5))`; fail-closed startup on lock-timeout.

### Fixed
- GraphQL variable substitution behavior hardened in module execution path.

## [1.9.1] - 2026-04-07

### Fixed
- API and gRPC hardening fixes for validation and reporting paths.

## [1.9.0] - 2026-03-25

### Added
- gRPC wiring and GraphQL WebSocket stability/hardening additions.

## [1.7.0] - 2026-03-09

### Added
- GraphQL WebSocket transport, gRPC surface, and async/tenant/API-gateway related additions.

## [1.6.0] - 2026-02-01

### Added
- GraphQL layer, streaming endpoints, tracing middleware, and OTLP exporter additions.

## [1.0.0] - 2024-01-01

### Added
- initial API HTTP/TLS/auth integration foundations.