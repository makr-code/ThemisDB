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
> Canonical source references: [graph index interface](../../include/index/graph_index.h), [graph headers](../../include/graph/), [graph module README](README.md)
> Related issue: #7122
>
> Exact model backend not verifiable from final document text alone.

# Graph data and algorithm modeling

This page describes the representations used by graph code. It does not define a separate storage schema; consult the owning index and storage contracts for persistence.

## Property graph and traversal input

- `GraphIndexManager` exposes edge and adjacency operations. Its `PathResult` contains an ordered vertex path and `totalCost`; edge weights are read from `_weight` with a default of `1.0` as documented in [graph_index.h](../../include/index/graph_index.h).
- The optimizer's `QueryConstraints` carries optional depth and result limits, graph ID, edge type, uniqueness and forbidden/required vertex selections, timeout, temporal bounds, schema hints, GPU preference, and parallelism hints. These are execution/planning inputs, not an independent persistent schema: [graph_query_optimizer.h](../../include/graph/graph_query_optimizer.h).
- `PathConstraints` models minimum/maximum length, node/edge property predicates, required/forbidden nodes and edges, cycle/uniqueness rules, custom predicates, weights, and optional ontology validation. A `PathResult` carries node IDs, edge IDs, cost, constraint status, and violation descriptions: [path_constraints.h](../../include/graph/path_constraints.h).
- Graph mutations and persistence are delegated through graph-index and storage interfaces. The graph algorithm module does not own raw blocks, WAL, or the database's canonical vertex/edge records; see [architecture](ARCHITECTURE.md).

## Planning and execution models

`GraphQueryOptimizer` represents algorithm choices (BFS, DFS, bidirectional, A*, Dijkstra), query patterns (including shortest path, k-hop neighborhood, pattern match, reachability, and connected component), graph statistics, and an `OptimizationPlan`. The plan records estimates, selected route, index/cache/parallel flags, alternatives, schema hints, and distributed shard information. The shape of this plan is an optimization description, not a result guarantee.

Execution returns result collections or `GraphIndexManager::PathResult`; the optimizer also exposes execution statistics such as nodes explored, edges traversed, observed depth, timing, early termination, and cache counts. Result truncation, timeout, or partial distributed execution must be handled according to the particular API; do not assume every successful return represents exhaustive graph reachability.

Parallel traversal records visited vertices and a vertex-to-source association. Distributed execution uses registered `ShardGraphExecutor` implementations and shard-qualified vertex IDs. Its merge and partial-failure semantics are implemented in [distributed_graph.cpp](distributed_graph.cpp) and tested in [test_graph_distributed.cpp](../../tests/graph/test_graph_distributed.cpp); a shard registry is not a storage topology definition.

## Semantic graph representation

- `Triple` is a subject/predicate/object tuple. `Rule` contains condition and conclusion triples; an `InferenceEdge` records a derived fact, its rule ID, and premise triples: [knowledge_graph_reasoner.h](../../include/graph/knowledge_graph_reasoner.h).
- `OntologyManager` holds concepts, parent links, allowed edge types, and source/edge/target axioms. The interface supports JSON/YAML loading and a build step; its schema and reasoning state are distinct from persisted graph records: [ontology_manager.h](../../include/graph/ontology_manager.h).
- Reasoner output is inferred context, not a mutation to canonical graph storage unless a caller explicitly persists it. Inference depth is bounded by the reasoner implementation.

## Embeddings and tensor fingerprints

- RotatE represents each entity using real and imaginary vectors of dimension `d`, and each relation using `d` phase values. Its score is an L1 distance after relation rotation; lower score ranks as more plausible. See [rotate_completion.h](../../include/graph/rotate_completion.h) and [rotate_completion.cpp](rotate_completion.cpp).
- `TensorFingerprintGraph` models tensor-train fingerprints using MinHash values, core norms, total norm, order, and maximum rank. It uses LSH buckets to produce candidates and exact tensor-train cosine similarity to score candidate pairs. This is a tensor similarity graph, not a learned node-embedding table: [tensor_fingerprint_graph.h](../../include/graph/tensor_fingerprint_graph.h), [implementation](tensor_fingerprint_graph.cpp).
- The `GraphEmbeddingConfig`/`IGraphEmbeddingProvider` surface declares several embedding algorithms, but no concrete provider implementation was found in the inspected `src/graph/` sources. Treat this as an interface declaration, not evidence that each listed algorithm is implemented.

## Representation limits and safe interpretation

- A graph query plan's cost estimate and a RotatE ranking score are estimates/scores, not proofs of result quality.
- Constraints attached to a query shape execution. They do not authenticate the caller, establish tenant entitlement, or replace authorization checks in the server/API layer.
- Fingerprint candidate generation may be bounded by `max_candidates` and `top_k`; a returned neighbor list is not proof that every possible similar tensor was considered.
- Watermark and tenant fields in graph/tensor data structures are metadata, not access-control enforcement.

## Canonical references

[Graph index contract](../../include/index/graph_index.h) · [Graph architecture](ARCHITECTURE.md) · [Graph API overview](API.md) · [Graph security boundaries](TRUST.md) · [Graph roadmap](ROADMAP.md)

## Graph developer pages

[RESEARCH](RESEARCH.md) · [MODELING](MODELING.md) · [EMBEDDING](EMBEDDING.md) · [IMPLEMENTATION](IMPLEMENTATION.md) · [API](API.md) · [MODULE_GUIDE](MODULE_GUIDE.md) · [RUNBOOK](RUNBOOK.md) · [OPERATIONS](OPERATIONS.md) · [TRUST](TRUST.md)
