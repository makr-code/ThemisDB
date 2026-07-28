> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-07-28 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Core Module - Future Enhancements

## Scope

- Central dependency injection (DI) context (`ConcernsContext`) for adapter ownership and resolution (storage, index, query, auth, logger, tracer, metrics, cache)
- Adapter lifecycle management: registration, validation, hot-swap, and graceful shutdown of adapters
- Runtime resilience controls around adapter dependencies (circuit breaker + fallback policy)
- Dynamic adapter reconfiguration and distributed cache integration without process restarts
- Unified observability wiring for logging, tracing, and metrics through the DI context

## 2026-07-28 Sync Snapshot (Issue #5638)

- [I] Plugin-based adapter loading (no recompile needed) remains tracked via Issue #1706
- [ ] Adapter plugin hardening and signing workflow remains targeted for Q4 2026
- [x] `AdapterRegistry::hotSwap()` drains in-flight refs within ≤ 100 ms (`kHotSwapTimeoutMs{100}`) (Implemented: 2026-07-28)
- [x] `ConcernsContext::resolve<T>()` uses `std::shared_mutex` reader-writer lock (no global lock contention) (Implemented: 2026-07-28)

---

## Design Constraints

- `[x]` Adapter hot-swap must complete in ≤ 100 ms and must not drop in-flight requests; callers hold a ref-counted handle (Implemented: 2026-07-28)
- `[x]` `ConcernsContext` must be fully thread-safe; concurrent adapter resolution must not require a global lock (Implemented: 2026-07-28)
- `[ ]` Circuit breaker state transitions (closed → open → half-open) must be observable via metrics and loggable at DEBUG level
- `[x]` No adapter may be registered without passing a synchronous `AdapterValidator::validate()` check; invalid adapters are rejected at registration time (Implemented: 2026-07-28)
- `[ ]` DI context construction must complete in ≤ 50 ms at server startup with up to 32 registered adapters
- `[x]` All adapter interfaces versioned with a `uint32_t` API version; version mismatch at registration returns a structured error (Implemented: 2026-07-28)
- `[ ]` Distributed cache adapter must not be a hard dependency; core must function correctly when no cache adapter is registered

---

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `ConcernsContext::resolve<T>()` | All modules | Returns shared adapter handle; thread-safe; ref-counted |
| `AdapterRegistry::registerAdapter(id, adapter, validator)` | Server startup / admin API | Validates before insertion |
| `AdapterRegistry::hotSwap(id, new_adapter)` | Admin API / config watcher | Drains in-flight refs before replacing |
| `CircuitBreaker::call(fn, fallback)` | Adapter call sites | Configurable failure threshold and reset timeout |
| `DistributedCache::get/set/invalidate(key)` | Query executor, analytics | Optional adapter; no-op stub when absent |
| `ObservabilityBus::emit(event)` | All adapters | Routes to logger/tracer/metrics based on event type |

---

## Implementation Notes

- Prioritize plugin runtime loading and signature verification as the next hardening block after the current compiled-adapter baseline.
- Preserve backwards-compatible concern interfaces while introducing adapter package signing and structured registration errors.
- Keep runtime swap and distributed cache improvements measurable via the test/performance constraints below.

---

## Planned Features

### Dynamic Adapter Reconfiguration
**Priority:** High
**Target Version:** v1.9.0

Harden runtime adapter switching for high-throughput production workloads.

```cpp
// Runtime adapter swap — no restart required
context->replaceLogger(std::make_unique<SpdlogLoggerAdapter>(...));
context->replaceMetrics(std::make_unique<PrometheusMetricsAdapter>(...));
```

**Planned hardening work:**
- Add swap-drain telemetry (duration, in-flight handles, error counts) per adapter type.
- Add rollback guardrails for failed replacement attempts under load.
- Add stress profile for repeated replace-cycles under mixed read/write traffic.

**Benefits:**
- Zero-downtime logging level changes
- Switch between tracing backends without restart
- Enable/disable metrics dynamically

---

### Distributed Cache Integration
**Priority:** High
**Target Version:** v1.9.0

Expand distributed caching toward multi-region and failure-domain-aware operation.

**Features:**
- Cluster-wide cache invalidation (via Redis pub/sub PUBLISH on DEL/clear)
- Consistent hashing (FNV-1a hash ring with virtual nodes) for key routing
- TTL support via Redis PSETEX (millisecond precision)
- Pub/sub for cache invalidation messages (background subscriber thread)
- Graceful degradation when Redis is unavailable (no exceptions, returns nullopt/false)

**Planned expansion:**
- Add multi-region keyspace strategy and region-local invalidation buffering.
- Add partition tolerance mode with deterministic stale-read policies.
- Add operator-facing cache health SLOs and alert thresholds.

