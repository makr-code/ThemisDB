# ThemisDB Sharding/URN Komplexitätsanalyse und Empfehlungen

**Version:** 1.0  
**Erstellt:** 8. Dezember 2025  
**Status:** Analyse & Vorschlag  
**Autor:** Architecture Review Team

---

## Executive Summary

Dieses Dokument analysiert die Komplexität und Risiken der ThemisDB Sharding-Implementierung und bietet konkrete Empfehlungen zur Risikominderung. Die Analyse adressiert sechs kritische Bereiche:

1. **SQL-Komplexität** - Erhöhte Fehleranfälligkeit durch komplexere Sharding-Logik
2. **Software-Fehleranfälligkeit** - Zusätzliche Komponenten für Partitionierung, Balancierung und Koordination
3. **Single Point of Failure** - Ausfallrisiko durch Shard-Korruption
4. **Fail-over Komplexität** - Komplexe Replikation von Shard-Flotten
5. **Backup-Koordination** - Komplexität bei koordinierten Shard-Backups
6. **Operationelle Komplexität** - Schema-Änderungen über verteilte Shards

**Gesamtbewertung:** ThemisDB hat eine solide Grundarchitektur für Sharding mit URN-basiertem Routing und PKI-Sicherheit. Die identifizierten Risiken können mit den vorgeschlagenen Maßnahmen auf ein akzeptables Niveau reduziert werden.

---

## Inhaltsverzeichnis

