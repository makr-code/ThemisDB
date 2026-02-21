# Core Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
**Beta** — Dependency injection, cross-cutting concerns management (logging, tracing, metrics, caching), and pluggable adapter infrastructure are functional. OpenTelemetry and Prometheus adapters are in progress.

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

## In Progress 🚧
- [ ] OpenTelemetry tracer adapter (Target: Q2 2026)
- [ ] Prometheus metrics adapter (Target: Q2 2026)
- [ ] Context propagation across async boundaries (Target: Q3 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] OpenTelemetry trace and span propagation
- [ ] Prometheus-compatible metrics adapter
- [ ] Structured log correlation (trace ID injection into log records)
- [ ] Health check interface in ConcernsContext
- [ ] Configuration-driven adapter selection (no code changes needed)
- [ ] Dynamic log level adjustment at runtime

### Long-term (6-12 months)
- [ ] Jaeger/Zipkin tracing backend adapters
- [ ] Distributed context propagation (W3C TraceContext standard)
- [ ] Circuit breaker interface as a first-class concern
- [ ] Feature flag interface (enable/disable features without redeployment)
- [ ] Secrets interface for credential injection into components
- [ ] Audit event interface for compliance logging

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

### Phase 2: Observability Adapters (Status: In Progress 🚧)
- [~] OpenTelemetry tracer adapter (`core/adapters/otel_tracer.cpp`, Target: Q2 2026)
- [~] Prometheus metrics adapter (`core/adapters/prometheus_metrics.cpp`, Target: Q2 2026)
- [ ] Context propagation across async boundaries (Target: Q3 2026)

### Phase 3: Advanced Concerns & Runtime Flexibility (Status: Planned 📋)
- [ ] Health check interface in ConcernsContext
- [ ] Structured log correlation (trace ID injection into log records)
- [ ] Async context propagation (W3C TraceContext standard)
- [ ] Plugin-based adapter loading (no recompile needed)
- [ ] Feature flag interface for runtime enable/disable
- [ ] Secrets interface for credential injection into components

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [x] Integration tests (DI context, adapter injection, factory methods)
- [ ] Performance benchmarks (DI overhead, logging throughput)
- [x] Security audit (no credential storage in context)
- [x] Documentation complete
- [x] API stability guaranteed for ConcernsContext and core interfaces

## Known Issues & Limitations
- OpenTelemetry adapter not yet implemented; tracing is a no-op by default
- Prometheus adapter not yet implemented; metrics are in-memory only
- Context propagation across async/thread boundaries requires manual passing
- Feature flags are not yet a first-class concern in the DI system

## Breaking Changes
- New concern interfaces (health check, feature flags, secrets) will be additive
- Existing ILogger, ITracer, IMetrics, ICache interfaces are stable
