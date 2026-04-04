# Shard Resource Manager

**Version:** 1.5.0  
**Status:** 🚧 In Development  
**YARN-Inspired:** NodeManager Local Resource Tracking

## Überblick

Der Shard Resource Manager überwacht lokale Ressourcen (CPU, RAM, VRAM, Disk, Netzwerk) und teilt diese Informationen via Gossip-Protokoll mit anderen Shards. Dies ermöglicht intelligente, dezentrale Entscheidungen über Query-Routing, Load Balancing und Throttling.

### YARN NodeManager Analogie

| YARN NodeManager | ThemisDB Shard Resource Manager |
|------------------|----------------------------------|
| Überwacht Container-Ressourcen | Überwacht Query-Ressourcen |
| Sendet Heartbeats an ResourceManager | Sendet Gossip-Updates an Peers |
| Meldet verfügbare Kapazität | Meldet Health Score |
| Lokale Entscheidungen (Kill Container) | Lokale Entscheidungen (Throttle Query) |

## Architektur

```
┌──────────────────────────────────────────────┐
│         Shard Resource Manager                │
│                                                │
│  ┌──────────────┐      ┌──────────────┐      │
│  │   Local      │      │    Peer      │      │
│  │  Monitoring  │      │    Cache     │      │
│  │              │      │              │      │
│  │ • CPU        │      │ • Shard2:     │      │
│  │ • RAM        │      │   85% load   │      │
│  │ • VRAM       │◄────►│ • Shard3:    │      │
│  │ • Disk       │Gossip│   20% load   │      │
│  │ • Network    │      │ • Shard4:    │      │
│  │              │      │   UNHEALTHY  │      │
│  └──────────────┘      └──────────────┘      │
│          │                     │              │
│          ▼                     ▼              │
│    canAcceptQuery?       Route to Shard3      │
└──────────────────────────────────────────────┘
```

## Verwendung

### Initialisierung

```cpp
#include "sharding/shard_resource_manager.h"

auto gossip_manager = std::make_shared<GossipConfigManager>(...);

ShardResourceManager::Config config;
config.snapshot_interval_ms = 5000;       // 5s
config.enable_auto_throttling = true;
config.throttle_threshold = 0.85f;        // 85%

ShardResourceManager resource_mgr("shard1", gossip_manager, config);
resource_mgr.start();
```

### Query Admission Control

```cpp
// Before executing a query, check if shard can handle it
ShardResourceManager::QuerySpec spec;
spec.estimated_memory_bytes = 500 * 1024 * 1024;  // 500MB
spec.estimated_cpu_percent = 30;
spec.estimated_duration = std::chrono::seconds(10);

if (resource_mgr.canAcceptQuery(spec)) {
    // Execute query
    executeQuery(query);
} else {
    // Throttle or route to another shard
    routeToLightlyLoadedShard(query);
}
```

### Peer-Aware Routing

```cpp
// Find the least loaded shard for routing
auto peer_resources = resource_mgr.getPeerResources();

std::string best_shard;
float min_load = 1.0f;

for (const auto& [shard_id, snapshot] : peer_resources) {
    float load = snapshot.cpu_usage_percent / 100.0f;
    if (load < min_load && snapshot.health_score > 70.0f) {
        min_load = load;
        best_shard = shard_id;
    }
}

// Route query to best_shard
```

### Health Score Interpretation

Der Health Score (0-100) basiert auf:
- **CPU-Auslastung** (30% Gewicht)
- **RAM-Auslastung** (25% Gewicht)
- **Disk I/O Latenz** (20% Gewicht)
- **Query-Latenz (p99)** (15% Gewicht)
- **Pending Queries** (10% Gewicht)

| Score | Zustand | Aktion |
|-------|---------|--------|
| 90-100 | Exzellent | Normal operations |
| 70-89 | Gut | Normal operations |
| 50-69 | Überlastet | Route neue Queries zu anderen Shards |
| 0-49 | Kritisch | Throttle/Reject neue Queries |

## Performance

### Benchmarks (Intel Xeon Gold 6248R)

| Operation | Latenz | Throughput |
|-----------|--------|------------|
| `getCurrentSnapshot()` | 2-5 μs | 200k ops/s |
| `canAcceptQuery()` | 50-100 ns | 10M ops/s |
| `getPeerResources()` (100 shards) | 15 μs | 66k ops/s |

## Integration mit anderen Komponenten

### ShardRouter

```cpp
class ShardRouter {
    std::shared_ptr<ShardResourceManager> resource_mgr_;
    
    std::string routeQuery(const Query& query) {
        auto spec = estimateQueryResources(query);
        
        if (resource_mgr_->canAcceptQuery(spec)) {
            return local_shard_id_;
        }
        
        // Route to least loaded peer
        auto peers = resource_mgr_->getPeerResources();
        return selectBestPeer(peers);
    }
};
```

### Prometheus Metrics

```cpp
// Export metrics
auto snapshot = resource_mgr.getCurrentSnapshot();

prometheus_metrics->recordGauge("shard_cpu_usage_percent", 
                                 snapshot.cpu_usage_percent);
prometheus_metrics->recordGauge("shard_health_score", 
                                 snapshot.health_score);
prometheus_metrics->recordGauge("shard_active_queries", 
                                 snapshot.active_queries);
```

## Troubleshooting

### Issue: Hohe CPU-Auslastung trotz weniger Queries

**Symptom:** `cpu_usage_percent > 80%` aber `active_queries < 10`

**Diagnose:**
```cpp
auto snapshot = resource_mgr.getCurrentSnapshot();
std::cout << "CPU: " << snapshot.cpu_usage_percent << "%\n";
std::cout << "Active Queries: " << snapshot.active_queries << "\n";
```

**Lösung:**
- Prüfe auf Background-Tasks (Compaction, Replication)
- Erhöhe `n_threads` für parallele Query-Verarbeitung

### Issue: Falsche Health Scores

**Symptom:** Shard meldet Health Score 90%, aber Queries sind langsam

**Lösung:**
```cpp
config.enable_adaptive_health_score = true;
// Health Score berücksichtigt nun p99-Latenz stärker
```

## Siehe auch

- [Gossip Config Manager](GOSSIP_CONFIG_MANAGER.md)
- [Locality-Aware Router](LOCALITY_AWARE_ROUTER.md)
- [YARN Architecture Overview](YARN_INSPIRED_ARCHITECTURE.md)
