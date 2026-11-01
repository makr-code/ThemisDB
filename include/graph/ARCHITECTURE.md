---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: ThemisDB documentation orchestration specialist
Human_Review: false
Human_Author: ThemisDB Contributors
Source level: level1
SOT domain: architecture-governance
Canonical source references:
  - ./
  - ../../src/graph/ARCHITECTURE.md
  - ../index/graph_index.h
Related milestone: DOC-WEEKLY-2026-44
AI_Model_Note: Exact model backend not verifiable from final document text alone.
---

> **Build:** see [`tests/graph/README.md`](../../tests/graph/README.md) for a test-enabled
> configuration; `linux-release` disables tests and benchmarks.

<!-- This header map was last validated 2026-06-01; corrected names below use current headers. -->
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
| `distributed_graph.h` | `DistributedGraphManager`, `ShardGraphExecutor` | Registered shard-executor coordination |
| `parallel_traversal.h` | `ParallelTraversal` | Multi-threaded BFS/DFS traversal |
| `gpu_traversal.h` | `GPUGraphTraversal` | Device-aware traversal API with result fallback/truncation fields |
| `path_constraints.h` | `PathConstraints` | Path length and predicate constraints |
| `graph_watermark.h` | `GraphWatermark` | Temporal watermark tracking for streaming graphs |
| `scheduled_edge_refresh.h` | `ScheduledGraphEdgeRefreshEngine` | Scheduled edge refresh API |
### 2.2 Knowledge Graph and Ontology

| Header | Public Type | Purpose |
|--------|------------|---------|
| `knowledge_graph_reasoner.h` | `KnowledgeGraphReasoner` | Ontological inference and graph reasoning |
| `ontology_manager.h` | `OntologyManager` | OWL/RDF ontology lifecycle management |
### 2.3 Graph Embeddings

| Header | Public Type | Purpose |
|--------|------------|---------|
| `graph_embedding.h` | `IGraphEmbeddingProvider` | Embedding-provider interface declaration; concrete implementation not established here |
| `tensor_fingerprint_graph.h` | `TensorFingerprintGraph` | Tensor-fingerprint graph deduplication |
| `tensor_deduplication_manager.h` | `TensorDeduplicationManager` | Cross-shard tensor deduplication manager |
### 2.4 Query Optimisation

| Header | Public Type | Purpose |
|--------|------------|---------|
| `graph_query_optimizer.h` | `GraphQueryOptimizer` | Cost-based graph query optimiser |
| `graph_query_rewriter.h` | `GraphQueryRewriter` | Semantic graph query rewriting |
| `explain_plan.h` | `GraphExplainPlan`, `IGraphExplainProvider` | Explain-plan representation/provider |

---

## 3. Namespace Layout

All public types reside in the `themis::graph` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/graph/` declare public-facing C++ surfaces; the word "public" here does
  not by itself promise ABI stability or a release-specific stability level.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **Graph Truth Layer**.

The table is a navigation map, not a substitute for checking each header declaration. Some
declared interfaces may not have a concrete implementation in `src/graph/`.
