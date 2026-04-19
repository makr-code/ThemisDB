# ThemisDB Core Module

## Module Purpose

The Core module provides the foundational cross-cutting concerns infrastructure for ThemisDB. It implements a sophisticated Dependency Injection framework that enables pluggable implementations of observability, caching, and security features throughout the database engine.

## Relevant Interfaces

| Interface / File | Role |
|-----------------|------|
| `concerns_context.h` / `concerns_context.cpp` | Central DI hub for all cross-cutting concerns |
| `i_logger.h` / `spdlog_adapter.cpp` | Logger interface and spdlog production adapter |
| `i_tracer.h` | Distributed tracing interface |
| `i_metrics.h` | Metrics collection interface |
| `i_cache.h` | Cache abstraction interface |

## Scope

**In Scope:**
- Cross-cutting concerns management (logging, tracing, metrics, caching)
- Dependency injection infrastructure via ConcernsContext
- Pluggable adapter interfaces for external frameworks
- Configuration-driven initialization
- Production-safe defaults with environment detection

**Out of Scope:**
- Data storage and persistence (handled by storage module)
- Network protocols and API handlers (handled by server module)
- Query parsing and execution (handled by query module)
- Specific framework implementations (adapters only)

## Key Components

### ConcernsContext
**Location:** `src/core/concerns/concerns_context.cpp`, `include/core/concerns/concerns_context.h`

Central hub for managing cross-cutting concerns using the Service Locator + Dependency Injection patterns.

```cpp
// Create a production-ready context with real implementations
auto context = ConcernsContext::create(config);

// Access concerns
context->logger()->info("Database started");
context->tracer()->startSpan("query_execution");
context->metrics()->incrementCounter("requests_total");
context->cache()->set("key", value);
```

**Features:**
- Thread-safe immutable context after creation
- Factory methods for different environments (production, testing, custom)
- Lazy initialization for optional components
- Environment variable detection for production mode

### Core Interfaces

#### ILogger
**Location:** `include/core/concerns/i_logger.h`

Abstract logging interface supporting multiple severity levels and structured logging.

**Adapters:**
- `SpdlogLoggerAdapter` - Integration with spdlog library
- `NoopLogger` - Minimal overhead for performance-critical paths

**Usage:**
```cpp
logger->info("User {} logged in", username);
logger->error("Failed to connect: {}", error_message);
logger->debug("Query plan: {}", query_plan);
```

#### ITracer
**Location:** `include/core/concerns/i_tracer.h`

Distributed tracing abstraction for request correlation and performance analysis.

**Adapters:**
- `OtelTracerAdapter` - OpenTelemetry integration for distributed tracing
- `NoopTracer` - Zero-overhead implementation

**Usage:**
```cpp
auto span = tracer->startSpan("database_query");
span->setAttribute("table", "users");
span->end(); // Automatically measured
```

#### IMetrics
**Location:** `include/core/concerns/i_metrics.h`

Metrics collection interface for monitoring and alerting.

**Adapters:**
- `PrometheusMetricsAdapter` - Prometheus metrics format
- `NoopMetrics` - Zero-overhead implementation

**Usage:**
```cpp
metrics->incrementCounter("queries_executed");
metrics->recordHistogram("query_duration_ms", duration);
metrics->setGauge("active_connections", count);
```

#### ICache
**Location:** `include/core/concerns/i_cache.h`

Generic caching abstraction supporting multiple eviction strategies.

**Implementations:**
- `InMemoryCacheImpl` - Local in-process cache
- `StrategicCacheImpl` - Cache with pluggable eviction strategies (LRU, LIRS, ARC, etc.)
- `NoopCache` - Pass-through implementation

**Usage:**
```cpp
if (auto value = cache->get("query_result:123")) {
    return *value;
}
auto result = execute_query();
cache->set("query_result:123", result);
```

### Security Initialization
**Location:** `src/core/security_initialization.cpp`

Provides secure initialization routines for cryptographic components and key management.

## Architecture

### Design Patterns

1. **Dependency Injection**
   - All components depend on interfaces, not concrete implementations
   - Runtime configuration of implementations
   - Testability through mock injection

