# Core Headers - Future Enhancements

## Scope

- API-level enhancements to `include/core/` headers — adapter lifecycle hooks for `IAdapter<T>` implementations
- Circuit breaker interface (`ICircuitBreaker`) for dependency fault isolation with thread-safe state transitions
- Configuration hot-reload API with async change-notification callbacks (`IConfigReloadListener`)
- Service health check interface extensions: liveness, readiness, and per-dependency health probes
- `resolve<T>()` performance improvements: non-blocking contract enforcement and optional CRTP devirtualization path
- Namespace standardization (`themis::core::`) and `std::expected`-based error handling adoption

## Design Constraints

- [ ] All new interfaces follow the existing `IAdapter<T>` contract; adapter lifecycle must not be bypassed
- [ ] `resolve<T>()` must remain non-blocking on the hot path; no I/O or mutex acquisition permitted inside `resolve<T>()`
- [ ] Circuit breaker state transitions (`CLOSED`→`OPEN`→`HALF_OPEN`) must use atomic operations; no locks on the fast path
- [ ] Adapter registration is restricted to the `ThemisContext::initialize()` phase; post-init registration returns a typed error
- [ ] New virtual methods added only at end of vtable to preserve ABI within a major version
- [ ] Configuration hot-reload callbacks delivered asynchronously; must not block the thread calling `resolve<T>()`

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `IAdapter<T>` (lifecycle hooks) | All adapter implementations | `onStart()`, `onStop()`, `onHealthCheck()` |
| `ICircuitBreaker` | Dependency fault isolation wrappers | Atomic state machine; ≤ 100 ns state check |
| `IConfigReloadListener` | Hot-reload consumers | Async callback; non-blocking delivery |
| `IHealthCheckProvider` | Liveness / readiness probe handlers | Extends existing `ProbeResult` / `HealthStatus` |
| `ContextScope` / `ContextPropagation` | Async boundary consumers | Implemented in v1.7.0; interface stable |
| `MetricLabels` (fluent builder) | Metrics instrumentation | Implemented in v1.6.0; interface stable |

## Planned Features

### IContext Interface for Contextual Logging
**Priority:** High
**Status:** ✅ Implemented in v1.6.0 (`include/core/concerns/i_context.h`)

General context propagation for request/call-chain metadata.  Key features:

- **`IContext`** — abstract interface: `set()`, `get()`, `has()`, `createChild()`, `toTraceContext()`
- **`SimpleContext`** — thread-safe, `enable_shared_from_this`-based concrete implementation
- **`context_keys::k*`** — 7 `constexpr string_view` constants (`kTraceId`, `kRequestId`, `kUserId`, `kTenantId`, `kOperation`, `kService`, `kSessionId`)
- **`toTraceContext()`** bridge — converts the context to `TraceContext` for direct use with `ILogger::logWithContext()`
- Parent/child chain: children inherit all parent attributes; writes are always local to the child

```cpp
// Root context
auto ctx = SimpleContext::create("trace-abc", "req-42");
ctx->set(context_keys::kUserId, "u-7");

// Sub-operation child
auto child = ctx->createChild();
child->set(context_keys::kOperation, "db.query");

// Structured log with full correlation context
logger.logWithContext(ILogger::Level::INFO, "query done",
                      child->toTraceContext(), {{"rows", "5"}});
```

`ILogger` is unchanged — `toTraceContext()` bridges to the existing
`logWithContext()` API with no interface modifications.



### Async Interfaces
**Priority:** High
**Status:** ✅ Implemented in v1.6.0 (`include/core/concerns/i_async_logger.h`, `include/core/concerns/i_async_cache.h`)

`IAsyncLogger` and `IAsyncCache` extend their synchronous counterparts with
`std::future`-returning methods for non-blocking dispatch on hot paths.
Default implementations wrap the synchronous methods with `std::async` so
existing `ILogger`/`ICache` implementations gain async variants for free.
`NoOpAsyncLogger` and `NoOpAsyncCache` use deferred futures (no real threads)
for fast, zero-overhead testing.

```cpp
// IAsyncLogger — fire-and-forget or awaitable
IAsyncLogger& logger = ...;
logger.infoAsync("Request received");          // fire-and-forget
auto f = logger.errorAsync("Fatal error");
f.get();                                       // await completion

// IAsyncCache — non-blocking get + write
IAsyncCache& cache = ...;
auto fut = cache.getAsync("user:42");
// ... do other work while I/O is in-flight ...
auto entry = fut.get();
if (entry) { /* cache hit */ }
cache.putAsync("user:42", updated_entry, 30000);
```

Any class that implements `ILogger` can opt in to async dispatch simply by
inheriting from `IAsyncLogger` and implementing the same sync pure-virtuals
— no additional overrides required.



### Context Propagation Across Async Boundaries
**Priority:** High
**Status:** ✅ Implemented in v1.7.0 (`include/core/concerns/context_propagation.h`)

`ContextPropagation` and `ContextScope` enable automatic propagation of the
active `IContext` into `std::async`/thread tasks without manual parameter
passing.  The mechanism uses a `thread_local` slot so each thread owns its
own current-context pointer.

- **`ContextScope`** — RAII guard; installs an `IContextPtr` as the current
  thread context and atomically restores the previous one on destruction
  (exception-safe, nestable)
- **`ContextPropagation::current()`** — returns the active context for the
  calling thread, or `nullptr` if none installed
- **`ContextPropagation::propagate(fn)`** — captures the current context,
  creates a **child** context (so writes inside the task don't affect the
  parent), launches `fn` via `std::async(launch::async)`, and installs the
  child as `current()` for the task's lifetime

