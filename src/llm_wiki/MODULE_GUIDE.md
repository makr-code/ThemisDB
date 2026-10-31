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

# LLM Wiki Module Guide

## Audience and boundaries

This guide is for engineers integrating or modifying the LLM Wiki module. The
public SDK is in `include/llm_wiki/`; implementation is under
`src/llm_wiki/`; focused module tests are under `tests/llm_wiki/`. The private
plugin implementation and its build packaging may differ from the core
sources in this checkout. See [`API.md`](API.md) before depending on an
operation's declared behavior.

## Find the right contract

1. Read [`ARCHITECTURE.md`](ARCHITECTURE.md) for stated module boundaries and
   [`README.md`](README.md) for module context.
2. Use `include/llm_wiki/llm_wiki_plugin_interface.h` for plugin option and
   response types.
3. Use `schema/llm_wiki_entity.schema.json` for persisted entity shape and
   `process/llm_wiki_process_policy.yaml` with its JSON Schema for process
   policy shape.
4. Trace behavior into the implementation before relying on an API comment:
   current ingestion, query, and Wikipedia dump code have material gaps
   relative to their declared contract.
5. Keep changes covered by the focused tests under `tests/llm_wiki/` and
   applicable LLM integration tests.

## Build and configuration orientation

The module-local CMake file defines the `themis_llm_wiki` static target and
conditionally links RocksDB when it finds a supported RocksDB target, an
explicit `THEMIS_ROCKSDB_TARGET`, or a recognized package/library result.
`THEMIS_USE_ROCKSDB` and `THEMISDB_WIKI_PHASE_B` are set by that conditional
path. A build without RocksDB does not provide the same persistence behavior;
check compile definitions and runtime configuration rather than assuming a
persistent backend is present.

The process policy is optional at the plugin implementation boundary. When a
policy path is supplied, loading resolves a schema path, parses YAML through
the configuration schema validator, then checks runtime invariants. Hot
reload is separately enabled by configuration. For the accepted fields and
non-tunable safety invariants, see
[`process/llm_wiki_process_policy.yaml`](process/llm_wiki_process_policy.yaml)
and [`process_policy_manager.cpp`](process_policy_manager.cpp).

Do not copy configuration examples from older docs without checking that the
current implementation consumes those keys. If provider credentials are
needed, inject them through the deployment's secret-management mechanism and
never persist them in wiki workspace content.

## Change checklist

- Identify whether the change is to the public SDK, internal implementation,
  entity schema, process policy, or persistence format.
- Preserve edition checks and policy invariants; fail closed on invalid
  configured policy.
- Do not rely on the public header's blanket thread-safety or lock-free-query
  statement: observed plugin locking differs, and `wikiLint()`/`stats()` do not
  lock plugin member access. Serialize calls externally and track the contract
  mismatch before changing concurrency behavior.
- Consider empty inputs, malformed policy, missing workspace, corrupt state,
  partial ingestion, and unavailable optional RocksDB.
- Add focused tests for the changed path and its negative/error cases.
- Update the relevant module docs in the same change; do not mark a roadmap
  target complete without the corresponding code and evidence.

## Related documents

- [`IMPLEMENTATION.md`](IMPLEMENTATION.md) — source map and behavior gaps
- [`API.md`](API.md) — public SDK contract
- [`RUNBOOK.md`](RUNBOOK.md) — diagnosis and recovery
- [`OPERATIONS.md`](OPERATIONS.md) — routine operational checks

> Exact model backend not verifiable from final document text alone.
