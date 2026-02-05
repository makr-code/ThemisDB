# PathConstraints and Vector Advanced Features - Usage Guide

## Overview

This document describes the PathConstraints and Advanced Vector Index features implemented in ThemisDB v1.5.0.

## PathConstraints (Graph Traversal)

### Description

PathConstraints provide fine-grained control over graph traversal algorithms (BFS and Dijkstra), allowing you to:
- Prevent cycles (unique vertices/edges)
- Block specific vertices or edges
- Require certain vertices to be visited
- Limit path length (min/max edge count)

### API Usage

#### C++ API

```cpp
#include "index/graph_index.h"

// Create constraints
themis::GraphIndexManager::PathConstraints constraints;

// Example 1: Prevent cycles
constraints.unique_vertices = true;
constraints.unique_edges = true;

// Example 2: Block specific nodes
constraints.forbidden_vertices.insert("blocked_user");
constraints.forbidden_edges.insert("spam_edge");

// Example 3: Require certain nodes
constraints.required_vertices.insert("checkpoint1");
constraints.required_vertices.insert("checkpoint2");

// Example 4: Limit path length
constraints.min_edge_count = 2;  // At least 2 hops
constraints.max_edge_count = 5;  // At most 5 hops

// Use with BFS
auto [status, vertices] = graph_index->bfsWithConstraints(
    "start_node",
    3,  // max depth
    constraints,
    "follows",  // edge type filter (optional)
    "social"    // graph ID filter (optional)
);

// Use with Dijkstra
auto [status, path_result] = graph_index->dijkstraWithConstraints(
    "start_node",
    "end_node",
    constraints,
    "",  // no edge type filter
    ""   // no graph ID filter
);

if (status.ok) {
    std::cout << "Path found with cost: " << path_result.totalCost << std::endl;
    for (const auto& vertex : path_result.path) {
        std::cout << vertex << " -> ";
    }
}
```

### Constraint Types

| Constraint | Type | Description | BFS | Dijkstra |
|------------|------|-------------|-----|----------|
| `unique_vertices` | bool | No vertex visited twice | ✅ | ✅ |
| `unique_edges` | bool | No edge traversed twice | ✅ | ✅ |
| `forbidden_vertices` | set<string> | Blacklist specific vertices | ✅ | ✅ |
| `forbidden_edges` | set<string> | Blacklist specific edges | ✅ | ✅ |
| `required_vertices` | set<string> | Must-visit vertices | ✅ | ✅ |
| `min_edge_count` | int | Minimum path length | ✅ | ✅ |
| `max_edge_count` | int | Maximum path length | ✅ | ✅ |

### Use Cases

1. **Fraud Detection**: Block suspicious nodes during traversal
   ```cpp
   constraints.forbidden_vertices = fraud_detector.getSuspiciousUsers();
   ```

2. **Quality Paths**: Ensure paths go through verified checkpoints
   ```cpp
   constraints.required_vertices.insert("verified_hub");
   ```

3. **Cycle Prevention**: Find acyclic paths
   ```cpp
   constraints.unique_vertices = true;
   ```

4. **Path Length Control**: Find paths within specific hop ranges
   ```cpp
   constraints.min_edge_count = 2;
   constraints.max_edge_count = 6;  // "six degrees of separation"
   ```

## Advanced Vector Index (FAISS Integration)

### Description

Advanced Vector Index provides FAISS-based IVF+PQ (Inverted File + Product Quantization) indexing for large-scale vector search with:
- 10-100x memory reduction vs flat index
- 2-10x faster search on large datasets (>1M vectors)
- GPU acceleration support
- Multiple index types (IVF_FLAT, IVF_PQ, HNSW_FLAT, IVF_HNSW_PQ)

### Requirements

- **Build flag**: `THEMIS_GPU_ENABLED` must be defined (requires FAISS library)
- **Hardware**: CPU or GPU (GPU recommended for >10M vectors)
- **Memory**: Training requires at least 30 * nlist vectors

### API Usage

#### C++ API

```cpp
#include "index/vector_index.h"

// Step 1: Configure advanced indexing (BEFORE init)
themis::VectorIndexManager::AdvancedIndexConfig config;
config.enabled = true;
config.index_type = themis::VectorIndexManager::AdvancedIndexConfig::Type::IVF_PQ;
config.nlist = 1024;      // Number of IVF clusters (sqrt(N) is good default)
config.nprobe = 64;       // Clusters to search (tradeoff: speed vs accuracy)
config.use_pq = true;     // Enable Product Quantization
config.pq_m = 8;          // Sub-quantizers (dimension % m must be 0)
config.pq_nbits = 8;      // Bits per sub-quantizer
config.use_gpu = false;   // Set to true for GPU acceleration
config.train_size = 100000; // Training set size

auto status = vector_index->setAdvancedIndexConfig(config);
if (!status.ok) {
    std::cerr << "Failed to enable advanced index: " << status.message << std::endl;
}

// Step 2: Initialize as normal
vector_index->init("documents", 768, themis::VectorIndexManager::Metric::COSINE);

// Step 3: Add vectors (training happens automatically when needed)
themis::BaseEntity doc("doc1");
doc.setField("embedding", std::vector<float>(768, 0.5f));
vector_index->addEntity(doc);

// Step 4: Search (uses advanced index transparently)
std::vector<float> query(768, 0.5f);
auto [status, results] = vector_index->searchKnn(query, 10);
```

### Configuration Guide

#### Index Types

