> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

# ThemisDB Core Headers

## Module Purpose

This directory contains the header files defining the core interfaces and abstractions for ThemisDB's cross-cutting concerns infrastructure. These headers establish the contracts for dependency injection, observability, and caching throughout the database engine.

## Scope

**In Scope:**
- Abstract interfaces for cross-cutting concerns (ILogger, ITracer, IMetrics, ICache)
- ConcernsContext API for dependency injection
- Adapter specifications for external frameworks
- Configuration structures and builders
- Initialization and builder interfaces for core subsystems

**Out of Scope:**
- Implementation details (see src/core/)
- Storage layer interfaces (see include/storage/)
- Server protocol definitions (see include/server/)
- Query execution interfaces (see include/query/)

## Key Components

### Public Header Entry Points

| Header | Purpose |
|---|---|
| `concerns/concerns_context.h` | Main DI entry point (`create`, `createNoOp`, `createCustom`, runtime `replace*`) |
| `config_validator.h` | Validation helpers for runtime config (adapters, cache, log/tracing settings) |
| `production_mode.h` | Environment-based production mode detection used for fail-closed checks |
| `health_probe.h` | Probe result/status types used by concern health checks |
| `security_initialization.h` | Security subsystem bootstrap contracts for startup wiring |
| `storage_initialization.h` / `index_initialization.h` / `query_engine_builder.h` | Core wiring helpers for major subsystems |

### Concerns Subdirectory

The `concerns/` subdirectory contains all cross-cutting concern interfaces and adapters.

#### Core Interfaces

**ILogger** (`i_logger.h`)
- Level-based logging via `log(level, message)` and severity helpers.
- Structured fields via `logStructured(...)` and trace correlation via `logWithContext(...)`.
- Runtime configuration (`setLevel`, `setPattern`) and lifecycle hooks (`flush`, `shutdown`, `isHealthy`).

**ITracer** (`i_tracer.h`)
- RAII span API via nested `ISpan` interface.
- Root/child span creation (`startSpan`, `startChildSpan`) plus HTTP propagation helpers (`startSpanFromHeaders`, `injectContext`).
- Explicit tracer lifecycle (`initialize`, `shutdown`, `isInitialized`, `flush`, `isHealthy`).

**IMetrics** (`i_metrics.h`)
- Counter, gauge, and histogram operations.
- High-level helpers (`recordLatency`, `recordError`, `recordSuccess`).
- Export/reset plus lifecycle and health hooks.

**ICache** (`i_cache.h`)
- Cache values are `CacheEntry` payload/version/timestamp records.
- Core operations (`get`, `put`, `invalidate`, `clear`, `invalidatePattern`).
- Statistics/configuration and optional extension points (`getEvictionStrategy`, `getMetrics`).

#### Adapter Implementations

**SpdlogLoggerAdapter** (`spdlog_logger_adapter.h`)
- Wraps spdlog library for production logging
- Supports async logging mode
- Configurable log levels and patterns

**OtelTracerAdapter** (`otel_tracer_adapter.h`)
- Integrates OpenTelemetry for distributed tracing
- Supports W3C Trace Context propagation
- Compatible with Jaeger, Zipkin, and other OTEL backends

**PrometheusMetricsAdapter** (`prometheus_metrics_adapter.h`)
- Exposes metrics in Prometheus format
- Supports counters, histograms, gauges, and summaries
- Label support for high-dimensional metrics

**NoOpImplementations** (`noop_implementations.h`)
- Zero-overhead no-op implementations for testing and minimal builds
- All virtual calls optimized away by compiler
- Useful for performance-critical paths

#### Secrets Implementations

**InMemorySecrets** (`inmemory_secrets.h`)
- Thread-safe map-backed `ISecrets` implementation
- Pre-populated at construction via `std::map<std::string, std::string>`
- `setSecret(name, value)` / `removeSecret(name)` for runtime updates
- `listSecretNames()` returns sorted names
- Use for unit tests and config-file-based credential injection

**EnvSecretsProvider** (`inmemory_secrets.h`)
- Reads credentials from environment variables
- Configurable prefix (default: `THEMIS_SECRET_`)
- Secret name mapped to env-var key: upper-cased, dots/dashes → underscores
- `registerName()` for selective `listSecretNames()` enumeration
- Use in single-process deployments where secrets are injected via the OS environment

#### Cache Implementations

