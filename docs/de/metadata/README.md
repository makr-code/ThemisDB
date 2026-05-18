# Metadata-Modul

**Stand:** 13. Mai 2026
**Version:** v1.6.0
**Kategorie:** Schema & Metadaten
<!-- status: current | validated: 2026-05-13 | commit: HEAD -->

---

## Übersicht

Das Metadata-Modul ist das zentrale Schema-Introspektions- und Metadaten-Managementsystem
von ThemisDB. Es bietet automatische Schema-Erkennung, tabellenbasierte Statistiken,
SQL-konforme INFORMATION_SCHEMA-Sichten, Schema-Versionierung und -Migration, Audit-Logging,
Konsistenzprüfung, ER-Diagramm-Export sowie Integration mit externen Datenkatalogen
(Apache Atlas, DataHub).

**Reifegrad:** 🟢 Production-Ready — Alle Phase 1–4-Features sind ausgeliefert (v1.6.0).

---

## Komponenten-Übersicht

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| SchemaManager | `include/metadata/schema_manager.h` | `src/metadata/schema_manager.cpp` | Automatische Tabellenerkennung via RocksDB-Key-Scan; Thread-sicherer Cache mit konfigurierbarem TTL und adaptivem TTL |
| StatisticsCollector | `include/metadata/statistics_collector.h` | `src/metadata/statistics_collector.cpp` | Kardinalität, Selektivität, Equi-Height-Histogramme, NULL-Quote; Index-Statistik-Import; Auto-Refresh |
| InformationSchema | `include/metadata/information_schema.h` | `src/metadata/information_schema.cpp` | SQL:2003-konforme INFORMATION_SCHEMA-Sichten: `tables`, `columns`, `indexes`, `statistics` |
| SchemaVersionManager | `include/metadata/schema_version_manager.h` | `src/metadata/schema_version_manager.cpp` | Monoton wachsender Versionszähler; Diff zwischen Versionen; DDL-Migrationsskript-Generierung; Rollback |
| SchemaAuditLog | `include/metadata/schema_audit_log.h` | `src/metadata/schema_audit_log.cpp` | Dauerhaftes tabellenspezifisches Audit-Trail in RocksDB; Integration mit SchemaVersionManager via `setAuditLog()` |
| SchemaConsistencyChecker | `include/metadata/schema_consistency_checker.h` | `src/metadata/schema_consistency_checker.cpp` | Hintergrund-Gesundheitsscan für Metadatenkonsistenz |
| SchemaConstraints | `include/metadata/schema_constraints.h` | `src/metadata/schema_constraints.cpp` | Benutzerdefinierte Schema-Constraint-Validierung |
| ColumnLineageTracker | `include/metadata/column_lineage.h` | `src/metadata/column_lineage.cpp` | Spaltenbasierter Ableitungs-DAG und Datenherkunfts-Tracking |
| ERDiagramExporter | `include/metadata/er_diagram_exporter.h` | `src/metadata/er_diagram_exporter.cpp` | ER-Diagramm-Export (Mermaid, DOT, JSON) |
| CatalogExporter | `include/metadata/catalog_exporter.h` | `src/metadata/catalog_exporter.cpp` | Apache Atlas und DataHub Integration |
| DistributedMetadataCatalog | `include/metadata/distributed_catalog.h` | `src/metadata/distributed_catalog.cpp` | Verteilter Metadaten-Katalog über Shards |
| IndexRecommender | `include/metadata/index_recommender.h` | `src/metadata/index_recommender.cpp` | Querybasierter Index-Nutzungs-Tracker und Empfehlungsmotor |
| AQLSchemaBridge | `include/metadata/aql_schema_bridge.h` | _(Header-only)_ | AQL-Integration für Metadaten-Abfragen |
| IMetadataSecurityProvider | `include/metadata/imetadata_security_provider.h` | _(Header-only)_ | Pluggable RBAC / Zugriffskontrolle für alle Metadata-Operationen |
| IMetadataChangeListener | `include/metadata/imetadata_change_listener.h` | _(Header-only)_ | Observer-Interface für Schema-Änderungsereignisse |
| IMetadataExportPolicy | `include/metadata/imetadata_export_policy.h` | _(Header-only)_ | Pluggable Export-Policy für externe Katalog-Integration |
| IMetadataEncryptionProvider | `include/metadata/imetadata_encryption_provider.h` | _(Header-only)_ | Pluggable Feld-Level-Verschlüsselung für Metadata-Werte |
| MetadataSnapshot | `include/metadata/metadata_snapshot.h` | _(Header-only)_ | Point-in-Time-Schema-Snapshots |
| SchemaDiffEngine | `include/metadata/schema_diff.h` | _(Header-only)_ | Struktureller Diff-Motor für TableSchema-Vergleiche |

---

## Schnellstart-Beispiele

### Schema-Erkennung

