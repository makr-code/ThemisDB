# MVCC Design für THEMIS

**Stand:** 6. April 2026  
**Version:** v0.0.33  
**Kategorie:** 🧩 Architecture

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Zwei MVCC-Subsysteme](#zwei-mvcc-subsysteme)
- [Implementierte Lösung: Engine-gestützte MVCC](#implementierte-lösung-engine-gestützte-mvcc)
- [Indexe mit MVCC](#indexe-mit-mvcc)
- [Aktuelle Situation vs. MVCC](#aktuelle-situation-vs-mvcc)
- [Design-Optionen (Archiv)](#design-optionen-archiv)
- [Kernkomponenten (Skizze)](#kernkomponenten-skizze)
- [Originales Design (Archiv)](#originales-design-archiv---nicht-implementiert)
- [Storage-Hinweise](#storage-hinweise)
- [Migration Plan (abgeschlossen)](#migration-plan)
- [Performance-Überlegungen](#performance-überlegungen)
- [Alternativen (Archiv)](#alternativen)
- [Ressourcen](#ressourcen)
- [Verwandte Dokumentation](#verwandte-dokumentation)

> **Weiterführende Dokumentation:**
> - Vollständige Referenz des `MVCCStore`/HLC-Subsystems → [Compendium Kapitel 18 – MVCC und HLC](../../../compendium/docs/chapter_mvcc_hlc.md)
> - Tuning und Konfiguration der `TransactionManager`-Schicht → [MVCC Tuning Guide (EN)](../../en/features/MVCC_TUNING_GUIDE.md)

## ✅ IMPLEMENTIERUNGSSTATUS: PRODUKTIONSREIF

MVCC ist vollständig implementiert (Snapshot-Isolation, Konflikterkennung, HLC-basierte Versionierung). Details zum physischen Layout und zu WAL/Snapshots siehe „RocksDB Storage“.

## Zwei MVCC-Subsysteme

ThemisDB nutzt zwei komplementäre MVCC-Schichten:

| Subsystem | Klassen | Zweck | Dokumentation |
|-----------|---------|-------|---------------|
| **MVCCStore / HLC** | `MVCCStore`, `HybridLogicalClock` | Per-Record-Versionierung mit HLC-Zeitstempeln; Snapshot-Reads ohne Locks; REST-API `/api/v1/mvcc/…` | [Compendium Kap. 18](../../../compendium/docs/chapter_mvcc_hlc.md) |
| **TransactionManager / RocksDB** | `TransactionManager`, `RocksDBWrapper::TransactionWrapper` | ACID-Transaktionen, Write-Write-Konflikterkennung, atomares Commit über alle Indizes | [MVCC Tuning Guide](../../en/features/MVCC_TUNING_GUIDE.md) |

Beide Schichten ergänzen sich: `MVCCStore` wird für HLC-gestempelten Zugriff auf einzelne Records und den MVCC-REST-Endpunkt eingesetzt; `TransactionManager` koordiniert zusammengesetzte Transaktionen über primäre Daten und alle Index-Typen.

### Test-Resultate
- **Transaction Tests**: 27/27 PASS (100%)
- **MVCC Tests**: 12/12 PASS (100%)
- **Performance** (TransactionManager-Schicht, Minimal-Overhead gegenüber WriteBatch):
  - SingleEntity: MVCC ~3.4k/s vs WriteBatch ~3.1k/s
  - Batch 100: WriteBatch ~27.8k/s
  - Rollback: MVCC ~35.3k/s
  - Snapshot Reads: ~44k/s
- Weitere Benchmarks (TransactionManager + RocksDB-Ebene): siehe [MVCC Tuning Guide](../../en/features/MVCC_TUNING_GUIDE.md#performance-benchmarks)

---

## Übersicht

MVCC (Multi-Version Concurrency Control) ermöglicht parallele Transaktionen ohne Locks durch Versionierung aller Daten.

## Implementierte Lösung: Engine-gestützte MVCC

Kerneigenschaften:

- **Snapshot Isolation**: Konsistentes Sichtfenster pro Transaktion
- **Conflict Detection**: Write-Write-Konflikte werden erkannt
- **Lock/Timeouts**: konfigurierbar
- **ACID-Garantien**: Atomarität über alle beteiligten Indizes

### Architektur

```
┌─────────────────────────────────────────────┐
│         TransactionManager                   │
│  (High-Level Transaction API)                │
└──────────────────┬──────────────────────────┘
                   │
┌──────────────────▼──────────────────────────┐
│      RocksDBWrapper::TransactionWrapper     │
│  • get(key) - snapshot reads                │
│  • put(key, value) - conflict detection     │
│  • del(key) - transactional deletes         │
│  • commit() - atomic persistence            │
│  • rollback() - automatic cleanup           │
└──────────────────┬──────────────────────────┘
                   │
┌──────────────────▼──────────────────────────┐
│          RocksDB TransactionDB              │
│  • Pessimistic Locking                      │
│  • Snapshot Isolation                       │
│  • Write-Write Conflict Detection           │
└─────────────────────────────────────────────┘
```

### Indexe mit MVCC

Alle Index-Manager unterstützen MVCC-Transaktionen:

- **SecondaryIndexManager**: Equality, Range, Sparse, Geo, TTL, Fulltext
- **GraphIndexManager**: Kanten und Adjazenz-Indizes
- **VectorIndexManager**: HNSW + Cache-Updates

Alle Index-Operationen sind **atomar** mit der Haupttransaktion - Rollback entfernt alle Änderungen vollständig.

---

## Aktuelle Situation vs. MVCC

### Vor MVCC (SAGA Pattern)
- ✅ Eventual Consistency durch Compensating Actions
- ✅ Vector Cache Consistency
- ❌ Last-Write-Wins bei konkurrierenden Writes
- ❌ Keine Snapshot Isolation
- ❌ Write-Write Conflicts werden nicht erkannt

### Mit MVCC (Implementiert)
- ✅ Vollständige Snapshot Isolation
- ✅ Write-Write Conflict Detection
- ✅ Concurrent Reads blockieren nie
- ✅ Atomare Rollbacks (inkl. Indizes)
- ✅ SAGA Pattern für Vector Cache (hybride Lösung)
- ⚠️  Höherer Speicherverbrauch durch RocksDB Locks
- ⚠️  Kein Point-in-Time Recovery (RocksDB Limitation)
---

## Design-Optionen (Archiv)

Die ursprünglich evaluierten Optionen sind unten dokumentiert. **Option 1 (RocksDB TransactionDB)** wurde implementiert.

## Kernkomponenten (Skizze)

### 1. Engine-Konfiguration (Beispiel)

```cpp
// In RocksDBWrapper::open()
rocksdb::TransactionDBOptions txn_db_options;
txn_db_options.transaction_lock_timeout = 1000;      // 1s Lock Timeout
txn_db_options.default_lock_timeout = 1000;          // 1s für alle Locks

rocksdb::TransactionOptions txn_options;
txn_options.set_snapshot = true;                     // Automatisches Snapshot
txn_options.deadlock_detect = true;                  // Deadlock Prevention

rocksdb::TransactionDB* txn_db;
rocksdb::TransactionDB::Open(options, txn_db_options, db_path, &txn_db);
```

### 2. Transaktions-API (Skizze)

```cpp
class TransactionWrapper {
public:
    // Reads (mit Snapshot Isolation)
    std::optional<std::vector<uint8_t>> get(const std::string& key);
    
    // Writes (mit Conflict Detection)
    bool put(const std::string& key, const std::vector<uint8_t>& value);
    bool del(const std::string& key);
    
    // Commit/Rollback
    bool commit();     // false = Conflict detected
    void rollback();   // Immer erfolgreich
    
    // Snapshot Management
    const rocksdb::Snapshot* getSnapshot() const;
    bool isActive() const;
};
```

### 3. Conflict Detection Flow

```
Thread A                    Thread B
────────────────────────────────────────────
txn_a = begin()
  snapshot_a = get_snapshot()
                              txn_b = begin()
                                snapshot_b = get_snapshot()
                              
put("user:1", data_a)       put("user:1", data_b)
  ✅ Lock acquired            ⏳ Waiting for lock...
  
commit()                    
  ✅ Lock released
  ✅ Committed
                              ❌ Conflict detected!
                              ❌ Abort & Rollback
```

### 4. Index-Integration

Alle Index-Operationen verwenden dieselbe MVCC-Transaktion:

```cpp
// In TransactionManager::Transaction::putEntity()
auto txn = mvcc_txn_;  // Shared MVCC transaction

// Primary data
txn->put(entityKey, entityData);

// Secondary indexes (atomisch mit primary data)
secIdx_.put(table, entity, *txn);  // MVCC variant

// Graph indexes (atomisch)
graphIdx_.addEdge(edge, *txn);     // MVCC variant

// Vector indexes (atomisch)
vecIdx_.addEntity(entity, *txn);   // MVCC variant

// Commit alles zusammen
txn->commit();  // Alles oder nichts
```

---

## Originales Design (Archiv - Nicht implementiert)

### Option 2/3: Custom Version Management

Die folgenden Designs wurden evaluiert aber nicht gewählt:
    
    pending_writes_[pk] = new_version;
    return Status::OK();
}
```

### 4. Commit-Protokoll

```cpp
Status Transaction::commit() {
    // 1. Atomare Version-Nummer holen
    uint64_t commit_version = global_version_counter_.fetch_add(1);
    
    // 2. Write-Write Conflicts prüfen
    for (auto& [pk, new_version] : pending_writes_) {
        auto latest = db_.getLatestVersion(pk);
        if (latest && latest->version_start > begin_version_) {
            rollback();
            return Status::Error("Serialization failure - retry transaction");
        }
    }
    
    // 3. Alte Versionen "abschließen"
    WriteBatch batch;
    for (auto& [pk, new_version] : pending_writes_) {
        auto old_version = db_.getLatestVersion(pk);
        if (old_version) {
            // Alte Version: version_end = commit_version
            old_version->version_end = commit_version;
            batch.put(makeVersionKey(pk, old_version->version_start), 
                     serialize(*old_version));
        }
        
        // Neue Version: version_start = commit_version
        new_version.version_start = commit_version;
        batch.put(makeVersionKey(pk, commit_version), 
                 serialize(new_version));
    }
    
    // 4. Atomarer Commit
    return batch.commit();
}
```

### 5. Garbage Collection

```cpp
class MVCCGarbageCollector {
    // Älteste aktive Transaktion finden
    uint64_t getOldestActiveTransaction() {
        std::lock_guard lock(txn_mutex_);
        uint64_t min_version = UINT64_MAX;
        for (auto& [txn_id, txn] : active_transactions_) {
            min_version = std::min(min_version, txn->begin_version);
        }
        return min_version;
    }
    
    // Alte Versionen löschen
    void collectGarbage() {
        uint64_t gc_horizon = getOldestActiveTransaction();
        
        // Alle Versionen mit version_end < gc_horizon können gelöscht werden
        for (auto& [pk, versions] : version_map_) {
            versions.erase(
                std::remove_if(versions.begin(), versions.end(),
                    [gc_horizon](const VersionedEntity& v) {
                        return v.version_end < gc_horizon;
                    }),
                versions.end()
            );
        }
    }
};
```

## Storage-Hinweise

Siehe ergänzend: [RocksDB Storage](../../storage/rocksdb_layout.md) für WAL, Snapshots und Compaction sowie Schlüsselpräfixe (entities, idx, ridx, graph, vector, changefeed, ts).

```cpp
#include <rocksdb/utilities/transaction_db.h>

class MVCCWrapper {
    rocksdb::TransactionDB* txn_db_;
    
    // RocksDB TransactionDB bietet:
    // - Built-in MVCC
    // - Optimistic/Pessimistic Concurrency Control
    // - Snapshot Isolation
    // - Conflict Detection
};
```

Hinweis: Engine-spezifische Tuning-Parameter (z. B. Lock-Timeouts, Snapshot-Handling) sollten anhand der Ziel-Workloads validiert werden.

### Option 2: Manuelle MVCC-Implementierung

Eigene Versionsverwaltung über RocksDB Keys:

```cpp
// Key-Format: entity:{table}:{pk}:v{version}
// Beispiel:   entity:users:user_123:v0000000000000042

class ManualMVCC {
    // Version-Range Scan
    std::vector<VersionedEntity> getAllVersions(const std::string& pk) {
        std::string prefix = "entity:" + table + ":" + pk + ":v";
        std::vector<VersionedEntity> versions;
        db_.scanPrefix(prefix, [&](auto key, auto value) {
            versions.push_back(deserialize(value));
            return true;
        });
        return versions;
    }
};
```

**Vorteile:**
- ✅ Volle Kontrolle
- ✅ Keine Breaking Changes
- ✅ Optimierbar für spezifische Workloads

**Nachteile:**
- ❌ Komplex zu implementieren
- ❌ Mehr Fehlerquellen
- ❌ GC muss selbst implementiert werden

## Index-Anpassungen

### Secondary Index mit MVCC

```cpp
// Aktuell: idx:users:age:25 -> ["user_123", "user_456"]
// MVCC:    idx:users:age:25:v42 -> ["user_123:v42", "user_456:v39"]

class MVCCSecondaryIndex {
    Status addToIndex(const BaseEntity& entity, uint64_t version) {
        // Index-Entry muss auch versioniert sein
        std::string idx_key = makeIndexKey(field, value, version);
        std::string pk_with_version = entity.getPrimaryKey() + ":v" + 
                                      std::to_string(version);
        // ...
    }
};
```

### Vector Index mit MVCC

```cpp
class MVCCVectorIndex {
    struct VersionedVector {
        std::string pk;
        uint64_t version;
        std::vector<float> embedding;
    };
    
    // HNSW Index: Nur aktuelle Versionen
    std::unordered_map<uint64_t, std::vector<VersionedVector>> version_snapshots_;
    
    std::vector<SearchResult> search(
        const std::vector<float>& query,
        uint64_t snapshot_version,
        size_t k
    ) {
        // Filter: Nur Vektoren die in snapshot_version sichtbar sind
        auto visible_vectors = getVisibleVectors(snapshot_version);
        return hnsw_index_.search(query, k, visible_vectors);
    }
};
```

### Graph Index mit MVCC

```cpp
class MVCCGraphIndex {
    // Kanten versionieren
    struct VersionedEdge {
        std::string edge_id;
        std::string from_node;
        std::string to_node;
        uint64_t version_start;
        uint64_t version_end;
    };
    
    // Graph-Traversierung mit Snapshot
    std::vector<std::string> traverse(
        const std::string& start_node,
        uint64_t snapshot_version
    ) {
        // Nur Kanten verwenden, die in snapshot_version sichtbar sind
        auto visible_edges = getVisibleEdges(snapshot_version);
        return bfs(start_node, visible_edges);
    }
};
```

## Migration Plan

### Phase 1: Foundation (2-3 Wochen)
1. **RocksDB TransactionDB Migration**
   - `RocksDBWrapper` zu `TransactionDB` migrieren
   - Snapshot-Management implementieren
   - Bestehende Tests anpassen

2. **TransactionManager Refactoring**
   - `begin()` gibt `rocksdb::Transaction*` zurück
   - Snapshot beim `begin()` erstellen
   - Commit/Rollback über TransactionDB

### Phase 2: Index Integration (2-3 Wochen)
3. **Secondary Index MVCC**
   - Index-Einträge versionieren
   - Range-Queries mit Snapshot
   - Visibility-Filter

4. **Vector Index MVCC**
   - Version-aware HNSW
   - Snapshot-basierte Suche
   - In-Memory Cache pro Version

5. **Graph Index MVCC**
   - Versionierte Kanten
   - Snapshot-Traversierung
   - Temporal Graph Queries

### Phase 3: Advanced Features (1-2 Wochen)
6. **Garbage Collection**
   - Background Thread
   - Version-Pruning
   - Configurable Retention

7. **Performance Optimization**
   - Version-Caching
   - Optimistic Locking
   - Batch Conflict Detection

### Phase 4: Testing & Documentation (1 Woche)
8. **Test Suite**
   - Concurrent Transaction Tests
   - Conflict Detection Tests
   - Snapshot Isolation Tests

9. **Documentation**
   - MVCC Architecture Guide
   - Migration Guide
   - Performance Tuning Guide

## Benötigte Änderungen

### Dateien zu erstellen:
- `include/transaction/mvcc_manager.h` - MVCC Coordination
- `src/transaction/mvcc_manager.cpp`
- `include/storage/versioned_entity.h` - Version-aware Entity
- `src/storage/versioned_entity.cpp`
- `include/transaction/garbage_collector.h` - GC
- `src/transaction/garbage_collector.cpp`

### Dateien zu ändern:
- `include/storage/rocksdb_wrapper.h` - TransactionDB statt DB
- `src/storage/rocksdb_wrapper.cpp`
- `include/transaction/transaction_manager.h` - Snapshot Support
- `src/transaction/transaction_manager.cpp`
- `include/index/secondary_index.h` - Versionierte Indizes
- `src/index/secondary_index.cpp`
- `include/index/vector_index.h` - Snapshot-aware Search
- `src/index/vector_index.cpp`
- `include/index/graph_index.h` - Temporal Graphs
- `src/index/graph_index.cpp`

### Dependencies:
```json
// vcpkg.json
{
  "dependencies": [
    "rocksdb[core,lz4,zlib,zstd,transactions]"  // +transactions feature
  ]
}
```

## Performance-Überlegungen

### Speicher-Overhead
- **Pro Version**: ~100 Bytes Metadata + Entity-Größe
- **Beispiel**: 1M Entities, 10 Versionen = ~1GB zusätzlicher Speicher
- **Mitigation**: Aggressive GC, Configurable Retention

### Write Amplification
- **Aktuell**: 1 Write = 1 RocksDB Put
- **MVCC**: 1 Write = 2 Puts (alte Version update + neue Version insert)
- **Mitigation**: RocksDB Compaction optimieren

### Read Performance
- **Snapshot Reads**: +5-10% Overhead (Version-Check)
- **Range Queries**: +10-20% Overhead (Visibility-Filter)
- **Mitigation**: Version-Cache, Bloom Filters

## Alternativen

### 1. Hybrid: SAGA + Optimistic Locking
- SAGA Pattern behalten
- Nur Write-Write Conflict Detection hinzufügen
- Kein vollständiges MVCC
- **Aufwand**: 1 Woche, **Benefit**: 70% von MVCC

### 2. PostgreSQL-Style MVCC
- Tuple-Versionierung in-place
- Vacuum statt GC
- **Aufwand**: 4-6 Wochen, **Benefit**: Bessere Performance

### 3. RocksDB OptimisticTransactionDB
- Leichtgewichtiger als TransactionDB
- Nur Conflict Detection, kein Locking
- **Aufwand**: 2-3 Wochen, **Benefit**: 80% von MVCC

## Ressourcen

- [RocksDB Transactions Wiki](https://github.com/facebook/rocksdb/wiki/Transactions)
- [PostgreSQL MVCC Internals](https://www.postgresql.org/docs/current/mvcc.html)
- [Cockroach MVCC Design](https://www.cockroachlabs.com/docs/stable/architecture/transaction-layer.html)
- [Percolator Paper](https://research.google/pubs/pub36726/) - Google's MVCC System
- [Git/GitHub/GitOps Vergleich](../../research/git_gitops_themis_vergleich.md) - Vergleich zur Code-Versionskontrolle

## Verwandte Dokumentation

| Dokument | Inhalt |
|----------|--------|
| [Compendium Kap. 18 – MVCC und HLC](../../../compendium/docs/chapter_mvcc_hlc.md) | Vollständige Referenz: `MVCCStore`, HLC-API, REST-Endpunkte, Metriken, Raft-Bridge |
| [MVCC Tuning Guide (EN)](../../en/features/MVCC_TUNING_GUIDE.md) | Konfiguration, Tuning und Benchmarks der TransactionManager-Schicht |
| [RocksDB Storage Layout](../../storage/rocksdb_layout.md) | Schlüsselpräfixe, WAL, Compaction, Snapshots |
| [Transaction Best Practices (EN)](../../en/features/TRANSACTION_BEST_PRACTICES.md) | Anwendungsempfehlungen für Transaktionen |