**InMemoryCacheImpl** (`inmemory_cache_impl.h`)
- Thread-safe in-memory cache
- Configurable max size with automatic eviction
- FIFO eviction on insert when capacity is exceeded

**StrategicCacheImpl** (`strategic_cache_impl.h`)
- Pluggable eviction strategies via Strategy pattern
- Supports LRU, LIRS, ARC, 2Q, MRU, and more
- See `CACHE_STRATEGIES_README.md` for details

#### Cache Strategies

**CacheStrategies** (`cache_strategies.h`)
- `IEvictionStrategy` interface with metadata callbacks (`onAccess`, `onInsert`, `onRemove`) and victim selection (`selectVictim`).
- `CacheMetrics` helper struct with `hitRate()` and `avgLatencyNs()` utilities.

**EvictionStrategies** (`eviction_strategies.h`)
- Concrete implementations for `LRU`, `LFU`, `TTL`, `TwoTier`, and `ARC`
- Pluggable via factory pattern
- Performance characteristics documented per strategy

### ConcernsContext API

**ConcernsContext** (`concerns_context.h`)

Central dependency injection container providing access to all concerns.

```cpp
class ConcernsContext {
public:
    // Factory methods
    static std::shared_ptr<ConcernsContext> create();
    static std::shared_ptr<ConcernsContext> create(const Config& config);
    static std::shared_ptr<ConcernsContext> createCustom(
        std::unique_ptr<ILogger> logger,
        std::unique_ptr<ITracer> tracer,
        std::unique_ptr<IMetrics> metrics,
        std::unique_ptr<ICache> cache
    );
    static std::shared_ptr<ConcernsContext> createNoOp();

    // Accessors
    ILogger& logger();
    ITracer& tracer();
    IMetrics& metrics();
    ICache& cache();

    // Thread-safe and immutable after construction
};
```

#### Configuration-Driven Adapter Selection

`ConcernsContext::Config` lets you select the concrete adapter for each
concern via string fields — no code changes required:

| Field | Default | Supported values |
|---|---|---|
| `loggerAdapter` | `"spdlog"` | `"spdlog"`, `"noop"` |
| `tracerAdapter` | `""` (auto) | `"otel"`, `"jaeger"`, `"zipkin"`, `"noop"`, `""` |
| `metricsAdapter` | `""` (auto) | `"prometheus"`, `"noop"`, `""` |
| `cacheAdapter` | `"inmemory"` | `"inmemory"`, `"redis"`, `"noop"` |
| `circuitBreakerAdapter` | `"default"` | `"default"`, `"noop"` |
| `featureFlagsAdapter` | `"inmemory"` | `"inmemory"`, `"noop"` |
| `auditAdapter` | `"noop"` | `"noop"`, `"inmemory"` |
| `secretsAdapter` | `"noop"` | `"noop"`, `"inmemory"`, `"env"` |

Auto-selection: empty `tracerAdapter` resolves to `"otel"` when
`tracingEnabled=true`, otherwise `"noop"`. Same rule for `metricsAdapter`.
An explicit non-empty adapter value always overrides the boolean flag.

**Secrets adapter behaviour:**
- `"noop"` — NoOpSecrets; always returns `nullopt` (default, minimal builds).
- `"inmemory"` — InMemorySecrets backed by `Config::initialSecrets`.
- `"env"` — EnvSecretsProvider using `Config::secretsEnvPrefix` (default: `"THEMIS_SECRET_"`).

### Initialization Headers

**storage_initialization.h**
- Storage layer initialization routines
- RocksDB configuration and setup
- Blob storage backend initialization

**index_initialization.h**
- Index subsystem initialization
- HNSW, graph, and spatial index setup
- GPU acceleration configuration

**security_initialization.h**
- Cryptographic component initialization
- Key management setup
- Field-level encryption configuration

**query_engine_builder.h**
- Query engine construction with dependency injection
- Parser, optimizer, and executor configuration
- Function registry initialization

## Architecture

### Dependency Injection Pattern

All core headers follow the Dependency Inversion Principle:

```
┌─────────────────────────┐
│   High-Level Modules    │
│  (Storage, Query, etc.) │
└───────────┬─────────────┘
            │ depends on
            ↓
┌─────────────────────────┐
│  Abstract Interfaces    │
│ (ILogger, ITracer, etc.)│
└───────────┬─────────────┘
            ↑ implemented by
            │
┌─────────────────────────┐
│   Concrete Adapters     │
│(Spdlog, OTEL, Prometheus)│
└─────────────────────────┘
```

