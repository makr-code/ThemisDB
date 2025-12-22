# ThemisDB - RAID-ähnliche Datenverteilung und Redundanz

**Version:** 1.0  
**Stand:** 2. Dezember 2025  
**Status:** Implementiert ✅

---

## Executive Summary

ThemisDB implementiert ein **RAID-inspiriertes Redundanzsystem** für Sharding, das verschiedene
Strategien für Load-Balancing, Datensicherheit und Ausfallsicherheit bietet. Ähnlich wie bei
RAID-Systemen können verschiedene Modi kombiniert werden, um den optimalen Trade-off zwischen
Performance, Speichereffizienz und Redundanz zu erreichen.

---

## Verfügbare Redundanz-Modi

### Übersicht

| Modus | Beschreibung | Redundanz | Speichereffizienz | Read-Performance | Write-Performance |
|-------|--------------|-----------|-------------------|------------------|-------------------|
| **NONE** | Kein RAID, nur Sharding | 0 | 100% | Baseline | Baseline |
| **MIRROR** | Vollständige Spiegelung (RAID-1-ähnlich) | N Kopien | 100/N% | N× besser | Baseline |
| **STRIPE** | Daten aufteilen (RAID-0-ähnlich) | 0 | 100% | N× besser | N× besser |
| **STRIPE_MIRROR** | Striping + Mirror (RAID-10-ähnlich) | N Kopien | 100/N% | Sehr gut | Gut |
| **PARITY** | Erasure Coding (RAID-5/6-ähnlich) | k Parity | (n-k)/n% | Gut | Langsamer |
| **GEO_MIRROR** | Geo-verteilte Spiegelung | N DCs | 100/N% | Lokal optimal | DC-Latenz |

### Detaillierte Beschreibung

#### 1. NONE - Nur Sharding (Standard)

```
┌─────────────────────────────────────────────────────────────┐
│                    Consistent Hash Ring                      │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐        │
│  │ Shard 1 │  │ Shard 2 │  │ Shard 3 │  │ Shard 4 │        │
│  │ D1, D5  │  │ D2, D6  │  │ D3, D7  │  │ D4, D8  │        │
│  └─────────┘  └─────────┘  └─────────┘  └─────────┘        │
└─────────────────────────────────────────────────────────────┘
```

- **Use Case:** Entwicklung, nicht-kritische Daten
- **Vorteil:** Maximale Speichereffizienz
- **Nachteil:** Datenverlust bei Shard-Ausfall

#### 2. MIRROR - Vollständige Spiegelung (RAID-1)

```
┌─────────────────────────────────────────────────────────────┐
│                    Replication Factor = 3                    │
│                                                              │
│  ┌─────────┐    ┌─────────┐    ┌─────────┐                 │
│  │ Primary │───▶│ Replica1│───▶│ Replica2│                 │
│  │ Shard 1 │    │ Shard 2 │    │ Shard 3 │                 │
│  │  D1-D4  │    │  D1-D4  │    │  D1-D4  │                 │
│  └─────────┘    └─────────┘    └─────────┘                 │
│       ▲                                                      │
│       │ Writes                                               │
│       │                                                      │
│  ─────┴───────────────────────────────────────────────────  │
│         Reads (Load-Balanced across all replicas)            │
└─────────────────────────────────────────────────────────────┘
```

- **Konfiguration:**
  ```yaml
  sharding:
    redundancy_mode: MIRROR
    replication_factor: 3
    read_preference: NEAREST  # PRIMARY, NEAREST, ROUND_ROBIN
    write_concern: MAJORITY   # ALL, MAJORITY, ONE
  ```

- **Vorteile:**
  - Höchste Ausfallsicherheit
  - Read-Skalierung (N× Lesekapazität)
  - Einfache Wiederherstellung

- **Nachteile:**
  - N× Speicherverbrauch
  - Write-Amplification

#### 3. STRIPE - Daten-Striping (RAID-0)

```
┌─────────────────────────────────────────────────────────────┐
│              Large Document Striping (4 Shards)              │
│                                                              │
│  Document: 40KB                                              │
│  ┌──────────────────────────────────────────────────┐       │
│  │ Chunk1   Chunk2   Chunk3   Chunk4   │            │       │
│  │ 10KB     10KB     10KB     10KB     │            │       │
│  └──────────────────────────────────────────────────┘       │
│       │        │        │        │                          │
│       ▼        ▼        ▼        ▼                          │
│  ┌────────┐┌────────┐┌────────┐┌────────┐                   │
│  │Shard 1 ││Shard 2 ││Shard 3 ││Shard 4 │                   │
│  │Chunk 1 ││Chunk 2 ││Chunk 3 ││Chunk 4 │                   │
│  └────────┘└────────┘└────────┘└────────┘                   │
│       │        │        │        │                          │
│       └────────┴────────┴────────┘                          │
│                    │                                         │
│            Parallel Read/Write                               │
│            (4× Throughput)                                   │
└─────────────────────────────────────────────────────────────┘
```

- **Konfiguration:**
  ```yaml
  sharding:
    redundancy_mode: STRIPE
    stripe_size: 64KB        # Chunk-Größe
    min_stripe_shards: 4     # Mindestanzahl Shards für Striping
    stripe_large_docs: true  # Nur große Dokumente stripen
    large_doc_threshold: 1MB
  ```

- **Vorteile:**
  - Maximaler Throughput für große Dokumente
  - Parallele I/O
  - Keine Speicher-Overhead

- **Nachteile:**
  - Keine Redundanz (Datenverlust bei jedem Shard-Ausfall)
  - Komplexere Recovery

#### 4. STRIPE_MIRROR - Kombination (RAID-10)

```
┌─────────────────────────────────────────────────────────────┐
│           STRIPE_MIRROR: Best of Both Worlds                 │
│                                                              │
│  ┌─────────────────────────────────────────────┐            │
│  │             Stripe Group 1                   │            │
│  │  ┌────────┐  ┌────────┐  ┌────────┐         │            │
│  │  │ S1-P   │  │ S2-P   │  │ S3-P   │ Primary │            │
│  │  │Chunk 1 │  │Chunk 2 │  │Chunk 3 │         │            │
│  │  └────────┘  └────────┘  └────────┘         │            │
│  │       │           │           │              │            │
│  │       ▼           ▼           ▼              │            │
│  │  ┌────────┐  ┌────────┐  ┌────────┐         │            │
│  │  │ S1-R   │  │ S2-R   │  │ S3-R   │ Replica │            │
│  │  │Chunk 1 │  │Chunk 2 │  │Chunk 3 │         │            │
│  │  └────────┘  └────────┘  └────────┘         │            │
│  └─────────────────────────────────────────────┘            │
└─────────────────────────────────────────────────────────────┘
```

- **Konfiguration:**
  ```yaml
  sharding:
    redundancy_mode: STRIPE_MIRROR
    stripe_size: 64KB
    replication_factor: 2
    stripe_across_datacenters: false
  ```

- **Vorteile:**
  - Hoher Throughput UND Redundanz
  - Kann einen Shard pro Stripe-Gruppe verlieren

- **Nachteile:**
  - 50% Speichereffizienz (bei RF=2)
  - Komplexere Verwaltung

#### 5. PARITY - Erasure Coding (RAID-5/6)

```
┌─────────────────────────────────────────────────────────────┐
│        Erasure Coding: Reed-Solomon (4+2 Konfiguration)      │
│                                                              │
│  Document → 4 Data Chunks + 2 Parity Chunks                 │
│                                                              │
│  ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐                 │
│  │ D1 │ │ D2 │ │ D3 │ │ D4 │ │ P1 │ │ P2 │                 │
│  └────┘ └────┘ └────┘ └────┘ └────┘ └────┘                 │
│    │      │      │      │      │      │                     │
│    ▼      ▼      ▼      ▼      ▼      ▼                     │
│  ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐                 │
│  │ S1 │ │ S2 │ │ S3 │ │ S4 │ │ S5 │ │ S6 │                 │
│  └────┘ └────┘ └────┘ └────┘ └────┘ └────┘                 │
│                                                              │
│  ✓ Kann 2 beliebige Shard-Ausfälle tolerieren               │
│  ✓ 67% Speichereffizienz (4/6)                              │
└─────────────────────────────────────────────────────────────┘
```

- **Konfiguration:**
  ```yaml
  sharding:
    redundancy_mode: PARITY
    erasure_coding:
      data_shards: 4      # k = Daten-Chunks
      parity_shards: 2    # m = Parity-Chunks
      algorithm: REED_SOLOMON  # oder CAUCHY, LRC
    min_doc_size: 1MB     # Nur für große Dokumente
  ```

- **Vorteile:**
  - Beste Speichereffizienz bei guter Redundanz
  - Skaliert gut mit Cluster-Größe

- **Nachteile:**
  - CPU-intensiv (Encoding/Decoding)
  - Langsamer bei Writes
  - Recovery erfordert Lesen von k Shards

#### 6. GEO_MIRROR - Geo-verteilte Spiegelung

```
┌─────────────────────────────────────────────────────────────┐
│              Geographic Multi-Datacenter Mirror              │
│                                                              │
│  ┌─────────────────┐        ┌─────────────────┐             │
│  │   DC: eu-west   │        │   DC: us-east   │             │
│  │                 │  Async │                 │             │
│  │  ┌───────────┐  │◀──────▶│  ┌───────────┐  │             │
│  │  │ Shard 1-P │  │        │  │ Shard 1-R │  │             │
│  │  │ Shard 2-P │  │        │  │ Shard 2-R │  │             │
│  │  │ Shard 3-P │  │        │  │ Shard 3-R │  │             │
│  │  └───────────┘  │        │  └───────────┘  │             │
│  │                 │        │                 │             │
│  │  RTT: <1ms      │        │  RTT: ~80ms     │             │
│  └─────────────────┘        └─────────────────┘             │
│           │                          │                       │
│           │                          │                       │
│           ▼                          ▼                       │
│  ┌─────────────────┐        ┌─────────────────┐             │
│  │   DC: ap-south  │        │   DC: ap-north  │             │
│  │  ┌───────────┐  │        │  ┌───────────┐  │             │
│  │  │ Shard 1-R │  │        │  │ Shard 1-R │  │             │
│  │  └───────────┘  │        │  └───────────┘  │             │
│  └─────────────────┘        └─────────────────┘             │
│                                                              │
│  Write: Primary DC → Async to all DCs                       │
│  Read:  Local DC (eventual consistency) or                   │
│         Primary DC (strong consistency)                      │
└─────────────────────────────────────────────────────────────┘
```

- **Konfiguration:**
  ```yaml
  sharding:
    redundancy_mode: GEO_MIRROR
    geo_replication:
      primary_dc: eu-west
      replica_dcs:
        - us-east
        - ap-south
        - ap-north
      replication_mode: ASYNC  # SYNC (langsam!), SEMI_SYNC, ASYNC
      conflict_resolution: LAST_WRITE_WINS
      read_preference: LOCAL_THEN_PRIMARY
  ```

---

## Hybrid-Konfigurationen (Mischvarianten)

### Beispiel 1: Collection-basierte Redundanz

```yaml
# Verschiedene Redundanz-Modi pro Collection
collections:
  users:
    # Kritische Daten: Hohe Redundanz
    redundancy_mode: MIRROR
    replication_factor: 3
    
  analytics:
    # Große, regenerierbare Daten: Hoher Throughput
    redundancy_mode: STRIPE
    stripe_size: 1MB
    
  logs:
    # Unkritisch, aber viele Daten: Speichereffizient
    redundancy_mode: PARITY
    erasure_coding:
      data_shards: 6
      parity_shards: 2
      
  user_sessions:
    # Schneller Zugriff + Ausfallsicherheit
    redundancy_mode: STRIPE_MIRROR
    replication_factor: 2
```

### Beispiel 2: Tiered Storage

