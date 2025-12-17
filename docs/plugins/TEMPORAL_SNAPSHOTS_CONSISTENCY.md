# Temporal Snapshots und Konsistenz bei RPC Transfers

**Version:** 1.0.0  
**Release:** v1.3.0  
**Datum:** 17. Dezember 2025  
**Kategorie:** RPC, Snapshots, Consistency, MVCC

---

## Executive Summary

Bei allen RPC-basierten Datentransfers (RocksDB Snapshots, LoRA Adapters, Blob-Transfers) ist es kritisch zu verstehen, dass:

1. **Transfers sind temporale Snapshots** - Der Zustand zum Zeitpunkt t₀
2. **Source kann sich ändern** - Während des Transfers können am Ausgangsort Änderungen erfolgen
3. **Konsistenz-Garantien** - Welche ACID-Eigenschaften werden gewährleistet?
4. **Versionierung ist erforderlich** - Um Snapshots eindeutig zu identifizieren

**Kernproblem:** Ein Transfer von Shard A → Shard B, der 5 Minuten dauert, basiert auf einem Snapshot zum Zeitpunkt t₀. Nach 2 Minuten kann auf Shard A bereits eine neue Version existieren!

---

## 1. Problem-Analyse: Concurrent Modifications

### 1.1 Szenario: LoRA Adapter Update während Transfer

```
Timeline:

t₀ = 0 min:  Shard A hat LoRA v1.5 (5 GB)
             Transfer zu Shard B beginnt
             Snapshot-ID: snap_001, Timestamp: 1234567890

t₁ = 2 min:  Transfer 40% complete (2 GB übertragen)
             ⚠️ User updated LoRA zu v1.6 auf Shard A!
             Neue Version erstellt, aber Transfer läuft weiter

t₂ = 5 min:  Transfer 100% complete
             Shard B hat jetzt: LoRA v1.5 (Snapshot von t₀)
             Shard A hat jetzt: LoRA v1.6 (aktuell)

Problem: Shard B hat "veraltete" Version!
```

### 1.2 Szenario: RocksDB Snapshot während Writes

```
Timeline:

t₀ = 10:00:00  Snapshot-Request für Token-Range [0, 1000]
               Snapshot-Timestamp: 10:00:00.000

t₁ = 10:00:05  Snapshot-Erstellung gestartet (RocksDB Checkpoint)
               ⚠️ Neue Writes kommen rein für Range [500, 600]

t₂ = 10:00:10  Checkpoint abgeschlossen
               Transfer beginnt

t₃ = 10:05:00  Transfer abgeschlossen
               ⚠️ In den letzten 5 Minuten: 1000 neue Writes auf Source!

Frage: Welchen Zustand hat Target?
Antwort: Zustand von t₁ (10:00:05), NICHT t₃!
```

---

## 2. Konsistenz-Level

### 2.1 Snapshot Isolation Levels

```protobuf
enum SnapshotIsolation {
    SNAPSHOT_NONE = 0;           // No isolation (read uncommitted)
    SNAPSHOT_CONSISTENT = 1;     // Read Committed (default)
    SNAPSHOT_SERIALIZABLE = 2;   // Serializable Snapshot Isolation
    SNAPSHOT_MVCC = 3;           // Multi-Version Concurrency Control
}
```

### 2.2 Level-Definitionen

**SNAPSHOT_NONE (Read Uncommitted)**
```
Garantien:
├─ KEINE Konsistenz-Garantie
├─ Kann Dirty Reads enthalten
├─ Kann Phantoms enthalten
└─ Schnellst, aber unsicher

Use Case:
└─ NICHT empfohlen für Production
```

**SNAPSHOT_CONSISTENT (Read Committed) - Default**
```
Garantien:
├─ Liest nur committed Daten zum Zeitpunkt t₀
├─ Snapshot ist intern konsistent
├─ KEINE Dirty Reads
└─ Kann Phantoms haben (neue Daten nach Snapshot-Erstellung)

Implementierung:
├─ RocksDB: Snapshot API
├─ Timestamp: snapshot_timestamp_ns
└─ Sequence: snapshot_sequence

Use Case:
├─ Standard für die meisten Transfers
└─ Balance zwischen Konsistenz und Performance
```

