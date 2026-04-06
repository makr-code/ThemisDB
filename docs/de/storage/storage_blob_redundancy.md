# ThemisDB - Granulare Binär-Blob Redundanz

**Version:** 1.0  
**Stand:** 6. April 2026  
**Status:** Konzept / Design

---

## Executive Summary

Dieses Dokument beschreibt ein **feingranulares Redundanz-System**, das bis auf die Ebene einzelner
RocksDB SST-Files (Binär-Blobs) heruntergeht. Jede Datei kann individuell konfiguriert werden
hinsichtlich Performance vs. Redundanz Trade-offs.

### Kernidee

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    ThemisDB Storage Hierarchy                            │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  Collection Level    →  Default-Redundanz für alle Dokumente            │
│       ↓                                                                  │
│  Document Level      →  Override pro Dokument (z.B. VIP-Kunden)         │
│       ↓                                                                  │
│  Field Level         →  Spezielle Felder (z.B. verschlüsselte Daten)    │
│       ↓                                                                  │
│  Binary Blob Level   →  RocksDB SST-Files, WAL-Segmente, Indexes        │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## Architektur

### Storage Layer Abstraction

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         Application Layer                                │
│   (Collections, Documents, Queries)                                      │
├─────────────────────────────────────────────────────────────────────────┤
│                         Storage Abstraction Layer                        │
│   StoragePolicy → RedundancyManager → BlobTracker                       │
├─────────────────────────────────────────────────────────────────────────┤
│                         Blob Storage Layer                               │
│   ┌──────────────┐ ┌──────────────┐ ┌──────────────┐                    │
│   │ RocksDB SST  │ │   WAL Logs   │ │   Indexes    │                    │
│   │    Files     │ │   Segments   │ │   (Vector,   │                    │
│   │              │ │              │ │    Graph)    │                    │
│   └──────────────┘ └──────────────┘ └──────────────┘                    │
├─────────────────────────────────────────────────────────────────────────┤
│                         Physical Storage Layer                           │
│   ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐                   │
│   │ Local SSD│ │ Local HDD│ │ Network  │ │  Object  │                   │
│   │          │ │          │ │ Storage  │ │  Storage │                   │
│   │ (Hot)    │ │ (Warm)   │ │ (Replicas│ │  (Cold)  │                   │
│   └──────────┘ └──────────┘ └──────────┘ └──────────┘                   │
└─────────────────────────────────────────────────────────────────────────┘
```

### Blob Types

| Blob Type | Beschreibung | Typische Größe | Default Redundanz |
|-----------|--------------|----------------|-------------------|
| `SST_L0` | RocksDB Level 0 (memtable flush) | 64MB | MIRROR(2) |
| `SST_L1` | RocksDB Level 1 | 256MB | MIRROR(2) |
| `SST_L2+` | RocksDB Level 2+ (cold data) | 256MB | PARITY(4+2) |
| `WAL` | Write-Ahead Log Segment | 128MB | MIRROR(3) |
| `MANIFEST` | RocksDB Manifest | <1MB | MIRROR(3) + GEO |
| `INDEX_VECTOR` | HNSW Vector Index | Variable | MIRROR(2) |
| `INDEX_GRAPH` | Graph Adjacency Index | Variable | MIRROR(2) |
| `INDEX_FTS` | Full-Text Search Index | Variable | PARITY(4+2) |
| `BLOB_LARGE` | Large Binary Objects (>1MB) | >1MB | PARITY(6+2) |
| `METADATA` | System Metadata | <1MB | MIRROR(3) + GEO |

---

## YAML Konfiguration

### Globale Konfiguration

```yaml
# config/storage_redundancy.yaml

# Global defaults
global:
  default_redundancy_mode: MIRROR
  default_replication_factor: 2
  enable_blob_tracking: true
  blob_metadata_store: etcd  # etcd, consul, embedded

# Storage tiers definition
storage_tiers:
  hot:
    type: LOCAL_SSD
    path: /data/themis/hot
    max_capacity_gb: 500
    redundancy_mode: MIRROR
    replication_factor: 2
    
  warm:
    type: LOCAL_HDD
    path: /data/themis/warm
    max_capacity_gb: 2000
    redundancy_mode: PARITY
    erasure_coding:
      data_shards: 4
      parity_shards: 2
      
  cold:
    type: OBJECT_STORAGE
    endpoint: s3://themis-cold-storage
    redundancy_mode: PARITY
    erasure_coding:
      data_shards: 10
      parity_shards: 4
    compression: ZSTD

# Blob type specific configuration
blob_types:
  # RocksDB SST Files
  SST_L0:
    description: "Level 0 SST files (fresh data)"
    tier: hot
    redundancy_mode: MIRROR
    replication_factor: 2
    sync_write: true
    priority: CRITICAL
    
  SST_L1:
    description: "Level 1 SST files"
    tier: hot
    redundancy_mode: MIRROR
    replication_factor: 2
    sync_write: false
    priority: HIGH
    
  SST_L2_PLUS:
    description: "Level 2+ SST files (compacted)"
    tier: warm
    redundancy_mode: PARITY
    erasure_coding:
      data_shards: 4
      parity_shards: 2
    priority: NORMAL
    auto_tier_down: true
    tier_down_after_days: 30
    
  # Write-Ahead Logs
  WAL:
    description: "Write-Ahead Log segments"
    tier: hot
    redundancy_mode: MIRROR
    replication_factor: 3
    sync_write: true
    priority: CRITICAL
    retention_hours: 24
    geo_replicate: true
    
  MANIFEST:
    description: "RocksDB Manifest files"
    tier: hot
    redundancy_mode: MIRROR
    replication_factor: 3
    sync_write: true
    priority: CRITICAL
    geo_replicate: true
    backup_on_change: true
    
  # Indexes
  INDEX_VECTOR:
    description: "HNSW Vector indexes"
    tier: hot
    redundancy_mode: MIRROR
    replication_factor: 2
    priority: HIGH
    rebuild_on_loss: true
    
  INDEX_GRAPH:
    description: "Graph adjacency indexes"
    tier: hot
    redundancy_mode: MIRROR
    replication_factor: 2
    priority: HIGH
    rebuild_on_loss: true
    
  INDEX_FTS:
    description: "Full-text search indexes"
    tier: warm
    redundancy_mode: PARITY
    erasure_coding:
      data_shards: 4
      parity_shards: 2
    priority: NORMAL
    rebuild_on_loss: true
    
  # Large Binary Objects
  BLOB_LARGE:
    description: "Large binary objects (>1MB)"
    tier: warm
    redundancy_mode: PARITY
    erasure_coding:
      data_shards: 6
      parity_shards: 2
    priority: NORMAL
    stripe_size_kb: 1024
    auto_tier_down: true
    tier_down_after_days: 7
    
  # System Metadata
  METADATA:
    description: "System metadata and configuration"
    tier: hot
    redundancy_mode: MIRROR
    replication_factor: 3
    sync_write: true
    priority: CRITICAL
    geo_replicate: true
    backup_on_change: true
    version_history: 100  # Keep 100 versions
```

### Collection-spezifische Overrides

```yaml
# config/collections/users.yaml

collection: users
description: "User accounts - critical data"

# Override global defaults for this collection
redundancy:
  mode: MIRROR
  replication_factor: 3
  write_concern: ALL
  read_preference: PRIMARY

# Per-field redundancy (for sensitive fields)
field_overrides:
  password_hash:
    redundancy_mode: MIRROR
    replication_factor: 3
    encryption: AES_256_GCM
    geo_replicate: true
    
  email:
    redundancy_mode: MIRROR
    replication_factor: 3
    
  profile_picture:
    # Large blob, use erasure coding
    redundancy_mode: PARITY
    erasure_coding:
      data_shards: 4
      parity_shards: 2
    tier: warm
    
  session_data:
    # Ephemeral, less redundancy needed
    redundancy_mode: MIRROR
    replication_factor: 1
    tier: hot
    ttl_hours: 24

# Blob-level overrides for this collection's storage
blob_overrides:
  SST_L0:
    replication_factor: 3  # More redundancy for user data
    
  WAL:
    replication_factor: 3
    geo_replicate: true
```

### Document-Level Redundanz

```yaml
# Example: Per-document redundancy via metadata

# In application code:
# db.users.insert({
#   _id: "user_123",
#   name: "VIP Customer",
#   _redundancy: {
#     mode: "MIRROR",
#     replication_factor: 5,
#     geo_replicate: true,
#     priority: "CRITICAL"
#   }
# })

# Or via API:
# POST /api/v1/collections/users/documents
# X-ThemisDB-Redundancy: {"mode": "MIRROR", "replication_factor": 5}
```

---

## Blob Tracking System

### Blob Metadata Schema

```yaml
# Blob metadata stored in etcd/internal store

blob:
  id: "blob_a1b2c3d4e5f6"
  type: SST_L2_PLUS
  collection: users
  
  # Physical location
  locations:
    - shard: shard_001
      path: /data/themis/warm/000123.sst
      tier: warm
      checksum: sha256:abc123...
      size_bytes: 268435456
      created_at: "2025-12-02T10:30:00Z"
      
    - shard: shard_002
      path: /data/themis/warm/000123.sst
      tier: warm
      checksum: sha256:abc123...
      size_bytes: 268435456
      created_at: "2025-12-02T10:30:01Z"
      is_parity: false
      chunk_index: 0
      
    - shard: shard_003
      path: /data/themis/warm/000123_p1.sst
      tier: warm
      checksum: sha256:def456...
      size_bytes: 268435456
      created_at: "2025-12-02T10:30:01Z"
      is_parity: true
      chunk_index: 4
      
  # Redundancy configuration
  redundancy:
    mode: PARITY
    data_shards: 4
    parity_shards: 2
    current_copies: 6
    required_copies: 6
    healthy: true
    
  # Lifecycle
  lifecycle:
    created_at: "2025-12-02T10:30:00Z"
    last_accessed: "2025-12-02T15:45:00Z"
    last_verified: "2025-12-02T14:00:00Z"
    tier_history:
      - tier: hot
        from: "2025-12-02T10:30:00Z"
        to: "2025-12-02T12:00:00Z"
      - tier: warm
        from: "2025-12-02T12:00:00Z"
        to: null
    scheduled_tier_down: "2026-01-02T00:00:00Z"
    
  # Statistics
  stats:
    read_count: 1523
    write_count: 1
    last_compaction: "2025-12-02T11:00:00Z"
    compression_ratio: 0.45
```

---

## Implementation Details

### BlobRedundancyManager Interface

```cpp
// include/storage/blob_redundancy_manager.h

namespace themisdb::storage {

/**
 * Blob Type Classification
 */
enum class BlobType {
    SST_L0,         // RocksDB Level 0
    SST_L1,         // RocksDB Level 1
    SST_L2_PLUS,    // RocksDB Level 2+
    WAL,            // Write-Ahead Log
    MANIFEST,       // RocksDB Manifest
    INDEX_VECTOR,   // Vector Index (HNSW)
    INDEX_GRAPH,    // Graph Index
    INDEX_FTS,      // Full-Text Search Index
    BLOB_LARGE,     // Large Binary Object
    METADATA,       // System Metadata
    CUSTOM          // Application-defined
};

/**
 * Storage Tier
 */
enum class StorageTier {
    HOT,            // Local SSD, fastest
    WARM,           // Local HDD or network SSD
    COLD,           // Object storage (S3, etc.)
    ARCHIVE         // Glacier-like, very slow retrieval
};

/**
 * Blob Priority
 */
enum class BlobPriority {
    CRITICAL,       // Must never lose, sync replication
    HIGH,           // Important, async replication OK
    NORMAL,         // Standard redundancy
    LOW,            // Can be regenerated
    EPHEMERAL       // No redundancy needed
};

/**
 * Blob Redundancy Configuration
 */
struct BlobRedundancyConfig {
    RedundancyMode mode = RedundancyMode::MIRROR;
    uint32_t replication_factor = 2;
    
    // Erasure coding (if mode == PARITY)
    uint32_t data_shards = 4;
    uint32_t parity_shards = 2;
    
    // Storage
    StorageTier tier = StorageTier::HOT;
    bool sync_write = false;
    bool geo_replicate = false;
    
    // Priority
    BlobPriority priority = BlobPriority::NORMAL;
    
    // Lifecycle
    bool auto_tier_down = false;
    uint32_t tier_down_after_days = 30;
    uint32_t retention_days = 0;  // 0 = forever
    
    // Recovery
    bool rebuild_on_loss = false;  // For indexes
    bool backup_on_change = false;
    uint32_t version_history = 0;
};

/**
 * Blob Location Information
 */
struct BlobLocation {
    std::string shard_id;
    std::string path;
    StorageTier tier;
    std::string checksum;
    uint64_t size_bytes;
    std::chrono::system_clock::time_point created_at;
    bool is_parity = false;
    uint32_t chunk_index = 0;
    bool is_healthy = true;
};

/**
 * Blob Metadata
 */
struct BlobMetadata {
    std::string blob_id;
    BlobType type;
    std::string collection;
    std::vector<BlobLocation> locations;
    BlobRedundancyConfig config;
    
    // Health
    bool isHealthy() const;
    uint32_t healthyLocationCount() const;
    uint32_t requiredLocationCount() const;
    
    // Serialization
    std::string toJson() const;
    static std::optional<BlobMetadata> fromJson(const std::string& json);
};

/**
 * Blob Redundancy Manager
 * 
 * Manages redundancy at the binary blob level, integrating with
 * RocksDB's storage layer.
 */
class BlobRedundancyManager {
public:
    /**
     * Configuration loaded from YAML
     */
    struct Config {
        std::string config_path;
        std::string metadata_store;  // etcd endpoint
        bool enable_tracking = true;
        
        // Default configs per blob type
        std::map<BlobType, BlobRedundancyConfig> blob_type_configs;
        
        // Storage tier definitions
        std::map<StorageTier, TierConfig> tier_configs;
        
        // Collection overrides
        std::map<std::string, CollectionConfig> collection_configs;
    };
    
    explicit BlobRedundancyManager(const Config& config);
    ~BlobRedundancyManager();
    
    // Lifecycle
    bool start();
    void stop();
    
    // Blob Registration
    // Called when a new blob is created (e.g., SST file flushed)
    std::string registerBlob(
        BlobType type,
        const std::string& collection,
        const std::string& local_path,
        uint64_t size_bytes
    );
    
    // Called when blob is deleted locally
    void unregisterBlob(const std::string& blob_id);
    
    // Redundancy Operations
    // Ensure blob meets redundancy requirements
    bool ensureRedundancy(const std::string& blob_id);
    
    // Check blob health
    BlobMetadata getBlobMetadata(const std::string& blob_id);
    
    // Repair degraded blob
    bool repairBlob(const std::string& blob_id);
    
    // Configuration
    BlobRedundancyConfig getConfigForBlob(
        BlobType type,
        const std::string& collection
    );
    
    void setCollectionConfig(
        const std::string& collection,
        const BlobRedundancyConfig& config
    );
    
    // Tier Management
    bool tierDown(const std::string& blob_id, StorageTier target_tier);
    bool tierUp(const std::string& blob_id, StorageTier target_tier);
    
    // Bulk Operations
    std::vector<std::string> getDegradedBlobs();
    std::vector<std::string> getBlobsForTierDown();
    void runMaintenanceCycle();
    
    // Statistics
    struct Stats {
        uint64_t total_blobs;
        uint64_t healthy_blobs;
        uint64_t degraded_blobs;
        uint64_t total_bytes;
        uint64_t redundant_bytes;
        std::map<BlobType, uint64_t> blobs_by_type;
        std::map<StorageTier, uint64_t> blobs_by_tier;
        uint64_t repair_operations;
        uint64_t tier_transitions;
    };
    
    Stats getStats() const;
    
    // Prometheus Metrics
    std::string exportPrometheusMetrics() const;
    
private:
    Config config_;
    // ... implementation details
};

/**
 * RocksDB Integration
 * 
 * Custom EventListener for RocksDB to track blob lifecycle
 */
class RocksDBBlobListener : public rocksdb::EventListener {
public:
    explicit RocksDBBlobListener(BlobRedundancyManager& manager);
    
    // Called when memtable is flushed to SST
    void OnFlushCompleted(
        rocksdb::DB* db,
        const rocksdb::FlushJobInfo& info
    ) override;
    
    // Called when compaction creates new SST files
    void OnCompactionCompleted(
        rocksdb::DB* db,
        const rocksdb::CompactionJobInfo& info
    ) override;
    
    // Called when SST file is deleted
    void OnTableFileDeleted(
        const rocksdb::TableFileDeletionInfo& info
    ) override;
    
private:
    BlobRedundancyManager& manager_;
    
    BlobType determineBlobType(int level);
};

} // namespace themisdb::storage
```

---

## Use Cases

### Use Case 1: Financial Transactions

```yaml
collection: transactions
description: "Financial transactions - highest redundancy"

redundancy:
  mode: MIRROR
  replication_factor: 5
  write_concern: ALL
  geo_replicate: true

blob_overrides:
  WAL:
    replication_factor: 5
    sync_write: true
    geo_replicate: true
    retention_hours: 8760  # 1 year
    
  SST_L0:
    replication_factor: 5
    sync_write: true
    
  MANIFEST:
    replication_factor: 5
    backup_on_change: true
    version_history: 1000
```

### Use Case 2: Media Assets

```yaml
collection: media_assets
description: "Large media files - storage efficient"

redundancy:
  mode: PARITY
  erasure_coding:
    data_shards: 10
    parity_shards: 4

blob_overrides:
  BLOB_LARGE:
    tier: cold
    erasure_coding:
      data_shards: 10
      parity_shards: 4
    stripe_size_kb: 4096
    auto_tier_down: true
    tier_down_after_days: 1
```

### Use Case 3: Session Data (Ephemeral)

```yaml
collection: sessions
description: "User sessions - regeneratable"

redundancy:
  mode: MIRROR
  replication_factor: 1  # Minimal redundancy

blob_overrides:
  SST_L0:
    replication_factor: 1
    priority: EPHEMERAL
    
  WAL:
    replication_factor: 1
    retention_hours: 1
    
field_overrides:
  session_token:
    redundancy_mode: NONE  # In-memory only
```

---

## Prometheus Metriken

```prometheus
# Blob-Level Metriken
themisdb_blob_total{type="SST_L0", collection="users"} 150
themisdb_blob_total{type="SST_L2_PLUS", collection="users"} 45
themisdb_blob_bytes_total{type="SST_L0", tier="hot"} 9663676416
themisdb_blob_bytes_total{type="SST_L2_PLUS", tier="warm"} 12079595520

# Health Metriken
themisdb_blob_healthy{type="SST_L0"} 150
themisdb_blob_degraded{type="SST_L0"} 0
themisdb_blob_repair_operations_total 15

# Tier Metriken
themisdb_blob_tier_transitions_total{from="hot", to="warm"} 500
themisdb_blob_tier_bytes{tier="hot"} 536870912000
themisdb_blob_tier_bytes{tier="warm"} 2147483648000
themisdb_blob_tier_bytes{tier="cold"} 10995116277760

# Redundancy Metriken
themisdb_blob_redundancy_factor{collection="transactions"} 5
themisdb_blob_redundancy_factor{collection="sessions"} 1
themisdb_blob_parity_chunks{collection="media_assets"} 4000

# Latency Metriken
themisdb_blob_replication_latency_seconds_bucket{le="0.01"} 9000
themisdb_blob_replication_latency_seconds_bucket{le="0.1"} 9500
themisdb_blob_tier_down_duration_seconds_bucket{le="10"} 450
```

---

## API Endpoints

```yaml
# REST API for Blob Management

# Get blob metadata
GET /api/v1/admin/blobs/{blob_id}

# List blobs for collection
GET /api/v1/admin/collections/{collection}/blobs
  ?type=SST_L0
  &tier=hot
  &health=degraded
  &limit=100

# Update blob redundancy
PATCH /api/v1/admin/blobs/{blob_id}/redundancy
  {
    "mode": "MIRROR",
    "replication_factor": 3
  }

# Force tier transition
POST /api/v1/admin/blobs/{blob_id}/tier
  {
    "target_tier": "warm"
  }

# Repair degraded blob
POST /api/v1/admin/blobs/{blob_id}/repair

# Get blob statistics
GET /api/v1/admin/blobs/stats
  ?collection=users
  &type=SST_L0

# Reload configuration
POST /api/v1/admin/config/reload
```

---

## Migration Strategy

### Von bestehenden Blobs

1. **Discovery Phase:** Scan aller existierenden Blobs
2. **Classification:** Blob-Type Erkennung basierend auf Pfad/Metadaten
3. **Tracking Activation:** Metadaten in etcd registrieren
4. **Gradual Enforcement:** Redundanz schrittweise anwenden

### Konfiguration Hot-Reload

```yaml
# Änderungen an storage_redundancy.yaml werden automatisch erkannt
# und ohne Neustart angewendet

hot_reload:
  enabled: true
  check_interval_seconds: 30
  apply_to_new_blobs_only: false  # true = bestehende Blobs behalten Config
```

---

## Zusammenfassung

Diese Architektur ermöglicht:

1. **Granulare Kontrolle:** Redundanz auf Blob-Ebene (RocksDB SST-Files)
2. **YAML-Konfiguration:** Einfache, deklarative Konfiguration
3. **Hierarchische Overrides:** Global → Collection → Document → Field → Blob
4. **Tiered Storage:** Automatische Tier-Transitions basierend auf Zugriffsmuster
5. **RocksDB-Integration:** Nahtlose Integration via EventListener
6. **Observability:** Prometheus Metriken für alle Blob-Operationen
