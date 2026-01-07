# Replication Module

**Stand:** 22. Dezember 2025  
**Version:** v1.3.0  
**Kategorie:** 💾 Storage & Replication

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Replication Strategien](#replication-strategien)
- [Source-Code Referenz](#source-code-referenz)

---

## Übersicht

Das Replication-Modul bietet verteilte Datenkonsistenz für ThemisDB mit zwei Hauptstrategien:
1. **Leader-Follower Replication** - WAL-basiert mit automatischem Failover
2. **Multi-Master Replication** - Schreiben auf jedem Knoten mit CRDT-Konfliktlösung

## Source-Code Referenz

| Komponente | Header | Source | LOC |
|------------|--------|--------|-----|
| ReplicationManager | `replication_manager.h` | `replication_manager.cpp` | ~500 |
| MultiMasterReplication | `multi_master_replication.h` | - | ~900 |

**Gesamt:** 2 Header, 1 Source-Datei, ~1,600 LOC

## Implementierte Klassen

### Leader-Follower Replication

```cpp
// replication_manager.h
enum class ReplicationRole { LEADER, FOLLOWER, CANDIDATE };
enum class ReplicationMode { SYNC, ASYNC, SEMI_SYNC };

class ReplicationManager {
    void start();
    void stop();
    void promoteToLeader();
    void demoteToFollower();
    void appendEntry(const WALEntry& entry);
    ReplicationStatus getStatus();
};

class WALManager {
    uint64_t append(const WALEntry& entry);
    std::vector<WALEntry> readSince(uint64_t lsn);
    void checkpoint();
};

class LeaderElection {
    void startElection();
    void vote(const std::string& candidate_id);
    std::string getLeader();
};
```

### Multi-Master Replication

```cpp
// multi_master_replication.h
enum class MMNodeState { ACTIVE, SYNCING, PARTITIONED, RECOVERING, OFFLINE };
enum class ConflictType { CONCURRENT_UPDATE, DELETE_UPDATE, SCHEMA_CONFLICT, CONSTRAINT_VIOLATION };

class VectorClock {
    void increment(const std::string& node_id);
    void merge(const VectorClock& other);
    bool happensBefore(const VectorClock& other) const;
    bool isConcurrent(const VectorClock& other) const;
};

class HybridLogicalClock {
    Timestamp now();
    Timestamp receive(const Timestamp& received);
};

class ConflictResolver {
    virtual MMWriteEntry resolve(const MMWriteEntry& local, const MMWriteEntry& remote) = 0;
};

class LastWriteWinsResolver : public ConflictResolver { ... };
class CRDTMergeResolver : public ConflictResolver { ... };
class CustomResolver : public ConflictResolver { ... };

class MultiMasterReplicationManager {
    void start();
    void stop();
    void write(const MMWriteEntry& entry);
    void syncWithPeer(const std::string& peer_id);
    void resolveConflicts();
};
```

## Features

### Vector Clocks
- Kausalitätsverfolgung zwischen Knoten
- `happensBefore()` Ordnung
- Concurrent-Write-Erkennung

### Hybrid Logical Clocks (HLC)
- Kombination aus physischer Zeit und logischen Zählern
- Paper: "Logical Physical Clocks and Consistent Snapshots in Globally Distributed Databases"
- Millisekunden-Auflösung mit logischem Counter

### Konfliktlösung

| Strategie | Beschreibung | Use Case |
|-----------|--------------|----------|
| **Last-Write-Wins** | Neuester Timestamp gewinnt | Einfache Daten |
| **CRDT Merge** | G-Counter, PN-Counter, LWW-Register, OR-Set | Komplexe Daten |
| **Custom** | Anwendungsspezifische Logik | Domain-spezifisch |

### CRDT-Typen

```cpp
enum class CRDTType {
    G_COUNTER,      // Grow-only Counter
    PN_COUNTER,     // Positive-Negative Counter
    LWW_REGISTER,   // Last-Writer-Wins Register
    MV_REGISTER,    // Multi-Value Register
    G_SET,          // Grow-only Set
    OR_SET,         // Observed-Remove Set
    LWW_MAP         // Last-Writer-Wins Map
};
```

## Konfiguration

```yaml
replication:
  mode: multi_master  # leader_follower, multi_master
  
  leader_follower:
    sync_mode: semi_sync
    min_replicas: 2
    failover_timeout_ms: 5000
    
  multi_master:
    conflict_resolution: crdt_merge  # lww, crdt_merge, custom
    anti_entropy_interval_ms: 1000
    vector_clock_prune_interval_ms: 60000
```

## Verwandte Dokumentation

- [Sharding: Redundancy Architecture](../sharding/sharding_redundancy.md) - RAID-like Redundanz
- [Sharding: Streaming Protocol](../sharding/sharding_streaming.md) - Streaming-Architektur
- [Features: Transactions](../features/features_transactions.md) - Transaktions-Semantik
