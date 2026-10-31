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
> SOT domain: api-contract
> Canonical source references: [public graph headers](../../include/graph/), [graph-index interface](../../include/index/graph_index.h), [API contract notes](GRAPH_API_CONTRACTS_PHASE1.md)
> Related issue: #7122
>
> Exact model backend not verifiable from final document text alone.

# Graph C++ API guide

This page is a navigational summary. The declarations in `include/graph/` and `include/index/` are authoritative for exact signatures and types; this is not an HTTP, gRPC, or authentication contract.

## Main public surfaces

| Surface | Header | Purpose and contract notes |
|---|---|---|
| `GraphQueryOptimizer` | [graph_query_optimizer.h](../../include/graph/graph_query_optimizer.h) | Optimizes shortest-path, k-hop, pattern-match, and reachability plans; executes BFS/DFS/Dijkstra and supports streams and incremental BFS. Query constraints include bounds, filters, timeouts, and optional execution hints. |
| `PathConstraints` | [path_constraints.h](../../include/graph/path_constraints.h) | Builds and checks path constraints, including required/forbidden vertices and edges, length, weight, uniqueness, custom predicates, and ontology rules. |
| `ParallelTraversal` | [parallel_traversal.h](../../include/graph/parallel_traversal.h) | Multi-source BFS/DFS; `Config` carries depth/result caps, thread count, timeout, forbidden vertices, and fan-out threshold. |
| `GPUGraphTraversal` | [gpu_traversal.h](../../include/graph/gpu_traversal.h) | Loads graph data into an internal CSR representation and offers BFS/DFS results with an explicit `used_cpu_fallback` and `truncated` indicator. |
| `DistributedGraphManager` | [distributed_graph.h](../../include/graph/distributed_graph.h) | Registers `ShardGraphExecutor`s and exposes shortest path, k-hop neighbors, plan optimization, and shard mapping. |
| `OntologyManager` / `KnowledgeGraphReasoner` | [ontology_manager.h](../../include/graph/ontology_manager.h), [knowledge_graph_reasoner.h](../../include/graph/knowledge_graph_reasoner.h) | Loads/builds ontology concepts and axioms; stores facts/rules and exposes inference/explanation. |
| `RotatEModel` / `KGCompletionEngine` | [rotate_completion.h](../../include/graph/rotate_completion.h) | Trains triple embeddings, scores triples, and ranks head/tail candidates. See [EMBEDDING.md](EMBEDDING.md) for evidence and limitations. |
| Tensor graph/dedup | [tensor_fingerprint_graph.h](../../include/graph/tensor_fingerprint_graph.h), [tensor_deduplication_manager.h](../../include/graph/tensor_deduplication_manager.h) | Similarity graph, external persistence hooks, tensor store/retrieve, and snapshot/restore surfaces. |
| Plan rewrite/explain | [graph_query_rewriter.h](../../include/graph/graph_query_rewriter.h), [explain_plan.h](../../include/graph/explain_plan.h) | Rewrites JSON plans and provides plan explanation/inspection surfaces. |

## Result and error behavior

Types vary by interface: for example, optimizer methods return `themis::Result<T>`, while graph-index methods return status/result pairs. Check the declaration being called rather than assuming a single module-wide result wrapper.

The frozen graph taxonomy in [graph_error_taxonomy.h](../../include/graph/graph_error_taxonomy.h) categorizes errors as:

- `DENIAL`: input, constraint, or resource precondition prevents the operation.
- `FALLBACK`: an accelerated/distributed route is unavailable; the caller may use the documented fallback.
- `REASONING_CONFLICT`: a constraint or inferred fact conflict needs semantic/operator review.

Important edge cases include untrained RotatE scoring (throws), unknown registered entity/relation IDs (throws), empty embedding access before training (empty vector), empty/invalid ontology input, traversal depth/result truncation, timeouts, and unavailable/failed shards or devices. See [RUNBOOK.md](RUNBOOK.md) for diagnosis.

## Contract ownership and change policy

- Public C++ declarations live in `include/graph/`; graph-index and storage-specific contracts live in their owning modules.
- The graph module does not own HTTP/wire serialization, API authentication/authorization, or canonical graph persistence. The server API handler is [include/server/graph_api_handler.h](../../include/server/graph_api_handler.h); the graph module provides behavior consumed by that layer.
- `GRAPH_API_CONTRACTS_PHASE1.md` is a historical Phase 1 document and is stale relative to present declarations in some areas. Do not copy its signatures or behavioral claims as current API facts; verify against current headers, implementations, and tests.
- For compatibility changes, consult current project versioning/governance requirements and coordinate with API consumers. Do not infer a current freeze policy solely from the historical Phase 1 document.

## Server integration and metrics

The server's [GraphApiHandler](../../include/server/graph_api_handler.h) receives and stores an `AuthMiddleware` dependency. No handler method call to `auth_` appears in [graph_api_handler.cpp](../../src/server/graph_api_handler.cpp), so this wiring does not establish endpoint enforcement or a graph-module authorization policy. Verify effective authentication/authorization at the server/router boundary before exposing graph operations or metrics.

The server routes `GET /api/v1/graph/metrics` and `GET /api/v1/graph/metrics/prometheus`; see [http_server.cpp](../../src/server/http_server.cpp) and [graph_api_handler.cpp](../../src/server/graph_api_handler.cpp). These are server endpoints, not graph-module C++ APIs.

## Canonical references

[Public graph headers](../../include/graph/) · [Graph index API](../../include/index/graph_index.h) · [Graph architecture](ARCHITECTURE.md) · [Implementation workflow](IMPLEMENTATION.md) · [Trust boundaries](TRUST.md)

## Graph developer pages

[RESEARCH](RESEARCH.md) · [MODELING](MODELING.md) · [EMBEDDING](EMBEDDING.md) · [IMPLEMENTATION](IMPLEMENTATION.md) · [API](API.md) · [MODULE_GUIDE](MODULE_GUIDE.md) · [RUNBOOK](RUNBOOK.md) · [OPERATIONS](OPERATIONS.md) · [TRUST](TRUST.md)