**Benefits:**
- High-level modules don't depend on low-level implementations
- Easy to swap implementations without modifying consumers
- Testable with mock implementations
- No compile-time dependencies on external frameworks

### Thread Safety Guarantees

All interfaces are designed for thread-safe usage:

- **ILogger**: Thread-safe (adapters use internal locking)
- **ITracer**: Thread-safe span creation and attribute setting
- **IMetrics**: Thread-safe metric updates (atomic operations)
- **ICache**: Thread-safe get/set/invalidate (implementation-dependent)
- **ConcernsContext**: Immutable after construction (thread-safe access)

### Memory Management

- `ConcernsContext` owns adapters via `std::unique_ptr` internally
- Consumers access concerns through references (`logger()`, `tracer()`, etc.)
- `std::shared_ptr<ConcernsContext>` is used to share one immutable context instance
- Adapter replacement drains old adapters (`flush`/`shutdown`) before release

## Integration Points

### With Storage Module

Storage headers include core headers for dependency injection:

```cpp
#include "core/concerns/concerns_context.h"

class StorageEngine {
    StorageEngine(/* ... */, std::shared_ptr<ConcernsContext> concerns);
    // Uses concerns->logger(), concerns->tracer(), concerns->metrics()
};
```

### With Server Module

Server handlers receive ConcernsContext for observability:

```cpp
#include "core/concerns/concerns_context.h"

class APIHandler {
    APIHandler(std::shared_ptr<ConcernsContext> concerns);
    // Logs requests, traces execution, records metrics
};
```

### With Query Module

Query engine uses all concerns for execution monitoring:

```cpp
#include "core/concerns/concerns_context.h"

class QueryEngine {
    QueryEngine(/* ... */, std::shared_ptr<ConcernsContext> concerns);
    // Traces query parsing, caches plans, logs execution
};
```

## API/Usage Examples

### Creating a Production Context

```cpp
#include "core/concerns/concerns_context.h"

int main() {
    ConcernsContext::Config config;
    config.logLevel          = "info";
    config.tracingEnabled    = true;
    config.tracingEndpoint   = "http://otel-collector:4318";
    config.metricsEnabled    = true;

    // Create production context with real adapters
    auto concerns = ConcernsContext::create(config);

    // Pass to all subsystems
    auto storage = std::make_shared<StorageEngine>(
        evaluator, encryption, keys, index_manager, concerns
    );
    auto server = std::make_shared<HttpServer>(storage, concerns);

    server->start();
}
```

### Configuration-Driven Adapter Selection

```cpp
#include "core/concerns/concerns_context.h"

// Select adapters purely via configuration — no code changes needed
ConcernsContext::Config cfg;
cfg.loggerAdapter  = "noop";        // silence all logs in this component
cfg.tracerAdapter  = "otel";        // explicit: ignore tracingEnabled flag
cfg.metricsAdapter = "prometheus";  // explicit: ignore metricsEnabled flag
cfg.cacheAdapter   = "inmemory";

auto ctx = ConcernsContext::create(cfg);
```

### Using Individual Concerns

```cpp
void processQuery(const Query& query, ConcernsContext* ctx) {
    // Logging
    ctx->logger().info("Processing query");

    // Tracing
    auto span = ctx->tracer().startSpan("query_execution");
    span->setAttribute("query_type", query.type);

    // Metrics
    ctx->metrics().incrementCounter("queries_total");

    auto result = executeQuery(query);

    ctx->metrics().observeHistogram("query_duration_ms", result.duration);
    span->end();

    return result;
}
```

### Testing with Mocks

```cpp
#include "core/concerns/noop_implementations.h"
#include <gtest/gtest.h>

TEST(StorageEngineTest, BasicPutGet) {
    // Create test context with no-op implementations
    auto ctx = ConcernsContext::createNoOp();

    StorageEngine storage(evaluator, encryption, keys, nullptr, ctx);

    ASSERT_OK(storage.put("key", "value"));
    ASSERT_EQ("value", storage.get("key").value());
}
```

### Custom Adapter Injection