```cpp
#include "metadata/schema_manager.h"
using namespace themis;

SchemaManager schema_mgr(db_wrapper, index_manager);

// Alle Tabellen auflisten
auto tables = schema_mgr.getAllTables();
for (const auto& t : tables) {
    std::cout << t.name << " (" << t.estimated_row_count << " Zeilen)\n";
}

// Einzelne Tabelle abfragen
auto schema = schema_mgr.getTableSchema("users");
if (schema) {
    for (const auto& prop : schema->properties) {
        std::cout << prop.name << ": " << prop.type << "\n";
    }
}
```

### Statistiken sammeln

```cpp
#include "metadata/statistics_collector.h"
using namespace themis;

StatisticsCollector stats(db_wrapper);

// Statistiken für eine Tabelle einsammeln
auto result = stats.collectStats("users");
if (result.ok) {
    std::cout << "Zeilen: " << result.value.row_count << "\n";
    for (const auto& [col, cs] : result.value.column_stats) {
        std::cout << col << " – Distinct: " << cs.distinct_count << "\n";
    }
}
```

### Schema-Versionierung mit Audit-Log

```cpp
#include "metadata/schema_version_manager.h"
#include "metadata/schema_audit_log.h"
using namespace themis;

SchemaAuditLog audit_log(db_wrapper);
SchemaVersionManager svm(db_wrapper, schema_mgr);
svm.setAuditLog(&audit_log);

// Neue Version anlegen
auto r = svm.createSchemaVersion("users", "alice", "E-Mail-Spalte hinzugefügt");
if (r.ok) std::cout << "Version " << r.value << " angelegt\n";

// Migrationsskript generieren
auto script = svm.generateMigrationScript("users", 1, 2);
if (script.ok) std::cout << script.value;

// Audit-Historie abrufen
for (const auto& entry : audit_log.getHistory("users")) {
    std::cout << entry.timestamp_str << "  " << entry.operation << "  " << entry.author << "\n";
}
```

### Apache Atlas / DataHub Export

```cpp
#include "metadata/catalog_exporter.h"
using namespace themis;

// Apache Atlas
CatalogExporter::Config cfg;
cfg.type     = CatalogExporter::CatalogType::APACHE_ATLAS;
cfg.endpoint = "http://atlas-host:21000";
cfg.username = "admin";
cfg.password = "admin";

CatalogExporter exporter(cfg);
auto r = exporter.publishSchema(schema_mgr.getAllTables());
if (!r.success) spdlog::error("Atlas-Fehler: {}", r.error);
```

---

## Feature-Matrix

| Feature | Status | Zieldokument |
|---------|--------|--------------|
| Automatische Tabellenerkennung | ✅ Production | `schema_manager.cpp` |
| Equi-Height-Histogramme | ✅ Production | `statistics_collector.cpp` |
| INFORMATION_SCHEMA-Sichten | ✅ Production | `information_schema.cpp` |
| Schema-Diff & Migrationsskript | ✅ Production | `schema_version_manager.cpp` |
| Changefeed-Benachrichtigungen | ✅ Production | `schema_manager.cpp::setChangefeed` |
| Adaptives TTL (Mutations-Rate) | ✅ Production | `schema_manager.cpp::enableAdaptiveTTL` |
| Audit-Log (RocksDB-persistent) | ✅ Production | `schema_audit_log.cpp` |
| Konsistenzprüfung (Background) | ✅ Production | `schema_consistency_checker.cpp` |
| ER-Diagramm-Export | ✅ Production | `er_diagram_exporter.cpp` |
| Spalten-Lineage / Provenienz | ✅ Production | `column_lineage.cpp` |
| Apache Atlas / DataHub | ✅ Production | `catalog_exporter.cpp` |
| Verteilter Katalog (Shards) | ✅ Production | `distributed_catalog.cpp` |
| Index-Empfehlungen | ✅ Production | `index_recommender.cpp` |
| Schema-API REST-Endpunkt | ✅ Production | `src/server/schema_api_handler.cpp` |
| Auto-generiertes OpenAPI-Schema | 🔲 Geplant v2.0/Q3 2027 | — |
| Explizites Compat-Mode-Policy-Enforcement | 🔲 Geplant v1.9/Q1 2027 | — |

---

## Tests

