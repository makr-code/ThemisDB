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
Canonical_References: "include/importers/importer_interface.h; include/importers/importers_api_contract.h; src/importers/flatfile_importer.cpp; src/importers/wikipedia_pipeline.cpp; src/importers/huggingface_ingest_plugin.cpp; tests/importers/CMakeLists.txt"
Related_Reference: "makr-code/ThemisDB#7353"
---

> Exact model backend not verifiable from final document text alone.

# Importers Implementation Guide

This guide is for maintainers changing connector or shared import behavior.
Internal implementation is under `src/importers/`; public contracts are under
[`include/importers/`](../../include/importers/). Connector feature availability
depends on the build and runtime configuration.

## Execution stages

1. Initialize the selected importer or plugin from its connector configuration.
2. Validate the source and reject unsupported formats or unavailable
   capabilities before processing records.
3. Read source rows, optionally infer/sample a schema, then map and validate
   records.
4. Apply configured filtering, ID handling, conflict behavior, and optional
   row callbacks.
5. Return `ImportStats` with record/conflict counters, warnings, and structured
   errors. For streaming imports, the row callback may stop processing by
   returning `false`.

This is a conceptual shared path. SQL dump parsing, live database connectors,
Kafka/S3 paths, Wikipedia dumps, and the HuggingFace plugin have different
source and persistence mechanisms; do not assume one stage has identical
transaction or resume behavior in every implementation.

## Shared invariants

The module contract requires source ordering to be preserved, partial state to
remain invisible until the governed commit, and replay of the same source with
the same `import_id` to succeed without adding rows at the coordinator. Reusing
the ID for a different source is rejected as `IMPORT_DUPLICATE_ID`. Additive
nullable columns are compatible;
breaking schema changes require an explicit migration. The bad-row
disposition controls whether a row is skipped or the import fails. These
requirements are specified in
[`importers_api_contract.h`](../../include/importers/importers_api_contract.h);
maintainers should add connector-level evidence when changing an implementation.

## Optional paths and feature boundaries

- `ARROW_ENABLED` gates Parquet handling in `FlatFileImporter`.
- Kafka, S3, and CDC live paths depend on the corresponding build/runtime
  features; capability checks and deterministic unsupported outcomes must be
  preserved.
- `ImportOptions` offers streaming, checkpoint, quarantine, metrics, tracing,
  permissions, and schema/conflict controls. Verify support at the target
  connector instead of assuming every option is consumed.
- Wikipedia uses a resumable XML pipeline and derived projections; its
  portable export is a JSON artifact with manifest and verification sidecars.
- HuggingFace ingestion currently consumes caller-provided rows and stores
  normalized state in in-process containers. `fetchRawRows()` is explicitly
  documented in source as a non-production adapter scaffold, not a live
  HuggingFace fetch path.

## Failure handling

Preserve structured error codes and record locations when mapping low-level
failures. Respect `continue_on_error`, bad-row disposition, quota and row-size
limits, and cancellation at the connector boundary. A fallback is valid only
when the capability contract names it and its observable performance or
semantic delta is reported. Do not silently downgrade validation, conflict
policy, or authorization.

## Focused validation

Focused C++ tests in the module-local registration file are discovered from
[`tests/importers/CMakeLists.txt`](../../tests/importers/CMakeLists.txt) as
`module_importers_<stem>_focused` targets and `<stem>_importers_FocusedTests`
CTest cases. That file currently discovers 12 `test_*.cpp` files and skips
three named suites before target creation. The contract hardening suite is
self-contained and mocks the coordinator: identical-source replay succeeds
without additional rows, while a different source under the same ID is rejected.
This tests the declared contract only. The versioned contract enum is not used
by an import coordinator in `src/importers/`, so the test is not evidence that
live connectors implement coordinator-level replay semantics. Connector-specific
changes also need matching regressions; mocked tests do not prove live I/O or
database atomicity.

References: [API contracts](./API.md), [module guide](./MODULE_GUIDE.md),
[research evidence](./RESEARCH.md), [roadmap](./ROADMAP.md).
