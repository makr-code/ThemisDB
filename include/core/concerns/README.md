> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Cross-Cutting Concerns Abstraction Layer

This directory contains the central abstraction layer for cross-cutting concerns in ThemisDB. It provides unified interfaces for logging, tracing, metrics collection, and caching, enabling dependency injection, testability, and flexibility in implementation choices.

## Overview

The concerns abstraction layer addresses the following goals:
- **Consistency**: Unified interfaces across all components
- **Flexibility**: Easy switching between implementations (production, testing, no-op)
- **Testability**: Mock implementations for unit testing
- **Monitoring**: Integrated observability through standard interfaces

## Core Interfaces

### ILogger
Abstract logger interface with standard log levels (TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL).

**Usage:**
```cpp
#include "core/concerns/i_logger.h"

void myFunction(ILogger& logger) {
    logger.info("Starting operation");
    logger.error("Something went wrong");
}
```

### ITracer
Abstract distributed tracing interface for creating and managing spans.

**Usage:**
```cpp
#include "core/concerns/i_tracer.h"

void myFunction(ITracer& tracer) {
    auto span = tracer.startSpan("myOperation");
    span->setAttribute("user_id", "123");
    // Work happens here
    span->end();
}
```

### W3CTraceContextPropagator
Stateless W3C Trace Context Level 1 propagator that bridges HTTP headers to/from the
`IContext`/`ContextPropagation` key-value store.  Use it at HTTP request boundaries
to populate `kTraceId` / `kSpanId` automatically in the thread-local context.

**Usage:**
```cpp
#include "core/concerns/w3c_trace_context_propagator.h"
#include "core/concerns/context_propagation.h"

// Inbound HTTP request handler
void onRequest(const HttpRequest& req) {
    // Extract traceparent/tracestate into a new IContext
    auto ctx = W3CTraceContextPropagator::extract(req.headers);
    ContextScope scope(ctx);     // install as current thread context

    // All downstream code can now call ContextPropagation::current()
    // ctx->get(context_keys::kTraceId) → "4bf92f3577b34da6a3ce929d0e0e4736"
    // ctx->get(context_keys::kSpanId)  → "00f067aa0ba902b7"
}

// Outbound HTTP call to downstream service
void callDownstream(const IContext& ctx, HttpRequest& out_req) {
    W3CTraceContextPropagator::inject(ctx, out_req.headers);
    // out_req.headers["traceparent"] == "00-<trace_id>-<span_id>-01"
}
```

### IMetrics
Abstract metrics interface for counters, gauges, and histograms.

**Usage:**
```cpp
#include "core/concerns/i_metrics.h"

void myFunction(IMetrics& metrics) {
    metrics.incrementCounter("requests_total");

    LatencyTimer timer(metrics, "operation_latency");
    // Work happens here
    // Latency automatically recorded on destruction
}
```

### ICache
Abstract cache interface for key-value storage with TTL support.

**Usage:**
```cpp
#include "core/concerns/i_cache.h"

void myFunction(ICache& cache) {
    auto entry = cache.get("user:123");
    if (!entry) {
        // Load from database
        CacheEntry data{"user_data", 1, timestamp};
        cache.put("user:123", data, 60000); // 60s TTL
    }
}
```

## ConcernsContext

The `ConcernsContext` class provides a unified container for all concerns, simplifying dependency injection.

**Basic Usage:**
```cpp
#include "core/concerns/concerns_context.h"

// Create default production context
auto context = ConcernsContext::create();

// Use concerns
context->logger().info("Application started");
auto span = context->tracer().startSpan("initialization");
context->metrics().incrementCounter("startup_count");
```

**Configuration:**
```cpp
ConcernsContext::Config config;
config.logLevel = "debug";
config.logFile = "app.log";
config.tracingEnabled = true;
config.tracingEndpoint = "http://jaeger:4318";
config.metricsEnabled = true;
config.cacheMaxSize = 10000;

auto context = ConcernsContext::create(config);
```

