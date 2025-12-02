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