```yaml
# Hot/Warm/Cold Tiers mit verschiedenen Redundanzen
tiers:
  hot:
    # Aktive Daten: Schnell + Redundant
    redundancy_mode: STRIPE_MIRROR
    storage_type: SSD
    replication_factor: 2
    
  warm:
    # Weniger aktiv: Gute Redundanz, weniger Performance
    redundancy_mode: MIRROR
    storage_type: HDD
    replication_factor: 2
    
  cold:
    # Archiv: Speichereffizient
    redundancy_mode: PARITY
    storage_type: OBJECT_STORAGE
    erasure_coding:
      data_shards: 10
      parity_shards: 4
```

### Beispiel 3: Multi-Region mit lokaler Optimierung

```yaml
# Geo-Mirror mit lokalem RAID-10
geo_replication:
  enabled: true
  primary_dc: eu-west
  
datacenters:
  eu-west:
    # Lokal STRIPE_MIRROR für Performance
    local_redundancy: STRIPE_MIRROR
    shards: 8
    replication_factor: 2
    
  us-east:
    # Nur Mirror für Disaster Recovery
    local_redundancy: MIRROR
    shards: 4
    replication_factor: 2
    read_only: false
    
  ap-south:
    # Read-Replica für lokale Latenz
    local_redundancy: MIRROR
    shards: 4
    replication_factor: 1
    read_only: true
```

---

## Implementierungsdetails

### Consistent Hash Ring mit Redundanz

```cpp
// include/sharding/redundancy_strategy.h

enum class RedundancyMode {
    NONE,           // Nur Sharding, keine Redundanz
    MIRROR,         // N vollständige Kopien
    STRIPE,         // Daten-Striping über Shards
    STRIPE_MIRROR,  // Striping + Mirroring
    PARITY,         // Erasure Coding
    GEO_MIRROR      // Geo-verteilte Spiegelung
};

struct RedundancyConfig {
    RedundancyMode mode = RedundancyMode::MIRROR;
    uint32_t replication_factor = 3;
    uint32_t stripe_size_kb = 64;
    uint32_t min_stripe_shards = 4;
    
    // Erasure Coding
    struct ErasureCoding {
        uint32_t data_shards = 4;
        uint32_t parity_shards = 2;
        std::string algorithm = "REED_SOLOMON";
    } erasure_coding;
    
    // Geo-Replication
    struct GeoReplication {
        std::string primary_dc;
        std::vector<std::string> replica_dcs;
        std::string replication_mode = "ASYNC";
        std::string conflict_resolution = "LAST_WRITE_WINS";
    } geo_replication;
    
    // Read/Write Preferences
    std::string read_preference = "NEAREST";
    std::string write_concern = "MAJORITY";
};
```

### Write-Path mit Redundanz

```cpp
// Pseudo-Code für Write-Operationen

WriteResult write(const Document& doc, const RedundancyConfig& config) {
    switch (config.mode) {
        case RedundancyMode::MIRROR: {
            // 1. Bestimme Primary Shard
            auto primary = hash_ring.getShardForURN(doc.urn);
            // 2. Hole Replica-Shards
            auto replicas = hash_ring.getSuccessors(doc.urn.hash(), 
                                                     config.replication_factor - 1);
            // 3. Schreibe parallel zu allen
            auto futures = parallelWrite({primary} + replicas, doc);
            // 4. Warte auf Write-Concern
            return waitForWriteConcern(futures, config.write_concern);
        }
        
        case RedundancyMode::STRIPE: {
            // 1. Teile Dokument in Chunks
            auto chunks = splitDocument(doc, config.stripe_size_kb);
            // 2. Verteile Chunks auf Shards
            for (size_t i = 0; i < chunks.size(); i++) {
                auto shard = hash_ring.getShardForHash(doc.urn.hash() + i);
                writeChunk(shard, chunks[i]);
            }
            return WriteResult::success();
        }
        
        case RedundancyMode::PARITY: {
            // 1. Teile Dokument in Data-Chunks
            auto data_chunks = splitDocument(doc, config.erasure_coding.data_shards);
            // 2. Berechne Parity-Chunks
            auto parity_chunks = reedSolomonEncode(data_chunks, 
                                                    config.erasure_coding.parity_shards);
            // 3. Verteile alle Chunks
            auto all_chunks = data_chunks + parity_chunks;
            for (size_t i = 0; i < all_chunks.size(); i++) {
                auto shard = hash_ring.getShardForHash(doc.urn.hash() + i);
                writeChunk(shard, all_chunks[i]);
            }
            return WriteResult::success();
        }
        
        // ... weitere Modi
    }
}
```

---

## Prometheus Metriken

```
# Redundanz-Metriken
themisdb_redundancy_mode{collection="users"} = 1  # MIRROR
themisdb_replication_factor{collection="users"} = 3
themisdb_replica_lag_seconds{shard="shard_001", replica="replica_1"} = 0.05
themisdb_stripe_chunks_total{collection="analytics"} = 10000

# Erasure Coding
themisdb_erasure_encode_duration_seconds_bucket{le="0.01"} = 9500
themisdb_erasure_decode_duration_seconds_bucket{le="0.05"} = 9000
themisdb_erasure_recovery_operations_total = 15

# Geo-Replication
themisdb_geo_replication_lag_seconds{source="eu-west", target="us-east"} = 0.08
themisdb_geo_cross_dc_writes_total{source="eu-west"} = 1000000
themisdb_geo_conflict_resolutions_total{strategy="LAST_WRITE_WINS"} = 50
```

---

## Vergleich mit echten RAID-Systemen

| Feature | RAID 0 | RAID 1 | RAID 5 | RAID 10 | ThemisDB |
|---------|--------|--------|--------|---------|----------|
| Striping | ✅ | ❌ | ✅ | ✅ | ✅ STRIPE |
| Mirroring | ❌ | ✅ | ❌ | ✅ | ✅ MIRROR |
| Parity | ❌ | ❌ | ✅ | ❌ | ✅ PARITY |
| Hybrid | ❌ | ❌ | ❌ | ✅ | ✅ STRIPE_MIRROR |
| Geo-Distribution | ❌ | ❌ | ❌ | ❌ | ✅ GEO_MIRROR |
| Per-Collection Config | ❌ | ❌ | ❌ | ❌ | ✅ |
| Dynamic Reconfig | ❌ | ❌ | ❌ | ❌ | ✅ |

---

## Empfehlungen

| Use Case | Empfohlener Modus | Begründung |
|----------|-------------------|------------|
| Kritische Geschäftsdaten | MIRROR (RF=3) | Höchste Ausfallsicherheit |
| Große Media-Dateien | STRIPE + separates Backup | Maximaler Throughput |
| Logs/Analytics | PARITY (6+2) | Speichereffizient, toleriert Ausfälle |
| E-Commerce | STRIPE_MIRROR | Balance aus Performance und Sicherheit |
| Multi-Region SaaS | GEO_MIRROR | Niedrige Latenz weltweit |
| Entwicklung | NONE | Kein Overhead |

---

## Cross-Shard Graph- und Hybrid-Suchen

### Herausforderung: Verteilte Suche im Sharding

Mit der komplexen Sharding- und Redundanz-Struktur stellt sich die Frage: **Wie stellen wir sicher, dass Graph-Suchen und Hybrid-Suchen über alle relevanten Shards durchgeführt werden?**

Die Lösung basiert auf:
1. **URN-basierte netzübergreifende Suche** - Globale Adressierung über Shard-Grenzen hinweg
2. **Hub-Shard-Knoten** - Koordinations-Layer für verteilte Queries
3. **Scatter-Gather mit intelligenter Filterung** - Nur relevante Shards anfragen

---

## URN-basierte Cross-Shard-Suche

### Konzept: Location-Transparent Queries

```
┌─────────────────────────────────────────────────────────────────┐
│              URN-basierte Cross-Network Search                   │
│                                                                  │
│  Client Query:                                                   │
│  "GRAPH SEARCH urn:themis:graph:docs:chunks:* EXPAND 2 HOPS"    │
│                                                                  │
│         │                                                        │
│         ▼                                                        │
│  ┌────────────────┐                                              │
│  │  Hub-Shard     │  1. URN Pattern Analysis                    │
│  │  (Coordinator) │  2. Shard Discovery via URN Resolver        │
│  └────────────────┘  3. Query Distribution                      │
│         │                                                        │
│         ├────────────────────┬──────────────────┐               │
│         ▼                    ▼                  ▼               │
│  ┌──────────┐        ┌──────────┐        ┌──────────┐          │
│  │ Shard A  │        │ Shard B  │        │ Shard C  │          │
│  │ URN:..01 │        │ URN:..02 │        │ URN:..03 │          │
│  └──────────┘        └──────────┘        └──────────┘          │
│       │                   │                   │                 │
│       │  Local Graph      │  Local Graph      │  Local Graph   │
│       │  Traversal        │  Traversal        │  Traversal     │
│       │                   │                   │                 │
│       ▼                   ▼                   ▼                 │
│  [Chunk A1, A2]      [Chunk B1]          [Chunk C1, C2, C3]    │
│       │                   │                   │                 │
│       └───────────────────┴───────────────────┘                 │
│                           │                                     │
│                           ▼                                     │
│                    ┌────────────────┐                           │
│                    │  Hub-Shard     │  4. Result Merging       │
│                    │  (Aggregator)  │  5. Score Re-ranking     │
│                    └────────────────┘  6. Deduplication        │
│                           │                                     │
│                           ▼                                     │
│                    [Merged Results]                             │
└─────────────────────────────────────────────────────────────────┘
```

### URN-Pattern Matching für Shard-Discovery

```cpp
// Beispiel: Graph-Suche über URN-Pattern
class ShardedGraphSearch {
public:
    /**
     * Cross-shard graph traversal via URN patterns
     * 
     * @param urn_pattern URN pattern (with wildcards)
     *        Example: "urn:themis:graph:docs:chunks:*"
     * @param hops Number of hops to traverse
     * @param edge_types Edge types to follow:
     *        - parent: Parent document/chunk relationship
     *        - next: Sequential ordering (e.g., pages in document)
     *        - prev: Reverse sequential ordering
     *        - geo: Geographical proximity (spatial neighbors)
     * @return Merged graph results from all relevant shards
     */
    GraphSearchResult search(
        std::string_view urn_pattern,
        uint32_t hops,
        const std::vector<std::string>& edge_types
    ) {
        // 1. Parse URN pattern and determine relevant shards
        auto urn_filter = URN::parsePattern(urn_pattern);
        auto target_shards = urn_resolver_.resolvePattern(urn_filter);
        
        // 2. Distribute query to all relevant shards
        std::vector<std::future<LocalGraphResult>> futures;
        for (const auto& shard : target_shards) {
            futures.push_back(std::async([&]() {
                return remote_executor_.executeGraphSearch(
                    shard, urn_pattern, hops, edge_types
                );
            }));
        }
        
        // 3. Collect and merge results
        GraphSearchResult merged;
        for (auto& future : futures) {
            auto local_result = future.get();
            mergeGraphResults(merged, local_result);
        }
        
        // 4. Handle cross-shard edges
        resolveInterShardEdges(merged, target_shards);
        
        return merged;
    }
    
private:
    /**
     * Resolve edges that cross shard boundaries
     * Example: Chunk in Shard A has "next" edge to Chunk in Shard B
     */
    void resolveInterShardEdges(
        GraphSearchResult& result,
        const std::vector<ShardInfo>& shards
    ) {
        // Find all URN references that point to different shards
        for (auto& node : result.nodes) {
            for (auto& edge : node.edges) {
                auto target_urn = URN::parse(edge.target_urn);
                auto target_shard = urn_resolver_.resolvePrimary(*target_urn);
                
                // If target is on different shard, fetch it
                if (!isLocalShard(target_shard)) {
                    auto remote_node = remote_executor_.fetchNode(
                        target_shard, *target_urn
                    );
                    result.nodes.push_back(remote_node);
                }
            }
        }
    }
};
```

