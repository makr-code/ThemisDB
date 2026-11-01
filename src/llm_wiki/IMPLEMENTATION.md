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

# LLM Wiki Implementation Notes

## Purpose

This is a source-oriented map for developers changing `llm_wiki`. It describes
the implementation currently present in this checkout, not the fuller
behavior described by older design prose or interface comments.

## Component map

| Area | Source | Responsibility |
|---|---|---|
| Plugin SDK | [`../../include/llm_wiki/llm_wiki_plugin_interface.h`](../../include/llm_wiki/llm_wiki_plugin_interface.h) | Request/response types and `ILLMWikiPlugin` contract |
| Edition decision | `edition_gate.h`, `edition_gate.cpp` | Edition detection and operation/sub-feature denial |
| Query pattern screening | `guardrail_patterns.h`; plugin-local matching in `wikipedia/llm_wiki_plugin_impl.cpp` | Case-folded substring checks for configured patterns |
| Workspace persistence | [`../../include/llm_wiki/workspace_state_manager.h`](../../include/llm_wiki/workspace_state_manager.h), `workspace_state_manager.cpp` | JSON file persistence, validation, and log recovery |
| Process policy | [`../../include/llm_wiki/process_policy_manager.h`](../../include/llm_wiki/process_policy_manager.h), `process_policy_manager.cpp` | YAML parse, schema lookup, and semantic invariant checks |
| Knowledge-sphere artifacts | `schema/knowledge_spheres.schema.json`, `process/knowledge_spheres_policy.yaml`, `runtime/knowledge_spheres.runtime.json` | Static design/configuration files; no runtime loader or registry use is present in the plugin path |
| Optional page store | [`../../include/llm_wiki/rocksdb_wiki_store.h`](../../include/llm_wiki/rocksdb_wiki_store.h), `rocksdb_wiki_store.cpp` | RocksDB key/value operations when `THEMIS_USE_ROCKSDB` is defined |
| Build target | `CMakeLists.txt` | `themis_llm_wiki` static library and optional RocksDB linkage |

The public plugin interface uses namespace
`themis::plugins::llm_wiki`; workspace and policy implementation headers also
contain types in `themis::llm_wiki`. Check the owning header before choosing a
type or namespace.

## Runtime and validation path

`LLMWikiPluginImpl::initialize` first checks the edition gate, parses JSON
configuration, and loads a process policy only when a policy path is
configured. A configured policy is schema-validated and then checked for
runtime invariants: `prompt_engineering` owns planning, a second planner is
forbidden, validation and policy snapshots/reason codes remain enabled, and
adjustable knobs must have valid bounds. Hot reload is opt-in and restores the
previous in-memory policy when a candidate reload fails.

The configuration keys read by the implementation include embedding provider
and dimension, splitter limits, workspace root, `fail_open`, Wikipedia
feature flag, JSON index path, process policy path, and hot-reload flag.
Validate changes against both the declared interface and implementation;
these surfaces are not fully synchronized.

## Important implementation limits

The current `LLMWikiPluginImpl` path must not be mistaken for the broader SDK
contract:

- `ingest()` creates a generic placeholder chunk for a matching path before
  handing the input to the workspace orchestrator; the core path shown here
  does not parse the source file contents into chunks.
- `query()` does not use `WikiQueryOptions` to apply `top_k` or `min_score`;
  it returns the in-memory chunk collection after its local unsafe-pattern
  filter.
- `ingestWikipediaDump()` currently ignores its arguments and returns fixed
  counts. It is not evidence that dump ingestion is operational in this
  implementation.
- **Synchronization mismatch:** the public header promises thread-safe methods
  and lock-free concurrent queries, but plugin `query()` takes an exclusive
  `unique_lock`. `wikiLint()` and `stats()` do not lock plugin member access;
  the workspace orchestrator's internal lock does not protect the plugin's
  pointer/root fields. Serialize plugin calls externally until the source/API
  contract is resolved; see [`API.md`](API.md).
- `guardrail_patterns.h` and the plugin's local `containsUnsafePattern`
  helper are separate matching implementations. Do not assume the former's
  complete pattern set is applied by the latter without tracing the call path.
- `WorkspaceStateManager` calls its checksum SHA-256, but its implementation
  uses a 64-bit FNV-1a hash formatted with a `sha256:` prefix. Treat that as a
  known integrity limitation.

These are documentation of source behavior and integration risks, not a
recommendation to expand or rely on fallback behavior. See
[`MODULE_GUIDE.md`](MODULE_GUIDE.md) for contributor workflow and
[`TRUST.md`](TRUST.md) for trust consequences.

## Tests and changes

Focused source coverage is under [`../../tests/llm_wiki/`](../../tests/llm_wiki/)
and wiki integration/provenance tests under [`../../tests/llm/`](../../tests/llm/).
The local state manager tests use temporary directories and cover successful
round trips, missing/corrupt state, checksum failure, and recovery. Update
focused tests for behavior changes; do not treat an integration or soak test
as a substitute for an edge-case assertion.

## Related documents

- [`API.md`](API.md) — public call contracts and implementation deltas
- [`RUNBOOK.md`](RUNBOOK.md) — failures and recovery actions
- [`ROADMAP.md`](ROADMAP.md) — planned work and status (not implementation proof)

> Exact model backend not verifiable from final document text alone.
