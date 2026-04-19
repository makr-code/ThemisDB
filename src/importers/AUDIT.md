> ⚠️ **Historischer Auditbericht** – Befunde ohne aktuellen Codebeleg mit `<!-- TODO: add source file evidence -->` markieren. Veraltete Befunde entfernen.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Audit Report — Importers Module

**Last Audit:** 2026-03-12  
**Auditor:** Copilot  
**Status:** ✅ Pass

## Summary

| Metric | Result |
|--------|--------|
| Build System Registration | ✅ Verified |
| Source Files | 31 |
| Test Coverage | ✅ Unit tests present for all importers; integration tests for SQL, MongoDB, S3 |
| Open TODOs | 2 (GraphQL federation depth limits; federated learning security review) |
| Open Stubs | 0 |
| Security Issues | None critical open; 2 known limitations documented in SECURITY.md |

## Build System

The importers module is registered in `CMakeLists.txt` as the `themis_importers` static library. All 31 source files are listed explicitly. The module links against: libpq, libmysqlclient, ODBC, mongoc, AWS SDK, Google Cloud BigQuery API, librdkafka, RocksDB, and the internal MDM and schema inference libraries.

## Source Files Audited

| File | Responsibility | Status |
|------|---------------|--------|
| `adaptive_import.cpp` | Dynamic batch-size tuning, pipeline orchestration | ✅ Reviewed |
| `audit_trail.cpp` | Immutable import event logging with credential masking | ✅ Reviewed |
| `blockchain_integrity.cpp` | Content-hash chain for audit record tamper-evidence | ✅ Reviewed |
| `canonical_resolver.cpp` | MDM golden-record selection and merge arbitration | ✅ Reviewed |
| `column_importance.cpp` | Mutual-information and frequency-based column scoring | ✅ Reviewed |
| `conflict_resolver.cpp` | Pluggable conflict resolution strategy registry | ✅ Reviewed |
| `crdt_importer.cpp` | CRDT-based import with set-union, LWW, MVR strategies | ✅ Reviewed |
| `data_quality.cpp` | Completeness, uniqueness, and referential integrity checks | ✅ Reviewed |
| `deterministic_matcher.cpp` | Rule-based entity matching for MDM deduplication | ✅ Reviewed |
| `entity_linker.cpp` | Cross-source entity resolution and ID alignment | ✅ Reviewed |
| `federated_learning.cpp` | Experimental: federated schema drift detection | ⚠️ Experimental — security review pending |
| `flatfile_importer.cpp` | CSV/JSON/Parquet import with path validation | ✅ Reviewed |
| `graphql_federation.cpp` | GraphQL federation source import | ⚠️ Query depth limits not yet enforced |
| `gui_import_wizard.cpp` | Step-by-step GUI import configuration and preview | ✅ Reviewed |
| `kafka_importer.cpp` | Kafka exactly-once consumer with SASL/SCRAM auth | ✅ Reviewed |
| `mdm_audit_trail.cpp` | MDM-specific entity lifecycle event log | ✅ Reviewed |
| `mdm_engine.cpp` | Probabilistic matching, deduplication, merge policy | ✅ Reviewed |
| `mdm_metrics.cpp` | Prometheus-compatible MDM metrics exporter | ✅ Reviewed |
| `mongo_importer.cpp` | MongoDB importer with change-stream support | ✅ Reviewed |
| `mysql_importer.cpp` | MySQL importer with parameterised queries | ✅ Reviewed |
| `oracle_importer.cpp` | Oracle ODBC importer with CLOB/BLOB support | ✅ Reviewed |
| `polyglot_mapper.cpp` | Cross-source schema alignment and type mapping | ✅ Reviewed |
| `postgres_cdc.cpp` | PostgreSQL logical replication CDC (pgoutput) | ✅ Reviewed |
| `postgres_importer.cpp` | PostgreSQL bulk import with prepared statements | ✅ Reviewed |
| `postgres_importer_mdm.cpp` | PostgreSQL import with MDM deduplication integration | ✅ Reviewed |
| `s3_importer.cpp` | S3 multi-part download with checksum verification | ✅ Reviewed |
| `schema_inference.cpp` | Automatic schema inference for CSV/JSON/Parquet/MongoDB | ✅ Reviewed |
| `schema_validator.cpp` | Strict/lenient schema validation before apply | ✅ Reviewed |
| `semantic_matcher.cpp` | Embedding-based entity matching for MDM | ✅ Reviewed |
| `sqlite_importer.cpp` | SQLite importer with parameterised queries | ✅ Reviewed |
| `temporal_support.cpp` | Bi-temporal import (valid-time + transaction-time) | ✅ Reviewed |

## Test Coverage

- Unit tests in `tests/importers/` cover all 31 source files.
- SQL importer tests use an in-process mock driver to verify parameterised query construction without a live database.
- Integration tests cover: PostgreSQL (including CDC), MongoDB, S3, and Kafka (using Docker Compose fixtures).
- MDM engine tests include property-based tests for deduplication correctness across generated record pairs.
- Schema inference tests cover CSV, JSON, Parquet, and MongoDB document shapes including edge cases (nulls, mixed types, sparse fields).
- `federated_learning.cpp` has unit tests for the public API but lacks adversarial/security-oriented tests.

## Findings

### Resolved
- **Path traversal in flatfile importer** — Fixed in v1.1.0; `realpath()` canonicalisation and base-directory allow-listing added.
- **MySQL deadlock on bulk insert** — Fixed in v1.4.0; batch insert uses explicit transaction with retry logic.
- **CRDT tombstone re-import** — Fixed in v1.3.0; tombstone state is checked before merge.
- **Oracle CLOB >32 KB failure** — Fixed in v1.5.0; streaming CLOB reader used.
- **MongoDB nullable detection** — Fixed in v1.5.0; sparse-field heuristic added to schema inference.

### Open

#### ⚠️ GraphQL federation query depth limits
- `graphql_federation.cpp` does not enforce a maximum query depth on incoming federation queries, which could allow deeply nested queries to cause excessive memory allocation.
- **Severity:** Medium
- **Action:** Add configurable depth limit (default 10) at the federation query parser entry point.

#### ⚠️ Federated learning security review pending
- `federated_learning.cpp` is marked experimental. It has not undergone a formal security review for data leakage via model gradient inversion.
- **Severity:** Medium (experimental only; not enabled in production builds by default)
- **Action:** Complete security review before promoting to production status.

## Compliance

| Requirement | Status |
|-------------|--------|
| Parameterised queries in all SQL importers | ✅ Enforced |
| Credential masking in all log outputs | ✅ Enforced |
| File path validation (no traversal) | ✅ Enforced |
| Schema validation before apply | ✅ Enforced |
| MDM quarantine for low-confidence matches | ✅ Enforced |
| TLS 1.2+ for all source connections | ✅ Enforced |
| Source host allow-listing | ✅ Enforced |
| No hardcoded credentials | ✅ Enforced |