---

## Hub-Shard-Knoten: Implementierung

### Architektur: Hub-and-Spoke Pattern

```
┌─────────────────────────────────────────────────────────────────┐
│                Hub-Shard Architecture                            │
│                                                                  │
│                    ┌──────────────────┐                          │
│                    │   Hub-Shard      │                          │
│                    │   (Coordinator)  │                          │
│                    │                  │                          │
│                    │  Capabilities:   │                          │
│                    │  - Query Planning│                          │
│                    │  - URN Resolution│                          │
│                    │  - Result Merging│                          │
│                    │  - Cross-Shard   │                          │
│                    │    Edge Tracking │                          │
│                    └──────────────────┘                          │
│                            │                                     │
│          ┌─────────────────┼─────────────────┐                  │
│          │                 │                 │                  │
│          ▼                 ▼                 ▼                  │
│    ┌──────────┐      ┌──────────┐      ┌──────────┐            │
│    │Worker    │      │Worker    │      │Worker    │            │
│    │Shard 1   │      │Shard 2   │      │Shard 3   │            │
│    │          │      │          │      │          │            │
│    │Data:     │      │Data:     │      │Data:     │            │
│    │Chunks    │      │Chunks    │      │Chunks    │            │
│    │A-F       │      │G-M       │      │N-Z       │            │
│    └──────────┘      └──────────┘      └──────────┘            │
└─────────────────────────────────────────────────────────────────┘
```

### Hub-Shard Konfiguration

```yaml
# config/sharding/hub-shard.yaml
sharding:
  topology: hub_and_spoke
  
  # Hub-Shard (Coordinator Knoten)
  hub_shard:
    shard_id: "hub_001"
    endpoint: "themis-hub.cluster.local:8080"
    
    # Spezielle Capabilities
    capabilities:
      - query_coordination    # Kann Queries auf Worker-Shards verteilen
      - urn_resolution       # Globale URN → Shard Resolution
      - result_aggregation   # Merge results from workers
      - cross_shard_join     # Cross-shard JOIN operations
      - graph_expansion      # Multi-hop graph traversal
      - hybrid_search        # Text+Vector fusion across shards
    
    # Keine eigenen Daten (oder nur Metadaten)
    data_storage: metadata_only
    
    # Cache für häufige URN → Shard Mappings
    urn_cache:
      enabled: true
      max_entries: 100000
      ttl_seconds: 300
    
    # Cross-shard edge index
    edge_index:
      enabled: true
      # Tracks edges that cross shard boundaries
      # Format: {source_urn → target_urn → target_shard}
      storage: in_memory  # oder redis, etcd
  
  # Worker-Shards (Data Knoten)
  worker_shards:
    - shard_id: "worker_001"
      endpoint: "themis-worker-001.cluster.local:8080"
      capabilities: [read, write, local_search, local_graph]
      data_partitions:
        - namespace: "chunks"
          hash_range: "0x0000000000000000-0x5555555555555555"
    
    - shard_id: "worker_002"
      endpoint: "themis-worker-002.cluster.local:8080"
      capabilities: [read, write, local_search, local_graph]
      data_partitions:
        - namespace: "chunks"
          hash_range: "0x5555555555555556-0xAAAAAAAAAAAAAAAA"
    
    - shard_id: "worker_003"
      endpoint: "themis-worker-003.cluster.local:8080"
      capabilities: [read, write, local_search, local_graph]
      data_partitions:
        - namespace: "chunks"
          hash_range: "0xAAAAAAAAAAAAAAAAB-0xFFFFFFFFFFFFFFFF"
```

### Hub-Shard Implementation

```cpp
// include/sharding/hub_shard.h

namespace themis::sharding {

/**
 * Hub-Shard: Coordination layer for distributed queries
 * 
 * Responsibilities:
 * 1. Query Planning - Determine which worker shards to query
 * 2. Query Distribution - Send sub-queries to workers
 * 3. Result Aggregation - Merge results from workers
 * 4. Cross-Shard Resolution - Resolve URN references across shards
 */
class HubShard {
public:
    struct Config {
        std::string hub_shard_id;
        
        // URN cache for fast shard lookup
        bool enable_urn_cache = true;
        size_t urn_cache_size = 100000;
        std::chrono::seconds urn_cache_ttl{300};
        
        // Cross-shard edge tracking
        bool enable_edge_index = true;
        
        // Query optimization
        bool enable_query_pushdown = true;
        bool enable_partial_results = false;  // Return partial on worker failure
    };
    
    HubShard(
        std::shared_ptr<ShardTopology> topology,
        std::shared_ptr<ConsistentHashRing> hash_ring,
        std::shared_ptr<RemoteExecutor> executor,
        const Config& config
    );
    
    /**
     * Execute distributed graph search
     * 
     * @param urn_pattern Pattern to match (e.g., "urn:themis:graph:docs:chunks:*")
     * @param hops Number of hops to traverse
     * @param edge_types Edge types to follow
     * @return Merged graph results
     */
    nlohmann::json executeGraphSearch(
        std::string_view urn_pattern,
        uint32_t hops,
        const std::vector<std::string>& edge_types
    );
    
    /**
     * Execute distributed hybrid search (Text + Vector + Graph)
     * 
     * @param params Hybrid search parameters
     * @return Merged and re-ranked results
     */
    nlohmann::json executeHybridSearch(
        const HybridSearchParams& params
    );
    
    /**
     * Register cross-shard edge
     * Called when a worker discovers an edge to a node on another shard
     * 
     * @param source_urn Source node URN
     * @param edge_type Edge type (parent, next, geo, etc.)
     * @param target_urn Target node URN
     * @param target_shard Shard where target node lives
     */
    void registerCrossShardEdge(
        const URN& source_urn,
        std::string_view edge_type,
        const URN& target_urn,
        std::string_view target_shard
    );
    
    /**
     * Resolve URN to shard (with caching)
     * 
     * @param urn URN to resolve
     * @return Shard info
     */
    std::optional<ShardInfo> resolveURN(const URN& urn);
    
private:
    std::shared_ptr<ShardTopology> topology_;
    std::shared_ptr<ConsistentHashRing> hash_ring_;
    std::shared_ptr<RemoteExecutor> executor_;
    Config config_;
    
    // URN → Shard cache
    mutable std::unordered_map<std::string, ShardInfo> urn_cache_;
    mutable std::mutex urn_cache_mutex_;
    
    // Cross-shard edge index
    // source_urn -> [(edge_type, target_urn, target_shard)]
    std::unordered_map<
        std::string,
        std::vector<std::tuple<std::string, std::string, std::string>>
    > cross_shard_edges_;
    mutable std::shared_mutex edge_index_mutex_;
    
    /**
     * Determine which worker shards contain data matching URN pattern
     */
    std::vector<ShardInfo> getRelevantShards(std::string_view urn_pattern);
    
    /**
     * Merge graph results from multiple workers
     */
    nlohmann::json mergeGraphResults(
        const std::vector<nlohmann::json>& worker_results
    );
    
    /**
     * Merge hybrid search results (Text + Vector + Graph)
     */
    nlohmann::json mergeHybridResults(
        const std::vector<nlohmann::json>& worker_results,
        const HybridSearchParams& params
    );
    
    /**
     * Follow cross-shard edges during graph traversal
     */
    void followCrossShardEdges(
        nlohmann::json& graph_result,
        uint32_t remaining_hops,
        const std::vector<std::string>& edge_types
    );
};

/**
 * Hybrid search parameters
 */
struct HybridSearchParams {
    // Text search
    std::optional<std::string> text_query;
    std::optional<std::string> text_column;
    
    // Vector search
    std::optional<std::vector<float>> vector_query;
    
    // Graph expansion
    std::optional<uint32_t> graph_hops;
    std::vector<std::string> graph_edge_types;
    
    // Fusion parameters
    std::string fusion_mode = "rrf";  // rrf or weighted
    float weight_text = 0.5;
    float weight_vector = 0.3;
    float weight_graph = 0.2;
    int k_rrf = 60;
    
    // Result limits
    int k = 10;
    int text_limit = 1000;
    int vector_limit = 1000;
    int graph_limit = 1000;
};

} // namespace themis::sharding
```

---

## Hybrid Search über Shards: Implementierung

### Beispiel: Text + Vector + Graph Fusion

```cpp
nlohmann::json HubShard::executeHybridSearch(
    const HybridSearchParams& params
) {
    auto start_time = std::chrono::steady_clock::now();
    
    // 1. Determine relevant shards
    //    For global hybrid search, query all shards
    auto worker_shards = topology_->getHealthyShards();
    
    // 2. Build sub-queries for each worker
    std::vector<std::future<nlohmann::json>> futures;
    
    for (const auto& shard : worker_shards) {
        futures.push_back(std::async([&, shard]() {
            // Each worker performs local hybrid search
            nlohmann::json local_query = {
                {"type", "hybrid_search"},
                {"params", {
                    {"text_query", params.text_query.value_or("")},
                    {"text_column", params.text_column.value_or("")},
                    {"vector_query", params.vector_query.value_or(std::vector<float>{})},
                    {"graph_hops", params.graph_hops.value_or(0)},
                    {"graph_edge_types", params.graph_edge_types},
                    {"k", params.text_limit}  // Over-fetch for better merge
                }}
            };
            
            return executor_->executeQuery(shard, local_query.dump());
        }));
    }
    
    // 3. Collect results from all workers
    std::vector<nlohmann::json> worker_results;
    for (auto& future : futures) {
        try {
            worker_results.push_back(future.get());
        } catch (const std::exception& e) {
            // Log error, continue with partial results if enabled
            // If partial results are disabled, the exception will be re-thrown
            // causing the entire query to fail
            if (!config_.enable_partial_results) {
                throw;
            }
        }
    }
    
    // 4. Merge and re-rank results
    auto merged = mergeHybridResults(worker_results, params);
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time
    ).count();
    
    merged["hub_shard_id"] = config_.hub_shard_id;
    merged["worker_shards_queried"] = worker_shards.size();
    merged["execution_time_ms"] = duration_ms;
    
    return merged;
}

nlohmann::json HubShard::mergeHybridResults(
    const std::vector<nlohmann::json>& worker_results,
    const HybridSearchParams& params
) {
    // Helper struct for tracking scores across modalities
    // Note: Defined here for simplicity; could be moved to class level for reusability
    struct HybridScore {
        std::string urn;
        float text_score = 0.0;
        float vector_score = 0.0;
        float graph_score = 0.0;
        int text_rank = INT_MAX;
        int vector_rank = INT_MAX;
        int graph_rank = INT_MAX;
        nlohmann::json data;
    };
    
    // Collect all results from workers
    std::unordered_map<std::string, HybridScore> results_map;
    
    // 1. Aggregate scores from all workers
    for (size_t worker_idx = 0; worker_idx < worker_results.size(); ++worker_idx) {
        const auto& result = worker_results[worker_idx];
        
        if (!result.contains("results") || !result["results"].is_array()) {
            continue;
        }
        
        int rank = 0;
        for (const auto& item : result["results"]) {
            std::string urn = item["urn"];
            
            auto& score = results_map[urn];
            score.urn = urn;
            
            // Accumulate scores from different modalities
            // Using max() to take best score across shards (assumes normalized scores)
            // Rationale: A document appearing in multiple shards should get the
            // highest score it received in any shard
            if (item.contains("text_score")) {
                score.text_score = std::max(score.text_score, 
                                           item["text_score"].get<float>());
                score.text_rank = std::min(score.text_rank, rank);
            }
            if (item.contains("vector_score")) {
                score.vector_score = std::max(score.vector_score, 
                                             item["vector_score"].get<float>());
                score.vector_rank = std::min(score.vector_rank, rank);
            }
            if (item.contains("graph_score")) {
                score.graph_score = std::max(score.graph_score, 
                                            item["graph_score"].get<float>());
                score.graph_rank = std::min(score.graph_rank, rank);
            }
            
            score.data = item;
            rank++;
        }
    }
    
    // 2. Apply fusion algorithm
    std::vector<std::pair<std::string, float>> final_scores;
    
    // Pre-compute reciprocal for RRF to avoid division in loop
    const float rrf_denominator_base = static_cast<float>(params.k_rrf);
    
    for (const auto& [urn, score] : results_map) {
        float final_score = 0.0;
        
        if (params.fusion_mode == "rrf") {
            // Reciprocal Rank Fusion (optimized with pre-computed base)
            if (score.text_rank != INT_MAX) {
                final_score += params.weight_text / (rrf_denominator_base + score.text_rank);
            }
            if (score.vector_rank != INT_MAX) {
                final_score += params.weight_vector / (rrf_denominator_base + score.vector_rank);
            }
            if (score.graph_rank != INT_MAX) {
                final_score += params.weight_graph / (rrf_denominator_base + score.graph_rank);
            }
        } else {
            // Weighted score fusion
            final_score = params.weight_text * score.text_score +
                         params.weight_vector * score.vector_score +
                         params.weight_graph * score.graph_score;
        }
        
        final_scores.push_back({urn, final_score});
    }
    
    // 3. Sort by final score
    std::sort(final_scores.begin(), final_scores.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // 4. Build final result
    nlohmann::json merged;
    merged["fusion_mode"] = params.fusion_mode;
    merged["total_candidates"] = final_scores.size();
    merged["k"] = params.k;
    
    nlohmann::json results = nlohmann::json::array();
    for (size_t i = 0; i < std::min(static_cast<size_t>(params.k), 
                                     final_scores.size()); ++i) {
        const auto& [urn, score] = final_scores[i];
        const auto& item_data = results_map[urn].data;
        
        nlohmann::json item;
        item["urn"] = urn;
        item["score"] = score;
        item["rank"] = i;
        item["data"] = item_data;
        
        results.push_back(item);
    }
    
    merged["results"] = results;
    return merged;
}
```

