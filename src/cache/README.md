# Cache Module

Caching implementations for ThemisDB.

## Module Purpose

Implements multi-level adaptive query result caching for ThemisDB with semantic-aware lookups, circuit breaker fault isolation, tenant isolation, and configurable rate limiting.

## Subsystem Scope

**In scope:** L1 in-memory LRU cache, L2 compressed cache, L3 RocksDB-backed persistent cache, semantic fingerprinting, circuit breaker, rate limiting, tenant namespacing.

**Out of scope:** Query parsing (handled by query module), storage layer (handled by storage module), distributed cache coordination (planned).

## Relevant Interfaces

- `adaptive_query_cache.h/cpp` — main cache facade (L1/L2/L3 pipeline)
- `semantic_cache.cpp` — vector similarity fingerprinting
- `embedding_cache.cpp` — embedding result caching
- `bounded_lru_cache.h` — L1 LRU implementation

## Current Delivery Status

**Maturity:** 🟡 Beta — L1/L2/L3 pipeline, circuit breaker, and tenant isolation complete; Admin API and distributed coordination planned.

## Components

- Adaptive Query Cache (multi-level: L1/L2/L3)
- Semantic query cache
- Result set caching
- Cache invalidation strategies
- In-memory cache with LRU eviction

## Features

- Semantic-aware query result caching
- Vector similarity-based cache lookups
- Configurable cache size and TTL
- Automatic cache invalidation on data changes
- **Phase 1 Production Readiness (2025):**
  - Per-entry size limits and validation
  - Circuit breaker for RocksDB fault isolation
  - Enhanced metrics (hits/misses, errors, compression ratios)
  - Retry logic with exponential backoff
  - L3 pattern-based invalidation (iterator-based)
- **Phase 2 Hardening (2025):**
  - Configuration validation on startup
  - Token bucket rate limiting
  - Tenant isolation and namespace enforcement
  - Per-tenant size quotas
- **Phase 3 Operational Excellence (2026):**
  - Admin API for operations and monitoring
  - Cache warmup with bulk operations
  - Tenant management API
  - Health checks and diagnostics
  - Adaptive TTL tuning based on access patterns

## Phase 1 Production-Readiness Improvements

The cache module has been enhanced with critical production-readiness features:

### Size Limits & Resource Protection
- Configurable per-entry size limits (`max_total_entry_size`)
- Per-level size validation (L1: 1KB, L2: 10KB, L3: configurable)
- Automatic rejection of oversized payloads
- Metrics tracking for size limit rejections

### Circuit Breaker & Fault Isolation
- Circuit breaker pattern for RocksDB operations
- Three states: CLOSED (normal), OPEN (failing), HALF_OPEN (testing)
- Configurable failure threshold and timeout
- Automatic retry with exponential backoff on RocksDB initialization
- Graceful degradation when L3 is unavailable

### Enhanced Observability
- Structured metrics: `cache::CacheMetrics`
- Per-tier hit/miss tracking
- Error metrics (L3 read/write errors, compression failures)
- Compression ratio calculation
- Circuit breaker state tracking
- JSON export for monitoring systems

### L3 Pattern Invalidation
- Fixed skipped L3 invalidation in regex-based cache clearing
- Iterator-based pattern matching using RocksDB prefix scan
- Circuit breaker integration for invalidation operations
- Proper error handling and metrics

## Phase 2 Hardening Improvements

Additional hardening features for production deployment:

### Configuration Validation
- Comprehensive validation on startup with `Config::validate()`
- Constructor throws `std::invalid_argument` on invalid config
- Validates all parameters: size limits, TTLs, compression level, etc.
- Clear error messages for debugging

### Rate Limiting
- Token bucket algorithm for request rate limiting
- Thread-safe implementation with atomics
- Configurable rate limit and burst size
- Metrics tracking for rate-limited requests
- Opt-in via `enable_rate_limiting` config

### Tenant Isolation
- Tenant namespacing in fingerprints and cache keys
- Per-tenant size quotas with `per_tenant_max_bytes`
- Quota enforcement prevents one tenant from consuming all cache
- Tenant-scoped get/put operations
- Opt-in via `enable_tenant_isolation` config

## Configuration

```cpp
AdaptiveQueryCache::Config config;

// Size limits (Phase 1)
config.max_total_entry_size = 10485760;  // 10MB max per entry
config.enable_size_limits = true;

// Circuit breaker (Phase 1)
config.enable_circuit_breaker = true;
config.cb_failure_threshold = 5;   // Open after 5 failures
config.cb_timeout_ms = 60000;      // 1 minute timeout

// Rate limiting (Phase 2)
config.enable_rate_limiting = true;
config.max_requests_per_second = 10000;  // Global rate limit

// Tenant isolation (Phase 2)
config.enable_tenant_isolation = true;
config.per_tenant_max_bytes = 104857600;  // 100MB per tenant

// L1/L2/L3 configuration
config.l1_max_entries = 10000;
config.l2_max_entries = 50000;
config.l3_db_path = "./themis_query_cache";

// Validate config before use
std::string error;
if (!config.validate(&error)) {
    throw std::invalid_argument("Invalid config: " + error);
}
```

## Usage with Tenant Isolation

