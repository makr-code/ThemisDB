# Vector Search Module — Architecture

<!-- Status: PRODUCTION_CANDIDATE | validated: 2026-08-10 -->

## Overview

The vector search module provides approximate nearest neighbor (ANN) search capabilities over high-dimensional embeddings, enabling efficient similarity queries for semantic search, recommendation systems, and retrieval-augmented generation (RAG) pipelines within ThemisDB.

## Design Principles

1. **Algorithm Flexibility:** Multiple ANN algorithms (HNSW, IVF) selectable per use case
2. **Memory Efficiency:** Avoid redundant storage; compress indices where feasible
3. **Query Responsiveness:** P99 latency < 10 ms for typical use cases
4. **Correctness First:** Approximate results validated against brute-force baseline
5. **Scalability:** Support indices up to available system memory

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│  Semantic Search / RAG Pipeline                             │
│  • Produces query vectors from text/embeddings              │
└──────────────────────┬──────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────┐
│  SimilaritySearch (Query Interface)                         │
│  • Execute k-nearest neighbor search                        │
│  • Apply distance metric (cosine, L2, inner product)        │
│  • Return ranked results with distances                     │
└──────────────────────┬──────────────────────────────────────┘
                       │
        ┌──────────────┼──────────────┐
        │              │              │
        ▼              ▼              ▼
   ┌─────────┐  ┌─────────┐  ┌──────────────┐
   │HNSW     │  │IVF      │  │BruteForce    │
   │Index    │  │Index    │  │(Validation)  │
   │         │  │         │  │              │
   └────┬────┘  └────┬────┘  └──────┬───────┘
        │            │              │
        └────────────┼──────────────┘
                     │
                     ▼
        ┌────────────────────────┐
        │ DistanceMetric         │
        │ • Cosine distance      │
        │ • L2 (Euclidean)       │
        │ • Inner product        │
        └────────────────────────┘
```

## Core Components

### SimilaritySearch (Main Entry Point)

**Purpose:** Unified interface for executing vector similarity queries.

**Responsibilities:**
- Accept query vector and search parameters (k, distance metric)
- Route to appropriate index algorithm
- Execute search and rank results
- Return K-nearest neighbors with distances

**Public API:**
```cpp
class SimilaritySearch {
  Result<KNearestNeighbors> search(
    const std::vector<float>& query,
    int k,
    DistanceMetric metric = DistanceMetric::COSINE
  );
};
```

### HNSW (Hierarchical Navigable Small World) Index

**Purpose:** Fast approximate nearest neighbor search using hierarchical graph structure.

**Algorithm Overview:**
- Multi-layer graph where each layer is a navigable small world
- Higher layers act as shortcuts for fast search
- Logarithmic time complexity for search (O(log N))

**Characteristics:**
- Excellent for high-dimensional vectors (hundreds to thousands)
- Fast insertion and deletion
- Tunable M and ef parameters for speed/accuracy tradeoff
- Memory efficient compared to brute force

**Configuration:**
- M: max connections per node (default: 16)
  - Higher M = more connections = faster search but more memory
- ef_construction: search depth during insertion (default: 200)
- ef_search: search depth during query (default: k + 100)
- Layer decay: 1/ln(2) ≈ 1.44

**Performance:**
- Insert: 10-100 µs per vector
- Search k=10: 1-10 ms for indices up to 1M vectors
- Memory: ~30% overhead vs. raw vectors

### IVF (Inverted File) Index

**Purpose:** Approximate nearest neighbor search via coarse quantization and clustering.

**Algorithm Overview:**
- Cluster vectors using k-means (coarse quantization)
- During search, examine only nearby clusters
- Within clusters, compute exact distances

**Characteristics:**
- Good for very large indices (millions to billions)
- Fast clustering-based filtering
- Adjustable nprobe parameter for accuracy/speed
- Enables hierarchical search (coarse then fine)

**Configuration:**
- n_clusters: number of k-means clusters (default: sqrt(N))
- nprobe: number of clusters to search (default: 10)
  - Higher nprobe = more accurate but slower
- Max cluster size: configurable
- Cluster rebuild interval: configurable

**Performance:**
- Insert: 100-500 µs per vector (includes clustering)
- Search k=10: 5-50 ms for large indices
- Memory: ~20% overhead vs. raw vectors

### Distance Metrics

**Purpose:** Compute similarity between vectors using different distance functions.

**Implemented Metrics:**

1. **Cosine Distance**
   - Most common for text embeddings
   - Works on unit-normalized vectors
   - Range: [0, 2] (0 = identical, 2 = opposite)
   - Formula: distance = 1 - (dot product of normalized vectors)

2. **L2 (Euclidean) Distance**
   - Standard Euclidean distance
   - Works on unnormalized vectors
   - Range: [0, ∞)
   - Formula: sqrt(sum of squared differences)

3. **Inner Product**
   - Dot product (for pre-normalized vectors)
   - Range: [-1, 1]
   - Higher value = more similar
   - Used for efficiency in some embeddings

**Optimization Strategies:**
- SIMD vectorization for batch distance computation
- Cache locality optimization for large vectors
- Pre-normalization for cosine similarity

## Data Flow

### Index Construction Pipeline

```
Input: Set of Vectors + Document IDs
  │
  ├─► Dimension validation (all vectors same dimension)
  │
  ├─► Normalization (if using cosine distance)
  │
  ├─► Algorithm Selection
  │   ├─► HNSW: for fast single queries
  │   └─► IVF: for very large indices
  │
  ├─► Index Building
  │   ├─► HNSW: hierarchical graph construction
  │   └─► IVF: k-means clustering
  │
  └─► Output: Indexed vectors ready for search
