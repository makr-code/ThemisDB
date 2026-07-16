# 🔍 Search Module - ThemisDB v1.4

**Kategorie:** Core Search  
**Version:** v1.4.0  
**Status:** ✅ Produktionsreif  
**Datum:** Januar 2026

---

## 📑 Inhaltsverzeichnis

- [📋 Übersicht](#-übersicht)
- [✨ Search-Methoden](#-search-methoden)
- [🚀 Schnellstart](#-schnellstart)
- [📖 Detaillierte Dokumentation](#-detaillierte-dokumentation)
- [💡 Best Practices](#-best-practices)
- [📊 Performance-Vergleich](#-performance-vergleich)
- [📚 Siehe auch](#-siehe-auch)

---

## 📋 Übersicht

ThemisDB bietet drei leistungsstarke Search-Methoden: **Full-Text Search**, **Vector Search** und **Hybrid Search**. Jede Methode hat spezifische Stärken und Use Cases.

### Vollständige Search-Dokumentation (v1.4)

- **[Full-Text Search Guide](FULLTEXT_SEARCH_GUIDE.md)** - Inverted Index, BM25 Ranking, Analyzers, Fuzzy Search, Phrase Search mit 3 realen Use Cases
- **[Vector Search Guide](VECTOR_SEARCH_GUIDE.md)** - HNSW Index, Similarity Metrics, Embeddings, Semantic Search mit 3 Beispielen
- **[Hybrid Search Guide](HYBRID_SEARCH_GUIDE.md)** - Fusion Methods, Adaptive Strategies, Multi-Signal Ranking mit 3 kompletten Implementierungen
- **[Search Feature Matrix](SEARCH_FEATURE_MATRIX.md)** - Vollständiger Vergleich, Decision Tree, Benchmarks, Limitations

---

## ✨ Search-Methoden

### 1. Full-Text Search

**Best für:** Exakte Keyword-Matches, Boolean Queries, Phrase Search

**Features:**
- ✅ BM25 Ranking
- ✅ Multi-Language Analyzers (DE, EN, FR, ES, etc.)
- ✅ Fuzzy Matching (Tippfehlertoleranz)
- ✅ Phrase Search
- ✅ Boolean Operators (AND, OR, NOT)
- ✅ Schnell (10-50ms)

**Quickstart:**
```aql
// Create index
CREATE FULLTEXT INDEX idx_content ON articles(content) ANALYZER "text_en"

// Search
FOR doc IN articles
  SEARCH ANALYZER(doc.content IN TOKENS(@query, "text_en"), "text_en")
  LET score = BM25(doc)
  SORT score DESC
  LIMIT 20
  RETURN {doc, score}
```

### 2. Vector Search

**Best für:** Semantic Search, Ähnlichkeitssuche, Multi-Modal Search

**Features:**
- ✅ HNSW Index (State-of-the-art ANN)
- ✅ Cosine, Euclidean, Dot Product Metrics
- ✅ Bis zu 2048 Dimensionen
- ✅ Sub-100ms Queries (bei Millionen Vektoren)
- ✅ Automatische Synonym-Erkennung
- ✅ Cross-Lingual Support

**Quickstart:**
```aql
// Create index
CREATE VECTOR INDEX idx_embedding ON articles(embedding)
  DIMENSIONS 768
  METRIC cosine

// Search
FOR doc IN articles
  LET similarity = COSINE_SIMILARITY(doc.embedding, @queryEmbedding)
  FILTER similarity > 0.7
  SORT similarity DESC
  LIMIT 20
  RETURN {doc, similarity}
```

### 3. Hybrid Search

**Best für:** Best of Both Worlds - Keywords + Semantik

**Features:**
- ✅ Kombiniert Full-Text + Vector
- ✅ Bessere Recall und Precision
- ✅ Multi-Signal Ranking
- ✅ Adaptive Fusion Strategies
- ✅ Robust gegen verschiedene Query-Typen

**Quickstart:**
```aql
// Full-Text Branch
LET ftResults = (
  FOR doc IN articles
    SEARCH ANALYZER(doc.content IN TOKENS(@query, "text_en"), "text_en")
    LET ftScore = BM25(doc)
    RETURN {doc, ftScore}
)

// Vector Branch
LET vecResults = (
  FOR doc IN articles
    LET vecScore = COSINE_SIMILARITY(doc.embedding, @queryEmbedding)
    FILTER vecScore > 0.7
    RETURN {doc, vecScore}
)

// Combine
FOR result IN UNION(ftResults, vecResults)
  COLLECT doc = result.doc
  AGGREGATE
    ftScore = MAX(result.ftScore || 0),
    vecScore = MAX(result.vecScore || 0)
  LET hybridScore = (ftScore * 0.4) + (vecScore * 0.6)
  SORT hybridScore DESC
  LIMIT 20
  RETURN {doc, ftScore, vecScore, hybridScore}
```

---

## 🚀 Schnellstart

### Full-Text Search Example

```javascript
// 1. Create index
await db.query(`
  CREATE FULLTEXT INDEX idx_blog ON blog_posts(title, content)
    ANALYZER "text_en"
`);

// 2. Insert documents
await db.query(`
  FOR post IN @posts
    INSERT post INTO blog_posts
`, {posts: blogPosts});

// 3. Search
const results = await db.query(`
  FOR doc IN blog_posts
    SEARCH ANALYZER(
      BOOST(doc.title IN TOKENS(@query, "text_en"), 3.0) OR
      doc.content IN TOKENS(@query, "text_en"),
      "text_en"
    )
    LET score = BM25(doc)
    SORT score DESC
    LIMIT 10
    RETURN {doc, score}
`, {query: "machine learning"});
```

### Vector Search Example

```python
from sentence_transformers import SentenceTransformer

# 1. Generate embeddings
model = SentenceTransformer('all-MiniLM-L6-v2')
texts = ["Quantum computing explained", "Machine learning basics"]
embeddings = model.encode(texts)

# 2. Insert with embeddings
for i, text in enumerate(texts):
    await db.query("""
        INSERT {
            content: @content,
            embedding: @embedding
        } INTO articles
    """, {
        "content": text,
        "embedding": embeddings[i].tolist()
    })

# 3. Search
query_embedding = model.encode("quantum physics")
results = await db.query("""
    FOR doc IN articles
      LET similarity = COSINE_SIMILARITY(doc.embedding, @queryEmbedding)
      FILTER similarity > 0.7
      SORT similarity DESC
      LIMIT 5
      RETURN {doc, similarity}
""", {"queryEmbedding": query_embedding.tolist()})
```

---

## 📖 Detaillierte Dokumentation

### Complete Guides

| Guide | Beschreibung | Seiten |
|-------|--------------|--------|
| [Full-Text Search](FULLTEXT_SEARCH_GUIDE.md) | Inverted Index, BM25, Analyzers, Fuzzy, Phrase | ~60 |
| [Vector Search](VECTOR_SEARCH_GUIDE.md) | HNSW, Similarity Metrics, Embeddings | ~70 |
| [Hybrid Search](HYBRID_SEARCH_GUIDE.md) | Fusion Methods, Adaptive Strategies | ~80 |
| [Feature Matrix](SEARCH_FEATURE_MATRIX.md) | Comparison, Decision Tree, Benchmarks | ~55 |

### Code Examples

Jeder Guide enthält:
- ✅ 3+ komplette Use Case Implementierungen
- ✅ Production-ready Code in Python, JavaScript, AQL
- ✅ Performance-Benchmarks
- ✅ Best Practices und Common Pitfalls

---

## 💡 Best Practices

### Wann welche Methode?

**Full-Text Search:**
- ✅ Exakte Keywords wichtig
- ✅ Boolean Logic erforderlich
- ✅ Low Latency kritisch (< 50ms)
- ✅ Code/Technical Docs

**Vector Search:**
- ✅ Semantik wichtiger als Keywords
- ✅ Multi-Language Support
- ✅ Recommendation Systems
- ✅ Image/Audio Search

**Hybrid Search:**
- ✅ Best of Both Worlds
- ✅ E-Commerce
- ✅ Enterprise Search
- ✅ High Quality Requirements

### Performance Tips

1. **Index nur benötigte Felder**
2. **Tune HNSW Parameters** (M, efConstruction, efSearch)
3. **Use Pre-Filtering** (Metadata filter vor Vector Search)
4. **Enable Caching** für häufige Queries
5. **Monitor Query Latency** mit Prometheus/Grafana

---

## 📊 Performance-Vergleich

**Test Setup:** 1M Dokumente, 16 CPU cores, 64GB RAM

| Method | Avg Latency | Throughput | Recall@20 | NDCG@20 |
|--------|-------------|------------|-----------|---------|
| Full-Text | 25ms | 600 QPS | 0.72 | 0.68 |
| Vector | 35ms | 400 QPS | 0.78 | 0.74 |
| **Hybrid** | **55ms** | **250 QPS** | **0.89** | **0.85** |

**Trade-offs:**
- Full-Text: Schnellste, aber begrenzte Semantik
- Vector: Beste Semantik, höhere Latenz
- Hybrid: Beste Qualität, höchste Latenz

### Index Size Overhead

| Method | Index Size | Build Time (1M docs) |
|--------|------------|----------------------|
| Full-Text | ~15-30% | 2-5 min |
| Vector | ~50-100% | 8-12 min |
| Hybrid | ~65-130% | 10-17 min |

---

## 📚 Siehe auch

### Related Documentation
- [AQL Syntax Guide](../aql/AQL_SYNTAX_GUIDE.md) - Query Language Reference
- [API Specifications](../apis/) - REST, gRPC, GraphQL APIs
- [Performance Tuning](performance_tuning.md) - Advanced Optimization
- [Examples](../../examples/) - Real-world Use Cases

### External Resources
- [BM25 Algorithm](https://en.wikipedia.org/wiki/Okapi_BM25)
- [HNSW Paper](https://arxiv.org/abs/1603.09320)
- [Sentence Transformers](https://www.sbert.net/)
- [OpenAI Embeddings](https://platform.openai.com/docs/guides/embeddings)

---

## 🔧 Zusätzliche Files

| Datei | Beschreibung |
|-------|--------------|
| [fulltext_api.md](fulltext_api.md) | Full-Text API Reference |
| [hybrid_fusion_api.md](hybrid_fusion_api.md) | Hybrid Fusion API |
| [performance_tuning.md](performance_tuning.md) | Performance Optimization |
| [migration_guide.md](migration_guide.md) | Migration from older versions |
| [stemming.md](stemming.md) | Stemming Configuration |

---

**Status:** ✅ All documentation complete and production-ready  
**Last Updated:** April 2026  
**Version:** 1.4.0
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