**SNAPSHOT_SERIALIZABLE (Serializable Snapshot Isolation - SSI)**
```
Garantien:
├─ Serializable Execution
├─ Snapshot + Write Conflict Detection
├─ Verhindert Write Skew
└─ Höchste Konsistenz

Implementierung:
├─ Transaction Tracking
├─ Conflict Detection
└─ Abort bei Konflikten

Use Case:
├─ Kritische Business-Logik
├─ Financial Transactions
└─ Wenn absolute Konsistenz erforderlich
```

**SNAPSHOT_MVCC (Multi-Version Concurrency Control)**
```
Garantien:
├─ Mehrere Versionen parallel
├─ Readers blockieren Writers nicht
├─ Writers blockieren Readers nicht
└─ Optimistic Concurrency Control

Implementierung:
├─ Version Chains
├─ Timestamp-based Ordering
└─ Garbage Collection

Use Case:
├─ High-Concurrency Systeme
├─ LoRA/Model Versioning
└─ Read-Heavy Workloads
```

---

## 3. Temporal Snapshot Metadata

### 3.1 Snapshot-Identifikation

```cpp
struct SnapshotMetadata {
    // Unique identifier
    std::string snapshot_id;             // UUID (e.g., "snap_a1b2c3d4")
    
    // Temporal information
    uint64_t snapshot_timestamp_ns;      // Nanoseconds since epoch
    uint64_t snapshot_sequence;          // Monotonically increasing
    
    // Versioning
    std::string base_version;            // For incremental: base version
    std::string base_snapshot_id;        // For incremental: base snapshot
    
    // Consistency
    SnapshotIsolation isolation_level;   // Consistency guarantee
    bool is_immutable;                   // True if source frozen during transfer
    
    // Metadata
    std::map<std::string, std::string> tags;  // Custom tags
};
```

### 3.2 Snapshot-Lifecycle

```
┌────────────────────────────────────────────────────────────────┐
│                  Snapshot Lifecycle                             │
└────────────────────────────────────────────────────────────────┘

1. CREATE
   ├─ Generate snapshot_id (UUID)
   ├─ Record snapshot_timestamp_ns (current time)
   ├─ Increment snapshot_sequence
   ├─ Create RocksDB Checkpoint (immutable)
   └─ Store metadata in catalog

2. TRANSFER
   ├─ Read from snapshot (NOT live data)
   ├─ Chunk & Compress
   ├─ Stream via gRPC
   └─ Verify integrity

3. VERIFY
   ├─ Check snapshot_id matches
   ├─ Verify timestamp is in expected range
   ├─ Validate checksum
   └─ Confirm sequence number

4. APPLY
   ├─ Target receives snapshot
   ├─ Records snapshot_id + timestamp
   ├─ Marks as "derived from snapshot_id"
   └─ Can now serve reads

5. CLEANUP (Optional)
   ├─ Delete old snapshots (retention policy)
   ├─ Garbage collect unreferenced chunks
   └─ Update catalog

State Transitions:
CREATING → READY → TRANSFERRING → TRANSFERRED → APPLIED → EXPIRED
```

---

## 4. Handling Concurrent Modifications

### 4.1 Strategy 1: Freeze Source (is_immutable = true)

**Ansatz:** Source wird während Transfer eingefroren

```cpp
// Pseudo-code
class ImmutableTransfer {
public:
    bool transfer() {
        // 1. Lock source (read-only)
        source_->setReadOnly(true);
        
        // 2. Create snapshot
        auto snapshot = source_->createSnapshot();
        snapshot.is_immutable = true;
        
        // 3. Transfer
        bool success = transferSnapshot(snapshot);
        
        // 4. Unlock source
        source_->setReadOnly(false);
        
        return success;
    }
};
```

**Vorteile:**
- ✅ Einfachste Konsistenz-Garantie
- ✅ Keine Concurrent Modification Issues