---

## Graph-Suche mit Cross-Shard Edges

### Edge-Tracking zwischen Shards

```cpp
void HubShard::registerCrossShardEdge(
    const URN& source_urn,
    std::string_view edge_type,
    const URN& target_urn,
    std::string_view target_shard
) {
    std::unique_lock lock(edge_index_mutex_);
    
    std::string source_key = source_urn.toString();
    cross_shard_edges_[source_key].push_back({
        std::string(edge_type),
        target_urn.toString(),
        std::string(target_shard)
    });
}

void HubShard::followCrossShardEdges(
    nlohmann::json& graph_result,
    uint32_t remaining_hops,
    const std::vector<std::string>& edge_types
) {
    if (remaining_hops == 0) {
        return;
    }
    
    // Find all nodes that have cross-shard edges
    std::vector<std::pair<URN, ShardInfo>> to_fetch;
    
    {
        std::shared_lock lock(edge_index_mutex_);
        
        for (const auto& node : graph_result["nodes"]) {
            std::string node_urn = node["urn"];
            
            if (cross_shard_edges_.count(node_urn) == 0) {
                continue;
            }
            
            for (const auto& [edge_type, target_urn_str, target_shard] : 
                 cross_shard_edges_[node_urn]) {
                
                // Check if edge type is requested
                if (std::find(edge_types.begin(), edge_types.end(), edge_type) 
                    != edge_types.end()) {
                    
                    auto target_urn = URN::parse(target_urn_str);
                    ShardInfo shard_info;
                    shard_info.shard_id = target_shard;
                    
                    to_fetch.push_back({*target_urn, shard_info});
                }
            }
        }
    }
    
    // Fetch nodes from other shards
    std::vector<std::future<nlohmann::json>> futures;
    for (const auto& [urn, shard] : to_fetch) {
        // Capture loop variables by value to avoid race condition
        futures.push_back(std::async([this, urn, shard]() {
            // Fetch node by URN from remote shard
            return executor_->fetchNodeByURN(shard, urn);
        }));
    }
    
    // Add fetched nodes to result
    for (auto& future : futures) {
        try {
            auto node = future.get();
            graph_result["nodes"].push_back(node);
        } catch (const std::exception& e) {
            // Log and continue
        }
    }
    
    // Recursively follow edges
    if (remaining_hops > 1) {
        followCrossShardEdges(graph_result, remaining_hops - 1, edge_types);
    }
}
```

---

## Referenzierung entfernter Nodes in Graph-Entities

### Speicherformat für Cross-Shard Referenzen

Graph-Entities speichern Referenzen zu entfernten Nodes (auf anderen Shards) durch **URN-basierte Edge-Referenzen**. Jede Edge enthält die vollständige URN des Zielnodes, wodurch der Hub-Shard die Shard-Location auflösen kann.

#### Graph-Entity-Struktur mit Remote References

```json
{
  "urn": "urn:themis:graph:docs:chunks:abc-123-on-shard-001",
  "shard": "worker_001",
  "type": "chunk",
  "data": {
    "content": "Machine learning is...",
    "embedding": [0.1, 0.2, ..., 0.768]
  },
  "edges": [
    {
      "type": "parent",
      "target_urn": "urn:themis:graph:docs:document:doc-456-on-shard-001",
      "local": true,
      "target_shard": "worker_001"
    },
    {
      "type": "next",
      "target_urn": "urn:themis:graph:docs:chunks:def-789-on-shard-002",
      "local": false,
      "target_shard": "worker_002"
    },
    {
      "type": "geo",
      "target_urn": "urn:themis:graph:docs:chunks:ghi-234-on-shard-003",
      "local": false,
      "target_shard": "worker_003"
    }
  ]
}
```

**Wichtige Felder:**
- `target_urn`: Vollständige URN des Zielnodes (globale Eindeutigkeit)
- `local`: Boolean - ob Zielnode auf gleichem Shard liegt
- `target_shard`: Shard-ID wo Zielnode gespeichert ist (optional, für Optimierung)

### Auflösung von Remote-Referenzen

```cpp
/**
 * Graph entity mit Cross-Shard Edge-Referenzen
 */
struct GraphEntity {
    std::string urn;           // Eigene URN
    std::string shard_id;      // Shard wo diese Entity liegt
    std::string type;          // Entity-Typ (chunk, document, etc.)
    nlohmann::json data;       // Entity-Daten
    
    struct Edge {
        std::string type;           // Edge-Typ (parent, next, geo)
        std::string target_urn;     // URN des Zielnodes (kann remote sein)
        bool is_local;              // true wenn auf gleichem Shard
        std::string target_shard;   // Shard-ID des Zielnodes
    };
    
    std::vector<Edge> edges;
};

/**
 * Auflösung einer Remote-Referenz
 */
nlohmann::json resolveRemoteReference(const GraphEntity::Edge& edge) {
    if (edge.is_local) {
        // Lokaler Zugriff auf gleichem Shard
        return local_storage_->getNode(edge.target_urn);
    } else {
        // Remote-Zugriff über Hub-Shard
        auto target_urn = URN::parse(edge.target_urn);
        auto shard_info = hub_shard_->resolveURN(*target_urn);
        
        // Fetch von remote Shard
        return remote_executor_->fetchNodeByURN(shard_info, *target_urn);
    }
}
```

### Speicherung in der Datenbank

Auf jedem Worker-Shard werden Graph-Entities mit **URN als Primary Key** gespeichert:

```cpp
// Worker Shard Storage Schema
namespace themis::storage {

class GraphEntityStore {
public:
    /**
     * Speichere Graph-Entity mit Edges
     * Edges können zu lokalen oder remote Nodes zeigen
     */
    Status putEntity(const GraphEntity& entity) {
        // Serialisiere Entity mit allen Edges
        nlohmann::json entity_json = {
            {"urn", entity.urn},
            {"shard", entity.shard_id},
            {"type", entity.type},
            {"data", entity.data},
            {"edges", nlohmann::json::array()}
        };
        
        // Speichere jede Edge mit vollständiger URN-Referenz
        for (const auto& edge : entity.edges) {
            entity_json["edges"].push_back({
                {"type", edge.type},
                {"target_urn", edge.target_urn},  // Vollständige URN!
                {"local", edge.is_local},
                {"target_shard", edge.target_shard}
            });
        }
        
        // Speichere in RocksDB mit URN als Key
        return db_->Put(entity.urn, entity_json.dump());
    }
    
    /**
     * Lade Graph-Entity mit allen Edge-Referenzen
     */
    std::optional<GraphEntity> getEntity(const URN& urn) {
        std::string value;
        auto status = db_->Get(urn.toString(), &value);
        
        if (!status.ok()) {
            return std::nullopt;
        }
        
        auto json = nlohmann::json::parse(value);
        GraphEntity entity;
        entity.urn = json["urn"];
        entity.shard_id = json["shard"];
        entity.type = json["type"];
        entity.data = json["data"];
        
        // Parse Edges (können remote sein)
        for (const auto& edge_json : json["edges"]) {
            GraphEntity::Edge edge;
            edge.type = edge_json["type"];
            edge.target_urn = edge_json["target_urn"];  // URN des remote Nodes
            edge.is_local = edge_json.value("local", false);
            edge.target_shard = edge_json.value("target_shard", "");
            entity.edges.push_back(edge);
        }
        
        return entity;
    }
};

} // namespace themis::storage
```

### Hub-Shard Edge-Index

Der Hub-Shard verwaltet einen **globalen Index** aller Cross-Shard Edges für effiziente Traversierung:

```cpp
// Hub-Shard Edge Index Format
std::unordered_map<std::string, std::vector<CrossShardEdge>> edge_index_;

struct CrossShardEdge {
    std::string source_urn;      // URN des Quellnodes
    std::string source_shard;    // Shard wo Quellnode liegt
    std::string edge_type;       // Edge-Typ (parent, next, geo)
    std::string target_urn;      // URN des Zielnodes (remote)
    std::string target_shard;    // Shard wo Zielnode liegt
};

// Beispiel-Eintrag:
edge_index_["urn:themis:graph:docs:chunks:abc-123"] = [
    {
        source_urn: "urn:themis:graph:docs:chunks:abc-123",
        source_shard: "worker_001",
        edge_type: "next",
        target_urn: "urn:themis:graph:docs:chunks:def-789",
        target_shard: "worker_002"
    }
];
```

### Beispiel: Graph-Traversierung mit Remote Nodes

```cpp
// Client-Code: Graph-Traversierung über Shard-Grenzen
auto start_urn = URN::parse("urn:themis:graph:docs:chunks:abc-123");

// 1. Hole Start-Node (kann auf beliebigem Shard sein)
auto start_node = hub_shard_->resolveAndFetchNode(start_urn);

// 2. Traverse Edges (einige davon sind remote)
for (const auto& edge : start_node["edges"]) {
    std::string target_urn_str = edge["target_urn"];
    bool is_local = edge["local"];
    
    if (is_local) {
        // Lokaler Node - direkt von Worker-Shard holen
        auto node = worker_shard_->getLocalNode(target_urn_str);
    } else {
        // Remote Node - über Hub-Shard auflösen
        auto target_urn = URN::parse(target_urn_str);
        auto node = hub_shard_->resolveAndFetchNode(target_urn);
    }
}
```

### Vorteile der URN-basierten Referenzierung

✅ **Globale Eindeutigkeit** - Jeder Node hat weltweit eindeutige URN  
✅ **Location Transparency** - Client muss Shard-Verteilung nicht kennen  
✅ **Resharding-fähig** - URNs bleiben bei Shard-Migration gleich  
✅ **Federation-ready** - URNs funktionieren cluster-übergreifend  
✅ **Type Safety** - URN-Schema enthält Model-Typ (graph, relational, vector)

---

## Link-Discovery bei der Ingestion

