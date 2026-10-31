---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: ThemisDB documentation orchestration specialist
Human_Review: false
Human_Author: unknown
Source level: level1
SOT domain: api-contract
Canonical source references:
  - ./
  - ../../src/graph/
  - ../../include/index/graph_index.h
Related milestone: DOC-WEEKLY-2026-44
AI_Model_Note: Exact model backend not verifiable from final document text alone.
---

# Graph public headers

This page indexes declarations under `include/graph/`. Exact signatures, defaults, result types,
and comments in each header are authoritative. A declaration alone does not establish that a
backend or implementation is available at runtime.

## Header groups

| Area | Headers |
|---|---|
| Adjacency and traversal | `adjacency_list.h`, `parallel_traversal.h`, `gpu_traversal.h`, `distributed_graph.h`, `path_constraints.h` |
| Planning, caching, explain, and rewrite | `graph_query_optimizer.h`, `graph_query_cache.h`, `graph_plan_cache.h`, `graph_cache_manager.h`, `explain_plan.h`, `graph_query_rewriter.h` |
| Errors, resources, and load distribution | `graph_error_taxonomy.h`, `graph_resource_pool.h`, `graph_load_balancer.h`, `graph_phase_gate_orchestrator.h` |
| Ontology and reasoning | `ontology_manager.h`, `knowledge_graph_reasoner.h` |
| Embedding and tensor utilities | `graph_embedding.h`, `rotate_completion.h`, `tensor_fingerprint_graph.h`, `tensor_deduplication_manager.h` |
| Refresh and watermark utilities | `scheduled_edge_refresh.h`, `graph_watermark.h` |
| Namespace configuration | `namespace_config.h` |

## Key declared classes

The current headers declare, among others, `GraphQueryOptimizer`, `PathConstraints`,
`ParallelTraversal`, `GPUGraphTraversal`, `DistributedGraphManager`, `GraphQueryRewriter`,
`GraphQueryCache`, `GraphLRUPlanCache`, `GraphMultiTierCache`, `OntologyManager`,
`KnowledgeGraphReasoner`, `RotatEModel`, `KGCompletionEngine`, `TensorFingerprintGraph`,
`TensorDeduplicationManager`, `ScheduledGraphEdgeRefreshEngine`, and `GraphWatermark`.
Use the individual header for complete declaration details and supported behavior.

`IGraphEmbeddingProvider` is an interface declaration in `graph_embedding.h`; the presence of
that interface does not prove a concrete graph-module provider is implemented. The inspected
`src/graph/` file inventory does not include `graph_embedding.cpp`.

## Integration boundary

Graph-index contracts belong to [`include/index/graph_index.h`](../index/graph_index.h).
Graph implementation and ownership boundaries are summarized in
[`src/graph/ARCHITECTURE.md`](../../src/graph/ARCHITECTURE.md). Build and focused-test
registration are defined by the project CMake files; this directory does not promise ABI stability
or a single module-wide result type.

## Related documentation

- [Graph C++ API guide](../../src/graph/API.md)
- [Graph architecture](../../src/graph/ARCHITECTURE.md)
- [Graph focused tests](../../tests/graph/README.md)
- [Graph benchmarks](../../benchmarks/graph/README.md)
