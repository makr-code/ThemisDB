> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Metadata Module

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Security Scope

This document covers the security posture of the Metadata module, including `SchemaManager`,
`StatisticsCollector`, `INFORMATION_SCHEMA` views, `SchemaVersionManager`, changefeed notifications,
`ColumnLineageTracker`, `DistributedMetadataCatalog`, ER diagram export, schema audit log,
`SchemaConsistencyChecker`, `IndexRecommender`, and external catalog integrations (Apache Atlas, DataHub).

## Threat Model

| Threat | Attack Vector | Mitigation |
|--------|--------------|------------|
| Unauthorised schema enumeration | Unauthenticated or low-privilege caller querying `INFORMATION_SCHEMA` views to map database structure | RBAC enforcement: `INFORMATION_SCHEMA` queries are evaluated against the caller's privilege set; tables and columns not visible to the caller are omitted from results |
| Information disclosure via statistics | `StatisticsCollector` histogram data revealing value distributions of sensitive columns | Access to statistics requires the same privilege level as the underlying table; histogram API enforces the same RBAC check |
| Schema injection via migration scripts | Attacker-controlled input reaching `SchemaVersionManager` diff/migration output, producing malicious DDL | Structural validation of all schema change operations before diff generation; migration scripts are generated from an internal AST representation, not by string concatenation |
| AQL query injection through schema metadata | Schema or column names containing AQL-special characters passed to query engine | Schema identifiers are quoted and escaped before being embedded in any AQL statement; same injection protections as the AQL/query module apply |
| Unauthorised schema modification | DDL operations by callers without schema-write privileges | Schema mutation operations require an explicit `SCHEMA_WRITE` privilege; all mutations are recorded in the schema audit log with actor identity |
| External catalog data exfiltration | Apache Atlas or DataHub connector sending internal schema metadata to an unauthorised endpoint | Connector endpoints are configured at deployment time by an administrator; connectors operate with least-privilege outbound network access |
| Changefeed notification interception | Subscriber receiving schema change events it should not see | Changefeed subscriptions are scoped to tables the subscriber has at least `TABLE_READ` privilege on; events for out-of-scope tables are filtered before delivery |
| ER diagram leakage | Exported diagram revealing table relationships to unauthorised parties | Export operations require the same privilege as reading all constituent tables; partial exports are not produced |

## Security Controls

### Access Control
- All `INFORMATION_SCHEMA` views enforce RBAC at query time using the caller's session identity.
- Schema mutation operations require `SCHEMA_WRITE` privilege; schema read operations require `SCHEMA_READ`.
- Statistics and histogram data are gated behind the same privilege as the underlying table.

### Audit Logging
- Every DDL event (CREATE TABLE, ALTER TABLE, DROP TABLE, ADD/DROP INDEX, ADD/DROP CONSTRAINT) is recorded in the RocksDB-backed schema audit log with:
  - Timestamp (UTC, microsecond precision)
  - Actor identity (session user)
  - Operation type and target object
  - Before/after schema diff

### Input Validation & Injection Prevention
- Schema identifiers (table names, column names) are validated against an allow-list character set before storage.
- All schema identifiers are quoted and escaped before inclusion in AQL statements or migration scripts.
- Migration script generation uses an internal AST; no raw user input is interpolated into script text.

### External Catalog Integration
- Apache Atlas and DataHub connectors authenticate using service-account credentials stored in the secrets provider.
- Outbound connections use TLS with peer verification.
- The metadata synchronised to external catalogs is limited to schema structure; row-level data is never exported.

## Data Handling

- The schema audit log is append-only; entries cannot be modified or deleted through the normal API.
- Column lineage metadata may reference sensitive data paths; access to lineage records is gated behind `LINEAGE_READ` privilege.
- ER diagram exports contain only structural metadata (table and column names, relationships); no row data or statistics are included.

## Known Limitations

- AQL injection protections depend on correct escaping in the AQL module; the metadata module trusts the AQL module's escaping utilities. Any regression in the AQL module could affect this module.
- External catalog connectors (Apache Atlas, DataHub) are in an early integration state; their authentication and authorisation behaviour should be reviewed before use in production environments handling sensitive schemas.
- The `DistributedMetadataCatalog` conflict resolution mechanism has not been audited for Byzantine fault scenarios; it assumes non-adversarial catalog nodes.

## Dependency Security

| Dependency | Purpose | Notes |
|------------|---------|-------|
| RocksDB | Schema registry and audit log persistence | Embedded; no network exposure |
| Internal AQL module | Query execution for INFORMATION_SCHEMA views | Injection protections inherited from AQL module |
| Apache Atlas client | External catalog synchronisation | TLS + service-account auth required |
| DataHub client | External catalog synchronisation | TLS + service-account auth required |
