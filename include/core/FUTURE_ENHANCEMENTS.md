# Core Headers - Future Enhancements

## Planned Features

### IContext Interface for Contextual Logging
**Priority:** High  
**Target Version:** v1.6.0

Add new interface for context propagation through call chains.

```cpp
class IContext {
public:
    virtual void setAttribute(std::string_view key, std::string_view value) = 0;
    virtual std::optional<std::string> getAttribute(std::string_view key) const = 0;
    virtual IContextPtr createChild() const = 0;
};

class ILogger {
    // New method
    virtual ILoggerPtr withContext(IContextPtr context) = 0;
};
```

**Benefits:**
- Automatic request ID propagation
- Structured logging context
- Thread-local context management

---

### Async Interfaces
**Priority:** High  
**Target Version:** v1.6.0

Add async variants of synchronous interfaces for non-blocking operations.

```cpp
class IAsyncLogger : public ILogger {
public:
    virtual std::future<void> infoAsync(std::string_view msg) = 0;
    virtual std::future<void> errorAsync(std::string_view msg) = 0;
};

class IAsyncCache : public ICache {
public:
    virtual std::future<std::optional<std::string>> getAsync(std::string_view key) = 0;
    virtual std::future<void> setAsync(std::string_view key, std::string_view value) = 0;
};
```

**Use Cases:**
- Non-blocking logging in critical paths
- Async cache warming
- Background metric updates

---

### Type-Safe Metrics Labels
**Priority:** Medium  
**Target Version:** v1.7.0

Replace string-based labels with type-safe label sets.

```cpp
struct MetricLabels {
    std::string endpoint;
    std::string method;
    int status_code;
};

class IMetrics {
    // Current (string-based)
    virtual void incrementCounter(std::string_view name, double value = 1.0) = 0;
    
    // Proposed (type-safe)
    template<typename Labels>
    void incrementCounter(std::string_view name, const Labels& labels, double value = 1.0);
};
```

**Benefits:**
- Compile-time label validation
- Prevents typos in label names
- Better IDE autocomplete

---

### Streaming Trace API
**Priority:** Medium  
**Target Version:** v1.7.0

Add streaming interface for large trace events.

```cpp
class IStreamingTracer : public ITracer {
public:
    virtual TraceStreamPtr createStream(std::string_view name) = 0;
};

class TraceStream {
public:
    void addEvent(std::string_view name);
    void addAttribute(std::string_view key, std::string_view value);
    void flush();  // Batch send to backend
};
```

**Use Cases:**
- Long-running operations with many events
- Reduce memory overhead for complex traces
- Better OTEL backend efficiency

---

### Cache Eviction Callbacks
**Priority:** Low  
**Target Version:** v1.8.0

Add callback interface for cache eviction events.

```cpp
class ICacheEvictionListener {
public:
    virtual void onEvict(std::string_view key, std::string_view value) = 0;
};

class ICache {
    virtual void registerEvictionListener(ICacheEvictionListenerPtr listener) = 0;
};
```

**Use Cases:**
- Cascade invalidation
- Metrics on eviction patterns
- Debugging cache behavior

---

### Health Check Interface
**Priority:** Medium  
**Status:** ✅ Implemented in v1.6.0

All four concern interfaces (`ILogger`, `ITracer`, `IMetrics`, `ICache`) now
expose an `isHealthy()` probe returning a `ProbeResult{ok, message}`.
`ConcernsContext::healthCheck()` and `readinessCheck()` aggregate all four
results into a `HealthStatus`.  `MonitoringApiHandler::handleLiveness()` and
`handleReadiness()` include per-concern health details in the JSON response
when a `ConcernsContext` is injected.

See `include/core/concerns/lifecycle.h` and the "Lifecycle Management"
section of `include/core/concerns/README.md` for API details.

---

## Performance Optimizations

### constexpr Interface Methods
**Priority:** High  
**Target Version:** v1.6.0

Make no-op implementations `constexpr` for compile-time elimination.

```cpp
class NoopLogger : public ILogger {
public:
    constexpr void info(std::string_view msg) override { /* no-op */ }
    constexpr void error(std::string_view msg) override { /* no-op */ }
};
```

**Expected Improvement:** Zero overhead for no-op paths (eliminated at compile time)

---

### Header-Only Cache Implementations
**Priority:** Medium  
**Target Version:** v1.7.0

Move simple cache implementations to header-only for inlining.

```cpp
// Current: Implementation in .cpp file
// Proposed: Entire implementation in header for inlining

template<size_t MaxSize = 1000>
class InlineInMemoryCache : public ICache {
    // Full implementation in header
};
```

**Expected Improvement:** 30-40% faster cache operations via inlining

---

### Small String Optimization in Interfaces
**Priority:** Low  
**Target Version:** v1.8.0

Use `std::string_view` everywhere instead of `std::string` parameters.