| Testdatei | Abdeckung |
|-----------|-----------|
| `tests/test_schema_manager.cpp` | SchemaManager, AdaptiveTTL, Changefeeds |
| `tests/test_statistics_collector.cpp` | Histogramme, Auto-Refresh, Index-Stats |
| `tests/test_statistics_auto_refresh.cpp` | Hintergrund-Auto-Refresh |
| `tests/test_information_schema.cpp` | INFORMATION_SCHEMA-Sichten (Integration) |
| `tests/test_schema_version_manager.cpp` | Versionierung, Diff, Migrationsskript, Audit-Log |
| `tests/test_schema_changefeed.cpp` | Changefeed-Benachrichtigungen |
| `tests/test_schema_audit_log.cpp` | Audit-Trail, JSON-Export, Round-Trip |
| `tests/test_schema_consistency_checker.cpp` | Konsistenzprüfung |
| `tests/test_schema_constraints.cpp` | Constraint-Validierung |
| `tests/test_schema_constraints_persistence.cpp` | Persistenz der Constraints |
| `tests/test_column_lineage.cpp` | Lineage-DAG, upstream/downstream |
| `tests/test_catalog_exporter.cpp` | Atlas & DataHub (injiziertes HTTP) |
| `tests/test_er_diagram_exporter.cpp` | Mermaid, DOT, JSON-Export |
| `tests/test_distributed_catalog.cpp` | Verteilter Katalog |
| `tests/test_index_recommender.cpp` | Empfehlungsmotor |
| `tests/test_schema_api_lineage.cpp` | REST-API-Endpunkte (Integration) |
| `tests/test_schema_migration_script.cpp` | DDL-Migrationsskriptgenerierung |
| `tests/test_schema_version_dryrun.cpp` | Dry-run-Validierung |
| `tests/test_schema_migration_regression.cpp` | Regressionstests für Schema-Migrationen |
| `tests/test_schema_migration_tester.cpp` | Schema-Migrations-Testsuite |
| `tests/test_schema_manager_fuzz.cpp` | Fuzz-Tests für SchemaManager |
| `tests/test_schema_validator.cpp` | Schema-Validierung |
| `tests/test_schema_encryption.cpp` | Verschlüsselung von Schema-Metadaten |
| `tests/test_metadata_security_provider.cpp` | RBAC-Interface (v1.6.0): grant/revoke/assertPermission |
| `tests/test_metadata_change_listener.cpp` | Observer-Interface (v1.6.0): event recording, callbacks |
| `tests/test_metadata_export_policy.cpp` | Export-Policy-Interface (v1.6.0): AlwaysExport, Never, Filtered |

---

## Bekannte Einschränkungen

- Erster Tabellenscan benötigt < 30 s für bis zu 10 M Keys (voller RocksDB-Scan).
- Statistiken sind stichprobenbasiert; Histogramm-Genauigkeit ±20 % bei gleichverteilten und schiefen Daten.
- `validateMigration` prüft strukturelle Konsistenz; vollständige Forward/Backward-Compat-Policy-Enforcement ist für v1.9 / Q1 2027 geplant.
- Schema-Versionshistorie ist auf die letzten 1.000 Versionen pro Tabelle im Speicher begrenzt.
- `FieldSetMetadataEncryptionProvider` (XOR-Cipher) ist ausschließlich für Tests geeignet; für Produktionsverschlüsselung ist eine eigene `IMetadataEncryptionProvider`-Implementierung (z. B. AES-256-GCM) erforderlich.

---

## Weiterführende Dokumentation (Primary)

| Dokument | Pfad | Beschreibung |
|----------|------|--------------|
| Modul-README | [`src/metadata/README.md`](../../../src/metadata/README.md) | Vollständige Komponentenbeschreibung, Konfiguration, Beispiele |
| Architektur | [`src/metadata/ARCHITECTURE.md`](../../../src/metadata/ARCHITECTURE.md) | Komponentendiagramme, Threading-Modell |
| Roadmap | [`src/metadata/ROADMAP.md`](../../../src/metadata/ROADMAP.md) | Implementierungsstatus und geplante Features |
| Future Enhancements | [`src/metadata/FUTURE_ENHANCEMENTS.md`](../../../src/metadata/FUTURE_ENHANCEMENTS.md) | Detaillierte Planung und wissenschaftliche Referenzen |
| Public API Headers | [`include/metadata/README.md`](../../../include/metadata/README.md) | API-Referenz für alle Header-Dateien |

---

## Verwandte Module

- [Storage Module](../storage/README.md) — Unterlagernde RocksDB-Schicht
- [Index Module](../../../src/index/README.md) — Index-Konstruktion und -Statistiken
- [Query Module](../query/README.md) — Query-Planung und -Optimierung (nutzt Statistiken)
- [Security Module](../security/README.md) — Zugriffskontrolle für Metadaten

---

## Installation

Das Metadata-Modul ist Bestandteil von ThemisDB. Die Header-Dateien befinden sich unter
`include/metadata/`. Für die Build-Konfiguration:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

Vollständige Build-Anleitung: [`src/metadata/README.md`](../../../src/metadata/README.md)

---

## Usage

Einstiegspunkt für die Metadata-API ist `SchemaManager`:

```cpp
#include "metadata/schema_manager.h"
using namespace themis;

SchemaManager schema_mgr(db, &idx_mgr);
auto tables = schema_mgr.getAllTables();
```

Weitere Schnellstart-Beispiele: Abschnitt [Schnellstart-Beispiele](#schnellstart-beispiele) oben.<br>
Vollständige API-Referenz: [`include/metadata/README.md`](../../../include/metadata/README.md)