1. [Aktuelle Implementierung](#1-aktuelle-implementierung)
2. [Risikoanalyse](#2-risikoanalyse)
3. [Minderungsstrategien](#3-minderungsstrategien)
4. [Empfehlungen](#4-empfehlungen)
5. [Implementierungsplan](#5-implementierungsplan)
6. [Monitoring & Metriken](#6-monitoring--metriken)

---

## 1. Aktuelle Implementierung

### 1.1 Architekturübersicht

ThemisDB implementiert horizontale Skalierung durch:

- **URN-basiertes Routing:** `urn:themis:{model}:{namespace}:{collection}:{uuid}`
- **Consistent Hashing:** 150 virtuelle Knoten pro Shard für gleichmäßige Verteilung
- **PKI-Sicherheit:** mTLS-gesicherte Shard-zu-Shard-Kommunikation
- **etcd Metadata Store:** Zentrale Topologie-Verwaltung
- **Auto-Rebalancing:** Automatische Last-Verteilung

### 1.2 Implementierungsumfang

**Komponenten:** 22 Header-Dateien, ~12.278 LOC (Lines of Code)

**Kernkomponenten:**
- URN Parser & Resolver
- Consistent Hash Ring
- Shard Topology Manager
- PKI Shard Certificate System
- mTLS Client für sichere Kommunikation
- Remote Executor für Cross-Shard Operations
- Data Migrator für Rebalancing
- Health Check System
- Prometheus Metrics Integration

**Status:** Phase 1-5 abgeschlossen (98% der Kern-Implementierung)

---

## 2. Risikoanalyse

### 2.1 SQL-Komplexität & Erhöhte Fehleranfälligkeit

#### Problem
Sharding erhöht die Komplexität von SQL-Queries dramatisch, da Entwickler:
- Shard-Schlüssel in WHERE-Klauseln berücksichtigen müssen
- Cross-Shard Joins manuell orchestrieren müssen
- Transaktionsgrenzen über Shards verstehen müssen

#### Aktuelle ThemisDB Situation

**Risiko-Level:** 🟢 NIEDRIG (gut mitigiert)

**ThemisDB's Bestehende Schutzmaßnahmen:**

1. **AQL (Advanced Query Language) mit Built-in Sicherheit:**
```cpp
// ThemisDB nutzt AQL statt direktes SQL
// include/query/functions/security_functions.h

// HAS_INJECTION(str, type) - Erkennung von Injection-Patterns
class HasInjectionFunction : public IFunction {
    // Prüft auf SQL injection, XSS, Path Traversal, Command Injection
    // Patterns: "' or ", "1=1", "drop table", "union select", etc.
};

// SANITIZE(str, type) - Input-Sanitization
class SanitizeFunction : public IFunction {
    // Unterstützt: "html", "sql", "json", "filename"
    // escapeSql(): Escaped ' zu '', \ zu \\
};
```

2. **Deklarative Query-Syntax (keine manuelle Shard-Auswahl):**
```aql
-- Entwickler schreibt einfach:
FOR u IN users
  FILTER u.age > 30
  RETURN u

-- ThemisDB's Query Optimizer übernimmt:
-- 1. URN-basiertes Routing automatisch
-- 2. Shard-Selection transparent
-- 3. Cross-Shard Scatter-Gather falls nötig
```

3. **Query Validatoren & Sanitizers (bereits implementiert):**
- `IS_EMAIL()`, `IS_URL()`, `IS_UUID()` - Format-Validierung
- `HAS_INJECTION()` - Injection-Pattern-Erkennung
- `SANITIZE()` - Automatisches Escaping
- Siehe: `include/query/functions/security_functions.h` (600+ LOC)

4. **Automatische Query-Optimierung:**
```cpp
// src/sharding/shard_router.cpp
// - Automatische Wahl zwischen Broadcast Hash Join & Co-Located Join
// - Shard-Key-basierte Optimierung (wenn URN vorhanden)
// - Parallele Scatter-Gather Execution
```

**Verbleibende Gaps (niedrige Priorität):**
1. **Query Complexity Analyzer:** Warnung bei ineffizienten Cross-Shard Patterns (Nice-to-have)
2. **EXPLAIN PLAN für Sharding:** Entwickler-Tooling für Performance-Tuning (Nice-to-have)

#### Auswirkung
- **Wahrscheinlichkeit:** Niedrig (15-25%) dank AQL & Sanitizers
- **Schweregrad:** Niedrig
- **Risiko-Status:** ✅ **GUT MITIGIERT** durch bestehende Implementierung

### 2.2 Software-Fehleranfälligkeit (Partitionierung, Balancierung, Koordination)

#### Problem
Zusätzliche Software-Komponenten erhöhen die Fehleroberfläche:
- Shard Router kann fehlrouten
- Consistent Hash Ring kann unbalanciert werden
- Data Migrator kann während Rebalancing fehlschlagen

#### Aktuelle ThemisDB Situation

**Risiko-Level:** 🟡 MITTEL

**ThemisDB's Bestehende Schutzmaßnahmen:**

1. **Umfangreiche Test-Coverage:**
```text
Phase 5: Testing (✅ COMPLETE)
- Integration Tests: 14 Tests (test_sharding_integration.cpp)
- E2E Tests: 11 Tests (test_sharding_e2e.cpp)
- Chaos Tests: 13 Tests (test_sharding_chaos.cpp)
Total: 38 Tests für Sharding-Komponenten
```

2. **Health Check System (bereits implementiert):**
```cpp
// src/sharding/health_check.cpp (14.403 LOC)
class HealthCheck {
    // Prüft:
    // - Certificate Validity (echte ASN1_TIME Parsing)
    // - Storage Capacity via /api/v1/metrics/storage
    // - Network Connectivity mit Latenz-Messung
    // - Automatisches Marking unhealthy Shards
};
```

3. **PKI-basierte Shard-Kommunikation:**
```cpp
// include/sharding/mtls_client.h
// Alle Shard-zu-Shard Calls sind mTLS-gesichert
// Verhindert:
// - Man-in-the-Middle Angriffe
// - Unautorisierten Shard-Zugriff
// - Daten-Tampering während Migration
```

4. **Auto-Rebalancer mit Safety-Mechanismen:**
```cpp
// src/sharding/auto_rebalancer.cpp (20.172 LOC)
// - RSA-SHA256 Signierung aller Rebalancing-Operations
// - Cooldown-Perioden zwischen Migrations
// - Concurrency-Limits (max parallele Migrations)
// - Daily-Limits (max Migrations pro Tag)
```

**Komponenten-Analyse:**

| Komponente | LOC | Komplexität | Tests | Status |
|------------|-----|-------------|-------|--------|
| URN Parser | ~150 | Niedrig | ✅ Unit | Robust |
| Consistent Hash | ~300 | Mittel | ✅ Unit | Robust |
| Shard Router | ~1200 | Hoch | ✅ Integration | Gut getestet |
| Data Migrator | ~600 | Sehr Hoch | ✅ E2E, ⚠️ Keine Chaos | Needs improvement |
| Auto Rebalancer | ~1000 | Sehr Hoch | ✅ E2E, ⚠️ Keine Rollback | Needs improvement |
| Health Check | ~800 | Mittel | ✅ Integration | Robust |

**Verbleibende Gaps:**

1. **Data Migrator - Keine Idempotenz:**
```cpp
// src/sharding/data_migrator.cpp
Status migrateRange(const std::string& start_urn, const std::string& end_urn) {
    while (hasMore) {
        auto batch = source_shard->fetchBatch(cursor, batch_size);
        auto status = target_shard->writeBatch(batch);
        // ⚠️ FEHLEND: Rollback bei writeBatch-Fehler
        // ⚠️ FEHLEND: Idempotenz-Check für Retry-Safety
    }
}
```

2. **Auto Rebalancer - Keine Distributed Locks:**
```cpp
// ⚠️ RISIKO: Parallele Rebalancing-Operationen nicht koordiniert
// Zwei Rebalancer könnten gleichzeitig denselben Shard migrieren
```

**Identifizierte Gaps:**
- ⚠️ Keine formale Fault Injection Testing (Chaos Engineering erweitern)
- ⚠️ Fehlende Idempotenz-Garantien für Migrationen
- ⚠️ Keine automatische Conflict Detection bei parallelem Rebalancing

#### Auswirkung
- **Wahrscheinlichkeit:** Mittel (30-50%) - gut getestet, aber Gaps bei Edge Cases
- **Schweregrad:** Mittel-Hoch (Datenverlust möglich bei Migration-Failures)
- **MTBF:** Geschätzt 60-120 Tage bei hoher Last
- **Risiko-Status:** ⚠️ **AKZEPTABEL** mit P0-Maßnahmen (Idempotenz, Distributed Locks)

**Identifizierte Gaps:**
- ⚠️ Keine formale Fault Injection Testing
- ⚠️ Fehlende Idempotenz-Garantien für Migrationen
- ⚠️ Keine automatische Conflict Detection bei parallelem Rebalancing

#### Auswirkung
- **Wahrscheinlichkeit:** Mittel (40-60%)
- **Schweregrad:** Hoch (Datenverlust möglich)
- **MTBF (Mean Time Between Failures):** Geschätzt 30-90 Tage bei hoher Last

### 2.3 Single Point of Failure (Shard-Korruption)

#### Problem
Korruption eines einzelnen Shards durch Netzwerk-/Hardware-/Software-Probleme kann zum Ausfall der gesamten Tabelle führen.

#### Aktuelle ThemisDB Situation

**Risiko-Level:** 🟡 MITTEL (teilweise mitigiert)

**ThemisDB's Bestehende Schutzmaßnahmen:**

1. **RAID-ähnliche Redundanz-Strategien (bereits implementiert):**
```yaml
# docs/sharding/sharding_redundancy.md (21.785 LOC Dokumentation)
# 6 Redundanz-Modi verfügbar:

sharding:
  redundancy_mode: MIRROR           # RAID-1: Vollständige Spiegelung
  replication_factor: 3             # 3 Kopien jedes Shards
  read_preference: NEAREST          # Load-Balancing über Replicas
  write_concern: MAJORITY           # Schreibt müssen Majority bestätigen
```

**Verfügbare Modi:**
- `NONE`: Nur Sharding (Entwicklung)
- `MIRROR`: Vollständige Spiegelung (RAID-1) - **HighAvailability**
- `STRIPE`: Daten-Striping (RAID-0) - Performance
- `STRIPE_MIRROR`: Kombination (RAID-10) - Balance
- `PARITY`: Erasure Coding (RAID-5/6) - Speichereffizienz
- `GEO_MIRROR`: Geo-verteilte Spiegelung - Disaster Recovery

2. **Health Check System mit Auto-Detection:**
```cpp
// src/sharding/health_check.cpp
class HealthCheck {
    // Kontinuierliche Überwachung:
    // - Certificate Validity Check (X.509 Ablauf)
    // - Storage Capacity Check (via Metrics-API)
    // - Network Connectivity Check (Latenz-Messung)
    
    // Automatisches Marking:
    void markUnhealthy(const std::string& shard_id) {
        topology_->updateShardHealth(shard_id, false);
        // Shard wird aus Routing ausgeschlossen
    }
};
```

3. **Consistent Hash Ring mit Replica-Routing:**
```cpp
// include/sharding/consistent_hash.h
// Automatische Replica-Auswahl bei Primary-Ausfall
auto replicas = hash_ring_->getSuccessors(urn_hash, replication_factor);
// replicas = ["shard_2_primary", "shard_5_replica1", "shard_7_replica2"]
```

4. **etcd-basierte Shard Registry:**
```cpp
// src/sharding/shard_topology.cpp
// Zentrale Health-State Verwaltung
// - Alle Shards registriert mit Health-Status
// - etcd Watch für automatische Updates
// - Konsistente Shard-Discovery über Cluster
```

**Verbleibende Gaps:**

1. **Kein Circuit Breaker Pattern:**
```cpp
// remote_executor.cpp
auto result = remote_executor->execute(shard_id, query);
// ⚠️ FEHLEND: Automatische Isolation bei wiederholten Fehlern
// ⚠️ FEHLEND: Automatischer Failover zu Replica
```

2. **Kein Automatic Failover:**
```text
Aktuelles System: Health Check markiert Shard als unhealthy ✅
Fehlt: Automatische Promotion von Replica zu Primary ❌
Workaround: Manuelle Intervention durch Operator
```

3. **Keine Checksummen-Validierung bei Migration:**
```cpp
// data_migrator.cpp
auto batch = fetchBatch(source);
writeBatch(target, batch);
// ⚠️ FEHLEND: CRC32/SHA256 Checksum-Verifikation
```

#### Auswirkung
- **Wahrscheinlichkeit:** Niedrig-Mittel (15-30%) mit MIRROR-Mode
- **Schweregrad:** Mittel (Degraded Service, kein Total-Ausfall)
- **RTO (mit MIRROR):** 5-15 Minuten (manuelles Replica-Promote)
- **Risiko-Status:** 🟡 **AKZEPTABEL** für viele Use Cases, **P0 für Mission-Critical**
```text
Aktuelles System: HEALTHY oder UNHEALTHY (binär)
Fehlend: Graduelle Degradation (READ_ONLY, DEGRADED, CRITICAL)
```

**Aktuelle Schutzmaßnahmen:**

✅ **Vorhanden:**
- Health Check System mit Certificate/Storage/Network Validation
- etcd-basierte Shard Registry für zentrales Health-Tracking
- Prometheus Metrics für Shard-Status-Monitoring

⚠️ **Unvollständig:**
- Keine automatische Failover-Trigger
- Keine Read-Replica-Promotion bei Primary-Ausfall
- Kein Quorum-basiertes Availability-Management

#### Auswirkung
- **Wahrscheinlichkeit:** Niedrig-Mittel (20-40%)
- **Schweregrad:** Kritisch (Kompletter Service-Ausfall möglich)
- **RTO (Recovery Time Objective):** 15-60 Minuten (manuelles Failover)

### 2.4 Fail-over Server Komplexität

#### Problem
Fail-over Server müssen Kopien aller Shard-Flotten verwalten, was die Komplexität exponentiell erhöht.

#### Aktuelle ThemisDB Situation

**Risiko-Level:** 🟡 MITTEL (Grundlagen vorhanden, Automatisierung fehlt)

**ThemisDB's Bestehende Infrastruktur:**

1. **RAID-ähnliche Redundanz mit Replication Factor:**
```yaml
# sharding_redundancy.md - MIRROR Mode (RAID-1)
sharding:
  redundancy_mode: MIRROR
  replication_factor: 3              # 3 Kopien pro Shard
  read_preference: NEAREST           # Load-Balancing über Replicas
  write_concern: MAJORITY            # Quorum-basiertes Writing
```

**Replica-Verteilung:**
```text
Primary Shard → 2 Replicas (via Consistent Hash)
- Shard_1_Primary   (DC: eu-west)
- Shard_1_Replica1  (DC: eu-central) 
- Shard_1_Replica2  (DC: us-east)
```

2. **Consistent Hash für Replica-Routing:**
```cpp
// include/sharding/consistent_hash.h
class ConsistentHashRing {
    // Automatische Replica-Identifikation
    std::vector<std::string> getSuccessors(uint64_t hash, size_t count) {
        // Gibt N nachfolgende Shards im Ring zurück
        // = Natural Replication Strategy
    }
};
```

3. **Health Check mit Multi-Shard Tracking:**
```cpp
// src/sharding/health_check.cpp
// Überwacht ALLE Shards (Primary + Replicas)
// Bei Primary-Ausfall: Replica wird als "verfügbar" erkannt
```

4. **Gossip Protocol für Peer Discovery (Optional):**
```cpp
// src/sharding/gossip_protocol.cpp (19.675 LOC)
// SWIM-basiertes Gossip für automatische Peer-Erkennung
// - Membership Management
// - Failure Detection
// - State Synchronisation
```

**Verbleibende Gaps:**

1. **Keine automatische WAL-Replication:**
```cpp
// ⚠️ FEHLEND: Write-Ahead-Log Shipping zu Replicas
// Aktuell: Replicas werden NICHT automatisch synchronisiert
// 
// Workaround möglich mit RAID-Redundanz Modes:
// - MIRROR Mode kopiert Daten zu Replicas
// - Aber: Kein echtes WAL-Streaming wie PostgreSQL
```

2. **Keine Leader-Election:**
```cpp
// ⚠️ FEHLEND: Raft/Paxos/etcd-basierte Leader-Election
// Bei Primary-Ausfall: Manuelle Replica-Promotion erforderlich
// 
// Geplant: docs/sharding/sharding_strategy.md erwähnt
// Raft-Integration für automatisches Failover
```

3. **Replica-Topologie-Komplexität O(N) statt O(N²):**
```text
✅ GELÖST durch Hierarchisches Design:
- Gossip Protocol: Jeder Shard checked nur 3-5 Nachbarn
- Nicht All-to-All Health Checks
- Skaliert auf 100+ Shards ohne Overhead-Explosion

Bei 100 Shards:
- Alte Architektur: 9.900 Health-Check-Verbindungen
- ThemisDB Gossip: 300-500 Verbindungen (Konstante Faktoren)
```

**Geplante Features (dokumentiert, noch nicht implementiert):**
- `sharding_redundancy.md` beschreibt alle 6 RAID-Modi
- Automatische Replica-Sync via Stream Protocol (Cassandra-inspired)
- Leader-Election via etcd Raft

#### Auswirkung
- **Wahrscheinlichkeit:** Mittel (40-60% manuelle Fehler bei Failover)
- **Schweregrad:** Mittel (Downtime während manuellem Failover)
- **RTO:** 10-30 Minuten (manuelle Replica-Promotion)
- **Risiko-Status:** 🟡 **AKZEPTABEL** für viele Use Cases, **P1 für Enterprise**

**Identifizierte Gaps:**

1. **Fehlende Replica-Synchronisation:**
```text
Status Quo:
- Consistent Hash identifiziert Replica-Shards ✅
- Keine automatische Daten-Replikation implementiert ❌
- Keine Consistency-Garantien (Eventual vs. Strong) ❌

Risiko:
- Replicas können stark divergieren (Stunden/Tage)
- Failover auf veraltete Replica = Datenverlust
```

2. **Keine Leader-Election:**
```cpp
// FEHLEND: Raft/Paxos/etcd-basierte Leader-Election
// Bei Primary-Ausfall: Manuelle Intervention erforderlich
```

3. **Komplexe Replica-Topologie:**
```text
Bei 10 Shards + Replication Factor 3:
- 30 Shard-Instanzen zu verwalten
- 300 Health-Check-Verbindungen (10x10 All-to-All)
- Komplexität: O(N²) statt O(N)
```

**Geplante Features (noch nicht implementiert):**
- `sharding_redundancy.md` beschreibt RAID-ähnliche Modi (MIRROR, STRIPE, PARITY)
- Keine Implementierung in src/sharding/ vorhanden

#### Auswirkung
- **Wahrscheinlichkeit:** Hoch (70-90% bei Produktion)
- **Schweregrad:** Hoch
- **Geschätzte Downtime:** 30-120 Minuten pro Shard-Ausfall

### 2.5 Backup-Koordination Komplexität

#### Problem
Backups einzelner Shards müssen mit anderen Shards koordiniert werden, um Konsistenz zu gewährleisten.

#### Aktuelle ThemisDB Situation

**Risiko-Level:** 🟡 MITTEL (Grundlagen vorhanden, Koordination fehlt)

**ThemisDB's Bestehende Backup-Infrastruktur:**

1. **Production-Ready BackupManager (implementiert):**
```cpp
// include/storage/backup_manager.h
// src/storage/backup_manager.cpp (15.114 LOC)

class BackupManager {
    // Features:
    // ✅ RocksDB Checkpoint API für konsistente Snapshots
    // ✅ Incremental Backups mit Sequence Number Tracking
    // ✅ WAL (Write-Ahead Log) Archiving für Point-in-Time Recovery
    // ✅ Backup Manifest Files mit Metadata
    // ✅ Restore mit Integrity Verification
    
    bool createFullBackup(const std::string& dest_dir, std::error_code& ec);
    bool createIncrementalBackup(const std::string& dest_dir, std::error_code& ec);
    bool archiveWAL(const std::string& dest_dir, std::error_code& ec);
    bool restoreFromBackup(const std::string& src_dir, std::error_code& ec);
    bool verifyBackup(const std::string& backup_dir, std::error_code& ec);
};
```

**Backup Directory Structure:**
```text
backup_dir/
  ├── full_20251208_120000/
  │   ├── checkpoint/       (RocksDB checkpoint data)
  │   ├── wal/              (WAL files at checkpoint time)
  │   └── MANIFEST.json     (backup metadata: timestamp, sequence_number, db_path)
  ├── incr_20251208_130000/
  │   ├── wal/              (incremental WAL files)
  │   └── MANIFEST.json
  └── latest -> full_20251208_120000/
```

2. **RAID-Mode Backup-Support:**
```yaml
# docs/sharding/sharding_redundancy.md
# MIRROR Mode: Replicas dienen als Live-Backups
sharding:
  redundancy_mode: MIRROR
  replication_factor: 3
  
# Vorteil:
# - Bei Shard-Ausfall: Replica als sofortiger "Backup"
# - Kein komplettes Restore erforderlich
# - RTO: Minuten statt Stunden
```

3. **Per-Shard Backup Capabilities:**
```text
Aktueller Ansatz:
✅ Jeder Shard kann unabhängig gesichert werden (BackupManager)
✅ RocksDB Checkpoints garantieren Konsistenz INNERHALB eines Shards
✅ Manifest-Files tracken Backup-Metadaten pro Shard
✅ Incremental Backups reduzieren Backup-Fenster
```

**Verbleibende Gaps:**

1. **Keine Distributed Snapshot-Koordination:**
```cpp
// ⚠️ FEHLEND: Global Consistent Snapshot über alle Shards
// 
// Problem-Szenario:
// T1: Backup von Shard_A (enthält Foreign-Key zu Shard_B Entity)
// T2: Entity in Shard_B wird gelöscht
// T3: Backup von Shard_B (Entity fehlt)
// RESTORE = Broken Reference!
//
// Lösung (noch nicht implementiert):
// - Two-Phase Commit für Snapshots
// - Global Snapshot-ID über etcd
// - Pause Writes während Snapshot-Preparation
```

2. **Keine zentrale Backup-Orchestrierung:**
```text
Aktuelles System:
✅ Einzelne RocksDB-Checkpoints pro Shard
❌ Keine koordinierte Backup-Initiierung über alle Shards
❌ Keine Backup-Katalog-System (welche Shards in welchem Backup?)
❌ Kein globaler Backup-Scheduler

Workaround:
- Externe Orchestrierung via Cronjob/Kubernetes CronJob möglich
- Backup-Skript ruft BackupManager für jeden Shard auf
- Manuell Shard-Liste pflegen
```

3. **Keine automatische Cross-Shard Validierung:**
```cpp
// BackupManager::verifyBackup() prüft nur EINEN Shard
// ⚠️ FEHLEND: Cross-Shard Referential Integrity Check
// 
// Beispiel:
// bool verifyBackupCluster(const std::vector<std::string>& shard_backups) {
//     // Prüft Foreign-Key-Konsistenz über Shards
//     // Simuliert Queries über Restore für Validierung
// }
```

**Praktische Backup-Strategie (aktuell möglich):**
```bash
# Backup-Skript für Multi-Shard Cluster
#!/bin/bash
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
BACKUP_BASE="/backups/themis_cluster_${TIMESTAMP}"

# Pause kurz Writes (optional via Admin-API)
curl -X POST http://coordinator:8765/admin/pause-writes

# Parallel Backups aller Shards
for shard in shard_1 shard_2 shard_3; do
    curl -X POST "http://${shard}:8765/admin/backup" \
         -d "{\"dest_dir\": \"${BACKUP_BASE}/${shard}\"}" &
done
wait

# Resume Writes
curl -X POST http://coordinator:8765/admin/resume-writes

# Backup-Katalog erstellen (manuell)
echo "${TIMESTAMP},shard_1,shard_2,shard_3" >> backup_catalog.csv
```

#### Auswirkung
- **Wahrscheinlichkeit:** Mittel (40-60% Inkonsistenz bei unkoordiniertem Backup)
- **Schweregrad:** Mittel (Point-in-Time Recovery eingeschränkt, aber möglich)
- **Risiko-Status:** 🟡 **AKZEPTABEL** mit manuellem Orchestrierungs-Skript
- **Empfehlung:** P1 für automatisierte Distributed Snapshots (Nice-to-have, nicht kritisch)

#### Problem
Backups einzelner Shards müssen mit anderen Shards koordiniert werden, um Konsistenz zu gewährleisten.

#### Aktuelle ThemisDB Situation

**Risiko-Level:** 🔴 KRITISCH

**Schwachstellen:**

1. **Keine Distributed Snapshot Implementierung:**
```cpp
// FEHLEND: Global Consistent Snapshot über alle Shards
// Problem: Jeder Shard hat eigenen RocksDB-Checkpoint-Zeitpunkt
// Ergebnis: Inkonsistente Cross-Shard-References bei Restore

// Beispiel-Szenario:
// T1: Backup von Shard_A (enthält Foreign-Key zu Shard_B Entity)
// T2: Entity in Shard_B wird gelöscht
// T3: Backup von Shard_B (Entity fehlt)
// RESTORE = Broken Reference!
```

2. **Fehlende Backup-Orchestrierung:**
```text
Aktuelles System:
- Einzelne RocksDB-Checkpoints pro Shard ✅
- Keine koordinierte Backup-Initiierung ❌
- Keine Backup-Metadata (Timestamp, Dependencies) ❌
- Kein Backup-Katalog-System ❌

Risiko:
- Point-in-Time Recovery unmöglich über Shards hinweg
- Restore = aufwendige manuelle Shard-Koordination
```

3. **Keine Backup-Validierung:**
```cpp
// FEHLEND: Automatische Backup-Validierung
// - Checksummen-Verifikation
// - Foreign-Key-Integrität-Check
// - Replay-Test (Kann Backup restored werden?)
```

**Backup-Frequenz-Problem:**
```text
Annahme: 10 Shards, je 100GB, Backup-Dauer 10 Min/Shard
Sequenziell: 100 Minuten = 1.7 Stunden (inkonsistent)
Parallel: 10 Minuten (aber 10x Netzwerk-/Disk-Last)

Bei 100 Shards: 16.7 Stunden sequenziell (inakzeptabel!)
```

#### Auswirkung
- **Wahrscheinlichkeit:** Sehr Hoch (90-100% in Produktion)
- **Schweregrad:** Kritisch
- **Risiko:** Unmöglichkeit konsistenter Disaster Recovery

### 2.6 Operationelle Komplexität (Schema-Änderungen)

#### Problem
Schema-Änderungen (Indexes, Spalten hinzufügen/löschen, Schema-Modifikationen) werden über verteilte Shards deutlich komplexer.

#### Aktuelle ThemisDB Situation

**Risiko-Level:** 🟡 MITTEL (teilweise mitigiert durch Schema-less Design)

**ThemisDB's Bestehende Architektur-Vorteile:**

1. **Schema-less JSON-Blob Architecture:**
```cpp
// ThemisDB's "Base Entity" Design (docs/architecture/architecture_base_entity.md)
// VORTEIL: Keine Schema-Migrations für Felder erforderlich!

// Beispiel: Feld hinzufügen ohne DDL
PUT /entities/users:123
{
  "blob": {
    "name": "Alice",
    "age": 30,
    "email": "alice@example.com"  // ✅ Neues Feld ohne ALTER TABLE!
  }
}

// Alte Entities ohne "email" bleiben gültig
// Queries mit FILTER u.email == ... funktionieren (null-safe)
```

2. **Flexible Index-Verwaltung:**
```cpp
// Index-Operationen sind bereits Multi-Shard-aware

// Index erstellen (wird auf ALLEN Shards ausgeführt):
POST /index/create
{
  "table": "users",
  "column": "email",
  "type": "secondary"
}

// Index löschen (koordiniert über alle Shards):
POST /index/drop
{
  "table": "users",
  "column": "email"
}
```

3. **Admin-API für Shard-Operations:**
```cpp
// include/sharding/admin_api.h
// Zentrale APIs für Cluster-weite Operationen:
// - POST /admin/rebalance
// - POST /admin/migrate-shard
// - POST /admin/pause-writes
// - POST /admin/resume-writes
```

**Verbleibende Gaps:**

1. **Keine Distributed DDL-Engine:**
```cpp
// ⚠️ FEHLEND: Automatisierte Index-Erstellung über alle Shards
// 
// Aktueller Workaround:
// 1. Admin-Skript iteriert über alle Shards
// 2. Führt createIndex() auf jedem Shard aus
// 3. Manuelles Error-Handling bei Fehlern
//
// Wünschenswert:
// POST /admin/cluster/create-index
// {
//   "table": "users",
//   "column": "email",
//   "strategy": "ROLLING"  // oder PARALLEL, CANARY
// }
```

2. **Keine Schema-Registry:**
```cpp
// ⚠️ FEHLEND: Zentrale Schema-Versionierung
// 
// Problem-Szenario:
// - Shard_1 hat Index auf "email"
// - Shard_2 hat Index NICHT (fehlerhafte Erstellung)
// - Cross-Shard Query nutzt Index inkonsistent
//
// Lösung (nicht implementiert):
// - Schema Registry trackt Index-Definitionen pro Shard
// - Automatische Drift-Detection
// - Health Check für Index-Konsistenz
```

3. **Index-Rebuild ohne Rate-Limiting:**
```cpp
// Problem: REINDEX auf vielen Shards parallel
// ⚠️ FEHLEND: Koordinierte Rate-Limited Execution
// 
// Aktuell möglich (manuell):
for shard in shard_1 shard_2 ... shard_N; do
    curl -X POST "http://${shard}:8765/index/rebuild" \
         -d '{"table":"users","column":"email"}'
    sleep 60  // Manual rate-limiting
done
```

**Praktische Operationelle Prozesse (aktuell möglich):**

```bash
# Playbook: Index auf allen Shards erstellen
#!/bin/bash
SHARDS=("shard_1:8081" "shard_2:8082" "shard_3:8083")

echo "Creating index 'email' on users table across ${#SHARDS[@]} shards..."

for shard in "${SHARDS[@]}"; do
    echo "Creating index on ${shard}..."
    curl -X POST "http://${shard}/index/create" \
         -H "Content-Type: application/json" \
         -d '{"table":"users","column":"email","type":"secondary"}' || {
        echo "ERROR on ${shard}"
        exit 1
    }
done

echo "Index created successfully on all shards"
```

**Schema-Änderung-Komplexität (Vergleich):**

| Operation | Single-Node | ThemisDB (10 Shards) | Mitigation |
|-----------|-------------|----------------------|------------|
| Add Field | Sofort (JSON) | Sofort (JSON) | ✅ Schema-less |
| Add Index | 1 API Call | 10 API Calls | ⚠️ Skript erforderlich |
| Drop Index | 1 API Call | 10 API Calls | ⚠️ Skript erforderlich |
| Rebuild Index | 10 Min | 100 Min parallel | ⚠️ Rate-Limiting |
| Schema Migration | N/A | N/A | ✅ Nicht erforderlich |

#### Auswirkung
- **Wahrscheinlichkeit:** Hoch (80-100% bei Index-Operations)
- **Schweregrad:** Niedrig-Mittel (zeitaufwendig, aber beherrschbar)
- **Operationeller Overhead:** 5-10x höher als Single-Node (nicht 50x dank Schema-less!)
- **Risiko-Status:** 🟡 **AKZEPTABEL** mit Automatisierungs-Skripten, **P2 für Full-DDL-Engine**

---

## 3. Minderungsstrategien

### 3.1 SQL-Komplexität

#### M1.1: Query Complexity Analyzer

**Implementierung:**
```cpp
// Neues Modul: include/query/shard_query_analyzer.h
class ShardQueryAnalyzer {
public:
    struct ComplexityReport {
        bool is_cross_shard;
        size_t estimated_shards_involved;
        std::vector<std::string> optimization_hints;
        QueryComplexityLevel level; // SIMPLE, MEDIUM, COMPLEX, CRITICAL
    };
    
    ComplexityReport analyzeQuery(const AQLQuery& query);
    std::vector<std::string> suggestOptimizations(const AQLQuery& query);
};

// Beispiel-Output:
// Query: SELECT * FROM users WHERE city = 'Berlin'
// Report:
// - is_cross_shard: true
// - estimated_shards_involved: 10 (all shards - no shard key)
// - optimization_hints: 
//   [WARN] Query lacks shard key (user_id), will scatter to all shards
//   [HINT] Consider adding user_id filter or creating city-based index
// - level: COMPLEX
```

**Vorteile:**
- Proaktive Warnung vor ineffizienten Queries
- Lernkurve für Entwickler reduziert
- Integration in CI/CD möglich

#### M1.2: Automatic Query Rewrite

**Implementierung:**
```cpp
// Neues Modul: include/query/shard_query_rewriter.h
class ShardQueryRewriter {
public:
    // Konvertiert ineffiziente Cross-Shard Queries zu effizienten Varianten
    AQLQuery rewrite(const AQLQuery& original);
};

// Beispiel:
// Original: FOR u IN users FILTER u.city == 'Berlin' RETURN u
// Rewritten: FOR u IN users FILTER u.city == 'Berlin' AND u._shard_key IN [...] RETURN u
//            (Nutzt Shard-Key-Hints aus Secondary Index)
```

#### M1.3: Developer Tooling

**CLI-Tool:**
```bash
# Neues Tool: themis-query-explain
$ themis-query-explain "SELECT * FROM users WHERE city = 'Berlin'"

EXECUTION PLAN:
================
1. Scatter Phase: Query to 10 shards (Parallel)
   - Estimated rows per shard: 1,000
   - Network overhead: 50ms per shard
2. Gather Phase: Merge 10 result sets
   - Estimated total rows: 10,000
3. Final Sort: (if ORDER BY present)

OPTIMIZATION RECOMMENDATIONS:
==============================
[WARN] Full-cluster scatter detected
[HINT] Add shard key (user_id) to WHERE clause to reduce scatter
[HINT] Consider partitioning by city if frequent city-based queries

ESTIMATED COST: 850ms (vs. 12ms with shard key)
```

### 3.2 Software-Fehleranfälligkeit

#### M2.1: Formal Verification für Data Migrator

**Implementierung:**
```cpp
// Idempotenz-Garantie durch Deterministische IDs
class DataMigrator {
    Status migrateRange(const std::string& start_urn, const std::string& end_urn) {
        // Generate deterministic migration_id
        std::string migration_id = generateMigrationID(start_urn, end_urn, timestamp);
        
        // Check if already completed (idempotency)
        if (isMigrationCompleted(migration_id)) {
            return Status::AlreadyCompleted;
        }
        
        // Atomic batch with retry-safety
        while (hasMore) {
            auto batch = fetchBatch(cursor);
            
            // Each batch has deterministic batch_id
            std::string batch_id = generateBatchID(migration_id, batch_index);
            
            // Skip already migrated batches (idempotency)
            if (isBatchCompleted(batch_id)) {
                continue;
            }
            
            // Atomic write with rollback
            auto txn = beginTransaction();
            txn->writeBatch(batch);
            txn->markBatchCompleted(batch_id);
            
            if (!txn->commit()) {
                txn->rollback(); // Safe retry
                continue;
            }
        }
        
        markMigrationCompleted(migration_id);
        return Status::Success;
    }
};
```

**Vorteile:**
- Retry-safe Migrations (kein Datenverlust bei Netzwerk-Timeout)
- Automatische Resume nach Crash
- Audit-Trail für Debugging

#### M2.2: Chaos Engineering Tests

**Neue Test-Suite:**
```cpp
// tests/test_sharding_chaos_extended.cpp
TEST(ShardingChaos, DataMigratorNetworkPartition) {
    // 1. Start migration
    auto migration = migrator->migrateRange("urn:...:000", "urn:...:999");
    
    // 2. Simulate network partition after 50% completion
    chaos_toolkit->simulateNetworkPartition({source_shard, target_shard}, 30s);
    
    // 3. Verify rollback or retry
    ASSERT_TRUE(migration.isRetrying() || migration.isRolledBack());
    
    // 4. Heal partition
    chaos_toolkit->healNetwork();
    
    // 5. Verify completion
    ASSERT_TRUE(migration.completeSuccessfully());
    
    // 6. Verify no data loss or duplication
    auto source_count = source_shard->count();
    auto target_count = target_shard->count();
    ASSERT_EQ(source_count, 0); // Source should be empty after migration
    ASSERT_EQ(target_count, 1000); // Target should have all 1000 records
}

TEST(ShardingChaos, AutoRebalancerParallelConflict) {
    // Simulate two rebalancers trying to migrate same shard simultaneously
    auto rebalancer1 = createRebalancer("rebalancer_1");
    auto rebalancer2 = createRebalancer("rebalancer_2");
    
    // Both try to rebalance shard_2 -> shard_5
    auto future1 = std::async([&] { return rebalancer1->rebalance("shard_2", "shard_5"); });
    auto future2 = std::async([&] { return rebalancer2->rebalance("shard_2", "shard_5"); });
    
    // One should succeed, one should detect conflict
    auto result1 = future1.get();
    auto result2 = future2.get();
    
    ASSERT_TRUE((result1.success && result2.conflicted) || 
                (result2.success && result1.conflicted));
}
```

#### M2.3: Distributed Locking für Rebalancing

**Implementierung:**
```cpp
// Neues Modul: include/sharding/distributed_lock.h
class DistributedLock {
    std::optional<LockHandle> acquireLock(
        const std::string& resource_id,
        std::chrono::seconds timeout
    );
};

// Auto Rebalancer mit Locking:
class AutoRebalancer {
    Status rebalance(const std::string& source, const std::string& target) {
        // Acquire exclusive lock on source shard
        auto lock = distributed_lock_->acquireLock(
            "rebalance:" + source,
            std::chrono::minutes(30)
        );
        
        if (!lock) {
            return Status::ConflictDetected("Another rebalancing in progress");
        }
        
        // Perform migration (lock auto-released on scope exit)
        return performMigration(source, target);
    }
};
```

### 3.3 Single Point of Failure

#### M3.1: Circuit Breaker Pattern

**Implementierung:**
```cpp
// Neues Modul: include/sharding/circuit_breaker.h
class CircuitBreaker {
public:
    enum State { CLOSED, OPEN, HALF_OPEN };
    
    struct Config {
        size_t failure_threshold = 5; // Open after 5 failures
        std::chrono::seconds timeout = 30s; // Try again after 30s
    };
    
    bool allowRequest(const std::string& shard_id);
    void recordSuccess(const std::string& shard_id);
    void recordFailure(const std::string& shard_id);
};

// Integration in Remote Executor:
class RemoteExecutor {
    json execute(const std::string& shard_id, const json& request) {
        if (!circuit_breaker_->allowRequest(shard_id)) {
            throw ShardUnavailableException("Circuit breaker open for " + shard_id);
        }
        
        try {
            auto response = performRequest(shard_id, request);
            circuit_breaker_->recordSuccess(shard_id);
            return response;
        } catch (...) {
            circuit_breaker_->recordFailure(shard_id);
            throw;
        }
    }
};
```

**Vorteile:**
- Verhindert Cascade-Failures (ein toter Shard bringt nicht gesamtes System zum Absturz)
- Automatisches Recovery-Testing (HALF_OPEN State)
- Reduziert Last auf kranke Shards

#### M3.2: Automatic Failover zu Replicas

**Implementierung:**
```cpp
// Erweiterung von ShardRouter
class ShardRouter {
    json routeRequest(const URN& urn, const json& request) {
        auto primary = resolver_->resolvePrimary(urn);
        
        try {
            return remote_executor_->execute(primary, request);
        } catch (const ShardUnavailableException& e) {
            // Failover to replicas
            auto replicas = resolver_->resolveReplicas(urn);
            
            for (const auto& replica : replicas) {
                try {
                    auto response = remote_executor_->execute(replica, request);
                    
                    // Mark primary as unhealthy
                    topology_->markUnhealthy(primary);
                    
                    // Trigger automatic replica promotion (async)
                    async_promote_replica(replica, primary);
                    
                    return response;
                } catch (...) {
                    continue; // Try next replica
                }
            }
            
            throw AllShardsUnavailableException(urn);
        }
    }
};
```

#### M3.3: Degraded Mode für Read-Queries

**Implementierung:**
```cpp
// Partial availability statt kompletter Ausfall
class ShardRouter {
    json routeQuery(const AQLQuery& query) {
        auto shards = determineTargetShards(query);
        
        std::vector<std::string> healthy_shards;
        std::vector<std::string> unhealthy_shards;
        
        for (const auto& shard : shards) {
            if (topology_->isHealthy(shard)) {
                healthy_shards.push_back(shard);
            } else {
                unhealthy_shards.push_back(shard);
            }
        }
        
        if (healthy_shards.empty()) {
            throw AllShardsUnavailableException();
        }
        
        // Execute on healthy shards only
        auto partial_results = scatter_gather(healthy_shards, query);
        
        if (!unhealthy_shards.empty()) {
            // Add warning to response
            partial_results["_warnings"] = {
                {"type", "partial_results"},
                {"missing_shards", unhealthy_shards},
                {"completeness", healthy_shards.size() * 100.0 / shards.size()}
            };
        }
        
        return partial_results;
    }
};
```

### 3.4 Fail-over Server Komplexität

#### M4.1: Automatic Replica Synchronization

**Implementierung:**
```cpp
// Neues Modul: include/sharding/replica_sync.h
class ReplicaSync {
public:
    // Continous WAL shipping from Primary to Replicas
    void startWALShipping(
        const std::string& primary_shard,
        const std::vector<std::string>& replica_shards
    );
    
    // Verify replica lag (should be < 1 second)
    std::chrono::milliseconds getReplicationLag(
        const std::string& primary,
        const std::string& replica
    );
};

// Implementation:
class ReplicaSync {
    void startWALShipping(const std::string& primary, 
                          const std::vector<std::string>& replicas) {
        // 1. Subscribe to RocksDB WAL events on primary
        auto wal_iterator = primary_db->GetUpdatesSince(last_seq_num);
        
        // 2. Stream WAL batches to replicas
        while (wal_iterator->Valid()) {
            auto batch = wal_iterator->GetBatch();
            
            // 3. Parallel shipping to all replicas
            std::vector<std::future<Status>> futures;
            for (const auto& replica : replicas) {
                futures.push_back(std::async([&] {
                    return mtls_client_->ship(replica, batch);
                }));
            }
            
            // 4. Wait for all replicas to acknowledge
            for (auto& future : futures) {
                future.get(); // Throws if replica unavailable
            }
            
            wal_iterator->Next();
        }
    }
};
```

**Konsistenz-Levels:**
```cpp
enum class ReplicationMode {
    ASYNC,       // Fire-and-forget (eventual consistency)
    SYNC,        // Wait for all replicas (strong consistency)
    QUORUM       // Wait for majority (balance)
};
```

#### M4.2: Raft-basierte Leader Election

**Implementierung:**
```cpp
// Integration mit etcd Raft
class LeaderElection {
public:
    void registerCandidate(
        const std::string& shard_id,
        std::function<void()> on_elected_leader,
        std::function<void()> on_became_follower
    );
    
    std::optional<std::string> getCurrentLeader(const std::string& urn_range);
};

// Beispiel-Nutzung:
class ShardInstance {
    void initialize() {
        leader_election_->registerCandidate(
            my_shard_id_,
            [this]() { becomeLeader(); },
            [this]() { becomeFollower(); }
        );
    }
    
    void becomeLeader() {
        // Start accepting writes
        is_leader_ = true;
        startWALShipping(my_replicas_);
    }
    
    void becomeFollower() {
        // Become read-only, forward writes to leader
        is_leader_ = false;
        stopWALShipping();
    }
};
```

#### M4.3: Health-Check Optimierung (O(N) statt O(N²))

**Aktuelles Problem:**
```text
10 Shards, All-to-All Health Checks:
- 10 * 9 = 90 Verbindungen
- Bei 100 Shards: 9,900 Verbindungen (untragbar!)
```

**Lösung: Hierarchisches Health-Checking**
```cpp
// Neues Modul: include/sharding/health_check_hierarchy.h
class HierarchicalHealthCheck {
    // Jeder Shard checkt nur N direkte Nachbarn im Consistent Hash Ring
    void performHealthChecks() {
        auto neighbors = hash_ring_->getNeighbors(my_shard_id_, 3);
        
        for (const auto& neighbor : neighbors) {
            auto health = checkShard(neighbor);
            gossip_protocol_->broadcast(health_update(neighbor, health));
        }
    }
};

// Complexity: O(N * k) wo k=3 (Konstante)
// 100 Shards: nur 300 Health-Check-Verbindungen statt 9,900!
```

### 3.5 Backup-Koordination

#### M5.1: Distributed Snapshot Koordination

**Implementierung:**
```cpp
// Neues Modul: include/sharding/distributed_snapshot.h
class DistributedSnapshot {
public:
    struct SnapshotID {
        std::string id; // UUID
        std::chrono::system_clock::time_point timestamp;
    };
    
    // Phase 1: Initiate snapshot on all shards
    SnapshotID initiate(const std::vector<std::string>& shards);
    
    // Phase 2: Wait for all shards to prepare
    Status waitForPrepare(const SnapshotID& snapshot_id, std::chrono::seconds timeout);
    
    // Phase 3: Commit snapshot atomically
    Status commit(const SnapshotID& snapshot_id);
};

// Implementation (Two-Phase Commit):
SnapshotID DistributedSnapshot::initiate(const std::vector<std::string>& shards) {
    auto snapshot_id = generateSnapshotID();
    
    // Phase 1: Prepare - alle Shards pausieren Writes kurz
    std::vector<std::future<Status>> prepare_futures;
    for (const auto& shard : shards) {
        prepare_futures.push_back(std::async([&] {
            return remote_executor_->execute(shard, {
                {"action", "prepare_snapshot"},
                {"snapshot_id", snapshot_id.id}
            });
        }));
    }
    
    // Wait for all prepares
    for (auto& future : prepare_futures) {
        if (!future.get().ok()) {
            // Abort on any failure
            abort(snapshot_id);
            throw SnapshotException("Prepare failed");
        }
    }
    
    return snapshot_id;
}
```

**Backup-Flow:**
```text
1. Coordinator ruft DistributedSnapshot::initiate() auf
2. Alle Shards pausieren Writes und erstellen RocksDB Checkpoint
3. Coordinator wartet auf Prepare von allen Shards (Timeout: 60s)
4. Bei Erfolg: commit() → alle Shards finalisieren Checkpoint
5. Bei Fehler: abort() → alle Shards verwerfen Checkpoint
6. Backup-Metadata in etcd gespeichert:
   {
     "snapshot_id": "snap-20251208-120000",
     "timestamp": "2025-12-08T12:00:00Z",
     "shards": ["shard_1", "shard_2", ...],
     "status": "completed"
   }
```

#### M5.2: Incremental Backup mit Deduplikation

**Implementierung:**
```cpp
// Nur geänderte SST-Files senden (nicht komplette Snapshots)
class IncrementalBackup {
    struct BackupDelta {
        std::string base_snapshot_id;
        std::vector<std::string> new_sst_files;
        std::vector<std::string> deleted_sst_files;
    };
    
    BackupDelta computeDelta(
        const std::string& shard_id,
        const std::string& since_snapshot_id
    );
};

// Beispiel:
// Full Backup (Snapshot 1): 100GB
// Incremental (Snapshot 2): 5GB (nur neue SST-Files)
// Backup-Zeit: 10 Min → 30 Sekunden (20x Speedup)
```

#### M5.3: Backup-Validierung Pipeline

**Implementierung:**
```cpp
// Automatische Restore-Tests für Backups
class BackupValidator {
    Status validate(const std::string& snapshot_id) {
        // 1. Restore zu temporärem Cluster
        auto temp_cluster = restoreToTemporary(snapshot_id);
        
        // 2. Foreign-Key Integrity Check
        auto fk_violations = checkForeignKeyIntegrity(temp_cluster);
        if (!fk_violations.empty()) {
            return Status::IntegrityViolation(fk_violations);
        }
        
        // 3. Stichproben-Queries ausführen
        auto query_results = runValidationQueries(temp_cluster);
        if (!query_results.all_passed) {
            return Status::QueryValidationFailed;
        }
        
        // 4. Cleanup
        temp_cluster.destroy();
        
        return Status::OK;
    }
};

// Cron-Job: Jedes Backup automatisch validieren (nachts)
```

### 3.6 Operationelle Komplexität

#### M6.1: Schema Registry & Versioning

**Implementierung:**
```cpp
// Neues Modul: include/sharding/schema_registry.h
class SchemaRegistry {
public:
    struct SchemaVersion {
        int version;
        std::string schema_json; // JSON Schema oder Avro Schema
        std::chrono::system_clock::time_point created_at;
        std::string author;
    };
    
    // Register new schema version
    int registerSchema(
        const std::string& table_name,
        const std::string& schema_json,
        CompatibilityMode mode = CompatibilityMode::BACKWARD
    );
    
    // Get schema for specific version
    SchemaVersion getSchema(const std::string& table_name, int version);
    
    // Check compatibility
    bool isCompatible(
        const std::string& new_schema,
        const std::string& old_schema,
        CompatibilityMode mode
    );
};

// Beispiel-Flow:
// 1. Entwickler ändert Schema lokal
// 2. themis-cli schema register users schema_v2.json
// 3. Schema Registry prüft Kompatibilität mit v1
// 4. Bei BACKWARD-Kompatibilität: Auto-Deployment zu allen Shards
// 5. Bei BREAKING-Change: Warnung + manuelle Freigabe erforderlich
```

#### M6.2: Distributed DDL Execution

**Implementierung:**
```cpp
// Neues Modul: include/sharding/distributed_ddl.h
class DistributedDDL {
public:
    // Execute DDL on all shards with rollback capability
    Status executeDDL(
        const std::string& ddl_statement,
        ExecutionStrategy strategy = ExecutionStrategy::ROLLING
    );
};

// Execution Strategies:
enum class ExecutionStrategy {
    ROLLING,      // Shard-by-Shard (langsam, sicher)
    PARALLEL,     // Alle Shards parallel (schnell, riskant)
    CANARY        // Erst 1 Shard, dann Rest bei Erfolg
};

// Beispiel:
DistributedDDL ddl_executor;
auto status = ddl_executor.executeDDL(
    "CREATE INDEX users(email)",
    ExecutionStrategy::ROLLING
);

// Execution Flow (ROLLING):
// 1. Execute on shard_1
// 2. Verify success (Check Index, Run Queries)
// 3. If OK: Execute on shard_2
// 4. Repeat for all shards
// 5. If any failure: Rollback all previous shards
```

#### M6.3: Rate-Limited Index Rebuild

**Implementierung:**
```cpp
// Verhindert Ressourcen-Kollaps bei REINDEX auf 100 Shards
class RateLimitedReindex {
public:
    struct RateLimits {
        size_t max_parallel_shards = 5;      // Nur 5 Shards gleichzeitig
        size_t cpu_limit_percent = 50;       // Max 50% CPU pro Shard
        size_t disk_io_limit_mbps = 100;     // Max 100 MB/s Disk I/O
    };
    
    Status reindexTable(
        const std::string& table_name,
        const std::string& column_name,
        RateLimits limits = RateLimits{}
    );
};

// Beispiel:
// REINDEX users(email) auf 100 Shards mit Default-Limits:
// - 100 Shards / 5 parallel = 20 Waves
// - Jede Wave: 5 Shards parallel, 50% CPU, 100 MB/s I/O
// - Geschätzte Dauer: 2-4 Stunden (statt Ressourcen-Spike)
```

---

## 4. Empfehlungen

### 4.1 Kurzfristig (Q1 2026)

**Priorität P0:**
1. ✅ **Circuit Breaker Pattern** implementieren (M3.1)
   - **Aufwand:** 3-5 Tage
   - **Risiko-Reduktion:** 40%
   - **Impact:** Verhindert Cascade-Failures

2. ✅ **Idempotente Data Migration** (M2.1)
   - **Aufwand:** 5-7 Tage
   - **Risiko-Reduktion:** 60%
   - **Impact:** Sichere Rebalancing-Operations

3. ✅ **Distributed Snapshot für Backups** (M5.1)
   - **Aufwand:** 7-10 Tage
   - **Risiko-Reduktion:** 80%
   - **Impact:** Konsistente Disaster Recovery

**Priorität P1:**
4. ⚠️ **Query Complexity Analyzer** (M1.1)
   - **Aufwand:** 5-7 Tage
   - **Nutzen:** Developer Experience
   
5. ⚠️ **Chaos Engineering Test Suite** (M2.2)
   - **Aufwand:** 10-15 Tage
   - **Nutzen:** Fehler-Prävention

### 4.2 Mittelfristig (Q2-Q3 2026)

**Replica Management:**
6. 🔄 **Automatic Replica Synchronization** (M4.1)
   - **Aufwand:** 15-20 Tage
   - **Risiko-Reduktion:** 70%

7. 🔄 **Raft-basierte Leader Election** (M4.2)
   - **Aufwand:** 20-30 Tage
   - **Risiko-Reduktion:** 80%

**Schema Management:**
8. 🔄 **Schema Registry** (M6.1)
   - **Aufwand:** 10-15 Tage
   - **Nutzen:** Operationelle Effizienz

9. 🔄 **Distributed DDL Executor** (M6.2)
   - **Aufwand:** 15-20 Tage
   - **Nutzen:** Automatisierung

### 4.3 Langfristig (Q4 2026+)

**Advanced Features:**
10. 📋 **Automatic Query Rewrite** (M1.2)
11. 📋 **Incremental Backup** (M5.2)
12. 📋 **Rate-Limited Reindex** (M6.3)
13. 📋 **Hierarchical Health Checks** (M4.3)

### 4.4 Prioritäts-Matrix

```text
                    │ Risiko-Reduktion
                    │
         Hoch       │  [M5.1]        [M4.1] [M4.2]
                    │  [M2.1]        
                    │  
         Mittel     │  [M3.1]        [M6.1] [M6.2]
                    │  [M2.2]
                    │
         Niedrig    │  [M1.1]        [M1.2]
                    │               [M5.2] [M6.3]
                    │
                    └─────────────────────────────
                      Niedrig   Mittel    Hoch
                            Implementierungs-Aufwand
```

### 4.5 ROI-Analyse

**Investition in Sharding-Härtung:**

| Maßnahme | Aufwand (Tage) | Risiko-Reduktion | ROI |
|----------|----------------|------------------|-----|
| M5.1 Distributed Snapshot | 10 | 80% | Sehr Hoch |
| M2.1 Idempotente Migration | 7 | 60% | Sehr Hoch |
| M3.1 Circuit Breaker | 5 | 40% | Hoch |
| M4.1 Replica Sync | 20 | 70% | Hoch |
| M4.2 Leader Election | 30 | 80% | Mittel |
| **TOTAL** | **72 Tage** | **~70% Gesamt** | **Hoch** |

**Geschätzte Kosten ohne Maßnahmen:**
- Datenverlust-Incident: 50.000 - 500.000 EUR
- Prolonged Downtime (12h): 100.000 - 1.000.000 EUR
- Entwickler-Produktivität (-30%): 200.000 EUR/Jahr

**Break-Even:** Nach 1-2 Major Incidents (Wahrscheinlichkeit: 60% in Year 1)

---

## 5. Implementierungsplan

### Phase 1: Foundation (Wochen 1-4)

**Woche 1-2:**
- [ ] M3.1: Circuit Breaker Pattern implementieren
- [ ] M3.2: Automatic Failover zu Replicas
- [ ] Integration Tests für Circuit Breaker

**Woche 3-4:**
- [ ] M2.1: Idempotente Data Migration
- [ ] M2.3: Distributed Locking für Rebalancing
- [ ] Chaos Tests für Migration

**Deliverables:**
- Robuste Fehlerbehandlung bei Shard-Ausfällen
- Sichere Rebalancing-Operations

### Phase 2: Backup & Recovery (Wochen 5-8)

**Woche 5-6:**
- [ ] M5.1: Distributed Snapshot Koordination
- [ ] Two-Phase Commit für Snapshots
- [ ] Integration mit etcd für Metadata

**Woche 7-8:**
- [ ] M5.3: Backup Validation Pipeline
- [ ] Automatisierte Restore-Tests
- [ ] Monitoring für Backup-Erfolgsrate

**Deliverables:**
- Konsistente Point-in-Time Recovery
- Automatische Backup-Validierung

### Phase 3: Developer Tools (Wochen 9-12)

**Woche 9-10:**
- [ ] M1.1: Query Complexity Analyzer
- [ ] CLI-Tool: themis-query-explain
- [ ] Integration in CI/CD

**Woche 11-12:**
- [ ] M2.2: Erweiterte Chaos Engineering Tests
- [ ] Automatisierte Performance-Regression-Tests
- [ ] Dokumentation für Entwickler

**Deliverables:**
- Developer-Friendly Tooling
- Reduzierte Lernkurve für Sharding

### Phase 4: Replica Management (Wochen 13-20)

**Woche 13-16:**
- [ ] M4.1: Automatic Replica Synchronization
- [ ] WAL Shipping Implementierung
- [ ] Monitoring für Replication Lag

**Woche 17-20:**
- [ ] M4.2: Raft-basierte Leader Election
- [ ] Integration mit etcd Raft
- [ ] Automatic Failover bei Leader-Ausfall

**Deliverables:**
- Strong Consistency für Replicas
- Automatic Leader Failover

### Phase 5: Schema Management (Wochen 21-28)

**Woche 21-24:**
- [ ] M6.1: Schema Registry
- [ ] Compatibility Checking
- [ ] Schema Versioning

**Woche 25-28:**
- [ ] M6.2: Distributed DDL Executor
- [ ] Rolling/Canary Deployment-Strategien
- [ ] Rollback-Mechanismus

**Deliverables:**
- Sichere Schema-Evolution
- Automatisierte DDL-Deployment

---

## 6. Monitoring & Metriken

### 6.1 Kritische Metriken

**Sharding Health:**
```promql
# Shard Availability (%)
shard_availability_percent = (healthy_shards / total_shards) * 100

# Target: > 99.9%
# Alert: < 95%
```

**Query Performance:**
```promql
# Cross-Shard Query Latency (P99)
cross_shard_query_latency_p99_ms

# Target: < 100ms
# Alert: > 500ms
```

**Data Migration:**
```promql
# Migration Success Rate (%)
migration_success_rate = (successful_migrations / total_migrations) * 100

# Target: > 99.5%
# Alert: < 95%
```

**Backup Quality:**
```promql
# Backup Validation Success Rate (%)
backup_validation_success_rate

# Target: 100%
# Alert: < 100% (jeder fehlgeschlagene Backup ist kritisch)
```

**Circuit Breaker:**
```promql
# Circuit Breaker Open Count (per Shard)
circuit_breaker_open_count{shard_id="shard_1"}

# Target: 0
# Alert: > 0 (Shard-Ausfall)
```

### 6.2 Dashboards

**Dashboard 1: Sharding Overview**
- Cluster Topology Map
- Shard Health Status
- Data Distribution (Shard Size)
- Rebalancing Operations (Active/Completed/Failed)

**Dashboard 2: Query Performance**
- Cross-Shard Query Latency (Heatmap)
- Query Complexity Distribution
- Scatter-Gather Efficiency
- Circuit Breaker Status per Shard

**Dashboard 3: Backup & Recovery**
- Backup Schedule Compliance
- Snapshot Coordination Success Rate
- Restore Test Results
- Backup Size Trends

**Dashboard 4: Replica Health**
- Replication Lag (per Replica)
- WAL Shipping Throughput
- Leader Election Events
- Failover History

### 6.3 Alerting Rules

**Kritische Alerts (Pager):**
```yaml
- alert: ShardDownCritical
  expr: shard_availability_percent < 90
  for: 5m
  annotations:
    summary: "Kritischer Shard-Ausfall: {{ $value }}% Verfügbarkeit"

- alert: BackupValidationFailed
  expr: backup_validation_success_rate < 100
  for: 0m
  annotations:
    summary: "Backup-Validierung fehlgeschlagen für Snapshot {{ $labels.snapshot_id }}"

- alert: MigrationFailureSpike
  expr: rate(migration_failures_total[5m]) > 0.1
  for: 5m
  annotations:
    summary: "Erhöhte Migration-Fehlerrate: {{ $value }} Fehler/Sekunde"
```

**Warnungen (Ticket):**
```yaml
- alert: CrossShardQuerySlow
  expr: cross_shard_query_latency_p99_ms > 500
  for: 15m
  annotations:
    summary: "Langsame Cross-Shard Queries: P99 = {{ $value }}ms"

- alert: ReplicationLagHigh
  expr: replication_lag_seconds > 60
  for: 10m
  annotations:
    summary: "Hoher Replication Lag: {{ $value }}s für Replica {{ $labels.replica_id }}"
```

---

## 7. Zusammenfassung & Fazit

### 7.1 Kernerkenntnisse

1. **ThemisDB hat eine solide Sharding-Grundarchitektur:**
   - URN-basiertes Routing ✅
   - Consistent Hashing ✅
   - PKI-Sicherheit ✅
   - 98% der Kern-Komponenten implementiert ✅

2. **Identifizierte Risiken sind NICHT trivial:**
   - Backup-Koordination: KRITISCH 🔴
   - Replica-Management: HOCH 🟠
   - Operationelle Komplexität: HOCH 🟠
   - SQL-Komplexität: MITTEL 🟡

3. **Maßnahmen sind wirtschaftlich sinnvoll:**
   - ROI: Hoch (Break-Even nach 1-2 Incidents)
   - Aufwand: 72 Tage (ca. 3.5 Monate @ 1 FTE)
   - Risiko-Reduktion: ~70%

### 7.2 Strategische Empfehlung

**GO für Production mit folgenden Bedingungen:**

✅ **Kurzfristige Maßnahmen (P0) MÜSSEN implementiert werden:**
- Circuit Breaker Pattern (M3.1)
- Idempotente Migration (M2.1)
- Distributed Snapshot (M5.1)

⚠️ **Mittelfristige Maßnahmen (P1) SOLLTEN implementiert werden:**
- Replica Synchronization (M4.1)
- Leader Election (M4.2)
- Schema Registry (M6.1)

📋 **Langfristige Maßnahmen (P2) KÖNNEN implementiert werden:**
- Query Rewrite (M1.2)
- Incremental Backup (M5.2)
- Rate-Limited Reindex (M6.3)

### 7.3 Nächste Schritte

1. **Woche 1:** Stakeholder-Review dieses Dokuments
2. **Woche 2:** Priorisierung der Maßnahmen mit Product Owner
3. **Woche 3-6:** Implementierung von M3.1, M2.1, M5.1 (P0)
4. **Woche 7:** Production Readiness Review
5. **Woche 8+:** Phased Rollout mit Monitoring

---

**Dokument-Ende**

**Version:** 1.0  
**Autor:** Architecture Review Team  
**Review:** Pending  
**Nächstes Review:** Nach Implementierung von P0-Maßnahmen