**Current:** Mix of `std::string` and `std::string_view`  
**Target:** Consistent `std::string_view` usage

**Expected Improvement:** Reduce string allocations by 50%

---

### Virtual Method Devirtualization
**Priority:** Low  
**Target Version:** v1.8.0

Use CRTP (Curiously Recurring Template Pattern) for hot-path interfaces.

```cpp
// Current: Virtual interface
class IMetrics {
    virtual void incrementCounter(std::string_view name, double value) = 0;
};

// Proposed: CRTP for zero-overhead
template<typename Derived>
class MetricsBase {
    void incrementCounter(std::string_view name, double value) {
        static_cast<Derived*>(this)->incrementCounterImpl(name, value);
    }
};
```

**Expected Improvement:** Eliminate vtable overhead (5-10ns per call)

---

## Refactoring Opportunities

### Separate Synchronous and Asynchronous Interfaces
**Priority:** High  
**Target Version:** v1.7.0

Split interfaces into sync and async variants for clarity.

**Current Structure:**
```
ILogger (mixed sync/async)
ITracer (mixed sync/async)
```

**Proposed Structure:**
```
ILogger (synchronous only)
IAsyncLogger : public ILogger (adds async methods)

ITracer (synchronous only)
IAsyncTracer : public ITracer (adds async methods)
```

**Benefits:**
- Clearer API contracts
- Optional async support
- Easier to implement

---

### Use std::expected Instead of std::optional
**Priority:** Medium  
**Target Version:** v1.7.0

Replace `std::optional` with `std::expected<T, Error>` for better error handling.

```cpp
// Current
class ICache {
    virtual std::optional<std::string> get(std::string_view key) = 0;
};

// Proposed
class ICache {
    virtual std::expected<std::string, CacheError> get(std::string_view key) = 0;
};
```

**Benefits:**
- Better error information (not just "not found")
- Distinguish cache miss from cache error
- Composable error handling

---

### Modularize Interface Headers
**Priority:** Low  
**Target Version:** v1.8.0

Split monolithic headers into smaller, more focused headers.

**Current:**
```
concerns_context.h (includes everything)
```

**Proposed:**
```
concerns/logger_interface.h
concerns/tracer_interface.h
concerns/metrics_interface.h
concerns/cache_interface.h
concerns/context.h (lightweight aggregator)
```

**Benefits:**
- Faster compile times
- Reduced header dependencies
- Easier to maintain

---

### Add Concepts for Template Constraints
**Priority:** Medium  
**Target Version:** v1.7.0 (C++20 required)

Use C++20 concepts to constrain generic code.

```cpp
template<typename T>
concept Logger = requires(T logger, std::string_view msg) {
    { logger.info(msg) } -> std::same_as<void>;
    { logger.error(msg) } -> std::same_as<void>;
};

template<Logger L>
class GenericHandler {
    // Compiler-enforced logger interface
};
```

**Benefits:**
- Better error messages
- Compile-time interface validation
- Enables template specialization

---

## Known Issues

### Issue #1: Header Inclusion Order Sensitivity
**Severity:** Medium  
**Reported:** v1.5.0  
**Status:** ✅ Fixed in v1.6.0

All headers already carried `#pragma once`.  The only remaining sensitivity
was that `concerns_context.h` directly included `lifecycle.h` even though
each of the four interface headers (`i_logger.h`, `i_tracer.h`, `i_metrics.h`,
`i_cache.h`) already transitively pull it in.  The redundant direct include
has been removed; all headers are now fully self-contained and can be included
in any order.

---

### Issue #2: Virtual Destructor Performance
**Severity:** Low  
**Reported:** v1.5.1

Virtual destructors add overhead in tight loops.

**Workaround:** Use CRTP or static polymorphism for hot paths  
**Fix:** Document performance implications in header comments

**Planned Fix:** v1.6.1 (documentation only)

---

### Issue #3: Missing noexcept Specifications
**Severity:** Low  
**Reported:** v1.5.2

Many interface methods don't specify `noexcept`.

**Workaround:** Assume exceptions can be thrown  
**Fix:** Add `noexcept` specifications where appropriate

**Planned Fix:** v1.6.0

---

### Issue #4: Inconsistent Namespace Usage
**Severity:** Low  
**Reported:** v1.5.0

Some headers use `themis::` namespace, others use `themis::core::`.

**Workaround:** Check header documentation for correct namespace  
**Fix:** Standardize on `themis::core::` for all core interfaces

**Planned Fix:** v1.7.0 (breaking change, needs migration period)

---

## Research Areas

### Compile-Time Dependency Injection
**Focus:** Zero-overhead DI using template metaprogramming

Explore compile-time dependency injection:
- No vtable overhead
- Monomorphized code generation
- Template-based factory

