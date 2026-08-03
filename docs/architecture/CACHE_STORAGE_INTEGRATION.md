# Cache-Storage Integration Guide

**Version:** 1.0  
**Status:** Active  
**Links:** [`UNIFIED_ACCESS_MODEL.md`](./UNIFIED_ACCESS_MODEL.md) · [`src/access_model/`](../../src/access_model/README.md)

---

## 1. Introduction

This guide explains how the Cache and Storage modules integrate through the **AccessCoordinator** broker. It is designed for:
- **Developers** integrating cache feedback with storage migration
- **Operators** configuring tier promotion/demotion policies
- **Architects** designing data-access patterns

---

## 2. Integration Points

### 2.1 Cache → Storage: Eviction Feedback

When the cache evicts an entry from L1 or L2, it notifies the AccessCoordinator:

```cpp
// From cache module (via EvictionListener)
coordinator->onCacheEvicted(key, from_tier, entry_size, access_count);

// Coordinator decision:
// - If entry is "hot" (high access count): candidate for warm-tier promotion
// - If entry is "cold" (low access count): candidate for warm-tier demotion
// - Async worker schedules storage tier action
```

**Benefits:**
- Storage sees real cache eviction patterns
- Predictive promotion: pre-load hot entries to L1 before next access
- Demotion: free cache space by migrating cold entries to warm storage

### 2.2 Storage → Cache: Promotion Signals

When storage detects hot access patterns or receives access-time updates:

```cpp
// From storage module (via PromotionListener)
coordinator->onStorageAccessDetected(key, from_tier, access_time, frequency);

// Coordinator decision:
// - If frequency > threshold: initiate cold→warm→L3 promotion
// - Schedule async fetch to cache tier
```

**Benefits:**
- Cache discovers hot data before application requests it
- Reduced latency for frequently accessed archived data
- Automatic cache warmup based on storage access patterns

### 2.3 Shared Policies: Age-Based Migration

Both layers use `AgeBasedPolicy` to make consistent demotion decisions:

```cpp
// Shared configuration (single source of truth)
AgeBasedPolicy policy;
policy.hot_to_warm_days = 30;
policy.warm_to_cold_days = 90;
policy.hot_zero_access_days = 14;
policy.warm_zero_access_days = 45;

// Cache layer uses this for L1→L2 demotion timing
cache_coordinator->setAgePolicy(policy);

// Storage layer uses this for hot→warm→cold migration timing
storage_coordinator->setAgePolicy(policy);

// Coordinator ensures both layers track age uniformly
access_coordinator->setUnifiedAgePolicy(policy);
```

**Benefits:**
- Consistent "hotness" definition across layers
- Simplified configuration (single policy vs. two separate ones)
- Operator has single source of truth for aging thresholds

---

## 3. Data Promotion Path (Cold → Warm → L1)

### Scenario: User requests archived data from S3 (cold storage)

```
Step 1: Access Request
  └─ Application calls: storage->get(key, {cache_hint: "L3"})
  
Step 2: Storage Retrieval
  └─ Cold-tier backend (S3) fetches data (500 ms latency)
  └─ Stores result in Warm tier (for next access)
  └─ Notifies coordinator: onStorageAccessDetected(key, "cold", frequency++)

Step 3: Coordinator Detection
  └─ Analyzes access pattern: "2 accesses in 10 seconds"
  └─ Decides: Promote to L3 (semantic cache)
  └─ Schedules async promotion worker

Step 4: Cache Promotion
  └─ Worker fetches from warm tier (10 ms latency)
  └─ Inserts into L3 (semantic cache)
  └─ Updates metrics: correlation_id, path_latency, tier_sequence

Step 5: Next Access (1 second later)
  └─ Application calls: cache->get(key)
  └─ L3 hit (10 µs latency instead of 500 ms)
  └─ Metrics show: "promoted_cold_to_l3_latency_saved_ms: 490"
```

### API Usage

