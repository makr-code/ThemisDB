# Index Module

Index implementations for ThemisDB's multi-model database.

## Components

- Adaptive index
- Vector index (HNSW)
- **Approximate radius search (IMPLEMENTED - GAP-006)**
- **Multi-vector search (IMPLEMENTED)**
- Graph index
- Property graph
- Secondary indexes
- GNN embeddings

## Features

- HNSW-based vector similarity search
- **Radius-based vector search** (find all within distance threshold) - Production ready
- **Multi-vector search** (ensemble search, query expansion, hybrid search, learned fusion) - Production ready
- Graph traversal indexing
- Property graph model support
- Adaptive index selection
- Multi-dimensional indexing

## Documentation

For index documentation, see:
- [Adaptive Index](../../docs/de/src/index/adaptive_index.cpp.md)
- [Vector Index](../../docs/de/src/index/vector_index.cpp.md)
- [Approximate Radius Search](../../docs/ApproximateRadiusSearch.md) - **Production Ready**
- [Multi-Vector Search](../../docs/multi_vector_search.md) - **Production Ready**
- [Vector Advanced Features](./VECTOR_ADVANCED_FEATURES_README.md) - Detailed guide
- [Graph Index](../../docs/de/src/index/graph_index.cpp.md)
- [Property Graph](../../docs/de/src/index/property_graph.cpp.md)
- [Secondary Index](../../docs/de/src/index/secondary_index.cpp.md)
- [GNN Embeddings](../../docs/de/src/index/gnn_embeddings.cpp.md)
- [HNSW Persistence](../../docs/hnsw_persistence.md)

## Tests and Benchmarks

Vector search tests and benchmarks:
- [Multi-Vector Search Tests](../../tests/test_multi_vector_search.cpp) - Unit and integration tests
- [Approximate Radius Search Tests](../../tests/test_approximate_radius_search_integration.cpp) - Comprehensive integration tests
- [Approximate Radius Search Benchmark](../../benchmarks/bench_approximate_radius_search.cpp) - Performance benchmarks