```

### Query Execution Pipeline

```
Query Vector + Parameters (k, metric)
  │
  ├─► Dimension validation
  │
  ├─► Normalization (if required by metric)
  │
  ├─► Distance Metric Selection
  │
  ├─► Index Algorithm Dispatch
  │   ├─► HNSW: hierarchical traversal
  │   └─► IVF: cluster filtering + fine search
  │
  ├─► Distance Computation (vectorized)
  │
  ├─► Result Ranking (sort by distance)
  │
  └─► Output: K-nearest neighbors with distances
```

## Concurrency Model

### Thread Safety

1. **Index Read Operations:** Multiple readers allowed
   - Search operations don't modify index
   - Read-write lock for index access
   - Allows concurrent queries

2. **Index Modification:** Exclusive write access
   - Adding/removing vectors requires write lock
   - Prevents corruption from concurrent modifications
   - Rebuild operations wait for exclusive lock

### Synchronization Primitives

- `std::shared_mutex` for index access control
- `std::atomic<>` for counters and flags

## Performance Characteristics

### Target Latencies (P99)

- **Insertion:** < 100 µs per vector (HNSW)
- **Search k=10:** < 10 ms (HNSW with 1M vectors)
- **Search k=100:** < 50 ms (IVF with large index)

### Throughput

- **Insert Throughput:** > 1000 vectors/sec
- **Query Throughput:** > 100 queries/sec
- **Concurrent Queries:** ≥ 100 with minimal overhead

### Resource Consumption

- **Per-Vector Memory:** ~0.1-0.5 MB (HNSW with M=16)
- **Per-Vector Memory:** ~0.05-0.2 MB (IVF with clustering)
- **Index Overhead:** 20-40% above raw vector storage

## Error Handling

### Graceful Degradation

1. **Dimension Mismatch** → Return error; don't corrupt index
2. **Invalid Vector** (NaN, inf) → Skip insertion; log warning
3. **Empty Index** → Return empty results
4. **Index Corruption** → Rebuild from scratch if possible
5. **Out of Memory** → Reject insertions; preserve existing index

### Error Codes (E5400–E5499)

- E5400: Invalid vector dimension
- E5401: Vector contains NaN or inf
- E5402: Index is empty
- E5403: Search returned no results
- E5404: Index corruption detected

## Integration Points

### Retrieval-Augmented Generation (RAG)

Vector search integrates into RAG pipelines:
- Query embedding passed to similarity search
- Top-k documents retrieved for context
- Results fed to LLM for answer generation

### Semantic Search

Used in document retrieval:
- Natural language queries converted to embeddings
- Vector search finds relevant documents
- Results ranked by similarity

## See Also

- [`ROADMAP.md`](ROADMAP.md) — Implementation phases and deliverables
- [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md) — Planned features
- [`../../include/vector_search/vector_index.h`](../../include/vector_search/vector_index.h) — Public API
