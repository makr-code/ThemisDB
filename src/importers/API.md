---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "api-contract"
Canonical_References: "include/importers/importer_interface.h; include/importers/importers_api_contract.h; include/importers/importer_interfaces.h; include/importers/importer_plugin.h; include/importers/importer_plugin_api.h; src/importers/README.md"
Related_Reference: "makr-code/ThemisDB#7353"
---

> Exact model backend not verifiable from final document text alone.

# Importers API Reference

This page is an entry point for developers integrating importer implementations.
The declarations in `include/importers/` are authoritative; this page
summarizes the shared contract and points to connector-specific headers.

## Core interfaces

[`importer_interface.h`](../../include/importers/importer_interface.h) defines
`IImporter`, `ImportOptions`, `ImportStats`, `ImportError`, and `ImportErrorCode`.
The base workflow exposes initialization, source validation, synchronous and
asynchronous import, cancellation, and source-schema inspection. Streaming
imports deliver converted rows to a callback; returning `false` requests early
termination. Check each concrete class for the supported source types and
whether an option is implemented.

`ImportOptions` groups:

- batch and error behavior (`batch_size`, `continue_on_error`, `dry_run`);
- table/schema filtering and mapping (`include_tables`, `exclude_tables`,
  `column_mappings`, `table_mappings`, `type_overrides`);
- conflict and relationship handling;
- row-size, statement-size, UTF-8, and schema-validation controls;
- checkpoints, delta hashes, quarantine output, streaming callbacks;
- optional metrics, tracing, and a `permission_check` callback.

The shared interface defaults `batch_size` to 1,000, `validate_schema` to
`false`, `schema_sample_rows` to 100, and `import_timeout_ms` to 0 (disabled).
Row and statement size limits also default to 0 (unlimited). These are header
defaults, not a production-safe deployment profile; connector implementations
may apply additional configuration.

The permission callback, when supplied, is called at import start with
`("import", "write")`; a denial is reported as `PERMISSION_DENIED`. An unset
callback is not an authorization policy.

`ImportStats` reports total, imported, failed, skipped, and quarantined records;
conflict counters; warnings; string errors; and machine-readable
`structured_errors`. Check these fields rather than treating a returned stats
object as success by itself.

## Contract header

[`importers_api_contract.h`](../../include/importers/importers_api_contract.h)
defines a second, versioned contract surface: `SchemaChangeKind`,
`BadRowDisposition`, and `ImporterErrorCode`, plus sizing constants and
retry classification. It specifies preserved row order, import-ID
idempotency, schema evolution, bad-row handling, and commit visibility. Do not
confuse its `ImporterErrorCode` with the connector-facing `ImportErrorCode`
enum in `importer_interface.h`; they are distinct declarations.

## Connector and plugin headers

| Header | Entry point |
|---|---|
| `flatfile_importer.h` | `FlatFileImporter`; CSV, TSV, JSONL/NDJSON, and optionally Parquet |
| `postgres_importer.h`, `mysql_importer.h`, `sqlite_importer.h`, `oracle_importer.h` | Relational/database importers |
| `mongo_importer.h` | MongoDB document import |
| `kafka_importer.h`, `s3_importer.h` | Stream and object-source paths |
| `importer_interfaces.h` | Extended importer, plugin, async, and registry interfaces |
| `importer_plugin.h`, `importer_plugin_api.h` | V1 C ABI, loading, adapters, and sandbox configuration |
| `wikipedia_plugin.hpp`, `wikipedia_pipeline.hpp` | Wikipedia dump lifecycle, import/update, projection, validation, and export |
| `huggingface_ingest_plugin.h` | Caller-supplied legal dataset rows, validation, and AdaLoRA JSONL export |

Plugin ABI behavior and examples are also covered by the
[third-party plugin guide](../../docs/importers/plugin_guide.md). That guide
is secondary to the current public headers.

## Error handling and compatibility

`ImportErrorCode` distinguishes file I/O, parse, schema, conversion,
permission/policy, timeout, conflict, and generic failures. Errors carry a
severity, message, and location. The separate contract helper
`isRetryableImportError()` identifies timeout and unavailable-connector
conditions for that contract enum; callers must not automatically retry every
failed `ImportStats` result.

`importers_api_contract.h` declares stability within v1.x and says breaking
changes require a v2.0 change. This is the contract's stated policy, not proof
that every connector currently conforms. In particular, its idempotent replay
wording differs from the duplicate-ID rejection behavior in the focused mock
suite; see [`BUILD_STATUS.md`](./BUILD_STATUS.md) for the unresolved test/API
drift. Review
[`ROADMAP.md`](./ROADMAP.md) and [`CHANGELOG.md`](./CHANGELOG.md) for active
module planning and recorded history.
