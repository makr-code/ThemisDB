# Replication Module

**Stand:** 9. März 2026
**Version:** 1.6.0
**Kategorie:** Replication / High Availability
**Validated:** 2026-03-09 (64a0233)
**Status:** current

---

## Übersicht

Das Replication-Modul implementiert ThemisDBs High-Availability- und Daten­dauerhaftigkeits­infrastruktur
durch umfassende Replikations­strategien. Es bietet sowohl Leader-Follower-Replikation (Raft-ähnlicher
Konsensus für starke Konsistenz) als auch Multi-Master-Replikation (Geo-verteilte Deployments mit
Eventual Consistency). Das Modul gewährleistet Daten­redundanz, automatisches Failover und horizontale
Lese-Skalierbarkeit über verteilte Datenbank-Cluster.

**Primäre Dokumentation:** [`src/replication/README.md`](../../../src/replication/README.md)  
**Roadmap:** [`src/replication/ROADMAP.md`](../../../src/replication/ROADMAP.md)  
**Fehlende Implementierungen:** [`docs/de/replication/missing-implementations.md`](./missing-implementations.md)

---

## Source-Code Referenz

### Header-Dateien (`include/replication/`)

| Komponente | Header | Beschreibung |
|------------|--------|--------------|
| ReplicationManager | `replication_manager.h` | Haupt-Orchestrator für Leader-Follower-Replikation, Raft-ähnliche Leader-Wahl |
| WALManager | `replication_manager.h` | Write-Ahead-Log-Manager: Segmentierung, Checksummen, PITR |
| LeaderElection | `replication_manager.h` | Raft-ähnliches Leader-Wahl-Protokoll (termbasiert, Mehrheitswahl) |
| ReplicationStream | `replication_manager.h` | WAL-Streaming von Leader zu Followern (Batch, Backpressure) |
| CompressedReplicationStream | `replication_manager.h` | Zstd-komprimierter WAL-Stream für bandbreiten-effiziente Replikation |
| IReplicationListener | `replication_manager.h` | Listener-Interface: `onWALEntryApplied`, `onLeaderChanged`, `onReplicaFailed` |
| CDCManager | `replication_manager.h` | Change Data Capture: WAL-Ereignisse zu externen Systemen streamen |
| CrossClusterPublication | `replication_manager.h` | Cluster-übergreifende logische Replikation (Publish-Seite) |
| CrossClusterSubscription | `replication_manager.h` | Cluster-übergreifende logische Replikation (Subscribe-Seite) |
| WALArchivalManager | `replication_manager.h` | Lokale WAL-Archivierung (Zstd, Retention-Policy, Segment-Retrieval) |
| LagBasedReadRouter | `replication_manager.h` | Lag-basiertes automatisches Read-Traffic-Routing auf Replikas |
| ReplicationAnalytics | `replication_manager.h` | Lag-History, Engpass-Klassifikation, Replication-Insights |
| ReplicationBenchmark | `replication_manager.h` | Integrierter Replication-Throughput-Benchmark |
| QuorumReadManager | `replication_manager.h` | Quorum-Lesevorgänge für linearisierbare Reads |
| ParallelReplicationWorker | `replication_manager.h` | Parallele WAL-Replikation auf mehrere Follower |
| BatchedAckTracker | `replication_manager.h` | Gebündelte ACK-Verarbeitung für effiziente Semi-Sync-Schreibvorgänge |
| PersistentReplicationState | `replication_manager.h` | Persistierter Replikations-Zustand (Sequence, Term) |
| MultiRegionActiveActiveManager | `replication_manager.h` | Multi-Region-Active-Active mit Bounded-Staleness-Garantien (Beta) |
| MultiMasterReplicationManager | `multi_master_replication.h` | Multi-Master-Replikation: Write-Anywhere-Semantik |
| VectorClock | `multi_master_replication.h` | Vektor-Uhren für kausale Ordnung und Konflikt-Erkennung |
| HybridLogicalClock | `multi_master_replication.h` | HLC: physische Zeit + logische Zähler für konsistente Snapshots |
| ConflictResolver | `multi_master_replication.h` | Abstrakte Basisklasse für Konfliktauflösungs-Strategien |
| LastWriteWinsResolver | `multi_master_replication.h` | LWW-Strategie (HLC-Timestamps) |
| CRDTMergeResolver | `multi_master_replication.h` | CRDT-basiertes Merge (11 Typen: LWW_REGISTER, G_COUNTER, OR_SET, RGA, FLAG_EW, …) |
| CustomResolver | `multi_master_replication.h` | Anwendungs-spezifische Konfliktauflösung via Callback |