**API:**
```cpp
auto redis_cache = RedisCache::create("redis://cluster:6379");
auto context = ConcernsContext::createCustom(
    logger, tracer, metrics, std::move(redis_cache)
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

## Performance Optimizations (Future)

### Zero-Copy Logging
**Priority:** High
**Target Version:** v1.9.0

Further reduce logging overhead for high-cardinality workloads.

**Current:** string_view hot path with thread-local format buffer
**Target:** bounded queue backpressure and adaptive flush policy by latency target

**Expected Improvement:** 30-50% reduction in logging overhead

---

### Lock-Free Metrics
**Priority:** High
**Target Version:** v1.9.0

Extend lock-free metrics pipeline for predictable p99 under burst load.

**Planned work:**
- Add bounded-memory histogram compaction mode.
- Add low-contention exporter fan-out for multi-sink metric backends.
- Add saturation metrics and adaptive flush interval.

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
  std::make_unique<SpdlogLoggerAdapter>(),
  std::make_unique<OpenTelemetryTracerAdapter>(),
  std::make_unique<PrometheusMetricsAdapter>(),
  std::make_unique<InMemoryCacheImpl>()
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

## Risk Backlog

### Issue #1: Cache Stampede
**Severity:** Medium
**Signal:** Duplicate compute after concurrent cache misses under burst traffic

Multiple threads simultaneously query cache miss, causing duplicate work.

**Workaround:** Use lock-based cache warming
**Fix:** Implement request coalescing in cache layer

**Planned Fix:** backlog (pending scheduling)

---

### Issue #2: Tracer Memory Leak (Edge Case)
**Severity:** Low
**Signal:** Long-running spans can accumulate if `end()` is not called

Long-running spans can accumulate if `end()` is not called.

**Workaround:** Use RAII span guards
**Fix:** Add automatic span timeout and cleanup

**Planned Fix:** backlog (pending scheduling)

---

### Issue #3: Metrics Label Cardinality Explosion
**Severity:** High
**Signal:** High-cardinality labels (e.g., user IDs) can cause unbounded memory growth

High-cardinality labels (e.g., user IDs) cause unbounded memory growth.

**Workaround:** Limit label values via configuration
**Fix:** Add automatic label cardinality limiting and warnings

**Planned Fix:** backlog (pending scheduling)

---

### Issue #4: Production Mode Detection False Positives
**Severity:** Low
**Signal:** Environment variable combinations can incorrectly trigger production mode

Environment variable detection can incorrectly trigger production mode.

**Workaround:** Explicitly set `THEMIS_PRODUCTION_MODE=0`
**Fix:** More robust production detection logic

**Planned Fix:** backlog (pending scheduling)

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

## Adoption Scenarios

### Scenario A: Runtime Adapter Hardening Rollout
**Breaking Changes:** None expected (additive)

**New APIs:**
```cpp
context->replaceLogger(new_logger);
context->replaceMetrics(new_metrics);
```

**Adoption Steps:**
1. Enable replacement telemetry in non-production environment.
2. Run swap stress profile with representative production traffic.
3. Promote with rollback guardrails enabled.

---

### Scenario B: Metrics API Refactor Preparation
**Breaking Changes:** anticipated API shape changes

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

**Adoption Steps:**
1. Inventory all callsites using legacy metric methods.
2. Prepare codemod/lint rule for builder-style calls.
3. Roll out per module behind compatibility switch.

---

### Scenario C: Modular Concerns Library Split
**Breaking Changes:** link configuration changes expected

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

**Adoption Steps:**
1. Introduce module-level target mapping for granular concerns libraries.
2. Validate binary size and startup deltas per build profile.
3. Deprecate monolithic link target after migration window.

---

## Community Contributions Welcome

We welcome contributions in the following areas:

### High-Impact, Beginner-Friendly
- [ ] Additional logger adapters (log4cpp, glog)
- [ ] More cache eviction strategies (FIFO, Random)
- [ ] Metrics exporter for other backends (InfluxDB, Datadog)
- [ ] Documentation improvements and examples

### Medium Complexity
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

*Last Updated: 2026-05-31*
*Review Cadence: monthly backlog review*

---

## Test Strategy

- **Unit tests** (≥ 90 % line coverage): `ConcernsContext::resolve<T>()` under concurrent access (≥ 16 threads); `AdapterRegistry` validation rejection paths; `CircuitBreaker` state machine (closed → open → half-open → closed)
- **Integration tests**: full server startup with all production adapters registered; hot-swap of logger and metrics adapters under load (1 000 req/s synthetic traffic); verify zero dropped requests during swap
- **Fault injection tests**: simulate adapter failures at rates 10 %, 50 %, 100 %; verify circuit breaker opens within the configured threshold (default: 5 consecutive failures) and closes after the reset timeout
- **Distributed cache tests** (Docker Compose Redis): cluster-wide cache invalidation propagates to all nodes within 500 ms; Redis failover handled gracefully with fallback to no-cache path
- **Property-based tests**: randomised adapter registration/deregistration sequences; `ConcernsContext` must never deadlock or return a dangling handle
- **CI coverage gate**: ≥ 88 % line coverage enforced; race detector (`-fsanitize=thread`) enabled in CI

## Performance Targets

- `ConcernsContext::resolve<T>()` under 32-thread contention: ≤ 1 µs median, ≤ 10 µs p99
- Adapter hot-swap end-to-end (register new + drain + replace): ≤ 100 ms
- Server startup with 32 adapters registered: DI context construction ≤ 50 ms
- Circuit breaker `call()` overhead (closed state, no failure): ≤ 200 ns per invocation
- Distributed cache `get` round-trip latency (Redis localhost): ≤ 1 ms p99
- ObservabilityBus `emit()` overhead (fire-and-forget async path): ≤ 500 ns per event

## Security / Reliability

- `AdapterRegistry` rejects adapters failing `AdapterValidator::validate()`; malformed or ABI-incompatible adapters never enter the live context
- Adapter API version checked at registration; version mismatch produces a structured error and is written to audit log
- `ConcernsContext` uses RAII ref-counted handles; no raw pointer sharing across module boundaries; dangling adapter access impossible by design
- Circuit breaker prevents cascading failures: when an adapter is open, requests use the configured fallback (error/stub) immediately without incurring full timeout latency
- Distributed cache keys namespaced per tenant to prevent cross-tenant cache poisoning
- All adapter lifecycle events (register, hot-swap, deregister, circuit-open, circuit-close) written to immutable audit log with timestamp and actor identity
