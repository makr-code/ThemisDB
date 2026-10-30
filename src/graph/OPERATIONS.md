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
> Canonical source references: [graph operational interfaces](../../include/graph/), [graph metrics implementation](distributed_graph.cpp), [graph performance mapping](PERFORMANCE_EXPECTATIONS.md)
> Related issue: #7122
>
> Exact model backend not verifiable from final document text alone.

# Graph operations and observability

This page describes operational signals and responsibility boundaries available in the graph code. It does not define a standalone graph service, deployment configuration, or service command.

## What to observe

| Signal | Source | Operational use |
|---|---|---|
| Optimizer `GraphQueryMetrics` and per-execution `ExecutionStats` | [graph_query_optimizer.h](../../include/graph/graph_query_optimizer.h) | Compare query counts/failures/timeouts, execution time, nodes/edges explored, cache activity, chosen algorithm, and termination behavior. The types expose counters/statistics; exporters and alert thresholds belong to the integrating service. |
| Graph HTTP metrics endpoints | [http_server.cpp](../../src/server/http_server.cpp), [graph_api_handler.cpp](../../src/server/graph_api_handler.cpp) | `GET /api/v1/graph/metrics` returns optimizer counters as JSON; `GET /api/v1/graph/metrics/prometheus` returns Prometheus text exposition. Availability depends on server integration and an initialized graph optimizer. |
| `graph_exact_traversal_errors_total` with `operation` and `reason` labels | [distributed_graph.cpp](distributed_graph.cpp), verified by [test_graph_exact_traversal.cpp](../../tests/graph/test_graph_exact_traversal.cpp) | Diagnose invalid input, no healthy shards, shard execution failure, and shard execution exceptions. Metric presence is not by itself a service health check. |
| GPU traversal `Stats` and result flags | [gpu_traversal.h](../../include/graph/gpu_traversal.h) | Inspect device availability, graph counts, CPU fallback, and truncation before attributing a result to accelerated execution. |
| Tensor fingerprint `nodeCount()`/`edgeCount()` and deduplication stats | [tensor_fingerprint_graph.h](../../include/graph/tensor_fingerprint_graph.h), [tensor_deduplication_manager.h](../../include/graph/tensor_deduplication_manager.h) | Review in-memory graph counts and storage/dedup results alongside the storage engine's own health and persistence evidence. |
| Scheduled refresh `RefreshStats` and audit trail | [scheduled_edge_refresh.h](../../include/graph/scheduled_edge_refresh.h) | Observe evaluated/added/removed edges, cycle duration, safety-gate aborts, removal-rate anomalies, and the bounded audit entries. |
| Watermark snapshot/fingerprint match | [graph_watermark.h](../../include/graph/graph_watermark.h) | Inspect watermark metadata where that feature is used. It is not tenant authorization or a substitute for access-control logs. |

## Operating constraints

- Bound depth, result count, timeout, frontier, resources, and refresh mutation volume according to the owning deployment's requirements. The graph interfaces expose limits; they do not choose production-safe values for every workload.
- A successful distributed call may have behavior different from an all-shards-healthy execution. Inspect health, errors, and result semantics before considering it complete.
- CPU fallback keeps an operation available only where the implementation supports it; it does not establish parity for every failure mode without tests.
- Tensor graph state is backed by caller-provided storage hooks/engines. Backups, restore sequencing, durable journal integrity, and data retention are owned by storage and deployment operators.
- The server `GraphApiHandler` stores an `AuthMiddleware`, but no handler method calls `auth_` in `graph_api_handler.cpp`. Verify which server routes and metrics endpoints enforce authentication/authorization at the server/router boundary; do not infer policy from graph constraints or constructor wiring.
- Do not infer p95/p99 compliance from counters, benchmark source code, or configured thresholds. Use reproducible run output and compare with the baseline mapping in [PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md).

## Change/release coordination

For public API or behavior changes, update the source-level contract, relevant tests, and module docs in the same change. The graph [ROADMAP](ROADMAP.md) is not a substitute for test evidence; [CHANGELOG](CHANGELOG.md) is historical. Coordinate graph-index/storage migrations with their owners and HTTP/wire/access-control changes with the server API owner.

## Incident handling

Use [RUNBOOK.md](RUNBOOK.md) for symptom-based triage and escalation. Use the root [SECURITY.md](../../SECURITY.md) for security incidents. The graph module does not provide commands for starting, stopping, scaling, restoring, or rolling back a deployed service.

## Graph developer pages

[RESEARCH](RESEARCH.md) · [MODELING](MODELING.md) · [EMBEDDING](EMBEDDING.md) · [IMPLEMENTATION](IMPLEMENTATION.md) · [API](API.md) · [MODULE_GUIDE](MODULE_GUIDE.md) · [RUNBOOK](RUNBOOK.md) · [OPERATIONS](OPERATIONS.md) · [TRUST](TRUST.md)
