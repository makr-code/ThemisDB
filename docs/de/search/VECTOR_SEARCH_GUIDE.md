# Vector Search Guide - ThemisDB v1.4

**Version:** 1.4.0  
**Status:** ✅ Produktionsreif  
**Aktualisiert:** Januar 2026

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Vector Index (HNSW)](#vector-index-hnsw)
- [Similarity Metrics](#similarity-metrics)
- [Vector Queries](#vector-queries)
- [Embeddings](#embeddings)
- [Beispiele](#beispiele)
- [Performance](#performance)
- [Best Practices](#best-practices)

---

## Übersicht

Vector Search ermöglicht Similarity Search basierend auf hochdimensionalen Vektoren (Embeddings). ThemisDB verwendet HNSW (Hierarchical Navigable Small World) Indizes für schnelle Approximate Nearest Neighbor (ANN) Suche.

### Features

- ✅ **HNSW Index**: State-of-the-art ANN-Algorithmus
- ✅ **Multiple Metrics**: Cosine, Euclidean, Dot Product
- ✅ **High Dimensions**: Bis zu 2048 Dimensionen
- ✅ **Fast Queries**: Sub-100ms für Millionen Vektoren
- ✅ **Hybrid Support**: Kombinierbar mit Full-Text Search
- ✅ **Incremental Updates**: Dynamisches Hinzufügen neuer Vektoren

---

## Vector Index (HNSW)

### Index erstellen

```aql
// Einfacher Vector Index
CREATE VECTOR INDEX idx_embedding ON documents(embedding)
  DIMENSIONS 768
  METRIC cosine

// Mit HNSW-Parametern
CREATE VECTOR INDEX idx_embedding_tuned ON documents(embedding)
  DIMENSIONS 1536
  METRIC euclidean
  OPTIONS {
    M: 16,              // Anzahl bidirektionaler Links pro Layer
    efConstruction: 200, // Search Width während Index-Bau
    efSearch: 50         // Search Width bei Queries (default)
  }
```

### HNSW Parameter

| Parameter | Beschreibung | Default | Empfehlung |
|-----------|--------------|---------|------------|
| **M** | Anzahl Connections pro Layer | 16 | 16-32 für Balance |
| **efConstruction** | Search width beim Bauen | 200 | 100-400, höher = bessere Qualität |
| **efSearch** | Search width bei Queries | 50 | 50-200, höher = bessere Recall |
| **maxLayer** | Max Anzahl Layers | auto | Automatisch berechnet |

**Trade-offs:**
- **Höheres M**: Bessere Qualität, mehr Memory, langsamere Inserts
- **Höheres efConstruction**: Bessere Index-Qualität, langsamerer Build
- **Höheres efSearch**: Bessere Recall, langsamere Queries

---

## Similarity Metrics

### Cosine Similarity

**Use Case:** Text Embeddings, Semantic Search

**Formula:**
```
similarity(A, B) = (A · B) / (||A|| × ||B||)
Range: [-1, 1], höher = ähnlicher
```

**AQL:**
```aql
FOR doc IN documents
  LET similarity = COSINE_SIMILARITY(doc.embedding, @queryVector)
  FILTER similarity > 0.8
  SORT similarity DESC
  LIMIT 10
  RETURN {doc, similarity}
```

### Euclidean Distance

**Use Case:** Spatial Data, Feature Vectors

**Formula:**
```
distance(A, B) = √(Σ(Ai - Bi)²)
Range: [0, ∞], niedriger = ähnlicher
```

**AQL:**
```aql
FOR doc IN documents
  LET distance = EUCLIDEAN_DISTANCE(doc.embedding, @queryVector)
  FILTER distance < 5.0
  SORT distance ASC
  LIMIT 10
  RETURN {doc, distance}
```

### Dot Product

**Use Case:** Normalized Vectors, Scoring

**Formula:**
```
similarity(A, B) = A · B = Σ(Ai × Bi)
Range: [-∞, ∞], höher = ähnlicher (bei normalisierten Vektoren)
```

**AQL:**
```aql
FOR doc IN documents
  LET score = DOT_PRODUCT(doc.embedding, @queryVector)
  SORT score DESC
  LIMIT 10
  RETURN {doc, score}
```

---

## Vector Queries

### K-Nearest Neighbors (K-NN)

```aql
// Finde die 10 ähnlichsten Dokumente
FOR doc IN documents
  OPTIONS {indexHint: "idx_embedding"}
  FILTER VECTOR_DISTANCE(doc.embedding, @queryVector, "cosine") < 0.3
  SORT VECTOR_DISTANCE(doc.embedding, @queryVector, "cosine") ASC
  LIMIT 10
  RETURN doc
```

### Mit Pre-Filtering

```aql
// Nur veröffentlichte Artikel durchsuchen
FOR doc IN documents
  FILTER doc.status == "published"
  FILTER VECTOR_DISTANCE(doc.embedding, @queryVector, "cosine") < 0.4
  SORT VECTOR_DISTANCE(doc.embedding, @queryVector, "cosine") ASC
  LIMIT 20
  RETURN doc
```

### Mit Score Threshold

```aql
// Nur Ergebnisse über Similarity-Threshold
FOR doc IN documents
  LET similarity = COSINE_SIMILARITY(doc.embedding, @queryVector)
  FILTER similarity > 0.75
  SORT similarity DESC
  LIMIT 50
  RETURN {
    id: doc._id,
    title: doc.title,
    similarity: similarity
  }
```

### Batch Vector Search

```aql
// Multiple Query Vectors
LET queryVectors = @queryVectorBatch

FOR queryVec IN queryVectors
  LET results = (
    FOR doc IN documents
      LET similarity = COSINE_SIMILARITY(doc.embedding, queryVec)
      FILTER similarity > 0.7
      SORT similarity DESC
      LIMIT 5
      RETURN {doc, similarity}
  )
  RETURN {
    query: queryVec,
    results: results
  }
```

---

## Embeddings

### Embedding-Modelle

| Model | Dimensions | Use Case | Provider |
|-------|------------|----------|----------|
| **text-embedding-ada-002** | 1536 | General Text | OpenAI |
| **all-MiniLM-L6-v2** | 384 | Lightweight Text | Sentence Transformers |
| **bge-large-en-v1.5** | 1024 | English Text | BAAI |
| **multilingual-e5-large** | 1024 | Multi-Language | Microsoft |
| **CLIP** | 512 | Image + Text | OpenAI |
| **Custom** | Variable | Domain-specific | Fine-tuned |

### Embedding-Generierung

**Python (OpenAI):**
```python
import openai

def get_embedding(text, model="text-embedding-ada-002"):
    response = openai.Embedding.create(
        input=[text],
        model=model
    )
    return response['data'][0]['embedding']

# Beispiel
text = "Quantum computing explained for beginners"
embedding = get_embedding(text)
print(f"Embedding: {len(embedding)} dimensions")  # 1536
```

**Python (Sentence Transformers):**
```python
from sentence_transformers import SentenceTransformer

model = SentenceTransformer('all-MiniLM-L6-v2')

texts = [
    "Quantum computing is fascinating",
    "Machine learning transforms industries",
    "Deep learning neural networks"
]

embeddings = model.encode(texts)
print(f"Shape: {embeddings.shape}")  # (3, 384)
```

**JavaScript (TensorFlow.js):**
```javascript
const use = require('@tensorflow-models/universal-sentence-encoder');

async function getEmbedding(text) {
  const model = await use.load();
  const embeddings = await model.embed([text]);
  return embeddings.arraySync()[0];
}

// Usage
const embedding = await getEmbedding("Hello world");
console.log(`Dimensions: ${embedding.length}`);  // 512
```

### Embedding in ThemisDB speichern

```javascript
// Node.js Client
const embedding = await getEmbedding(document.content);

await db.query(`
  INSERT {
    title: @title,
    content: @content,
    embedding: @embedding,
    created_at: DATE_NOW()
  } INTO documents
`, {
  title: document.title,
  content: document.content,
  embedding: embedding
});
```

---

## Beispiele

### Beispiel 1: Semantic Document Search

```aql
// Semantische Suche in Dokumenten
LET queryText = "How does machine learning work?"
LET queryEmbedding = @queryEmbedding  // Generated with embedding model

FOR doc IN documents
  LET similarity = COSINE_SIMILARITY(doc.embedding, queryEmbedding)
  FILTER similarity > 0.75
  SORT similarity DESC
  LIMIT 10
  RETURN {
    id: doc._id,
    title: doc.title,
    summary: SUBSTRING(doc.content, 0, 200),
    similarity: ROUND(similarity * 100, 2),
    author: doc.author,
    date: doc.published_date
  }
```

**Bind Variables:**
```json
{
  "queryEmbedding": [0.012, -0.045, 0.128, ...]  // 768 or 1536 dimensions
}
```

**Result:**
```json
{
  "id": "docs/123",
  "title": "Introduction to Machine Learning",
  "summary": "Machine learning is a subset of artificial intelligence that enables systems to learn and improve from experience without being explicitly programmed...",
  "similarity": 92.5,
  "author": "Jane Smith",
  "date": "2026-01-15"
}
```

### Beispiel 2: Similar Product Recommendations

```aql
// Ähnliche Produkte finden
LET currentProduct = DOCUMENT("products", @productId)

FOR product IN products
  FILTER product._id != currentProduct._id
  FILTER product.category == currentProduct.category
  LET similarity = COSINE_SIMILARITY(product.feature_vector, currentProduct.feature_vector)
  FILTER similarity > 0.6
  SORT similarity DESC
  LIMIT 5
  RETURN {
    id: product._id,
    name: product.name,
    price: product.price,
    image: product.image_url,
    similarity: ROUND(similarity * 100, 2)
  }
```

### Beispiel 3: Image Search (CLIP Embeddings)

```aql
// Bilder mit ähnlichem Inhalt finden
FOR image IN images
  LET similarity = COSINE_SIMILARITY(image.clip_embedding, @queryImageEmbedding)
  FILTER similarity > 0.7
  SORT similarity DESC
  LIMIT 20
  RETURN {
    id: image._id,
    url: image.url,
    caption: image.caption,
    tags: image.tags,
    similarity: ROUND(similarity * 100, 2)
  }
```

---

## Performance

### Benchmark-Ergebnisse

**Test Setup:**
- Collection: 1M vectors
- Dimensions: 768 (text-embedding-ada-002)
- Index: HNSW (M=16, efConstruction=200)
- Hardware: 16 CPU cores, 64GB RAM

| K (Neighbors) | Avg. Latency | P95 Latency | Recall@10 |
|---------------|--------------|-------------|-----------|
| 5 | 12ms | 20ms | 0.98 |
| 10 | 18ms | 30ms | 0.97 |
| 50 | 45ms | 75ms | 0.96 |
| 100 | 85ms | 140ms | 0.95 |

### Scaling

| Dataset Size | Index Build Time | Index Size | Query Latency (K=10) |
|--------------|------------------|------------|----------------------|
| 100K | 45s | 150MB | 8ms |
| 1M | 8min | 1.5GB | 18ms |
| 10M | 90min | 15GB | 35ms |
| 100M | 18h | 150GB | 80ms |

### Optimization Tips

**1. Tune efSearch:**
```aql
// Higher efSearch = better recall, slower queries
FOR doc IN documents
  OPTIONS {
    vectorSearchOptions: {
      efSearch: 100  // Default: 50
    }
  }
  FILTER VECTOR_DISTANCE(doc.embedding, @queryVector, "cosine") < 0.3
  LIMIT 10
  RETURN doc
```

**2. Pre-Filter reduziert Search Space:**
```aql
// Filter auf Kategorie reduziert Candidates
FOR doc IN documents
  FILTER doc.category == "technology"  // Metadata filter
  FILTER VECTOR_DISTANCE(doc.embedding, @queryVector, "cosine") < 0.4
  LIMIT 10
  RETURN doc
```

**3. Batch Queries:**
```javascript
// Batch multiple queries to amortize overhead
const queries = [vector1, vector2, vector3];
const results = await Promise.all(
  queries.map(vec => db.query(vectorQuery, {queryVector: vec}))
);
```

---

## Best Practices

### 1. Normalisierung

```python
# Normalize vectors für Cosine Similarity
import numpy as np

def normalize(vector):
    norm = np.linalg.norm(vector)
    return vector / norm if norm > 0 else vector

embedding = get_embedding(text)
normalized_embedding = normalize(embedding)
```

### 2. Dimension Reduction

```python
# Reduce dimensions mit PCA (bei Bedarf)
from sklearn.decomposition import PCA

pca = PCA(n_components=384)
reduced_embeddings = pca.fit_transform(embeddings)
```

### 3. Embedding Cache

```javascript
// Cache Embeddings to avoid recomputation
const embeddingCache = new Map();

async function getCachedEmbedding(text) {
  if (!embeddingCache.has(text)) {
    const embedding = await getEmbedding(text);
    embeddingCache.set(text, embedding);
  }
  return embeddingCache.get(text);
}
```

### 4. Batch Insert

```javascript
// Insert vectors in batches
const batchSize = 1000;
for (let i = 0; i < documents.length; i += batchSize) {
  const batch = documents.slice(i, i + batchSize);
  await db.query(`
    FOR doc IN @batch
      INSERT doc INTO documents
  `, {batch});
}
```

### 5. Monitor Index Health

```aql
// Check index statistics
FOR index IN _indexes
  FILTER index.type == "vector"
  RETURN {
    name: index.name,
    size: index.size,
    figures: index.figures
  }
```

---

## Use Cases

### 1. Semantic Search Engine

```javascript
// Complete semantic search pipeline
async function semanticSearch(query, k = 10) {
  // 1. Generate query embedding
  const queryEmbedding = await getEmbedding(query);
  
  // 2. Vector search
  const results = await db.query(`
    FOR doc IN documents
      LET similarity = COSINE_SIMILARITY(doc.embedding, @queryVector)
      FILTER similarity > 0.7
      SORT similarity DESC
      LIMIT @k
      RETURN {
        id: doc._id,
        title: doc.title,
        content: doc.content,
        similarity: similarity
      }
  `, {queryVector: queryEmbedding, k});
  
  return results;
}

// Usage
const results = await semanticSearch("explain quantum computing");
```

### 2. Duplicate Detection

```aql
// Finde potentielle Duplikate
FOR doc1 IN documents
  FOR doc2 IN documents
    FILTER doc1._id < doc2._id  // Avoid duplicates
    LET similarity = COSINE_SIMILARITY(doc1.embedding, doc2.embedding)
    FILTER similarity > 0.95  // Very similar
    RETURN {
      doc1: {id: doc1._id, title: doc1.title},
      doc2: {id: doc2._id, title: doc2.title},
      similarity: similarity
    }
```

### 3. Content Recommendation

```javascript
// Personalized content recommendations
async function getRecommendations(userId, k = 20) {
  // Get user's interaction history
  const user = await db.query(`
    FOR u IN users
      FILTER u._id == @userId
      RETURN u
  `, {userId});
  
  // Generate user preference vector (average of liked items)
  const userVector = computeAverageVector(user.liked_embeddings);
  
  // Find similar content
  const recommendations = await db.query(`
    FOR doc IN documents
      FILTER doc._id NOT IN @excludeIds
      LET similarity = COSINE_SIMILARITY(doc.embedding, @userVector)
      FILTER similarity > 0.6
      SORT similarity DESC
      LIMIT @k
      RETURN {
        id: doc._id,
        title: doc.title,
        score: similarity
      }
  `, {
    userVector,
    excludeIds: user.viewed_items,
    k
  });
  
  return recommendations;
}
```

---

## Siehe auch

- [Full-Text Search Guide](FULLTEXT_SEARCH_GUIDE.md)
- [Hybrid Search Guide](HYBRID_SEARCH_GUIDE.md)
- [Search Feature Matrix](SEARCH_FEATURE_MATRIX.md)
- [Embedding Models Guide](../llm/EMBEDDING_MODELS.md)
- [Performance Tuning](performance_tuning.md)
