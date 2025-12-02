# Replication-Dokumentation

**Source Code:** `src/replication/`, `include/replication/`

Diese Dokumentation beschreibt die Replikations-Komponenten von ThemisDB.

## Übersicht

ThemisDB unterstützt verschiedene Replikationsstrategien:
- Leader-Follower Replication (WAL-basiert, Auto-Failover)
- Multi-Master Replication (CRDTs, Vector Clocks, HLC)
- Geo-Distributed Replication

## Verwandte Dokumentation

- [Sharding: RAID Redundancy Architecture](../sharding/RAID_REDUNDANCY_ARCHITECTURE.md)
- [Sharding: Streaming Architecture](../sharding/STREAMING_ARCHITECTURE.md)
