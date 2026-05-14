# DYNAMIC_SCHEMA_RECONFIGURATION_RESEARCH

**Projekt:** ThemisDB
**Kategorie:** Research Documentation
**Status:** Review-ready (überarbeitet)
**Datum:** 2026-05-14
**Version:** 1.1

---

## Abstract / Zusammenfassung

Dieses Dokument bewertet den Stand der dynamischen Schema-Rekonfiguration in ThemisDB auf Basis von **verifizierbaren Artefakten** (Code, Tests, Architektur-Dokumente) statt auf hypothetischen Implementierungen. Ergebnis:

- ThemisDB besitzt bereits produktive Bausteine für Schema-Evolution:
  - Versionierung, Diff, Dry-Run-Validierung und Migrationsskript-Generierung (`SchemaVersionManager`)
  - Online-Schema-Migration über gestufte DDL-Operationen (`storage::SchemaMigrator`)
  - Additive In-Place-Migration ohne Datenkopie (`updates::InPlaceSchemaMigrator`)
  - HTTP-Endpunkte für Schema- und Versionsabfragen (`SchemaApiHandler`)
- Eine deklarative YAML/JSON-Runtime-Pipeline für vollautomatische Re-Konfiguration ist in der Open-Source-Codebasis **nicht als durchgängiger End-to-End-Flow** nachweisbar.
- Für Review-Zwecke wird daher ein evidenzbasierter Zielpfad empfohlen: bestehende Migrations- und Versionierungs-APIs orchestrieren, statt neue, unbelegte Architektur-Claims zu formulieren.

---

## Introduction / Einleitung

### Problemstellung

Schema-Änderungen in produktiven Multi-Model-Datenbanken müssen ohne ungeplante Downtime, mit nachvollziehbarer Historie und sicherem Rollback erfolgen. Gleichzeitig sollen Query- und API-Schichten konsistent bleiben.

### Ziel dieses Reviews

1. Fachliche Claims des bisherigen Dokuments gegen den aktuellen ThemisDB-Stand prüfen.
2. Terminologie konsistent zum Repository setzen (AQL, Multi-Model, MVCC/Snapshot-Isolation, Komponenten-Namen).
3. Einen belastbaren, review-fähigen Stand mit klaren Grenzen liefern.

### Terminologie (vereinheitlicht)

- **AQL** = *Advanced Query Language* (ThemisDB Query-Modul, Parser/Translator/Optimizer).
- **Multi-Model** = relational + graph + vector + document (u. a. laut `README.md`, `ARCHITECTURE.md`).
- **Konsistenzmodell (lokal dokumentiert):** ACID mit MVCC/Snapshot-Isolation (Architektur-/README-Aussagen).
- **Schema-Rekonfiguration** = kontrollierte Schema-Evolution über Versionierung + Migration, nicht automatisch „self-adaptive YAML magic“.

---

## Methodik / Ansatz

### 1) Artefaktbasierter Faktencheck

Verwendete primäre Evidenz:

- Implementierungsdateien (`src/...`, `include/...`)
- Fokus-Tests (`tests/test_schema_*.cpp`, `tests/test_online_schema_migration.cpp`, `tests/test_in_place_schema_migrator.cpp`)
- Modul-/Architektur-Dokumentation (`README.md`, `ARCHITECTURE.md`, `src/metadata/ROADMAP.md`, `src/query/README.md`)

### 2) Claim-Klassifizierung

Jeder relevante Claim wurde einer Kategorie zugeordnet:

- **Bestätigt:** direkt durch Code + Test belegbar.
- **Teilweise bestätigt:** Bausteine vorhanden, aber kein vollständiger End-to-End-Flow im OSS-Baum.
- **Nicht bestätigt:** im bisherigen Dokument behauptet, aber ohne belastbare Artefakte.

### 3) Redaktionsprinzip

