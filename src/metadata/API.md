# Metadata API Reference

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
**SOT domain:** api-contract
**Canonical source references:** [metadata public headers](../../include/metadata/), [catalog exporter API](../../include/metadata/catalog_exporter.h), [AQL schema adapter](../../include/metadata/aql_schema_bridge.h)
**Related issue / milestone:** [#7133](https://github.com/makr-code/ThemisDB/issues/7133) · DOC-WEEKLY-2026-W44

> Exact model backend not verifiable from final document text alone.

## Entry points

The public C++ surface is declared under [`include/metadata`](../../include/metadata/).
Use those headers as the API authority; this page is an index, not a second
definition of signatures.

| API | Main operations | Contract notes |
|---|---|---|
| `SchemaManager` | Discover tables/relationships; read JSON; refresh cache; validate, set, patch, or delete custom schemas. | Unknown `getTable()` returns `std::nullopt`; schema parser can throw on malformed JSON; setters return explicit boolean outcomes. |
| AQL schema adapter | `themis::aql::fromTableSchema()` copies a `SchemaManager::TableSchema` into `CollectionMetadata`. | The adapter accepts an empty property list; the metadata header does not declare an AQL field-resolution API. |
| `StatisticsCollector` | Collect, read, update, and clear table/index statistics; set refresh interval. | Operations return typed `StatsResult`; reads may load persisted data; refresh thread is stopped by `stopRefresh()` or destruction. |
| `InformationSchema` | Read TABLES, COLUMNS, STATISTICS, and related rows. | Derived from `SchemaManager`; no independent schema persistence. |
| `SchemaVersionManager` | Create/read history, diff versions, validate migrations, generate migration scripts, roll back. | Uses `VersionResult`; documented not thread-safe, so externally synchronize shared use. |
| `SchemaConstraints` | Register, validate, persist, and load constraints. | Missing row-map key is treated as NULL; persistence is explicit via `persistTo()` / `persistTableTo()` and `loadFrom()` / `loadTableFrom()`; the class is not thread-safe. |
| `SchemaAuditLog` | Record and retrieve schema change history. | Writes are durable via RocksDB; `record()` returns false on storage failure. |
| `ColumnLineageTracker` | Record derivations, traverse lineage, export JSON. | In-memory, append-only entries; all public methods are thread-safe. |
| `CatalogExporter` | Publish one table or a table set to Atlas or DataHub. | `PublishResult` exposes success/error; HTTP transport is replaceable in tests. Its constructor/API does not accept `IMetadataExportPolicy`. |
| `DistributedMetadataCatalog` | Publish, sync, fetch, list, and remove distributed schema records. | Router is a non-owning reference; failures are explicit boolean/optional results. |

## Stale contract artifact and integration boundaries

[`metadata_api_contract.h`](../../include/metadata/metadata_api_contract.h) is
stale relative to the implemented metadata API and must not be used as its
current contract. It declares `MetaError` and documents intended behavior, but
its prose names `CatalogExporter::exportSchema()` and
`AqlSchemaBridge::resolve()`. Those statements describe the frozen artifact,
not implemented runtime behavior: current interfaces expose
`CatalogExporter::publishSchema()` / `publishTable()` and
[`themis::aql::fromTableSchema()`](../../include/metadata/aql_schema_bridge.h).
Current API evidence is in the owning headers, implementations, and behavior
tests: [`catalog_exporter.h`](../../include/metadata/catalog_exporter.h),
[`catalog_exporter.cpp`](./catalog_exporter.cpp),
[`test_catalog_exporter.cpp`](../../tests/metadata/test_catalog_exporter.cpp),
and [`aql_schema_bridge.h`](../../include/metadata/aql_schema_bridge.h).

The metadata APIs in this tree do not return the `MetaError` values merely
because the frozen artifact declares them. Use the actual result/error types
declared by each owning header (`StatsErrorCode`, `VersionErrorCode`,
`ConstraintErrorCode`, and `PublishResult` where applicable). The focused
[`test_metadata_contract_hardening_focused.cpp`](../../tests/metadata/test_metadata_contract_hardening_focused.cpp)
checks `MetaError` enum values only; it does not prove the mismatched method
names or documented behavior are implemented.

`IMetadataExportPolicy` is a separate interface with unit tests, but
[`CatalogExporter`](../../include/metadata/catalog_exporter.h) has no policy
constructor argument or setter, and its publish API does not expose a policy
parameter. Do not assume policy checks or delays are applied to publish calls;
an integration needing filtering must explicitly enforce it at its caller.
See [`test_metadata_export_policy.cpp`](../../tests/metadata/test_metadata_export_policy.cpp)
for policy-object behavior, not exporter integration.

## Caller checklist

- Respect non-owning pointer/reference lifetimes (metrics hooks, routers,
  changefeeds, audit log, statistics collector).
- Handle false, empty, `std::nullopt`, typed error results, and documented
  exceptions as distinct outcomes.
- Do not infer thread-safety for one class from another: consult the owning
  header.
- Treat `generateMigrationScript()` as output generation, not execution.

For implementation ownership and test entry points, see
[IMPLEMENTATION.md](./IMPLEMENTATION.md).
