# Locality-Aware Query Router

**Version:** 1.5.0  
**Status:** 🚧 In Development  
**YARN-Inspired:** Container Localization Pattern

## Überblick

Der Locality-Aware Query Router optimiert das Query-Routing durch Berücksichtigung von:
- **Daten-Lokalität:** Wo sind die Daten bereits vorhanden?
- **Shard-Last:** Welche Shards sind überlastet?
- **Netzwerk-Distanz:** Sind wir im gleichen Datacenter?

### YARN Container Localization Analogie

| YARN Container Localization | ThemisDB Locality-Aware Router |
|-----------------------------|--------------------------------|
| Launch container where data is | Route query where data is |
| Minimize HDFS reads | Minimize cross-shard transfers |
| Rack-awareness | Datacenter-awareness |
| Data locality vs. resource availability | Locality score vs. load score |

## Architektur

```
┌──────────────────────────────────────────────────┐
│         Locality-Aware Query Router               │
│                                                    │
│  ┌────────────────┐      ┌────────────────┐      │
│  │  Data Placement│      │ Affinity       │      │
│  │  Cache          │      │ Calculator     │      │
│  │                │      │                │      │
│  │ users:123 →    │      │ • Locality: 0.8│      │
│  │   shard2       │◄────►│ • Load: 0.3    │      │
│  │ orders:456 →   │      │ • Network: 1.0 │      │
│  │   shard2       │      │ → Score: 0.67  │      │
│  │ products:789 → │      │                │      │
│  │   shard3       │      │ Best: shard2   │      │
│  └────────────────┘      └────────────────┘      │
│          │                      │                  │
│          ▼                      ▼                  │
│    Route Query to Shard2 (80% data local)         │
└──────────────────────────────────────────────────┘
```

## Verwendung

### Initialisierung

```cpp
#include "sharding/locality_aware_router.h"

LocalityAwareRouter::Config config;
config.locality_weight = 0.50f;   // 50% locality
config.load_weight = 0.30f;        // 30% load
config.network_weight = 0.20f;     // 20% network
config.prefer_local_shard = true;
config.local_shard_bonus = 0.2f;   // 20% bonus

LocalityAwareRouter router(
    "shard1",
    topology,
    resource_manager,
    config
);
```

### Query Routing

```cpp
// Define query specification
LocalityAwareRouter::QuerySpec spec;
spec.query_aql = "FOR u IN users FILTER u._key == '123' RETURN u";
spec.accessed_collections = {"users"};
spec.accessed_keys = {"user:123"};

// Route to best shard
std::string target_shard = router.routeQuery(spec);

// Execute on target
executeOnShard(target_shard, spec.query_aql);
```

### Data Placement Tracking

```cpp
// After writing data, update placement cache
void onDataWritten(const std::string& collection,
                   const std::string& key,
                   const std::string& shard_id) {
    router.updateDataPlacement(collection, key, shard_id);
}

// After deleting data, remove from cache
void onDataDeleted(const std::string& collection,
                   const std::string& key) {
    router.removeDataPlacement(collection, key);
}
```

### Affinity Scoring

```cpp
// Get detailed affinity scores for all shards
auto affinities = router.computeAffinity(spec);

for (const auto& affinity : affinities) {
    std::cout << "Shard: " << affinity.shard_id << "\n";
    std::cout << "  Locality: " << affinity.locality_score << "\n";
    std::cout << "  Load: " << affinity.load_score << "\n";
    std::cout << "  Network: " << affinity.network_score << "\n";
    std::cout << "  Combined: " << affinity.combined_score << "\n";
}

// Best shard is first in sorted list
std::string best_shard = affinities[0].shard_id;
```

## Scoring-Algorithmus

### Combined Score Formel

```
combined_score = (locality_weight × locality_score) +
                 (load_weight × (1 - load_score)) +
                 (network_weight × network_score) +
                 (is_local ? local_bonus : 0)
```

### Locality Score

```
locality_score = keys_on_shard / total_keys
```

Beispiel:
- Query greift auf 10 Keys zu
- Shard1 hat 8 davon → locality_score = 0.8
- Shard2 hat 2 davon → locality_score = 0.2

### Load Score

```
load_score = (cpu_usage × 0.4) + (ram_usage × 0.3) +
             (query_latency_p99 × 0.3)
```

Invertiert in Combined Score: `(1 - load_score)` → bevorzugt niedrige Last

### Network Score

```
network_score = 1.0    (same datacenter)
              = 0.5    (different datacenter, same region)
              = 0.1    (cross-region)
```

## Performance

### Benchmarks (Intel Xeon Gold 6248R)

| Operation | Latenz | Durchsatz |
|-----------|--------|-----------|
| `routeQuery()` (10 shards) | 8-15 μs | 66k-125k ops/s |
| `routeQuery()` (100 shards) | 25-40 μs | 25k-40k ops/s |
| `computeAffinity()` (10 keys) | 5-10 μs | 100k-200k ops/s |
| `updateDataPlacement()` | 200-500 ns | 2M-5M ops/s |

### Skalierung

- **Placement Cache:** O(1) Lookup via Hash Map
- **Affinity Calculation:** O(N × K) - N shards, K keys
- **Memory:** ~50 bytes pro Cache-Eintrag (100k entries = 5 MB)

## Integration

### ShardRouter

```cpp
class ShardRouter {
    std::shared_ptr<LocalityAwareRouter> locality_router_;
    
    nlohmann::json executeQuery(const std::string& query) {
        // Analyze query
        auto spec = analyzeQuery(query);
        
        // Route with locality awareness
        auto target_shard = locality_router_->routeQuery(spec);
        
        // Execute
        return executeOnShard(target_shard, query);
    }
};
```

### WAL Integration

```cpp
// Update placement cache on WAL apply
void WALApplier::applyInsert(const WALEntry& entry) {
    // ... apply insert ...
    
    // Track data placement
    locality_router_->updateDataPlacement(
        entry.collection,
        entry.key,
        local_shard_id_
    );
}
```

## Optimization-Strategien

### Co-Location Hints

```cpp
// Suggest which collections should be co-located
auto suggestions = router.suggestCoLocation({
    "users", "orders", "payments"
});

// Output: "users and orders should share shards (90% join rate)"
```

### Bloom Filter Mode

```cpp
// For very large datasets, use Bloom Filter
config.use_bloom_filter = true;
config.max_cache_entries = 1000000;  // 1M entries

// Trade-off: 1% false positive rate, 10x memory savings
```

## Troubleshooting

### Issue: Queries immer lokal geroutet trotz Remote-Daten

**Symptom:** `local_routes / queries_routed > 0.9`

**Diagnose:**
```cpp
auto stats = router.getStatistics();
std::cout << "Local routes: " << stats.local_routes << "\n";
std::cout << "Remote routes: " << stats.remote_routes << "\n";
```

**Lösung:**
```cpp
config.local_shard_bonus = 0.1f;  // Reduce bonus (war 0.2f)
config.locality_weight = 0.7f;    // Increase locality weight
```

### Issue: Cross-Shard Queries trotz lokaler Daten

**Symptom:** Hohe `cross_shard_joins` Metrik

**Lösung:**
- Prüfe Placement Cache: `router.hasData(shard_id, collection, key)`
- Update Cache nach Writes/Deletes
- Erhöhe `cache_ttl_seconds` (Default: 300s)

## Siehe auch

- [Shard Resource Manager](SHARD_RESOURCE_MANAGER.md)
- [Distributed Scheduler](DISTRIBUTED_COORDINATOR.md)
- [Gossip Config Manager](GOSSIP_CONFIG_MANAGER.md)