- Entfernen unbelegter Industrie-/Leistungsbehauptungen ohne Quellenbezug zum aktuellen Stand.
- Ersetzen durch präzise Aussagen mit Dateireferenzen.
- Pflichtstruktur für Review ergänzt.

---

## Evaluation / Experimente

## A) Verifizierte Implementierungsbefunde (Problem → Ansatz → Evidenz)

| Problem | Ansatz in ThemisDB | Evidenz |
|---|---|---|
| Nachvollziehbare Schema-Historie | Versionierte Schema-Snapshots und Diff | `include/metadata/schema_version_manager.h`, `src/metadata/schema_version_manager.cpp` |
| Vorabprüfung ohne Persistenz | Dry-Run via `validateMigration()` | `src/metadata/schema_version_manager.cpp` (Dry-Run-Checks), `tests/test_schema_version_dryrun.cpp` |
| SQL-nahe Migrationsableitung | `generateMigrationScript()` (ADD/DROP/ALTER) | `src/metadata/schema_version_manager.cpp`, `tests/test_schema_migration_script.cpp` |
| Online-Änderungen in Operationen | Gestufte DDL-Operationen mit Phasenmodell | `include/storage/online_schema_migration.h`, `src/storage/online_schema_migration.cpp`, `tests/test_online_schema_migration.cpp` |
| Additive Zero-Copy-Änderungen | In-place additive Migration + Versionseintrag | `include/updates/in_place_schema_migrator.h`, `src/updates/in_place_schema_migrator.cpp`, `tests/test_in_place_schema_migrator.cpp` |
| API-Zugriff für Schema/Versionen | REST-Handler inkl. Version/Diff-Endpunkte | `include/server/schema_api_handler.h`, `src/server/schema_api_handler.cpp` |

## B) Gegenprüfung zentraler Alt-Claims

| Alt-Claim (vorheriges Draft) | Ergebnis | Begründung |
|---|---|---|
| „YAML-basierte Laufzeit-Rekonfiguration ist implementiert“ | **Nicht bestätigt** | Kein belastbarer End-to-End-Pfad (Parser → Planer → sicherer Apply-Loop) im untersuchten OSS-Baum. |
| „Kubernetes-Operator ist empfohlene bestehende Implementierung“ | **Nicht bestätigt** | Als mögliche Integrationsrichtung valide, aber nicht als implementierter Kernpfad der Schema-Evolution belegt. |
| „Industrie-Metriken einzelner Firmen direkt übertragbar“ | **Nicht bestätigt** | Ohne direkte ThemisDB-Messung nur als externe Orientierung nutzbar, nicht als Produktnachweis. |
| „Schema-Evolution-Bausteine fehlen vollständig“ | **Falsch** | Versionierung/Migration/Dry-Run/API sind bereits vorhanden und getestet. |

## C) Reproduzierbare Qualitätsprüfung des Dokuments

Für diese Datei wurden die vorhandenen Dokumenten-Checks ausgeführt:

```bash
python3 scripts/docs-lint.py research/DYNAMIC_SCHEMA_RECONFIGURATION_RESEARCH.md
python3 scripts/link-check.py --internal-only research/DYNAMIC_SCHEMA_RECONFIGURATION_RESEARCH.md
```

Interpretation: Der frühere Stand hatte v. a. Struktur-/Hierarchieprobleme; der überarbeitete Stand folgt einer konsistenten Überschriftenstruktur und belegt Claims artefaktbasiert.

---

## Limitations / Known Issues

1. **Kein vollständiger deklarativer Runtime-Controller im OSS-Stand nachgewiesen**
   Vorhandene Bausteine müssen orchestriert werden; „one-click self-adaptive schema reconfiguration“ ist aktuell kein belegter Lieferzustand.

2. **Validierungstiefe von `validateMigration()` ist bewusst begrenzt**
   Fokus liegt auf Struktur-/Konsistenzchecks (z. B. leere Namen, Duplikate, identisches Schema), nicht auf umfassender semantischer Kompatibilitäts-Policy.

