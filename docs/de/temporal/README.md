# Temporal-Modul

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: PRIMARY_SOURCES.md · ../../../src/temporal/README.md -->

**Stand:** 6. April 2026  
**Version:** aktuell  
**Kategorie:** Zeitliche Datenverwaltung  
**Status:** 🟢 Production-Ready

---

## Übersicht

Das Temporal-Modul implementiert bi-temporale Datenverwaltung mit System- und Gültigkeitszeitverfolgung, Snapshot-Management, Datenretention und zeitlichen Komprimierung für ThemisDB.

**Primäre Quelle:** [`src/temporal/`](../../../src/temporal/) · [`include/temporal/`](../../../include/temporal/)

---

## Kernkomponenten

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| BiTemporal | `bi_temporal.h` | `bi_temporal.cpp` | Bi-temporales Datenmodell (Systemzeit + Gültigkeitszeit) |
| BitemporalJoin | `bitemporal_join.h` | `bitemporal_join.cpp` | Bi-temporale Join-Operatoren |
| IntervalTreeIndex | `interval_tree_index.h` | `interval_tree_index.cpp` | Intervallbaum-Index für temporale Bereichsabfragen |
| RetentionManager | `retention_manager.h` | `retention_manager.cpp` | Datenretentionsrichtlinien und automatisches Löschen |
| SnapshotManager | `snapshot_manager.h` | `snapshot_manager.cpp` | Point-in-Time-Recovery und Snapshot-Verwaltung |
| SystemVersionedTable | `system_versioned_table.h` | `system_versioned_table.cpp` | SQL:2011-kompatible systemversionierte Tabellen |
| TemporalAggregator | `temporal_aggregator.h` | `temporal_aggregator.cpp` | Temporale Aggregationsfunktionen |
| TemporalCDC | `temporal_cdc.h` | `temporal_cdc.cpp` | Change-Data-Capture für temporale Tabellen |
| TemporalCompressor | `temporal_compressor.h` | `temporal_compressor.cpp` | Temporale Datenkomprimierung (Delta-Encoding) |
| TemporalConflictResolver | `temporal_conflict_resolver.h` | `temporal_conflict_resolver.cpp` | Konfliktauflösung für temporale Aktualisierungen |
| TemporalIndex | `temporal_index.h` | *(impl. in bi_temporal)* | Allgemeiner temporaler Index |
| TemporalQueryEngine | `temporal_query_engine.h` | *(impl. in query-Modul)* | Temporal-SQL-Abfrageausführung |

---

## Primäre Dokumentation

| Dokument | Beschreibung |
|----------|--------------|
| [`src/temporal/README.md`](../../../src/temporal/README.md) | Modulübersicht |