### Herausforderung: Cross-Shard Referenzen erkennen

Bei der Ingestion von Dokumenten stellt sich die Frage: **Wie erkennt ThemisDB, ob ein Dokument zu einer entfernten Entity (z.B. Behörde XY) referenziert werden muss?**

Zwei Hauptansätze:

1. **Client-gesteuerte Link-Deklaration** (sofort verfügbar)
2. **Shard-übergreifende Link-Discovery** (asynchrone Hintergrundaufgabe)

### Ansatz 1: Client-gesteuerte Link-Deklaration

Der Client liefert Link-Informationen bereits beim HTTP-Endpoint:

```bash
# Ingestion mit expliziten Cross-Shard Links
POST /api/v1/data/ingest
{
  "document": {
    "urn": "urn:themis:graph:docs:document:doc-123",
    "type": "administrative_document",
    "data": {
      "title": "Antrag für Behörde XY",
      "content": "..."
    }
  },
  
  # Explizite Referenzen zu anderen Entities
  "links": [
    {
      "type": "belongs_to_authority",
      "target_urn": "urn:themis:hierarchy:government:institutional:de_bmf:uuid-xyz",
      "metadata": {
        "relationship": "submission",
        "timestamp": "2025-12-13T10:00:00Z"
      }
    },
    {
      "type": "references_case",
      "target_urn": "urn:themis:graph:cases:case:case-456",
      "metadata": {
        "case_number": "C-2025-456"
      }
    }
  ]
}
```

**Ablauf:**

```cpp
// HTTP Endpoint Handler
Status IngestDocument(const IngestionRequest& request) {
    // 1. Parse URN und bestimme Ziel-Shard für Dokument
    auto doc_urn = URN::parse(request.document.urn);
    auto target_shard = urn_resolver_->resolvePrimary(*doc_urn);
    
    // 2. Speichere Dokument auf Ziel-Shard
    auto doc_result = writeDocumentToShard(target_shard, request.document);
    
    // 3. Verarbeite explizite Links
    for (const auto& link : request.links) {
        auto link_target_urn = URN::parse(link.target_urn);
        auto link_target_shard = urn_resolver_->resolvePrimary(*link_target_urn);
        
        // 3a. Erstelle Edge vom Dokument zum Ziel
        GraphEntity::Edge edge{
            .type = link.type,
            .target_urn = link.target_urn,
            .is_local = (target_shard.shard_id == link_target_shard.shard_id),
            .target_shard = link_target_shard.shard_id
        };
        
        // 3b. Speichere Edge im Dokument
        addEdgeToEntity(target_shard, doc_urn, edge);
        
        // 3c. Falls Cross-Shard: Registriere im Hub-Shard Edge-Index
        if (!edge.is_local) {
            hub_shard_->registerCrossShardEdge(
                *doc_urn, link.type, *link_target_urn, link_target_shard.shard_id
            );
        }
        
        // 3d. Optional: Erstelle Rück-Referenz (bidirektional)
        if (link.metadata.contains("bidirectional") && 
            link.metadata["bidirectional"] == true) {
            GraphEntity::Edge reverse_edge{
                .type = "referenced_by",
                .target_urn = request.document.urn,
                .is_local = !edge.is_local,
                .target_shard = target_shard.shard_id
            };
            addEdgeToEntity(link_target_shard, link_target_urn, reverse_edge);
        }
    }
    
    return Status::OK();
}
```

**Vorteile:**
- ✅ Sofort verfügbar bei Ingestion
- ✅ Keine zusätzliche Discovery-Logik nötig
- ✅ Client hat vollständige Kontrolle über Links
- ✅ Deterministisch und vorhersagbar

**Nachteile:**
- ❌ Client muss URNs kennen
- ❌ Keine automatische Link-Erkennung

### Ansatz 2: Asynchrone Link-Discovery (Hintergrundaufgabe)

Shards tauschen Informationen über relevante URNs aus und entdecken Links automatisch:

```cpp
/**
 * Link Discovery Service (läuft als Hintergrund-Task)
 * Niedrige Priorität, dauerhaft laufend
 */
class LinkDiscoveryService {
public:
    struct Config {
        std::chrono::seconds scan_interval{300};  // Alle 5 Minuten
        size_t batch_size = 100;                  // Dokumente pro Scan
        bool enable_nlp_extraction = true;        // NLP-basierte Link-Extraktion
        bool enable_urn_scanning = true;          // URN-Pattern-Scanning in Text
    };
    
    /**
     * Hauptschleife: Scanne neue Dokumente auf potenzielle Links
     */
    void run() {
        while (!should_stop_) {
            // 1. Hole neue/unverarbeitete Dokumente vom lokalen Shard
            auto unprocessed_docs = getUnprocessedDocuments(config_.batch_size);
            
            for (const auto& doc : unprocessed_docs) {
                // 2. Extrahiere potenzielle URN-Referenzen aus Dokument
                auto potential_links = extractPotentialLinks(doc);
                
                // 3. Validiere Links (prüfe ob URNs existieren)
                auto validated_links = validateLinks(potential_links);
                
                // 4. Registriere validierte Links
                for (const auto& link : validated_links) {
                    registerDiscoveredLink(doc.urn, link);
                }
                
                // 5. Markiere Dokument als verarbeitet
                markAsProcessed(doc.urn);
            }
            
            std::this_thread::sleep_for(config_.scan_interval);
        }
    }
    
private:
    /**
     * Extrahiere potenzielle Links aus Dokument-Content
     */
    std::vector<PotentialLink> extractPotentialLinks(const Document& doc) {
        std::vector<PotentialLink> links;
        
        // Methode 1: URN-Pattern Matching im Text
        if (config_.enable_urn_scanning) {
            // Regex: urn:themis:.*
            std::regex urn_pattern(R"(urn:themis:[a-z]+:[a-z_]+:[a-z_]+:[a-f0-9\-]+)");
            std::smatch matches;
            std::string content = doc.data["content"];
            
            auto it = content.cbegin();
            while (std::regex_search(it, content.cend(), matches, urn_pattern)) {
                std::string found_urn = matches[0];
                links.push_back({
                    .target_urn = found_urn,
                    .type = "references",
                    .confidence = 0.9,
                    .extraction_method = "urn_pattern"
                });
                it = matches.suffix().first;
            }
        }
        
        // Methode 2: NLP-basierte Entity-Extraktion
        if (config_.enable_nlp_extraction) {
            // Extrahiere Named Entities (Behörden, Personen, Orte)
            auto entities = nlp_extractor_->extractEntities(doc.data["content"]);
            
            for (const auto& entity : entities) {
                if (entity.type == "ORGANIZATION" || entity.type == "AUTHORITY") {
                    // Versuche URN für bekannte Behörden zu finden
                    auto urn = lookupAuthorityURN(entity.text);
                    if (urn.has_value()) {
                        links.push_back({
                            .target_urn = *urn,
                            .type = "mentions_authority",
                            .confidence = entity.confidence,
                            .extraction_method = "nlp_entity"
                        });
                    }
                }
            }
        }
        
        // Methode 3: Metadata-basierte Links
        if (doc.data.contains("metadata")) {
            auto& meta = doc.data["metadata"];
            
            // Beispiel: "authority_id" → URN-Lookup
            if (meta.contains("authority_id")) {
                std::string authority_id = meta["authority_id"];
                auto urn = lookupAuthorityURNById(authority_id);
                if (urn.has_value()) {
                    links.push_back({
                        .target_urn = *urn,
                        .type = "belongs_to_authority",
                        .confidence = 1.0,
                        .extraction_method = "metadata"
                    });
                }
            }
        }
        
        return links;
    }
    
    /**
     * Validiere ob URNs tatsächlich existieren
     * Fragt Hub-Shard oder target Shards
     */
    std::vector<ValidatedLink> validateLinks(
        const std::vector<PotentialLink>& potential_links
    ) {
        std::vector<ValidatedLink> validated;
        
        for (const auto& link : potential_links) {
            auto target_urn = URN::parse(link.target_urn);
            if (!target_urn.has_value()) {
                continue;  // Ungültige URN
            }
            
            // Prüfe ob URN existiert (leichtgewichtige Existenz-Prüfung)
            auto exists = hub_shard_->checkURNExists(*target_urn);
            
            if (exists) {
                auto target_shard = hub_shard_->resolveURN(*target_urn);
                validated.push_back({
                    .target_urn = link.target_urn,
                    .target_shard = target_shard->shard_id,
                    .type = link.type,
                    .confidence = link.confidence,
                    .method = link.extraction_method
                });
            }
        }
        
        return validated;
    }
    
    /**
     * Lookup Authority URN by Name
     * Cache für häufige Behörden-Namen → URN Mapping
     */
    std::optional<std::string> lookupAuthorityURN(std::string_view authority_name) {
        // Cache-Lookup
        auto cached = authority_cache_.find(std::string(authority_name));
        if (cached != authority_cache_.end()) {
            return cached->second;
        }
        
        // Query Hub-Shard: Suche nach Behörde mit diesem Namen
        nlohmann::json query = {
            {"type", "hierarchy_search"},
            {"hierarchy_id", "government"},
            {"level", "institutional"},
            {"filter", {
                {"name", authority_name}
            }}
        };
        
        auto result = hub_shard_->executeQuery(query.dump());
        if (result.contains("results") && !result["results"].empty()) {
            std::string urn = result["results"][0]["urn"];
            authority_cache_[std::string(authority_name)] = urn;
            return urn;
        }
        
        return std::nullopt;
    }
};
```

### Shard-übergreifender Link-Austausch

Shards können sich gegenseitig über relevante URNs informieren:

```cpp
/**
 * Shard-to-Shard Link-Notification
 */
class ShardLinkExchange {
public:
    /**
     * Worker-Shard meldet neu entdeckte Links an Hub-Shard
     */
    void notifyLinkDiscovered(
        const URN& source_urn,
        const URN& target_urn,
        std::string_view link_type,
        float confidence
    ) {
        LinkNotification notification{
            .source_urn = source_urn.toString(),
            .target_urn = target_urn.toString(),
            .link_type = std::string(link_type),
            .confidence = confidence,
            .timestamp = std::chrono::system_clock::now(),
            .source_shard = local_shard_id_
        };
        
        // Sende an Hub-Shard zur zentralen Registrierung
        hub_shard_client_->sendLinkNotification(notification);
    }
    
    /**
     * Hub-Shard empfängt Link-Notification von Worker
     */
    void handleLinkNotification(const LinkNotification& notification) {
        auto source_urn = URN::parse(notification.source_urn);
        auto target_urn = URN::parse(notification.target_urn);
        
        if (!source_urn || !target_urn) {
            return;  // Ungültige URNs
        }
        
        auto source_shard = resolveURN(*source_urn);
        auto target_shard = resolveURN(*target_urn);
        
        // Registriere Cross-Shard Edge wenn Shards unterschiedlich
        if (source_shard->shard_id != target_shard->shard_id) {
            registerCrossShardEdge(
                *source_urn, 
                notification.link_type, 
                *target_urn,
                target_shard->shard_id
            );
        }
        
        // Optional: Benachrichtige beide Shards über den Link
        if (notification.confidence >= 0.8) {  // Nur hohe Konfidenz
            notifyShardAboutLink(source_shard->shard_id, notification);
            notifyShardAboutLink(target_shard->shard_id, notification);
        }
    }
};
```

### Konfiguration: Hybrid-Ansatz

Best Practice: Kombination beider Ansätze:

