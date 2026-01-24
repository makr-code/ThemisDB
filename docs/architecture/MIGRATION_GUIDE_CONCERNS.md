# Migration Guide: Integrating Cross-Cutting Concerns

This guide demonstrates how to migrate an existing ThemisDB component to use the new concerns abstraction layer.

## Example: Migrating a Component

### Before Migration

```cpp
// old_component.h
#pragma once
#include "utils/logger.h"

class OldComponent {
public:
    OldComponent() {
        // Direct use of global logger
        THEMIS_INFO("Component initialized");
    }
    
    void process() {
        THEMIS_DEBUG("Processing started");
        // ... do work ...
        THEMIS_INFO("Processing completed");
    }
};
```

### After Migration

```cpp
// new_component.h
#pragma once
#include "core/concerns/concerns_context.h"
#include <memory>

class NewComponent {
public:
    explicit NewComponent(std::shared_ptr<themis::core::concerns::ConcernsContext> concerns)
        : concerns_(concerns) {
        // Use injected logger
        concerns_->logger().info("Component initialized");
    }
    
    void process() {
        // Start tracing span
        auto span = concerns_->tracer().startSpan("NewComponent::process");
        
        // Track latency
        themis::core::concerns::LatencyTimer timer(
            concerns_->metrics(), "component_process_duration");
        
        concerns_->logger().debug("Processing started");
        
        // ... do work ...
        
        concerns_->metrics().incrementCounter("component_process_count");
        concerns_->logger().info("Processing completed");
        span->setStatus(true);
    }
    
private:
    std::shared_ptr<themis::core::concerns::ConcernsContext> concerns_;
};
```

### Usage in Tests

```cpp
// test_new_component.cpp
#include "new_component.h"
#include "core/concerns/concerns_context.h"
#include <gtest/gtest.h>

TEST(NewComponentTest, ProcessWorks) {
    // Use no-op context for fast testing
    auto concerns = themis::core::concerns::ConcernsContext::createNoOp();
    
    NewComponent component(concerns);
    component.process();
    
    // Test passes without any logging/tracing overhead
    EXPECT_TRUE(true);
}

TEST(NewComponentTest, ProcessWithMockLogger) {
    // Use custom mock to verify logging
    auto mockLogger = std::make_unique<MockLogger>();
    EXPECT_CALL(*mockLogger, info(_)).Times(2); // Init + complete
    
    auto concerns = themis::core::concerns::ConcernsContext::createCustom(
        std::move(mockLogger),
        std::make_unique<themis::core::concerns::NoOpTracer>(),
        std::make_unique<themis::core::concerns::NoOpMetrics>(),
        std::make_unique<themis::core::concerns::NoOpCache>()
    );
    
    NewComponent component(concerns);
    component.process();
}
```

## Migration Strategy

### Phase 1: New Components (Recommended)
All new components should use `ConcernsContext` from the start.

```cpp
class MyNewService {
public:
    explicit MyNewService(std::shared_ptr<ConcernsContext> concerns)
        : concerns_(concerns) {}
    
private:
    std::shared_ptr<ConcernsContext> concerns_;
};
```

### Phase 2: Gradual Migration (Existing Components)
Existing components can be migrated gradually:

1. **Add optional concerns parameter to constructor**
   ```cpp
   class ExistingService {
   public:
       ExistingService(
           std::shared_ptr<ConcernsContext> concerns = nullptr)
           : concerns_(concerns) {}
       
       void doWork() {
           if (concerns_) {
               concerns_->logger().info("Using new concerns");
           } else {
               THEMIS_INFO("Using old logger");
           }
       }
   
   private:
       std::shared_ptr<ConcernsContext> concerns_;
   };
   ```

2. **Update call sites gradually**
   ```cpp
   // Old code (still works)
   auto service = std::make_shared<ExistingService>();
   
   // New code (uses concerns)
   auto concerns = ConcernsContext::create();
   auto service = std::make_shared<ExistingService>(concerns);
   ```

