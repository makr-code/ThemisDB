# Scheduled Semantic Graph Edge Refresh

**Module:** `graph/scheduled_edge_refresh`  
**Version:** 0.1.0  
**Status:** 🚧 Beta  
**Issue:** #FEATURE/ScheduledGraphEdgeRefresh  
**Target Milestone:** Q4 2026

---

## Overview

The **Scheduled Graph Edge Refresh** module provides automatic, policy-driven maintenance of graph edges in ThemisDB. It periodically evaluates the relevance of existing edges using a combination of:

- **Vector similarity** (cosine, dot-product, or Euclidean distance between node embeddings)
- **Temporal decay** (exponential half-life applied to edge age)
- **Centrality weighting** (degree-based dampening to avoid hub over-representation)

Low-relevance edges are pruned and new high-similarity edges are discovered, keeping the graph semantically up-to-date without manual intervention.

### Use Cases

| Domain | Benefit |
|--------|---------|
| Knowledge graphs | Dynamically link semantically related entities; remove stale connections |
| Social graphs | Enrich connections based on evolving user data; decay inactive links |
| ML feature/embedding graphs | Keep distributed representations current as data evolves |

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│  ScheduledGraphEdgeRefreshEngine                                    │
│                                                                     │
│  ┌─────────────┐   ┌──────────────────┐   ┌───────────────────┐   │
│  │  Scheduler  │──▶│  runRefreshCycle │──▶│  scoreAllEdges    │   │
│  │  Thread     │   │                  │   │  (sim + decay +   │   │
│  └─────────────┘   │  collectEdges()  │   │   centrality)     │   │
│                    │                  │   └───────────────────┘   │
│  triggerRefresh()──▶  discoverCand.() │         │                  │
│                    │                  │   ┌──────▼──────────────┐  │
│                    │  applyBatch()    │◀──│  Safety Gates       │  │
│                    │  (ACID commit)   │   │  (max_removal_frac) │  │
│                    └──────────────────┘   └─────────────────────┘  │
│                           │                                         │
│                    ┌──────▼──────────────────────────────────────┐ │
│                    │  Audit Trail (in-memory, bounded ring)      │ │
│                    └─────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────┘
         │                    │
         ▼                    ▼
  GraphIndexManager    NodeEmbeddingProvider
  (ACID write batch)   (user-supplied callback)
```

---

## Configuration: `RefreshPolicy`

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `refresh_interval` | `std::chrono::seconds` | 3600 | Interval between automatic refresh cycles. Set to 0 for manual-only mode. |
| `similarity_metric` | `SimilarityMetric` | `COSINE` | Metric used to compare node embedding vectors. |
| `relevance_threshold` | `float` [0,1] | 0.5 | Edges with `relevance < threshold` are removal candidates. |
| `add_threshold` | `float` [0,1] | 0.7 | Minimum similarity score for a candidate edge to be added. |
| `top_k_candidates` | `uint32_t` | 10 | Top-k most similar neighbours to consider per node during discovery. |
| `decay_half_life_seconds` | `double` | 86400 | Half-life for temporal decay (seconds). 0 = disabled. |
| `max_removal_fraction` | `float` [0,1] | 0.10 | **Safety gate**: maximum fraction of existing edges that may be removed in one cycle. |
| `max_edges_to_add` | `uint32_t` | 1000 | Hard cap on additions per cycle (0 = unlimited). |
| `max_edges_to_remove` | `uint32_t` | 500 | Hard cap on removals per cycle (0 = unlimited). |
| `graph_id` | `std::string` | `""` | Restrict refresh to a specific graph. Empty = all graphs. |

### Validation

All policy fields are validated at construction time. Out-of-range values throw `std::invalid_argument`.

---

## Scoring Model

Each edge's relevance is computed as:

```
relevance = similarity × temporal_factor × centrality_weight
```

### Similarity

Computed between the embedding vectors of the edge's source and target nodes, using the configured `SimilarityMetric`:

| Metric | Formula | Range |
|--------|---------|-------|
| `COSINE` | `(cos(a,b) + 1) / 2` | [0, 1] |
| `DOT_PRODUCT` | `dot(a,b) / (‖a‖ · ‖b‖)`, mapped to [0,1] | [0, 1] |
| `EUCLIDEAN` | `1 / (1 + dist(a,b))` | (0, 1] |

### Temporal Decay

Exponential decay based on edge age:

```
temporal_factor = 2^(−age / half_life)
```

Where `age` is read from the edge's `_created_at` field (seconds since epoch). If the field is absent or `decay_half_life_seconds = 0`, the factor is `1.0` (no decay).

### Centrality Weight

Proportional to the inverse log-degree of the source vertex:

```
centrality_weight = 1 / (1 + log(1 + out_degree))
```

This dampens scores for hub nodes, preventing highly-connected vertices from dominating the refresh.

---

## Refresh Cycle

Each cycle follows these steps:

1. **Collect edges** — Enumerate all edges (scoped by `graph_id` if set).
2. **Score edges** — Compute `relevance` for each edge.
3. **Identify removal candidates** — Edges where `relevance < relevance_threshold`.
4. **Safety gate check** — If `|candidates| / |total_edges| > max_removal_fraction`, abort with `aborted_safety_gate = true`.
5. **Discover new candidates** — For each vertex, find top-k most similar neighbours (using `embedding_fn`). Filter out already-existing edges and those below `add_threshold`.
6. **Apply ACID batch** — All removals and additions are applied atomically via a `WriteBatchWrapper`. On commit failure, the cycle reports an error but does not crash.
7. **Update audit trail** — One `RefreshAuditEntry` per mutation.
8. **Update stats** — `RefreshStats` is updated atomically.

---

## ACID Guarantees

All edge mutations (removals and additions) within a single refresh cycle are submitted as a single `RocksDBWrapper::WriteBatchWrapper`. If the commit fails, no partial mutations are applied and the error is logged. This ensures atomicity at the storage level.

> **Note:** If a safety gate fires, the batch is never submitted – the graph is left untouched.

---

## Audit Trail

Every edge mutation is recorded as a `RefreshAuditEntry`:

```cpp
struct RefreshAuditEntry {
    enum class Action { ADD, REMOVE };
    Action  action;
    std::string edge_id;
    std::string from_vertex;
    std::string to_vertex;
    float   relevance_score;
    std::chrono::system_clock::time_point timestamp;
    uint64_t cycle_number;
};
```

The trail is bounded at **10,000 entries** (oldest entries are evicted). Access via `getAuditTrail()`.

---

## API Reference

### Constructor

```cpp
ScheduledGraphEdgeRefreshEngine(
    GraphIndexManager& graph_mgr,
    const RefreshPolicy& policy,
    NodeEmbeddingProvider embedding_fn = nullptr);