```yaml
# config/link_discovery.yaml
link_discovery:
  # Client-gesteuert: Sofort verfügbar
  client_declared_links:
    enabled: true
    require_validation: true  # Prüfe ob target URN existiert
    
  # Automatische Discovery: Hintergrundaufgabe
  automatic_discovery:
    enabled: true
    priority: low
    scan_interval_seconds: 300
    batch_size: 100
    
    # Extraktions-Methoden
    extraction_methods:
      urn_pattern_scanning:
        enabled: true
        confidence_threshold: 0.9
        
      nlp_entity_extraction:
        enabled: true
        confidence_threshold: 0.7
        model: "de_core_news_lg"  # Spacy German model
        
      metadata_mapping:
        enabled: true
        confidence_threshold: 1.0
        mappings:
          - field: "authority_id"
            target_type: "government_institution"
            
    # Cache für häufige Lookups
    cache:
      authority_name_to_urn:
        enabled: true
        max_entries: 10000
        ttl_seconds: 3600
```

### Deployment-Beispiel

```yaml
# kubernetes/link-discovery-service.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: themisdb-link-discovery
spec:
  replicas: 2  # Niedrige Priorität, wenige Replicas
  template:
    spec:
      containers:
      - name: link-discovery
        image: themisdb:latest
        env:
        - name: SERVICE_TYPE
          value: "link_discovery"
        - name: SCAN_INTERVAL_SECONDS
          value: "300"
        - name: BATCH_SIZE
          value: "100"
        resources:
          requests:
            cpu: "100m"      # Niedrige CPU (Hintergrund-Task)
            memory: "256Mi"
          limits:
            cpu: "500m"
            memory: "512Mi"
```

### Monitoring: Link-Discovery Metriken

```
# Link-Discovery Metriken
themisdb_link_discovery_documents_scanned_total = 15420
themisdb_link_discovery_links_found_total = 892
themisdb_link_discovery_links_validated_total = 734
themisdb_link_discovery_cross_shard_links_total = 245

# Nach Extraktions-Methode
themisdb_link_discovery_links_by_method{method="urn_pattern"} = 456
themisdb_link_discovery_links_by_method{method="nlp_entity"} = 189
themisdb_link_discovery_links_by_method{method="metadata"} = 89

# Konfidenz-Verteilung
themisdb_link_discovery_confidence_bucket{le="0.5"} = 23
themisdb_link_discovery_confidence_bucket{le="0.7"} = 156
themisdb_link_discovery_confidence_bucket{le="0.9"} = 512
themisdb_link_discovery_confidence_bucket{le="1.0"} = 734

# Graph-Topologie Metriken: Incoming/Outgoing Edges
themisdb_graph_node_incoming_edges{urn="urn:themis:hierarchy:government:institutional:de_bmf:*"} = 1243
themisdb_graph_node_outgoing_edges{urn="urn:themis:graph:docs:document:doc-123"} = 5

# Degree-Verteilung (In-Degree)
themisdb_graph_indegree_bucket{le="1"} = 8934      # Wenig referenziert
themisdb_graph_indegree_bucket{le="10"} = 12450
themisdb_graph_indegree_bucket{le="100"} = 13892
themisdb_graph_indegree_bucket{le="1000"} = 14123  # Sehr viel referenziert
themisdb_graph_indegree_bucket{le="+Inf"} = 14234

# Top referenzierte Entities (Hub-Nodes)
themisdb_graph_top_referenced_entities{
  urn="urn:themis:hierarchy:government:institutional:de_bmf:uuid-1",
  type="authority",
  indegree="1243"
} = 1
```

### Graph-Topologie Analyse: Hub-Nodes und Leaf-Nodes

Der Link-Discovery-Service erfasst automatisch **Graph-Metriken** für jede Entity:

```cpp
/**
 * Graph Topology Metrics Tracker
 */
class GraphTopologyMetrics {
public:
    struct NodeMetrics {
        std::string urn;
        std::string type;
        int64_t indegree = 0;   // Anzahl eingehender Edges (wie oft referenziert)
        int64_t outdegree = 0;  // Anzahl ausgehender Edges (wie viele Referenzen)
        double centrality = 0.0; // PageRank-ähnliche Zentralität
        std::chrono::system_clock::time_point last_updated;
    };
    
    /**
     * Update Metrics wenn neue Edge registriert wird
     */
    void onEdgeRegistered(const URN& source_urn, const URN& target_urn) {
        std::unique_lock lock(metrics_mutex_);
        
        // Erhöhe OutDegree für Source
        auto& source_metrics = getOrCreateMetrics(source_urn);
        source_metrics.outdegree++;
        source_metrics.last_updated = std::chrono::system_clock::now();
        
        // Erhöhe InDegree für Target
        auto& target_metrics = getOrCreateMetrics(target_urn);
        target_metrics.indegree++;
        target_metrics.last_updated = std::chrono::system_clock::now();
        
        // Update Prometheus Metrics
        prometheus_indegree_->Set({{"urn", target_urn.toString()}}, 
                                  target_metrics.indegree);
        prometheus_outdegree_->Set({{"urn", source_urn.toString()}}, 
                                   source_metrics.outdegree);
        
        // Update Histogramme
        indegree_histogram_->Observe(target_metrics.indegree);
        outdegree_histogram_->Observe(source_metrics.outdegree);
    }
    
    /**
     * Finde Top-N referenzierte Entities (Hub-Nodes)
     */
    std::vector<NodeMetrics> getTopReferencedEntities(size_t top_n = 10) {
        std::shared_lock lock(metrics_mutex_);
        
        std::vector<NodeMetrics> all_nodes;
        for (const auto& [urn, metrics] : node_metrics_) {
            all_nodes.push_back(metrics);
        }
        
        // Sortiere nach InDegree (absteigend)
        std::partial_sort(
            all_nodes.begin(), 
            all_nodes.begin() + std::min(top_n, all_nodes.size()),
            all_nodes.end(),
            [](const NodeMetrics& a, const NodeMetrics& b) {
                return a.indegree > b.indegree;
            }
        );
        
        all_nodes.resize(std::min(top_n, all_nodes.size()));
        return all_nodes;
    }
    
    /**
     * Finde Entities mit wenigen Referenzen (Leaf-Nodes)
     */
    std::vector<NodeMetrics> getLowReferencedEntities(
        int64_t max_indegree = 5, 
        size_t limit = 100
    ) {
        std::shared_lock lock(metrics_mutex_);
        
        std::vector<NodeMetrics> low_referenced;
        for (const auto& [urn, metrics] : node_metrics_) {
            if (metrics.indegree <= max_indegree) {
                low_referenced.push_back(metrics);
            }
            if (low_referenced.size() >= limit) {
                break;
            }
        }
        
        return low_referenced;
    }
    
    /**
     * Export Metrics für Prometheus
     */
    nlohmann::json exportMetrics() {
        std::shared_lock lock(metrics_mutex_);
        
        nlohmann::json result;
        result["total_nodes"] = node_metrics_.size();
        
        // Statistiken
        int64_t total_indegree = 0;
        int64_t total_outdegree = 0;
        int64_t max_indegree = 0;
        int64_t max_outdegree = 0;
        
        for (const auto& [urn, metrics] : node_metrics_) {
            total_indegree += metrics.indegree;
            total_outdegree += metrics.outdegree;
            max_indegree = std::max(max_indegree, metrics.indegree);
            max_outdegree = std::max(max_outdegree, metrics.outdegree);
        }
        
        result["avg_indegree"] = static_cast<double>(total_indegree) / node_metrics_.size();
        result["avg_outdegree"] = static_cast<double>(total_outdegree) / node_metrics_.size();
        result["max_indegree"] = max_indegree;
        result["max_outdegree"] = max_outdegree;
        
        // Top referenzierte
        auto top_referenced = getTopReferencedEntities(10);
        nlohmann::json top_array = nlohmann::json::array();
        for (const auto& node : top_referenced) {
            top_array.push_back({
                {"urn", node.urn},
                {"type", node.type},
                {"indegree", node.indegree}
            });
        }
        result["top_referenced"] = top_array;
        
        return result;
    }
    
private:
    std::unordered_map<std::string, NodeMetrics> node_metrics_;
    mutable std::shared_mutex metrics_mutex_;
    
    std::shared_ptr<prometheus::Gauge> prometheus_indegree_;
    std::shared_ptr<prometheus::Gauge> prometheus_outdegree_;
    std::shared_ptr<prometheus::Histogram> indegree_histogram_;
    std::shared_ptr<prometheus::Histogram> outdegree_histogram_;
};
```

### API: Graph-Topologie Abfragen

```bash
# API: Top referenzierte Entities (Hub-Nodes)
GET /api/v1/graph/topology/top-referenced?limit=10

Response:
{
  "total_nodes": 14234,
  "avg_indegree": 3.2,
  "max_indegree": 1243,
  "top_referenced": [
    {
      "urn": "urn:themis:hierarchy:government:institutional:de_bmf:uuid-1",
      "type": "authority",
      "indegree": 1243,
      "outdegree": 15,
      "centrality": 0.89
    },
    {
      "urn": "urn:themis:graph:docs:policy:policy-456",
      "type": "policy_document",
      "indegree": 892,
      "outdegree": 23,
      "centrality": 0.76
    }
  ]
}

# API: Wenig referenzierte Entities (Leaf-Nodes)
GET /api/v1/graph/topology/low-referenced?max_indegree=2&limit=100

Response:
{
  "total_matching": 8934,
  "returned": 100,
  "low_referenced": [
    {
      "urn": "urn:themis:graph:docs:document:doc-9876",
      "type": "document",
      "indegree": 0,
      "outdegree": 3
    },
    {
      "urn": "urn:themis:graph:docs:chunk:chunk-5432",
      "type": "chunk",
      "indegree": 1,
      "outdegree": 2
    }
  ]
}
```

### Grafana Dashboard: Shard Network Monitoring

Ein umfassendes Grafana-Dashboard für Shard-Netzwerk-Monitoring ist verfügbar:

**Location:** `deploy/kubernetes/monitoring/grafana-dashboards/themisdb-shard-network-dashboard.json`

**Dashboard-Funktionen:**
- **Hub-Shard Network Overview**: Aktive Shards, Cross-Shard Edges, URN Cache
- **Cross-Shard Query Performance**: Query-Raten, Latenz, Fanout-Verteilung
- **Link Discovery**: Tracking nach Extraktions-Methode, Konfidenz-Verteilung
- **Graph Topology Analysis**: Top-10 Hub-Nodes, InDegree-Distribution, Orphaned Documents
- **Shard Communication**: Netzwerk-Traffic, Routing-Pattern, Error-Raten

**Import-Anleitung:**

```bash
# Method 1: Via Grafana UI
# 1. Navigate to Dashboards → Import
# 2. Upload themisdb-shard-network-dashboard.json
# 3. Select Prometheus data source

# Method 2: Via kubectl (Kubernetes)
kubectl apply -f deploy/kubernetes/monitoring/grafana-dashboards/themisdb-shard-network-dashboard.json

# Method 3: ConfigMap
kubectl create configmap themisdb-shard-network-dashboard \
  --from-file=deploy/kubernetes/monitoring/grafana-dashboards/themisdb-shard-network-dashboard.json \
  -n monitoring
```

**Dashboard-Panels:**

1. **Hub-Shard Network Overview**
   - Hub Shards Active
   - Worker Shards Active
   - Total Cross-Shard Edges
   - URN Cache Hit Rate
   - Avg Query Fanout

2. **Cross-Shard Query Performance**
   - Cross-Shard Query Rate (Graph, Hybrid, Scatter-Gather)
   - Cross-Shard Query Latency (P95, P99)
   - Scatter-Gather Fanout Distribution

3. **Link Discovery**
   - Link Discovery Rate by Method (URN Pattern, NLP, Metadata)
   - Link Discovery Progress (Scanned, Found, Validated, Cross-Shard)
   - Link Confidence Distribution

4. **Graph Topology Analysis**
   - Top 10 Referenced Entities (Hub Nodes) - Table
   - InDegree Distribution - Histogram
   - Low-Referenced Entities Count
   - Average Graph Degree Over Time
   - Max InDegree (Most Referenced)
   - Total Graph Nodes Tracked