2. **Factory Pattern**
   - `ConcernsContext::create()` - Default production configuration
   - `ConcernsContext::create(const Config&)` - Configuration-driven adapter selection
   - `ConcernsContext::createNoOp()` - Test-friendly no-op configuration
   - `ConcernsContext::createCustom()` - User-defined configuration

3. **Adapter Pattern**
   - Framework-specific adapters implement abstract interfaces
   - Allows swapping between spdlog, log4cpp, etc. without code changes
   - Zero-dependency core interfaces

4. **Strategy Pattern**
   - Multiple cache eviction strategies (LRU, LIRS, ARC, 2Q, MRU)
   - Pluggable at runtime via configuration
   - See `include/core/concerns/CACHE_STRATEGIES_README.md`

### Thread Safety

- **ConcernsContext**: Immutable after construction, thread-safe access
- **Logger/Tracer/Metrics Adapters**: Thread-safe implementations
- **Cache**: Thread-safe with internal locking (implementation-dependent)

### Initialization Flow

```
1. Parse Configuration
2. Create ConcernsContext via factory
   ├─ Initialize Logger (spdlog or noop)
   ├─ Initialize Tracer (OTEL or noop)
   ├─ Initialize Metrics (Prometheus or noop)
   └─ Initialize Cache (InMemory/Strategic or noop)
3. Pass context to all subsystems
4. Subsystems access concerns via context
```

## Integration Points

### Storage Module Integration
The Storage Engine accepts a ConcernsContext to enable observability:

```cpp
StorageEngine storage(evaluator, encryption, key_provider, index_manager);
storage.setConcerns(context);  // Enable logging, tracing, metrics
```

**Used For:**
- Query execution tracing
- Performance metrics (read/write latency)
- Debug logging for troubleshooting
- Query result caching

### Server Module Integration
API handlers receive ConcernsContext during construction:

```cpp
EntityAPIHandler handler(storage, context);
// Handler automatically logs requests, traces execution, records metrics
```

**Used For:**
- Request/response logging
- Distributed tracing across services
- API metrics (request counts, latencies, errors)
- Response caching

### Query Module Integration
Query execution leverages all concerns:

```cpp
QueryEngine engine(storage, context);
// Traces query parsing, optimization, execution
// Logs query plans and execution times
// Caches compiled query plans
```

## API/Usage Examples

### Basic Usage

```cpp
#include "core/concerns/concerns_context.h"

// Create context for production
auto context = ConcernsContext::create(config);

// Use throughout application
void processRequest(Request req, std::shared_ptr<ConcernsContext> ctx) {
    ctx->logger()->info("Processing request {}", req.id);

    auto span = ctx->tracer()->startSpan("process_request");
    span->setAttribute("request_id", req.id);

    ctx->metrics()->incrementCounter("requests_total");

    // Check cache
    if (auto cached = ctx->cache()->get(req.cache_key())) {
        ctx->metrics()->incrementCounter("cache_hits");
        return *cached;
    }

    // Process and cache result
    auto result = doWork(req);
    ctx->cache()->set(req.cache_key(), result);

    span->end();
}
```

### Testing with Mock Implementations

```cpp
#include "core/concerns/concerns_context.h"
#include "core/concerns/noop_implementations.h"

// Create no-op context for unit tests
auto test_context = ConcernsContext::createNoOp();

// Or create custom context with mocks
auto custom_context = ConcernsContext::createCustom(
    std::make_unique<MockLogger>(),
    std::make_unique<NoOpTracer>(),
    std::make_unique<NoOpMetrics>(),
    std::make_unique<NoOpCache>()
);

// Verify logging calls
processRequest(request, custom_context);
ASSERT_EQ(1, mock_logger->info_calls.size());
```

### Production Mode Detection

The core module automatically detects production environments:

```bash
# Enable production mode (triggers safety checks)
export THEMIS_PRODUCTION_MODE=1
# OR
export THEMIS_ENVIRONMENT=production

# Production mode enforces:
# - No default (no-op) encryption allowed
# - No default (insecure) key providers
# - Warnings for default implementations
```

## Dependencies

### Internal Dependencies
- **themis/base/interfaces**: Core interface definitions
- **utils/tracing**: Tracing utilities
- **utils/expected**: Result types for error handling

