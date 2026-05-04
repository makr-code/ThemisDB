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

## In Progress

- [~] Integration tests for the PostgreSQL importer using a Docker-based Postgres fixture

## Completed ✅ (additional)
- [x] MongoDB importer (`src/importers/mongo_importer.cpp`) — Production Ready

## Planned Features

- [ ] **CSV / TSV importer** – bulk-load flat files into ThemisDB collections (Target: Q3 2026)
- [ ] Incremental import mode: import only rows added/changed since last timestamp (Target: Q3 2026)
- [ ] Schema auto-detection: infer ThemisDB schema from source table structure (Target: Q3 2026)
- [ ] **Kafka consumer importer** – real-time Kafka topic ingestion (Target: Q3 2026)
- [ ] **Elasticsearch importer** – migrate indices to ThemisDB (Target: Q4 2026)
- [ ] **Redis importer** – import Redis keys/values/hashes (Target: Q4 2026)
- [ ] **Change Data Capture (CDC)** via Debezium / logical replication (Target: 2027)

---

## Short-term Goals (next 1–2 sprints)

- [ ] Add integration tests for the PostgreSQL importer using a Docker-based Postgres fixture.
- [ ] Document all configuration parameters for all importers.

## Mid-term Goals (1–3 months)

- [x] **MySQL / MariaDB importer** – `src/importers/mysql_importer.cpp` *(Production Ready)*
- [x] **SQLite importer** – `src/importers/sqlite_importer.cpp` *(Production Ready)*
- [x] **CSV / TSV / Parquet flat-file importer** – `src/importers/flatfile_importer.cpp` with schema auto-detection *(Production Ready)*
- [x] **Oracle Database importer** – `src/importers/oracle_importer.cpp` *(Production Ready)*
- [ ] Incremental import mode: import only rows added/changed since last import timestamp.

## Long-term Goals (3–12 months)

- [x] **Kafka consumer importer** – consume and import Kafka topic messages in real time. (`src/importers/kafka_importer.cpp`) *(Production Ready)*
- [ ] **Elasticsearch importer** – migrate indices to ThemisDB.
- [ ] **Redis importer** – import Redis keys/values/hashes.
- [ ] **Change Data Capture (CDC)** – continuous real-time ingestion via Debezium / logical replication.
- [ ] Generic JDBC-compatible importer using libpqxx abstractions.

## Milestones

| Milestone | Target | Status |
|-----------|--------|--------|
| MySQL importer MVP | Q2 2026 | ✅ Done |
| SQLite importer MVP | Q2 2026 | ✅ Done |
| Oracle importer MVP | Q2 2026 | ✅ Done |
| Kafka consumer importer | Q2 2026 | ✅ Done |
| CSV / flat-file importer | Q2 2026 | ✅ Done |
| CDC / real-time ingestion | 2026 | 🔲 Planned |

## Implementation Phases

### Phase 1 – CSV / TSV & Incremental Import
- [ ] Implement `CSVImporter` with configurable delimiter, quoting, header detection, and NULL mapping
- [ ] Incremental import: persist last-imported timestamp/offset; re-import only changed rows
- [ ] Unit tests: type coercion, null values, malformed rows, UTF-8 edge cases

### Phase 2 – Schema Auto-Detection
- [ ] Introspect source table/file schema and generate ThemisDB collection definition automatically
- [ ] Conflict resolution: handle schema drift between subsequent imports
- [ ] Integration test: auto-schema PostgreSQL → ThemisDB round-trip

### Phase 3 – Kafka & CDC
- [ ] Implement `KafkaConsumerImporter` using librdkafka; configurable consumer group and offset reset
- [ ] CDC via Debezium: consume PostgreSQL logical replication stream as ThemisDB upserts/deletes
- [ ] At-least-once delivery guarantee; idempotency via document fingerprint

### Phase 4 – Elasticsearch & Redis Importers
- [ ] `ElasticsearchImporter`: scroll API-based bulk migration with index mapping → schema conversion
- [ ] `RedisImporter`: import keys matching a pattern; support String, Hash, List, Set types
- [ ] Target throughput documented and benchmarked (rows/sec per importer)

---

## Dependencies

- `libpqxx` / `libmongoc` (already in ThemisDB vcpkg)
- ThemisDB `BaseEntity` storage layer
- ThemisDB `QueryEngine` + AQL

## Open Questions

- [ ] Should importers run as background jobs or synchronous blocking calls?
- [ ] What is the target throughput for bulk import (rows/sec)?

---

## Production Readiness Checklist

| Item | Status |
|------|--------|
| PostgreSQL importer | ✅ Ready |
| MySQL / MariaDB importer | ✅ Ready |
| SQLite importer | ✅ Ready |
| Oracle Database importer | ✅ Ready |
| S3-compatible object storage importer | ✅ Ready |
| MongoDB importer | ✅ Ready |
| Kafka consumer importer | ✅ Ready (requires `THEMIS_ENABLE_KAFKA`) |
| CSV / TSV / Parquet flat-file importer | ✅ Ready |
| Integration tests running in CI | ❌ Pending |
| Elasticsearch importer | ❌ Not implemented |
| CDC / Debezium importer | ❌ Not implemented |
| Target throughput documented | ❌ Undefined |

## Known Issues & Limitations

- No integration tests are running in CI; importers are tested with unit tests and manual testing
- MySQL/Oracle importers require the respective client library at link time (`THEMIS_ENABLE_MYSQL`, `THEMIS_ENABLE_OCI`); builds without those flags return `CONNECTOR_NOT_SUPPORTED`
- Kafka importer requires `THEMIS_ENABLE_KAFKA` and librdkafka; compiles cleanly without it but every `importData()` call returns an error describing the missing build flag
- Target import throughput (rows/sec) has not been defined or benchmarked

---

*See also: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)*
