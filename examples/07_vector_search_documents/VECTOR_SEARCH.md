> **Hinweis:** Algorithmen und Typen gegen aktuellen Sourcecode verifizieren.

# Vector Search Documents - Technische Details

## Übersicht

Dieser Guide erklärt die technischen Details der Vector Search-Implementierung mit ThemisDB, einschließlich Embeddings, Similarity Search und RAG-Workflows.

## Vector Embeddings

### Was sind Embeddings?

Embeddings sind numerische Repräsentationen von Text (oder anderen Daten) als Vektoren in einem hochdimensionalen Raum.

**Beispiel**:
```
Text: "ThemisDB ist eine Datenbank"
Embedding: [0.12, -0.45, 0.78, ..., 0.34]  # 384 Dimensionen
```

**Eigenschaften**:
- Semantisch ähnliche Texte haben ähnliche Vektoren
- Distance-Metriken messen Ähnlichkeit
- Dimensionalität typisch 384-1536

### Embedding-Modelle

**sentence-transformers (verwendet)**:
```python
from sentence_transformers import SentenceTransformer

model = SentenceTransformer('all-MiniLM-L6-v2')

# Text → Embedding
text = "Dokumentensuche mit Vektoren"
embedding = model.encode(text)
print(embedding.shape)  # (384,)
```

**Modell-Vergleich**:

| Modell | Dimensionen | Speed | Qualität | Use-Case |
|--------|-------------|-------|----------|----------|
| all-MiniLM-L6-v2 | 384 | Sehr schnell | Gut | General Purpose |
| all-mpnet-base-v2 | 768 | Mittel | Sehr gut | High Quality |
| paraphrase-multilingual | 768 | Mittel | Gut | Mehrsprachig |
| distiluse-base | 512 | Schnell | Gut | Semantic Search |

**Modell laden**:
```python
class EmbeddingGenerator:
    def __init__(self, model_name='all-MiniLM-L6-v2'):
        self.model = SentenceTransformer(model_name)
        self.dimensions = self.model.get_sentence_embedding_dimension()
    
    def generate(self, text):
        """Generiert Embedding für Text"""
        return self.model.encode(text, convert_to_numpy=True)
    
    def generate_batch(self, texts, batch_size=32):
        """Effiziente Batch-Verarbeitung"""
        return self.model.encode(
            texts,
            batch_size=batch_size,
            show_progress_bar=True,
            convert_to_numpy=True
        )
```

## Similarity Metrics

### 1. Cosine Similarity

**Definition**: Winkel zwischen zwei Vektoren

```python
import numpy as np

def cosine_similarity(vec1, vec2):
    """Berechnet Cosine Similarity [-1, 1]"""
    dot_product = np.dot(vec1, vec2)
    norm1 = np.linalg.norm(vec1)
    norm2 = np.linalg.norm(vec2)
    
    if norm1 == 0 or norm2 == 0:
        return 0.0
    
    return dot_product / (norm1 * norm2)
```

**Interpretation**:
- 1.0 = Identisch (gleiche Richtung)
- 0.0 = Orthogonal (unabhängig)
- -1.0 = Gegensätzlich (entgegengesetzte Richtung)

**Vorteile**:
- Unabhängig von Vector-Magnitude
- Gut für Text-Ähnlichkeit
- Standard in NLP

### 2. Euclidean Distance

**Definition**: Direkte Distanz im Raum

```python
def euclidean_distance(vec1, vec2):
    """Berechnet Euklidische Distanz [0, ∞]"""
    return np.linalg.norm(vec1 - vec2)
```

**Interpretation**:
- 0 = Identisch
- Höherer Wert = Unähnlicher

**Nachteile**:
- Sensitive zu Vector-Magnitude
- Nicht ideal für normalisierte Embeddings

### 3. Dot Product

**Definition**: Skalarprodukt der Vektoren

```python
def dot_product_similarity(vec1, vec2):
    """Berechnet Dot Product"""
    return np.dot(vec1, vec2)
```

**Verwendung**:
- Schnell zu berechnen
- Für normalisierte Vektoren äquivalent zu Cosine

### Similarity Score Normalisierung