### External Dependencies
- **spdlog** (optional): Logging adapter implementation
- **OpenTelemetry** (optional): Tracing adapter implementation
- **Prometheus C++ Client** (optional): Metrics adapter implementation
- **fmt**: String formatting library
- **Standard Library**: Thread-safe containers, mutex

### Build Configuration
```cmake
# Minimal build (no-op implementations only)
-DTHEMIS_BUILD_TYPE=MINIMAL

# Community build (spdlog, basic observability)
-DTHEMIS_BUILD_TYPE=COMMUNITY

# Enterprise build (full OTEL, Prometheus, distributed tracing)
-DTHEMIS_BUILD_TYPE=ENTERPRISE
```

## Performance Characteristics

### Memory Footprint
- **ConcernsContext**: ~200 bytes (shared pointers to adapters)
- **No-op implementations**: Near-zero overhead (inline no-ops)
- **Cache**: Configurable (default 100MB for InMemoryCacheImpl)

### CPU Overhead
- **No-op adapters**: <1ns per call (optimized away by compiler)
- **Spdlog adapter**: ~50-100ns per log call (async mode)
- **Prometheus metrics**: ~200-500ns per metric update
- **OTEL tracing**: ~1-5μs per span creation

### Recommendations
- Use no-op implementations for hot paths (e.g., tight loops)
- Use async logging to minimize I/O blocking
- Sample traces (e.g., 1 in 100 requests) for high-traffic endpoints
- Tune cache size based on working set size

## Known Limitations

1. **Single Process Only**
   - ConcernsContext is not shared across processes
   - Distributed caching requires external system (Redis)

2. **Configuration at Startup**
   - Cannot change adapters after ConcernsContext creation
   - Requires restart to switch logging/tracing backends

3. **Cache Consistency**
   - In-memory cache is local to each node
   - No automatic invalidation across distributed systems

4. **Metric Cardinality**
   - High-cardinality labels can cause memory issues
   - Recommendation: Limit unique label combinations

5. **Default Implementations**
   - Default evaluator, encryption, key provider are NOT production-safe
   - Will throw errors in production mode if not overridden

## Status

**Production Ready** (as of v1.5.0)

✅ **Stable Features:**
- Core interfaces (ILogger, ITracer, IMetrics, ICache)
- ConcernsContext factory methods
- Adapter implementations (spdlog, OTEL, Prometheus)
- Production mode detection
- Thread-safe operations

⚠️ **Beta Features:**
- Distributed cache integration (Redis adapter)
- Advanced cache strategies (LIRS, ARC)

🔬 **Experimental:**
- Dynamic adapter switching (runtime reconfiguration)
- Metrics aggregation across shards

## Related Documentation

- [Cache Strategies Guide](../../include/core/concerns/CACHE_STRATEGIES_README.md)
- [Architecture Overview](../../ARCHITECTURE.md)
- [Dependency Injection Patterns](../../docs/patterns/dependency-injection.md)
- [Observability Configuration](../../docs/configuration/observability.md)

## Contributing

When adding new cross-cutting concerns:

1. Define interface in `include/core/concerns/`
2. Provide no-op implementation
3. Add adapter for specific framework
4. Update ConcernsContext factory methods
5. Document in this README
6. Add usage examples in tests

## See Also

- [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md) - Planned features and improvements
- [Storage Module](../storage/README.md) - Storage layer documentation
- [Server Module](../server/README.md) - Server implementation documentation

## Scientific References

1. Fowler, M. (2004). **Inversion of Control Containers and the Dependency Injection Pattern**. martinfowler.com. https://martinfowler.com/articles/injection.html

2. Gamma, E., Helm, R., Johnson, R., & Vlissides, J. (1994). **Design Patterns: Elements of Reusable Object-Oriented Software**. Addison-Wesley. ISBN: 978-0-201-63361-0

3. Martin, R. C. (2003). **Agile Software Development, Principles, Patterns, and Practices**. Prentice Hall. ISBN: 978-0-135-97444-5

4. Johnson, R., & Foote, B. (1988). **Designing Reusable Classes**. *Journal of Object-Oriented Programming*, 1(2), 22–35.

5. Steele, G. L. (1994). **Building Interpreters by Composing Monads**. *Proceedings of the 21st ACM SIGPLAN-SIGACT Symposium on Principles of Programming Languages (POPL)*, 472–492. https://doi.org/10.1145/174675.178068

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.
