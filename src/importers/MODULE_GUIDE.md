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
SOT_Domain: "module-behavior"
Canonical_References: "src/importers/README.md; include/importers/README.md; include/importers/importer_interface.h; include/importers/wikipedia_plugin.hpp; include/importers/huggingface_ingest_plugin.h; tests/importers/CMakeLists.txt; cmake/CMakeLists.txt"
Related_Reference: "makr-code/ThemisDB#7353"
---

> Exact model backend not verifiable from final document text alone.

# Importers Developer Module Guide

Use this guide to select and validate an import path. Read the
[architecture](./ARCHITECTURE.md) for boundaries and the
[API reference](./API.md) for shared options before relying on connector
behavior.

## Choose a path

- Use `IImporter` and a concrete connector for supported database, stream,
  object, or file sources.
- Use `FlatFileImporter` for CSV, TSV, JSONL/NDJSON. Parquet requires an
  `ARROW_ENABLED` build.
- Use `WikipediaIngestionPlugin` for Wikimedia XML full imports and later
  incremental updates with checkpoint, validation, and portable export.
- Use `HuggingFaceIngestPlugin` only with caller-provided seed/changed rows.
  Its current implementation is an in-process scaffold, not a live HuggingFace
  API client or persistent table writer.

## Recommended integration sequence

1. Confirm the connector is built and its source capability is available.
2. Initialize the importer with connector configuration and call
   `validateSource()`; report all returned validation errors to the caller.
3. Set `ImportOptions` explicitly: choose dry-run/schema validation, table
   filters and mappings, batch size, conflict behavior, row limits, and
   checkpoint/quarantine paths appropriate to the source.
4. Supply permission, metrics, and tracing callbacks where required by the
   host. Do not assume unset callbacks enforce access control or emit telemetry.
5. Run the import or streaming variant. Treat callback `false` as a request to
   stop, and observe cancellation behavior for the selected connector.
6. Inspect `ImportStats`, especially structured errors and the separate failed,
   skipped, and quarantined counters. Validate target state before declaring
   the migration complete.

For a file import, `FlatFileImporter::initialize()` accepts JSON configuration
for format, delimiter, quote character, header presence, and table name.
`AUTO` detects the format from the filename extension. Call
`getSourceSchema()` or enable schema validation when the mapping depends on
source types; sampling cannot guarantee later rows conform.

## Wikipedia pipeline lifecycle

Initialize the plugin with a checkpoint path, use a distinct source ID for the
dump being processed, and call `runFullImport()` or
`runIncrementalUpdate()`. After the import, call `validateDatabase()` before
`exportPortable()`. The export is a portable `wikipedia.db` JSON artifact plus
manifest and verification sidecars; it is not evidence that an external vector
index or live embedding provider was updated.

## Change and test workflow

Keep connector-specific parsing in its connector implementation and shared
policy in the shared interface/helpers. Add coverage for invalid source,
partial rows, schema drift, conflict disposition, timeout/cancellation, and
resume behavior as applicable. The module-local focused test registration and its current exclusions are
listed in [`tests/importers/CMakeLists.txt`](../../tests/importers/CMakeLists.txt)
and summarized in the [build/test status](./BUILD_STATUS.md). Registration is
not the same as a successful build or test run.

Related: [implementation](./IMPLEMENTATION.md),
[operations](./OPERATIONS.md), [runbook](./RUNBOOK.md).
