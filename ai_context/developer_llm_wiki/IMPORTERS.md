---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "include/importers/importer_interface.h; include/importers/importers_api_contract.h; src/importers/README.md; src/importers/API.md; src/importers/IMPLEMENTATION.md; tests/importers/CMakeLists.txt"
---

> Exact model backend not verifiable from final document text alone.

# Importers — Developer LLM Wiki

This page is a curated orientation for coding and review work in the importer
module. It is secondary context: public headers define contracts, implementation
and tests establish connector behavior, and module documentation explains the
evidence. Do not infer that every connector implements every shared option.

## Module shape

Importers adapt relational, document, streaming, file, and object sources into
ThemisDB ingestion paths. Shared interfaces and import options live under
[`include/importers/`](../../include/importers/); connector implementations and
pipelines live under [`src/importers/`](../../src/importers/). The server uses
the importer interfaces to dispatch API jobs, and the `llm_wiki` plugin uses
the Wikipedia ingestion pipeline. These are downstream consumers, not owners
of importer connector internals.

Choose the contract matching the code being changed:

- [`importer_interface.h`](../../include/importers/importer_interface.h)
  defines `IImporter`, common options/results, connector-facing error types,
  streaming, cancellation, and source-schema surfaces.
- [`importers_api_contract.h`](../../include/importers/importers_api_contract.h)
  defines a separate versioned contract with ordering, commit visibility,
  idempotency, schema evolution, bad-row disposition, and retry classification.
  Its `ImporterErrorCode` is not the same enum as `ImportErrorCode` in the
  interface header.
- Plugin ABI and extended connector APIs are declared in
  [`importer_plugin_api.h`](../../include/importers/importer_plugin_api.h) and
  [`importer_interfaces.h`](../../include/importers/importer_interfaces.h).

## Contract invariants

The versioned API contract states that source row order is preserved and that
partial imported state is not visible before the governed commit. Replaying a
source with the same `import_id` is idempotent at the coordinator. Additive
nullable columns are compatible; breaking column or key changes require an
explicit migration. Bad-row policy is explicit: skip with an error summary or
fail the import. The contract classifies timeout and unavailable-connector
errors as retryable; this does not make every connector failure safe to replay.
The contract-hardening mock models an identical-source replay as successful
with no added rows and a different source under the same ID as
`IMPORT_DUPLICATE_ID`. No coordinator in `src/importers/` currently consumes the
versioned contract enum, so this test is not evidence of live connector
conformance.

The common interface also exposes an optional `permission_check` callback.
When supplied it is invoked for `("import", "write")`; omission does not provide
authorization. Check a concrete connector before relying on streaming,
checkpoint, quarantine, metrics, tracing, schema, or conflict options, because
support and persistence boundaries vary.

## Connector-specific boundaries

- **Flat files:** CSV, TSV, and JSONL/NDJSON are supported by the flat-file
  importer; Parquet depends on an `ARROW_ENABLED` build.
- **Wikipedia:** the pipeline reads Wikimedia XML, supports full and incremental
  processing, checkpointing, validation, and portable JSON export. The vector
  projection constructs records but does not demonstrate a provider inference
  call or vector-index write.
- **HuggingFace legal ingestion:** current fetch methods normalize caller-supplied
  rows into in-process state. They are not a live HuggingFace API client or a
  durable database writer. The resume token is in-memory, and the current
  embedding hook receives an empty vector.

Treat these as observed implementation limits, not promised future behavior.
For other connectors, inspect their implementation, capability checks, and
focused regression rather than extrapolating from the shared interface.

## Tests and evidence

Importer focused tests are discovered and registered in
[`tests/importers/CMakeLists.txt`](../../tests/importers/CMakeLists.txt). The
contract-hardening suite exercises ordering, idempotency, schema evolution,
bad-row handling, visibility, quotas, and retry classification. Its mocked
external interactions do not prove live connector I/O, cross-connector parity,
or production database atomicity. The registration file currently skips three
focused suites because of known test-only API/assertion drift; check its current
contents before selecting a test target.

For measured performance, use the matching benchmark output and build
configuration. Source-declared thresholds in
[`bench_importers_release_gates.cpp`](../../benchmarks/importers/bench_importers_release_gates.cpp)
are in-memory/mock gates, not end-to-end connector measurements.

## Operational and trust boundaries

Use the module [API guide](../../src/importers/API.md) and
[implementation guide](../../src/importers/IMPLEMENTATION.md) when changing
contracts or connector behavior. For operating a job, consult the
[operations guide](../../src/importers/OPERATIONS.md) and
[incident runbook](../../src/importers/RUNBOOK.md); for data and authorization
boundaries, consult the [trust model](../../src/importers/TRUST.md) and
[module security guide](../../src/importers/SECURITY.md).

At minimum, review source credentials, host-supplied authorization, row-size and
schema bounds, conflict policy, and access/retention for checkpoints, quarantine,
and diagnostic output. Do not treat cancellation as proof of rollback or an
in-memory checkpoint as durable recovery evidence.

## Source priority and freshness

The active module documentation entry point is
[`src/importers/README.md`](../../src/importers/README.md). Its API, research,
modeling, embedding, runbook, operations, and trust pages link to the relevant
headers, implementation files, and tests. If this summary conflicts with those
sources, follow the canonical contract and implementation evidence and correct
this page.

`WIKI_STATUS.json` records a `full_sync` metadata generation at
2026-10-31T11:12:38Z, and `SOURCE_MANIFEST.json` records this page as inventoried
at 11:07:02Z. These timestamps establish the freshness of that inventory
snapshot, not validation of the current working-tree content: this page has
changed since the manifest entry, and no subsequent importer-context/content
validation is claimed here.