```cpp
AdaptiveQueryCache cache(config);

// Generate fingerprint with tenant ID
std::string fp = cache.generateFingerprint(
    "SELECT * FROM users WHERE id = ?",
    {{"id", 123}},
    "tenant_abc"  // Optional tenant ID
);

// Store result with tenant
cache.put(fp, params, result, "tenant_abc");

// Retrieve with tenant
auto cached = cache.get(fp, "tenant_abc");

// Different tenant cannot access the data
auto denied = cache.get(fp, "tenant_xyz");  // Returns nullopt
```

## Phase 3 Operational Excellence

Admin API for production operations and monitoring:

### Admin API Methods

**Per-Tier Statistics:**
```cpp
json stats = cache.getStatsByTier();
// Returns entries, utilization, and hits per tier (L1, L2, L3)
```

**Health Status:**
```cpp
json health = cache.getHealthStatus();
// Returns health status with warnings (high utilization, circuit breaker, low hit rate)
```

**Tenant Management:**
```cpp
// Get tenant usage
json tenant_stats = cache.getTenantStats();

// Invalidate all entries for a tenant
size_t count = cache.invalidateTenant("tenant_123");
```

**Cache Warmup:**
```cpp
// Bulk insert for startup warmup
std::vector<std::tuple<std::string, json, json, std::string>> entries;
// ... populate entries ...
size_t cached = cache.bulkPut(entries);
```

**Debug & Diagnostics:**
```cpp
// Export sample cache keys
auto keys = cache.exportKeys(100);
for (const auto& key : keys) {
    std::cout << key << std::endl;
}
```

### Adaptive TTL Tuning

Automatically adjusts TTL based on entry access patterns using logarithmic scaling:

```cpp
config.enable_adaptive_ttl = true;
config.adaptive_ttl_min_seconds = 60;     // 1 minute minimum
config.adaptive_ttl_max_seconds = 86400;  // 24 hours maximum
config.adaptive_ttl_scaling_factor = 5.0; // Growth rate
```

**Algorithm:**
- Formula: `adaptive_ttl = base_ttl * (1 + log(access_count + 1) / scaling_factor)`
- Bounds: Clamped to `[min_ttl, max_ttl]`
- New entries start at min TTL
- Each cache hit increments access_count and recalculates TTL
- Logarithmic scaling provides diminishing returns

**Benefits:**
- Popular queries stay cached longer automatically
- Unpopular queries expire quickly to free space
- No manual tuning required
- Reduces cache churn and database load

**Example TTL Growth:**
```
Access Count |  TTL (min=60s, factor=5.0)
-------------|---------------------------
     0       |  60s  (minimum)
     1       |  68s
     5       |  82s
    10       |  89s
    50       | 108s
  1000       | 143s
Max bounded  | 86400s (24 hours)
```

## Testing

Phase 1, 2 & 3 improvements include comprehensive test coverage:

```bash
# Run all cache tests (includes Phase 1, 2 & 3)
./build/tests/test_adaptive_cache_phase1

# Run integration tests
./build/tests/test_adaptive_cache_integration

# Run fuzz tests
./build/tests/test_adaptive_cache_fuzz

# Run all cache module tests
./build/tests/test_adaptive_query_cache
./build/tests/test_bounded_lru_cache
./build/tests/test_embedding_cache
./build/tests/test_semantic_cache
```

## Documentation

For detailed caching documentation, see:
- [Production Readiness Roadmap](../../docs/cache_roadmap.md)
- [Semantic Cache Implementation](../../docs/de/src/cache/semantic_cache.cpp.md)
- [Semantic Cache Feature Documentation](../../docs/de/features/features_semantic_cache.md)
- [Cache Invalidation Strategy](../../docs/de/architecture/architecture_cache_invalidation.md)
- [Core Concerns - Cache Strategies](../../include/core/concerns/CACHE_STRATEGIES_README.md)

## Scientific References

1. Megiddo, N., & Modha, D. S. (2003). **ARC: A Self-Tuning, Low Overhead Replacement Cache**. *Proceedings of the 2nd USENIX Conference on File and Storage Technologies (FAST)*, 115–130. https://www.usenix.org/legacy/events/fast03/tech/full_papers/megiddo/megiddo.pdf

2. Jiang, S., & Zhang, X. (2002). **LIRS: An Efficient Low Inter-Reference Recency Set Replacement Policy to Improve Buffer Cache Performance**. *ACM SIGMETRICS Performance Evaluation Review*, 30(1), 31–42. https://doi.org/10.1145/511399.511340

3. Einziger, G., & Friedman, R. (2014). **TinyLFU: A Highly Efficient Cache Admission Policy**. *Proceedings of the 22nd Euromicro International Conference on Parallel, Distributed, and Network-Based Processing*, 146–153. https://doi.org/10.1109/PDP.2014.34

4. King, W. F. (1971). **Analysis of Demand Paging Algorithms**. *IFIP Congress*, 485–490.

5. Mattson, R. L., Gecsei, J., Slutz, D. R., & Traiger, I. L. (1970). **Evaluation Techniques for Storage Hierarchies**. *IBM Systems Journal*, 9(2), 78–117. https://doi.org/10.1147/sj.92.0078
