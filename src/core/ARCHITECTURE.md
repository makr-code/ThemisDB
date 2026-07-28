> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Core Module — Architecture Guide

**Version:** 1.1
**Last Updated:** 2026-07-28
**Module Path:** `src/core/`

---

## 1. Overview

The Core module provides ThemisDB's cross-cutting concerns infrastructure through a
Dependency Injection (DI) framework. It defines abstract interfaces for logging, distributed
tracing, metrics collection, and caching, and supplies production-ready adapters (spdlog,
OpenTelemetry) as well as no-op implementations for testing.

Every other module that needs to log, trace, or record metrics calls through the
`ConcernsContext` service container rather than depending directly on spdlog or OpenTelemetry.
This allows test code to inject silent/mock adapters without touching production code.

---

## 2. Design Principles

- **Interface → Adapter Pattern** – each concern (`ILogger`, `ITracer`, `IMetrics`,
  `ICache`) has a pure-virtual interface and one or more concrete adapters.
- **No-Op Defaults (non-production)** – no-op adapters are valid in test/dev mode;
  production mode rejects `createNoOp()` and no-op tracing/metrics.
- **Thread-Safe Runtime Replacement** – `ConcernsContext` supports runtime replacement
  for logger/tracer/metrics/cache/secrets/feature flags/audit sink with guarded swap semantics.
- **Environment Detection** – factory methods inspect environment variables to choose
  production vs. test adapters automatically.
- **Single Source of Truth** – all cross-cutting concern access flows through
  `ConcernsContext`; no direct spdlog or OTel calls in domain code.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `concerns/concerns_context.cpp` | Central DI hub: holds logger, tracer, metrics, cache, secrets, feature flags, and audit references |
| `concerns/spdlog_logger_adapter.h` | Production logger adapter |
| `concerns/context_propagation.cpp` | Distributed trace context propagation (W3C TraceContext) |
| `security_initialization.cpp` | OpenSSL and security library initialization at startup |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                    All ThemisDB Modules                         │
│   context->logger()->info(...)                                  │
│   context->tracer()->startSpan(...)                             │
│   context->metrics()->incrementCounter(...)                     │
│   context->cache()->set(...)                                    │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                    ConcernsContext (service container)            │
│                                                                  │
│  logger()  → ILogger*  → SpdlogLoggerAdapter | NoopLogger       │
│  tracer()  → ITracer*  → OtelTracerAdapter   | NoopTracer       │
│  metrics() → IMetrics* → PrometheusAdapter   | NoopMetrics      │
│  cache()   → ICache*   → RedisAdapter        | LocalCacheAdapter│
│  secrets() → ISecrets* → InMemory/Env/NoOp                      │
│  featureFlags() → IFeatureFlags* → InMemory/NoOp                │
│  auditLog() → IAuditLog* → NoOp/adapter                         │
│                                                                  │
│  create(config)  : factory for production                        │
│  createNoOp()    : factory with all Noop adapters               │
│  createCustom(logger, tracer, ...) : full custom injection      │
│  replace*()      : runtime adapter replacement                   │
└──────────────────────────────────────────────────────────────────┘
```

---

## 4. Data Flow

### 4.1 Module Logging Example

```
QueryEngine::execute(query)
    │
    ▼
context->logger()->info("Executing query: {}", query.id)
    │
    ▼
SpdlogLoggerAdapter → spdlog::info(...)  [production]
       or
NoopLogger → (nothing)                   [test mode]
```

### 4.2 Distributed Tracing Example

```
ApiHandler::handle(request)
    │
    ▼
context->tracer()->startSpan("api_request")  → Span
    │
    ▼
  (calls query engine...)
    │
    ▼
span.end()  → OtelTracerAdapter → OpenTelemetry exporter → Jaeger/Zipkin
```

### 4.3 Context Propagation

```
Inbound gRPC/HTTP request with W3C traceparent header
    │
    ▼
context_propagation.cpp: extract trace ID, span ID, flags
    │
    ▼
ConcernsContext: set active trace context for this request thread
    │
    ▼
All logger/tracer calls on this thread include the trace ID automatically
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Provides to** | All modules | `ConcernsContext` for logging, tracing, metrics, cache |
| **Uses** | spdlog library | Logging adapter |
| **Uses** | OpenTelemetry | Tracing adapter |
| **Uses** | Prometheus client | Metrics adapter |
| **Initializes** | OpenSSL | `security_initialization.cpp` at server startup |

---

## 6. Threading & Concurrency Model

- `ConcernsContext` is immutable after `create()`; all reads are lock-free.
- Adapters are responsible for their own thread safety (spdlog and OTel are thread-safe).
- `context_propagation.cpp` uses thread-local storage for the active trace context.
- `security_initialization.cpp` must be called once on the main thread before any TLS/crypto
  operations.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| No-op defaults | Logging/tracing disabled in test mode with zero overhead |
| Thread-local context | Trace context propagation uses TLS — no shared state |
| Lazy adapter init | Optional adapters (Redis cache) initialized on first use |

---

## 8. Security Considerations

- `security_initialization.cpp` enforces fail-closed bootstrap behavior in production mode
  (invalid provider/JWT configuration raises `std::runtime_error`).
- `ConcernsContext::create(config)` rejects invalid adapter/config combinations and
  enforces production constraints for tracing/metrics (no noop adapters in production).
- Runtime adapter replacement APIs reject `nullptr` and fail fast via
  `std::invalid_argument`.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `core.log_level` | "info" | spdlog log level (trace/debug/info/warn/error/critical) |
| `core.tracing.enabled` | false | Enable OpenTelemetry tracing |
| `core.tracing.exporter` | "otlp" | OTel exporter (otlp/jaeger/zipkin) |
| `core.tracing.endpoint` | "" | OTel collector endpoint |
| `core.metrics.enabled` | true | Enable Prometheus metrics |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| Invalid runtime configuration | Fail closed during `create(config)` with `std::runtime_error` |
| Production policy violation (noop tracing/metrics) | Fail closed with `std::runtime_error` |
| Runtime `replace*` with null adapter | Reject immediately with `std::invalid_argument` |

---

## 11. Known Limitations & Future Work

- Redis cache adapter (`RedisCache`) is implemented in `include/core/concerns/redis_cache.h` / `src/core/concerns/redis_cache.cpp` with consistent hashing, TTL, and pub/sub invalidation.
- OTel baggage propagation is partial; full W3C Baggage support is planned.
- Log sampling (high-frequency event suppression) is not yet implemented.
- Plugin-based adapter loading without recompilation is still open (Issue #1706), and adapter signing/trust hardening remains planned for Q4 2026.

---

## 12. References

- `src/core/README.md` — module overview
- `docs/architecture/CONCERNS_ARCHITECTURE_DIAGRAM.md` — visual architecture diagram
- `docs/architecture/CONCERNS_IMPLEMENTATION_SUMMARY.md` — implementation history
- `docs/architecture/MIGRATION_GUIDE_CONCERNS.md` — migration guide
- `ARCHITECTURE.md` (root) — full system architecture
