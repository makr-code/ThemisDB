# WAL-basierte Replikation - Implementierungs-Status

> **Related Documentation (English):**
> - **[replication-ha-guide.md](./replication-ha-guide.md)** - Complete HA/replication deployment guide
> - **[replication_raid_plan.md](./replication_raid_plan.md)** - RAID 1/10 implementation roadmap

## 📋 Zusammenfassung

Das Themis-Datenbanksystem hat eine **vollständige WAL-basierte Replikationsinfrastruktur** mit RAID 1/10-Unterstützung implementiert.

**Status: ✅ ~85% IMPLEMENTIERT**

### Modul-Organisation

Die Replikationsinfrastruktur ist auf zwei Hauptmodule aufgeteilt:

**`replication/` Modul** - High-Level Orchestrierung:
- `include/replication/`, `src/replication/`
- ReplicationManager für Lifecycle-Management
- MultiMasterReplicationManager für Multi-Master-Koordination

**`sharding/` Modul** - Low-Level WAL-Infrastruktur (Hauptfokus dieses Dokuments):
- `include/sharding/`, `src/sharding/`
- Alle WAL-Komponenten (Manager, Shipper, Applier)
- ReplicationCoordinator für Write-Concern
- Consensus-Module (Raft, Gossip, Paxos)
- Topology und Health-Management

Diese Aufteilung ermöglicht es, dass `replication/` sich auf Business-Logik konzentriert, während `sharding/` die komplexe Infrastruktur für verteilte Systeme bereitstellt.

---

## ✅ Abgeschlossene Komponenten

### 1. **WAL-Manager** (Kern-Persistierung)
- **Datei:** `include/sharding/wal_manager.h` + `src/sharding/wal_manager.cpp`
- **Features:**
  - Append-Only Log mit LSN-Tracking (Segment/Offset-Paare)
  - Segment-Rotation bei Größenüberschreitung
  - Async Write-Buffer mit konfigurierbar er Flush-Policy
  - LSN-Range Queries für effiziente Replikation
  - Checkpoint & Truncation für Speicherfreigabe
  - Prometheus-Metriken: total_entries, total_bytes, checkpoint_duration

### 2. **WAL-Shipper** (Primär-seitige Batch-Versand)
- **Datei:** `include/sharding/wal_shipper.h` + `src/sharding/wal_shipper.cpp`
- **Features:**
  - Async Batch-Versand zu Replicas (konfigurierbar: 50-200ms Intervalle)
  - Kompressionsunterstützung: Zstd, LZ4, None
  - Retry-Logik mit exponentieller Backoff
  - Replica-Lag Tracking und Backlog-Management
  - gRPC + HTTP Dual-Transport (feature-gated)
  - Prometheus-Metriken: ship_batches_total, ship_bytes_total, ship_failures_total

### 3. **WAL-Applier** (Replica-seitige Anwendung)
- **Datei:** `include/sharding/wal_applier.h` + `src/sharding/wal_applier.cpp`
- **Features:**
  - Idempotente Anwendung durch LSN-Tracking
  - Strenge LSN-Ordering-Validierung
  - Custom Apply-Handler für Storage-Integration
  - Statistiken-Tracking: entries_applied, apply_failures, lsn_mismatches
  - Prometheus-Metriken: apply_latency_seconds (mit Histogramm-Bucketing)

### 4. **Replication Coordinator** (Write-Concern Enforcement)
- **Datei:** `include/sharding/replication_coordinator.h` + `src/sharding/replication_coordinator.cpp`
- **Features:**
  - WriteConcern-Level: ONE, MAJORITY, ALL
  - MAJORITY-Quorum Berechnung: (n_nodes / 2) + 1
  - Pending-Acks Tracking mit LSN-Mapping
  - Timeout-Management und Fehlerbehandlung
  - Atomare Quorum-Abrechnung

