### Context

This issue implements the roadmap item 'MongoDB Document Importer (BSON types)' for the importers domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: MongoDB Document Importer

### Goal

Deliver the scoped changes for MongoDB Document Importer (BSON types) in src/importers/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### MongoDB Document Importer
**Priority:** High
**Target Version:** v1.8.0
**Status:** `src/importers/mongo_importer.cpp` exists; verify it handles all BSON extended JSON types.

**Implementation Notes:**
- `[ ]` BSON-to-JSON conversion must handle `ObjectId` → string, `ISODate` → ISO 8601, `Decimal128` → string, `Binary` → base64.
- `[ ]` Add integration test using a Docker MongoDB 6.0 container.

**Performance Targets:**
- Import throughput ≥ 30 000 documents/sec from a local MongoDB 6.0 instance with 2 KB average documents.



Add a MySQL/MariaDB source connector that mirrors the existing `postgres_importer.cpp` structure. MySQL is the second most commonly requested source after PostgreSQL and is needed to support migration workflows from legacy LAMP-stack applications.

**Implementation Notes:**
- Add `mysql_importer.cpp` implementing `IImporter`; use `libmysqlclient` or the MariaDB C Connector (prefer the latter for MariaDB-specific data types).
- Reuse `SchemaMapper` extracted from `postgres_importer.cpp`; add MySQL-specific type mappings (`TINYINT(1)` → `bool`, `TEXT` vs `LONGTEXT` sizing, `JSON` column native support via MySQL 5.7+).
- Support both full and incremental import; incremental mode uses a configurable `updated_at` column as the high-watermark (same pattern as the PostgreSQL importer).
- Add `importers_mysql_rows_imported_total` and `importers_mysql_errors_total` Prometheus counters, consistent with the naming convention in the import pipeline.
- Credentials must come from environment variables or a secrets manager reference; never from the `ImportConfig` struct fields that appear in log output.

**Performance Targets:**
- Import throughput ≥ 50 000 rows/sec from a local MySQL 8.0 instance (measured with a 10 M row benchmark table in `benchmarks/importers_bench.cpp`).
- Batch size of 1 000 rows per `IImporter::importBatch()` call with configurable override via `ImportConfig::batch_size`.

---

### Acceptance Criteria

- [ ] BSON-to-JSON conversion must handle `ObjectId` → string, `ISODate` → ISO 8601, `Decimal128` → string, `Binary` → base64.
- [ ] Add integration test using a Docker MongoDB 6.0 container.
- [ ] Import throughput ≥ 30 000 documents/sec from a local MongoDB 6.0 instance with 2 KB average documents.
- [ ] Add `mysql_importer.cpp` implementing `IImporter`; use `libmysqlclient` or the MariaDB C Connector (prefer the latter for MariaDB-specific data types).
- [ ] Reuse `SchemaMapper` extracted from `postgres_importer.cpp`; add MySQL-specific type mappings (`TINYINT(1)` → `bool`, `TEXT` vs `LONGTEXT` sizing, `JSON` column native support via MySQL 5.7+).
- [ ] Support both full and incremental import; incremental mode uses a configurable `updated_at` column as the high-watermark (same pattern as the PostgreSQL importer).
- [ ] Add `importers_mysql_rows_imported_total` and `importers_mysql_errors_total` Prometheus counters, consistent with the naming convention in the import pipeline.
- [ ] Credentials must come from environment variables or a secrets manager reference; never from the `ImportConfig` struct fields that appear in log output.
- [ ] Import throughput ≥ 50 000 rows/sec from a local MySQL 8.0 instance (measured with a 10 M row benchmark table in `benchmarks/importers_bench.cpp`).
- [ ] Batch size of 1 000 rows per `IImporter::importBatch()` call with configurable override via `ImportConfig::batch_size`.

### Relationships

- Roadmap row: #69 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/importers/FUTURE_ENHANCEMENTS.md#mongodb-document-importer
- Source key: roadmap:69:importers:v1.8.0:mongodb-document-importer

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:69:importers:v1.8.0:mongodb-document-importer -->
<!-- roadmap-ref: row=69;module=importers;target=v1.8.0 -->
<!-- roadmap-detail: src/importers/FUTURE_ENHANCEMENTS.md#mongodb-document-importer -->