**Testing with No-Op:**
```cpp
// Create no-op context for testing (all operations are no-ops)
auto context = ConcernsContext::createNoOp();

// Or create custom context with mock implementations
auto context = ConcernsContext::createCustom(
    std::make_unique<MockLogger>(),
    std::make_unique<MockTracer>(),
    std::make_unique<MockMetrics>(),
    std::make_unique<MockCache>()
);
```

## Implementations

### Production Implementations
- **SpdlogLoggerAdapter**: Wraps existing spdlog-based logger
- **OpenTelemetryTracerAdapter**: Wraps OpenTelemetry distributed tracing
- **PrometheusMetricsAdapter**: Wraps MetricsCollector for Prometheus export
- **InMemoryCacheImpl**: Thread-safe LRU cache with TTL support

### Testing Implementations
- **NoOpLogger**: Silent logger for testing
- **NoOpTracer**: No-op tracer for testing
- **NoOpMetrics**: No-op metrics for testing
- **NoOpCache**: No-op cache that always misses

## Integration Guide

### 1. Update Component Constructor

**Before:**
```cpp
class MyComponent {
public:
    MyComponent() {
        // Direct usage of global logger
        THEMIS_INFO("Component initialized");
    }
};
```

**After:**
```cpp
#include "core/concerns/concerns_context.h"

class MyComponent {
public:
    explicit MyComponent(std::shared_ptr<ConcernsContext> concerns)
        : concerns_(concerns) {
        concerns_->logger().info("Component initialized");
    }

private:
    std::shared_ptr<ConcernsContext> concerns_;
};
```

### 2. Use Concerns Throughout Component

```cpp
void MyComponent::performOperation(const std::string& userId) {
    // Logging
    concerns_->logger().debug("Starting operation for user: " + userId);

    // Tracing
    auto span = concerns_->tracer().startSpan("performOperation");
    span->setAttribute("user_id", userId);

    // Metrics
    LatencyTimer timer(concerns_->metrics(), "operation_duration");

    try {
        // Check cache
        auto cacheKey = "user:" + userId;
        auto cached = concerns_->cache().get(cacheKey);

        if (cached) {
            concerns_->metrics().incrementCounter("cache_hits");
            return;
        }

        concerns_->metrics().incrementCounter("cache_misses");

        // Perform operation
        // ...

        concerns_->metrics().recordSuccess("performOperation");
        span->setStatus(true);

    } catch (const std::exception& e) {
        concerns_->logger().error("Operation failed: " + std::string(e.what()));
        concerns_->metrics().recordError("performOperation");
        span->recordError(e.what());
        span->setStatus(false);
        throw;
    }
}
```

### 3. Component Testing

```cpp
#include <gtest/gtest.h>
#include "core/concerns/concerns_context.h"

TEST(MyComponentTest, PerformOperation) {
    // Create no-op context for testing
    auto concerns = ConcernsContext::createNoOp();

    // Or use custom mocks
    auto mockLogger = std::make_unique<MockLogger>();
    EXPECT_CALL(*mockLogger, info(_)).Times(1);

    auto concerns = ConcernsContext::createCustom(
        std::move(mockLogger),
        std::make_unique<NoOpTracer>(),
        std::make_unique<NoOpMetrics>(),
        std::make_unique<NoOpCache>()
    );

    MyComponent component(concerns);
    component.performOperation("user123");
}
```

## Lifecycle Management

Every concern interface (`ILogger`, `ITracer`, `IMetrics`, `ICache`) exposes
three lifecycle methods that must be honoured in production deployments:

| Method | Purpose |
|--------|---------|
| `flush()` | Forward any buffered data to the sink immediately. |
| `shutdown()` | Flush, then tear down the resource and release connections. |
| `isHealthy()` | Probe whether the underlying sink/backend is operational. |

`ConcernsContext` provides matching aggregate methods that iterate over all
four concerns in the correct order:

```cpp
// Flush pending log/span/metric data without shutting down
context->flush();

// Graceful shutdown – call in signal handler or atexit()
context->shutdown();
```

> **Note:** After `shutdown()` the context must **not** be reused.

### Health and Readiness Probes