5. **Shard Communication**
   - Routing Request Types (Local, Remote, Scatter-Gather)
   - Shard Network Traffic (Bytes Sent/Received)
   - Shard Routing Error Rate

**Vollständige Dokumentation:** `deploy/kubernetes/monitoring/grafana-dashboards/README.md`

### Grafana Dashboard: Graph-Topologie (Alternative Ansicht)

```yaml
# grafana/dashboards/graph_topology.json (vereinfacht)
panels:
  - title: "Top 10 Referenzierte Entities (Hub-Nodes)"
    type: "table"
    targets:
      - expr: |
          topk(10, themisdb_graph_node_incoming_edges)
    columns:
      - URN
      - Type
      - InDegree
      
  - title: "InDegree Verteilung"
    type: "histogram"
    targets:
      - expr: |
          themisdb_graph_indegree_bucket
          
  - title: "Entities mit wenigen Referenzen"
    type: "stat"
    targets:
      - expr: |
          count(themisdb_graph_node_incoming_edges <= 2)
    thresholds:
      - value: 1000
        color: "green"
      - value: 5000
        color: "yellow"
      - value: 10000
        color: "red"
        
  - title: "Avg InDegree über Zeit"
    type: "graph"
    targets:
      - expr: |
          avg(themisdb_graph_node_incoming_edges)
```

### Use Cases für Graph-Topologie Metriken

**1. Wichtige Entities identifizieren (Hub-Nodes)**
- Behörden mit vielen Dokumenten
- Policies die oft referenziert werden
- Zentrale Personen/Organisationen

```sql
-- Query: Finde die 10 wichtigsten Behörden nach Referenzen
SELECT urn, type, indegree 
FROM graph_topology_metrics 
WHERE type = 'authority' 
ORDER BY indegree DESC 
LIMIT 10;
```

**2. Orphaned Documents erkennen (Leaf-Nodes)**
- Dokumente ohne eingehende Referenzen
- Potentiell isolierte oder vergessene Inhalte
- Kandidaten für Archivierung oder Review

```sql
-- Query: Finde Dokumente ohne Referenzen
SELECT urn, type, created_at 
FROM graph_topology_metrics 
WHERE indegree = 0 AND type = 'document'
ORDER BY created_at DESC;
```

**3. Anomalie-Erkennung**
- Plötzlicher Anstieg von Referenzen (virales Dokument)
- Unerwartete Link-Patterns
- Potentielle Daten-Qualitätsprobleme

```python
# Alerting Rule (Prometheus)
- alert: HighInDegreeAnomaly
  expr: |
    (themisdb_graph_node_incoming_edges - 
     themisdb_graph_node_incoming_edges offset 1h) > 100
  for: 5m
  annotations:
    summary: "Entity {{ $labels.urn }} hat ungewöhnlich viele neue Referenzen"
```

### Persistent Storage für Topologie-Metriken

```cpp
/**
 * Persistiere Graph-Topologie Metriken in TimeSeries DB
 */
class TopologyMetricsPersister {
public:
    /**
     * Snapshot Metriken alle N Minuten
     */
    void scheduleSnapshot(std::chrono::minutes interval) {
        while (!should_stop_) {
            auto metrics = topology_metrics_->exportMetrics();
            
            // Speichere in TimeSeries (z.B. InfluxDB)
            influxdb_->write(
                "graph_topology",
                {
                    {"measurement", "node_metrics"},
                    {"time", std::chrono::system_clock::now()},
                    {"fields", {
                        {"total_nodes", metrics["total_nodes"]},
                        {"avg_indegree", metrics["avg_indegree"]},
                        {"max_indegree", metrics["max_indegree"]}
                    }}
                }
            );
            
            // Speichere Top-N Hub-Nodes
            for (const auto& node : metrics["top_referenced"]) {
                influxdb_->write(
                    "graph_topology",
                    {
                        {"measurement", "hub_nodes"},
                        {"tags", {{"urn", node["urn"]}, {"type", node["type"]}}},
                        {"time", std::chrono::system_clock::now()},
                        {"fields", {
                            {"indegree", node["indegree"]},
                            {"outdegree", node["outdegree"]}
                        }}
                    }
                );
            }
            
            std::this_thread::sleep_for(interval);
        }
    }
};
```


### Zusammenfassung: Link-Discovery Strategien

| Ansatz | Verfügbarkeit | Genauigkeit | Aufwand | Use Case |
|--------|--------------|-------------|---------|----------|
| **Client-deklariert** | Sofort | Hoch (100%) | Client | Bekannte Referenzen, Formular-basierte Eingabe |
| **URN-Pattern Scan** | Async | Hoch (90%+) | Niedrig | URNs direkt im Text erwähnt |
| **NLP Entity-Extraktion** | Async | Mittel (70%+) | Hoch | Natürlichsprachliche Dokumente |
| **Metadata-Mapping** | Async | Hoch (100%) | Niedrig | Strukturierte Metadaten vorhanden |

**Empfehlung:**
1. **Start mit Client-deklarierten Links** - Sofort verfügbar, volle Kontrolle
2. **Erweitern mit URN-Pattern Scanning** - Automatische Erkennung expliziter URNs
3. **Optional NLP** - Für unstrukturierte Dokumente mit Behörden-Referenzen
4. **Alle Methoden als niedrig-priorisierte Hintergrundaufgabe** - Keine Blockierung der Ingestion

---

## API-Beispiele für Cross-Shard Queries

### 1. Graph-Suche über alle Shards

```bash
# Graph-Suche: Finde alle Chunks und ihre 2-Hop Nachbarn
POST /api/v1/graph/search
{
  "urn_pattern": "urn:themis:graph:docs:chunks:*",
  "hops": 2,
  "edge_types": ["parent", "next", "prev"],
  "limit": 100
}

# Response:
{
  "hub_shard_id": "hub_001",
  "worker_shards_queried": 3,
  "execution_time_ms": 45,
  "nodes": [
    {"urn": "urn:themis:graph:docs:chunks:abc-123", "shard": "worker_001", ...},
    {"urn": "urn:themis:graph:docs:chunks:def-456", "shard": "worker_002", ...}
  ],
  "edges": [
    {"source": "abc-123", "target": "def-456", "type": "next", "cross_shard": true}
  ]
}
```

### 2. Hybrid-Suche über alle Shards

```bash
# Hybrid: Text + Vector + Graph
POST /api/v1/search/hybrid
{
  "text_query": "machine learning optimization",
  "text_column": "content",
  "vector_query": [0.1, 0.2, ..., 0.768],
  "graph_hops": 1,
  "graph_edge_types": ["parent"],
  "fusion_mode": "rrf",
  "k": 20
}

# Response:
{
  "hub_shard_id": "hub_001",
  "worker_shards_queried": 3,
  "fusion_mode": "rrf",
  "total_candidates": 156,
  "results": [
    {
      "urn": "urn:themis:graph:docs:chunks:xyz-789",
      "score": 0.0892,
      "rank": 0,
      "data": {
        "text_score": 0.85,
        "vector_score": 0.92,
        "graph_score": 0.78,
        "content": "..."
      }
    }
  ]
}
```

---

## Deployment-Beispiel: Hub + 3 Workers

```yaml
# kubernetes/hub-shard-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: themisdb-hub
spec:
  replicas: 2  # HA für Hub
  template:
    spec:
      containers:
      - name: themisdb
        image: themisdb:latest
        env:
        - name: SHARD_ROLE
          value: "hub"
        - name: SHARD_ID
          value: "hub_001"
        - name: ENABLE_URN_CACHE
          value: "true"
        - name: ENABLE_EDGE_INDEX
          value: "true"

---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: themisdb-workers
spec:
  replicas: 3  # 3 Worker-Shards
  template:
    spec:
      containers:
      - name: themisdb
        image: themisdb:latest
        env:
        - name: SHARD_ROLE
          value: "worker"
        - name: HUB_SHARD_ENDPOINT
          value: "themisdb-hub:8080"
```

---

## Monitoring: Cross-Shard Query Metriken

```
# Hub-Shard Metriken
themisdb_hub_graph_searches_total = 1542
themisdb_hub_hybrid_searches_total = 892
themisdb_hub_cross_shard_edges_total = 15623
themisdb_hub_urn_cache_hit_rate = 0.94
themisdb_hub_query_fanout_avg = 2.8  # Durchschnittlich 2.8 Worker pro Query

# Worker-Shard Metriken
themisdb_worker_local_graph_searches_total{shard="worker_001"} = 512
themisdb_worker_local_hybrid_searches_total{shard="worker_001"} = 298
themisdb_worker_cross_shard_edge_reports_total{shard="worker_001"} = 5234
```

---

## Zusammenfassung

### URN-basierte Cross-Shard Suche

✅ **Location Transparency** - Clients kennen Shard-Topologie nicht  
✅ **URN Pattern Matching** - `urn:themis:graph:docs:chunks:*` findet alle relevanten Shards  
✅ **Globale Eindeutigkeit** - URNs sind über alle Shards hinweg eindeutig

### Hub-Shard Knoten

✅ **Query Coordination** - Hub verteilt Queries an relevante Worker  
✅ **Result Aggregation** - Hub merged Ergebnisse von allen Workern  
✅ **Cross-Shard Edge Tracking** - Hub verwaltet Edges zwischen Shards  
✅ **URN Caching** - Hub cached URN → Shard Mappings

### Graph + Hybrid Suche

✅ **Scatter-Gather** - Query wird an alle relevanten Shards verteilt  
✅ **Local Processing** - Jeder Worker führt lokale Graph/Hybrid-Suche aus  
✅ **Global Fusion** - Hub merged und re-rankt Ergebnisse  
✅ **Cross-Shard Traversal** - Hub folgt Edges über Shard-Grenzen hinweg

---

## Production Considerations

### Failure Scenarios & Recovery

#### Hub-Shard Ausfall

**Problem:** Hub-Shard ist Single Point of Failure für Query-Koordination

**Lösung: Hub-Shard High Availability**

```yaml
# Hub-Shard HA-Konfiguration
hub_shard:
  high_availability:
    enabled: true
    mode: "active_passive"  # oder "active_active"
    
    # Primary Hub
    primary:
      shard_id: "hub_001"
      endpoint: "themis-hub-001.cluster.local:8080"
      
    # Standby Hubs (Hot Standby)
    standbys:
      - shard_id: "hub_002"
        endpoint: "themis-hub-002.cluster.local:8080"
        sync_mode: "async"  # Edge-Index Replikation
      - shard_id: "hub_003"
        endpoint: "themis-hub-003.cluster.local:8080"
        sync_mode: "async"
    
    # Failover
    failover:
      detection_timeout_ms: 5000
      health_check_interval_ms: 1000
      automatic_failover: true
      
    # Shared State (etcd)
    shared_state:
      backend: "etcd"
      endpoints: ["etcd-001:2379", "etcd-002:2379", "etcd-003:2379"]
      # Cross-Shard Edge Index in etcd
      sync_edge_index: true
```

**Failover-Ablauf:**

1. Health-Check erkennt Primary-Hub-Ausfall
2. Standby-Hub wird zum Primary promoted
3. URN-Cache wird aus etcd geladen
4. Cross-Shard Edge-Index wird synchronisiert
5. Worker-Shards werden über neuen Primary informiert

**Recovery Time Objective (RTO):** < 30 Sekunden

#### Worker-Shard Ausfall

**Problem:** Worker-Shard mit Daten nicht erreichbar

**Lösung: Replica-Failover + Partial Results**

