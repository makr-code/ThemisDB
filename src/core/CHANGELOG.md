> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Core Module

All notable changes to the Core module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]
- Plugin-based adapter loading without recompile (Issue #1706)
- Documentation refresh (Welle 1): core README/ARCHITECTURE/ROADMAP/SECURITY/AUDIT/PRODUCTION_REQUIREMENTS/FUTURE_ENHANCEMENTS aligned with current source and verified issue mapping.

### Roadmap Completion Archive (moved from ROADMAP on 2026-05-31)
- ConcernsContext: central DI hub for cross-cutting concerns
- ILogger abstract interface with severity levels and structured logging
- SpdlogLoggerAdapter for spdlog integration
- NoopLogger for performance-critical paths
- ITracer abstract interface for distributed tracing
- IMetrics abstract interface for counters, gauges, and histograms
- ICache abstract interface for pluggable cache backends
- Factory methods: `create()`, `createNoOp()`, `createCustom()`
- Thread-safe immutable context after creation
- Environment variable detection for production mode
- Lazy initialization for optional components
- OpenTelemetry tracer adapter with circuit-breaker guarded OTLP export
- Prometheus metrics adapter — all IMetrics methods forwarded to MetricsCollector
- Structured log correlation — `span_id` in TraceContext; `ConcernsContext::logWithTrace()` auto-injects active trace/span IDs; SpdlogLoggerAdapter prepends `[trace=…][span=…][req=…]` in plain-text mode
- OpenTelemetry trace and span propagation — `ITracer::startSpanFromHeaders()` (W3C TraceContext inbound extraction) and `ITracer::injectContext()` (outbound header injection + W3C Baggage); implemented in `OpenTelemetryTracerAdapter` with circuit-breaker guard; convenience wrappers on `ConcernsContext`
- Health check interface in ConcernsContext — `ProbeResult`/`HealthStatus` in `lifecycle.h`; `isHealthy()` on all four concern interfaces; `ConcernsContext::healthCheck()` and `readinessCheck()` aggregate per-concern results; `MonitoringApiHandler` exposes per-concern health in `/health/live` and `/health/ready` JSON responses (Issue #1410)
- Jaeger tracing backend adapter — `JaegerTracerAdapter` with `uber-trace-id` propagation and W3C `traceparent` fallback; circuit-breaker guarded; selectable via `tracerAdapter="jaeger"` in `ConcernsContext::Config` (Issue #1413)
- Zipkin tracing backend adapter — `ZipkinTracerAdapter` with B3 single/multi-header and W3C `traceparent` propagation; injects all three header formats on outbound; circuit-breaker guarded; selectable via `tracerAdapter="zipkin"` in `ConcernsContext::Config` (Issue #1413)
- Distributed context propagation (W3C TraceContext standard) — `W3CTraceContextPropagator` extracts `traceparent`/`tracestate` headers into `IContext` (populates `kTraceId`, `kSpanId`) and injects them for outbound calls; `kSpanId` added to `context_keys`; `SimpleContext::toTraceContext()` now includes span_id (Issue #1414)
- Zero-Copy Logging — `ZeroCopyLogger` in `include/core/concerns/zero_copy_logger.h` + `src/core/concerns/zero_copy_logger.cpp`; `string_view` hot-path API (`logSV`/`infoSV`/…/`logStructuredSV`); pre-allocated thread-local format buffer; early `shouldLog()` level-check; PII redaction; `json_mode_` field is `std::atomic<bool>` for safe concurrent `setJsonMode()` + logging; 41 focused tests in `tests/test_zero_copy_logging.cpp`
- Secrets interface for credential injection — `ISecrets` interface; `InMemorySecrets` (map-backed, thread-safe, `setSecret`/`removeSecret`); `EnvSecretsProvider` (env-var backed with configurable prefix); config-driven selection via `Config::secretsAdapter` (`"noop"`, `"inmemory"`, `"env"`); `ConfigValidator` extended; 30+ tests (Issue #1417)

## [1.8.0] — 2026-03-22
### Added
- `InMemorySecrets`: thread-safe map-backed `ISecrets` implementation; pre-populated via `Config::initialSecrets`; `setSecret()`/`removeSecret()` for runtime updates; sorted `listSecretNames()` (Issue #1417)
- `EnvSecretsProvider`: `ISecrets` implementation that reads credentials from environment variables using a configurable prefix (default `THEMIS_SECRET_`); dots and dashes in the secret name are mapped to underscores and the name is upper-cased; `registerName()` enables selective `listSecretNames()` enumeration (Issue #1417)
- `ConcernsContext::Config::secretsAdapter`: selects which secrets provider `create(config)` instantiates — `"noop"` (default), `"inmemory"`, or `"env"` (Issue #1417)
- `ConcernsContext::Config::initialSecrets`: pre-populated key-value pairs for the `"inmemory"` secrets provider (Issue #1417)
- `ConcernsContext::Config::secretsEnvPrefix`: environment-variable prefix used by the `"env"` secrets provider; default `"THEMIS_SECRET_"` (Issue #1417)
- `ConfigValidator::validateAdapterConfig()` extended with `secrets_adapter` parameter; validates against `{"noop", "inmemory", "env"}` (Issue #1417)

## [1.7.0] — 2026-03-09
### Added
- Jaeger tracing backend adapter: `JaegerTracerAdapter` with `uber-trace-id` propagation and W3C `traceparent` fallback; circuit-breaker guarded; selectable via `tracerAdapter="jaeger"` (Issue #1413)
- Zipkin tracing backend adapter: `ZipkinTracerAdapter` with B3 single/multi-header and W3C `traceparent` propagation; all three header formats injected on outbound; circuit-breaker guarded; selectable via `tracerAdapter="zipkin"` (Issue #1413)
- `W3CTraceContextPropagator`: extracts `traceparent`/`tracestate` headers into `IContext`; injects them for outbound calls; `kSpanId` added to context keys (Issue #1414)
- `SimpleContext::toTraceContext()` now includes `span_id`
- Audit event interface for compliance logging (Issue #1418)
- Feature flag interface for runtime enable/disable (Issue #1707)
- Dynamic log level adjustment at runtime (Issue #1412)

## [1.6.0] — 2026-01-20
### Added
- Structured log correlation: `span_id` in `TraceContext`; `ConcernsContext::logWithTrace()` auto-injects active trace/span IDs; `SpdlogLoggerAdapter` prepends `[trace=…][span=…][req=…]` in plain-text mode
- OpenTelemetry trace and span propagation: `ITracer::startSpanFromHeaders()` (W3C TraceContext inbound extraction) and `ITracer::injectContext()` (outbound header injection + W3C Baggage); circuit-breaker guard
- Health check interface in `ConcernsContext`: `ProbeResult`/`HealthStatus`; `ConcernsContext::healthCheck()` and `readinessCheck()` aggregate per-concern results; `MonitoringApiHandler` exposes `/health/live` and `/health/ready` JSON endpoints (Issue #1410)

## [1.5.0] — 2025-11-01
### Added
- OpenTelemetry tracer adapter with circuit-breaker guarded OTLP export (`core/adapters/otel_tracer.cpp`) (Issue #1708)
- Prometheus metrics adapter: all `IMetrics` methods forwarded to `MetricsCollector` (`include/core/concerns/prometheus_metrics_adapter.h`) (Issue #1709)
- Context propagation across async boundaries (Issue #1705)

## [1.0.0] — 2024-01-01
### Added
- `ConcernsContext`: central dependency injection hub for cross-cutting concerns
- `ILogger` abstract interface with severity levels and structured logging
- `SpdlogLoggerAdapter` for spdlog integration and `NoopLogger` for performance-critical paths
- `ITracer` abstract interface for distributed tracing
- `IMetrics` abstract interface for counters, gauges, and histograms
- `ICache` abstract interface for pluggable cache backends
- Factory methods: `create()`, `createNoOp()`, `createCustom()`
- Thread-safe immutable context after creation
- Environment variable detection for production mode
- Lazy initialization for optional components