```cpp
// Liveness probe (e.g. Kubernetes /healthz)
auto health = context->healthCheck();
if (!health.isHealthy()) {
    // At least one concern is unhealthy
    if (!health.logger.ok)  std::cerr << "Logger: " << health.logger.message << "\n";
    if (!health.tracer.ok)  std::cerr << "Tracer: " << health.tracer.message << "\n";
    if (!health.metrics.ok) std::cerr << "Metrics: " << health.metrics.message << "\n";
    if (!health.cache.ok)   std::cerr << "Cache: " << health.cache.message << "\n";
}

// Readiness probe (e.g. Kubernetes /readyz)
auto ready = context->readinessCheck();
if (!ready.isHealthy()) {
    // Service is not ready to accept traffic
}
```

### `ProbeResult` and `HealthStatus`

Defined in `include/core/concerns/lifecycle.h`:

```cpp
struct ProbeResult {
    bool ok = true;
    std::string message;

    static ProbeResult healthy(const std::string& msg = "ok");
    static ProbeResult unhealthy(const std::string& msg);
};

struct HealthStatus {
    ProbeResult logger, tracer, metrics, cache;
    bool isHealthy() const;  // true iff all four are ok
};
```

### Production Deployment Pattern

```cpp
// 1. Build context
auto context = ConcernsContext::create(config);

// 2. Register shutdown hook (signal handler or atexit)
// NOTE: capture context by value (shared_ptr) so it remains valid at exit.
std::atexit([context]{ context->shutdown(); });

// 3. Expose /healthz endpoint
httpServer.get("/healthz", [&](auto& req, auto& res) {
    auto status = context->healthCheck();
    if (status.isHealthy()) {
        res.status = 200;
        res.body   = "ok";
    } else {
        res.status = 503;
        // Populate response with per-concern details
    }
});

// 4. Expose /readyz endpoint
httpServer.get("/readyz", [&](auto& req, auto& res) {
    auto status = context->readinessCheck();
    res.status = status.isHealthy() ? 200 : 503;
});
```



1. **Phase 1**: Create concerns abstraction layer (DONE)
2. **Phase 2**: Update new components to use ConcernsContext
3. **Phase 3**: Gradually migrate existing components
4. **Phase 4**: Maintain backward compatibility with existing Logger/Tracer/MetricsCollector

## CI Integration

### Recommended Pattern for CI/CD Pipelines

In CI environments, use a **no-op context** so tests run without external
dependencies (spdlog file sinks, OTLP collectors, Prometheus endpoints):

```cpp
// test_main.cpp or test fixture SetUp()
auto concerns = themis::core::concerns::ConcernsContext::createNoOp();
// Inject into the component under test
MyComponent component(concerns);
```

### Validating Lifecycle Hooks in Tests

After exercising a component, verify that lifecycle hooks are callable and
leave no dangling state:

```cpp
TEST(MyComponentTest, LifecycleHooksAreClean) {
    auto concerns = ConcernsContext::createNoOp();
    MyComponent component(concerns);
    component.doWork();

    // Flush should be idempotent and not crash
    EXPECT_NO_THROW(concerns->flush());

    // Shutdown should be idempotent and not crash
    EXPECT_NO_THROW(concerns->shutdown());
}
```

### Validating Health/Readiness Probes

```cpp
TEST(MyComponentTest, HealthCheckPassesAfterInit) {
    auto concerns = ConcernsContext::createNoOp();
    MyComponent component(concerns);

    auto status = concerns->healthCheck();
    EXPECT_TRUE(status.isHealthy());
}
```

### Simulating Unhealthy Dependencies

Use `ConcernsContext::createCustom()` to inject an implementation that
reports unhealthy — useful for testing circuit-breaker and retry logic:

```cpp
class UnhealthyCache : public NoOpCache {
public:
    ProbeResult isHealthy() const override {
        return ProbeResult::unhealthy("cache backend unavailable");
    }
};

TEST(MyComponentTest, HandlesUnhealthyCache) {
    auto concerns = ConcernsContext::createCustom(
        std::make_unique<NoOpLogger>(),
        std::make_unique<NoOpTracer>(),
        std::make_unique<NoOpMetrics>(),
        std::make_unique<UnhealthyCache>()
    );

    auto status = concerns->healthCheck();
    EXPECT_FALSE(status.isHealthy());
    EXPECT_FALSE(status.cache.ok);
    EXPECT_EQ(status.cache.message, "cache backend unavailable");
}
```

