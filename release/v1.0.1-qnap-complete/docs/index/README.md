# Index Module

**Stand:** 5. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** Index

---

## Übersicht

Das Index-Modul bietet verschiedene Index-Typen für effiziente Datenabfragen in ThemisDB.

## Source-Code Referenz

| Komponente | Header | Source | LOC | Beschreibung |
|------------|--------|--------|-----|--------------|
| SecondaryIndexManager | `secondary_index.h` | `secondary_index.cpp` | ~2,500 | B-Tree Indexes |
| VectorIndexManager | `vector_index.h` | `vector_index.cpp` | ~2,000 | HNSW ANN Search |
| GraphIndexManager | `graph_index.h` | `graph_index.cpp` | ~2,500 | Graph Traversal |
| PropertyGraph | `property_graph.h` | `property_graph.cpp` | ~1,500 | Property Graph |
| FulltextIndex | `fulltext_index.h` | `fulltext_index.cpp` | ~1,000 | Inverted Index |
| TemporalGraph | `temporal_graph.h` | `temporal_graph.cpp` | ~800 | Zeit-Filter |
| GNNEmbeddings | `gnn_embeddings.h` | `gnn_embeddings.cpp` | ~700 | Graph Neural Nets |
| AdaptiveIndex | `adaptive_index.h` | `adaptive_index.cpp` | ~600 | Selbstoptimierung |
| SpatialIndex | `spatial_index.h` | - | ~400 | R-Tree |
| EdgeTypes | `edge_types.h` | - | ~300 | Edge Categories |

**Gesamt:** 12 Header, 11 Source-Dateien, ~14,600 LOC

## Implementierte Index-Typen

### SecondaryIndexManager

```cpp
class SecondaryIndexManager {
    // Index-Typen
    enum class IndexType { SINGLE, COMPOSITE, RANGE, SPARSE, TTL, FULLTEXT };
    
    // API
    Status createIndex(const IndexConfig& config);
    Status dropIndex(const std::string& name);
    Status rebuildIndex(const std::string& name);
    std::vector<std::string> lookup(const std::string& field, const Value& value);
    std::vector<std::string> rangeQuery(const std::string& field, const Value& min, const Value& max);
};
```

### VectorIndexManager (HNSW)

```cpp
class VectorIndexManager {
    enum class Metric { L2, COSINE, DOT };
    
    // API
    Status init(objectName, dim, metric, M, efConstruction, efSearch, savePath);
    Status addEntity(const BaseEntity& e, vectorField = "embedding");
    Status removeByPk(std::string_view pk);
    
    // KNN-Suche mit optionalem Whitelist Pre-Filtering
    std::pair<Status, std::vector<Result>> searchKnn(
        const std::vector<float>& query,
        size_t k,
        const std::vector<std::string>* whitelistPks = nullptr
    );
    
    // Attribut-Filter (Post-Filtering)
    struct AttributeFilter { field, value, op };
    std::pair<Status, std::vector<Result>> searchKnnWithFilter(query, k, filters);
    
    // Persistenz
    Status saveIndex(directory);
    Status loadIndex(directory);
};
```

### GraphIndexManager

```cpp
class GraphIndexManager {
    // Traversal
    enum class Direction { OUTBOUND, INBOUND, ANY };
    
    // API
    Status addEdge(from, to, type, properties);
    Status removeEdge(from, to, type);
    
    // Algorithmen
    std::vector<Path> bfs(start, maxDepth, direction, edgeFilter);
    std::vector<Path> dijkstra(start, end, weightField);
    std::vector<Path> astar(start, end, heuristic);
    
    // Graph Analytics
    double pagerank(nodeId);
    std::vector<Community> louvain();
    std::vector<std::string> shortestPath(from, to);
};
```

### PropertyGraph

```cpp
class PropertyGraph {
    // Nodes & Edges
    Status addNode(id, labels, properties);
    Status addEdge(from, to, type, properties);
    
    // Pattern Matching
    std::vector<Match> match(pattern);  // Cypher-ähnlich
    
    // Properties
    Status setProperty(id, key, value);
    Value getProperty(id, key);
};
```

### TemporalGraph

```cpp
class TemporalGraph {
    // Zeit-basierte Abfragen
    std::vector<Edge> getEdgesAt(timestamp);
    std::vector<Edge> getEdgesInRange(from, to);
    
    // Aggregation
    double aggregateEdgeProperty(node, edgeType, field, from, to, AggType);
};
```

## Performance-Metriken

| Index-Typ | Lookup | Insert | Memory |
|-----------|--------|--------|--------|
| Secondary (B-Tree) | O(log n) | O(log n) | ~100 bytes/entry |
| Vector (HNSW) | O(log n) | O(log n) | ~500 bytes/vector |
| Graph (Adjacency) | O(1) + O(degree) | O(1) | ~50 bytes/edge |
| Fulltext (Inverted) | O(k) k=terms | O(n) n=terms | ~200 bytes/doc |

## Verwandte Dokumentation

- [Features: Indexes](../features/features_indexes.md) - Feature-Übersicht
- [Features: Vector Operations](../features/features_vector_ops.md) - Vector Search
- [Query: Vector Hybrid Search](../query/query_vector_hybrid.md) - Hybrid Queries
- [Features: Property Graph](../features/features_property_graph.md) - Graph Features
