---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Ingestion Implementation

## Purpose and scope

This guide follows the runtime path from source configuration to a reported
ingestion outcome. It is for maintainers changing connector orchestration,
workflow execution, validation, or persistence handoff. The implementation is
owned under this directory; public contracts are under
[`include/ingestion/`](../../include/ingestion/).

## Runtime path

1. `IngestionBuilder` collects database connection, sources, target collection,
   retry/rate-limit settings, parallelism, and optional schemas before building
   an `IngestionManager`.
2. `IngestionManager` registers and selects source connectors. A source must be
   enabled and its connector must initialize/be available; otherwise the run
   returns structured errors.
3. Connector implementations retrieve data, parse/extract supported content,
   apply configured schema validation, and report per-source statistics.
4. Filesystem sources can use a configured `WorkflowEngine`. The manager builds
   an `ExtractionContext` and either selects a profile by MIME type and filename
   or executes the explicitly named `workflow_profile`. A workflow error
   increments failures and is reported as `PROCESSING_FAILED`.
5. Workflow steps operate on the shared extraction context. The engine returns
an assembled entity set, but the inspected manager branch only increments
processed/byte counters on successful execution; it does not hand that result
to a sink. Do not interpret that branch's processed count as persistence.
`IngestionReport` aggregates totals, per-source stats, quarantine entries,
elapsed time, and dry-run state.

Workflow profiles carry ordered `StepConfig` entries. The engine executes those
steps against the same context and returns an assembled `BaseEntitySet`; when no
profile matches, it uses the loaded `default` profile or returns
`ERR_WORKFLOW_NO_MATCHING_PROFILE`. Profile registration order matters: the
first matching profile is selected.

## State and invariants

- An acknowledgement must not precede schema validation and the configured
  durability point described by
  [`ingestion_api_contract.h`](../../include/ingestion/ingestion_api_contract.h).
  Verify that the concrete persistence path satisfies this contract; the header
  is not evidence that every connector writes through the same WAL/quorum path.
- A configured checkpoint directory must already exist. Incremental mode uses
  stored progress to skip already processed offsets; test resume behavior with
  the actual connector, since source ordering and offset semantics vary.
- Quarantine is per failed item and retry attempts are bounded by
  `RetryConfig::max_quarantine_retries`. Permanently failed entries are excluded
  from subsequent retries.
- Dry-run scans and counts documents without writing to the target collection.
- Lineage tracking is disabled by default and stores records in an in-memory
  store; it is not a durable audit database.
- Connector callbacks and sink interfaces have their own threading guarantees;
  do not assume all module work executes on one universal worker thread.

## Error handling and edge cases

Connectors produce structured `IngestionError` values with severity, code,
source ID, message, and details. A report may contain partial success: inspect
both `documents_processed` and `documents_failed`, then review `errors` and
`quarantine`. Retry classification is component-specific. The manager,
`ingestion_api_contract.h`, and `ingestion_error_contract.h` each declare an
`IngestionErrorCode` in the same namespace, with differing declarations. They
cannot be included together in one translation unit without a redeclaration
conflict; treat this as an unresolved source-contract inconsistency, not as
interchangeable numeric code sets.

Schema field rules only apply to JSON-parsable documents. A format parser or
workflow can fail after source retrieval; such an error is not equivalent to a
schema rejection. For optional inference steps, an unavailable backend may
produce a warning and skip the step according to its configuration.

## Change and validation path

- Keep connector-specific behavior in its connector implementation; shared
  lifecycle belongs in `ingestion_manager.cpp`.
- Preserve source IDs, run correlation IDs, and chunk/entity provenance when
  changing transformations.
- Add regression coverage under [`tests/ingestion/`](../../tests/ingestion/);
  use integration/soak fixtures for cross-module or sustained-load behavior.
- Update [`ARCHITECTURE.md`](./ARCHITECTURE.md), [`API.md`](./API.md), and the
  applicable security or operator documentation when contracts change.

See [`MODULE_GUIDE.md`](./MODULE_GUIDE.md) for the test/build entry points.

> Exact model backend not verifiable from final document text alone.
