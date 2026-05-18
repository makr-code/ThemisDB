> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

# Importers Module — Implementation Overview

**Module Path:** `src/importers/`
**Public API:** `../../include/importers/README.md`

## Purpose

The `importers` module implements one-time and incremental ingestion into ThemisDB from relational, document, streaming, and file-based sources. It combines schema mapping, validation, conflict handling, and optional MDM post-processing behind a shared importer contract.

## Runtime Behavior

A typical import run performs:

1. Source validation (`IImporter::validateSource`)
2. Import execution (`importData`, `importDataStreaming`, or `importDataAsync`)
3. Optional schema validation / FK preservation / relationship mapping
4. Optional conflict resolution (`OVERWRITE`, `SKIP`, `MERGE`, `ERROR`)
5. Optional MDM linking and golden-record generation
6. Stats/error reporting via `ImportStats` and structured `ImportError` entries

## Main Components (`src/importers`)

| Component | File(s) | Runtime Role |
|---|---|---|
| Core SQL import | `postgres_importer.cpp`, `mysql_importer.cpp`, `sqlite_importer.cpp`, `oracle_importer.cpp` | Batch and incremental DB ingestion with schema conversion |
| Document / stream import | `mongo_importer.cpp`, `kafka_importer.cpp` | MongoDB and Kafka ingestion (`THEMIS_ENABLE_KAFKA` for live Kafka backend) |
| File / object import | `flatfile_importer.cpp`, `s3_importer.cpp` | CSV/TSV/Parquet/NDJSON and S3-backed imports (`THEMIS_ENABLE_S3` for S3 backend) |
| Validation and mapping | `schema_validator.cpp`, `schema_inference.cpp`, `polyglot_mapper.cpp`, `relationship_mapper` logic in importer stack | Type validation, schema inference, mapping decisions |
| Conflict/quality/audit | `conflict_resolver.cpp`, `data_quality.cpp`, `audit_trail.cpp` | Deduplication strategy, quality scoring, immutable audit trail |
| MDM stack | `mdm_engine.cpp`, `entity_linker.cpp`, `canonical_resolver.cpp`, `mdm_audit_trail.cpp`, `mdm_metrics.cpp`, `deterministic_matcher.cpp`, `semantic_matcher.cpp` | Entity linking, canonicalization, merge policy, MDM observability |
| Advanced pipelines | `adaptive_import.cpp`, `crdt_importer.cpp`, `postgres_cdc.cpp`, `temporal_support.cpp`, `blockchain_integrity.cpp`, `federated_learning.cpp`, `graphql_federation.cpp`, `column_importance.cpp`, `gui_import_wizard.cpp` | Optimization, CRDT merge, CDC contract (`THEMIS_ENABLE_CDC` for live stream), analytics, orchestration helpers |

## Public API + Configuration Surface

Primary entry points are defined in `include/importers/`:

- `importer_interface.h`: `IImporter`, `ImportOptions`, `ImportStats`, `ImportErrorCode`
- `importer_interfaces.h`: extension interfaces and plugin contracts
- `importer_plugin.h` + `importer_plugin_api.h`: stable C-linkage plugin ABI (`THEMIS_IMPORTER_PLUGIN_V1`) and runtime loader

Important `ImportOptions` groups used at runtime:

- Execution: `dry_run`, `continue_on_error`, `batch_size`
- Filtering/mapping: `include_tables`, `exclude_tables`, `column_mappings`, `table_mappings`, `type_overrides`
- Safety/limits: `max_row_size_bytes`, `max_statement_size_bytes`, `enforce_utf8`
- Resume/streaming: `checkpoint_file`, `delta_hash_file`, `delta_key_columns`, `streaming_row_callback`
- Conflict handling: `conflict_strategy`, `conflict_key_columns`, `protected_fields`, `merge_depth`
- Schema/FK behavior: `validate_schema`, `schema_sample_rows`, `preserve_foreign_keys`, `preserve_relationships`, `validate_references`

## Errors, Failure Modes, and Limits

Structured errors are exposed through `ImportStats::structured_errors` using `ImportErrorCode` (for example: file open/read failures, parsing failures, schema/type mismatches, permission denial, and conflict abort in `ERROR` mode).

Current module constraints:

- Kafka runtime ingestion requires `THEMIS_ENABLE_KAFKA` + librdkafka
- S3 runtime ingestion requires `THEMIS_ENABLE_S3`
- Live PostgreSQL CDC stream requires `THEMIS_ENABLE_CDC`
- Without connector-specific build flags, importers may compile but return "connector not supported" style runtime errors

## Installation

The module is built as part of the main ThemisDB build. Include headers from
`include/importers/` and link against the standard ThemisDB targets provided by
the root CMake configuration.

## Usage Snippet

```cpp
#include "importers/importer_interface.h"

using namespace themis::importers;

void runImport(IImporter& importer, const std::string& sourcePath) {
    ImportOptions options;
    options.batch_size = 2000;
    options.validate_schema = true;
    options.preserve_foreign_keys = true;
    options.conflict_strategy = ConflictStrategy::MERGE;

    auto stats = importer.importData(sourcePath, options);
    // inspect stats.imported_records / stats.structured_errors
}
```

## Troubleshooting

- `FILE_NOT_FOUND` / `FILE_OPEN_FAILED`: verify source path, permissions, and mount visibility
- `PERMISSION_DENIED`: verify ACL callback (`permission_check`) and caller role mapping
- `SCHEMA_VALIDATION_FAILED`: check source typing inconsistencies; tune `schema_sample_rows` / type overrides
- `CONFLICT_ERROR`: switch strategy from `ERROR` to `SKIP`/`MERGE` or adjust conflict keys
- Missing live CDC/Kafka/S3 behavior: confirm build-time flags and linked dependencies

## Related Docs

- Architecture: [`ARCHITECTURE.md`](./ARCHITECTURE.md)
- Roadmap: [`ROADMAP.md`](./ROADMAP.md)
- Future enhancements: [`FUTURE_ENHANCEMENTS.md`](./FUTURE_ENHANCEMENTS.md)
- Public header docs: [`../../include/importers/README.md`](../../include/importers/README.md)
- PostgreSQL importer details: [`../../docs/importers/POSTGRES_IMPORTER_V2.md`](../../docs/importers/POSTGRES_IMPORTER_V2.md)
- Plugin guide: [`../../docs/importers/plugin_guide.md`](../../docs/importers/plugin_guide.md)
- Troubleshooting runbook: [`../../docs/troubleshooting/importers_troubleshooting.md`](../../docs/troubleshooting/importers_troubleshooting.md)
- Operations runbook (DE): [`../../docs/de/features/importers_runbook.md`](../../docs/de/features/importers_runbook.md)
- Roadmap mirror (DE): [`../../docs/de/roadmap/importers_roadmap.md`](../../docs/de/roadmap/importers_roadmap.md)
- Primary sources index (EN): [`../../docs/en/importers/PRIMARY_SOURCES.md`](../../docs/en/importers/PRIMARY_SOURCES.md)