```

- `graph_mgr` — Must outlive the engine.
- `policy` — Validated on construction; throws `std::invalid_argument` on error.
- `embedding_fn` — Optional. If null, similarity scoring is skipped and `similarity = 1.0` is used.

### Lifecycle

```cpp
void start();  // Start background scheduler thread
void stop();   // Stop scheduler and join thread (idempotent)
```

### Manual Trigger

```cpp
RefreshStats triggerRefresh();  // Run a cycle synchronously (thread-safe)
```

### Observation

```cpp
RefreshStats                 getStats()       const;  // Last cycle stats
std::vector<RefreshAuditEntry> getAuditTrail() const;  // Mutation log
const RefreshPolicy&         getPolicy()      const;  // Current policy
```

### Runtime Policy Update

```cpp
void setPolicy(const RefreshPolicy& policy);  // Takes effect on next cycle
```

### Scoring Helpers (testable)

```cpp
float computeSimilarity(const std::vector<float>& a, const std::vector<float>& b) const;
float computeTemporalDecay(const BaseEntity& edge_entity) const;
EdgeScore scoreEdge(const BaseEntity& edge_entity) const;
```

---

## Usage Example

```cpp
#include "graph/scheduled_edge_refresh.h"
#include "index/graph_index.h"

// 1. Configure policy
themis::graph::RefreshPolicy policy;
policy.refresh_interval     = std::chrono::seconds(300);  // every 5 minutes
policy.similarity_metric    = themis::graph::SimilarityMetric::COSINE;
policy.relevance_threshold  = 0.4f;
policy.add_threshold        = 0.75f;
policy.decay_half_life_seconds = 86400.0;  // 1 day
policy.max_removal_fraction = 0.05f;       // never remove more than 5% per cycle
policy.top_k_candidates     = 20;

// 2. Embedding provider (wire up to GNN index or in-memory cache)
themis::graph::NodeEmbeddingProvider embedding_fn =
    [&](const std::string& node_id) -> std::vector<float> {
        return my_gnn_index.getEmbedding(node_id);
    };

// 3. Create and start engine
themis::graph::ScheduledGraphEdgeRefreshEngine engine(
    graph_manager, policy, embedding_fn);
engine.start();

// 4. Manual trigger (optional, for testing or forced refresh)
auto stats = engine.triggerRefresh();
spdlog::info("Removed {} edges, added {} edges in {:.2f}ms",
             stats.edges_removed, stats.edges_added, stats.cycle_duration_ms);

// 5. Inspect audit trail
for (const auto& entry : engine.getAuditTrail()) {
    spdlog::info("[{}] edge {} ({} → {})",
        entry.action == themis::graph::RefreshAuditEntry::Action::ADD ? "ADD" : "REMOVE",
        entry.edge_id, entry.from_vertex, entry.to_vertex);
}

// 6. Graceful shutdown
engine.stop();
```

---

## Integration Points

| Module | Integration |
|--------|-------------|
| `index/graph_index.h` | Edge CRUD, adjacency queries, all-vertices listing |
| `acceleration` | Provide `NodeEmbeddingProvider` backed by GNN/HNSW index |
| `analytics/cep_engine` | Emit edge mutation events into CEP streams for real-time analytics |
| `cdc/changefeed` | Forward audit entries to ChangeFeed for downstream consumers |
| `temporal_graph` | Use `_created_at` field set by temporal graph module |

---

## Performance Considerations

- **Brute-force similarity search** is used for candidate discovery. For graphs with >10,000 nodes, replace with an ANN index (HNSW via `acceleration` module) to keep discovery time sub-linear.
- **Batch writes** amortise RocksDB write amplification.
- **Centrality dampening** reduces unnecessary churn on hub nodes.
- The background scheduler does not run concurrent cycles; each cycle holds `cycle_mutex_` for its duration.

---

## References

- Yu et al. (2017) — STGCN: Spatio-Temporal Graph Convolutional Networks
- Leskovec et al. (2008) — Microscopic Evolution of Social Networks
- Brandes (2008) — On Variants of Shortest-Path Betweenness Centrality
- ThemisDB `analytics/docs/gnn_embeddings.md`
- ThemisDB `analytics/cep_engine.cpp`
