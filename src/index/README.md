# Index Module

Index implementations for ThemisDB's multi-model database.

## Components

- Adaptive index
- Vector index (HNSW)
- **Approximate radius search (NEW - GAP-006)**
- Multi-vector search (stub)
- Graph index
- Property graph
- Secondary indexes
- GNN embeddings

## Features

- HNSW-based vector similarity search
- **Radius-based vector search** (find all within distance threshold)
- Graph traversal indexing
- Property graph model support
- Adaptive index selection
- Multi-dimensional indexing

## Documentation

For index documentation, see:
- [Adaptive Index](../../docs/src/index/adaptive_index.cpp.md)
- [Vector Index](../../docs/src/index/vector_index.cpp.md)
- [Approximate Radius Search](../../docs/src/index/approximate_radius_search.cpp.md) *(NEW)*
- [Graph Index](../../docs/src/index/graph_index.cpp.md)
- [Property Graph](../../docs/src/index/property_graph.cpp.md)
- [Secondary Index](../../docs/src/index/secondary_index.cpp.md)
- [GNN Embeddings](../../docs/src/index/gnn_embeddings.cpp.md)
- [Indexes Documentation](../../docs/indexes.md)
- [HNSW Persistence](../../docs/hnsw_persistence.md)
