> ⚠️ **Historische Messdaten** – Die in diesem Dokument enthaltenen Zahlen entstammen einem bestimmten Messzeitpunkt und sind nicht mehr reproduzierbar ohne die ursprüngliche Testumgebung.
> Für reproduzierbare Ergebnisse: Benchmark-Kommandos und aktuelle CMake-Presets unter [`benchmarks/README.md`](../README.md) verwenden.

# Multi-Modal Database Benchmark with Embeddings (MMDB-E)
## Specialized Benchmark for ThemisDB

**Version:** 1.0  
**Date:** 2025-12-23  
**Status:** Design & Implementation

---

## Executive Summary

Existing benchmarks (TPC-C, YCSB, LDBC) focus on single data models. Modern applications require **multi-modal databases** that seamlessly integrate:
- **Relational data** (structured records)
- **Documents** (JSON, nested structures)
- **Graphs** (relationships, traversals)
- **Vectors** (embeddings for semantic search)
- **LLM Integration** (RAG, semantic queries)

This benchmark addresses the **gap** in testing multi-modal databases with AI/ML workloads.

---

## 1. Problem Statement

### Current Benchmark Limitations

**TPC-C:**
- ✅ Excellent for OLTP transactions
- ❌ No vector/embedding support
- ❌ No graph traversal
- ❌ No document model

**YCSB:**
- ✅ Good for key-value workloads
- ❌ Simple data model (flat records)
- ❌ No semantic search
- ❌ No multi-modal queries

**LDBC:**
- ✅ Excellent for graph workloads
- ❌ No vector embeddings
- ❌ Limited to social network model

### What's Missing?

Modern applications need to:
1. Store product data (relational)
2. Store product descriptions (documents)
3. Connect products to categories (graph)
4. Search by semantic similarity (vectors)
5. Answer natural language questions (LLM/RAG)

**No existing benchmark tests this combined workload!**

---

## 2. Benchmark Design: MMDB-E

### 2.1 Core Concept

**MMDB-E (Multi-Modal Database Benchmark with Embeddings)** simulates a real-world **E-Commerce + Knowledge Base** system with AI features.

### 2.2 Data Models

#### **Model 1: Relational (Products)**
```sql
Products (
    product_id INT PRIMARY KEY,
    name VARCHAR(200),
    price DECIMAL(10,2),
    stock INT,
    category_id INT,
    brand VARCHAR(100),
    rating DECIMAL(3,2)
)
```

#### **Model 2: Document (Product Details)**
```json
{
    "product_id": 12345,
    "description": "High-quality wireless headphones...",
    "specifications": {
        "battery_life": "30 hours",
        "connectivity": ["Bluetooth 5.0", "USB-C"],
        "weight": "250g"
    },
    "reviews": [
        {"user": "alice", "rating": 5, "text": "Great sound!"},
        {"user": "bob", "rating": 4, "text": "Good value"}
    ],
    "metadata": {
        "manufacturer": "TechCorp",
        "warranty": "2 years"
    }
}
```

#### **Model 3: Graph (Relationships)**
```
Nodes:
- Product
- Category
- Brand
- User
- Tag

Edges:
- Product --[BELONGS_TO]--> Category
- Product --[MANUFACTURED_BY]--> Brand
- Product --[SIMILAR_TO]--> Product
- User --[PURCHASED]--> Product
- User --[REVIEWED]--> Product
- Product --[TAGGED_WITH]--> Tag
```

#### **Model 4: Vector (Embeddings)**
```cpp
struct ProductEmbedding {
    product_id: int
    text_embedding: float[768]      // Description embedding (e.g., BERT)
    image_embedding: float[512]     // Product image embedding (e.g., ResNet)
    combined_embedding: float[1024] // Multimodal fusion
}
```

### 2.3 Workload Categories

#### **Workload 1: Hybrid CRUD (30%)**
Operations combining multiple models:

**H1: Product Lookup with Details**
```
1. SELECT * FROM Products WHERE product_id = ?    (Relational)
2. GET document:product:?                          (Document)
3. MATCH (p:Product {id:?})-[:SIMILAR_TO]->(s)    (Graph - similar products)
4. VECTOR_SEARCH(embedding, k=5)                   (Vector - visually similar)
```

**H2: User Purchase with Recommendations**
```
1. INSERT INTO Orders (user_id, product_id, ...)  (Relational)
2. UPDATE Products SET stock = stock - 1           (Relational)
3. CREATE (u:User)-[:PURCHASED]->(p:Product)       (Graph)
4. RECOMMEND_SIMILAR(product_embedding, k=10)      (Vector)
```