```python
def normalize_score(similarity, metric='cosine'):
    """Normalisiert Score auf [0, 1]"""
    if metric == 'cosine':
        # Cosine [-1, 1] → [0, 1]
        return (similarity + 1) / 2
    elif metric == 'euclidean':
        # Euclidean [0, ∞] → [0, 1]
        # Annahme: max_distance bekannt
        max_distance = 2.0  # Für normalized vectors
        return 1 - min(similarity / max_distance, 1.0)
    return similarity
```

## Vector Search Implementierung

### Naive Linear Search

**Für kleine Datasets (< 10.000 Dokumente)**:

```python
def linear_search(query_embedding, document_embeddings, top_k=10):
    """Brute-Force Suche durch alle Vektoren"""
    similarities = []
    
    for doc_id, doc_embedding in document_embeddings.items():
        similarity = cosine_similarity(query_embedding, doc_embedding)
        similarities.append((doc_id, similarity))
    
    # Sortiere nach Similarity (absteigend)
    similarities.sort(key=lambda x: x[1], reverse=True)
    
    return similarities[:top_k]
```

**Komplexität**: O(n × d)
- n = Anzahl Dokumente
- d = Dimensionen

### Optimierte Vector Search

**Mit NumPy Vectorization**:

```python
import numpy as np

def optimized_linear_search(query_embedding, doc_embeddings_matrix, doc_ids, top_k=10):
    """Vectorized Search - viel schneller"""
    # query_embedding: (384,)
    # doc_embeddings_matrix: (n_docs, 384)
    
    # Normalize query
    query_norm = query_embedding / np.linalg.norm(query_embedding)
    
    # Normalize documents
    doc_norms = np.linalg.norm(doc_embeddings_matrix, axis=1, keepdims=True)
    doc_embeddings_normalized = doc_embeddings_matrix / doc_norms
    
    # Batch Cosine Similarity
    similarities = np.dot(doc_embeddings_normalized, query_norm)
    
    # Top-K
    top_indices = np.argsort(similarities)[::-1][:top_k]
    
    results = [
        (doc_ids[idx], float(similarities[idx]))
        for idx in top_indices
    ]
    
    return results
```

**Speedup**: 10-100x schneller durch Vectorization

### FAISS Integration

**Für große Datasets (> 100.000 Dokumente)**:

```python
import faiss

class FAISSIndex:
    def __init__(self, dimension=384):
        self.dimension = dimension
        # Flat L2 Index (exact search)
        self.index = faiss.IndexFlatL2(dimension)
        self.doc_ids = []
    
    def add_documents(self, embeddings, doc_ids):
        """Fügt Dokumente zum Index hinzu"""
        embeddings_np = np.array(embeddings, dtype=np.float32)
        
        # Normalize für Cosine Similarity
        faiss.normalize_L2(embeddings_np)
        
        self.index.add(embeddings_np)
        self.doc_ids.extend(doc_ids)
    
    def search(self, query_embedding, top_k=10):
        """Sucht ähnliche Dokumente"""
        query_np = np.array([query_embedding], dtype=np.float32)
        faiss.normalize_L2(query_np)
        
        # Search
        distances, indices = self.index.search(query_np, top_k)
        
        # Convert L2 distance to similarity
        similarities = 1 - (distances[0] / 2)  # For normalized vectors
        
        results = [
            (self.doc_ids[idx], float(sim))
            for idx, sim in zip(indices[0], similarities)
            if idx < len(self.doc_ids)
        ]
        
        return results
```

**FAISS Index-Typen**:

| Index | Precision | Speed | Memory | Use-Case |
|-------|-----------|-------|--------|----------|
| IndexFlatL2 | 100% | Slow | High | < 100K docs |
| IndexIVFFlat | ~99% | Fast | Medium | 100K-1M docs |
| IndexIVFPQ | ~95% | Very Fast | Low | > 1M docs |
| IndexHNSW | ~99% | Very Fast | High | Best overall |

**HNSW Index** (empfohlen für Production):

```python
class HNSWIndex:
    def __init__(self, dimension=384, ef_construction=200, M=32):
        """
        ef_construction: Konstruktions-Parameter (höher = besser aber langsamer)
        M: Anzahl Verbindungen pro Node (höher = besser aber mehr Memory)
        """
        self.dimension = dimension
        self.index = faiss.IndexHNSWFlat(dimension, M)
        self.index.hnsw.efConstruction = ef_construction
        self.doc_ids = []
    
    def set_search_params(self, ef_search=50):
        """Suchgeschwindigkeit vs Precision"""
        self.index.hnsw.efSearch = ef_search
```