```cpp
// Create custom adapter
class MyCustomLogger : public ILogger {
    // Implementation...
};

auto ctx = ConcernsContext::createCustom(
    std::make_unique<MyCustomLogger>(),
    std::make_unique<NoOpTracer>(),
    std::make_unique<PrometheusMetricsAdapter>(),
    std::make_unique<InMemoryCacheImpl>(1000, 0)
);
```

## Dependencies

### Internal Dependencies
- **themis/base/interfaces**: Base interface definitions
- **Standard Library**: `<memory>`, `<string>`, `<optional>`

### External Dependencies (Optional)
Adapters require external libraries, but core interfaces have no dependencies:

- **spdlog**: For SpdlogLoggerAdapter (optional)
- **OpenTelemetry**: For OtelTracerAdapter (optional)
- **Prometheus C++ Client**: For PrometheusMetricsAdapter (optional)

### Compilation Flags
```cmake
# Include core headers
target_include_directories(my_target PRIVATE include/core)

# No required libraries for headers-only interfaces
# Add adapters as needed:
# target_link_libraries(my_target spdlog::spdlog)
```

## Performance Characteristics

### Header-Only Overhead
- Core interfaces are pure virtual (vtable indirection only)
- No-op implementations compile to zero overhead (empty inline functions)
- Template methods in headers for zero-cost abstractions

### Adapter Overhead
See [src/core/README.md](../../src/core/README.md) for adapter performance details and implementation context.

**Summary:**
- No-op adapters: <1ns per call
- Spdlog adapter: ~50-100ns per log call
- OTEL tracer: ~1-5μs per span
- Prometheus metrics: ~200-500ns per update

## Known Limitations

1. **Single Process Scope**
   - Concerns are not shared across processes
   - Distributed systems need external coordination

2. **Interface Stability**
   - Interfaces are stable but may evolve in major versions
   - Use virtual destructors for forward compatibility

3. **No Dynamic Type Checking**
   - Interface casting requires knowledge of concrete type
   - Consider using `dynamic_cast` with caution

4. **Adapter-Specific Dependencies**
    - External dependencies are required only by concrete adapters
    - Core interface headers remain backend-agnostic

## Troubleshooting

1. **`Production mode violation` on startup**
   - Cause: `THEMIS_PRODUCTION_MODE=1` (or `THEMIS_ENVIRONMENT=production`) with disabled tracing/metrics or `createNoOp()`.
   - Fix: enable production adapters (`tracerAdapter=otel|jaeger|zipkin`, `metricsAdapter=prometheus`) and use `create(config)`.

2. **`Invalid adapter configuration` errors**
   - Cause: unsupported adapter string in `ConcernsContext::Config`.
   - Fix: validate values using `core/config_validator.h` and the adapter matrix above.

3. **No secrets resolved from environment**
   - Cause: wrong prefix or secret name mapping.
   - Fix: set `secretsAdapter="env"` and verify `secretsEnvPrefix` + uppercase/underscore key mapping.

## Status

**Production Ready** (as of v1.5.0)

✅ **Stable Interfaces:**
- ILogger, ITracer, IMetrics, ICache
- ConcernsContext API
- All adapter headers

⚠️ **Beta:**
- Distributed cache interfaces
- Advanced cache strategy headers

🔬 **Experimental:**
- Dynamic adapter replacement APIs
- Contextual logging interfaces

## Related Documentation

- [src/core/README.md](../../src/core/README.md) - Implementation details
- [CACHE_STRATEGIES_README.md](concerns/CACHE_STRATEGIES_README.md) - Cache strategies guide
- [ARCHITECTURE.md](../../ARCHITECTURE.md) - Overall system architecture
- [Roadmap](../../src/core/ROADMAP.md)
- [Future Enhancements](../../src/core/FUTURE_ENHANCEMENTS.md)
- [Production Requirements](../../src/core/PRODUCTION_REQUIREMENTS.md)

## Contributing

When modifying core headers:

1. Maintain backward compatibility (add, don't change)
2. Document all public APIs with Doxygen comments
3. Update this README with new interfaces
4. Provide no-op implementations for testing
5. Consider ABI stability implications

For detailed contribution guidelines, see [CONTRIBUTING.md](../../CONTRIBUTING.md).

## See Also

- [Roadmap](../../src/core/ROADMAP.md) - Module status, phases, and production-readiness checklist
- [Storage Headers](../storage/README.md) - Storage layer interfaces
- [Server Headers](../server/README.md) - Server protocol interfaces

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
