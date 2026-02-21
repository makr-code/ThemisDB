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

## In Progress 🚧
- [I] Context propagation across async boundaries (Target: Q3 2026) (Issue: #1406)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] OpenTelemetry trace and span propagation (Issue: #1407)
- [I] Prometheus-compatible metrics adapter (Issue: #1408)
- [I] Structured log correlation (trace ID injection into log records) (Issue: #1409) — **completed**
- [I] Health check interface in ConcernsContext (Issue: #1410)
- [I] Configuration-driven adapter selection (no code changes needed) (Issue: #1411)
- [I] Dynamic log level adjustment at runtime (Issue: #1412)

### Long-term (6-12 months)
- [I] Jaeger/Zipkin tracing backend adapters (Issue: #1413)
- [I] Distributed context propagation (W3C TraceContext standard) (Issue: #1414)
- [I] Circuit breaker interface as a first-class concern (Issue: #1415)
- [I] Feature flag interface (enable/disable features without redeployment) (Issue: #1416)
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

### Phase 2: Observability Adapters (Status: Complete ✅)
- [x] OpenTelemetry tracer adapter (`core/concerns/otel_tracer_adapter.h`, circuit-breaker guarded)
- [x] Prometheus metrics adapter (`core/concerns/prometheus_metrics_adapter.h`, all IMetrics methods implemented)
- [ ] Context propagation across async boundaries (Target: Q3 2026)

### Phase 3: Advanced Concerns & Runtime Flexibility (Status: In Progress 🚧)
- [x] Structured log correlation (trace ID + span ID injection into log records)
- [ ] Health check interface in ConcernsContext
- [ ] Async context propagation (W3C TraceContext standard)
- [ ] Plugin-based adapter loading (no recompile needed)
- [ ] Feature flag interface for runtime enable/disable
- [ ] Secrets interface for credential injection into components

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1419)
- [x] Integration tests (DI context, adapter injection, factory methods)
- [I] Performance benchmarks (DI overhead, logging throughput) (Issue: #1420)
- [x] Security audit (no credential storage in context)
- [x] Documentation complete
- [x] API stability guaranteed for ConcernsContext and core interfaces

## Known Issues & Limitations
- Context propagation across async/thread boundaries requires manual passing
- Feature flags are not yet a first-class concern in the DI system

## Breaking Changes
- New concern interfaces (health check, feature flags, secrets) will be additive
- Existing ILogger, ITracer, IMetrics, ICache interfaces are stable