## Hybrid Search

**Kombination von Vector Search + Keyword Search**:

```python
class HybridSearch:
    def __init__(self, vector_index, keyword_index):
        self.vector_index = vector_index
        self.keyword_index = keyword_index
    
    def search(self, query, top_k=10, vector_weight=0.7):
        """
        Hybrid Search mit gewichteter Kombination
        vector_weight: 0.7 = 70% Vector, 30% Keyword
        """
        # Vector Search
        vector_results = self.vector_index.search(query, top_k=top_k*2)
        
        # Keyword Search (BM25)
        keyword_results = self.keyword_index.search(query, top_k=top_k*2)
        
        # Kombiniere Scores
        combined_scores = {}
        
        for doc_id, score in vector_results:
            combined_scores[doc_id] = score * vector_weight
        
        for doc_id, score in keyword_results:
            if doc_id in combined_scores:
                combined_scores[doc_id] += score * (1 - vector_weight)
            else:
                combined_scores[doc_id] = score * (1 - vector_weight)
        
        # Sortiere und returniere Top-K
        sorted_results = sorted(
            combined_scores.items(),
            key=lambda x: x[1],
            reverse=True
        )
        
        return sorted_results[:top_k]
```

**Wann Hybrid Search?**:
- ✅ Exakte Keyword-Matches wichtig
- ✅ Technische Dokumentation (Code, Befehle)
- ✅ Namen, IDs, Nummern
- ❌ Nur semantische Suche nötig

## RAG (Retrieval Augmented Generation)

### RAG Workflow

```
1. User Query
   ↓
2. Generate Query Embedding
   ↓
3. Vector Search → Top-K relevante Dokumente
   ↓
4. Retrieve Full Documents
   ↓
5. Construct Context (Prompt)
   ↓
6. LLM Generation mit Context
   ↓
7. Return Response
```

### RAG Implementation

```python
class RAGPipeline:
    def __init__(self, vector_search, llm_client):
        self.vector_search = vector_search
        self.llm_client = llm_client
        self.embedding_generator = EmbeddingGenerator()
    
    def query(self, user_query, top_k=5, max_context_length=2000):
        """RAG Pipeline"""
        # 1. Generate Query Embedding
        query_embedding = self.embedding_generator.generate(user_query)
        
        # 2. Vector Search
        results = self.vector_search.search(query_embedding, top_k=top_k)
        
        # 3. Retrieve Documents
        documents = []
        for doc_id, score in results:
            doc = self.get_document(doc_id)
            documents.append({
                'id': doc_id,
                'title': doc.title,
                'content': doc.content,
                'score': score
            })
        
        # 4. Construct Context
        context = self._build_context(documents, max_context_length)
        
        # 5. LLM Generation
        prompt = self._build_prompt(user_query, context)
        response = self.llm_client.generate(prompt)
        
        return {
            'answer': response,
            'sources': documents,
            'context': context
        }
    
    def _build_context(self, documents, max_length):
        """Baut Context aus Top-Dokumenten"""
        context_parts = []
        current_length = 0
        
        for doc in documents:
            doc_text = f"[{doc['title']}]\n{doc['content']}\n\n"
            doc_length = len(doc_text)
            
            if current_length + doc_length > max_length:
                # Truncate letztes Dokument
                remaining = max_length - current_length
                doc_text = doc_text[:remaining] + "..."
                context_parts.append(doc_text)
                break
            
            context_parts.append(doc_text)
            current_length += doc_length
        
        return "".join(context_parts)
    
    def _build_prompt(self, query, context):
        """Erstellt LLM-Prompt"""
        return f"""Beantworte die folgende Frage basierend auf dem gegebenen Kontext.

Kontext:
{context}

Frage: {query}

Antwort:"""
```

### Advanced RAG Techniques

