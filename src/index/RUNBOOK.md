---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: level1
SOT_Domain: module-behavior
Canonical_Sources:
  - "src/index/ROADMAP.md"
  - "docs/operability/RUNBOOK_INDEX_ENGINE.md"
  - "include/index/ann_frontdoor.h"
  - "include/index/vector_index.h"
  - "include/index/gpu_vector_index.h"
  - "include/index/index_error_codes.h"
  - "src/index/ann_frontdoor.cpp"
  - "src/index/gpu_vector_index.cpp"
Related: "Issue #7125; DOC-WEEKLY-2026-44"
---

Exact model backend not verifiable from final document text alone.

# Index module incident runbook

## Purpose and reader

This runbook gives index operators and on-call engineers a source-grounded
triage path for retrieval degradation, backend failure, and index lifecycle
problems. It deliberately avoids assuming a particular admin CLI, service
layout, metrics exporter, or shell path. Use the deployment's approved
control-plane procedures for actions that mutate persistent state.

## Safety first

1. Record the affected index/scope, time window, deployment/build, configured
   backend, and whether results are absent, partial, or merely slower.
2. Preserve authoritative stored entities and available backups. Do not
   delete/recreate an index or start overlapping rebuilds as a first response.
3. Check `Status`/`Result` errors and `AnnFrontdoorResult` fields, including
   `strategy_used`, `fallback_mode`, `fallback_reason_code`,
   `partial_results`, and shard attempt/success/failure counts.
4. Escalate when source data, storage health, or rollback safety is uncertain.
   The manifest's rollback flags are metadata; they do not perform a rollback.

## Triage by symptom

### Empty or invalid ANN results

- Compare the requested scope and `AnnQueryContext` with registered backend
  scopes and the plan returned by `planRetrieval()`.
- Check query pointer/dimension at the caller; `search()` throws
  `std::invalid_argument` for a null pointer or zero dimension. Confirm the
  actual backend's expected dimension.
- Inspect fallback reason and route metadata. The current frontdoor
  brute-force helper returns an empty set; do not treat a flat fallback route
  label as proof that a storage scan completed.
- Check returned distances and candidate count. The frontdoor truncates
  overlarge results and removes NaN/negative values, but does not prove
  completeness.

### GPU backend unavailable or failed

- Read the configured `GPUVectorIndex::Backend`, build flags, active backend,
  and `Statistics::isGPUActive`. Confirm device/runtime health using the
  deployment's approved GPU diagnostics.
- With `allowCPUFallback=true`, backend initialization/runtime failure may
  switch to CPU. With it false, initialization fails closed when the requested
  backend is unavailable. Record which behavior was configured before
  changing it.
- If degraded CPU operation is not approved for the workload, stop routing new
  requests to that index path and escalate to the GPU/index owners. Do not claim
  GPU parity from CPU-only tests; representative hardware parity remains
  pending.

### Rebuild or update failure

- Check each returned `VectorIndexManager::Status` and
  `IncrementalReindexStats`; distinguish a failed operation from successful
  reconciliation or a full rebuild trigger.
- Confirm RocksDB/source records are available and preserve them before
  attempting `rebuildFromStorage()`.
- Avoid repeated rebuild attempts while storage errors, resource exhaustion,
  or a concurrent lifecycle operation remain unresolved.
- After recovery, compare index count/statistics and representative query
  results against the expected data. Use the module's focused tests in a
  controlled build to separate code regression from environment failure.

### Distributed query is partial or unavailable

- Inspect `shards_attempted`, `shards_succeeded`, `shards_failed`,
  `partial_results`, and the distributed fallback reason.
- Check shard registration and backend health. Retries are bounded by
  `distributed_retry_attempts`; partial-result behavior is controlled by
  `distributed_allow_partial_results`.
- Consumers must decide whether partial candidates are acceptable. If not,
  preserve fail-closed behavior and escalate rather than silently treating
  partial results as complete.

## Signals and evidence

The source exposes `ann_frontdoor_route_type` counter labels for route type,
fallback mode, and distributed status. GPU index statistics expose active
backend, query timing/throughput fields, and optional oversubscription counters.
Structured error categories are defined in
[`index_error_codes.h`](../../include/index/index_error_codes.h).

The existing downstream
[`RUNBOOK_INDEX_ENGINE.md`](../../docs/operability/RUNBOOK_INDEX_ENGINE.md)
lists additional alert names, trace identifiers, and example commands. Those
are not all verified as emitted/implemented by the source inspected for this
document. Before alerting on them or executing a listed command, confirm that
the deployment exports the signal and that the command/configuration exists
for its actual release.

## Recovery and rollback boundary

Use the storage-owning team's documented backup/restore process before any
destructive repair. The index API exposes rebuild, save/load, and reindex
operations, but choosing the correct recovery point is deployment-specific.
`IndexManifestV1::can_rollback_to_previous()` only checks history and two
metadata flags; it does not atomically swap index data. Do not declare rollback
safe until the previous data is actually accessible and the owning lifecycle
implementation has been verified.

## Escalation and validation

Escalate persistent backend errors, suspected index corruption, repeated
resource exhaustion, unaccounted partial results, and any uncertainty about
data loss or tenant isolation to the index and storage owners. Use
[`test_ann_frontdoor.cpp`](../../tests/index/test_ann_frontdoor.cpp),
[`test_index_maintenance.cpp`](../../tests/index/test_index_maintenance.cpp),
and [`test_gpu_vector_index_llm_paths_focused.cpp`](../../tests/index/test_gpu_vector_index_llm_paths_focused.cpp)
for focused regression evidence; a passing unit test is not a recovery
validation for production data.
