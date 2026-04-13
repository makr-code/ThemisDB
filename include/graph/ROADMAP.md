<!-- Status: current | validated: 2026-04-13 -->
<!-- Links: README.md · ARCHITECTURE.md · ../../src/graph/ROADMAP.md -->

# Roadmap — Graph Module (Public Headers)

> Implementation roadmap: `../../src/graph/ROADMAP.md`

## Current Status

v1.4.0 — Production-ready. 6 public headers. `ScheduledGraphEdgeRefreshEngine` with ANN index and CEP event callbacks. Distributed shard execution and GPU traversal available.

## Completed ✅

- [x] GPU traversal header (`gpu_traversal.h`)
- [x] Cost-based query optimizer (`graph_query_optimizer.h`)
- [x] Work-stealing parallel traversal (`parallel_traversal.h`)
- [x] Path constraint validation (`path_constraints.h`)
- [x] Distributed shard execution (`distributed_graph.h`)
- [x] Scheduled edge refresh with ANN + temporal decay (`scheduled_edge_refresh.h`)
- [x] `setANNIndex()` and `setCEPEventCallback()` on `ScheduledGraphEdgeRefreshEngine`
- [x] Bilingual documentation EN (`docs/scheduled_edge_refresh.md`) and DE (`docs/de/scheduled_edge_refresh.md`) — status ✅ Production Ready
- [x] Compendium §6.11 for `ScheduledGraphEdgeRefreshEngine` added to `chapter_06_graph.md`
- [x] `appendix_d_feature_status.md` updated with `ScheduledGraphEdgeRefreshEngine` entry (§6.11)

## Planned

- [ ] GPU BFS/DFS kernel coverage (Issue #1829) (Target: v1.5.0)
- [ ] ANN/GNN graph embedding interface in `scheduled_edge_refresh.h` (Issue #1830) (Target: v1.5.0)
- [ ] EXPLAIN plan inspection header (Target: v1.5.0)

## Implementation Phases

### Phase 1: Core Graph (Complete ✅)
- [x] Traversal, optimizer, path constraints

### Phase 2: Distributed & GPU (Complete ✅)
- [x] Distributed shard execution, GPU traversal

### Phase 3: Semantic Edges (Complete ✅)
- [x] Scheduled edge refresh with ANN scoring

### Phase 4: GNN / Extended GPU (Planned)
- [ ] GPU BFS/DFS kernels, ANN/GNN integration

## Production Readiness Checklist

- [x] 6 public headers compile cleanly
- [x] Distributed execution shard-isolated
- [x] ANN index integration in edge refresh
- [x] Bilingual documentation (EN + DE) for `ScheduledGraphEdgeRefreshEngine`
- [x] Compendium §6.11 documented
- [ ] GPU BFS/DFS kernel coverage
- [ ] GNN embedding interface