**H3: Product Search by Description**
```
1. Generate query_embedding from text              (LLM)
2. VECTOR_SEARCH(query_embedding, k=20)           (Vector - semantic search)
3. Filter by category, price range                 (Relational)
4. Enrich with reviews from documents              (Document)
```

#### **Workload 2: Semantic Search (25%)**
Real-world AI-powered search:

**S1: Text Similarity Search**
```
Query: "wireless headphones with noise cancellation"
1. Embed query with sentence-transformer
2. ANN search in product embeddings
3. Re-rank with hybrid score (vector + metadata)
4. Return top-k with full details
```

**S2: Image Similarity Search**
```
Query: Upload product image
1. Extract image embedding (e.g., CLIP)
2. ANN search in image embeddings
3. Filter by category/price
4. Return visually similar products
```

**S3: Multimodal Search**
```
Query: "red sports car under $50k"
1. Text embedding + attribute filters
2. Combined vector + filter search
3. Graph traversal for related models
4. Aggregate results
```

#### **Workload 3: Graph Traversal (20%)**
Relationship queries:

**G1: Product Recommendations (Collaborative Filtering)**
```
MATCH (u:User {id:?})-[:PURCHASED]->(p:Product)<-[:PURCHASED]-(other:User)
      (other)-[:PURCHASED]->(rec:Product)
WHERE NOT (u)-[:PURCHASED]->(rec)
RETURN rec ORDER BY COUNT(*) DESC LIMIT 10
```

**G2: Category Exploration**
```
MATCH (p:Product)-[:BELONGS_TO]->(c:Category)
      (c)<-[:PARENT_OF*1..3]-(root:Category)
RETURN p, c, root
```

**G3: Influence Propagation**
```
MATCH (u:User)-[:REVIEWED]->(p:Product)
WHERE u.influence_score > 0.8
RETURN p, AVG(review.rating) as weighted_rating
```

#### **Workload 4: RAG (Retrieval Augmented Generation) (15%)**
LLM integration:

**R1: Question Answering**
```
Question: "What are the best wireless headphones under $200 with good battery life?"
1. Extract semantic query: "wireless headphones", "$200", "battery life"
2. Vector search for relevant products
3. Retrieve detailed documents
4. LLM prompt: "Based on these products: {context}, answer: {question}"
5. Return generated answer + source products
```

**R2: Product Comparison**
```
Question: "Compare Product A vs Product B"
1. Retrieve both product documents
2. Retrieve specifications and reviews
3. LLM generates structured comparison
4. Return comparison table + summary
```

**R3: Personalized Recommendation Explanation**
```
Question: "Why are you recommending this product to me?"
1. Retrieve user purchase history (graph)
2. Find similar products (vector)
3. Extract key features (document)
4. LLM generates natural language explanation
```

#### **Workload 5: Aggregation & Analytics (10%)**
Multi-modal analytics:

**A1: Semantic Category Analysis**
```
1. Cluster product embeddings by category
2. Calculate average ratings per cluster
3. Identify trending products (time-series + vector similarity)
4. Generate insights with LLM
```

**A2: Cross-Model Join**
```
SELECT p.product_id, p.name, d.description, COUNT(r.user_id) as reviews
FROM Products p
JOIN Documents d ON p.product_id = d.product_id
LEFT JOIN Graph_Edges r ON p.product_id = r.target_id
WHERE VECTOR_DISTANCE(p.embedding, query_embedding) < 0.5
GROUP BY p.product_id
```

---

## 3. Technical Implementation

### 3.1 Data Generation

**Dataset Size:**
- **Small:** 10K products, 100K documents, 500K graph edges, 10K embeddings
- **Medium:** 100K products, 1M documents, 5M edges, 100K embeddings
- **Large:** 1M products, 10M documents, 50M edges, 1M embeddings

**Data Characteristics:**
- **Realistic product names:** Generated with language models
- **Embeddings:** Pre-computed with sentence-transformers (768-dim)
- **Graph density:** 10-20 edges per product
- **Documents:** Nested JSON with 5-10 fields

### 3.2 Metrics

**Primary Metrics:**
1. **Throughput (ops/sec):**
   - Hybrid queries: 10,000-20,000 ops/sec
   - Semantic search: 5,000-10,000 ops/sec
   - Graph traversal: 1,000-5,000 ops/sec
   - RAG queries: 100-500 ops/sec (LLM-bound)

