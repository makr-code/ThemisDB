---
Author: "ThemisDB Team"
Created: "2026-10-21"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "unknown"
---

Exact model backend not verifiable from final document text alone.

# Production Requirements — Vector Search

## Purpose and boundary

These requirements describe evidence needed to approve a vector-search
deployment. The runtime is owned by `VectorIndexManager` under the shared
`index` subsystem. This document and the module directory do not grant
production approval.

## Runtime contract

- Initialize the manager with a non-empty namespace and positive vector
  dimension; operation status must be checked by the caller.
- Use a query dimension consistent with the initialized index. Dimension
  mismatch is returned as a failed `Status`.
- The direct-commit `addEntity()` overload currently does not check vector
  length; callers requiring dimension safety must validate before using it. The
  `WriteBatchWrapper` and MVCC overloads perform that dimension check.
- Treat entity field, RocksDB, persistence, and optional encryption failures as
  operation failures; do not report success when the manager returns an error.
- Determine optional HNSW/advanced backend availability from the build and
  runtime configuration. Do not assume a backend from the module name.
- Use the explicit save/load and rebuild/reindex operations where the
  deployment requires those lifecycle behaviors.

The public API uses `VectorIndexManager::Status`; the former E5400-series error
codes in this module's older documentation are not part of the verified API
contract.

The source does not establish NaN/Inf rejection. Do not claim finite-value
validation until it is verified in code and covered by tests.

## Test and release evidence

- Run the focused vector-search CTest selection from a configured,
  repository-supported preset.
- Include runtime-backed tests for the deployment's chosen backend and
  persistence configuration.
- Keep the flat-index high-cardinality and soak fixtures distinct from
  production runtime evidence.
- Validate actual performance measurements against
  [documented thresholds](PERFORMANCE_EXPECTATIONS.md); static benchmark
  counters do not satisfy this requirement.
- Obtain maintainer sign-off before making release-readiness claims.

## Operational and recovery requirements

- Document the namespace, dimension, metric, backend, and persistence settings
  used in deployment.
- Monitor operation failures and storage/persistence errors through the
  caller's operational instrumentation.
- Test save/load and rebuild recovery using the deployed configuration before
  relying on persisted index state.
- Do not treat the in-process `DistributedVectorIndex` as durable, replicated,
  or high-availability infrastructure.

## Status

The repository artifacts cited by this module do not provide sufficient
runtime-backed benchmark and deployment evidence to assert that all
production requirements have been met.
