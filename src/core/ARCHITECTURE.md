# Core Module — Architecture Guide

**Version:** 1.0
**Last Updated:** 2026-04-06
**Module Path:** `src/core/`

---

## 1. Overview

The Core module provides ThemisDB's cross-cutting concerns infrastructure through a
Dependency Injection (DI) framework. It defines abstract interfaces for logging, distributed
tracing, metrics collection, and caching, and supplies production-ready adapters (spdlog,
OpenTelemetry) as well as no-op implementations for testing.

Every other module that needs to log, trace, or record metrics calls through the
`ConcernsContext` singleton rather than depending directly on spdlog or OpenTelemetry.
This allows test code to inject silent/mock adapters without touching production code.

---

## 2. Design Principles

- **Interface → Adapter Pattern** – each concern (`ILogger`, `ITracer`, `IMetrics`,
  `ICache`) has a pure-virtual interface and one or more concrete adapters.
- **No-Op Defaults** – uninitialized context components fall back to no-op implementations,
  preventing null-pointer crashes in early startup or tests.
- **Immutable After Creation** – `ConcernsContext` is thread-safe after `create()`; no
  locking is needed on the read path.
- **Environment Detection** – factory methods inspect environment variables to choose
  production vs. test adapters automatically.
- **Single Source of Truth** – all cross-cutting concern access flows through
  `ConcernsContext`; no direct spdlog or OTel calls in domain code.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `concerns/concerns_context.cpp` | Central DI hub: holds logger, tracer, metrics, cache references |
| `concerns/i_logger.cpp` | ILogger interface + SpdlogLoggerAdapter + NoopLogger |
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
│                    ConcernsContext (singleton)                   │
│                                                                  │
│  logger()  → ILogger*  → SpdlogLoggerAdapter | NoopLogger       │
│  tracer()  → ITracer*  → OtelTracerAdapter   | NoopTracer       │
│  metrics() → IMetrics* → PrometheusAdapter   | NoopMetrics      │
│  cache()   → ICache*   → RedisAdapter        | LocalCacheAdapter│
│                                                                  │
│  create(config)  : factory for production                        │
│  createForTest() : factory with all Noop adapters               │
│  createCustom(logger, tracer, ...) : full custom injection      │
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

- `security_initialization.cpp` initializes OpenSSL with FIPS-compliant settings when
  `THEMIS_FIPS_MODE=1` is set.
- TLS private keys are locked to memory (mlock) to prevent swapping to disk.
- The `ILogger` interface sanitizes format strings to prevent format-string injection
  in spdlog calls.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `core.log_level` | "info" | spdlog log level (trace/debug/info/warn/error/critical) |
| `core.tracing.enabled` | false | Enable OpenTelemetry tracing |
| `core.tracing.exporter` | "otlp" | OTel exporter (otlp/jaeger/zipkin) |
| `core.tracing.endpoint` | "" | OTel collector endpoint |
| `core.metrics.enabled` | true | Enable Prometheus metrics |
| `core.fips_mode` | false | Enable FIPS-compliant crypto |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| Adapter initialization failure | Fall back to Noop adapter; log warning |
| OTel exporter unreachable | Buffer spans locally; retry with backoff; drop on overflow |
| Logger write failure | Silently drop (logging must not throw) |

---

## 11. Known Limitations & Future Work

- Redis cache adapter is planned; currently only local in-memory cache is implemented.
- OTel baggage propagation is partial; full W3C Baggage support is planned.
- Log sampling (high-frequency event suppression) is not yet implemented.

---

## 12. References

- `src/core/README.md` — module overview
- `docs/architecture/CONCERNS_ARCHITECTURE_DIAGRAM.md` — visual architecture diagram
- `docs/architecture/CONCERNS_IMPLEMENTATION_SUMMARY.md` — implementation history
- `docs/architecture/MIGRATION_GUIDE_CONCERNS.md` — migration guide
- `ARCHITECTURE.md` (root) — full system architecture