### Implementierungs-Dateien (`src/replication/`)

| Datei | Beschreibung |
|-------|--------------|
| `replication_manager.cpp` | Vollständige Implementierung aller Replikations-Klassen (~5 100 Zeilen) |

**Gesamt:** 2 Header-Dateien, 1 Implementierungsdatei in `src/replication/`

---

## Kern-Enumerationen

```cpp
enum class ReplicationMode  { SYNC, SEMI_SYNC, ASYNC };
enum class ReplicationRole  { LEADER, FOLLOWER, CANDIDATE, OBSERVER, WITNESS };
enum class ReadPreference   { PRIMARY, SECONDARY, PRIMARY_PREFERRED,
                              SECONDARY_PREFERRED, NEAREST };
enum class ConflictResolution { LAST_WRITE_WINS, CRDT, CUSTOM };
enum class ConsistencyLevel { STRONG, SESSION, BOUNDED_STALENESS, EVENTUAL };
```

---

## Test-Abdeckung

| Test-Datei | Beschreibt |
|------------|-----------|
| `tests/test_replication_ha.cpp` | Failover, Quorum-Schreibvorgänge, Witness-Nodes |
| `tests/test_wal_replication.cpp` | WAL-Append, -Replay, Segment-Rotation |
| `tests/test_wal_replication_integration.cpp` | End-to-End WAL-Replikation |
| `tests/test_replica_consistency.cpp` | Konsistenz-Checks zwischen Leader und Follower |
| `tests/test_cache_replication.cpp` | Cache-Invalidierung über Replikations-Events |
| `tests/test_replication_topology_api_handler.cpp` | Topologie-Visualisierer / API-Handler |

---

## Produktions-Reife

| Kategorie | Status |
|-----------|--------|
| Leader-Follower-Replikation | ✅ Stabil |
| WAL-Shipping & PITR | ✅ Stabil |
| Automatisches Failover | ✅ Stabil |
| Multi-Master + CRDT | ✅ Stabil |
| Zstd-komprimiertes WAL-Streaming | ✅ Stabil |
| CDC & Cross-Cluster Pub/Sub | ✅ Stabil |
| Witness-Nodes | ✅ Stabil |
| WAL-Archivierung (lokal) | ✅ Stabil |
| Multi-Region Active-Active | ⚠️ Beta |
| Schema-aware CDC (Avro/Protobuf) | ⚠️ Beta |
| `observability.h` / `event_stream.h` / `policy.h` / `conflict_resolution.h` / `replication_slot.h` | 🔬 Geplant (v1.7.0) |

---

## Bekannte Einschränkungen (Stand März 2026)

1. Raft-Implementierung ist Raft-ähnlich, kein Joint-Consensus (Mitgliedschafts-Änderungen in Planung).
2. WAL-Dateien auf Disk sind nicht komprimiert (nur WAL-Streaming verwendet Zstd).
3. WAL-Archivierung unterstützt lokale Verzeichnisse; Cloud-Archivierung (S3, GCS) ist nicht implementiert.
4. Kein automatisches Read-Your-Writes-Guarantee bei SECONDARY-Read-Präferenz.
5. Cascading-Replikation ist auf 2-stufige Hierarchien begrenzt.

---

*Geprüft von: Copilot-Agent (2026-03-09)*  
*Nächste Überprüfung: v1.7.0-Milestone*
