# Replikations-Modul

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: PRIMARY_SOURCES.md · ../../../src/replication/README.md -->

**Stand:** 6. April 2026  
**Version:** aktuell  
**Kategorie:** Hochverfügbarkeit / Replikation  
**Status:** 🟢 Production-Ready

---

## Übersicht

Das Replikations-Modul stellt ThemisDBs Hochverfügbarkeits- und Datenhaltbarkeitsinfrastruktur bereit. Es implementiert Leader-Follower-Replikation mit Raft-ähnlichem Konsensus, Multi-Master-Replikation und WAL-Shipping.

**Primäre Quelle:** [`src/replication/`](../../../src/replication/) · [`include/replication/`](../../../include/replication/)

---

## Kernkomponenten

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| ReplicationManager | `replication_manager.h` | `replication_manager.cpp` | Haupt-Replikationsfassade |
| RaftV2 | `raft_v2.h` | `raft_v2.cpp` | Raft-Konsensusprotokoll v2 (Leader-Wahl, Log-Replikation) |
| MultiMasterReplication | `multi_master_replication.h` | *(impl. in replication_manager)* | Multi-Master-Replikation für geo-verteilte Deployments |
| MultiTierReplication | `multi_tier_replication.h` | `multi_tier_replication.cpp` | Mehrstufige Replikations-Topologien |
| LogicalReplication | `logical_replication.h` | `logical_replication.cpp` | Schema-bewusste logische Replikation mit Slots |
| ReplicationSlot | `replication_slot.h` | `replication_slot.cpp` | Replikations-Slot-Verwaltung |
| SchemaCDC | `schema_cdc.h` | `schema_cdc.cpp` | Schema-Change-Data-Capture |
| ConflictResolution | `conflict_resolution.h` | `conflict_resolution.cpp` | LWW, CRDT-basierte Konfliktauflösung |
| CRDTTypes | `crdt_types.h` | *(header-only)* | CRDT-Datentypen (G-Set, OR-Set, LWW-Register) |
| EventStream | `event_stream.h` | `event_stream.cpp` | Change-Data-Capture Event-Stream |
| Policy | `policy.h` | `policy.cpp` | Replikationsrichtlinien (Selektiv, Filter-basiert) |

---

## Primäre Dokumentation

| Dokument | Beschreibung |
|----------|--------------|
| [`src/replication/README.md`](../../../src/replication/README.md) | Modulübersicht und Scope |