### noexcept Contract

The following lifecycle methods are declared `noexcept` and are safe to call
from destructors, signal handlers, and other non-throwing contexts:

| Method | `noexcept` |
|--------|-----------|
| `ILogger::flush()` default | ✅ |
| `ILogger::shutdown()` default | ✅ |
| `ITracer::flush()` default | ✅ |
| `IMetrics::flush()` default | ✅ |
| `IMetrics::shutdown()` default | ✅ |
| `ICache::flush()` default | ✅ |
| `ICache::shutdown()` default | ✅ |
| `HealthStatus::isHealthy()` | ✅ |
| `TraceContext::empty()` | ✅ |
| `LatencyTimer::elapsedMs()` | ✅ |
| All `NoOp*::flush()` / `shutdown()` | ✅ |

> **Note:** Concrete adapters (spdlog, OpenTelemetry, Prometheus) do NOT
> declare their lifecycle overrides `noexcept` because they interact with
> external resources that may throw.

## Benefits

### Before (Current State)
- Direct coupling to specific implementations (spdlog, OpenTelemetry)
- Difficult to test components in isolation
- Global state makes testing harder
- Inconsistent usage patterns across codebase
- Hard to switch implementations

### After (With Concerns Layer)
- Loose coupling through interfaces
- Easy to test with mock/no-op implementations
- Dependency injection enables isolation
- Consistent patterns across all components
- Easy to switch or combine implementations
- Better observability integration

## Files

### Interfaces
- `include/core/concerns/i_logger.h` - Logger interface
- `include/core/concerns/i_tracer.h` - Tracer interface
- `include/core/concerns/i_metrics.h` - Metrics interface
- `include/core/concerns/i_cache.h` - Cache interface
- `include/core/concerns/lifecycle.h` - `ProbeResult` and `HealthStatus` types
- `include/core/concerns/metric_labels.h` - `MetricLabels` fluent builder and `labels::k*` constants
- `include/core/concerns/i_context.h` - `IContext` interface, `SimpleContext` impl, and `context_keys::k*` constants
- `include/core/concerns/i_async_logger.h` - `IAsyncLogger` interface and `NoOpAsyncLogger` impl
- `include/core/concerns/i_async_cache.h` - `IAsyncCache` interface and `NoOpAsyncCache` impl
- `include/core/concerns/w3c_trace_context_propagator.h` - `W3CTraceContextPropagator` for W3C TraceContext extract/inject
- `include/core/concerns/context_propagation.h` - `ContextPropagation` thread-local store and `ContextScope` RAII guard

### Implementations
- `include/core/concerns/spdlog_logger_adapter.h` - Spdlog adapter
- `include/core/concerns/otel_tracer_adapter.h` - OpenTelemetry adapter
- `include/core/concerns/prometheus_metrics_adapter.h` - Prometheus adapter
- `include/core/concerns/inmemory_cache_impl.h` - In-memory cache
- `include/core/concerns/noop_implementations.h` - No-op implementations

### Context
- `include/core/concerns/concerns_context.h` - Main DI context
- `src/core/concerns/concerns_context.cpp` - Context implementation

### Tests
- `tests/test_concerns_context.cpp` - Comprehensive test suite
- `tests/test_context_propagation.cpp` - ContextPropagation / ContextScope tests
- `tests/test_w3c_trace_context_propagator.cpp` - W3CTraceContextPropagator tests (23 cases)

### Examples
- `examples/concerns_example.cpp` - End-to-end demonstration of all interfaces: `ILogger`, `ITracer`, `IMetrics`, `ICache`, `IContext`, `MetricLabels`, lifecycle hooks, health/readiness probes

## Future Enhancements

- [ ] Redis cache adapter
- [ ] StatsD metrics adapter
- [ ] Custom logger backends (JSON, structured logging)
- [ ] Automatic concern injection via service locator pattern
- [ ] Performance profiling and optimization
- [ ] Additional cache strategies (LFU, ARC)
- [ ] Distributed cache support

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
