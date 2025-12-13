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

**Status:** Design dokumentiert, Implementation folgt in Phase 2  
**Dependencies:** Sharding ✅, URN System ✅, Graph API ✅, Hybrid Search ✅
