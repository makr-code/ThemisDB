# Core Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
**Production Ready** — Dependency injection, cross-cutting concerns management (logging, tracing, metrics, caching, secrets, feature flags, audit log), and pluggable adapter infrastructure are functional. OpenTelemetry and Prometheus adapters are implemented.

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
- [x] Jaeger tracing backend adapter — `JaegerTracerAdapter` with `uber-trace-id` propagation and W3C `traceparent` fallback; circuit-breaker guarded; selectable via `tracerAdapter="jaeger"` in `ConcernsContext::Config` (Issue: #1413)
- [x] Zipkin tracing backend adapter — `ZipkinTracerAdapter` with B3 single/multi-header and W3C `traceparent` propagation; injects all three header formats on outbound; circuit-breaker guarded; selectable via `tracerAdapter="zipkin"` (Issue: #1413)
- [x] Distributed context propagation (W3C TraceContext standard) — `W3CTraceContextPropagator` extracts `traceparent`/`tracestate` headers into `IContext` (populates `kTraceId`, `kSpanId`) and injects them for outbound calls; `kSpanId` added to `context_keys`; `SimpleContext::toTraceContext()` now includes span_id (Issue: #1414)
- [x] Zero-Copy Logging — `ZeroCopyLogger` in `include/core/concerns/zero_copy_logger.h` + `src/core/concerns/zero_copy_logger.cpp`; `string_view` hot-path API (`logSV`/`infoSV`/…/`logStructuredSV`); pre-allocated thread-local format buffer; early `shouldLog()` level-check; PII redaction; `json_mode_` field is `std::atomic<bool>` for safe concurrent `setJsonMode()` + logging; 41 focused tests in `tests/test_zero_copy_logging.cpp` (Issue: #65)
- [x] Secrets interface for credential injection — `ISecrets` interface; `InMemorySecrets` (map-backed, thread-safe, `setSecret`/`removeSecret`); `EnvSecretsProvider` (env-var backed with configurable prefix); config-driven selection via `Config::secretsAdapter` (`"noop"`, `"inmemory"`, `"env"`); `ConfigValidator` extended; 30+ tests (Issue: #1417)

## In Progress 🚧
*(none currently in progress)*

## Planned Features 📋

### Short-term (Next 3-6 months)
- [x] Dynamic log level adjustment at runtime (Issue: #1412)
- [x] Distributed Cache Integration — Redis-backed ICache with consistent hashing, TTL, and pub/sub invalidation (Issue: #64, Target: v1.6.0)

### Long-term (6-12 months)
- [x] Secrets interface for credential injection into components (Issue: #1417)
- [x] Audit event interface for compliance logging (Issue: #1418)
- [x] `IHealthProbe` interface for liveness/readiness checking — `HealthProbeRegistry`, `FunctionalHealthProbe` (Target: Q3 2026)
- [x] `IConfigHotReloader` for runtime config hot-reload — `InMemoryConfigHotReloader`, `IConfigChangeListener` (Target: Q3 2026)
- [x] `IDistributedLock` interface for cross-node coordination — `InMemoryDistributedLock`, `DistributedLockGuard` (Target: Q4 2026)

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

### Phase 3: Advanced Concerns & Runtime Flexibility (Status: Completed ✅)
- [x] Structured log correlation (trace ID + span ID injection into log records)
- [x] Health check interface in ConcernsContext
- [x] Structured log correlation (trace ID injection into log records)
- [x] Async context propagation (W3C TraceContext standard) (Issue: #1705)
- [I] Plugin-based adapter loading (no recompile needed) (Issue: #1706)
- [x] Feature flag interface for runtime enable/disable (Issue: #1707)
- [x] Secrets interface for credential injection into components
- [x] Dynamic log level adjustment at runtime (Issue: #1412)
- [x] Audit event interface for compliance logging (Issue: #1418)
- [x] Zero-Copy Logging — `ZeroCopyLogger` with `string_view` hot-path API, pre-allocated thread-local buffer, and `std::atomic<bool>` `json_mode_` for safe concurrent mode changes (Issue: #65)
- [x] `IHealthProbe` + `HealthProbeRegistry` — named liveness/readiness probe registry wired into `ConcernsContext::healthProbes()`
- [x] `IConfigHotReloader` + `InMemoryConfigHotReloader` — runtime config hot-reload with async `IConfigChangeListener` callbacks
- [x] `IDistributedLock` + `InMemoryDistributedLock` + `DistributedLockGuard` — cross-node coordination lock with RAII guard

## Production Readiness Checklist
- [x] Unit tests coverage > 80% (Issue: #1419) — test_concerns_context.cpp (146 tests), test_fuzz_core.cpp; standalone targets ConcernsContextFocusedTests + FuzzCoreFocusedTests added
- [x] Integration tests (DI context, adapter injection, factory methods)
- [x] Performance benchmarks (DI overhead, logging throughput) (Issue: #1420) — benchmarks/bench_di_logging.cpp registered in benchmarks/CMakeLists.txt
- [x] Security audit (no credential storage in context)
- [x] Documentation complete
- [x] API stability guaranteed for ConcernsContext and core interfaces
- [x] Distributed cache adapter (RedisCache) — cluster-wide invalidation, consistent hashing, TTL, pub/sub (Issue: #64)

## Known Issues & Limitations
- Context propagation across async/thread boundaries is supported via `startSpanFromHeaders` / `injectContext`; caller is responsible for passing headers across async boundaries
- Feature flags are now a first-class concern in the DI system via `IFeatureFlags` / `InMemoryFeatureFlags`

## Breaking Changes
- New concern interfaces (health check, feature flags, secrets) will be additive
- Existing ILogger, ITracer, IMetrics, ICache interfaces are stable
