---
Author: "ThemisDB Team"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "api-contract"
Canonical_References: "include/importers/*.h; include/importers/*.hpp; include/importers/importer_interface.h; include/importers/importers_api_contract.h; src/importers/README.md; CMakePresets.json"
Related_Reference: "makr-code/ThemisDB#7353"
---

> Creation-date note: Original creation date is not recorded in the available checkout; the front-matter `Created` date is a metadata placeholder, not independently verified.

> Exact model backend not verifiable from final document text alone.

# Importers Module — Public Headers

**Module Path:** `include/importers/`
**Implementation Overview:** `../../src/importers/README.md`

## Purpose

This directory contains the public C++ interfaces for importer execution, options, stats, plugin integration, and source-specific import connectors.

## Header Entry-Points

| Header | Primary API | Runtime Role |
|---|---|---|
| `importer_interface.h` | `IImporter`, `ImportOptions`, `ImportStats`, `ImportErrorCode`, `ConflictStrategy` | Core import contract, options, status and structured error model |
| `importer_interfaces.h` | `IImportConflictResolver`, `IFlatFileSchemaDetector`, `IKafkaConsumerSource`, `IIncrementalImportCursor`, `IImporterPlugin`, `IImporterPluginRegistry` | Extended connector, source, and plugin interfaces |
| `importer_plugin.h` | `THEMIS_IMPORTER_PLUGIN_V1`, `THEMIS_IMPORTER_CREATE_SYMBOL` | Stable C ABI descriptor for third-party importer plugins |
| `importer_plugin_api.h` | `ImporterPluginRegistry`, `V1ImporterAdapter`, `PluginSandboxConfig` | Runtime plugin loading, ABI validation, timeout/memory sandbox hooks |
| `huggingface_ingest_plugin.h` | `HuggingFaceIngestPlugin` | Legal-domain HuggingFace ingest (snapshot/update/validate/AdaLoRA export) |
| Source headers (`postgres_importer.h`, `mysql_importer.h`, `mongo_importer.h`, `sqlite_importer.h`, `oracle_importer.h`, `kafka_importer.h`, `s3_importer.h`, `flatfile_importer.h`, `redis_importer.h`, `elasticsearch_importer.h`, `debezium_cdc_importer.h`) | Concrete importer classes and connector surfaces | Source-specific declarations; optional availability depends on build/runtime configuration |
| MDM and pipeline headers (`mdm_engine.h`, `entity_linker.h`, `canonical_resolver.h`, `conflict_resolver.h`, `adaptive_import.h`, `data_quality.h`, `schema_inference.h`, `schema_validator.h`) | Specialized processing APIs | MDM, conflict handling, validation, optimization, and quality scoring |
| `wikipedia_plugin.hpp`, `wikipedia_pipeline.hpp`, `wikipedia_transform.hpp`, `wikipedia_checkpoint.hpp`, `wikipedia_types.hpp`, `wikipedia_config.hpp` | `WikipediaIngestionPlugin`, `WikipediaIngestionPipeline` | Wikimedia full import, delta refresh, multi-model projection rebuild, checkpoint/resume, validation, and portable `wikipedia.db` export |

## Public API declarations

The behavior summaries below describe declared interfaces. They do not prove
that every connector implements each shared option or that a configured build
contains every optional source.

### Core importer contract (`IImporter`)

- `initialize(config)` configures a connector instance
- `validateSource(source, errors)` performs preflight validation
- `importData(...)` runs synchronous import
- `importDataStreaming(...)` emits row-by-row callbacks for memory-bounded processing
- `importDataAsync(...)` returns `ImportHandle` with live counters and future-based completion
- `getSourceSchema(...)` exposes source schema metadata

### Shared options and stats

- `ImportOptions` controls dry-run, batching, filtering, mapping, validation, conflict strategy, resume/checkpoint, observability callbacks, and MDM linking
- `ImportStats` reports record counters, conflict counters, FK/relationship counters, warnings/errors, and machine-readable `structured_errors`
- `ImportErrorCode` classifies failure causes (I/O, parsing, schema/type conversion, permission/policy, conflict failure)
- `WikipediaIngestionPipeline` keeps a canonical Wikipedia relational core (`wiki_page`, `wiki_revision`, `wiki_link`, `wiki_category`, `wiki_redirect`, ingest state, dead letters) and rebuilds graph/vector/process/timeseries projections from dirty pages only

### Plugin ABI behavior

- V1 plugins must export `themis_importer_create` returning `THEMIS_IMPORTER_PLUGIN_V1`
- Loader validates ABI version and struct size before registration
- `ImporterPluginRegistry::loadPlugin()` supports per-job sandbox limits (memory + timeout)
- `lastLoadError()` surfaces human-readable load failures

## Configuration and Limits