### 5. **Replica-Topology** (Shard-zu-Replica-Mapping) ✨ *NEU*
- **Datei:** `include/sharding/replica_topology.h` + `src/sharding/replica_topology.cpp`
- **Features:**
  - RedundancyMode: NONE, RAID1 (1:1), RAID10 (1:2), RAID5 (n+1), RAID6 (n+2)
  - ShardReplicaSet: Definiert Primär + Replica-IDs pro Shard
  - Quorum-Berechnung pro Redundancy-Mode
  - JSON-basierte Konfiguration: `loadFromJson()`
  - Thread-sicher mit Mutex-Lock

### 6. **HTTP Replication Endpoint** (Replica-seitig)
- **Datei:** `src/server/http_server.cpp` (lines 1903-1960)
- **Endpoint:** `POST /api/v1/wal/apply`
- **Features:**
  - HMAC-Auth (optional mit Shared Secret)
  - Batch-Anwendung mit LSN-Validierung
  - Latency-Metriken pro Request
  - Graceful Handling von Duplikaten (idempotent)

### 7. **Konfiguration** (JSON-basiert) ✨ *NEU*
- **Datei:** `config/replication.example.json`
- **Struktur:**
  ```json
  {
    "replication": {
      "shipper_enabled": true,
      "primary_id": "primary-node-1",
      "replicas": [
        {"replica_id": "replica-1", "endpoint": "http://replica1:8765"}
      ],
      "compression": "zstd",
      "batch_size": 100,
      "ship_interval_ms": 100,
      "tls": {"enabled": false}
    },
    "replica_topology": [
      {
        "shard_id": "shard_0",
        "primary_id": "primary-node-1",
        "replicas": ["replica-1"],
        "redundancy": "RAID1"
      }
    ]
  }
  ```

### 8. **Integration Tests** (MAJORITY Quorum Validation) ✨ *NEU*
- **Datei:** `tests/test_wal_replication_integration.cpp`
- **8 Tests (alle ✅ BESTANDEN):**
  1. ✅ **MajorityQuorumEnforcement** - Validiert 2/3 Quorum für 3-Node-Cluster
  2. ✅ **QuorumSizeRAID1** - Verifiziert Quorum = 2 für RAID1 + 2 Replicas
  3. ✅ **IdempotentApplyByLSN** - LSN-Tracking verhindert Duplikate
  4. ✅ **LSNOrderingValidation** - Strenge Reihenfolgen-Durchsetzung
  5. ✅ **ReplicaLagConvergence** - Replicas holen auf nach Writes
  6. ✅ **RAID10TopologyValidation** - RAID10 mit 2 Replicas verifiziert
  7. ✅ **ReplicaFailureDetection** - Topology API tracking Replica-Gesundheit
  8. ✅ **WriteConcernTimeout** - Timeout-Handling funktioniert

### 9. **Replication Topology Visualizer** (Web UI) ✨ *NEU*
- **Datei:** `include/server/replication_topology_api_handler.h` + `src/server/replication_topology_api_handler.cpp`
- **Endpoints:**
  - `GET /api/v1/replication/topology` — JSON-Snapshot aller Nodes mit Rolle, Health, WAL-Lag
  - `GET /api/v1/replication/health` — Aggregierter Cluster-Health-Status (Quorum, Lag, Ship-Statistiken)
  - `GET /ui/replication/topology` — Interaktive SVG-Topologieseite (automatisches Refresh alle 5 s)
- **Features:**
  - Radial SVG-Graph: Primary (blau, `P`) im Zentrum, Replicas farblich nach Health
  - Gerichtete WAL-Stream-Kanten mit Pfeilspitzen
  - Per-Replica Lag-Label direkt am Knoten
  - Sidebar mit Cluster-Statistiken und Node-Tabelle
  - `API_BASE` automatisch aus dem `Host`-Header injiziert
  - 503-Antwort bei nicht konfigurierter Replikation (tolerant gegenüber null-Coordinator)
