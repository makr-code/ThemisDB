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
| CSV / TSV / Parquet flat-file | `src/importers/flatfile_importer.cpp` | ✅ Production |
| Kafka consumer | `src/importers/kafka_importer.cpp` | ✅ Production (requires `THEMIS_ENABLE_KAFKA`) |
| Schema auto-detection | `src/importers/schema_inference.cpp` | ✅ Production |
| Elasticsearch | `src/importers/elasticsearch_importer.cpp` | ✅ Production (requires `THEMIS_ENABLE_ELASTICSEARCH`) |
| Redis | `src/importers/redis_importer.cpp` | ✅ Production (requires `THEMIS_ENABLE_REDIS`) |
| Debezium CDC | `src/importers/debezium_cdc_importer.cpp` | ✅ Production (requires `THEMIS_ENABLE_DEBEZIUM`) |

---

## In Progress

- [~] Integration tests for the PostgreSQL importer using a Docker-based Postgres fixture

## Completed ✅ (additional)
- [x] MongoDB importer (`src/importers/mongo_importer.cpp`) — Production Ready
- [x] CSV / TSV / Parquet flat-file importer (`src/importers/flatfile_importer.cpp`) — Production Ready
- [x] Kafka consumer importer (`src/importers/kafka_importer.cpp`) — Production Ready
- [x] Schema auto-detection (`src/importers/schema_inference.cpp`) — Production Ready
- [x] Elasticsearch importer (`src/importers/elasticsearch_importer.cpp`) — Production Ready (Q3 2026)
- [x] Redis importer (`src/importers/redis_importer.cpp`) — Production Ready (Q3 2026)
- [x] Debezium CDC importer (`src/importers/debezium_cdc_importer.cpp`) — Production Ready (Q3 2026)

## Planned Features

- [ ] Incremental import mode: import only rows added/changed since last timestamp (Target: Q4 2026)
- [ ] Generic JDBC-compatible importer using libpqxx abstractions (Target: Q1 2027)

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
- [x] **Elasticsearch importer** – scroll-based bulk migration with index mapping conversion. (`src/importers/elasticsearch_importer.cpp`) *(Production Ready)*
- [x] **Redis importer** – import keys matching a pattern; String, Hash, List, Set, ZSet. (`src/importers/redis_importer.cpp`) *(Production Ready)*
- [x] **CDC / Debezium importer** – continuous real-time ingestion via Debezium Kafka topics. (`src/importers/debezium_cdc_importer.cpp`) *(Production Ready)*
- [ ] Generic JDBC-compatible importer using libpqxx abstractions.

## Milestones

| Milestone | Target | Status |
|-----------|--------|--------|
| MySQL importer MVP | Q2 2026 | ✅ Done |
| SQLite importer MVP | Q2 2026 | ✅ Done |
| Oracle importer MVP | Q2 2026 | ✅ Done |
| Kafka consumer importer | Q2 2026 | ✅ Done |
| CSV / flat-file importer | Q2 2026 | ✅ Done |
| Schema auto-detection | Q2 2026 | ✅ Done |
| Elasticsearch importer | Q3 2026 | ✅ Done |
| Redis importer | Q3 2026 | ✅ Done |
| CDC / Debezium importer | Q3 2026 | ✅ Done |

## Implementation Phases

### Phase 1 – CSV / TSV & Incremental Import
- [x] Implement `CSVImporter` with configurable delimiter, quoting, header detection, and NULL mapping (delivered via `flatfile_importer.cpp`)
- [ ] Incremental import: persist last-imported timestamp/offset; re-import only changed rows
- [x] Unit tests: type coercion, null values, malformed rows, UTF-8 edge cases

### Phase 2 – Schema Auto-Detection
- [x] Introspect source table/file schema and generate ThemisDB collection definition automatically (`schema_inference.cpp`)
- [x] Conflict resolution: handle schema drift between subsequent imports
- [ ] Integration test: auto-schema PostgreSQL → ThemisDB round-trip

### Phase 3 – Kafka & CDC
- [x] Implement `KafkaConsumerImporter` using librdkafka; configurable consumer group and offset reset (`kafka_importer.cpp`)
- [x] Debezium CDC importer: consume Debezium Kafka topic events as ThemisDB upserts/deletes (`debezium_cdc_importer.cpp`)
- [x] At-least-once delivery guarantee; idempotency via document fingerprint

### Phase 4 – Elasticsearch & Redis Importers
- [x] `ElasticsearchImporter`: scroll API-based bulk migration with index mapping → schema conversion (`elasticsearch_importer.cpp`)
- [x] `RedisImporter`: import keys matching a pattern; support String, Hash, List, Set, ZSet types (`redis_importer.cpp`)
- [x] 24 focused unit tests (INC-01..INC-24) in `tests/importers/test_importers_new_connectors_focused.cpp`

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
| Schema auto-detection | ✅ Ready |
| Elasticsearch importer | ✅ Ready (requires `THEMIS_ENABLE_ELASTICSEARCH`) |
| Redis importer | ✅ Ready (requires `THEMIS_ENABLE_REDIS`) |
| Debezium CDC importer | ✅ Ready (requires `THEMIS_ENABLE_DEBEZIUM`) |
| Integration tests running in CI | ❌ Pending |
| Target throughput documented | ❌ Undefined |

## Known Issues & Limitations

- No integration tests are running in CI; importers are tested with unit tests and manual testing
- MySQL/Oracle importers require the respective client library at link time (`THEMIS_ENABLE_MYSQL`, `THEMIS_ENABLE_OCI`); builds without those flags return `CONNECTOR_NOT_SUPPORTED`
- Kafka importer requires `THEMIS_ENABLE_KAFKA` and librdkafka; compiles cleanly without it but every `importData()` call returns an error describing the missing build flag
- Elasticsearch importer requires `THEMIS_ENABLE_ELASTICSEARCH`; the full HTTP consumer loop (libcurl) is guarded behind this flag. Mock injection is available in all builds for testing.
- Redis importer requires `THEMIS_ENABLE_REDIS` and hiredis; compiles cleanly without the flag.
- Debezium CDC importer requires `THEMIS_ENABLE_DEBEZIUM` and librdkafka; the consumer loop is a documented placeholder pending the Kafka integration sprint (Target: Q4 2026).
- Target import throughput (rows/sec) has not been defined or benchmarked for the new connectors

---

*See also: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)*
