### Context

This issue implements the roadmap item 'MySQL / MariaDB Importer (wire & verify)' for the importers domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: MySQL / MariaDB Importer

### Goal

Deliver the scoped changes for MySQL / MariaDB Importer (wire & verify) in src/importers/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### MySQL / MariaDB Importer
**Priority:** High
**Target Version:** v1.8.0
**Status:** `src/importers/mysql_importer.cpp` exists but needs verification that it is fully wired.

**Implementation Notes:**
- `[ ]` Verify `mysql_importer.cpp` registers with `ImporterRegistry` and is reachable from the admin import API.
- `[ ]` Add Prometheus counters `importers_mysql_rows_imported_total` and `importers_mysql_errors_total` consistent with other importer naming.
- `[ ]` Add integration test using a Docker MySQL 8.0 container.

### Acceptance Criteria

- [ ] Verify `mysql_importer.cpp` registers with `ImporterRegistry` and is reachable from the admin import API.
- [ ] Add Prometheus counters `importers_mysql_rows_imported_total` and `importers_mysql_errors_total` consistent with other importer naming.
- [ ] Add integration test using a Docker MySQL 8.0 container.

### Relationships

- Roadmap row: #68 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/importers/FUTURE_ENHANCEMENTS.md#mysql--mariadb-importer
- Source key: roadmap:68:importers:v1.8.0:mysql-mariadb-importer

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:68:importers:v1.8.0:mysql-mariadb-importer -->
<!-- roadmap-ref: row=68;module=importers;target=v1.8.0 -->
<!-- roadmap-detail: src/importers/FUTURE_ENHANCEMENTS.md#mysql--mariadb-importer -->
