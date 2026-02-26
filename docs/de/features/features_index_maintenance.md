---
category: "🔄 Data Operations"
version: "v1.3.0"
status: "✅"
date: "22.12.2025"
---

# 🔧 Index Maintenance

Index-Defragmentation, Rebuild und Wartung.

## 📋 Inhaltsverzeichnis

- [📋 Übersicht](#-übersicht)
- [✨ Features](#-features)
- [🚀 Schnellstart](#-schnellstart)
- [📖 Detaillierte Dokumentation](#-detaillierte-dokumentation)
- [💡 Best Practices](#-best-practices)
- [🔧 Troubleshooting](#-troubleshooting)
- [📚 Siehe auch](#-siehe-auch)
- [📝 Changelog](#-changelog)

## 📋 Übersicht

---


Dieses Dokument erklärt die Statistik- und Wartungsfunktionen für Indizes.

## IndexStats

```cpp
struct IndexStats {
    std::string type;                 // "regular", "composite", "range", "sparse", "geo", "ttl", "fulltext"
    std::string table;                // Tablename
    std::string column;               // Spaltenname bzw. "col1+col2" für Composite
    size_t entry_count = 0;           // Anzahl Index-Einträge
    size_t estimated_size_bytes = 0;  // grobe Schätzung
    bool unique = false;              // Unique-Constraint
    std::string additional_info;      // Typ-spezifisch (z. B. Composite-Spaltenliste, "sorted", "inverted_index", TTL)
};
```

- Composite: `additional_info` enthält die Spaltenliste (z. B. `"customer_id, status"`).
- Range: `additional_info = "sorted"`
- Fulltext: `additional_info = "inverted_index"`
- TTL: `additional_info = "ttl_seconds=<N>"`

Abruf der Statistiken:

```cpp
SecondaryIndexManager idx(db);

auto s = idx.getIndexStats("users", "email");
auto all = idx.getAllIndexStats("users");
```

Zusätzlich stehen Metriken und erweiterte Rebuild-Funktionen zur Verfügung (siehe unten).

## Rebuild eines Index

Rebuild löscht alle bestehenden Indexeinträge eines Indexes und baut sie aus den Entities neu auf.

```cpp
idx.rebuildIndex("users", "email");        // Single-Column
idx.rebuildIndex("orders", "customer_id+status"); // Composite
```

Wann sinnvoll?
- Nach manuellen Eingriffen (inkonsistente Index-Keys)
- Nach Bugfixes im Index-Aufbau

Implementierungsdetails:
- Entities werden unter Prefix `"<table>:"` gescannt (gemäß `KeySchema::makeRelationalKey`).
- Pro Entity wird der passende Index-Key neu erzeugt und gespeichert.

## Online-Rebuild mit minimalem Lese-Impact

`rebuildIndexOnline` baut den Index in einem Shadow-Prefix auf, während der Live-Index für Reads
verfügbar bleibt. Am Ende wird in einem einzigen `WriteBatch` atomar auf den Live-Prefix umgeschaltet –
Leseanfragen sehen zu keinem Zeitpunkt einen leeren Index.

```cpp
// Einfachster Aufruf – kein Throttling, kein Callback
idx.rebuildIndexOnline("users", "email");

// Gedrosselter Rebuild: 500 µs Pause pro 100 Entities + Fortschritts-Callback
idx.rebuildIndexOnline("orders", "price", 500, [](size_t done, size_t total) {
    if (done % 1000 == 0) { /* log */ }
    return true; // false → Rebuild abbrechen
});
```

Parameter:

| Parameter | Typ | Standard | Bedeutung |
|-----------|-----|---------|-----------|
| `table` | `string` | – | Tabellenname |
| `column` | `string` | – | Spaltenname (ggf. `"col1+col2"` für Composite) |
| `throttle_us` | `uint32_t` | `0` | Mikrosekunden Pause alle 100 Entities (0 = kein Throttling) |
| `progress` | `function<bool(size_t,size_t)>` | `nullptr` | Callback `(done, total) → weiter?` |

Wann sinnvoll?
- Im laufenden Betrieb, wenn Leseanfragen nicht unterbrochen werden dürfen
- Bei großen Tabellen mit hohem Schreibdurchsatz
- Als Alternative zu `rebuildIndex` in produktionskritischen Szenarien

Implementierungsdetails:
- Stale Shadow-Entries eines abgebrochenen vorherigen Online-Rebuilds werden beim Start bereinigt.
- Der Shadow-Prefix ist `__rb__:<livePrefix>` und nimmt an keiner regulären Index-Suche teil.
- HTTP-Endpunkt: `POST /index/rebuild` mit `"online": true` (optional `"throttle_us": <N>`).

## Reindex der gesamten Tabelle

```cpp
idx.reindexTable("users");
```

- Sucht alle Meta-Keys (`idxmeta:`, `ridxmeta:`, `sidxmeta:`, `gidxmeta:`, `ttlidxmeta:`, `ftidxmeta:`) für die Tabelle und führt `rebuildIndex` je Spalte aus.

## TTL-Cleanup

```cpp
auto [st, removed] = idx.cleanupExpiredEntities("sessions", "last_seen");
```

- Löscht abgelaufene Entities und zugehörige Indexeinträge atomar.
- Muss regelmäßig aufgerufen werden (Timer/Cron).

## Rebuild-Metriken & Query-Metriken

Header: `index/secondary_index.h`

```cpp
auto& rbm = idx.getRebuildMetrics();
auto& qm  = idx.getQueryMetrics();

// Counter auslesen (z. B. für Prometheus Exporter)
uint64_t rebuilds        = rbm.rebuild_count.load();
uint64_t durationMs      = rbm.rebuild_duration_ms.load();
uint64_t processed       = rbm.rebuild_entities_processed.load();
uint64_t onlineRebuilds  = rbm.online_rebuild_count.load();  // Online-Rebuilds seit Start

uint64_t cursorAnchors = qm.cursor_anchor_hits_total.load();
uint64_t rangeSteps    = qm.range_scan_steps_total.load();
```

Empfohlene Prometheus-Namen:

- `themis_index_rebuild_count`
- `themis_index_rebuild_duration_ms_total`
- `themis_index_rebuild_entities_processed_total`
- `themis_index_online_rebuild_count`
- `themis_index_cursor_anchor_hits_total`
- `themis_index_range_scan_steps_total`

Diese Zähler können im HTTP-/Metrics-Endpunkt exponiert werden.

## Rebuild mit Progress-Callback

Für lange Rebuilds kann ein Fortschritts-Callback registriert werden, der den Fortschritt meldet und Abbruch erlaubt:

```cpp
size_t calls = 0;
idx.rebuildIndex("users", "email", [&](size_t done, size_t total){
  ++calls;
  // Logging oder UI-Update
  return true; // false → Rebuild abbrechen
});
```

Dasselbe gilt für `rebuildIndexOnline`:

```cpp
idx.rebuildIndexOnline("users", "email", 0, [&](size_t done, size_t total){
  return done < 50000; // Abbruch nach 50 000 Entities
});
```

Siehe auch Testfälle in `tests/test_index_stats.cpp` (Callback wird aufgerufen und kann abbrechen).

## Performance-Hinweise

- `estimated_size_bytes` ist eine einfache, konservative Schätzung (Entry-Anzahl × Durchschnittsgröße). Für exakte Größen Messungen auf Key-Ranges durchführen.
- `rebuildIndex` und `reindexTable` sind IO-intensiv. Für große Tabellen in Wartungsfenstern planen oder `rebuildIndexOnline` mit `throttle_us` nutzen.
- `rebuildIndexOnline` eignet sich für den laufenden Betrieb – der Live-Index bleibt während des Scans vollständig lesbar.
- Range-Scans mit `scanKeysRangeAnchored` minimieren Duplikate bei Pagination (Server-Side Cursor).
- Geo- und Fulltext-Indizes können separat reindiziert werden, falls der Rebuild teuer ist.