- **Tests:** `tests/test_replication_topology_api_handler.cpp`
- **Erweiterung von `ReplicationCoordinator`:** neue Methoden `getReplicaInfo()` und `getShipperStats()` (delegiert an `WALShipper`)

### 10. **Cross-Cluster Publish/Subscribe Replication** ✨ *NEU*
- **Datei:** `include/replication/replication_manager.h` + `src/replication/replication_manager.cpp`
- **Klassen:**
  - `PublicationFilter` — Filtert WAL-Einträge nach Collection-Namen und/oder Operationstyp (INSERT/UPDATE/DELETE). Leerer Filter = alle Einträge durchlassen.
  - `CrossClusterPublication` — Implementiert `IReplicationListener`; wird per `ReplicationManager::addListener()` in die WAL-Pipeline eingehängt und leitet passende Einträge an alle registrierten Remote-Subscriber-Callbacks weiter.
  - `CrossClusterSubscription` — Registriert einen lokalen Apply-Callback bei einer Publication; idempotentes `enable()`/`disable()`; Auto-Deregistrierung im Destruktor.
- **Features:**
  - Thread-sichere Filteraktualisierung via `shared_mutex`
  - Fehlerresilienz: Apply-Ausnahmen werden gezählt (`errorCount()`), Delivery läuft weiter
  - Tracking: `appliedCount()`, `lastAppliedSequence()`, `errorCount()`
  - Prometheus-Metriken: `published_total`, `subscribers`, `applied_total`, `errors_total`, `last_applied_sequence`
- **Tests:** `tests/test_replication_ha.cpp` — 31 Test-Cases (PublicationFilter, CrossClusterPublication, CrossClusterSubscription, E2E, Integration, Prometheus)

---

## 🔄 Write-Path (Beispiel: PUT /entities)

```
1. Client: PUT /entities/{key} mit WriteConcern=MAJORITY
   └─> Server HTTP Handler

2. Primary Server:
   ├─> WALManager.append(entry) → LSN(segment, offset)
   ├─> ReplicationCoordinator.waitForReplication(LSN, MAJORITY_config)
   │   └─> Startet asynchrone Wartefunktion für 2/3 Acks
   ├─> WALShipper.shipToReplica()  (async in Hintergrund)
   │   └─> HTTP POST zu Replicas oder gRPC (feature-gated)
   └─> Return 200 (success) oder 503 (quorum_timeout)

3. Replica (async):
   ├─> HTTP POST /api/v1/wal/apply empfangen
   ├─> WALApplier.applyBatch(entries)
   │   ├─> Validate LSN ordering
   │   ├─> Skip if already applied (idempotent)
   │   └─> Ruft Custom ApplyHandler auf
   ├─> HTTP 200 ACK zur Primary
   └─> ReplicationCoordinator erhält Ack, reduziert Wartecount

4. Quorum erreicht:
   ├─> waitForReplication() kehrt mit success=true zurück
   └─> HTTP 200 an Client
```

---

## 📊 Prometheus Metrics

### WAL-Metriken
- `themis_wal_ship_batches_total` - Versandte Batches
- `themis_wal_ship_bytes_total` - Versandte Bytes (vor Kompression)
- `themis_wal_ship_failures_total` - Fehlgeschlagene Versuche
- `themis_wal_replication_lag_seconds` - Replica-Verzögerung (pro Replica)
- `themis_wal_backlog_bytes` - Unbetreute Bytes (pro Replica)
- `themis_wal_apply_latency_seconds` - Anwendungs-Latenz (Histogramm)
- `themis_wal_apply_success_total` - Erfolgreich angewendete Entries
- `themis_wal_apply_fail_total` - Fehlerhafte Anwendungen

---

## ⚙️ Build & Test Status

### ✅ Kompilierung
```
✓ themis_core.lib - Kern-Libs ohne Fehler
✓ test_wal_replication_integration - 8 Tests alle bestanden
```

