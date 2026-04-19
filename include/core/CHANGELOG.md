<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Core Module Public Headers

All notable changes to public headers in `include/core/`.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.8.0] — 2026-03-22
### Added
- `concerns/i_async_logger.h`: `IAsyncLogger` for non-blocking async log emission
- `concerns/zero_copy_logger.h`: `ZeroCopyLogger` for zero-allocation structured logging
- `concerns/lockfree_metrics.h`: `LockfreeMetrics` for atomic, lock-free metric updates
- `concerns/metric_labels.h`: `MetricLabels` for typed label key-value pairs
- `concerns/w3c_trace_context_propagator.h`: `W3CTraceContextPropagator` for W3C traceparent propagation
- `concerns/otel_tracer_adapter.h`: `OtelTracerAdapter` for OpenTelemetry SDK adapter
- `security_initialization.h`: `ISecurityInitializer` for security subsystem bootstrap

### Changed
- `concerns/concerns_context.h`: `ConcernsContext` now includes `IAsyncLogger` and `IFeatureFlags` injection
- `concerns/i_secrets.h`: `ISecrets` extended with `watchSecret()` for hot-rotation callbacks
- `query_engine_builder.h`: `QueryEngineConfig` extended with `feature_flags` injection point

## [1.7.0] — 2026-03-09
### Added
- `concerns/i_circuit_breaker.h`: `ICircuitBreaker` with `open()`, `close()`, `halfOpen()` states
- `concerns/i_feature_flags.h`: `IFeatureFlags` and `FlagKey` for runtime feature toggling
- `concerns/redis_cache.h`: `RedisCache<K,V>` as Redis-backed concerns cache
- `concerns/strategic_cache_impl.h`: `StrategicCacheImpl` with swappable eviction strategies
- `concerns/lifecycle.h`: `ILifecycle` and `LifecycleState` for component lifecycle management
- `concerns/noop_implementations.h`: `NoopLogger`, `NoopMetrics`, `NoopTracer` for test doubles

## [1.6.0] — 2026-02-01
### Added
- Initial core headers: `config_validator.h`, `index_initialization.h`, `storage_initialization.h`,
  `production_mode.h`, `query_engine_builder.h`
- `concerns/i_logger.h`, `concerns/i_metrics.h`, `concerns/i_secrets.h`, `concerns/i_tracer.h`
- `concerns/i_context.h`, `concerns/context_propagation.h`
- `concerns/i_cache.h`, `concerns/i_async_cache.h`
- `concerns/concerns_context.h`, `concerns/inmemory_secrets.h`, `concerns/inmemory_cache_impl.h`
- `concerns/cache_strategies.h`, `concerns/eviction_strategies.h`
- `concerns/i_audit_log.h`
- `concerns/spdlog_logger_adapter.h`, `concerns/jaeger_tracer_adapter.h`,
  `concerns/zipkin_tracer_adapter.h`, `concerns/prometheus_metrics_adapter.h`