| Area | Important Fields / Flags | Notes |
|---|---|---|
| Execution | `dry_run`, `continue_on_error`, `batch_size` | Dry-run validates without writes; batch size controls throughput/memory tradeoff |
| Filtering and mapping | `include_tables`, `exclude_tables`, `column_mappings`, `table_mappings`, `type_overrides` | Restricts import scope and remaps schema |
| Safety and validation | `max_row_size_bytes`, `max_statement_size_bytes`, `enforce_utf8`, `validate_schema`, `schema_sample_rows` | Guards oversized input and invalid text / type drift |
| Resume and incremental | `checkpoint_file`, `delta_hash_file`, `delta_key_columns` | Enables restart-safe and delta-style runs |
| Conflict control | `conflict_strategy`, `conflict_key_columns`, `protected_fields`, `merge_depth` | Supports `OVERWRITE`, `SKIP`, `MERGE`, `ERROR` |
| Build-dependent features | `ARROW_ENABLED`; optional connector CMake conditions | Check current CMake feature and source registration; no single flag table guarantees all backends |
| Plugin ABI | `THEMIS_IMPORTER_PLUGIN_V1`, `THEMIS_IMPORTER_PLUGIN_ABI_V1` | Stable C ABI for third-party importers |

## Installation

Headers are part of the repository include tree. The following is an include
path example only; it is not a standalone external SDK/package guarantee:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

### Synchronous import with shared options

```cpp
#include "importers/importer_interface.h"

using namespace themis::importers;

ImportOptions options;
options.batch_size = 1000;
options.dry_run = false;
options.validate_schema = true;

// `importer` can be any IImporter implementation.
ImportStats stats = importer->importData(sourcePath, options);
```

### Wikipedia full import, delta update, verify, and export

```cpp
#include "importers/wikipedia_plugin.hpp"

using namespace themis::importers;

WikipediaIngestionPlugin plugin;
plugin.initialize(R"({"checkpoint_path":"./wikipedia.checkpoint.json"})");

ImportOptions options;
options.checkpoint_file = "./wikipedia.checkpoint.json";

plugin.runFullImport({.source_path = "./pages-articles.xml", .source_id = "full-2026-06"});
plugin.runIncrementalUpdate({.source_path = "./pages-articles-next.xml", .source_id = "delta-2026-07"}, options);
auto validation = plugin.validateDatabase();
auto manifest = plugin.exportPortable("./wikipedia.db", "./manifest.json");
```

### Stream rows without buffering full datasets

```cpp
auto stats = importer->importDataStreaming(sourcePath, options,
    [](const std::string& table, const nlohmann::json& row) {
        return true; // return false to abort early
    });
```

### HuggingFace legal ingest workflow (snapshot → update → validate → export)

```cpp
#include "importers/huggingface_ingest_plugin.h"

themis::importers::HuggingFaceIngestPlugin plugin;
plugin.init();
plugin.runFullImport({.dataset_name = "legal_hf_dataset", .split = "train", .seed_rows = rows});
plugin.runIncrementalUpdate({.dataset_name = "legal_hf_dataset", .split = "train", .changed_rows = delta_rows});
auto quality = plugin.validateQuality();
auto exported = plugin.exportAdaLoraJsonl({.output_path = "adalora_legal.jsonl"});
plugin.shutdown();
```

## Build/test evidence boundary

The checked-in `linux-release` and `community-release` presets disable tests by
default; community-release also disables benchmarks. See
[`../../src/importers/BUILD_STATUS.md`](../../src/importers/BUILD_STATUS.md) and
the current CMake files before selecting a configure/test workflow. The sample
build command above is not evidence that a build or test passed.

## Troubleshooting

- `FILE_NOT_FOUND` / `FILE_OPEN_FAILED`: source path invalid or unreadable
- `PERMISSION_DENIED`: permission callback denied `("import", "write")`
- `SCHEMA_VALIDATION_FAILED`: source values violate inferred schema types
- `CONFLICT_ERROR`: conflict strategy is `ERROR`; choose `SKIP`/`MERGE` if desired
- Plugin load failure: inspect `lastLoadError()` for missing symbol / ABI mismatch details

## Related Docs

- Implementation overview: [`../../src/importers/README.md`](../../src/importers/README.md)
- Architecture guide: [`../../src/importers/ARCHITECTURE.md`](../../src/importers/ARCHITECTURE.md)
- Module roadmap: [`../../src/importers/ROADMAP.md`](../../src/importers/ROADMAP.md)
- Future enhancements: [`../../src/importers/FUTURE_ENHANCEMENTS.md`](../../src/importers/FUTURE_ENHANCEMENTS.md)
- Plugin guide: [`../../docs/importers/plugin_guide.md`](../../docs/importers/plugin_guide.md)
- Troubleshooting: [`../../docs/troubleshooting/importers_troubleshooting.md`](../../docs/troubleshooting/importers_troubleshooting.md)
- Primary sources (EN): [`../../docs/en/importers/PRIMARY_SOURCES.md`](../../docs/en/importers/PRIMARY_SOURCES.md)
