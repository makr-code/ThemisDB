[docs](../../index.md) > [de](../index.md) > [aql](./index.md) > [AQL_2_0_0_ROADMAP_INDEX](./AQL_2_0_0_ROADMAP_INDEX.md)  
**Datum:** 2026-07-17  
**Status:** current  
**Scope:** Navigations- und Referenzindex fuer die AQL-v2.0.0-Implementierungsroadmaps in den kanonischen Query-Quellen.  
**Primary (Quelle der Wahrheit):**
- `src/query/AQL_V2_0_0_COMPLETE_ROADMAP.md`
- `src/query/AQL_MUTATIONS_ROADMAP.md`
- `docs/de/aql/PRIMARY_SOURCES.md`

---

# AQL v2.0.0 Roadmap Index

## Zweck

Dieser Index verlinkt die kanonischen Implementierungsroadmaps fuer **AQL v2.0.0**. Die Datei selbst ist
kein Source of Truth fuer Feature-Status, sondern ein oeffentlicher Einstiegspunkt in die zugrunde
liegenden Roadmap-Dokumente des Query-Moduls.

## Aktueller Stand

- **AQL v1.3.x** bleibt die dokumentierte produktive Baseline in den Nutzerdokumenten unter `docs/de/aql/`.
- **AQL v2.0.0** wird ueber Roadmaps in `src/query/` geplant und umgesetzt.
- Mutations besitzen eine eigene Detail-Roadmap; DDL, Geospatial und Full-Text-Suche sind in der
  Gesamtroadmap als separate Workstreams beschrieben.

## Roadmap-Referenzen

| Thema | Quelle | Hinweis |
|------|--------|---------|
| Gesamtstandard AQL v2.0.0 | [`src/query/AQL_V2_0_0_COMPLETE_ROADMAP.md`](../../../src/query/AQL_V2_0_0_COMPLETE_ROADMAP.md) | Gesamtbild fuer Mutations, DDL, Geospatial, Full-Text, Integration und Testing |
| Mutations (INSERT/UPDATE/REPLACE/REMOVE/UPSERT) | [`src/query/AQL_MUTATIONS_ROADMAP.md`](../../../src/query/AQL_MUTATIONS_ROADMAP.md) | Detailplanung fuer Parser, Validator, Execution, Transactions, Tests und Doku |
| DDL (CREATE/DROP COLLECTION/INDEX/VIEW) | [`AQL_V2_0_0_COMPLETE_ROADMAP.md#2-ddl-createdrop-collectionindexview`](../../../src/query/AQL_V2_0_0_COMPLETE_ROADMAP.md#2-ddl-createdrop-collectionindexview) | Roadmap-Abschnitt fuer Katalog-, Executor- und Validierungsarbeit |
| Geospatial (ST_Distance/ST_Contains/ST_Within/ST_DWithin) | [`AQL_V2_0_0_COMPLETE_ROADMAP.md#3-geospatial-st_distance-st_contains-st_within-st_dwithin`](../../../src/query/AQL_V2_0_0_COMPLETE_ROADMAP.md#3-geospatial-st_distance-st_contains-st_within-st_dwithin) | v2.0.0-Plan fuer Parser-Integration und Executor-Pfade |
| Full-Text Search (SEARCH + FTS Operators) | [`AQL_V2_0_0_COMPLETE_ROADMAP.md#4-full-text-search-search--fts-operators`](../../../src/query/AQL_V2_0_0_COMPLETE_ROADMAP.md#4-full-text-search-search--fts-operators) | Roadmap-Abschnitt fuer Syntax, Optimizer und Suchausfuehrung |

## Validierung / Herkunft

- Die kanonischen AQL-Quellen fuer Status- und Planungsangaben sind in [`PRIMARY_SOURCES.md`](./PRIMARY_SOURCES.md) dokumentiert.
- Dieser Index wurde aus den aktuellen Roadmap-Dateien im Query-Modul abgeleitet und darf nur synchron mit
  deren Inhalt aktualisiert werden.

## Folgepunkte

- Bei neuen AQL-v2.0.0-Workstreams diesen Index um den entsprechenden kanonischen Roadmap-Verweis erweitern.
- Wenn ein Workstream von Planung auf produktive Dokumentation uebergeht, die Nutzerdokumente unter
  `docs/de/aql/` separat aktualisieren.