3. **Remove fallback once migration is complete**
   ```cpp
   class ExistingService {
   public:
       // Make concerns required
       explicit ExistingService(
           std::shared_ptr<ConcernsContext> concerns)
           : concerns_(concerns) {
           assert(concerns != nullptr);
       }
       
       void doWork() {
           concerns_->logger().info("Always uses concerns");
       }
   
   private:
       std::shared_ptr<ConcernsContext> concerns_;
   };
   ```

### Phase 3: Backward Compatibility
The old logging/tracing/metrics APIs remain available:
- `THEMIS_INFO()` macros still work
- `utils::Logger::get()` still available
- `observability::MetricsCollector::getInstance()` still available

## Application Initialization

### Before

```cpp
int main() {
    // Initialize logger globally
    themis::utils::Logger::init("themisdb.log", 
        themis::utils::Logger::Level::INFO);
    
    // Initialize tracer globally
    themis::Tracer::initialize("themisdb", "http://localhost:4318");
    
    // Components use global instances
    auto service = std::make_shared<SomeService>();
    service->run();
    
    return 0;
}
```

### After

```cpp
int main() {
    // Create concerns context
    themis::core::concerns::ConcernsContext::Config config;
    config.logLevel = "info";
    config.logFile = "themisdb.log";
    config.tracingEnabled = true;
    config.tracingServiceName = "themisdb";
    config.tracingEndpoint = "http://localhost:4318";
    config.metricsEnabled = true;
    
    auto concerns = themis::core::concerns::ConcernsContext::create(config);
    
    // Components receive injected concerns
    auto service = std::make_shared<SomeService>(concerns);
    service->run();
    
    return 0;
}
```

## Benefits After Migration

### Testability
```cpp
// Easy to test with no-op implementations
TEST(ServiceTest, FastTest) {
    auto concerns = ConcernsContext::createNoOp();
    Service service(concerns);
    // Test runs fast without I/O
}

// Easy to test with mocks
TEST(ServiceTest, VerifyBehavior) {
    auto mockLogger = std::make_unique<MockLogger>();
    EXPECT_CALL(*mockLogger, error(_)).Times(0); // Verify no errors
    
    auto concerns = ConcernsContext::createCustom(
        std::move(mockLogger), ...);
    Service service(concerns);
}
```

### Flexibility
```cpp
// Easy to switch implementations
auto prodConcerns = ConcernsContext::create(); // Real implementations
auto testConcerns = ConcernsContext::createNoOp(); // No-op for tests
auto devConcerns = ConcernsContext::createCustom(...); // Custom setup
```

### Consistency
```cpp
// All components use the same interface
class ServiceA {
    explicit ServiceA(std::shared_ptr<ConcernsContext> concerns);
};

class ServiceB {
    explicit ServiceB(std::shared_ptr<ConcernsContext> concerns);
};

// Both have consistent logging, tracing, metrics, caching
```

## Troubleshooting

### Q: Can I mix old and new approaches?
**A:** Yes! The old macros (`THEMIS_INFO`, etc.) and the new interfaces can coexist. Migrate gradually.

### Q: What if I don't need all concerns?
**A:** Use `createNoOp()` or `createCustom()` to provide only the concerns you need.

### Q: Performance impact?
**A:** Minimal. Virtual function calls are typically inlined by the compiler. No-op implementations have zero overhead.

### Q: How to handle legacy code?
**A:** Use the adapter pattern. Wrap legacy interfaces to implement the new concerns interfaces.

## Next Steps

1. Start using `ConcernsContext` in all new components
2. Write new tests using no-op or mock concerns
3. Gradually migrate existing components
4. Remove old global logger/tracer usage once migration is complete

For more details, see:
- `include/core/concerns/README.md` - Full API documentation
- `tests/test_concerns_context.cpp` - Comprehensive test examples
- `examples/concerns/example_concerns_usage.cpp` - Usage examples
