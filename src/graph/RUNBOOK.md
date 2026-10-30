> Author: ThemisDB Team
> Created: 2026-10-30
> Last Updated: 2026-10-30
> Status: active
> AI_Used: true
> AI_Models: unknown
> AI_Contributor: GitHub Copilot
> Human_Review: false
> Human_Author: ThemisDB Team
> Source level: level1
> SOT domain: module-behavior
> Canonical source references: [graph error taxonomy](../../include/graph/graph_error_taxonomy.h), [graph security guidance](SECURITY.md), [focused tests](../../tests/graph/)
> Related issue: #7122
>
> Exact model backend not verifiable from final document text alone.

# Graph incident runbook

This runbook covers diagnosis at the graph module boundary. It does not prescribe service start/stop commands, database recovery commands, or deployment procedures; those are owned by the server, storage, and deployment runbooks.

## First response

1. Record the operation, graph/query identifier if available, timestamp, input bounds, algorithm/acceleration route, and whether the caller received a complete, truncated, partial, or failed result.
2. Capture the graph error code/category and diagnostic context. `ErrorContext` supports component, operation, context, and recovery hint; see [graph_error_taxonomy.h](../../include/graph/graph_error_taxonomy.h).
3. Where the server integration is available and access is permitted, inspect `GET /api/v1/graph/metrics` or `GET /api/v1/graph/metrics/prometheus` for query counters, timeouts, execution totals, and cache counts. The routes are implemented by [http_server.cpp](../../src/server/http_server.cpp) and the handlers by [graph_api_handler.cpp](../../src/server/graph_api_handler.cpp). The handler stores an `AuthMiddleware`, but no handler method calls it; verify endpoint authorization at the server/router boundary.
4. Preserve relevant server, storage, and shard logs before retries. A retry can alter cache state or produce a different partial outcome.
5. Do not treat query constraints as access-control enforcement. Route suspected cross-tenant exposure to the security incident process in [root SECURITY.md](../../SECURITY.md), and verify authorization at the server/router boundary.

## Symptom-driven diagnosis and recovery

| Symptom | Inspect | Safe next action |
|---|---|---|
| Invalid source/target, depth, or conflicting constraints | Caller inputs and `QueryConstraints`; distributed validation in [distributed_graph.cpp](distributed_graph.cpp); `PathConstraints` values | Correct the invalid input or conflicting required/forbidden vertices. Re-run only after the caller validates the intended bounds. |
| Constraint appears to have no effect | The caller's `PathConstraints` construction and `getConstraints()`; invalid identifiers/property values are rejected by early return in [path_constraints.cpp](path_constraints.cpp) | Validate IDs/field names before calling void mutators; add a focused regression. Do not assume a rejected constraint was installed. |
| Traversal timeout, frontier overflow, or unexpectedly large output | Requested depth/result cap, observed node/edge counts and truncation fields, graph fan-out | Reduce caller-supplied bounds or narrow filters; reproduce with the focused traversal test. Do not increase limits blindly. |
| GPU route reports fallback or does not use GPU | `GPUGraphTraversal::getStats()`, result `used_cpu_fallback`/`truncated`, device availability and caller configuration | Treat the returned CPU fallback as the safe execution path. Investigate backend availability separately; do not retry indefinitely or claim GPU execution. |
| Distributed query returns partial results or fails | `ShardGraphExecutor::isHealthy()`, registered shard IDs, shard execution errors, `graph_exact_traversal_errors_total` operation/reason labels | Restore/repair shard health through the shard owner. Retry only when the caller can tolerate duplicate work and partial results are not being presented as complete. Escalate persistent shard or merge failures to distributed systems owners. |
| Ontology or reasoning result is unexpected | Ontology load/build status, source concepts/axioms, rule IDs, base facts, inference depth, `explain()`/inference premises | Correct the ontology/rules or resolve the contradiction with the domain owner. Do not automatically convert a reasoning conflict into a graph fact. |
| RotatE candidate list is empty or scoring throws | `isTrained()`, registered entity/relation IDs, training result and input triples | Train with registered IDs and check `RotatETrainResult::success`; handle pre-training empty vectors and exceptions explicitly. Treat rankings as candidate suggestions. |
| Tensor similarity or restore differs from expectation | Fingerprint configuration, candidate cap, external train loader, imported snapshot/journal state and storage outcome | Validate the storage-owned snapshot/journal using [tensor tests](../../tests/graph/test_tensor_fingerprint_graph.cpp). Do not delete or rewrite persisted state as a diagnostic shortcut. |
| Scheduled edge refresh aborts or removes too many edges | `RefreshStats::aborted_safety_gate`, removal rate, audit trail, configured max removal/addition limits | Pause/stop the refresh through its owning lifecycle if needed, retain audit evidence, and review policy and embedding provider before resuming. No direct storage rollback is defined by this graph runbook. |

## Escalation and rollback

- Escalate suspected data loss, persistence corruption, tenant isolation failure, or authorization bypass to the storage/server security owner and project [SECURITY.md](../../SECURITY.md).
- Escalate reproducible graph algorithm, constraint, or fallback defects with a minimal test case and relevant error code to the graph maintainers.
- Escalate unavailable peers, shard routing, and merge failures to the distributed systems owner; the graph module does not repair shard infrastructure.
- For a suspected regression, stop further rollout through the deployment owner's established rollback process. Preserve the failing build, configuration, error context, and test reproduction; this document defines no service or rollback command.

## Verification after recovery

Re-run the smallest applicable focused test using the current source files and target/CTest naming in [tests/graph/CMakeLists.txt](../../tests/graph/CMakeLists.txt), then exercise the caller integration. For example, `test_graph_exact_traversal.cpp` maps to target `module_graph_test_graph_exact_traversal_focused` and CTest name `test_graph_exact_traversal_GraphFocusedTests`. Verify complete-vs-partial semantics, error counters, and audit evidence. Record actual outcomes; test names or expected thresholds alone are not evidence of recovery.

## Graph developer pages

[RESEARCH](RESEARCH.md) · [MODELING](MODELING.md) · [EMBEDDING](EMBEDDING.md) · [IMPLEMENTATION](IMPLEMENTATION.md) · [API](API.md) · [MODULE_GUIDE](MODULE_GUIDE.md) · [RUNBOOK](RUNBOOK.md) · [OPERATIONS](OPERATIONS.md) · [TRUST](TRUST.md)
