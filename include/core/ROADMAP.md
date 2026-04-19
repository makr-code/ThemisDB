<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · AUDIT.md · SECURITY.md -->

# Roadmap — Core Module Public Headers

**Module Path:** `include/core/`
**Implementation Roadmap:** `../../src/core/ROADMAP.md`

---

## Current Status

Public headers at v1.8.0. All foundational concerns (logging, metrics, secrets, tracing,
caching, circuit breaking, feature flags, lifecycle) have stable abstract interfaces.
Six observability adapters (spdlog, Jaeger, OTel, Zipkin, Prometheus, W3C propagator)
are present. Configuration validation, initialisation, and production mode gate headers
are complete.

---

## Completed Features

- [x] `IConfigValidator` for startup configuration validation
- [x] `IIndexInitializer` and `IStorageInitializer` for subsystem bootstrap
- [x] `ISecurityInitializer` for security subsystem bootstrap
- [x] `ProductionMode` gate for production vs. debug mode
- [x] `QueryEngineBuilder` with feature flags injection
- [x] `ConcernsContext` as unified DI injection point
- [x] `ILogger`, `IAsyncLogger`, `IAuditLog` logging interfaces
- [x] `ZeroCopyLogger` for zero-allocation structured logging
- [x] `IMetrics`, `LockfreeMetrics` for lock-free metric updates
- [x] `ISecrets` with `watchSecret()` for hot rotation
- [x] `InMemorySecrets` provider
- [x] `ITracer` and `ISpan` for distributed tracing
- [x] `IContext` and `IContextPropagator` for request context
- [x] `W3CTraceContextPropagator`
- [x] `ICache<K,V>`, `IAsyncCache<K,V>`, `InMemoryCacheImpl`, `RedisCache`, `StrategicCacheImpl`
- [x] `ICircuitBreaker` for failure isolation
- [x] `IFeatureFlags` for runtime feature toggling
- [x] `ILifecycle` for component lifecycle management
- [x] All 4 observability adapters (spdlog, Jaeger, OTel, Zipkin, Prometheus)
- [x] `NoopLogger`, `NoopMetrics`, `NoopTracer` for testing

---

## Planned Features

- [x] `IHealthProbe` interface for liveness/readiness checking (Target: Q3 2026)
- [x] `IConfigHotReloader` for runtime config hot-reload (Target: Q3 2026)
- [ ] `IDistributedLock` interface for cross-node coordination (Target: Q4 2026)

---

## Implementation Phases

### Phase 1: Core Concern Interfaces
- [x] `ILogger`, `IMetrics`, `ISecrets`, `ITracer`, `IContext`

### Phase 2: Cache and Resilience
- [x] `ICache`, `IAsyncCache`, `ICircuitBreaker`, `IFeatureFlags`

### Phase 3: Startup & Production Mode
- [x] `IConfigValidator`, init headers, `ProductionMode`

### Phase 4: Observability Adapters
- [x] spdlog, Jaeger, OTel, Zipkin, Prometheus, W3C adapters

### Phase 5: Performance Headers
- [x] `ZeroCopyLogger`, `LockfreeMetrics`, `MetricLabels`

### Phase 6: Future Concerns
- [x] `IHealthProbe` (Q3 2026)
- [x] `IConfigHotReloader` (Q3 2026)

---

## Production Readiness Checklist

- [x] All concern interfaces have no-op test implementations
- [x] `ConcernsContext` provides single injection point
- [x] `ProductionMode` gate available
- [x] Secrets hot-rotation via `watchSecret()` available
- [x] Lock-free metrics for hot path
- [x] `IHealthProbe` interface published
- [x] `IConfigHotReloader` interface published