**Nachteile:**
- ❌ Source ist während Transfer nicht beschreibbar
- ❌ Blocking - nicht geeignet für lange Transfers
- ❌ High Availability beeinträchtigt

**Use Cases:**
- Kleine Transfers (<1 min)
- Maintenance Windows
- Initial Data Loads

### 4.2 Strategy 2: MVCC mit Versionsketten (Empfohlen)

**Ansatz:** Mehrere Versionen parallel halten

```cpp
// Multi-Version Storage
class MVCCBlobStore {
public:
    struct BlobVersion {
        std::string version_id;          // "v1.5", "v1.6", etc.
        uint64_t created_timestamp_ns;
        uint64_t sequence;
        std::vector<uint8_t> data;
        bool is_active;                  // Current version
    };
    
    // Store multiple versions
    std::map<std::string, std::vector<BlobVersion>> version_chains_;
    
    // Get specific version (snapshot)
    BlobVersion getVersion(const std::string& blob_id, uint64_t snapshot_timestamp) {
        auto& chain = version_chains_[blob_id];
        
        // Find version valid at snapshot_timestamp
        for (auto& version : chain) {
            if (version.created_timestamp_ns <= snapshot_timestamp) {
                return version;
            }
        }
        
        throw VersionNotFoundException();
    }
    
    // Add new version (doesn't affect old snapshots)
    void addVersion(const std::string& blob_id, const BlobVersion& new_version) {
        version_chains_[blob_id].push_back(new_version);
        
        // Mark as active
        for (auto& v : version_chains_[blob_id]) {
            v.is_active = false;
        }
        version_chains_[blob_id].back().is_active = true;
    }
    
    // Garbage collect old versions
    void gcOldVersions(uint64_t retention_ns) {
        uint64_t cutoff = currentTimestamp() - retention_ns;
        
        for (auto& [blob_id, chain] : version_chains_) {
            // Keep active + versions within retention
            auto it = std::remove_if(chain.begin(), chain.end(),
                [cutoff](const BlobVersion& v) {
                    return !v.is_active && v.created_timestamp_ns < cutoff;
                });
            chain.erase(it, chain.end());
        }
    }
};
```

**Vorteile:**
- ✅ Source bleibt schreibbar während Transfer
- ✅ Snapshot-Konsistenz garantiert
- ✅ Hohe Verfügbarkeit
- ✅ Readers blockieren Writers nicht

**Nachteile:**
- ❌ Mehr Speicher (multiple Versionen)
- ❌ Komplexere Implementation
- ❌ Garbage Collection nötig

**Use Cases:**
- Production-Systeme
- Lange Transfers (>5 min)
- LoRA/Model Versioning

### 4.3 Strategy 3: Snapshot + WAL Replay

**Ansatz:** Snapshot + nachträgliches Replay von Änderungen

```
┌────────────────────────────────────────────────────────────────┐
│          Snapshot + WAL Replay Strategy                         │
└────────────────────────────────────────────────────────────────┘

Source Shard                                      Target Shard
┌──────────────────┐                             ┌──────────────────┐
│ t₀: Create       │                             │                  │
│ Snapshot         │                             │                  │
│ (LSN = 1000)     │                             │                  │
│        ↓         │                             │                  │
│ t₁-t₅: Transfer │────── Snapshot Data ────────>│ t₁-t₅: Receive   │
│ Snapshot         │                             │ & Apply          │
│                  │                             │                  │
│ Meanwhile:       │                             │ Snapshot applied │
│ t₁: Write A      │                             │ (LSN = 1000)     │
│     (LSN 1001)   │                             │        ↓         │
│ t₂: Write B      │                             │                  │
│     (LSN 1002)   │                             │                  │
│ t₃: Write C      │                             │                  │
│     (LSN 1003)   │                             │                  │
│        ↓         │                             │                  │
│ t₆: Transfer     │─── WAL [1001-1003] ────────>│ t₆: Apply WAL    │
│ WAL Delta        │                             │ Replay Writes    │
│                  │                             │ A, B, C          │
│                  │                             │                  │
│ Current State:   │                             │ Final State:     │
│ Snapshot +       │                             │ Snapshot +       │
│ LSN 1003         │                             │ LSN 1003         │
└──────────────────┘                             └──────────────────┘

Result: Target is now fully consistent with Source!
```

