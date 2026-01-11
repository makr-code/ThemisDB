# Git-ähnliche Features für ThemisDB MVCC

**Datum:** 11. Januar 2026  
**Version:** 1.0  
**Kategorie:** 🔬 Research  
**Status:** ✅ Analysiert

---

## 📑 Inhaltsverzeichnis

- [Executive Summary](#executive-summary)
- [Hintergrund](#hintergrund)
- [Vergleich: Git vs. ThemisDB MVCC](#vergleich-git-vs-themisdb-mvcc)
- [Bereits vorhandene Git-ähnliche Features](#bereits-vorhandene-git-ähnliche-features)
- [Fehlende Git-Features mit Potential](#fehlende-git-features-mit-potential)
- [Empfohlene Implementierungen](#empfohlene-implementierungen)
- [Nicht empfohlene Features](#nicht-empfohlene-features)
- [Implementierungsplan](#implementierungsplan)
- [Referenzen](#referenzen)

---

## Executive Summary

### Problem Statement
ThemisDB nutzt MVCC (Multi-Version Concurrency Control) als Wrapper um RocksDB, was konzeptionell hohe Ähnlichkeiten zu Git aufweist. Diese Analyse untersucht, welche zusätzlichen Git-Features für ThemisDB hilfreich wären bzw. bereits vorhanden sind aber nicht optimal genutzt werden.

### Kernerkenntnisse

**✅ Bereits vorhanden (stark):**
- Snapshot Isolation (wie Git commits)
- Temporal Queries (wie Git time-travel)
- Changefeed (wie Git log)
- Audit Logging (wie Git blame metadata)
- Version History (wie Git history)

**🟡 Teilweise vorhanden:**
- Content-Addressed Storage (RocksDB keys, aber nicht content-based)
- Branching (isolierte Transaktionen, aber keine persistenten Branches)
- Tagging (Version-Nummern, aber kein semantisches Tagging)

**❌ Fehlend (mit Potential):**
- Semantische Tags (Named Snapshots)
- Diff/Patch zwischen Versionen
- Merge zwischen Branches
- Cherry-Pick von einzelnen Änderungen
- Interactive Rebase für History Cleanup

### Empfehlung

**Priorität 1 - Schneller Mehrwert (2-3 Wochen):**
1. Named Snapshots (Semantic Tagging)
2. Version Diff API
3. Point-in-Time Recovery

**Priorität 2 - Mittelfristig (1-2 Monate):**
4. Persistent Branches
5. Three-Way Merge
6. Cherry-Pick API

**Nicht empfohlen:**
- Interactive Rebase (zu komplex für DB-Kontext)
- Force Push (Datenverlust-Risiko)
- Detached HEAD (verwirrend für DB-Nutzer)

---

## Hintergrund

### MVCC in ThemisDB

ThemisDB nutzt RocksDB's TransactionDB für MVCC:

```cpp
// Aktuelle Implementierung
class TransactionManager {
    RocksDBWrapper& db_;  // RocksDB TransactionDB
    
    // Features:
    // - Snapshot Isolation
    // - Write-Write Conflict Detection  
    // - Atomic Commit/Rollback
    // - Concurrent Reads ohne Locks
};
```

**Kernfunktionen:**
- Jede Transaktion erhält eine monotone Sequence Number
- Snapshots ermöglichen konsistente Reads
- Changefeeds tracken alle Änderungen
- Temporal Graphs unterstützen Zeit-basierte Queries

### Git's Datenmodell

Git ist ein **content-addressed filesystem** mit folgenden Kernkonzepten:

1. **Commits:** Snapshots mit Parent-Referenzen
2. **Branches:** Benannte Zeiger auf Commits
3. **Tags:** Unveränderliche Marker für wichtige Commits
4. **Merge:** Kombinieren von Branch-Histories
5. **Rebase:** History neu schreiben
6. **Cherry-Pick:** Einzelne Commits übertragen

---

## Vergleich: Git vs. ThemisDB MVCC

| Feature | Git | ThemisDB MVCC | Ähnlichkeit | Notizen |
|---------|-----|---------------|-------------|---------|
| **Snapshots** | Commits | Transactions | ✅ 95% | Beide sind atomare Snapshots |
| **History** | git log | Changefeed | ✅ 90% | Beide tracken chronologische Änderungen |
| **Branching** | Branches | - | ❌ 0% | ThemisDB hat keine persistenten Branches |
| **Tagging** | Tags | - | 🟡 30% | Nur numerische Versionen, keine Namen |
| **Merging** | 3-way merge | - | ❌ 0% | Keine Merge-Funktionalität |
| **Diff** | git diff | - | ❌ 0% | Keine strukturierte Diff-API |
| **Blame** | git blame | Audit Log | ✅ 80% | Audit Log hat Autor+Timestamp |
| **Time-Travel** | git checkout <hash> | Temporal Queries | ✅ 85% | Beide können alte Zustände abfragen |
| **Revert** | git revert | Rollback | ✅ 90% | Transactional Rollback vorhanden |
| **Content-Addressing** | SHA-1 | - | 🟡 40% | RocksDB Keys, aber nicht content-based |
| **Garbage Collection** | git gc | - | ❌ 0% | Keine MVCC GC implementiert |
| **Cherry-Pick** | git cherry-pick | - | ❌ 0% | Nicht vorhanden |
| **Rebase** | git rebase | - | ❌ 0% | Nicht vorhanden |

**Legende:**
- ✅ 80-100%: Feature ist vorhanden und gut umgesetzt
- 🟡 30-79%: Teilweise vorhanden oder konzeptionell ähnlich
- ❌ 0-29%: Feature fehlt oder ist nicht anwendbar

---

## Bereits vorhandene Git-ähnliche Features

### 1. ✅ Snapshots (wie Git Commits)

**Git Equivalent:** `git commit`

**ThemisDB Implementation:**
```cpp
// Transaction = Atomic Snapshot
auto txn_id = txn_mgr.beginTransaction(IsolationLevel::Snapshot);
auto txn = txn_mgr.getTransaction(txn_id);

// Änderungen sammeln
txn->putEntity("users", entity);
txn->addEdge(edge);

// Atomic Commit (wie git commit)
txn_mgr.commitTransaction(txn_id);
```

**Ähnlichkeit:** 95%
- Beide sind atomare Snapshots des gesamten Zustands
- Beide haben Timestamps und Author-Metadaten
- Beide unterstützen Rollback

**Unterschied:**
- Git Commits haben Parent-Referenzen (DAG)
- ThemisDB Transactions sind linear (kein DAG)

### 2. ✅ Change History (wie Git Log)

**Git Equivalent:** `git log --all`

**ThemisDB Implementation:**
```cpp
// Changefeed = Change History
Changefeed::ListOptions opts;
opts.from_sequence = 0;
opts.limit = 100;

auto events = changefeed.listEvents(opts);
for (const auto& event : events) {
    // sequence, type, key, timestamp, metadata
}
```

**Features:**
- Chronologische Event-Liste mit Sequence Numbers
- Filtern nach Event-Type (PUT, DELETE, COMMIT, ROLLBACK)
- Filtern nach Key-Prefix
- Long-Polling für Real-Time Updates
- Metadata: tx_id, user, timestamp

**Ähnlichkeit:** 90%

**Fehlend im Vergleich zu Git:**
- Keine Commit-Messages (nur Metadata)
- Keine Parent-Child Beziehungen sichtbar
- Keine visuellen History-Graphs

### 3. ✅ Temporal Queries (wie Git Time-Travel)

**Git Equivalent:** `git checkout <commit-hash>`

**ThemisDB Implementation:**
```cpp
// Temporal Graph Queries
TemporalFilter filter = TemporalFilter::at(timestamp_ms);

// Graph Traversal zu bestimmtem Zeitpunkt
auto edges = graph_index.getEdgesAt(node_id, timestamp_ms);
auto path = graph_index.traverseAt(start, end, timestamp_ms);
```

**Features:**
- Point-in-Time Graph Queries
- Valid-From/Valid-To Zeitfenster auf Kanten
- Null-Values = unbegrenzt gültig
- Filtern in AQL: `FILTER e.valid_from <= @t AND e.valid_to >= @t`

**Ähnlichkeit:** 85%

**Fehlend:**
- Nur Graph-spezifisch, nicht global
- Keine Named Time Points (Tags)
- Kein automatisches Point-in-Time Recovery für alle Daten

### 4. ✅ Audit Logging (wie Git Blame)

**Git Equivalent:** `git blame <file>`

**ThemisDB Implementation:**
```cpp
// Audit Logger
AuditLogger logger(db_);

logger.logAccess(user, action, resource, success, metadata);
logger.logDataChange(user, operation, table, pk, old_value, new_value);
logger.logAuthEvent(user, event_type, success, reason);

// Query audit trail
auto entries = logger.queryAuditLog(filter);
```

**Features:**
- User + Timestamp für alle Änderungen
- Before/After Values (wie Git diff)
- Success/Failure Tracking
- Compliance-Ready (DSGVO, HIPAA)

**Ähnlichkeit:** 80%

**Unterschied:**
- Audit Log ist linear, nicht per-line wie Git blame
- Fokus auf Compliance, nicht auf Entwickler-Workflow

### 5. 🟡 Version Management (wie Git Branches - teilweise)

**Git Equivalent:** `git branch`, `git tag`

**ThemisDB Implementation:**
```cpp
// Content Version Manager (nur für Content, nicht global)
VersionManager vm;

int v = vm.createVersion(
    content_id,
    content_hash,  // SHA-256
    size_bytes,
    author,
    comment
);

auto history = vm.getVersionHistory(content_id);
auto version = vm.getVersion(content_id, version_number);
```

**Features:**
- Version History mit SHA-256 Hashes
- Author + Comment (wie Git commit message)
- Timestamp Tracking

**Ähnlichkeit:** 30%

**Limitations:**
- Nur für Content Manager, nicht für ganze DB
- Nur numerische Versionen (1, 2, 3, ...), keine Namen
- Keine Branching oder Merging
- In-Memory Storage (nicht persistent in RocksDB)

---

## Fehlende Git-Features mit Potential

### 1. ❌ Named Snapshots / Semantic Tagging

**Git Equivalent:** `git tag v1.0.0 -m "Release 1.0"`

**Aktueller Stand:** Nicht vorhanden

**Warum nützlich:**
- Wichtige DB-Zustände markieren (vor Schema-Migration, vor großem Import)
- Point-in-Time Recovery zu benannten Zeitpunkten
- Audit/Compliance: "State vor Quartalswechsel"

**Use Cases:**
```
// Beispiel Use Cases
tag("before_schema_migration_2026_q1", "Vor Migration von User-Schema")
tag("quarterly_backup_2026_q1", "Quartalsabschluss")
tag("incident_recovery_point", "Letzter bekannter guter Zustand")

// Recovery
restoreToTag("before_schema_migration_2026_q1")
```

**Implementierungsaufwand:** 🟢 Niedrig (200-300 LOC)
- Tags als Metadaten in separater Column Family
- Mapping: tag_name → sequence_number + timestamp
- REST API: POST /tags, GET /tags, GET /tags/:name

**Priorität:** ⭐⭐⭐ Hoch

### 2. ❌ Diff API (Structured Diff)

**Git Equivalent:** `git diff <commit1> <commit2>`

**Aktueller Stand:** Nicht vorhanden

**Warum nützlich:**
- Audit: "Was hat sich zwischen gestern und heute geändert?"
- Debugging: "Welche Entities wurden modifiziert?"
- Compliance: Strukturierte Change Reports

**Use Cases:**
```
// API Design (Beispiel)
GET /api/v1/diff?from=sequence_100&to=sequence_200
GET /api/v1/diff?from=tag:before_migration&to=tag:after_migration
GET /api/v1/diff?from=2026-01-01T00:00:00Z&to=2026-01-11T23:59:59Z

// Response
{
  "added": [
    {"table": "users", "pk": "user_123", "entity": {...}}
  ],
  "modified": [
    {"table": "users", "pk": "user_456", "old": {...}, "new": {...}}
  ],
  "deleted": [
    {"table": "users", "pk": "user_789", "entity": {...}}
  ],
  "stats": {
    "total_changes": 150,
    "added_count": 50,
    "modified_count": 80,
    "deleted_count": 20
  }
}
```

**Implementierungsaufwand:** 🟡 Mittel (500-800 LOC)
- Changefeed zwischen zwei Sequenzen abfragen
- Events gruppieren nach Add/Modify/Delete
- Strukturierte JSON Response generieren
- Optional: Diffing-Algorithmus für Entity-Fields

**Priorität:** ⭐⭐ Mittel-Hoch

### 3. ❌ Persistent Branches

**Git Equivalent:** `git branch feature/new-schema`

**Aktueller Stand:** Nicht vorhanden

**Warum nützlich:**
- Schema-Migrations-Testing: Branch für Test, dann Merge
- A/B Testing: Zwei parallele Datenzustände
- What-If Analysen: "Was wäre wenn...?"

**Use Cases:**
```
// Beispiel Workflow
createBranch("feature/new-user-schema")
switchBranch("feature/new-user-schema")

// Änderungen in Branch
txn.putEntity("users", new_schema_entity)
txn.commit()

// Testen, Validieren...
if (tests_passed) {
    mergeBranch("feature/new-user-schema", "main")
} else {
    deleteBranch("feature/new-user-schema")
}
```

**Implementierungsaufwand:** 🔴 Hoch (1500-2000 LOC)
- Branch-Metadaten in RocksDB
- Copy-on-Write für Branch-Isolation
- Merge-Strategie (3-Way Merge)
- Conflict Resolution

**Priorität:** ⭐ Niedrig-Mittel (nur für spezifische Use Cases)

### 4. ❌ Three-Way Merge

**Git Equivalent:** `git merge feature-branch`

**Aktueller Stand:** Nicht vorhanden (Conflict Detection nur für concurrent writes)

**Warum nützlich:**
- Multi-User Schema Migrations
- Distributed Database Merges
- Branch-Reconciliation

**Herausforderungen:**
- Automatisches Merging nur für non-overlapping changes
- Conflict Resolution bei überlappenden Änderungen
- Merge-Strategie: Fast-Forward, Recursive, Ours, Theirs

**Implementierungsaufwand:** 🔴 Sehr Hoch (2000-3000 LOC)

**Priorität:** ⭐ Niedrig (nur wenn Branching implementiert)

### 5. ❌ Cherry-Pick

**Git Equivalent:** `git cherry-pick <commit-hash>`

**Aktueller Stand:** Nicht vorhanden

**Warum nützlich:**
- Einzelne Änderung aus einer Branch übernehmen
- Hotfix von Produktion zu Development kopieren
- Selektives Rollback einzelner Änderungen

**Use Cases:**
```
// Beispiel
cherryPick(
    from_sequence: 1234,
    to_branch: "current",
    filter: {
        table: "users",
        pk: "user_123"
    }
)
```

**Implementierungsaufwand:** 🟡 Mittel (600-900 LOC)
- Changefeed Event selektieren
- Replay in aktuelle Transaktion
- Conflict Detection bei Kollisionen

**Priorität:** ⭐ Niedrig (nice-to-have)

### 6. ❌ Content-Addressed Storage

**Git Equivalent:** SHA-1 für alle Objekte

**Aktueller Stand:** Teilweise (SHA-256 nur im Content Manager)

**Warum nützlich:**
- Deduplication: Identische Entities nur einmal speichern
- Integrität: Automatic Corruption Detection
- Effizienz: Shared Storage für identische Werte

**Implementierungsaufwand:** 🔴 Sehr Hoch (3000+ LOC, Breaking Change)
- Komplettes Storage-Redesign nötig
- Key-Format ändern: `entity:users:{pk}` → `blob:{sha256}`
- Index-Updates massiv betroffen
- Migration bestehender Daten

**Priorität:** ❌ Nicht empfohlen (zu disruptiv)

### 7. ❌ Garbage Collection

**Git Equivalent:** `git gc --aggressive`

**Aktueller Stand:** Nicht vorhanden (RocksDB Compaction, aber kein MVCC GC)

**Warum nützlich:**
- Alte Versionen löschen (nach Retention Policy)
- Disk Space reduzieren
- Performance verbessern

**Implementation:**
```cpp
class MVCCGarbageCollector {
    void collectGarbage(uint64_t retention_days) {
        uint64_t cutoff_timestamp = now() - retention_days * 86400;
        
        // Lösche alle Changefeed Events älter als cutoff
        changefeed_.deleteEventsOlderThan(cutoff_timestamp);
        
        // RocksDB Compaction triggern
        db_.compactRange();
    }
};
```

**Implementierungsaufwand:** 🟡 Mittel (400-600 LOC)

**Priorität:** ⭐⭐ Mittel (wichtig für Production)

---

## Empfohlene Implementierungen

### Priorität 1: Named Snapshots (Semantic Tagging)

**Aufwand:** 2-3 Wochen  
**Mehrwert:** Hoch  
**Risiko:** Niedrig

**Implementation Plan:**

```cpp
// File: include/transaction/snapshot_manager.h
class SnapshotManager {
public:
    struct Snapshot {
        std::string tag_name;
        uint64_t sequence_number;
        int64_t timestamp_ms;
        std::string description;
        std::string created_by;
    };
    
    // Tag Management
    Status createTag(const std::string& name, const std::string& description);
    Status deleteTag(const std::string& name);
    std::vector<Snapshot> listTags() const;
    std::optional<Snapshot> getTag(const std::string& name) const;
    
    // Point-in-Time Recovery
    Status restoreToTag(const std::string& tag_name);
    Status restoreToSequence(uint64_t sequence);
    Status restoreToTimestamp(int64_t timestamp_ms);
    
private:
    RocksDBWrapper& db_;
    Changefeed& changefeed_;
    
    // Storage: tags:{name} -> {sequence, timestamp, description, user}
};
```

**REST API:**
```
POST   /api/v1/snapshots/tags
GET    /api/v1/snapshots/tags
GET    /api/v1/snapshots/tags/:name
DELETE /api/v1/snapshots/tags/:name
POST   /api/v1/snapshots/restore
```

**Benefits:**
- Compliance: Named backups für Audits
- DevOps: Safe Points vor Deployments
- Testing: Wiederholbare Test-Zustände

### Priorität 2: Diff API

**Aufwand:** 3-4 Wochen  
**Mehrwert:** Mittel-Hoch  
**Risiko:** Niedrig

**Implementation Plan:**

```cpp
// File: include/analytics/diff_engine.h
class DiffEngine {
public:
    struct DiffResult {
        std::vector<ChangeEvent> added;
        std::vector<ChangeEvent> modified;  // mit old/new value
        std::vector<ChangeEvent> deleted;
        
        struct Stats {
            size_t added_count;
            size_t modified_count;
            size_t deleted_count;
            size_t total_changes;
        } stats;
    };
    
    DiffResult computeDiff(
        uint64_t from_sequence,
        uint64_t to_sequence,
        const DiffOptions& options = {}
    );
    
    DiffResult computeDiffByTag(
        const std::string& from_tag,
        const std::string& to_tag,
        const DiffOptions& options = {}
    );
    
    DiffResult computeDiffByTimestamp(
        int64_t from_timestamp,
        int64_t to_timestamp,
        const DiffOptions& options = {}
    );
    
private:
    Changefeed& changefeed_;
    SnapshotManager& snapshot_mgr_;
};
```

**REST API:**
```
GET /api/v1/diff?from=100&to=200
GET /api/v1/diff?from=tag:v1.0&to=tag:v2.0
GET /api/v1/diff?from=2026-01-01&to=2026-01-11&table=users
```

**Benefits:**
- Audit Reports automatisieren
- Change Tracking für Compliance
- Debugging: Was hat sich wann geändert?

### Priorität 3: Point-in-Time Recovery (PITR)

**Aufwand:** 2-3 Wochen  
**Mehrwert:** Hoch  
**Risiko:** Mittel (Daten-Recovery ist kritisch)

**Implementation Plan:**

```cpp
// File: include/storage/pitr_manager.h
class PITRManager {
public:
    // Restore entire database to specific point
    Status restoreToSequence(uint64_t target_sequence);
    Status restoreToTimestamp(int64_t timestamp_ms);
    Status restoreToTag(const std::string& tag_name);
    
    // Selective restore (nur bestimmte Tables)
    Status restoreTableToSequence(
        const std::string& table,
        uint64_t target_sequence
    );
    
    // Dry-Run: Was würde restored werden?
    RestorePreview previewRestore(uint64_t target_sequence);
    
private:
    RocksDBWrapper& db_;
    Changefeed& changefeed_;
    SnapshotManager& snapshot_mgr_;
    
    // Replay Changefeed events von target bis current, aber rückwärts
    Status replayBackward(uint64_t from, uint64_t to);
};
```

**Safety Features:**
- Automatic Snapshot vor Restore
- Dry-Run Mode mit Preview
- Abort on First Error
- Rollback bei Fehlern

**REST API:**
```
POST /api/v1/restore/pitr
{
  "target": "tag:before_migration",
  "dry_run": true,
  "selective": {
    "tables": ["users", "orders"]
  }
}
```

**Benefits:**
- Disaster Recovery
- Schema Migration Rollback
- Data Corruption Recovery

---

## Nicht empfohlene Features

### ❌ Interactive Rebase

**Warum nicht:**
- Zu komplex für DB-Kontext
- History rewrite = Audit Trail Probleme
- Compliance-Konflikte (DSGVO, HIPAA)
- Potential für Datenverlust

**Alternative:**
- Changefeeds sind append-only
- Keine History Manipulation
- Audit Trail bleibt konsistent

### ❌ Force Push / History Rewrite

**Warum nicht:**
- Datenverlust-Risiko
- Multi-User Sync Probleme
- Compliance Violations
- Breaks Immutability

### ❌ Detached HEAD State

**Warum nicht:**
- Verwirrend für DB-Nutzer (kein Git-Background)
- Kein klarer Use Case in DB-Kontext
- Alternative: Read-Only Snapshots besser

### ❌ Submodules

**Warum nicht:**
- DB-Föderierung ist besser durch Sharding gelöst
- Komplexität ohne klaren Mehrwert

### ❌ Stash

**Warum nicht:**
- Transaktionen haben bereits Rollback
- Kein "work in progress" Konzept in DB
- Alternative: Save Points innerhalb Transaktion

---

## Implementierungsplan

### Phase 1: Foundation (Sprint 1-2, 3-4 Wochen)

**Ziel:** Named Snapshots und Basis-Infrastruktur

**Tasks:**
1. SnapshotManager Klasse erstellen
   - Tag Storage in RocksDB CF
   - CRUD Operationen für Tags
   - Mapping zu Sequence Numbers
   
2. REST API Endpoints
   - POST /api/v1/snapshots/tags
   - GET /api/v1/snapshots/tags
   - DELETE /api/v1/snapshots/tags/:name
   
3. Tests
   - Unit Tests für SnapshotManager
   - Integration Tests mit Changefeed
   - REST API Tests

**Deliverables:**
- `include/transaction/snapshot_manager.h`
- `src/transaction/snapshot_manager.cpp`
- `src/server/snapshot_api_handler.cpp`
- `tests/test_snapshot_manager.cpp`
- Documentation: `docs/features/features_snapshots.md`

**Akzeptanzkriterien:**
- [ ] Tags können erstellt und abgerufen werden
- [ ] Tags sind persistent (überleben DB-Restart)
- [ ] REST API funktioniert
- [ ] 100% Test Coverage

### Phase 2: Diff Engine (Sprint 3-4, 3-4 Wochen)

**Ziel:** Strukturierte Diff-API zwischen Snapshots

**Tasks:**
1. DiffEngine Klasse
   - Changefeed Range Queries
   - Event Kategorisierung (Add/Modify/Delete)
   - Efficient Diff Algorithm
   
2. REST API
   - GET /api/v1/diff mit Query Parameters
   - JSON Response Format
   - Pagination für große Diffs
   
3. Performance Optimization
   - Caching häufiger Diff-Anfragen
   - Parallel Processing
   - Incremental Diff Computation

**Deliverables:**
- `include/analytics/diff_engine.h`
- `src/analytics/diff_engine.cpp`
- `src/server/diff_api_handler.cpp`
- `tests/test_diff_engine.cpp`
- Benchmarks: `benchmarks/bench_diff_engine.cpp`

**Akzeptanzkriterien:**
- [ ] Diff zwischen beliebigen Sequences
- [ ] Diff zwischen Tags
- [ ] Diff zwischen Timestamps
- [ ] Performance: <100ms für 1000 Changes
- [ ] Pagination funktioniert

### Phase 3: Point-in-Time Recovery (Sprint 5-6, 3-4 Wochen)

**Ziel:** Safe Restore zu beliebigem Zeitpunkt

**Tasks:**
1. PITRManager Implementation
   - Backward Replay von Changefeed
   - Snapshot-Before-Restore
   - Dry-Run Mode
   
2. Safety Features
   - Validate vor Restore
   - Automatic Backup
   - Rollback bei Errors
   
3. REST API
   - POST /api/v1/restore/pitr
   - Preview Endpoint
   - Status Tracking

**Deliverables:**
- `include/storage/pitr_manager.h`
- `src/storage/pitr_manager.cpp`
- `src/server/pitr_api_handler.cpp`
- `tests/test_pitr_manager.cpp`
- Disaster Recovery Guide

**Akzeptanzkriterien:**
- [ ] Restore funktioniert korrekt
- [ ] Automatic Backup vor Restore
- [ ] Dry-Run gibt korrektes Preview
- [ ] Rollback bei Fehlern
- [ ] Comprehensive Error Handling

### Phase 4: Optional Features (Future)

**Nur bei Bedarf implementieren:**

1. **Persistent Branches** (6-8 Wochen)
   - Branch Storage in RocksDB
   - Copy-on-Write Mechanism
   - Branch Switching API
   
2. **Merge Engine** (8-10 Wochen)
   - Three-Way Merge Algorithm
   - Conflict Detection
   - Manual Conflict Resolution
   
3. **Cherry-Pick** (3-4 Wochen)
   - Selective Event Replay
   - Conflict Handling

**Entscheidungskriterien:**
- Klarer Business Case
- User Demand
- Keine Workarounds verfügbar

---

## Testing Strategy

### Unit Tests

**Coverage Ziel:** 95%+

```cpp
// Example: test_snapshot_manager.cpp
TEST(SnapshotManager, CreateAndRetrieveTag) {
    SnapshotManager mgr(db_);
    
    auto status = mgr.createTag("v1.0", "Release 1.0");
    ASSERT_TRUE(status.ok);
    
    auto tag = mgr.getTag("v1.0");
    ASSERT_TRUE(tag.has_value());
    EXPECT_EQ(tag->tag_name, "v1.0");
    EXPECT_EQ(tag->description, "Release 1.0");
}

TEST(SnapshotManager, DuplicateTagFails) {
    SnapshotManager mgr(db_);
    
    mgr.createTag("v1.0", "First");
    auto status = mgr.createTag("v1.0", "Duplicate");
    
    EXPECT_FALSE(status.ok);
    EXPECT_THAT(status.message, HasSubstr("already exists"));
}
```

### Integration Tests

**Szenarien:**
1. Tag erstellen → Änderungen machen → Zu Tag restoren
2. Diff zwischen zwei Tags
3. PITR mit großen Datenmengen (1M+ Events)
4. Concurrent Tag Creation (Race Conditions)
5. DB Restart mit Tags

### Performance Tests

**Benchmarks:**
```cpp
// bench_snapshot_manager.cpp
BENCHMARK(CreateTag) {
    for (int i = 0; i < 1000; i++) {
        mgr.createTag("tag_" + std::to_string(i), "Description");
    }
}
// Target: <1ms per tag

BENCHMARK(RestoreToTag) {
    // Restore mit 100K Events
    // Target: <5 seconds
}

BENCHMARK(DiffBetweenTags) {
    // Diff mit 10K Changes
    // Target: <100ms
}
```

---

## Migration & Backward Compatibility

### Keine Breaking Changes

**Garantien:**
- Alle neuen Features sind opt-in
- Bestehende APIs bleiben unverändert
- Alte Changefeeds funktionieren weiter
- Keine Schema-Änderungen in Haupt-CFs

### Feature Flags

```yaml
# config.yaml
features:
  enable_named_snapshots: true
  enable_diff_api: true
  enable_pitr: false  # Standardmäßig aus (zu kritisch)
  
  snapshot_retention_days: 90
  max_tags: 1000
```

### Graceful Degradation

- Wenn Features disabled: 404 oder Feature-Not-Enabled Error
- Keine Crashes bei fehlenden CFs
- Auto-Migration beim ersten Aktivieren

---

## Monitoring & Observability

### Neue Metriken

**Prometheus Metrics:**
```cpp
// Snapshot Manager
themis_snapshots_total{type="created|deleted"}
themis_snapshots_active_count
themis_snapshot_restore_duration_seconds
themis_snapshot_restore_errors_total

// Diff Engine
themis_diff_requests_total
themis_diff_duration_seconds
themis_diff_changes_computed

// PITR
themis_pitr_restores_total{status="success|failed"}
themis_pitr_restore_duration_seconds
themis_pitr_events_replayed
```

### Logging

**Structured Logs:**
```cpp
THEMIS_INFO("Snapshot created: tag={}, sequence={}, user={}", 
            tag_name, sequence, user);
THEMIS_WARN("PITR restore initiated: target={}, current={}", 
            target_seq, current_seq);
THEMIS_ERROR("PITR restore failed: error={}, rollback_complete={}", 
             error, rollback_ok);
```

### Audit Trail

**Zusätzliche Audit Events:**
- SNAPSHOT_CREATED
- SNAPSHOT_DELETED
- PITR_RESTORE_STARTED
- PITR_RESTORE_COMPLETED
- PITR_RESTORE_FAILED
- DIFF_COMPUTED

---

## Security Considerations

### Access Control

**RBAC Policies:**
```yaml
roles:
  snapshot_admin:
    permissions:
      - snapshots:create
      - snapshots:delete
      - pitr:restore
      
  snapshot_viewer:
    permissions:
      - snapshots:list
      - snapshots:get
      - diff:compute
      
  regular_user:
    permissions:
      - diff:compute  # Nur eigene Daten
```

### Audit Requirements

**Compliance:**
- Jede Snapshot Operation wird geloggt
- PITR Restores sind vollständig audit-bar
- Diff API respektiert Row-Level Security
- Tags können nicht unbemerkt gelöscht werden

### Data Protection

**Encryption:**
- Tags enthalten keine sensiblen Daten (nur Metadaten)
- Diff API returned nur Daten mit korrekten Permissions
- PITR respektiert Field-Level Encryption

---

## Documentation

### User Documentation

**Zu erstellen:**
1. `docs/en/features/features_snapshots.md` - Named Snapshots Guide
2. `docs/en/features/features_diff.md` - Diff API Reference
3. `docs/en/features/features_pitr.md` - PITR User Guide
4. `docs/en/guides/disaster_recovery.md` - DR Best Practices

**Deutsche Versionen:**
- `docs/de/features/features_snapshots.md`
- `docs/de/features/features_diff.md`
- `docs/de/features/features_pitr.md`

### API Documentation

**OpenAPI Spec Update:**
```yaml
# openapi/openapi.yaml
paths:
  /api/v1/snapshots/tags:
    post:
      summary: Create named snapshot
      requestBody:
        required: true
        content:
          application/json:
            schema:
              type: object
              properties:
                name:
                  type: string
                  pattern: '^[a-z0-9_-]+$'
                description:
                  type: string
                  maxLength: 500
      responses:
        '201':
          description: Snapshot created
        '409':
          description: Tag already exists
```

### Developer Documentation

**Architecture Docs:**
- `docs/architecture/architecture_snapshots.md`
- `docs/architecture/architecture_diff.md`
- `docs/architecture/architecture_pitr.md`

**Code Examples:**
```cpp
// docs/examples/snapshots_example.cpp
#include <themis/transaction/snapshot_manager.h>

int main() {
    // Create snapshot before critical operation
    SnapshotManager mgr(db);
    mgr.createTag("before_migration", "Safe point");
    
    // Perform migration...
    migrate_schema();
    
    // If failed, restore
    if (!validate()) {
        mgr.restoreToTag("before_migration");
    }
}
```

---

## Referenzen

### Git Internals
- [Git Book - Git Internals](https://git-scm.com/book/en/v2/Git-Internals-Git-Objects)
- [Git Repository Layout](https://github.com/git/git/blob/master/Documentation/gitrepository-layout.txt)
- [Understanding Git Conceptually](https://www.sbf5.com/~cduan/technical/git/)

### MVCC in Databases
- [PostgreSQL MVCC Documentation](https://www.postgresql.org/docs/current/mvcc.html)
- [CockroachDB MVCC](https://www.cockroachlabs.com/docs/stable/architecture/transaction-layer.html)
- [Google Percolator Paper](https://research.google/pubs/pub36726/) - Large-scale incremental processing using distributed transactions
- [RocksDB Transactions](https://github.com/facebook/rocksdb/wiki/Transactions)

### Content-Addressed Storage
- [IPFS Whitepaper](https://github.com/ipfs/papers/raw/master/ipfs-cap2pfs/ipfs-p2p-file-system.pdf)
- [Git Packfile Format](https://git-scm.com/docs/pack-format)

### Time-Series Databases
- [InfluxDB Time-Series Design](https://docs.influxdata.com/influxdb/v2.0/reference/key-concepts/design-principles/)
- [TimescaleDB Architecture](https://docs.timescale.com/timescaledb/latest/overview/core-concepts/)

### Related ThemisDB Documentation
- [MVCC Design](architecture_mvcc.md) - Aktuelle MVCC Implementation
- [Changefeed Documentation](../cdc/changefeed.md) - CDC Implementation
- [Temporal Graphs](../index/temporal_graph.md) - Time-based queries
- [Audit Logging](../security/audit_logging.md) - Compliance features

---

## Anhang A: Vergleichstabelle aller Features

| Feature | Git | ThemisDB Status | Priorität | Aufwand | Mehrwert |
|---------|-----|-----------------|-----------|---------|----------|
| Snapshots | ✅ | ✅ Implementiert | - | - | - |
| History | ✅ | ✅ Changefeed | - | - | - |
| Time-Travel | ✅ | ✅ Temporal Queries | - | - | - |
| Audit Logging | ✅ | ✅ Audit Log | - | - | - |
| Named Snapshots | ✅ | ❌ → Implementieren | ⭐⭐⭐ Hoch | 🟢 Niedrig | 🟢 Hoch |
| Diff API | ✅ | ❌ → Implementieren | ⭐⭐ Mittel-Hoch | 🟡 Mittel | 🟢 Hoch |
| PITR | ✅ | ❌ → Implementieren | ⭐⭐⭐ Hoch | 🟡 Mittel | 🟢 Hoch |
| Branches | ✅ | ❌ Optional | ⭐ Niedrig | 🔴 Hoch | 🟡 Mittel |
| Merging | ✅ | ❌ Optional | ⭐ Niedrig | 🔴 Sehr Hoch | 🟡 Mittel |
| Cherry-Pick | ✅ | ❌ Optional | ⭐ Niedrig | 🟡 Mittel | 🟢 Niedrig |
| Garbage Collection | ✅ | ❌ → Implementieren | ⭐⭐ Mittel | 🟡 Mittel | 🟢 Mittel |
| Content-Addressing | ✅ | 🟡 Teilweise | ❌ Nicht empfohlen | 🔴 Sehr Hoch | 🔴 Niedrig |
| Rebase | ✅ | ❌ - | ❌ Nicht empfohlen | 🔴 Sehr Hoch | 🔴 Niedrig |
| Force Push | ✅ | ❌ - | ❌ Nicht empfohlen | - | 🔴 Negativ |

**Legende:**
- Priorität: ⭐⭐⭐ Hoch, ⭐⭐ Mittel, ⭐ Niedrig, ❌ Nicht empfohlen
- Aufwand: 🟢 Niedrig, 🟡 Mittel, 🔴 Hoch, 🔴 Sehr Hoch
- Mehrwert: 🟢 Hoch, 🟡 Mittel, 🔴 Niedrig, 🔴 Negativ

---

## Anhang B: Code-Beispiele

### Beispiel 1: Named Snapshot Workflow

```cpp
#include <themis/transaction/snapshot_manager.h>
#include <themis/transaction/transaction_manager.h>

// Before critical operation: Create named snapshot
SnapshotManager snapshot_mgr(db);
auto tag_result = snapshot_mgr.createTag(
    "before_q1_2026_migration",
    "Snapshot before Q1 2026 schema migration"
);

if (!tag_result.ok) {
    THEMIS_ERROR("Failed to create snapshot: {}", tag_result.message);
    return;
}

// Perform critical operation
TransactionManager txn_mgr(db, sec_idx, graph_idx, vec_idx);
auto txn_id = txn_mgr.beginTransaction();
auto txn = txn_mgr.getTransaction(txn_id);

// Schema migration...
for (const auto& entity : old_schema_entities) {
    auto new_entity = migrate_entity(entity);
    txn->putEntity("users", new_entity);
}

auto commit_status = txn_mgr.commitTransaction(txn_id);

if (!commit_status.ok) {
    THEMIS_ERROR("Migration failed, restoring to snapshot...");
    auto restore_status = snapshot_mgr.restoreToTag("before_q1_2026_migration");
    
    if (restore_status.ok) {
        THEMIS_INFO("Successfully restored to pre-migration state");
    } else {
        THEMIS_ERROR("CRITICAL: Restore failed: {}", restore_status.message);
    }
}
```

### Beispiel 2: Diff Between Two Time Points

```cpp
#include <themis/analytics/diff_engine.h>

DiffEngine diff_engine(changefeed, snapshot_mgr);

// Compute diff between yesterday and today
auto yesterday = now_timestamp_ms() - 86400000;  // 24h ago
auto today = now_timestamp_ms();

DiffEngine::DiffOptions opts;
opts.table_filter = "users";  // Only users table
opts.include_values = true;   // Include actual values in diff

auto diff = diff_engine.computeDiffByTimestamp(yesterday, today, opts);

// Print summary
std::cout << "Changes in last 24h:\n";
std::cout << "  Added: " << diff.stats.added_count << " entities\n";
std::cout << "  Modified: " << diff.stats.modified_count << " entities\n";
std::cout << "  Deleted: " << diff.stats.deleted_count << " entities\n";

// Process modifications
for (const auto& change : diff.modified) {
    std::cout << "Modified: " << change.key << "\n";
    std::cout << "  Old: " << change.old_value.value() << "\n";
    std::cout << "  New: " << change.new_value.value() << "\n";
}
```

### Beispiel 3: Point-in-Time Recovery

```cpp
#include <themis/storage/pitr_manager.h>

PITRManager pitr_mgr(db, changefeed, snapshot_mgr);

// First: Preview what would be restored
auto preview = pitr_mgr.previewRestore("before_q1_2026_migration");

std::cout << "PITR Preview:\n";
std::cout << "  Target sequence: " << preview.target_sequence << "\n";
std::cout << "  Current sequence: " << preview.current_sequence << "\n";
std::cout << "  Events to replay (backward): " << preview.event_count << "\n";
std::cout << "  Estimated duration: " << preview.estimated_duration_sec << "s\n";
std::cout << "  Tables affected: " << preview.affected_tables.size() << "\n";

// Confirm restore
std::cout << "Proceed with restore? (yes/no): ";
std::string confirm;
std::cin >> confirm;

if (confirm == "yes") {
    auto restore_status = pitr_mgr.restoreToTag("before_q1_2026_migration");
    
    if (restore_status.ok) {
        THEMIS_INFO("Restore completed successfully");
    } else {
        THEMIS_ERROR("Restore failed: {}", restore_status.message);
        // Automatic rollback already performed
    }
}
```

---

## Fazit

ThemisDB hat bereits eine **solide MVCC-Grundlage** mit Snapshots, Changefeeds und Temporal Queries. Die größten Lücken im Vergleich zu Git sind:

**Quick Wins (Priorität 1):**
1. ✅ Named Snapshots - Für Disaster Recovery und Compliance
2. ✅ Diff API - Für Audit Reports und Debugging
3. ✅ PITR - Für Safe Rollbacks

**Optional (bei Bedarf):**
4. Persistent Branches - Nur für spezielle Use Cases
5. Garbage Collection - Wichtig für Production Long-Term

**Nicht empfohlen:**
- Rebase, Force Push, Detached HEAD - Zu komplex, zu riskant, wenig Mehrwert

**Gesamtaufwand für Priorität 1:** 9-12 Wochen  
**Erwarteter ROI:** Hoch (Compliance, DR, DevOps Workflows)  
**Risiko:** Niedrig (keine Breaking Changes)

---

**Nächste Schritte:**
1. Review dieser Analyse mit Team
2. Priorisierung final abstimmen
3. Sprint Planning für Phase 1
4. Implementation starten

**Fragen?** Siehe [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
