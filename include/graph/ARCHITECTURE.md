<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · ../../src/graph/ -->

# Graph Module — Public Header Architecture
**Version:** 1.4.0
**Module Path:** `include/graph/`
**Implementation:** `../../src/graph/`

---

## Overview

The Graph module provides public headers for property graph storage, traversal (BFS/DFS/parallel), distributed shard-based execution, GPU-accelerated graph traversal, query optimization, scheduled edge refresh, and path constraint validation.

## Design Principles

- **ANN-Aware Edges** — `ScheduledGraphEdgeRefreshEngine` (v1.3+) uses vector similarity scoring and temporal decay to refresh semantic edges automatically.
- **Distributed Execution** — `DistributedGraphManager` / `ShardGraphExecutor` enable shard-isolated graph query execution.
- **GPU Traversal** — `GPUGraphTraversal` dispatches BFS/DFS onto CUDA kernels; falls back to CPU on unavailable hardware.
- **Cost-Based Optimization** — `GraphQueryOptimizer` uses cardinality estimation for multi-hop path planning.

## Interface Inventory

| Header | Classes / Structs | Purpose |
|--------|-------------------|---------|
| `scheduled_edge_refresh.h` | `ScheduledGraphEdgeRefreshEngine`, `RefreshPolicy`, `EdgeScore`, `RefreshStats`, `RefreshAuditEntry` | Scheduled semantic edge refresh with ANN scoring and temporal decay |
| `distributed_graph.h` | `DistributedGraphManager`, `ShardGraphExecutor`, `LocalShardGraphExecutor`, `DistributedGraphConfig` | Distributed shard-isolated graph query execution |
| `gpu_traversal.h` | `GPUGraphTraversal` | CUDA-accelerated graph BFS/DFS traversal |
| `graph_query_optimizer.h` | `GraphQueryOptimizer` | Cost-based query optimizer with cardinality estimation |
| `parallel_traversal.h` | `ParallelTraversal` | Work-stealing parallel BFS/DFS traversal |
| `path_constraints.h` | `PathConstraints`, `GraphIndexManager` | Path constraint validation for node/edge filter expressions |

## References

- Implementation details: `../../src/graph/`
- ANN index integration: `include/index/ann_index.h`
- CEP event callbacks: `../../src/graph/ARCHITECTURE.md`
