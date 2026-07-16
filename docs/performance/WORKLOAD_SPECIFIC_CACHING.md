# Workload-Specific Query Caching Guide

## Overview

ThemisDB's workload-specific query caching automatically adapts caching strategies based on query patterns to maximize performance and resource efficiency. This guide explains how to configure and optimize query caching for different workload types.

## Table of Contents

- [Workload Types](#workload-types)
- [Quick Start](#quick-start)
- [Configuration Guide](#configuration-guide)
- [Performance Tuning](#performance-tuning)
- [Monitoring and Troubleshooting](#monitoring-and-troubleshooting)
- [Best Practices](#best-practices)
- [Integration Examples](#integration-examples)

---

## Workload Types

### OLTP (Online Transaction Processing)

**Characteristics:**
- High frequency: >10 queries per minute
- Small results: <50KB per query
- Short execution time: <100ms
- Predictable patterns

**Optimal Configuration:**
- Cache type: Basic (single-level)
- Eviction: LRU (Least Recently Used)
- TTL: Short (5-10 minutes)
- Memory: 200-500MB
- Max entries: 50,000+

**Use Cases:**
- E-commerce transactions
- User authentication
- Session management
- Real-time inventory checks
- Order processing

**Performance Gains:**
- 70-85% cache hit rate
- 2-4x throughput improvement
- <1ms cache lookup latency
- 40-60% CPU reduction

### OLAP (Online Analytical Processing)

**Characteristics:**
- Low frequency: <1 query per minute
- Large results: >1MB per query
- Long execution time: >1 second
- Complex aggregations

**Optimal Configuration:**
- Cache type: Adaptive (3-tier)
- Eviction: LFU (Least Frequently Used)
- TTL: Long (1-24 hours)
- Memory: 500MB-2GB
- Max entries: 5,000-10,000

**Use Cases:**
- Business intelligence reports
- Dashboard analytics
- Data warehouse queries
- Machine learning feature extraction
- Historical data analysis

**Performance Gains:**
- 40-60% cache hit rate
- 10-50x throughput improvement
- <10ms cache lookup latency
- 80-95% compute time reduction

### MIXED (Hybrid Workload)

**Characteristics:**
- Variable frequency: 0.5-10 queries per minute
- Mixed result sizes: 10KB-2MB
- Variable execution time: 10ms-10s
- Dynamic patterns

**Optimal Configuration:**
- Cache type: Adaptive (3-tier)
- Eviction: LRU with priority
- TTL: Adaptive (1 minute-12 hours)
- Memory: 300MB-1GB
- Max entries: 20,000

**Use Cases:**
- SaaS platforms
- Web applications
- API services
- Multi-tenant systems
- General-purpose databases

**Performance Gains:**
- 60-75% cache hit rate
- 2-5x average throughput improvement
- Adaptive latency (<1ms to <10ms)
- 50-70% resource reduction

### STREAMING (Real-Time)

**Characteristics:**
- Very high frequency: >50 queries per minute
- Very small results: <10KB
- Real-time data
- Short data validity

**Optimal Configuration:**
- Cache type: Basic (minimal)
- Eviction: LRU (aggressive)
- TTL: Very short (10-60 seconds)
- Memory: 10-50MB
- Max entries: 1,000-5,000

**Use Cases:**
- IoT data ingestion
- Real-time monitoring
- Live dashboards
- Event streaming
- Time-series data

**Performance Gains:**
- 20-40% cache hit rate
- 1.5-2x throughput improvement
- Sub-millisecond latency
- Minimal overhead

---

## Quick Start

### 1. Choose Configuration Template

Copy the appropriate template to your config directory:

```bash
# For OLTP workloads
cp config/query_cache_oltp.yaml config/query_cache.yaml

# For OLAP workloads
cp config/query_cache_olap.yaml config/query_cache.yaml

# For Mixed workloads (recommended for most use cases)
cp config/query_cache_mixed.yaml config/query_cache.yaml
```

### 2. Enable Query Caching

Add to your `config.yaml`:

```yaml
query_cache:
  enabled: true
  type: "adaptive"  # or "basic"
  
  # Enable automatic workload detection
  workload_detection:
    enabled: true
    sample_rate: 0.1
```

### 3. Start ThemisDB

```bash
themisdb --config config.yaml
```

The cache will automatically:
- Detect your workload pattern
- Optimize cache strategy
- Report statistics

### 4. Monitor Cache Performance

```bash
# Check cache statistics
curl http://localhost:8080/api/v1/cache/stats

# Get detailed monitoring info
curl http://localhost:8080/api/v1/cache/monitoring
```

---

## Configuration Guide

### Basic vs. Adaptive Cache

**Basic Cache:**
- Single-level in-memory cache
- LRU or LFU eviction
- Simple configuration
- Lower memory overhead
- Best for: OLTP, Streaming

**Adaptive Cache:**
- 3-tier architecture (HOT/WARM/COLD)
- Automatic level promotion/demotion
- Compressed storage (L2)
- Persistent cache (L3, RocksDB)
- Best for: OLAP, Mixed

### Workload Detection Configuration

```yaml
workload_detection:
  # Enable automatic detection
  enabled: true
  
  # Sample rate (0.0-1.0)
  # Higher = more accurate, more overhead
  sample_rate: 0.1
  
  # Minimum samples before classification
  min_samples: 100
  
  # Re-detection interval
  detection_window_sec: 300
  
  # Thresholds for classification
  oltp_frequency_threshold: 10.0      # queries/min
  olap_frequency_threshold: 0.5       # queries/min
  oltp_result_threshold_bytes: 51200  # 50KB
  olap_result_threshold_bytes: 1048576 # 1MB
```

### TTL (Time-To-Live) Configuration

```yaml
ttl:
  # Default TTL for all queries
  default_seconds: 1800
  
  # Minimum and maximum TTL
  min_seconds: 60
  max_seconds: 86400
  
  # Adaptive TTL based on query frequency
  adaptive: true
  
  # TTL multipliers by workload type
  ttl_multipliers:
    oltp: 0.5   # 50% of default
    olap: 2.0   # 200% of default
```

### Memory Configuration

```yaml
sizing:
  # Maximum number of cached queries
  max_entries: 20000
  
  # Total memory limit
  max_memory_bytes: 314572800  # 300MB
  
  # Maximum size per entry
  max_entry_size_bytes: 10485760  # 10MB
  
  # Memory pressure threshold
  memory_pressure_threshold: 0.9  # 90%
```

### Cache Warming

```yaml
cache_warming:
  # Enable warming on startup
  enabled: true
  
  # Number of hot queries to warm
  top_k_queries: 100
  
  # Maximum time for warming
  warmup_timeout_sec: 120
  
  # Warming strategy
  strategy: "hybrid"  # "frequent" | "expensive" | "hybrid"
  
  # Restore from persistent cache (L3)
  from_persistent: true
```

---

## Performance Tuning

### OLTP Optimization

**Problem: Low cache hit rate (<60%)**
```yaml
# Increase cache size
max_entries: 100000
max_memory_bytes: 524288000  # 500MB

# Shorter TTL for fresher data
ttl:
  default_seconds: 180  # 3 minutes
```

**Problem: High memory usage**
```yaml
# Reduce max entry size
max_entry_size_bytes: 51200  # 50KB

# More aggressive eviction
memory_pressure_threshold: 0.8  # 80%
```

**Problem: Stale data in cache**
```yaml
# Enable immediate invalidation
invalidation:
  by_table: true
  batch_mode: false  # Immediate invalidation
```

### OLAP Optimization

**Problem: Cache misses on expensive queries**
```yaml
# Increase L3 cache TTL
adaptive:
  l3:
    ttl_seconds: 172800  # 48 hours
    
# Enable persistent cache
cache_warming:
  from_persistent: true
```

**Problem: Large memory footprint**
```yaml
# Increase compression
adaptive:
  l2:
    compression_level: 9  # Max compression
    
  l3:
    enable_compression: true
```

**Problem: Slow cache lookups**
```yaml
# Optimize L1 for aggregates
adaptive:
  l1:
    max_entry_size_bytes: 20480  # 20KB for aggregated results
```

### Mixed Workload Optimization

**Problem: Suboptimal for either OLTP or OLAP**
```yaml
# Enable separate pools
mixed:
  separate_pools:
    enabled: true
    oltp_pool_ratio: 0.7  # Adjust based on workload mix
    olap_pool_ratio: 0.3
```

**Problem: Workload detection inaccurate**
```yaml
# Increase sampling
workload_detection:
  sample_rate: 0.2  # 20%
  min_samples: 200
  
  # Adjust thresholds based on your data
  oltp_frequency_threshold: 8.0
  olap_frequency_threshold: 0.8
```

---

## Monitoring and Troubleshooting

### Monitoring Endpoints

```bash
# Overall cache statistics
GET /api/v1/cache/stats

# Detailed monitoring info
GET /api/v1/cache/monitoring

# Hot queries for warming
GET /api/v1/cache/hot-queries?limit=50

# Current workload type
GET /api/v1/cache/workload
```

### Key Metrics to Monitor

**Cache Performance:**
- Hit rate: Target >60% for OLTP, >40% for OLAP
- Lookup latency: <1ms for L1, <10ms for L2/L3
- Memory utilization: 60-85% (avoid 100%)
- Eviction rate: <10% of total requests

**Workload Classification:**
- Detected workload type
- Query frequency distribution
- Result size distribution
- Execution time distribution

**Resource Usage:**
- Current memory usage
- Cache entry count
- Eviction count
- Invalidation count

### Troubleshooting Guide

**Problem: Cache not enabled**
```
Check: Is query_cache.enabled: true?
Check: Is cache integrated in QueryEngine?
Log: Search for "QueryCacheManager initialized"
```

**Problem: Zero cache hits**
```
Check: Are queries identical (same fingerprint)?
Check: Is TTL too short?
Check: Are queries being invalidated too frequently?
Debug: Enable detailed stats and check cache stores
```

**Problem: High memory usage**
```
Solution 1: Reduce max_entries
Solution 2: Reduce max_entry_size_bytes
Solution 3: Lower memory_pressure_threshold
Solution 4: Increase eviction frequency
```

**Problem: Workload misclassification**
```
Solution 1: Increase sample_rate
Solution 2: Increase min_samples
Solution 3: Adjust classification thresholds
Solution 4: Manual workload type override
```

**Problem: Poor cache performance**
```
Analyze: Check hit rate by workload type
Analyze: Check average query execution time
Analyze: Compare cached vs. uncached performance
Tune: Adjust TTL, eviction policy, or cache size
```

---

## Best Practices

### 1. Start with Mixed Workload Configuration

Unless you have a pure OLTP or OLAP workload, start with mixed configuration and enable auto-detection.

### 2. Enable Workload Detection

Always enable workload detection for automatic optimization:

```yaml
workload_detection:
  enabled: true
  sample_rate: 0.1
```

### 3. Monitor and Adjust

Monitor cache statistics regularly and adjust configuration:

```bash
# Set up periodic monitoring
*/5 * * * * curl http://localhost:8080/api/v1/cache/stats >> /var/log/themisdb/cache-stats.log
```

### 4. Use Cache Warming

Enable cache warming for faster startup:

```yaml
cache_warming:
  enabled: true
  top_k_queries: 100
```

### 5. Implement Cache Invalidation

Invalidate cache when data changes:

```python
# Python example
db.query_cache.invalidate_by_dependency("users")
```

### 6. Set Appropriate TTLs

- OLTP: 5-15 minutes
- OLAP: 1-24 hours
- Mixed: 30 minutes-2 hours

### 7. Size Cache Appropriately

General guideline:
- OLTP: 200-500MB (50K+ entries)
- OLAP: 500MB-2GB (5K-10K entries)
- Mixed: 300MB-1GB (20K entries)

### 8. Use Compression for Large Results

```yaml
adaptive:
  l2:
    compression_level: 3  # Good balance of speed/compression
```

### 9. Configure Memory Pressure Handling

```yaml
memory_pressure_threshold: 0.9  # Evict before running out of memory
```

### 10. Enable Detailed Statistics

```yaml
monitoring:
  enabled: true
  detailed_stats: true
  per_workload_stats: true
```

---

## Integration Examples

### Example 1: E-Commerce Application (OLTP)

```yaml
query_cache:
  enabled: true
  type: "basic"
  
  sizing:
    max_entries: 100000
    max_memory_bytes: 524288000  # 500MB
  
  ttl:
    default_seconds: 300  # 5 minutes
    adaptive: true
  
  eviction_policy: "LRU"
  
  cache_warming:
    enabled: true
    strategy: "frequent"
```

**Expected Performance:**
- Product listing: 85% cache hit
- User session: 90% cache hit
- Cart operations: 75% cache hit
- Average latency reduction: 60%

### Example 2: Business Intelligence Platform (OLAP)

```yaml
query_cache:
  enabled: true
  type: "adaptive"
  
  adaptive:
    l1:
      max_entries: 500
      ttl_seconds: 3600
    l2:
      max_entries: 2000
      compression_level: 6
    l3:
      ttl_seconds: 86400
  
  cache_warming:
    enabled: true
    strategy: "expensive"
```

**Expected Performance:**
- Dashboard queries: 65% cache hit
- Report generation: 50% cache hit
- Ad-hoc analytics: 30% cache hit
- Average time savings: 80%

### Example 3: SaaS Platform (Mixed)

```yaml
query_cache:
  enabled: true
  type: "adaptive"
  
  workload_detection:
    enabled: true
    auto_adjust: true
  
  mixed:
    separate_pools:
      enabled: true
      oltp_pool_ratio: 0.6
      olap_pool_ratio: 0.4
    
    smart_routing: true
```

**Expected Performance:**
- User queries (OLTP): 75% cache hit
- Analytics (OLAP): 45% cache hit
- Overall: 65% cache hit
- Resource savings: 55%

---

## Conclusion

Workload-specific query caching in ThemisDB provides:

✅ **Automatic Optimization**: Auto-detects workload patterns
✅ **Significant Performance Gains**: 2-50x improvements
✅ **Resource Efficiency**: 40-95% CPU/IO reduction
✅ **Flexibility**: Adapts to changing workloads
✅ **Production-Ready**: Comprehensive monitoring and tuning

For additional support:
- Documentation: https://makr-code.github.io/ThemisDB/
- Performance Tips: `docs/knowledge-base/PERFORMANCE_TIPS.md`
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues

---

**Version:** 1.5.0  
**Last Updated:** 2026-04-06  
**Maintainer:** ThemisDB Team