**Implementierung:**
```cpp
class SnapshotWithWALReplay {
public:
    bool transferWithWAL() {
        // 1. Create snapshot and record LSN
        auto snapshot = createSnapshot();
        uint64_t snapshot_lsn = getCurrentLSN();
        
        // 2. Transfer snapshot
        transferSnapshot(snapshot);
        
        // 3. Get WAL entries since snapshot
        auto wal_entries = getWALSince(snapshot_lsn);
        
        // 4. Transfer and replay WAL
        transferAndReplayWAL(wal_entries);
        
        return true;
    }
};
```

**Vorteile:**
- ✅ Target wird eventually consistent
- ✅ Keine Writes verloren
- ✅ Source bleibt verfügbar

**Nachteile:**
- ❌ Komplexer (Snapshot + WAL)
- ❌ Zweistufiger Transfer
- ❌ WAL kann groß werden bei langen Transfers

**Use Cases:**
- Kritische Daten (Financial)
- Wo eventual consistency nicht ausreicht
- Disaster Recovery

---

## 5. Versionierung & Tracking

### 5.1 Version Catalog

```cpp
// Version Catalog for Blob/Snapshot Tracking
class VersionCatalog {
public:
    struct VersionEntry {
        std::string version_id;           // "v1.5.2"
        std::string snapshot_id;          // "snap_abc123"
        uint64_t timestamp_ns;            // Creation time
        uint64_t sequence;                // Monotonic counter
        std::string base_version;         // Parent version (for diffs)
        std::string checksum_sha256;      // Content hash
        uint64_t size_bytes;              // Total size
        SnapshotIsolation isolation;      // Consistency level
        bool is_current;                  // Active version
        std::map<std::string, std::string> metadata;
    };
    
    // Add new version
    void registerVersion(const VersionEntry& entry) {
        versions_[entry.version_id] = entry;
        
        // Update current pointer
        if (entry.is_current) {
            current_version_ = entry.version_id;
        }
    }
    
    // Get version at specific timestamp
    std::optional<VersionEntry> getVersionAtTime(uint64_t timestamp_ns) {
        for (const auto& [vid, entry] : versions_) {
            if (entry.timestamp_ns <= timestamp_ns) {
                // Find most recent version before timestamp
                // (implementation omitted for brevity)
            }
        }
        return std::nullopt;
    }
    
    // List all versions
    std::vector<VersionEntry> listVersions() {
        std::vector<VersionEntry> result;
        for (const auto& [vid, entry] : versions_) {
            result.push_back(entry);
        }
        
        // Sort by sequence
        std::sort(result.begin(), result.end(),
            [](const auto& a, const auto& b) {
                return a.sequence < b.sequence;
            });
        
        return result;
    }
    
private:
    std::map<std::string, VersionEntry> versions_;
    std::string current_version_;
};
```

### 5.2 Snapshot Catalog Storage

```sql
-- SQL Schema for Snapshot Catalog
CREATE TABLE snapshot_catalog (
    snapshot_id VARCHAR(64) PRIMARY KEY,
    blob_id VARCHAR(64) NOT NULL,
    version_id VARCHAR(32) NOT NULL,
    snapshot_timestamp_ns BIGINT NOT NULL,
    snapshot_sequence BIGINT NOT NULL,
    base_snapshot_id VARCHAR(64),
    isolation_level ENUM('NONE', 'CONSISTENT', 'SERIALIZABLE', 'MVCC'),
    is_immutable BOOLEAN DEFAULT FALSE,
    checksum_sha256 VARCHAR(64) NOT NULL,
    size_bytes BIGINT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP,
    INDEX idx_blob_version (blob_id, version_id),
    INDEX idx_timestamp (snapshot_timestamp_ns),
    INDEX idx_sequence (snapshot_sequence)
);

-- Track which snapshots are in use (prevent GC)
CREATE TABLE snapshot_references (
    snapshot_id VARCHAR(64),
    shard_id VARCHAR(64),
    reference_type ENUM('TRANSFER_SOURCE', 'TRANSFER_TARGET', 'CACHED'),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (snapshot_id, shard_id, reference_type),
    FOREIGN KEY (snapshot_id) REFERENCES snapshot_catalog(snapshot_id)
);
```

