# Metadata-Modul — Primär-Inventar
<!-- status: current | validated: 2026-05-13 | commit: HEAD -->

**Modul:** `metadata`
**Stand:** 2026-05-13
**Quelle:** Reality-Check gegen Sourcecode-Stand HEAD

---

## Primäre Dokumentationsdateien

### `src/metadata/`

| Datei | Typ | Status |
|-------|-----|--------|
| `src/metadata/README.md` | Modul-Übersicht, Komponenten, Konfiguration, Beispiele | ✅ aktuell |
| `src/metadata/ROADMAP.md` | Feature-Status (Phases 1–4 abgeschlossen), geplante Features | ✅ aktuell |
| `src/metadata/ARCHITECTURE.md` | Komponentendiagramme, Datenfluss, Threading-Modell | ✅ aktuell |
| `src/metadata/FUTURE_ENHANCEMENTS.md` | Geplante Erweiterungen, Design Constraints, IEEE-Referenzen | ✅ aktuell |

### `include/metadata/`

| Datei | Typ | Status |
|-------|-----|--------|
| `include/metadata/README.md` | Public API Referenz (alle 19 Header, Klassen, Beispiele, Troubleshooting) | ✅ aktuell |

---

## Header-Dateien (Public API)

| Header | Klasse / Typ | Beschreibung |
|--------|-------------|--------------|
| `include/metadata/schema_manager.h` | `SchemaManager` | Automatische Tabellenerkennung, Thread-sicherer Cache, Changefeeds, adaptives TTL |
| `include/metadata/statistics_collector.h` | `StatisticsCollector` | Kardinalität, Selektivität, Equi-Height-Histogramme |
| `include/metadata/information_schema.h` | `InformationSchema` | SQL:2003-konforme INFORMATION_SCHEMA-Sichten |
| `include/metadata/schema_version_manager.h` | `SchemaVersionManager` | Schema-Versionierung, Diff, Migrationsskript, Rollback |
| `include/metadata/schema_audit_log.h` | `SchemaAuditLog` | Dauerhaftes Audit-Trail (RocksDB) |
| `include/metadata/schema_consistency_checker.h` | `SchemaConsistencyChecker` | Hintergrund-Konsistenzprüfung |
| `include/metadata/schema_constraints.h` | `SchemaConstraints` | Benutzerdefinierte Constraint-Validierung |
| `include/metadata/column_lineage.h` | `ColumnLineageTracker` | Spalten-Ableitungs-DAG, Provenienz |
| `include/metadata/er_diagram_exporter.h` | `ERDiagramExporter` | ER-Diagramm-Export (Mermaid, DOT, JSON) |
| `include/metadata/catalog_exporter.h` | `CatalogExporter` | Apache Atlas & DataHub Integration |
| `include/metadata/distributed_catalog.h` | `DistributedMetadataCatalog` | Verteilter Katalog über Shards |
| `include/metadata/index_recommender.h` | `IndexRecommender` | Query-Pattern-basierter Index-Empfehlungsmotor |
| `include/metadata/aql_schema_bridge.h` | `aql::fromTableSchema()` | AQL-Integration für Metadaten-Abfragen (header-only) |
| `include/metadata/imetadata_security_provider.h` | `IMetadataSecurityProvider`, `NoOpMetadataSecurityProvider`, `InMemoryRbacMetadataSecurityProvider` | Pluggable RBAC / Zugriffskontrolle (v1.6.0, header-only) |
| `include/metadata/imetadata_change_listener.h` | `IMetadataChangeListener`, `RecordingMetadataChangeListener`, `MetadataChangeEvent` | Observer-Interface für Schema-Änderungsereignisse (v1.6.0, header-only) |
| `include/metadata/imetadata_export_policy.h` | `IMetadataExportPolicy`, `AlwaysExportPolicy`, `NeverExportPolicy`, `FilteredExportPolicy` | Export-Policy für externe Katalog-Integration (v1.6.0, header-only) |
| `include/metadata/imetadata_encryption_provider.h` | `IMetadataEncryptionProvider`, `NoOpMetadataEncryptionProvider`, `FieldSetMetadataEncryptionProvider` | Feld-Level-Verschlüsselung (v1.6.0, header-only) |
| `include/metadata/metadata_snapshot.h` | `MetadataSnapshot`, `IMetadataSnapshotStore`, `InMemoryMetadataSnapshotStore` | Point-in-Time-Schema-Snapshots (header-only) |
| `include/metadata/schema_diff.h` | `SchemaDiffEngine`, `SchemaDiff`, `ColumnDiff`, `IndexDiff` | Struktureller Diff-Motor für TableSchema-Vergleiche (header-only) |

---

## Implementierungsdateien (Source)

| Datei | Klasse / Beschreibung |
|-------|-----------------------|
| `src/metadata/schema_manager.cpp` | `SchemaManager` + SystemCatalog-Persistenz (kein separates `system_catalog.cpp`) |
| `src/metadata/statistics_collector.cpp` | `StatisticsCollector` |
| `src/metadata/information_schema.cpp` | `InformationSchema` |
| `src/metadata/schema_version_manager.cpp` | `SchemaVersionManager` |
| `src/metadata/schema_audit_log.cpp` | `SchemaAuditLog` |
| `src/metadata/schema_consistency_checker.cpp` | `SchemaConsistencyChecker` |
| `src/metadata/schema_constraints.cpp` | `SchemaConstraints` |
| `src/metadata/column_lineage.cpp` | `ColumnLineageTracker` |
| `src/metadata/er_diagram_exporter.cpp` | `ERDiagramExporter` |
| `src/metadata/catalog_exporter.cpp` | `CatalogExporter` |
| `src/metadata/distributed_catalog.cpp` | `DistributedMetadataCatalog` |
| `src/metadata/index_recommender.cpp` | `IndexRecommender` |

> **Hinweis:** `system_catalog.cpp` existiert nicht. Die SystemCatalog-Funktionalität
> (Persistenz via RocksDB) ist in `schema_manager.cpp` implementiert. Siehe META-MISSING-006
> in [`MISSING_IMPLEMENTATIONS.md`](MISSING_IMPLEMENTATIONS.md).

---

## Sekundäre Dokumentationsdateien

| Datei | Beschreibung |
|-------|-------------|
| `docs/de/metadata/README.md` | Deutsche Überblicks-Dokumentation mit Schnellstart, Feature-Matrix, Tests |
| `docs/de/metadata/MISSING_IMPLEMENTATIONS.md` | Befund-Report aus Reality-Check (6 Findings) |
| `docs/de/metadata/missing-implementations.json` | Maschinenlesbare Version (6 Findings) |
| `docs/de/metadata/inventory.md` | Dieses Dokument |

---

## Weiterführende Dokumentation

- [src/metadata/README.md](../../../src/metadata/README.md) — Vollständige Modulbeschreibung
- [src/metadata/ROADMAP.md](../../../src/metadata/ROADMAP.md) — Feature-Status und Planung
- [include/metadata/README.md](../../../include/metadata/README.md) — Public API Referenz
- [docs/de/metadata/README.md](README.md) — Sekundäre Dokumentation (Deutsch)
- [docs/de/metadata/MISSING_IMPLEMENTATIONS.md](MISSING_IMPLEMENTATIONS.md) — Befund-Report
