---
name: "[Ethics AI] Vector Search Integration"
about: Integrate Ethics AI with VectorIndexManager for semantic search
title: "[Ethics AI] Integrate vector search for argument similarity"
labels: ethics-ai, enhancement, vector-search
assignees: ''
---

## 🎯 Objective

Integrate Ethics AI Plugin with ThemisDB's VectorIndexManager to enable semantic similarity search for arguments and dilemmas.

## 📋 Background

Currently, `ETHICS_FIND_SIMILAR_DILEMMAS()` function is stubbed. Need to:
- Generate embeddings for arguments/dilemmas
- Store embeddings in VectorIndexManager
- Implement semantic similarity search
- Support filtered vector queries

## 🔧 Tasks

### Embedding Generation

- [ ] Choose embedding model (sentence-transformers, etc.)
- [ ] Implement embedding generation for argument text
- [ ] Add embedding field to BaseEntity for arguments
- [ ] Batch embedding generation for existing arguments
- [ ] Handle multi-language embeddings

### VectorIndexManager Integration

- [ ] Create vector index for ethics arguments
- [ ] Store embeddings when arguments are created
- [ ] Update embeddings when arguments are modified
- [ ] Delete embeddings when arguments are removed
- [ ] Configure vector dimension and distance metric

### Similarity Search Implementation

- [ ] Implement `ETHICS_FIND_SIMILAR_DILEMMAS()` using VectorIndexManager
- [ ] Support threshold-based filtering
- [ ] Add philosophy school filtering
- [ ] Support argument type filtering
- [ ] Return ranked results with similarity scores

### API Endpoint Enhancement

- [ ] Update POST /ethics/arguments/search to use vector search
- [ ] Add embedding generation for query text
- [ ] Support hybrid search (vector + filters)
- [ ] Add pagination for results

### Performance Optimization

- [ ] Benchmark vector search performance
- [ ] Add caching for frequent queries
- [ ] Optimize index parameters (HNSW, IVF, etc.)
- [ ] Monitor index size and query latency

## ✅ Acceptance Criteria

- [ ] Embeddings generated for all arguments
- [ ] VectorIndexManager integrated
- [ ] Similarity search returns relevant results
- [ ] Query latency < 50ms (p95)
- [ ] Tests pass
- [ ] Documentation updated

## 🧪 Testing

```cpp
TEST(EthicsVectorSearch, FindSimilarArguments) {
    // Store argument with embedding
    EthicalArgument arg;
    arg.content = "All persons have inherent dignity";
    auto embedding = generateEmbedding(arg.content);
    store.storeArgumentWithEmbedding(arg, embedding);
    
    // Search for similar
    auto query_embedding = generateEmbedding("human dignity");
    auto results = store.findSimilar(query_embedding, 0.7, 10);
    
    ASSERT_GT(results.size(), 0);
    ASSERT_EQ(results[0].id, arg.id);
}
```

## 📚 References

- VectorIndexManager: `include/storage/vector_index_manager.h`
- Vector API handler: `include/server/vector_api_handler.h`
- Ethics storage: `plugins/ethics_ai/argument_store.cpp`

## ⏱️ Estimated Effort

**Total:** 8-10 hours

- Embedding generation: 2-3 hours
- VectorIndexManager integration: 3-4 hours
- Search implementation: 2-3 hours
- Testing & optimization: 2-3 hours

## 🏷️ Labels

- `ethics-ai`: Ethics AI Plugin feature
- `enhancement`: New capability
- `vector-search`: Vector functionality
