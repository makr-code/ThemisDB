---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-11-23"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

> Exact model backend not verifiable from final document text alone.

# Tensor Module API Contract

## Public interfaces in scope

This module exposes a small set of public tensor-facing contracts that are used by other parts of ThemisDB:

### `TensorIndexManager`

Location: `include/tensor/tensor_index_manager.h`

Responsibilities:

- choose a route for a tenant/collection/field using `routeFor()`
- create or retrieve index instances using `createIndex()` and `getIndex()`
- remove and list indexes through `dropIndex()`, `dropTenantIndexes()`, `listIndexes()`
- optionally pin TT-core pages using `mapCores()` and `ggmlCorePtrs()`
- persist indexes with `setDataDir()` and `flushAll()`

Contract notes:

- route decisions are `TENSOR_TRAIN`, `HYBRID`, or `HNSW`
- null returns are valid when no index is found or the route is unsupported
- configuration and cleanup semantics are explicit; no silent fallback is expected for malformed state
- **Implementation caveat:** `routeFor()` computes a route separately; current `createIndex()` constructs a `FlatTensorIndex` without consulting that route. This differs from the header comment describing `nullptr` for an HNSW route and remains an unresolved source/API contract discrepancy.
- `ggmlCorePtrs()` is deprecated; use the owning `mapCores()` API. Its cached
  bridge keeps pointers valid across index mutations, but cache eviction or
  manager destruction invalidates them.

### `TensorFingerprintGraph`

Location: `include/tensor/tensor_fingerprint_graph.h`

Responsibilities:

- register adapter fingerprints via `addAdapter()`
- remove them with `removeAdapter()`
- query candidate similarity with `findSimilar()` and `findSimilarByFingerprint()`
- inspect graph state via `entry()`, `size()`, `adapterKeys()`, and `stats()`

Contract notes:

- `findSimilar()` excludes the query key from the result set
- the request-aware `findSimilar(query_key, k, request, stop_reason)` overload
  polls while collecting candidates and between per-candidate similarity
  operations; it discards partial results and reports cancellation or deadline
  expiry through `stop_reason`
- each TT inner-product and injected exact-similarity callback remains
  synchronous and checkpoint-only, as does result sorting, so one such operation
  can overrun a deadline
- `findSimilarByFingerprint()` returns results sorted by descending similarity
- invalid magnitude, invalid scores, or missing candidate trains result in empty output with diagnostics rather than silent bad values

### `AdapterRepository`

Location: `include/tensor/adapter_repository.h`

Responsibilities:

- persist TT trains for a domain / base-model pair
- load adapter descriptors through `loadAdapter()`
- list domains and adapter keys
- optionally hook a `TensorFingerprintGraph` with `setFingerprintGraph()`

Contract notes:

- storage is tenant-scoped and keyed by domain + base-model metadata
- the repository is responsible for adapter provenance and retrieval orchestration
- missing or malformed adapter state is treated as a fail-safe condition

### `TensorMmapBridge`

Location: `include/tensor/tensor_mmap_bridge.h`

Responsibilities:

- represent pinned or mmap-backed TT-core views
- expose RAII ownership of mapped memory blocks
- keep pointer validity aligned with the lifetime of the bridge object

Contract notes:

- bridge lifetime must be kept alive as long as mapped slices are used
- memory resets and munmap behavior are part of the object contract
- the manager's `mapCores()` path currently builds from an in-memory TT train; the bridge uses anonymous mapping plus a copy unless an SST mapper is injected. `buildFromFd()` separately supports a file-backed `MAP_SHARED` path where available.

### `DreamOrchestrator`

Location: `include/tensor/dream_orchestrator.h`

Responsibilities:

- create research-mode runs and retain their descriptors by `run_id`
- aggregate completed evaluation, ethics, isolation, provenance, and security evidence
- retain and serialize the unified report with all five `BRAIN-DREAM-*` gate statuses

Contract notes:

- `DreamGateStatus` is the shared `pass|hold|fail` vocabulary. Missing evidence is
  `hold`; non-finite metrics and detected regressions fail closed.
- Ethics vetoes fail `BRAIN-DREAM-ETHICS-BOUNDARY`; unresolved security vetoes
  hold, and confirmed leakage fails the security outcome and isolation gate.
- Utility requires finite Recall@10 and nDCG@10 deltas, neither below baseline,
  and a positive gain in at least one. Positive hallucination or error-rate
  deltas fail nonregression.
- Reports conform to `src/tensor/DREAM_MODE_GATE_REPORT_SCHEMA.json` and are
  queryable by run ID for the orchestrator instance lifetime; they are not
  durable across process restarts. Callers must supply evidence from the
  evaluation, ethics, and security checks.
- Lifecycle transitions to R1/R2/R3 require all five gates and security to
  pass. Reaching R3 records lifecycle state only; it does not authorize or
  execute promotion transfer. Callers that previously transitioned without
  validation evidence must call `validateRun()` first and handle a rejected
  transition.

## Error and edge-case contract

The module strongly prefers explicit failure to silent recovery. In API terms that means:

- `addAdapter()` returns `false` for empty or malformed TT cores.
- `ITensorIndex::add()` rejects malformed TT shape/rank metadata and mode sizes
  that differ from existing entries; `search()` returns no results for malformed
  queries or query modes incompatible with the index. `.ttidx` loading rejects
  mode/core count or dimension mismatches, including mixed shapes across entries,
  and leaves existing contents intact.
  Persisted files with incomplete or inconsistent mode metadata must be rebuilt
  from their source vectors.
- invalid exact-similarity math is rejected/skipped with diagnostics on the relevant `findSimilar()` paths; empty fingerprint input and `k == 0` return empty results directly.
- missing index or adapter is represented by `nullptr`, `false`, or an empty result according to the called API.
- data persistence and cleanup must remain paired

This is the public behavior contract for the tensor module, and it is the contract that callers should rely on when integrating with higher-level retrieval flows.
