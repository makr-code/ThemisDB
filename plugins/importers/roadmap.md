# Importer Plugins – Roadmap

## Current Status

**Status:** ✅ Production-ready

Entry-point: `plugins/importers/` · implementations in `src/importers/`

| Importer | Implementation | Status |
|----------|---------------|--------|
| PostgreSQL | `src/importers/postgres_importer.cpp` | ✅ Production |
| MySQL / MariaDB | `src/importers/mysql_importer.cpp` | ✅ Production |
| MongoDB | `src/importers/mongo_importer.cpp` | ✅ Production |
| SQLite | `src/importers/sqlite_importer.cpp` | ✅ Production |
| S3-compatible Object Storage | `src/importers/s3_importer.cpp` | ✅ Production |

---

## Short-term Goals (next 1–2 sprints)

- [ ] Add integration tests for the PostgreSQL importer using a Docker-based Postgres fixture.
- [ ] Document all configuration parameters for both importers.
- [ ] Verify MongoDB importer status and update this roadmap accordingly.

## Mid-term Goals (1–3 months)

- [P] **MySQL / MariaDB importer** – common alternative to PostgreSQL. *(PR open)*
- [P] **SQLite importer** – portable single-file database import. *(PR open)*
- [ ] **CSV / TSV importer** – bulk-load flat files into ThemisDB collections.
- [ ] Incremental import mode: import only rows added/changed since last import timestamp.
- [ ] Schema auto-detection: infer ThemisDB collection schema from source table structure.

## Long-term Goals (3–12 months)

- [~] **Kafka consumer importer** – consume and import Kafka topic messages in real time. (`src/importers/kafka_importer.cpp`)
- [ ] **Elasticsearch importer** – migrate indices to ThemisDB.
- [ ] **Redis importer** – import Redis keys/values/hashes.
- [ ] **Change Data Capture (CDC)** – continuous real-time ingestion via Debezium / logical replication.
- [ ] Generic JDBC-compatible importer using libpqxx abstractions.

## Milestones

| Milestone | Target | Status |
|-----------|--------|--------|
| MySQL importer MVP | Q2 2026 | ✅ Done |
| SQLite importer MVP | Q2 2026 | ✅ Done |
| CSV importer | Q3 2026 | 🔲 Planned |
| CDC / real-time ingestion | 2026 | 🚧 In Progress |

## Dependencies

- `libpqxx` / `libmongoc` (already in ThemisDB vcpkg)
- ThemisDB `BaseEntity` storage layer
- ThemisDB `QueryEngine` + AQL

## Open Questions

- [ ] Should importers run as background jobs or synchronous blocking calls?
- [ ] What is the target throughput for bulk import (rows/sec)?

---

*See also: [`future_enhancements.md`](future_enhancements.md)*
