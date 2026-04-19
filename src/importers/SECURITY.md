> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Importers Module

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Security Scope

The Importers module connects to external data sources (PostgreSQL, SQLite, MySQL, Oracle, MongoDB, S3, BigQuery, Kafka, flat files), executes data extraction queries, infers and validates schemas, resolves entity conflicts via MDM, and writes records into ThemisDB. Security controls apply to all source connections, query construction, credential handling, file access, and MDM data ingestion.

## Threat Model

| Threat | Mitigation |
|--------|------------|
| SQL injection via malicious import query parameters or user-supplied filter expressions | All SQL importers use parameterised queries exclusively; string interpolation into SQL is prohibited at compile time via a linter rule |
| Credential leakage in import logs (database passwords, S3 access keys, OAuth tokens) | Credential masking applied in all log sinks; secrets are redacted before any log output; masked fields are configured in `audit_trail.cpp` |
| Path traversal in flat-file imports (e.g., `../../etc/passwd` via user-supplied file paths) | File paths are canonicalised and validated against an allow-listed base directory; relative traversal sequences rejected |
| Untrusted or malformed schema data causing schema inference engine to corrupt the target schema | Schema validator enforces type, constraint, and cardinality checks before any schema is applied; strict mode rejects unknown fields |
| MDM data quality attacks (injecting malformed golden records to corrupt the master data graph) | MDM engine validates all incoming records against the canonical schema; probabilistic match scores below threshold are quarantined for human review |
| SSRF via import source URL (e.g., pointing S3/BigQuery connector at internal metadata service) | Source URLs are validated against an allow-list of approved hosts/CIDRs; private RFC-1918 ranges are blocked |
| Credential exposure via blockchain integrity log | Blockchain integrity records contain only content hashes; raw record data and credentials are never written to the integrity log |

## Security Controls

### Parameterised Queries
- `postgres_importer.cpp`, `mysql_importer.cpp`, `sqlite_importer.cpp`, `oracle_importer.cpp` all use driver-level prepared statements for every query.
- User-supplied column filters and WHERE predicates are bound as parameters, never interpolated.
- A static analysis rule (`no-raw-sql-concat`) is enforced in CI to prevent regression.

### Credential Handling
- Database credentials are loaded from environment variables or a secrets manager; hardcoded credentials are prohibited.
- All credential fields are masked in `audit_trail.cpp` and `mdm_audit_trail.cpp` before log emission.
- S3 access keys and BigQuery service account tokens are never written to import progress logs.

### File Import Path Validation
- `flatfile_importer.cpp` canonicalises all user-supplied paths using `realpath()` and validates the result against the configured import base directory.
- Symlinks pointing outside the base directory are rejected.

### Schema Validation
- `schema_validator.cpp` enforces strict or lenient validation modes; strict mode is the default for production imports.
- Untrusted external schemas (e.g., inferred from MongoDB documents) are always validated before application.

### MDM Security
- Incoming records to the MDM engine are validated against the golden-record schema before deduplication.
- Match confidence scores are logged; records below the configured threshold are quarantined and not merged automatically.
- The `canonical_resolver.cpp` golden-record selection logic is deterministic and auditable.

### Network Security
- All external source connections use TLS 1.2+ (configurable minimum).
- Source host/CIDR allow-listing is enforced in the connection factory.

## Data Handling

- Import pipelines do not cache raw source records beyond the active import transaction.
- PII fields identified during schema inference are tagged; downstream masking policies can be applied before persistence.
- CRDT state vectors are stored in an isolated partition and do not contain raw record data.
- Blockchain integrity hashes cover record content only; credentials and internal IDs are excluded.

## Known Limitations

- GraphQL federation importer currently does not enforce query depth limits on incoming federation queries; a depth limit is planned.
- Federated learning component (`federated_learning.cpp`) is experimental; it should not be used with sensitive training data until a formal security review is completed.
- Kafka importer does not yet support mTLS for broker connections (plain SASL/SCRAM is supported); mTLS support is planned.

## Dependency Security

| Dependency | Usage | Review Status |
|------------|-------|---------------|
| libpq (PostgreSQL) | PostgreSQL/CDC connection and query | Reviewed; prepared statements used |
| libmysqlclient | MySQL import | Reviewed; prepared statements used |
| ODBC (Oracle) | Oracle import | Reviewed; parameterised binds used |
| mongoc driver | MongoDB import and change streams | Reviewed; no raw query string construction |
| AWS SDK (S3) | S3 multi-part download | Reviewed; credentials via SDK credential chain |
| Google Cloud BigQuery API | BigQuery streaming/batch read | Reviewed; OAuth via service account |
| librdkafka | Kafka import | Reviewed; SASL/SCRAM auth enabled |
| RocksDB | CRDT state storage | Reviewed; tenant key prefix isolation |