2. **Latency (p50, p95, p99):**
   - Hybrid queries: < 10ms (p95)
   - Semantic search: < 50ms (p95)
   - Graph traversal: < 100ms (p95)
   - RAG queries: < 2s (p95)

3. **Accuracy:**
   - Vector search recall@10: > 95%
   - Graph traversal correctness: 100%
   - RAG answer quality: Human evaluation or LLM-as-judge

**Secondary Metrics:**
- Memory usage per query
- Index build time
- Multi-model query optimization efficiency
- Cache hit rates

### 3.3 Test Scenarios

**Scenario 1: Cold Start**
- Empty caches
- Measure index loading time
- Baseline performance

**Scenario 2: Hot Data**
- Pre-warmed caches
- Focus on query execution
- Maximum throughput

**Scenario 3: Mixed Workload**
- Combine all workload types
- Realistic usage pattern
- Measure resource contention

**Scenario 4: Scale-Out**
- Test with increasing data sizes
- Measure scaling efficiency
- Identify bottlenecks

**Scenario 5: Concurrent Users**
- 10, 100, 1000 concurrent clients
- Measure throughput degradation
- Test isolation and consistency

---

## 4. Performance Targets

### 4.1 ThemisDB Targets (8-core, 32GB, NVMe)

| Workload | Target Throughput | Target Latency (p95) |
|----------|------------------|---------------------|
| Hybrid CRUD | 15,000 ops/sec | < 10ms |
| Semantic Search | 8,000 ops/sec | < 50ms |
| Graph Traversal | 3,000 ops/sec | < 100ms |
| RAG Queries | 200 ops/sec | < 2s |
| Analytics | 500 queries/sec | < 500ms |

### 4.2 Comparison with Competitors

| Database | Multi-Modal? | Vector Search | Graph | RAG | Score |
|----------|-------------|---------------|-------|-----|-------|
| **ThemisDB** | ✅ | ✅ | ✅ | ✅ | **Target** |
| PostgreSQL + pgvector | ⚠️ | ✅ | ❌ | ⚠️ | Baseline |
| MongoDB + Atlas Search | ⚠️ | ✅ | ⚠️ | ❌ | 70% |
| Neo4j + Vector Index | ⚠️ | ⚠️ | ✅ | ❌ | 65% |
| Elasticsearch | ❌ | ✅ | ❌ | ⚠️ | 50% |

---

## 5. Implementation Roadmap

### Phase 1: Data Generator (Week 1)
- [ ] Generate synthetic product catalog
- [ ] Create embeddings with sentence-transformers
- [ ] Build graph relationships
- [ ] Generate JSON documents

### Phase 2: Core Workloads (Week 2)
- [ ] Implement Hybrid CRUD operations
- [ ] Implement Semantic Search
- [ ] Implement Graph Traversal
- [ ] Basic metrics collection

### Phase 3: Advanced Workloads (Week 3)
- [ ] Implement RAG workflows
- [ ] Implement Analytics queries
- [ ] LLM integration (local or API)

### Phase 4: Validation & Tuning (Week 4)
- [ ] Run full benchmark suite
- [ ] Compare with baselines
- [ ] Optimize and iterate
- [ ] Generate report

---

## 6. Expected Impact

### For ThemisDB
1. **Differentiation:** First benchmark specifically for multi-modal AI databases
2. **Validation:** Prove ThemisDB's multi-model capabilities
3. **Optimization:** Identify performance bottlenecks
4. **Marketing:** Show leadership in AI database benchmarking

### For Industry
1. **Standard:** Establish new benchmark for AI databases
2. **Comparison:** Enable fair comparison of multi-modal systems
3. **Research:** Contribute to database research community
4. **Innovation:** Drive innovation in database design

---

## 7. Next Steps

1. **Approve Design:** Review and approve benchmark specification
2. **Implement Generator:** Create data generation tool
3. **Build Benchmarks:** Implement workloads in C++ with Google Benchmark
4. **Run Tests:** Execute on ThemisDB
5. **Publish Results:** Share findings with community

---

## References

1. **TPC-C:** http://www.tpc.org/tpcc/
2. **YCSB:** Cooper et al., SoCC 2010
3. **LDBC:** Erling et al., SIGMOD 2015
4. **ANN-Benchmarks:** Aumüller et al., SISAP 2017
5. **RAG:** Lewis et al., "Retrieval-Augmented Generation", NeurIPS 2020
6. **Multi-Modal DBs:** Garcia-Molina, "Database Systems: The Complete Book"

---

**Status:** ✅ Design Complete - Ready for Implementation  
**Next:** Implement data generator and core workloads