3. **Online-Migration ist operationell, aber kein Ersatz für globale Betriebsprozesse**
   Themen wie abgestimmtes Canary-Routing, SLO-Gating und Governance-Workflows liegen teilweise außerhalb der hier belegten Kernimplementierungen.

4. **Externe Literatur ist kontextgebend, nicht automatisch produktgleich**
   Ergebnisse aus F1, Vitess, TiDB, pt-osc etc. müssen in ThemisDB-spezifischen Benchmarks validiert werden.

---

## Schlussfolgerung / Fazit

**Kurzfazit:** ThemisDB verfügt bereits über einen substanziellen, testgestützten Kern für Schema-Evolution. Das Review entfernt unbelegte Aussagen und ersetzt sie durch belegbare Architektur- und Codebefunde.

**Empfohlener nächster Schritt (reviewfähig):**

1. Bestehende Komponenten (`SchemaVersionManager`, `SchemaMigrator`, `InPlaceSchemaMigrator`, `SchemaApiHandler`) als verbindlichen Basispfad dokumentieren.
2. Optionale YAML/JSON-Deklaration als separates, messbar validiertes Inkrement planen.
3. ThemisDB-spezifische Benchmarks für Schema-Änderungen ergänzen, bevor Performance- oder Betriebsversprechen erweitert werden.

---

## References / Quellen

## Interne ThemisDB-Artefakte

1. ThemisDB README (Multi-Model, ACID/MVCC-Claims):
   `../README.md`
2. Architektur-Übersicht:
   `../ARCHITECTURE.md`
3. Query-Modul (AQL-Terminologie):
   `../src/query/README.md`
4. Metadata-Roadmap (Status/Limitations):
   `../src/metadata/ROADMAP.md`
5. Schema-Versionierung und Dry-Run:
   `../include/metadata/schema_version_manager.h`, `../src/metadata/schema_version_manager.cpp`
6. Online Schema Migration:
   `../include/storage/online_schema_migration.h`, `../src/storage/online_schema_migration.cpp`
7. In-place additive Migration:
   `../include/updates/in_place_schema_migrator.h`, `../src/updates/in_place_schema_migrator.cpp`
8. Schema API:
   `../include/server/schema_api_handler.h`, `../src/server/schema_api_handler.cpp`
9. Fokus-Tests:
   `../tests/test_schema_version_dryrun.cpp`, `../tests/test_schema_migration_script.cpp`, `../tests/test_online_schema_migration.cpp`, `../tests/test_in_place_schema_migrator.cpp`

## Externe Literatur und Systeme (validierbare URL/DOI)

1. Shute, J. et al. (2013). *F1: A Distributed SQL Database That Scales.*
   URL: https://research.google/pubs/pub41344/
2. Klettke, M., Scherzinger, S., & Störl, U. (2015). *Schema evolution for NoSQL data stores.*
   DOI: https://doi.org/10.1145/2814710
3. Percona Toolkit – `pt-online-schema-change` (operational pattern reference):
   URL: https://docs.percona.com/percona-toolkit/pt-online-schema-change.html
4. GitHub `gh-ost` (online migration tooling pattern):
   URL: https://github.com/github/gh-ost
5. Vitess Schema Management:
   URL: https://vitess.io/docs/22.0/user-guides/schema-management/
6. TiDB Online DDL Overview:
   URL: https://docs.pingcap.com/tidb/stable/ddl-introduction
7. CockroachDB Online Schema Changes:
   URL: https://www.cockroachlabs.com/docs/stable/online-schema-changes

---

## Changelog

| Datum | Version | Änderungen |
|---|---|---|
| 2026-05-14 | 1.1 | Vollständige Review-Überarbeitung: Pflichtstruktur ergänzt, unbelegte Claims entfernt/relativiert, Terminologie vereinheitlicht, evidenzbasierte Evaluation und Quellenliste neu aufgebaut |
| 2026-02-10 | 1.0 | Initiale Fassung |