---

## 6. Best Practices

### 6.1 Empfohlene Defaults

```yaml
# Recommended Configuration
snapshot_config:
  # Isolation level
  default_isolation: SNAPSHOT_CONSISTENT  # Read Committed
  
  # Versioning
  enable_mvcc: true                       # Keep multiple versions
  version_retention_hours: 24             # Keep versions for 24h
  max_versions_per_blob: 10               # Max version chain length
  
  # Snapshot management
  snapshot_ttl_hours: 48                  # Snapshots expire after 48h
  auto_gc_enabled: true                   # Automatic garbage collection
  gc_interval_hours: 6                    # GC runs every 6 hours
  
  # Transfer behavior
  immutable_for_small_transfers: true     # Freeze source if transfer < 5 min
  small_transfer_threshold_mb: 1000       # 1 GB threshold
  
  # WAL replay
  enable_wal_catchup: true                # Apply WAL after snapshot transfer
  max_wal_catchup_mb: 100                 # Max WAL to replay (fallback to new snapshot)
```

### 6.2 Transfer Workflow mit Snapshot Awareness

```python
# Pseudo-code: Proper Snapshot-Aware Transfer
def transfer_blob_with_snapshot(blob_id, source_shard, target_shard):
    # 1. Create snapshot on source
    snapshot = source_shard.create_snapshot(
        blob_id=blob_id,
        isolation_level=SnapshotIsolation.SNAPSHOT_CONSISTENT
    )
    
    # Record snapshot metadata
    snapshot_metadata = {
        'snapshot_id': snapshot.id,
        'timestamp_ns': snapshot.timestamp_ns,
        'sequence': snapshot.sequence,
        'version': snapshot.version_id,
        'checksum': snapshot.checksum_sha256
    }
    
    # 2. Check if differential update possible
    target_versions = target_shard.list_versions(blob_id)
    enable_differential = len(target_versions) > 0
    
    # 3. Transfer with snapshot context
    request = BlobTransferRequest(
        blob_id=blob_id,
        snapshot_id=snapshot.id,
        snapshot_timestamp_ns=snapshot.timestamp_ns,
        snapshot_sequence=snapshot.sequence,
        isolation_level=SnapshotIsolation.SNAPSHOT_CONSISTENT,
        enable_differential=enable_differential,
        base_version=target_versions[-1] if enable_differential else None
    )
    
    # 4. Stream transfer
    response = transfer_blob(request)
    
    # 5. Verify snapshot integrity
    assert response.snapshot_id == snapshot.id
    assert response.checksum == snapshot.checksum_sha256
    
    # 6. Record on target
    target_shard.register_snapshot(
        snapshot_id=snapshot.id,
        timestamp_ns=snapshot.timestamp_ns,
        version_id=snapshot.version_id,
        source_shard_id=source_shard.id
    )
    
    # 7. Optional: WAL catchup if needed
    if config.enable_wal_catchup:
        wal_entries = source_shard.get_wal_since(snapshot.timestamp_ns)
        if wal_entries:
            target_shard.apply_wal(wal_entries)
    
    return snapshot_metadata
```

### 6.3 Conflict Detection & Resolution

```cpp
// Detect if snapshot is stale
class SnapshotConflictDetector {
public:
    enum ConflictResolution {
        ABORT_TRANSFER,         // Stop transfer, let user decide
        USE_SNAPSHOT,           // Continue with snapshot (ignore new writes)
        MERGE_WITH_WAL,         // Apply WAL to catch up
        TAKE_NEW_SNAPSHOT       // Discard old, create new snapshot
    };
    
    ConflictResolution detectAndResolve(
        const SnapshotMetadata& snapshot,
        uint64_t current_version_sequence
    ) {
        uint64_t snapshot_age_sec = 
            (currentTimestamp() - snapshot.snapshot_timestamp_ns) / 1e9;
        
        uint64_t sequence_delta = 
            current_version_sequence - snapshot.snapshot_sequence;
        
        // Snapshot is very stale
        if (snapshot_age_sec > 3600 || sequence_delta > 100) {
            return ConflictResolution::TAKE_NEW_SNAPSHOT;
        }
        
        // Moderate staleness - can catch up with WAL
        if (snapshot_age_sec > 300 || sequence_delta > 10) {
            return ConflictResolution::MERGE_WITH_WAL;
        }
        
        // Fresh snapshot - use as-is
        return ConflictResolution::USE_SNAPSHOT;
    }
};
```