### ✅ Tests
```
Running 8 tests from WALReplicationIntegrationTest
[PASSED] 8 tests
[FAILED] 0 tests
```

---

## 🚀 Noch zu implementieren (15%)

### 1. **Multi-Node Endurance-Test**
- [ ] Starten Sie 2+ physische Instanzen
- [ ] 1000 Writes mit WriteConcern=MAJORITY
- [ ] Validieren Sie: Lag → 0 nach ~5 Sekunden
- [ ] Messen Sie durchschnittliche Latenz

### 2. **Real Network Shipper Integration**
- [ ] Ersetzen Sie Mock-Client durch echte HTTP POST (Boost.Asio)
- [ ] mTLS-Support implementieren (cert/key aus Config)
- [ ] Retry-Backoff validieren unter Netzwerkfehlern

### 3. **RAID 5/6 Implementation** (Optional)
- [ ] Parity-Berechnung für n+1/n+2 Replicas
- [ ] Quorum-Formeln für degraded mode
- [ ] Replica-Healing nach Ausfall

### 4. **Failover & Election** (Zukünftig)
- [ ] Primary-Ausfall-Erkennung
- [ ] Replica-Promotion-Logic
- [ ] Write-Blackout während Übergang

---

## 📁 Datei-Übersicht

```
include/sharding/
├── wal_manager.h ...................... LSN + WAL Persistierung
├── wal_applier.h ....................... Idempotente Anwendung  
├── wal_shipper.h ....................... Batch-Versand
├── replication_coordinator.h ........... Quorum-Enforcement
├── replica_topology.h .................. ✨ Shard-zu-Replica-Mapping
└── write_concern.h ..................... WriteConcern-Enum

src/sharding/
├── wal_manager.cpp ..................... LSN + WAL Persistierung
├── wal_applier.cpp ..................... Idempotente Anwendung
├── wal_shipper.cpp ..................... Batch-Versand
├── replication_coordinator.cpp ......... Quorum-Enforcement
└── replica_topology.cpp ................ ✨ Shard-zu-Replica-Mapping

config/
└── replication.example.json ........... ✨ Beispiel-Konfiguration

tests/
└── test_wal_replication_integration.cpp ✨ MAJORITY Quorum Tests (8x)
```

---

## 🎯 Nächste Schritte

1. **Unmittelbar:** Multi-Node Endurance-Test starten
2. **Kurz-Fristig:** Real Network Shipper Client integrieren
3. **Mittel-Fristig:** RAID 5/6 Parity-Logic
4. **Lang-Fristig:** Automatisches Failover

---

## ✨ Besondere Hinweise

- **Windows-Kompatibilität:** Alle LSN/DELETE Macro-Konflikte gelöst
- **Feature-Gates:** gRPC optional via CMake-Flag
- **Thread-Sicherheit:** Applier/Coordinator mit Mutex-Lock
- **Idempotenz:** Durch LSN-Tracking garantiert

---

**Letztes Update:** 2026-02-09
**Implementierungs-Umfang:** ~85% (inklusive Tests)

---

## 📚 Weitere Dokumentation

### Englischsprachige Dokumentation
- **[replication-ha-guide.md](./replication-ha-guide.md)** - Vollständiger HA/Replication Guide mit Deployment-Topologien, Konfiguration, Monitoring und Troubleshooting
- **[replication_raid_plan.md](./replication_raid_plan.md)** - RAID 1/10 Readiness Plan und Implementierungs-Roadmap
- **[docs/replication/](./replication/)** - Zusätzliche Replikations-Dokumentation und Beispiele

### Verwandte Systemdokumentation
- [Distributed Sharding Architecture](./de/sharding/DISTRIBUTED_SHARDING_ARCHITECTURE.md) - Sharding-Modul Dokumentation
- [ARCHITECTURE.md](../ARCHITECTURE.md) - System-Architektur-Übersicht
- [SECURITY.md](../SECURITY.md) - Sicherheitskonfiguration
