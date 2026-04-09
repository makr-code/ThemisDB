<!-- Status: current | validated: 2026-04-09 -->
<!-- Links: README.md · ARCHITECTURE.md · ../../src/graph/ROADMAP.md -->

# Roadmap — Graph Module (Public Headers)

> Implementation roadmap: `../../src/graph/ROADMAP.md`

## Current Status

v1.5.0 — Production-ready. 9 public headers. `ScheduledGraphEdgeRefreshEngine` with ANN index and CEP event callbacks. Distributed shard execution and GPU traversal available. New interfaces: `GraphEmbedding` (Node2Vec/DeepWalk GNN embeddings), `GraphQueryRewriter` (6 rewrite rules, dry-run explain), `MultiLayerGraph` (layer-aware path finding, PageRank aggregation).

## Completed ✅

- [x] GPU traversal header (`gpu_traversal.h`)
- [x] Cost-based query optimizer (`graph_query_optimizer.h`)
- [x] Work-stealing parallel traversal (`parallel_traversal.h`)
- [x] Path constraint validation (`path_constraints.h`)
- [x] Distributed shard execution (`distributed_graph.h`)
- [x] Scheduled edge refresh with ANN + temporal decay (`scheduled_edge_refresh.h`)
- [x] `setANNIndex()` and `setCEPEventCallback()` on `ScheduledGraphEdgeRefreshEngine`
- [x] GNN/ANN embedding interface (`graph_embedding.h`) — IGraphEmbeddingProvider + GraphEmbedding; Node2Vec, DeepWalk; link prediction; node classification (Issue #1830)
- [x] Graph query rewriting interface (`graph_query_rewriter.h`) — IGraphQueryRewriter + GraphQueryRewriter; 6 rewrite rules; dry-run explainRewrite; stats
- [x] Multi-layer graph interface (`multi_layer_graph.h`) — IMultiLayerGraph + MultiLayerGraph; Dijkstra across layers; PageRank with layer aggregation

## Planned

- [ ] GPU BFS/DFS kernel coverage (Issue #1829) (Target: v1.6.0)

## Implementation Phases

### Phase 1: Core Graph (Complete ✅)
- [x] Traversal, optimizer, path constraints

### Phase 2: Distributed & GPU (Complete ✅)
- [x] Distributed shard execution, GPU traversal

### Phase 3: Semantic Edges (Complete ✅)
- [x] Scheduled edge refresh with ANN scoring

### Phase 4: GNN / Extended GPU (Complete ✅)
- [x] GNN embedding interface (`graph_embedding.h`, `src/graph/graph_embedding.cpp`)
- [x] Graph query rewriting (`graph_query_rewriter.h`, `src/graph/graph_query_rewriter.cpp`)
- [x] Multi-layer graph (`multi_layer_graph.h`, `src/graph/multi_layer_graph.cpp`)
- [ ] GPU BFS/DFS kernels (Issue #1829, Target: v1.6.0)

## Production Readiness Checklist

- [x] 9 public headers compile cleanly
- [x] Distributed execution shard-isolated
- [x] ANN index integration in edge refresh
- [x] GNN embedding interface (Node2Vec, DeepWalk, link prediction, node classification)
- [x] Graph query rewriter (6 rules: predicate pushdown, CSE, join reordering, decomposition, view utilization, edge-type pushdown)
- [x] Multi-layer graph (path finding, BFS reachability, PageRank with AVG/SUM/MAX/MIN/COUNT)
- [x] 42+ tests: `tests/graph/test_graph_future_interfaces.cpp`
- [ ] GPU BFS/DFS kernel coverage
