# Core Module Roadmap

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
- [I] OpenTelemetry tracer adapter (Target: Q2 2026) (Issue: #1404)
- [I] Prometheus metrics adapter (Target: Q2 2026) (Issue: #1405)
- [I] Context propagation across async boundaries (Target: Q3 2026) (Issue: #1406)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] OpenTelemetry trace and span propagation (Issue: #1407)
- [I] Prometheus-compatible metrics adapter (Issue: #1408)
- [I] Structured log correlation (trace ID injection into log records) (Issue: #1409)
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

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1419)
- [x] Integration tests (DI context, adapter injection, factory methods)
- [I] Performance benchmarks (DI overhead, logging throughput) (Issue: #1420)
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
