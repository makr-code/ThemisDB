> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/graph/ARCHITECTURE.md -->

# Graph Module — Public Header Architecture

**Module Path:** `include/graph/`  
**Implementation:** `../../src/graph/`  
**Canonical architecture doc:** [`../../src/graph/ARCHITECTURE.md`](../../src/graph/ARCHITECTURE.md)

---

## 1. Overview

`include/graph/` defines the **public graph traversal, embedding, knowledge-graph reasoning, ontology management, tensor deduplication, and query optimisation API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/graph/ARCHITECTURE.md`](../../src/graph/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Graph Traversal and Analytics

| Header | Public Type | Purpose |
|--------|------------|---------|
| `distributed_graph.h` | `DistributedGraph` | Distributed graph partition management |
| `parallel_traversal.h` | `ParallelTraversal` | Multi-threaded BFS/DFS traversal |
| `gpu_traversal.h` | `GPUTraversal` | GPU-accelerated graph traversal |
| `path_constraints.h` | `PathConstraints` | Path length and predicate constraints |
| `graph_watermark.h` | `GraphWatermark` | Temporal watermark tracking for streaming graphs |
| `scheduled_edge_refresh.h` | `ScheduledEdgeRefresh` | Time-triggered edge refresh |
### 2.2 Knowledge Graph and Ontology

| Header | Public Type | Purpose |
|--------|------------|---------|
| `knowledge_graph_reasoner.h` | `KnowledgeGraphReasoner` | Ontological inference and graph reasoning |
| `ontology_manager.h` | `OntologyManager` | OWL/RDF ontology lifecycle management |
### 2.3 Graph Embeddings

| Header | Public Type | Purpose |
|--------|------------|---------|
| `graph_embedding.h` | `GraphEmbedding` | GNN-based node and edge embedding generation |
| `tensor_fingerprint_graph.h` | `TensorFingerprintGraph` | Tensor-fingerprint graph deduplication |
| `tensor_deduplication_manager.h` | `TensorDeduplicationManager` | Cross-shard tensor deduplication manager |
### 2.4 Query Optimisation

| Header | Public Type | Purpose |
|--------|------------|---------|
| `graph_query_optimizer.h` | `GraphQueryOptimizer` | Cost-based graph query optimiser |
| `graph_query_rewriter.h` | `GraphQueryRewriter` | Semantic graph query rewriting |
| `explain_plan.h` | `ExplainPlan` | Graph query execution plan explanation |

---

## 3. Namespace Layout

All public types reside in the `themis::graph` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/graph/` expose the **stable public API**; internal types live in `src/graph/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **Graph Truth Layer**.
