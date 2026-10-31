# Metadata Module Modeling

**Author:** ThemisDB Contributors
**Created:** 2026-10-31
**Last Updated:** 2026-10-31
**Status:** review
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot coding agent / ThemisDB documentation orchestrator
**Human_Review:** false
**Human_Author:** ThemisDB Contributors
**Source level:** level1 — module-local
**SOT domain:** module-behavior
**Canonical source references:** [schema manager contract](../../include/metadata/schema_manager.h), [schema version contract](../../include/metadata/schema_version_manager.h), [information schema API](../../include/metadata/information_schema.h), [catalog exporter API](../../include/metadata/catalog_exporter.h), [API reference and stale-artifact warning](./API.md)
**Related issue / milestone:** [#7133](https://github.com/makr-code/ThemisDB/issues/7133) · DOC-WEEKLY-2026-W44

> Exact model backend not verifiable from final document text alone.

## Conceptual model

The module models metadata as discoverable schema descriptions and related
operational records. It does not own the database's general entity data or query
execution. See [`README.md`](./README.md) for the module boundary.

| Model | Representation and behavior |
|---|---|
| Database | `SchemaManager::DatabaseMetadata`: version, table count, estimated row total, capability flags, and refresh timestamp. |
| Table / collection | `SchemaManager::TableSchema`: name, kind, property descriptors, index descriptors, and estimated row count. The declared kinds are relational, document, graph node, graph edge, and vector. |
| Property | `SchemaManager::PropertyInfo`: name/type, nullable and index metadata. Discovery samples stored entities; it is not a full scan of all values. |
| Relationship | `SchemaManager::RelationshipSchema`: relationship name, source and destination table names, and properties. |
| Statistics | `TableStats`, `ColumnStats`, and `IndexStats`: sampled row/column and imported index estimates; collected statistics are cached and persisted by `StatisticsCollector`. |
| Constraint | `ColumnConstraint`: NOT NULL, UNIQUE, CHECK, DEFAULT, or FOREIGN KEY. `SchemaConstraints` holds its active state in memory and exposes explicit RocksDB persistence methods; persistence/load is not automatic. |
| Version and audit | `SchemaChange` stores a versioned schema snapshot; `SchemaAuditEntry` records schema-change events. These are separate records and APIs. |
| Lineage | `ColumnRef` and append-only `ColumnLineageEntry` records form an in-memory directed graph of derivations. JSON export does not by itself provide durable storage. |

## Lifecycle and state

1. `SchemaManager` discovers table prefixes and samples entities to build
   metadata; cache TTL and explicit `refreshCache()` govern refresh behavior.
2. Callers may apply validated schema overrides or patches through
   `setTableSchema()` / `patchTableSchema()`.
3. `SchemaVersionManager` snapshots schemas into version history and can apply
   a previous snapshot as a rollback, recording the rollback as a new version.
4. `SchemaAuditLog` provides a separate append-only durable history in RocksDB.
5. Statistics, constraints, lineage, and catalog publication have independent
   lifecycle and persistence contracts; do not assume one operation updates all
   of them automatically.

## Data-model caveats

- Discovery is observational: a property omitted from a sample may not appear
  in the inferred metadata. Check `discoverProperties()` in
  [`schema_manager.cpp`](./schema_manager.cpp).
- Mixed non-null property types are represented as `"string"` by the current
  discovery implementation. This is a compatibility fallback, not proof that
  the stored values are strings.
- Schema `vector` is a recognized field/table type; this module does not define
  vector encoding, embedding generation, or similarity-search semantics.
- Constraint durability requires callers to explicitly invoke
  `SchemaConstraints::persistTo()` / `persistTableTo()` and restore state with
  `loadFrom()` / `loadTableFrom()`. See
  [`schema_constraints.h`](../../include/metadata/schema_constraints.h) and
  [`test_schema_constraints_persistence.cpp`](../../tests/metadata/test_schema_constraints_persistence.cpp).
- `SchemaVersionManager` documents itself as not thread-safe. Callers must
  externally synchronize shared use.
- For current wire/JSON details, use the owning header for the API in question:
  [`schema_manager.h`](../../include/metadata/schema_manager.h),
  [`schema_version_manager.h`](../../include/metadata/schema_version_manager.h),
  [`information_schema.h`](../../include/metadata/information_schema.h), or
  [`catalog_exporter.h`](../../include/metadata/catalog_exporter.h). See
  [`API.md`](./API.md) for the warning about the stale
  `metadata_api_contract.h`; do not use that artifact as the current runtime
  contract.

## Related pages

- [API reference](./API.md)
- [Implementation map](./IMPLEMENTATION.md)
- [Embedding boundary](./EMBEDDING.md)
- [Trust and security boundaries](./TRUST.md)