---

## 7. Monitoring & Observability

### 7.1 Metrics

```cpp
// Snapshot-related metrics
namespace metrics {
    // Snapshot creation
    Counter snapshot_created_total{"snapshot_created_total"};
    Histogram snapshot_creation_duration_seconds{"snapshot_creation_duration_seconds"};
    
    // Snapshot age
    Gauge snapshot_age_seconds{"snapshot_age_seconds", {"snapshot_id"}};
    Gauge active_snapshots{"active_snapshots"};
    
    // Versioning
    Gauge version_chain_length{"version_chain_length", {"blob_id"}};
    Counter versions_created_total{"versions_created_total"};
    Counter versions_gc_total{"versions_gc_total"};
    
    // Transfers
    Counter transfers_with_stale_snapshot{"transfers_with_stale_snapshot"};
    Histogram snapshot_transfer_lag_seconds{"snapshot_transfer_lag_seconds"};
    
    // Consistency
    Counter consistency_violations{"consistency_violations", {"type"}};
    Counter wal_catchup_applied{"wal_catchup_applied"};
}
```

### 7.2 Alerts

```yaml
# Prometheus Alert Rules
groups:
  - name: snapshot_alerts
    rules:
      # Snapshot too old
      - alert: SnapshotStale
        expr: snapshot_age_seconds > 3600
        for: 5m
        annotations:
          summary: "Snapshot {{ $labels.snapshot_id }} is stale (> 1h old)"
          
      # Too many active snapshots
      - alert: TooManyActiveSnapshots
        expr: active_snapshots > 100
        for: 10m
        annotations:
          summary: "Too many active snapshots ({{ $value }}), possible GC issue"
          
      # Version chain too long
      - alert: VersionChainTooLong
        expr: version_chain_length > 50
        annotations:
          summary: "Version chain for {{ $labels.blob_id }} is too long ({{ $value }})"
```

---

## 8. Zusammenfassung

### 8.1 Key Takeaways

✅ **Alle Transfers sind temporale Snapshots**
- Transfers repräsentieren einen Zeitpunkt t₀
- Source kann sich während Transfer ändern
- Target erhält Snapshot-Version, nicht aktuelle Version

✅ **Isolation Levels sind kritisch**
- SNAPSHOT_CONSISTENT: Default, Read Committed
- SNAPSHOT_MVCC: Für Production-Systeme empfohlen
- SNAPSHOT_SERIALIZABLE: Für kritische Business-Logik

✅ **Versionierung ist Pflicht**
- snapshot_id: Eindeutige Identifikation
- snapshot_timestamp_ns: Temporale Ordnung
- snapshot_sequence: Monotone Sequenz

✅ **MVCC ist empfohlene Strategie**
- Multiple Versionen parallel
- Source bleibt verfügbar
- Konsistenz garantiert

✅ **WAL Replay für Consistency**
- Snapshot + WAL = Eventually Consistent
- Catch-up nach Transfer
- Kritisch für Financial/ACID

### 8.2 Implementation Checklist

- [x] Protobuf Messages mit Snapshot-Metadaten
- [ ] Snapshot Catalog Implementation
- [ ] MVCC Version Store
- [ ] WAL Replay Mechanismus
- [ ] Conflict Detection
- [ ] Garbage Collection
- [ ] Monitoring & Metrics
- [ ] Documentation & Examples

---

**Status:** Design Complete - Ready for Implementation  
**Autor:** ThemisDB Development Team  
**Review:** Pending