```cpp
// Storage layer initiates promotion
auto result = storage->get(key, GetOptions{
  .cache_hint = CacheHint::L3_SEMANTIC,
  .access_coordinator = coordinator  // Optional
});

// Coordinator async worker handles promotion
coordinator->promoteAsync(key, "cold", "warm", {
  .target_tier = "L3",
  .max_wait_ms = 100,
  .on_complete = [app](const auto& result) {
    app->handlePromotedData(result);
  }
});

// Metrics logged automatically with correlation ID
auto metrics = coordinator->getTierMetrics(key);
// → latency_cold_to_warm_ms: 45
// → latency_warm_to_l3_ms: 12
// → total_promotion_path_ms: 57
```

---

## 4. Data Demotion Path (L1 → L2 → Warm → Cold)

### Scenario: Cache eviction triggers storage tier demotion

```
Step 1: L1 Eviction
  └─ Cache reaches capacity
  └─ LRU evicts key with low access count
  └─ Notifies coordinator: onCacheEvicted(key, "L1", size, access_count=2)

Step 2: Coordinator Analysis
  └─ Access count = 2 (very low)
  └─ Decision: Candidate for warm-tier demotion
  └─ Schedule background demotion worker

Step 3: L2 Demotion
  └─ If L2 capacity available: cache key moves to L2
  └─ If L2 full: coordinator notifies storage

Step 4: Warm-Tier Demotion
  └─ Storage checks age: 25 days (close to 30-day threshold)
  └─ Combined with cache eviction signal: Priority for demotion
  └─ Schedules warm→cold migration
  └─ Notifies coordinator: onDemotionScheduled(key, "warm", "cold")

Step 5: Background Migration
  └─ Worker transfers data from HDD to S3
  └─ Updates access-time metadata
  └─ Frees warm-tier capacity (now available for hot data)
```

### API Usage

```cpp
// Cache notifies on eviction
coordinator->onCacheEvicted(key, CacheTierLevel::L1, 2_MB, {
  .access_count = 2,
  .last_access_age_secs = 3600,
  .eviction_reason = "lru"
});

// Coordinator decides demotion
auto demotion_plan = coordinator->planDemotion(key, {
  .from_tier = "warm",
  .to_tier = "cold",
  .grace_period_secs = 600,  // 10 min delay before execution
  .reason = "cache_eviction_signal"
});

// Storage executes with coordination
storage->demoteAsync(key, demotion_plan, {
  .on_complete = [](auto result) {
    // Metrics: demotion_latency_ms, freed_capacity_bytes
  }
});
```

---

## 5. Configuration Best Practices

### Conservative (Prefer Cache Over Storage Migration)
```ini
[cache_tier]
l1_max_entries = 50000           # Large L1
l2_max_entries = 500000          # Large L2
l1_promotion_threshold = 5       # Keep more entries

[storage_tier]
hot_to_warm_days = 60            # Longer hot tier life
warm_to_cold_days = 180          # Longer warm tier life
migration_check_interval_secs = 86400  # Daily checks (not hourly)
```

### Aggressive (Optimize Storage Capacity)
```ini
[cache_tier]
l1_max_entries = 5000            # Smaller L1 (working memory only)
l2_max_entries = 50000           # Smaller L2
l1_promotion_threshold = 10      # Evict sooner

[storage_tier]
hot_to_warm_days = 14            # Aggressive hot→warm
warm_to_cold_days = 30           # Aggressive warm→cold
migration_check_interval_secs = 3600   # Hourly checks
```

### Balanced (Default ThemisDB)
```ini
[cache_tier]
l1_max_entries = 10000
l2_max_entries = 100000
l1_promotion_threshold = 10

[storage_tier]
hot_to_warm_days = 30
warm_to_cold_days = 90
migration_check_interval_secs = 3600
```

---

## 6. Observability Checklist

### Metrics to Monitor

