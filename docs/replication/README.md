# Replication-Dokumentation

**Stand:** 5. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** Replication

---


**Source Code:** `src/replication/`, `include/replication/`

Diese Dokumentation beschreibt die Replikations-Komponenten von ThemisDB.

## Übersicht

ThemisDB unterstützt verschiedene Replikationsstrategien:
- Leader-Follower Replication (WAL-basiert, Auto-Failover)
- Multi-Master Replication (CRDTs, Vector Clocks, HLC)
- Geo-Distributed Replication

## Dokumentation in diesem Ordner

| Datei | Beschreibung | Status |
|-------|--------------|--------|
| [replication_overview.md](./replication_overview.md) | Konzept und Architektur | 📋 TODO |
| [replication_implementation.md](./replication_implementation.md) | Implementierungsdetails | 📋 TODO |
| [replication_config.md](./replication_config.md) | Konfigurationsoptionen | 📋 TODO |
| [replication_security.md](./replication_security.md) | mTLS, Verschlüsselung | 📋 TODO |
| [replication_performance.md](./replication_performance.md) | Benchmarks & Latenz | 📋 TODO |

## Verwandte Dokumentation

- [Sharding: RAID Redundancy Architecture](../sharding/sharding_redundancy.md)
- [Sharding: Streaming Architecture](../sharding/sharding_streaming.md)
