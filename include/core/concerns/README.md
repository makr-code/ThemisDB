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

## Migration Strategy

1. **Phase 1**: Create concerns abstraction layer (DONE)
2. **Phase 2**: Update new components to use ConcernsContext
3. **Phase 3**: Gradually migrate existing components
4. **Phase 4**: Maintain backward compatibility with existing Logger/Tracer/MetricsCollector

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

## Future Enhancements

- [ ] Redis cache adapter
- [ ] StatsD metrics adapter
- [ ] Custom logger backends (JSON, structured logging)
- [ ] Automatic concern injection via service locator pattern
- [ ] Performance profiling and optimization
- [ ] Additional cache strategies (LFU, ARC)
- [ ] Distributed cache support
