# Core Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
**Beta** — Dependency injection, cross-cutting concerns management (logging, tracing, metrics, caching), and pluggable adapter infrastructure are functional. OpenTelemetry and Prometheus adapters are implemented.

## Completed ✅
- [x] ConcernsContext: central DI hub for cross-cutting concerns
- [x] ILogger abstract interface with severity levels and structured logging
- [x] SpdlogLoggerAdapter for spdlog integration
- [x] NoopLogger for performance-critical paths
- [x] ITracer abstract interface for distributed tracing
- [x] IMetrics abstract interface for counters, gauges, and histograms
- [x] ICache abstract interface for pluggable cache backends
- [x] Factory methods: `createForProduction()`, `createForTesting()`, `createCustom()`
- [x] Thread-safe immutable context after creation
- [x] Environment variable detection for production mode
- [x] Lazy initialization for optional components
- [x] OpenTelemetry tracer adapter with circuit-breaker guarded OTLP export
- [x] Prometheus metrics adapter — all IMetrics methods forwarded to MetricsCollector
- [x] Structured log correlation — `span_id` in TraceContext; `ConcernsContext::logWithTrace()` auto-injects active trace/span IDs; SpdlogLoggerAdapter prepends `[trace=…][span=…][req=…]` in plain-text mode
- [x] OpenTelemetry trace and span propagation — `ITracer::startSpanFromHeaders()` (W3C TraceContext inbound extraction) and `ITracer::injectContext()` (outbound header injection + W3C Baggage); implemented in `OpenTelemetryTracerAdapter` with circuit-breaker guard; convenience wrappers on `ConcernsContext`
- [x] Health check interface in ConcernsContext — `ProbeResult`/`HealthStatus` in `lifecycle.h`; `isHealthy()` on all four concern interfaces; `ConcernsContext::healthCheck()` and `readinessCheck()` aggregate per-concern results; `MonitoringApiHandler` exposes per-concern health in `/health/live` and `/health/ready` JSON responses (Issue: #1410)

## In Progress 🚧
- [x] OpenTelemetry tracer adapter (Target: Q2 2026) (Issue: #1404)
- [x] Prometheus metrics adapter (Target: Q2 2026) (Issue: #1405)
- [x] Context propagation across async boundaries (Target: Q3 2026) (Issue: #1406)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [x] OpenTelemetry trace and span propagation (Issue: #1407)
- [x] Prometheus-compatible metrics adapter (Issue: #1408)
- [I] Structured log correlation (trace ID injection into log records) (Issue: #2377)
- [I] Health check interface in ConcernsContext (Issue: #1410)
- [x] Configuration-driven adapter selection (Issue: #1411)
- [I] Dynamic log level adjustment at runtime (Issue: #1412)

### Long-term (6-12 months)
- [I] Jaeger/Zipkin tracing backend adapters (Issue: #1413)
- [I] Distributed context propagation (W3C TraceContext standard) (Issue: #1414)
- [x] Circuit breaker interface as a first-class concern (Issue: #1415)
- [x] Feature flag interface (enable/disable features without redeployment) (Issue: #1416)
- [I] Secrets interface for credential injection into components (Issue: #1417)
- [I] Audit event interface for compliance logging (Issue: #1418)

## Implementation Phases

### Phase 1: Dependency Injection & Core Interfaces (Status: Completed ✅)
- [x] ConcernsContext: central DI hub for cross-cutting concerns (`core/concerns_context.cpp`)
- [x] ILogger abstract interface with SpdlogLoggerAdapter and NoopLogger (`core/adapters/spdlog_logger.cpp`)
- [x] ITracer abstract interface for distributed tracing
- [x] IMetrics abstract interface for counters, gauges, and histograms
- [x] ICache abstract interface for pluggable cache backends
- [x] Factory methods: `createForProduction()`, `createForTesting()`, `createCustom()`
- [x] Thread-safe immutable context after creation
- [x] Environment variable detection for production mode
- [x] Lazy initialization for optional components

### Phase 2: Observability Adapters (Status: Completed ✅)
- [x] OpenTelemetry tracer adapter (`core/adapters/otel_tracer.cpp`, Target: Q2 2026) (Issue: #1708)
- [x] Prometheus metrics adapter (`include/core/concerns/prometheus_metrics_adapter.h`, Target: Q2 2026) (Issue: #1709)
- [x] Context propagation across async boundaries (Target: Q3 2026)

### Phase 3: Advanced Concerns & Runtime Flexibility (Status: In Progress 🚧)
- [x] Structured log correlation (trace ID + span ID injection into log records)
- [x] Health check interface in ConcernsContext
- [x] Structured log correlation (trace ID injection into log records)
- [x] Async context propagation (W3C TraceContext standard) (Issue: #1705)
- [I] Plugin-based adapter loading (no recompile needed) (Issue: #1706)
- [I] Feature flag interface for runtime enable/disable (Issue: #1707)
- [x] Secrets interface for credential injection into components

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1419)
- [x] Integration tests (DI context, adapter injection, factory methods)
- [P] Performance benchmarks (DI overhead, logging throughput) (Issue: #1420)
- [x] Security audit (no credential storage in context)
- [x] Documentation complete
- [x] API stability guaranteed for ConcernsContext and core interfaces

## Known Issues & Limitations
- Prometheus adapter not yet implemented; metrics are in-memory only
- Context propagation across async/thread boundaries is supported via `startSpanFromHeaders` / `injectContext`; caller is responsible for passing headers across async boundaries
- Feature flags are now a first-class concern in the DI system via `IFeatureFlags` / `InMemoryFeatureFlags`

## Breaking Changes
- New concern interfaces (health check, feature flags, secrets) will be additive
- Existing ILogger, ITracer, IMetrics, ICache interfaces are stable