| Metric | Target | Alert Threshold |
|--------|--------|-----------------|
| L1 Hit Rate | >90% | <75% |
| L2 Hit Rate | >70% | <50% |
| L3 Hit Rate | >60% | <40% |
| Promotion Latency (cold→warm) | <100ms | >500ms |
| Demotion Throughput (warm→cold) | >1000 entries/min | <100 entries/min |
| Coordination Overhead | <5% of query time | >10% |

### Dashboard Panels

1. **Tier Hit Rates** — Line chart over 24 hours
2. **Promotion/Demotion Throughput** — Stacked bar chart (cold→warm, warm→cold)
3. **Cache Eviction Signal → Storage Demotion Latency** — Scatter plot with correlation
4. **Age Distribution by Tier** — Histogram of data age in each tier
5. **Capacity Utilization** — Gauge per tier (L1, L2, L3, warm, cold)

### Structured Logging Query

```sql
-- Find slow promotions (debug)
SELECT correlation_id, from_tier, to_tier, latency_ms, data_size_bytes
FROM access_model_events
WHERE event = 'promotion_complete'
  AND latency_ms > 100
  AND timestamp > NOW() - INTERVAL '1 hour'
ORDER BY latency_ms DESC
LIMIT 20;

-- Analyze eviction→demotion correlation
SELECT 
  ce.correlation_id,
  ce.timestamp as eviction_time,
  sd.timestamp as demotion_time,
  (sd.timestamp - ce.timestamp) * 1000 as correlation_latency_ms
FROM cache_eviction_events ce
JOIN storage_demotion_events sd
  ON ce.key = sd.key
  AND sd.timestamp BETWEEN ce.timestamp 
                       AND ce.timestamp + INTERVAL '10 seconds'
WHERE ce.timestamp > NOW() - INTERVAL '1 day'
ORDER BY correlation_latency_ms DESC;
```

---

## 7. Troubleshooting

### Problem: High Cache Eviction Rate but Low Demotion Rate

**Symptom:** L1 hit rate declining, but storage demotion is slow

**Causes:**
1. Coordinator thread pool too small (default: 4)
2. Demotion grace period too long (default: 600s)
3. Storage I/O bottleneck (warm-tier disk saturated)

**Fix:**
```ini
[access_coordinator]
promote_demotion_threads = 16        # Increase workers
demote_to_cold_grace_period_secs = 60  # Reduce delay

[storage_tier]
max_migrations_per_cycle = 10000     # Increase throughput
migration_check_interval_secs = 300  # Check more frequently
```

### Problem: Cold-Tier Promotions Taking >1 Second

**Symptom:** User queries to archived data are very slow

**Causes:**
1. S3 retrieve latency baseline is 500ms–1s (normal)
2. Async promotion queue is backlogged
3. Network latency to object storage

**Fix:**
```ini
[access_coordinator]
promote_from_cold_timeout_ms = 200   # Increase timeout
promote_cold_thread_pool_size = 8    # More parallel fetches

# In application code:
storage->getPrefetch(key, {
  .cache_hint = CacheHint::L2_EPISODIC,  // Fetch to L2 instead of L3
  .async = true,
  .on_complete = callback
});
```

### Problem: Coordinator Consuming 20% of CPU

**Symptom:** High CPU usage from access_model threads

**Causes:**
1. Too many background workers (default: 4)
2. Metrics sampling rate at 100% (default)
3. Promotion/demotion decisions happening too frequently

**Fix:**
```ini
[access_model]
coordinator_thread_pool_size = 2     # Reduce workers

[access_coordinator]
correlation_id_sample_rate = 0.1     # 10% sampling (less overhead)
enable_detailed_tracing = false       # Disable detailed logs
```

---

## 8. See Also

- [`UNIFIED_ACCESS_MODEL.md`](./UNIFIED_ACCESS_MODEL.md) — Core concepts
- [`src/cache/ROADMAP.md`](../../src/cache/ROADMAP.md) — Cache roadmap
- [`src/storage/ROADMAP.md`](../../src/storage/ROADMAP.md) — Storage roadmap
- [`src/access_model/`](../../src/access_model/README.md) — Implementation guide

