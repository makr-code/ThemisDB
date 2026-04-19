<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · AUDIT.md · SECURITY.md -->

# Core Module — Public Header Architecture

**Version:** 1.8.0
**Last Updated:** 2026-04-06
**Module Path:** `include/core/`
**Implementation:** `../../src/core/`

---

## 1. Overview

The `include/core/` directory exposes public C++ headers for ThemisDB's foundational
cross-cutting concerns. These headers define the configuration validation contract,
index and storage initialisation interfaces, production-mode enforcement, and the
query engine builder. The `concerns/` subdirectory provides abstract interfaces for
logging, metrics, secrets, tracing, caching, circuit breaking, and feature flags —
the foundational dependency injection interfaces used throughout the entire codebase.

---

## 2. Design Principles

- **Dependency Injection via Concerns** – `concerns/concerns_context.h` is the single
  injection point for all cross-cutting concerns; no module directly constructs loggers,
  metrics, or secrets — it receives them via `ConcernsContext`.
- **Configuration Validation at Startup** – `config_validator.h` must be consulted during
  startup; invalid configurations fail fast with a structured error.
- **Production Mode Enforcement** – `production_mode.h` provides a compile/runtime gate
  that disables debug paths and enforces security defaults in production builds.
- **Interface Segregation** – Each concern (`ILogger`, `IMetrics`, `ISecrets`, `ITracer`,
  `ICache`, `ICircuitBreaker`, `IFeatureFlags`) has its own header; consumers depend only
  on the interfaces they need.
- **Zero-Copy Observability** – `concerns/zero_copy_logger.h` and `concerns/lockfree_metrics.h`
  provide high-performance observability without allocation on the hot path.

---

## 3. Interface Inventory

### Core Headers

| Header | Classes / Interfaces | Purpose |
|--------|----------------------|---------|
| `config_validator.h` | `IConfigValidator`, `ConfigValidationResult` | Startup configuration validation |
| `index_initialization.h` | `IIndexInitializer`, `IndexInitConfig` | Index subsystem initialisation |
| `storage_initialization.h` | `IStorageInitializer`, `StorageInitConfig` | Storage subsystem initialisation |
| `security_initialization.h` | `ISecurityInitializer` | Security subsystem initialisation |
| `production_mode.h` | `ProductionMode`, `isProductionBuild()` | Production mode gate |
| `query_engine_builder.h` | `QueryEngineBuilder`, `QueryEngineConfig` | Query engine assembly |

### concerns/ Headers

| Header | Classes / Interfaces | Purpose |
|--------|----------------------|---------|
| `concerns_context.h` | `ConcernsContext` | Unified DI context for all concerns |
| `i_logger.h` | `ILogger`, `LogLevel` | Structured logging interface |
| `i_async_logger.h` | `IAsyncLogger` | Async logging interface |
| `i_audit_log.h` | `IAuditLog`, `AuditEntry` | Audit log interface |
| `i_metrics.h` | `IMetrics`, `MetricHandle` | Metrics emission interface |
| `i_secrets.h` | `ISecrets` | Secrets retrieval interface |
| `i_tracer.h` | `ITracer`, `ISpan` | Distributed tracing interface |
| `i_context.h` | `IContext`, `ContextKey` | Request context propagation |
| `context_propagation.h` | `IContextPropagator` | W3C trace context propagation |
| `i_cache.h` | `ICache<K,V>` | Generic cache interface |
| `i_async_cache.h` | `IAsyncCache<K,V>` | Async cache interface |
| `i_circuit_breaker.h` | `ICircuitBreaker` | Circuit breaker interface |
| `i_feature_flags.h` | `IFeatureFlags` | Feature flag evaluation interface |
| `inmemory_secrets.h` | `InMemorySecrets` | In-memory secrets provider |
| `inmemory_cache_impl.h` | `InMemoryCacheImpl<K,V>` | In-memory cache implementation |
| `cache_strategies.h` | `CacheStrategy` enum | Cache strategy types |
| `eviction_strategies.h` | `IEvictionStrategy` | Eviction strategy interface |
| `strategic_cache_impl.h` | `StrategicCacheImpl<K,V>` | Strategy-pattern cache |
| `redis_cache.h` | `RedisCache<K,V>` | Redis-backed cache |
| `lockfree_metrics.h` | `LockfreeMetrics` | Lock-free metrics for hot paths |
| `metric_labels.h` | `MetricLabels` | Label key-value pairs for metrics |
| `zero_copy_logger.h` | `ZeroCopyLogger` | Zero-allocation logger |
| `lifecycle.h` | `ILifecycle`, `LifecycleState` | Component lifecycle management |
| `noop_implementations.h` | `NoopLogger`, `NoopMetrics`, `NoopTracer` | No-op implementations for testing |
| `spdlog_logger_adapter.h` | `SpdlogLoggerAdapter` | spdlog adapter for `ILogger` |
| `jaeger_tracer_adapter.h` | `JaegerTracerAdapter` | Jaeger adapter for `ITracer` |
| `otel_tracer_adapter.h` | `OtelTracerAdapter` | OpenTelemetry adapter for `ITracer` |
| `zipkin_tracer_adapter.h` | `ZipkinTracerAdapter` | Zipkin adapter for `ITracer` |
| `prometheus_metrics_adapter.h` | `PrometheusMetricsAdapter` | Prometheus adapter for `IMetrics` |
| `w3c_trace_context_propagator.h` | `W3CTraceContextPropagator` | W3C trace context header propagation |

> **Implementation details:** `../../src/core/`
