# Cache Module

Caching implementations for ThemisDB.

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

## Testing

Phase 1 & 2 improvements include comprehensive test coverage:

```bash
# Run Phase 1 & 2 tests
./build/tests/test_adaptive_cache_phase1

# Run all cache tests
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