**1. Query Rewriting**:
```python
def rewrite_query(query, llm):
    """Erweitert Query für bessere Retrieval"""
    prompt = f"""Generiere 3 alternative Formulierungen der Frage:
{query}

Alternativen:"""
    alternatives = llm.generate(prompt).split('\n')
    return [query] + alternatives[:3]

# Multi-Query RAG
from collections import defaultdict

def multi_query_rag(queries, vector_search, top_k=5):
    all_results = defaultdict(float)
    
    for query in queries:
        results = vector_search.search(query, top_k=top_k)
        for doc_id, score in results:
            all_results[doc_id] = max(all_results[doc_id], score)
    
    return sorted(all_results.items(), key=lambda x: x[1], reverse=True)[:top_k]
```

**2. Re-Ranking**:
```python
from sentence_transformers import CrossEncoder

class ReRanker:
    def __init__(self):
        self.model = CrossEncoder('cross-encoder/ms-marco-MiniLM-L-12-v2')
    
    def rerank(self, query, documents, top_k=5):
        """Re-rankt Ergebnisse mit Cross-Encoder"""
        pairs = [[query, doc.content] for doc in documents]
        scores = self.model.predict(pairs)
        
        # Sortiere nach Re-Rank Score
        ranked = sorted(
            zip(documents, scores),
            key=lambda x: x[1],
            reverse=True
        )
        
        return ranked[:top_k]
```

**3. Contextualized Embeddings**:
```python
def generate_contextualized_embedding(document, context_window=512):
    """Generiert Embeddings mit Context"""
    chunks = chunk_text(document.content, chunk_size=context_window)
    
    embeddings = []
    for chunk in chunks:
        # Füge Title als Context hinzu
        contextualized_text = f"{document.title}: {chunk}"
        embedding = generate_embedding(contextualized_text)
        embeddings.append(embedding)
    
    # Average Pooling
    return np.mean(embeddings, axis=0)
```

## Performance-Optimierung

### Batch Processing

```python
def index_documents_batch(documents, batch_size=100):
    """Effizientes Batch-Indexing"""
    for i in range(0, len(documents), batch_size):
        batch = documents[i:i+batch_size]
        
        # Batch Embedding Generation
        texts = [doc.content for doc in batch]
        embeddings = embedding_generator.generate_batch(texts)
        
        # Batch Index Addition
        doc_ids = [doc.id for doc in batch]
        faiss_index.add_documents(embeddings, doc_ids)
        
        print(f"Indexed {i+len(batch)}/{len(documents)}")
```

### Caching

```python
from functools import lru_cache

class CachedEmbeddingGenerator:
    def __init__(self, model):
        self.model = model
        self._cache = {}
    
    def generate(self, text):
        """Cached Embedding Generation"""
        text_hash = hash(text)
        
        if text_hash not in self._cache:
            self._cache[text_hash] = self.model.encode(text)
        
        return self._cache[text_hash]
```

### Quantization

```python
def quantize_embeddings(embeddings, bits=8):
    """Reduziert Speicher durch Quantisierung"""
    # Float32 (4 bytes) → Int8 (1 byte) = 4x Kompression
    min_val = np.min(embeddings)
    max_val = np.max(embeddings)
    
    # Scale to [0, 255]
    scaled = (embeddings - min_val) / (max_val - min_val) * 255
    quantized = scaled.astype(np.uint8)
    
    # Store min/max für Dequantisierung
    return quantized, min_val, max_val

def dequantize_embeddings(quantized, min_val, max_val):
    """Rekonstruiert Embeddings"""
    scaled = quantized.astype(np.float32) / 255
    return scaled * (max_val - min_val) + min_val
```

## Best Practices

### ✅ DO

1. **Normalize Embeddings** - Für Cosine Similarity
2. **Batch Processing** - Für große Datasets
3. **HNSW Index** - Für Production
4. **Re-Ranking** - Für höhere Precision
5. **Hybrid Search** - Für robuste Ergebnisse
6. **Cache Embeddings** - Für wiederkehrende Queries
7. **Monitoring** - Latency und Quality tracken

### ❌ DON'T

1. **Linear Search > 100K docs** - Zu langsam
2. **Embedding-Models mischen** - Inkompatibel
3. **Zu kleine Chunks** - Verliert Context
4. **Zu große Chunks** - Precision leidet
5. **Keine Normalisierung** - Inkonsistente Scores
6. **Embedding pro Query neu** - Cachen!

---

**Letzte Aktualisierung**: 2025-12-22
