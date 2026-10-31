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
---

# LLM Wiki Plugin API

## Scope and canonical interface

The public C++ SDK is
[`include/llm_wiki/llm_wiki_plugin_interface.h`](../../include/llm_wiki/llm_wiki_plugin_interface.h).
It declares `ILLMWikiPlugin`, request options, result types, status, and plugin
factory symbol names. The interface is the API contract; the concrete
implementation in `src/llm_wiki/wikipedia/llm_wiki_plugin_impl.cpp` does not
currently fulfill every documented operation. Integrators must distinguish
declared contract from behavior verified in their build.

## Lifecycle and edition

The intended lifecycle is plugin factory/manager load, `initialize(config_json)`,
operations, then `shutdown()` before unloading. `initialize` returns `Status`
with `Ok`, `Error`, `PermissionDenied`, `InvalidArgument`, or
`NotInitialized` categories. A null C-string initialization argument returns
false in the implementation. A non-empty JSON configuration is needed to
reach the implementation's successful initialized state; an empty JSON
string returns `Ok` early, before setting that state.

Edition availability is determined by the
`THEMISDB_LLM_WIKI_ENTERPRISE_ENABLED` compile-time define in
`edition_gate.cpp`. In the current implementation, this resolves to
Enterprise when defined and Community otherwise; the enum values for Minimal,
Hyperscaler, and Military do not imply that runtime configuration detects
those editions. Known sub-feature checks follow the same enabled-edition
decision; the source does not perform an independent license-key validation.

## Operations

| Operation | Inputs and outputs | Contract and observed behavior |
|---|---|---|
| `ingest(path, options)` | Source path and `WikiIngestOptions`; returns counts, failed paths, and elapsed time | The interface describes directory/file ingestion and partial failures. Current implementation seeds a generic placeholder chunk for a matching path and delegates to workspace code; verify actual file parsing before relying on content ingestion. |
| `query(text, options)` | Query and `WikiQueryOptions`; returns candidates and guardrail counters | The interface describes scored retrieval. Current plugin implementation does not apply `top_k` or `min_score`; it returns in-memory chunks after a local unsafe-pattern screen. |
| `wikiInit(root)` | Workspace root; returns `Status` | Initializes the workspace orchestrator. Use an absolute, service-owned path and restrict filesystem permissions. |
| `wikiIngest(path, options)` | Source path and options; returns ingestion result | Delegates to `ingest`; see the implementation limitation above. |
| `wikiQuery(text, options)` | Query and options; returns query result | Delegates to `query`; current implementation behavior and option limitations apply. |
| `wikiLint(root, max_staleness_days)` | Workspace path and staleness threshold; returns categorized findings | Intended to report orphan, backlink, stale synthesis, unresolved-task, degraded, and re-anchor findings; confirm behavior in the concrete build. |
| `ingestWikipediaDump(path, options)` | Dump path and dump options; returns ingestion result | Declared as a Wikipedia pipeline operation with a feature gate. Current implementation ignores both arguments and returns fixed counts; do not use as proof of dump ingestion. |
| `stats(root)` | Optional workspace root; returns index/workspace stats | Returns empty stats when no workspace object exists; declared fields are not necessarily populated by all implementations. |

The request/response structures and all option defaults are defined in the
public header. Key option groups include file recursion/glob, chunk size and
overlap, embedding provider/model/dimension, skip-existing and workspace root;
query top-k, minimum score and optional saved page; and Wikipedia language,
batch size, article limit, projections, checkpoint and category filters.

## Thread-safety contract: unresolved implementation mismatch

The public header declares that all methods are thread-safe after successful
initialization and that concurrent `query()` calls are lock-free. That is the
declared contract, not the synchronization observed in this implementation:

- `initialize()`, `shutdown()`, `ingest()`, `query()`, and `wikiInit()` acquire
  an exclusive `unique_lock` on the plugin mutex. `query()` is therefore not
  lock-free, and concurrent query calls serialize. `wikiIngest()` and
  `wikiQuery()` delegate to the corresponding locked methods.
- `wikiLint()` and `stats()` do not acquire the plugin mutex before accessing
  `workspace_` and, for `stats()`, `workspace_root_`. The orchestrator's own
  mutex protects its individual `lint()`/`stats()` calls, but not the plugin's
  member access or lifecycle of the `workspace_` pointer.

Consequently, the header's blanket thread-safety claim is not verified by the
implementation. Until the API and implementation owners reconcile the
contract, callers should serialize plugin calls—including `wikiLint()` and
`stats()`—with each other and with initialization/shutdown. The public header
cannot be corrected within this documentation-only scope; track this as an
unresolved API/source contract issue.

## Factory contract

The public header declares plugin ID `llm_wiki`, factory symbol
`themisdb_llm_wiki_create`, and destroy symbol
`themisdb_llm_wiki_destroy`. The plugin manager owns the instance returned by
the factory and must use the matching destroy function. Confirm the runtime
library exports both names and matches the SDK ABI before deployment.

## Errors and edge cases

- Treat a non-`Ok` `Status` as a failed operation; the message may supply
  diagnostic context.
- Ingestion results may represent partial success; inspect `errors` and
  `failed_files`, not only `files_processed`.
- An empty query result can mean uninitialized plugin, policy reload error,
  unsafe query rejection, missing workspace, or no available chunks; the
  current return type does not encode these causes uniformly.
- Gate denial is an explicit permission status during initialization or
  through the edition gate. Do not interpret it as an empty successful result.
- Workspace filesystem and policy path failures should be diagnosed with
  [`RUNBOOK.md`](RUNBOOK.md).

## Related documents

- [`IMPLEMENTATION.md`](IMPLEMENTATION.md) — current differences from the SDK contract
- [`MODULE_GUIDE.md`](MODULE_GUIDE.md) — build and integration orientation
- [`TRUST.md`](TRUST.md) — trust assumptions for inputs, workspace, and providers

> Exact model backend not verifiable from final document text alone.
