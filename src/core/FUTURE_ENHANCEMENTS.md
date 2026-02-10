# Core Module - Future Enhancements

## Planned Features

### Dynamic Adapter Reconfiguration
**Priority:** High  
**Target Version:** v1.6.0

Enable runtime switching of adapters without restarting the database.

```cpp
// Future API
context->replaceLogger(new_logger_adapter);
context->reloadMetricsConfig(new_config);
```

**Benefits:**
- Zero-downtime logging level changes
- Switch between tracing backends without restart
- Enable/disable metrics dynamically

**Implementation Considerations:**
- Thread-safe adapter swapping
- Graceful handling of in-flight operations
- Configuration validation before swap

---

### Distributed Cache Integration
**Priority:** High  
**Target Version:** v1.6.0

Full Redis/Memcached adapter for distributed caching across cluster nodes.

**Features:**
- Cluster-wide cache invalidation
- Consistent hashing for distributed keys
- TTL support
- Pub/sub for cache invalidation messages

**API:**
```cpp
auto redis_cache = RedisCache::create("redis://cluster:6379");
auto context = ConcernsContext::createCustom(
    logger, tracer, metrics, redis_cache
);
```

**Use Cases:**
- Query result caching across nodes
- Session state management
- Distributed rate limiting state

---

### Contextual Logging
**Priority:** Medium  
**Target Version:** v1.7.0

Automatic context propagation through call chains for better log correlation.

```cpp
// Automatically include request_id in all logs
auto scoped_context = logger->withContext({
    {"request_id", "req-123"},
    {"user_id", "user-456"}
});

// All subsequent logs automatically include context
logger->info("Processing query");
// Output: [request_id=req-123, user_id=user-456] Processing query
```

**Benefits:**
- Easier log correlation
- Automatic structured logging
- Reduced boilerplate

---

### Metrics Aggregation Service
**Priority:** Medium  
**Target Version:** v1.7.0

Centralized metrics aggregation across sharded nodes.

**Features:**
- Aggregate counters/histograms from all nodes
- Push to central Prometheus/Grafana
- Automatic shard labeling
- Query-based metric filtering

---

### Adaptive Cache Strategies
**Priority:** Low  
**Target Version:** v1.8.0

Machine learning-based cache eviction that adapts to workload patterns.

**Approach:**
- Monitor hit/miss patterns
- Automatically switch between LRU/LIRS/ARC
- Predict hot data based on access patterns
- Adjust cache size dynamically

---

### Custom Concern Types
**Priority:** Low  
**Target Version:** v1.8.0

Allow users to register custom cross-cutting concerns.

```cpp
class ICustomConcern {
public:
    virtual void onRequest(const Request& req) = 0;
    virtual void onResponse(const Response& res) = 0;
};

context->registerConcern<ICustomConcern>(my_custom_concern);
```

---

## Performance Optimizations

### Zero-Copy Logging
**Priority:** High  
**Target Version:** v1.6.0

Reduce memory allocations in logging hot paths.

**Current:** String formatting and copying for every log call  
**Target:** Pre-allocated buffers and string_view usage

**Expected Improvement:** 30-50% reduction in logging overhead

---

### Lock-Free Metrics
**Priority:** High  
**Target Version:** v1.6.0

Replace mutex-based counters with atomic operations.

**Implementation:**
- `std::atomic` for counters
- Lock-free ring buffer for histograms
- Thread-local aggregation with periodic flush

**Expected Improvement:** 80% reduction in metric update latency

---

### Span Pool Reuse
**Priority:** Medium  
**Target Version:** v1.7.0

Reuse span objects instead of allocating on every trace.

**Current:** Allocate new span for every operation  
**Target:** Object pool with 1000 pre-allocated spans

**Expected Improvement:** 60% reduction in tracing overhead

---

### Lazy Context Initialization
**Priority:** Medium  
**Target Version:** v1.7.0

Defer adapter creation until first use.

**Benefits:**
- Faster startup time
- Lower memory footprint for unused concerns
- Pay-for-what-you-use model

---

### Batched Metrics Export
**Priority:** Low  
**Target Version:** v1.8.0

Batch multiple metric updates before sending to Prometheus.

**Current:** Export every metric update immediately  
**Target:** Buffer updates and export every 100ms

**Expected Improvement:** 90% reduction in network overhead

---

## Refactoring Opportunities

### Separate Concerns into Individual Libraries
**Priority:** Medium  
**Target Version:** v1.7.0

Split concerns into standalone libraries for better modularity.

```
libthemis-logging.so      (ILogger + adapters)
libthemis-tracing.so      (ITracer + adapters)
libthemis-metrics.so      (IMetrics + adapters)
libthemis-caching.so      (ICache + implementations)
```

**Benefits:**
- Independent versioning
- Reduced binary size for minimal builds
- Easier testing and maintenance

---

### Move Cache Strategies to Plugin System
**Priority:** Low  
**Target Version:** v1.8.0

Allow custom cache eviction strategies via plugin API.

**Benefits:**
- User-defined eviction policies
- A/B testing of strategies
- Domain-specific optimization

---

### Simplify ConcernsContext API
**Priority:** Low  
**Target Version:** v1.9.0

Reduce boilerplate in context creation.

```cpp
// Current
auto context = ConcernsContext::createCustom(
    std::make_shared<SpdlogLogger>(),
    std::make_shared<OtelTracer>(),
    std::make_shared<PrometheusMetrics>(),
    std::make_shared<InMemoryCache>()
);

// Proposed
auto context = ConcernsContextBuilder()
    .withLogger<SpdlogLogger>()
    .withTracer<OtelTracer>()
    .withMetrics<PrometheusMetrics>()
    .withCache<InMemoryCache>()
    .build();
```