| Type | Memory | Speed | Accuracy | Use Case |
|------|--------|-------|----------|----------|
| IVF_FLAT | High | Fast | High | <10M vectors, accuracy critical |
| IVF_PQ | Low | Very Fast | Medium | >10M vectors, memory constrained |
| HNSW_FLAT | Very High | Very Fast | Highest | <5M vectors, speed critical |
| IVF_HNSW_PQ | Medium | Fast | High | Best tradeoff for large datasets |

#### Parameter Tuning

**nlist (number of clusters)**
- Formula: `sqrt(N)` where N is dataset size
- Small datasets (<100K): 256-512
- Medium datasets (100K-10M): 1024-4096
- Large datasets (>10M): 8192-16384

**nprobe (clusters to search)**
- Tradeoff: Higher = better accuracy, slower search
- Fast: nprobe = nlist / 16
- Balanced: nprobe = nlist / 8
- Accurate: nprobe = nlist / 4

**Product Quantization (PQ)**
- `pq_m`: Number of sub-quantizers (dimension must be divisible by m)
  - Typical: 8, 16, 32
  - Higher = less compression, better accuracy
- `pq_nbits`: Bits per sub-quantizer (8 or 16)
  - 8 bits: 256 centroids per sub-space
  - 16 bits: 65536 centroids per sub-space

### Recommendations

**When to use Advanced Index:**
- Dataset >100K vectors
- Memory constrained (PQ provides 10-100x compression)
- Need fast search (IVF provides 2-10x speedup)
- Have GPU available (use gpu acceleration)

**When to use Standard HNSW:**
- Dataset <100K vectors
- Accuracy is critical (no compression loss)
- Real-time insertion (no training required)
- Incremental updates frequent

### Example Configurations

#### Small Dataset (100K vectors, 384 dim)
```cpp
config.nlist = 256;
config.nprobe = 32;
config.use_pq = false;  // Use IVF_FLAT
config.index_type = AdvancedIndexConfig::Type::IVF_FLAT;
```

#### Medium Dataset (1M vectors, 768 dim)
```cpp
config.nlist = 1024;
config.nprobe = 64;
config.use_pq = true;
config.pq_m = 8;
config.index_type = AdvancedIndexConfig::Type::IVF_PQ;
```

#### Large Dataset (10M vectors, 1536 dim, GPU)
```cpp
config.nlist = 4096;
config.nprobe = 128;
config.use_pq = true;
config.pq_m = 16;
config.use_gpu = true;
config.gpu_device = 0;
config.index_type = AdvancedIndexConfig::Type::IVF_PQ;
```

## Testing

### PathConstraints Tests

Run the PathConstraints test suite:
```bash
./build/themis_tests --gtest_filter="PathConstraintsTest.*"
```

Tests cover:
- Unique vertices/edges
- Forbidden vertices/edges
- Required vertices
- Min/max edge count
- Combined constraints

### Advanced Vector Index Tests

Run the Advanced Vector Index tests:
```bash
./build/themis_tests --gtest_filter="AdvancedVectorIntegrationTest.*"
```

Tests cover:
- Index creation and configuration
- Training and search
- Different index types
- Save/load persistence
- Batch search
- Error handling

## Performance Characteristics

### PathConstraints Performance

| Operation | Time Complexity | Space Complexity | Notes |
|-----------|----------------|------------------|-------|
| BFS with constraints | O(V + E) | O(V) | Same as standard BFS |
| Dijkstra with constraints | O((V + E) log V) | O(V) | Priority queue overhead |
| Constraint checking | O(1) | O(C) | C = number of constraints |

**Impact**: Minimal overhead (<5%) for most constraint configurations.

### Advanced Vector Index Performance

| Metric | Flat Index | IVF_FLAT | IVF_PQ |
|--------|-----------|----------|---------|
| Memory (1M, 768d) | 3 GB | 3 GB | 200 MB |
| Search QPS | 100 | 500-1000 | 800-1500 |
| Accuracy (recall@10) | 100% | 99% | 95-98% |
| Training time | - | 30s | 60s |

## Troubleshooting

### PathConstraints

**Problem**: BFS/Dijkstra returns empty results
- **Cause**: Constraints too restrictive
- **Solution**: Check forbidden_vertices doesn't block all paths

**Problem**: "Required vertices not found" error
- **Cause**: Required vertices not reachable
- **Solution**: Verify graph connectivity or remove constraint

### Advanced Vector Index

**Problem**: "FAISS not available" error
- **Cause**: THEMIS_GPU_ENABLED not defined
- **Solution**: Rebuild with FAISS support or use standard HNSW

**Problem**: Training fails
- **Cause**: Insufficient training data
- **Solution**: Ensure at least 30 * nlist vectors available

**Problem**: Low search accuracy
- **Cause**: nprobe too small or PQ too aggressive
- **Solution**: Increase nprobe or reduce pq_m

## Future Enhancements

### PathConstraints (Planned)
- AQL syntax support: `PATH.ALL()`, `PATH.ANY()`, `PATH.NONE()`
- Expression-based constraints
- Path-wide predicates

### Advanced Vector Index (Planned)
- Automatic parameter tuning
- Multi-GPU support
- Online index updates without retraining

## References

- [PathConstraints Documentation](../de/features/features_path_constraints.md)
- [Vector Index Documentation](../en/features/vector_search.md)
- [FAISS Documentation](https://github.com/facebookresearch/faiss)
- [HNSW Paper](https://arxiv.org/abs/1603.09320)
