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
| [`include/temporal/README.md`](../../../include/temporal/README.md) | Öffentliche Header-API mit C++-Snippets |
| [`src/temporal/ROADMAP.md`](../../../src/temporal/ROADMAP.md) | Lieferstatus, Phasen und bekannte Einschränkungen |
| [`src/temporal/FUTURE_ENHANCEMENTS.md`](../../../src/temporal/FUTURE_ENHANCEMENTS.md) | Geplante Ausbauten und technische Ziele |

---

## Laufzeitverhalten, Fehlerfälle und Grenzen

- Historische Versionen bleiben bis zur Löschung/Archivierung durch Retention-Policies verfügbar.
- SQL/AQL-Temporal-DDL (`PERIOD FOR`, `FOR SYSTEM_TIME`) ist weiterhin nicht vollständig im Parser verfügbar; produktiv wird derzeit die C++-API verwendet.
- Typische Fehlerbilder: ungültige Zeitintervalle, unerwartete Konfliktauflösung (Policy/HLC), fehlende historische Daten nach aggressiver Retention.
- Ohne Retention- und Kompressionskonfiguration kann der Speicherbedarf historischer Daten stark wachsen.

---

## Installation

Das Modul wird automatisch mit ThemisDB gebaut. Für Linux:

```bash
cmake --preset linux-release
cmake --build --preset linux-release
```

---

## Usage

```cpp
#include "temporal/system_versioned_table.h"
#include "temporal/temporal_query_engine.h"

themisdb::temporal::SystemVersionedTable employees("employees");
employees.insert("emp1", {{"name", "Alice"}});

auto rows = themisdb::temporal::TemporalQueryEngine::queryAsOf(
    employees,
    std::chrono::system_clock::now()
);
```

Weitere Beispiele: [`include/temporal/README.md`](../../../include/temporal/README.md#examples)

---

## Troubleshooting

- **Keine Daten in historischen Abfragen:** Prüfen, ob der Zeitstempel im gültigen Bereich liegt und ob Retention alte Versionen bereits entfernt hat.
- **Konflikte verhalten sich unerwartet:** `ConflictPolicy` und HLC-Zeitquellen prüfen.
- **Speicherverbrauch steigt:** `RetentionManager` aktivieren und `TemporalCompressor` einsetzen.