---

### Standardize Error Handling
**Priority:** Medium  
**Target Version:** v1.7.0

Use `Expected<T, Error>` consistently across all concern interfaces.

**Current:** Mix of exceptions, optionals, and error codes  
**Target:** Uniform `Result<T>` return type

---

## Known Issues

### Issue #1: Cache Stampede
**Severity:** Medium  
**Reported:** v1.5.0

Multiple threads simultaneously query cache miss, causing duplicate work.

**Workaround:** Use lock-based cache warming  
**Fix:** Implement request coalescing in cache layer

**Planned Fix:** v1.6.0

---

### Issue #2: Tracer Memory Leak (Edge Case)
**Severity:** Low  
**Reported:** v1.5.1

Long-running spans can accumulate if `end()` is not called.

**Workaround:** Use RAII span guards  
**Fix:** Add automatic span timeout and cleanup

**Planned Fix:** v1.6.1

---

### Issue #3: Metrics Label Cardinality Explosion
**Severity:** High  
**Reported:** v1.5.0

High-cardinality labels (e.g., user IDs) cause unbounded memory growth.

**Workaround:** Limit label values via configuration  
**Fix:** Add automatic label cardinality limiting and warnings

**Planned Fix:** v1.6.0

---

### Issue #4: Production Mode Detection False Positives
**Severity:** Low  
**Reported:** v1.5.2

Environment variable detection can incorrectly trigger production mode.

**Workaround:** Explicitly set `THEMIS_PRODUCTION_MODE=0`  
**Fix:** More robust production detection logic

**Planned Fix:** v1.6.0

---

## Research Areas

### Observability-Driven Optimization
**Focus:** Automatic performance tuning based on metrics

Use collected metrics to:
- Automatically tune cache sizes
- Adjust thread pool sizes
- Predict query hotspots
- Optimize index selection

**Research Questions:**
- Which metrics best correlate with performance?
- Can we use reinforcement learning for auto-tuning?
- How to avoid oscillation in adaptive systems?

---

### Privacy-Preserving Logging
**Focus:** Secure logging without PII exposure

**Approaches:**
- Automatic PII detection and redaction
- Differential privacy for aggregate metrics
- Encrypted logging with key rotation
- Secure multi-party computation for log analysis

**Research Questions:**
- How to balance debuggability with privacy?
- Can we detect PII with high accuracy?
- What's the performance cost of encrypted logging?

---

### Predictive Caching
**Focus:** ML-based cache prediction

Use query patterns to:
- Pre-fetch likely future queries
- Identify cold data for eviction
- Predict query result sizes
- Optimize cache partitioning

**Research Questions:**
- Which ML models best predict cache behavior?
- Can we do online learning without overhead?
- How to handle concept drift in workloads?

---

### Cross-Platform Tracing
**Focus:** Unified tracing across languages/platforms

Enable tracing from:
- C++ core engine
- Python client SDKs
- JavaScript web clients
- Mobile applications

**Research Questions:**
- How to propagate context across boundaries?
- Can we standardize trace formats?
- What's the overhead of polyglot tracing?

---

## Migration Paths

### v1.5.x → v1.6.x: Dynamic Adapter API
**Breaking Changes:** None (additive)

**New APIs:**
```cpp
context->replaceLogger(new_logger);
context->reloadConfig(new_config);
```

**Migration Steps:**
1. Update to v1.6.0
2. Test existing code (no changes needed)
3. Optionally adopt new dynamic APIs

---

### v1.6.x → v1.7.x: Metrics API Refactor
**Breaking Changes:** Metrics signature changes

**Old API:**
```cpp
metrics->incrementCounter("counter_name");
metrics->recordHistogram("histogram_name", value);
```

**New API:**
```cpp
metrics->counter("counter_name").increment();
metrics->histogram("histogram_name").record(value);
```

**Migration Steps:**
1. Update all `metrics->` calls to new builder-style API
2. Run provided migration script: `scripts/migrate_metrics_v17.sh`
3. Rebuild and test

**Timeline:** 6 months deprecation period

---

### v1.7.x → v1.8.x: Modular Concerns Libraries
**Breaking Changes:** Link flags changes

**Old CMake:**
```cmake
target_link_libraries(my_app themis-core)
```

**New CMake:**
```cmake
target_link_libraries(my_app 
    themis-logging 
    themis-tracing 
    themis-metrics 
    themis-caching
)
```

**Migration Steps:**
1. Update CMakeLists.txt with granular libraries
2. Remove unnecessary dependencies for smaller binary size
3. Rebuild

**Timeline:** 12 months deprecation period (v1.7.x still provides monolithic library)

---

## Community Contributions Welcome

We welcome contributions in the following areas:

### High-Impact, Beginner-Friendly
- [ ] Additional logger adapters (log4cpp, glog)
- [ ] More cache eviction strategies (FIFO, Random)
- [ ] Metrics exporter for other backends (InfluxDB, Datadog)
- [ ] Documentation improvements and examples

### Medium Complexity
- [ ] Redis cache adapter implementation
- [ ] Contextual logging framework
- [ ] Span pool for tracer optimization
- [ ] Configuration hot-reload

### Advanced Topics
- [ ] Lock-free metrics implementation
- [ ] Distributed tracing correlation
- [ ] ML-based cache prediction
- [ ] Privacy-preserving logging

**Contribution Guide:** See [CONTRIBUTING.md](../../CONTRIBUTING.md)

---

## Feedback and Discussion

Have ideas for core module improvements? Open an issue or discussion:

- 💡 Feature requests: [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- 💬 Design discussions: [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- 🐛 Bug reports: [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)

---

*Last Updated: February 2026*  
*Module Version: v1.5.x*  
*Next Review: v1.6.0 Release*