**Research Questions:**
- Can we maintain flexibility with compile-time DI?
- How to balance compile time vs runtime cost?
- Can we support dynamic plugins?

---

### Lock-Free Interface Implementations
**Focus:** Wait-free concurrent data structures

Investigate lock-free implementations for:
- Metrics counters (atomic operations only)
- Cache updates (compare-and-swap)
- Log buffers (SPSC/MPSC queues)

**Research Questions:**
- What's the performance gain vs complexity?
- How to handle contention?
- Can we guarantee progress?

---

### Interface Versioning Strategy
**Focus:** ABI stability across versions

Develop strategy for:
- Adding new virtual methods without breaking ABI
- Interface evolution over time
- Deprecation without removal

**Research Questions:**
- Can we use pimpl idiom for stable ABI?
- How to version interfaces explicitly?
- What's the cost of ABI stability?

---

### Reflection and Introspection
**Focus:** Runtime interface discovery

Enable runtime interface inspection:
- List available methods
- Query method signatures
- Generate bindings for other languages

**Research Questions:**
- Can we use C++20 reflection (when available)?
- What's the overhead of reflection?
- How to maintain type safety?

---

## Migration Paths

### v1.5.x → v1.6.x: Async Interfaces
**Breaking Changes:** None (additive)

**New Interfaces:**
- `IAsyncLogger`
- `IAsyncCache`
- `IContext`

**Migration Steps:**
1. Update to v1.6.0
2. Existing code continues to work (no changes needed)
3. Optionally adopt async interfaces for new code

---

### v1.6.x → v1.7.x: Expected Error Handling
**Breaking Changes:** Cache interface signature change

**Old Interface:**
```cpp
virtual std::optional<std::string> get(std::string_view key) = 0;
```

**New Interface:**
```cpp
virtual std::expected<std::string, CacheError> get(std::string_view key) = 0;
```

**Migration Steps:**
1. Update all cache `get()` call sites to handle `expected`
2. Run provided migration script: `scripts/migrate_expected_v17.sh`
3. Rebuild and test

**Timeline:** 6 months deprecation period

---

### v1.7.x → v1.8.x: Namespace Standardization
**Breaking Changes:** Namespace changes from `themis::` to `themis::core::`

**Migration Steps:**
1. Search and replace `themis::ILogger` → `themis::core::ILogger`
2. Update all `using` declarations
3. Rebuild (compiler errors will guide remaining changes)

**Compatibility Shim:**
```cpp
// Provided in v1.7.x for compatibility
namespace themis {
    using core::ILogger;
    using core::ITracer;
    // ...
}
```

**Timeline:** 12 months deprecation period

---

### v1.8.x → v2.0.x: CRTP Refactor
**Breaking Changes:** Major API redesign for zero-overhead

**Old API:**
```cpp
class MyClass {
    ILogger* logger_;
    void doWork() {
        logger_->info("Working...");  // Virtual call
    }
};
```

**New API:**
```cpp
template<typename Logger>
class MyClass {
    Logger& logger_;
    void doWork() {
        logger_.info("Working...");  // Devirtualized call
    }
};
```

**Migration Steps:**
1. Convert classes to templates (automated tool provided)
2. Update factory methods to use CRTP
3. Extensive testing (behavior should be identical)

**Timeline:** 24 months deprecation period (v1.8.x maintained in parallel)

---

## Community Contributions Welcome

We welcome contributions in the following areas:

### High-Impact, Beginner-Friendly
- [x] Add missing Doxygen comments to interfaces (all pure-virtual methods in `ILogger`, `ITracer`, `IMetrics`, `ICache` now documented)
- [ ] Create example programs for each interface
- [x] Add `noexcept` specifications (done for lifecycle methods: `flush()`, `shutdown()`, `HealthStatus::isHealthy()`, `LatencyTimer::elapsedMs()`, `TraceContext::empty()`, and all NoOp implementations)
- [x] Fix include order dependencies (removed redundant `lifecycle.h` from `concerns_context.h`; all headers are now self-contained and order-independent)

### Medium Complexity
- [ ] Implement IContext interface
- [ ] Add type-safe metrics labels
- [ ] Create async interface variants
- [x] Health check interface (implemented in v1.6.0)

### Advanced Topics
- [ ] Compile-time DI framework
- [ ] Lock-free interface implementations
- [ ] CRTP refactor for zero overhead
- [ ] ABI stability framework

**Contribution Guide:** See [CONTRIBUTING.md](../../CONTRIBUTING.md)

---

## Feedback and Discussion

Have ideas for interface improvements? We'd love to hear from you:

- 💡 Feature requests: [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- 💬 Design discussions: [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- 📚 Documentation feedback: [Docs Issues](https://github.com/makr-code/ThemisDB/issues?label=documentation)

---

*Last Updated: February 2026*  
*Module Version: v1.5.x*  
*Next Review: v1.6.0 Release*