```cpp
class HubShard {
    /**
     * Worker-Shard Failover bei Scatter-Gather
     */
    nlohmann::json executeWithFailover(
        const std::vector<ShardInfo>& target_shards,
        const std::string& query
    ) {
        std::vector<std::future<ShardResult>> futures;
        
        for (const auto& shard : target_shards) {
            futures.push_back(std::async([this, shard, query]() {
                ShardResult result;
                result.shard_id = shard.shard_id;
                
                try {
                    // 1. Versuche Primary Shard
                    result = executor_->executeQuery(shard, query);
                } catch (const ShardUnavailableException& e) {
                    // 2. Failover zu Replica
                    auto replicas = topology_->getReplicas(shard.shard_id);
                    
                    for (const auto& replica : replicas) {
                        try {
                            result = executor_->executeQuery(replica, query);
                            result.served_by_replica = true;
                            result.replica_shard_id = replica.shard_id;
                            break;  // Success
                        } catch (...) {
                            continue;  // Try next replica
                        }
                    }
                    
                    // 3. Wenn alle Replicas fehlschlagen
                    if (!result.success) {
                        if (config_.enable_partial_results) {
                            // Partial Results: Fortfahren ohne diesen Shard
                            result.success = false;
                            result.partial_failure = true;
                        } else {
                            throw;  // Propagate error
                        }
                    }
                }
                
                return result;
            }));
        }
        
        // Collect und merge mit Partial Results Handling
        return collectAndMergeWithPartialResults(futures);
    }
};
```

#### Netzwerk-Partition (Split-Brain)

**Problem:** Hub kann Worker nicht erreichen, aber Worker sind aktiv

**Lösung: Quorum-basierte Entscheidungen**

```yaml
consistency:
  # Quorum für Cross-Shard Operations
  quorum:
    read_quorum: "majority"   # N/2 + 1 Shards müssen antworten
    write_quorum: "majority"
    
  # Timeout-Konfiguration
  timeouts:
    shard_request_timeout_ms: 5000
    scatter_gather_timeout_ms: 30000
    
  # Split-Brain Prevention
  split_brain:
    enabled: true
    coordination_backend: "etcd"  # Distributed Consensus
    lease_timeout_seconds: 10
```

### Consistency Guarantees

#### Read Consistency Levels

```cpp
enum class ReadConsistency {
    EVENTUAL,        // Schnellste, kann veraltete Daten liefern
    MONOTONIC_READ,  // Read-your-writes innerhalb Session
    STRONG           // Immer aktuellste Daten (langsamer)
};

nlohmann::json HubShard::executeGraphSearch(
    const URN& start_urn,
    uint32_t hops,
    ReadConsistency consistency_level
) {
    switch (consistency_level) {
        case ReadConsistency::EVENTUAL:
            // Lese von beliebigem Worker (Replica OK)
            return executeFastRead(start_urn, hops);
            
        case ReadConsistency::MONOTONIC_READ:
            // Lese von Primary oder aktuellsten Replica
            return executeMonotonicRead(start_urn, hops);
            
        case ReadConsistency::STRONG:
            // Lese nur von Primary, warte auf Sync
            return executeStrongRead(start_urn, hops);
    }
}
```

#### Write-After-Read Consistency

**Problem:** Client schreibt Edge, sofortiger Read findet Edge nicht

**Lösung: Session-basierte Consistency + Version Tracking**

```cpp
struct SessionContext {
    std::string session_id;
    uint64_t last_write_version;  // Höchste Version die Session geschrieben hat
    std::chrono::system_clock::time_point session_start;
};

class ConsistencyManager {
public:
    /**
     * Garantiert: Reads sehen eigene Writes
     */
    nlohmann::json readAfterWrite(
        const SessionContext& session,
        const URN& urn
    ) {
        auto result = hub_shard_->get(urn);
        
        // Prüfe ob Result mindestens so aktuell wie letzter Write
        if (result.contains("version") && 
            result["version"].get<uint64_t>() < session.last_write_version) {
            
            // Warte auf Replikation oder lese von Primary
            return hub_shard_->getFromPrimary(urn);
        }
        
        return result;
    }
};
```

### Security & Access Control

**Referenz:** Die Shard-Authentifizierung ist bereits in `docs/sharding/sharding_strategy.md` dokumentiert.

**Wichtige Aspekte für Cross-Shard Security:**

1. **Mutual TLS zwischen Shards**
   - Hub ↔ Worker: PKI-basierte Zertifikate
   - Worker ↔ Worker: Peer-Authentifizierung

2. **URN-basierte Access Control**
   ```cpp
   // URN enthält Namespace → Access Control Check
   bool canAccess = acl_->checkPermission(
       session.user_id,
       urn.namespace_,
       Permission::READ
   );
   ```

3. **Cross-Shard Query Authorization**
   - Hub prüft Berechtigung BEVOR Scatter-Gather
   - Worker validiert Anfragen vom Hub (mutual auth)

**Konfiguration:**

```yaml
security:
  shard_authentication:
    enabled: true
    mode: "mutual_tls"
    pki:
      ca_cert: "/etc/themisdb/certs/ca.crt"
      hub_cert: "/etc/themisdb/certs/hub.crt"
      hub_key: "/etc/themisdb/certs/hub.key"
      
  urn_access_control:
    enabled: true
    enforce_namespace_acl: true
    
  cross_shard_queries:
    require_authentication: true
    validate_hub_certificate: true
```

### Capacity Planning

#### Wann neue Shards hinzufügen?

**Trigger-Metriken:**

```yaml
capacity_triggers:
  # Storage-basiert
  storage:
    high_watermark_percent: 80
    critical_watermark_percent: 90
    action: "add_shard"
    
  # Request-basiert
  requests:
    requests_per_second_threshold: 10000
    avg_latency_ms_threshold: 100
    action: "add_shard"
    
  # Memory-basiert
  memory:
    urn_cache_eviction_rate_threshold: 0.2
    action: "increase_cache_or_add_shard"
```

**Shard-Sizing-Guidelines:**

| Metrik | Empfohlener Wert | Max Wert |
|--------|------------------|----------|
| Entities pro Shard | 10M - 50M | 100M |
| Storage pro Shard | 100GB - 500GB | 1TB |
| Requests/s pro Shard | 1k - 5k | 10k |
| URN Cache Size | 100k - 1M URNs | 5M |

#### Shard Rebalancing

**Trigger:** Neuer Shard hinzugefügt

**Prozess:**

```cpp
class ShardRebalancer {
public:
    /**
     * Rebalance nach Shard-Hinzufügung
     */
    void rebalanceAfterShardAddition(const std::string& new_shard_id) {
        // 1. Update Consistent Hash Ring
        hash_ring_->addShard(new_shard_id, config_.virtual_nodes);
        
        // 2. Identifiziere zu verschiebende URNs
        auto urns_to_migrate = identifyMigrationCandidates(new_shard_id);
        
        // 3. Starte Migration (Hintergrund)
        auto migration_job = std::make_unique<MigrationJob>(
            urns_to_migrate,
            new_shard_id,
            MigrationMode::GRADUAL  // Nicht alle auf einmal
        );
        
        migration_scheduler_->schedule(migration_job);
        
        // 4. Update Hub-Shard URN-Cache schrittweise
        // Während Migration: Dual-Read (alter + neuer Shard)
        
        // 5. Nach Migration: Bereinige alte Shards
    }
};
```

### Performance Tuning

#### URN Cache Optimierung

**Cache Size Berechnung:**

```python
# Formel für optimale Cache-Größe
def calculate_optimal_cache_size(
    total_urns: int,
    hot_urn_percentage: float = 0.1,  # 10% sind "hot"
    avg_urn_size_bytes: int = 100
) -> int:
    """
    Optimal: Alle "hot" URNs im Cache
    """
    hot_urns = int(total_urns * hot_urn_percentage)
    cache_size_bytes = hot_urns * avg_urn_size_bytes
    
    # Add 20% Overhead
    return int(cache_size_bytes * 1.2)

# Beispiel: 100M URNs, 10% hot
# = 10M hot URNs × 100 bytes × 1.2 = 1.2GB Cache
```

**Cache-Konfiguration:**

```yaml
hub_shard:
  urn_cache:
    max_entries: 10000000  # 10M URNs
    max_memory_mb: 1200
    eviction_policy: "lru"
    ttl_seconds: 3600
    
    # Preload häufige URNs beim Start
    preload:
      enabled: true
      top_n_urns: 1000000  # Top 1M nach Access-Count
```

#### Query Timeout-Tuning

```yaml
timeouts:
  # Basis-Timeouts
  single_shard_query_ms: 1000
  cross_shard_query_ms: 5000
  scatter_gather_query_ms: 30000
  
  # Per-Query-Type Overrides
  graph_search:
    timeout_ms_per_hop: 2000  # 2s pro Hop
    max_total_timeout_ms: 60000
    
  hybrid_search:
    text_search_timeout_ms: 3000
    vector_search_timeout_ms: 5000
    graph_expansion_timeout_ms: 5000
    fusion_timeout_ms: 2000
    max_total_timeout_ms: 60000
```

### Troubleshooting Guide

#### Häufige Probleme

**Problem 1: Hohe Cross-Shard Query Latenz**

**Diagnose:**
```bash
# Check Scatter-Gather Fanout
curl http://hub:9090/metrics | grep themisdb_hub_query_fanout_avg

# Check Worker Latenz
curl http://hub:9090/metrics | grep themisdb_routing_latency_ms
```

**Lösungen:**
1. Reduziere Fanout durch besseres Shard-Targeting
2. Erhöhe Worker-Shard Anzahl (mehr Parallelismus)
3. Optimiere Query (z.B. kleinere Graph-Hops)
4. Enable Query Result Caching

**Problem 2: URN Cache Miss Rate hoch**

**Diagnose:**
```bash
# Cache Hit Rate prüfen
curl http://hub:9090/metrics | grep themisdb_hub_urn_cache_hit_rate
```

**Lösungen:**
1. Erhöhe Cache-Größe (`max_entries`)
2. Erhöhe TTL (`ttl_seconds`)
3. Enable Preloading häufiger URNs
4. Prüfe ob URN-Pattern zu divers (viele verschiedene URNs)

**Problem 3: Cross-Shard Edge Tracking unvollständig**

**Diagnose:**
```bash
# Check Edge-Index Größe
curl http://hub:8080/api/v1/admin/edge-index/stats

# Vergleiche mit erwarteter Anzahl
```

**Lösungen:**
1. Prüfe Link-Discovery-Service Status
2. Prüfe ob Worker Edges korrekt an Hub melden
3. Check Edge-Index Replikation (bei HA)
4. Manuelles Edge-Index Rebuild triggern

**Problem 4: Partial Results nach Worker-Ausfall**

**Diagnose:**
```bash
# Check Failed Shards
curl http://hub:9090/metrics | grep themisdb_routing_errors_total
```

**Lösungen:**
1. Prüfe Worker-Shard Health
2. Aktiviere Replica-Failover
3. Falls akzeptabel: `enable_partial_results: true`
4. Erhöhe Worker-Shard Redundanz (mehr Replicas)

#### Debug-Modus

```yaml
# Für Troubleshooting: Verbose Logging
logging:
  level: "debug"
  
  # Spezifische Module
  modules:
    hub_shard: "trace"
    shard_router: "debug"
    urn_resolver: "debug"
    
  # Request Tracing
  request_tracing:
    enabled: true
    sample_rate: 1.0  # 100% während Debug
    include_query_plans: true
```

**Distributed Tracing:**

```yaml
tracing:
  enabled: true
  backend: "jaeger"
  endpoint: "jaeger-collector:14268"
  
  # Cross-Shard Request Tracing
  trace_cross_shard_requests: true
  trace_scatter_gather: true
  trace_edge_resolution: true
```

---

**Status:** Design dokumentiert, Implementation folgt in Phase 2  
**Dependencies:** Sharding ✅, URN System ✅, Graph API ✅, Hybrid Search ✅

**Weitere Referenzen:**
- Shard Security: `docs/sharding/sharding_strategy.md`
- Shard Migration: `docs/sharding/sharding_implementation.md`
- PKI Setup: `docs/security/security_pki.md`
- Monitoring: `deploy/kubernetes/monitoring/grafana-dashboards/README.md`
