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

# Ingestion API Contract

## Purpose and scope

This page is for C++ clients and module developers integrating the public
ingestion interfaces. The declarations in
[`include/ingestion/`](../../include/ingestion/) define the API; internal
implementation details in `src/ingestion/` may change independently.

## Main entry points

| Interface | Header | Contract |
|---|---|---|
| `IngestionBuilder` | [`ingestion_manager.h`](../../include/ingestion/ingestion_manager.h) | Configure sources and manager options, then call `build()`. |
| `IngestionManager` | same | Register/reconfigure sources, run `ingestSource()` or `ingestAll()`, configure validation, retries, rate limits, dry-run, checkpoints, and workflow/backend injection. |
| `ISourceConnector` | same | Initialize, report availability/count, ingest to a collection, and optionally accept a document validator. |
| `IngestionAdminApi` | same | In-process source status/control and quarantine management; it does not itself provide an HTTP server or authorization layer. |
| `WorkflowEngine` | [`workflow_engine.h`](../../include/ingestion/workflow_engine.h) | Load profiles and execute ordered steps against `ExtractionContext`. |
| `IngestionCoordinator` | [`ingestion_coordinator.h`](../../include/ingestion/ingestion_coordinator.h) | Assign source work to worker nodes and aggregate partial reports. |
| Sink interfaces | [`ingestion_sinks.h`](../../include/ingestion/ingestion_sinks.h) | Write graph entities/relations, vectors, or documents through injected adapters. |

## Inputs, outputs, and defaults

`SourceConfig` identifies a source with `source_id`, `SourceType`, `location`,
priority, enabled state, and connector-specific string options. The builder's
target collection defaults to `legal_documents`; the manager can change it with
`setTargetCollection()`. Retry defaults are three attempts, 500 ms initial
delay, 2x backoff, 30 s maximum delay and request timeout, and five quarantine
retries. `RateLimitConfig` is disabled by default; zero request/byte limits mean
unlimited. These defaults are API defaults, not a production capacity profile.

`ingestSource()` returns `IngestionStats`; `ingestAll()` returns
`IngestionReport`. Check per-source processed/failed counts, structured errors,
correlation ID, timing, and quarantine contents. A report can represent partial
success and should not be treated as an all-or-nothing transaction across
sources.

For schema validation, `SchemaConfig` may enforce content length, a content
pattern, and JSON field constraints. Non-JSON content skips field-level checks.
With `reject_invalid=true`, invalid items count as failures and are eligible for
quarantine. Dry-run mode suppresses writes. `previewSource()` returns up to 100
sample documents and does not write them; protect preview results as source
content.

## Durability, errors, and compatibility

[`ingestion_api_contract.h`](../../include/ingestion/ingestion_api_contract.h)
declares the frozen v1.x write-path requirements: validate before
acknowledgement, report buffer-full explicitly, and make batch commits atomic.
Concrete caller paths must be checked against the selected persistence backend;
do not infer a quorum or WAL guarantee solely from the API declaration.

`ingestion_manager.h`, `ingestion_api_contract.h`, and
`ingestion_error_contract.h` each declare `themis::ingestion::IngestionErrorCode`.
The declarations differ and cannot coexist in one translation unit without a
redeclaration conflict. This is a source-contract inconsistency, not three
interchangeable error-code sets; inspect the include graph and use only the
header required by the current interface until the declarations are reconciled.
Preserve the error context and source ID when translating errors.

## Integration constraints

- Authenticate and authorize callers before exposing admin methods. The
  in-process `IngestionAdminApi` is a control wrapper, not an HTTP auth system.
- Keep secrets out of source option dumps; use `sanitizeOptions()` when logging
  options (see [`TRUST.md`](./TRUST.md)).
- Verify that the chosen path actually invokes a sink before treating a
  successful report as persisted data. The filesystem workflow manager path
  currently records successful processing without handing the result to a sink;
  see [`IMPLEMENTATION.md`](./IMPLEMENTATION.md).
- Set finite connector limits where the relevant connector uses `0` as unlimited
  (`max_pages`, `max_messages`, or `max_keys`).
- For shared or multi-host resume behavior, inject durable shared checkpoint and
  leader-election implementations; the in-process defaults do not coordinate
  separate processes.

## Minimal usage

```cpp
using namespace themis::ingestion;

auto manager = IngestionBuilder("rocksdb://./data")
    .withFilesystemSource("manuals", "/srv/manuals",
                          {{"extensions", ".txt,.md"}, {"recursive", "true"}})
    .withRetryConfig({.max_attempts = 3, .initial_delay_ms = 500.0,
                      .backoff_factor = 2.0})
    .withTargetCollection("manual_documents")
    .build();

const auto report = manager->ingestAll();
```

The example follows the builder interface in `ingestion_manager.h`; deployment
configuration, authorization, checkpoint policy, and sink wiring remain the
caller's responsibility.

> Exact model backend not verifiable from final document text alone.