```cpp
// At request entry point
auto ctx = SimpleContext::create("trace-abc", "req-42");
ContextScope scope(ctx);   // installs on calling thread

// Spawn async work — trace context propagates automatically
auto fut = ContextPropagation::propagate([] {
    auto c = ContextPropagation::current();      // child of `ctx`
    c->get(context_keys::kTraceId);              // "trace-abc"
    c->set(context_keys::kOperation, "db.query"); // local to this task
});
fut.get();

// Parent context unmodified — kOperation is NOT set on `ctx`
```

Existing `IContext` / `SimpleContext` code is unchanged — no breaking changes.
Thread-safety guarantees of `SimpleContext` are preserved since each thread
writes to its own child context.


### Type-Safe Metrics Labels
**Priority:** Medium
**Status:** ✅ Implemented in v1.6.0 (`include/core/concerns/metric_labels.h`)

`MetricLabels` is a fluent builder that converts implicitly to `IMetrics::Labels`
so it can be passed to any existing `IMetrics` method without changes at the
call site.  Predefined label-name constants in the `labels::` namespace prevent
typos at compile time.

```cpp
// Inline (ad-hoc labels)
metrics.incrementCounter("http_requests_total", 1,
    MetricLabels()
        .add(labels::kMethod,   "GET")
        .add(labels::kStatus,   "200")
        .add(labels::kEndpoint, "/api/v1/query"));

// Domain-specific labels struct
struct HttpRequestLabels {
    std::string method;
    std::string status;

    IMetrics::Labels toMetricLabels() const {
        return MetricLabels()
            .add(labels::kMethod, method)
            .add(labels::kStatus, status);
    }
};
```

`IMetrics::Labels` (`std::map<std::string, std::string>`) is unchanged — all
existing code compiles without modification.

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
**Status:** ✅ Implemented for Metrics (v1.6.0)

Lock-free implementations delivered:
- **Metrics counters**: `std::atomic<int64_t>` (see `LockFreeMetrics`)
- **Histogram buffers**: per-thread SPSC lock-free ring buffers
- **Thread-local aggregation**: background flush thread, 100 ms interval

Still under investigation:
- Cache updates (compare-and-swap)
- Log buffers (SPSC/MPSC queues)

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
- [x] Create example programs for each interface (`examples/concerns_example.cpp` — covers `ILogger`, `ITracer`, `IMetrics`, `ICache`, `IContext`, `MetricLabels`, lifecycle hooks, and `ConcernsContext` aggregation)
- [x] Add `noexcept` specifications (done for lifecycle methods: `flush()`, `shutdown()`, `HealthStatus::isHealthy()`, `LatencyTimer::elapsedMs()`, `TraceContext::empty()`, and all NoOp implementations)
- [x] Fix include order dependencies (removed redundant `lifecycle.h` from `concerns_context.h`; all headers are now self-contained and order-independent)

### Medium Complexity
- [x] Implement IContext interface (`SimpleContext` + `context_keys::k*` in `include/core/concerns/i_context.h`; bridges to `logWithContext()` via `toTraceContext()`)
- [x] Add type-safe metrics labels (`MetricLabels` fluent builder + `labels::k*` constants in `include/core/concerns/metric_labels.h`; implicitly converts to `IMetrics::Labels` so existing code is unchanged)
- [x] Create async interface variants (`IAsyncLogger` and `IAsyncCache` in `i_async_logger.h`/`i_async_cache.h`; default impls wrap sync methods with `std::async`; NoOp variants use deferred futures)
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

## Test Strategy

- Unit tests for each new interface: verify lifecycle hook invocation order and thread-safety guarantees
- Circuit breaker tests: verify `CLOSED`→`OPEN`→`HALF_OPEN`→`CLOSED` transitions under simulated failure injection
- Configuration hot-reload tests: verify callbacks are delivered asynchronously without blocking `resolve<T>()`
- `resolve<T>()` performance regression tests: assert median latency ≤ 1 µs with no lock contention under load
- Circuit breaker state check regression tests: assert ≤ 100 ns via atomic read (verified under concurrent load)
- Adapter registration security tests: verify that registration attempted after `ThemisContext::initialize()` returns a typed error

## Performance Targets

- `resolve<T>()` median latency (warm cache, no contention): ≤ 1 µs
- Circuit breaker state check (`isOpen()` / `isClosed()`): ≤ 100 ns via single atomic load
- Adapter lifecycle hook (`onStart()` / `onStop()`) invocation overhead (framework side): ≤ 5 µs excluding user code
- Configuration hot-reload notification delivery (change detected → callback invoked): ≤ 10 ms
- `ContextScope` RAII install / restore overhead: ≤ 50 ns (thread-local pointer swap)
- `MetricLabels::add()` per-label overhead: ≤ 100 ns (map insert, small label count ≤ 8)

## Security / Reliability

- Adapter registration restricted to `ThemisContext::initialize()` phase; post-init attempts return a typed error, never silently succeed
- No runtime injection from untrusted sources: adapter factories validate type tokens before any registration is accepted
- Circuit breaker state protected by atomic operations; no data races under concurrent access (verified by TSAN)
- Configuration hot-reload callbacks must not expose raw configuration values to callers without proper authorization checks
- `ContextPropagation::propagate()` creates child contexts; writes inside async tasks cannot corrupt the parent context
- Health check interface must not expose internal error details to unauthenticated liveness / readiness probe consumers

*Last Updated: April 2026*
*Module Version: v1.5.x*
*Next Review: v1.6.0 Release*
