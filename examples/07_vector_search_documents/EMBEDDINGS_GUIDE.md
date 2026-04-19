# Vector Search Documents - Embeddings Guide

> **Historischer Stand:** 2026-01-31 — Inhalte nicht gegen aktuelle Quellen geprüft.

## Übersicht

Dieser Guide erklärt, wie man Text-Embeddings generiert, verwaltet und für Semantic Search verwendet.

## Was sind Embeddings?

### Konzept

Embeddings transformieren Text in numerische Vektoren, die semantische Bedeutung repräsentieren.

**Beispiel**:
```
"Hund" → [0.2, 0.8, -0.1, ..., 0.3]  # 384 Zahlen
"Katze" → [0.3, 0.7, -0.2, ..., 0.4]  # Ähnlich zu "Hund"
"Auto" → [-0.5, 0.1, 0.9, ..., -0.2]  # Weit von "Hund"
```

### Eigenschaften

**Semantic Similarity**:
```python
similarity("Hund", "Katze") = 0.85  # Hoch (beides Haustiere)
similarity("Hund", "Auto") = 0.12   # Niedrig (verschiedene Konzepte)
```

**Dimensionen**:
- 384 Dimensionen: all-MiniLM-L6-v2 (schnell)
- 768 Dimensionen: all-mpnet-base-v2 (präzise)
- 1536 Dimensionen: OpenAI text-embedding-ada-002

## Model Setup

### Installation

```bash
pip install sentence-transformers
```

### Modell-Auswahl

**Für Deutsch + Englisch**:
```python
from sentence_transformers import SentenceTransformer

# Option 1: Mehrsprachig (empfohlen)
model = SentenceTransformer('paraphrase-multilingual-MiniLM-L12-v2')

# Option 2: Nur Englisch (schneller)
model = SentenceTransformer('all-MiniLM-L6-v2')

# Option 3: Beste Qualität
model = SentenceTransformer('all-mpnet-base-v2')
```

**Modell-Vergleich**:

| Modell | Sprachen | Dim | Geschwindigkeit | Qualität | RAM |
|--------|----------|-----|-----------------|----------|-----|
| all-MiniLM-L6-v2 | EN | 384 | ⚡⚡⚡ | ⭐⭐⭐ | 80 MB |
| paraphrase-multilingual | Multi | 384 | ⚡⚡ | ⭐⭐⭐⭐ | 420 MB |
| all-mpnet-base-v2 | EN | 768 | ⚡⚡ | ⭐⭐⭐⭐⭐ | 420 MB |
| distiluse-base | EN | 512 | ⚡⚡⚡ | ⭐⭐⭐ | 250 MB |

### Modell laden und cachen

```python
import os
from sentence_transformers import SentenceTransformer

class EmbeddingService:
    def __init__(self, model_name='all-MiniLM-L6-v2', cache_dir='./models'):
        """
        Lädt Modell und cached es lokal
        """
        self.model_name = model_name
        self.cache_dir = cache_dir
        
        # Erstelle Cache-Verzeichnis
        os.makedirs(cache_dir, exist_ok=True)
        
        # Lade Modell (wird bei erstem Mal heruntergeladen)
        self.model = SentenceTransformer(
            model_name,
            cache_folder=cache_dir
        )
        
        self.dimensions = self.model.get_sentence_embedding_dimension()
        print(f"✓ Modell geladen: {model_name} ({self.dimensions}D)")
```

## Embeddings Generieren

### Einzelner Text

```python
def generate_embedding(text):
    """Generiert Embedding für einen Text"""
    # Normalisiere Text
    text = text.strip()
    
    if not text:
        return None
    
    # Generiere Embedding
    embedding = model.encode(text)
    
    # Konvertiere zu Liste für JSON-Serialisierung
    return embedding.tolist()

# Beispiel
text = "ThemisDB ist eine Multi-Model Datenbank"
embedding = generate_embedding(text)
print(f"Dimensionen: {len(embedding)}")  # 384
print(f"Erste 5 Werte: {embedding[:5]}")
```

### Batch Processing

**Effizient für viele Texte**:

```python
def generate_embeddings_batch(texts, batch_size=32, show_progress=True):
    """Generiert Embeddings für Liste von Texten"""
    embeddings = model.encode(
        texts,
        batch_size=batch_size,
        show_progress_bar=show_progress,
        convert_to_numpy=True
    )
    
    return embeddings

# Beispiel
texts = [
    "Dokument 1: Python Programmierung",
    "Dokument 2: Machine Learning",
    "Dokument 3: Datenbanken"
]

embeddings = generate_embeddings_batch(texts)
print(f"Form: {embeddings.shape}")  # (3, 384)
```

**Performance-Vergleich**:
```
1 Text → Einzeln:        ~10 ms
100 Texte → Einzeln:     ~1000 ms
100 Texte → Batch(32):   ~250 ms  # 4x schneller!
```

### Text-Preprocessing

**Wichtig für beste Ergebnisse**:

```python
def preprocess_text(text, max_length=512):
    """Bereitet Text für Embedding vor"""
    # Entferne übermäßige Whitespace
    text = ' '.join(text.split())
    
    # Truncate auf max_length Tokens (~4 chars/token)
    max_chars = max_length * 4
    if len(text) > max_chars:
        text = text[:max_chars]
    
    # Optional: Lowercasing (nicht immer nötig)
    # text = text.lower()
    
    return text.strip()

# Beispiel
raw_text = """
    ThemisDB    ist   eine
    Multi-Model    Datenbank
"""
clean_text = preprocess_text(raw_text)
print(clean_text)  # "ThemisDB ist eine Multi-Model Datenbank"
```

## Chunking-Strategien

### Fixed-Size Chunks

**Für lange Dokumente**:

```python
def chunk_text_fixed(text, chunk_size=512, overlap=50):
    """Teilt Text in überlappende Chunks"""
    words = text.split()
    chunks = []
    
    for i in range(0, len(words), chunk_size - overlap):
        chunk = ' '.join(words[i:i+chunk_size])
        if chunk:
            chunks.append(chunk)
    
    return chunks

# Beispiel
long_text = "..." * 1000  # Langer Text
chunks = chunk_text_fixed(long_text, chunk_size=100, overlap=20)
print(f"{len(chunks)} Chunks erstellt")
```

**Overlap-Zweck**: Verhindert Informationsverlust an Chunk-Grenzen

### Semantic Chunking

**Basiert auf Satz-Grenzen**:

```python
import re

def chunk_text_semantic(text, max_chunk_size=500):
    """Chunks an Satzgrenzen"""
    # Split in Sätze
    sentences = re.split(r'[.!?]+', text)
    
    chunks = []
    current_chunk = []
    current_size = 0
    
    for sentence in sentences:
        sentence = sentence.strip()
        if not sentence:
            continue
        
        sentence_size = len(sentence.split())
        
        if current_size + sentence_size > max_chunk_size and current_chunk:
            # Chunk ist voll
            chunks.append(' '.join(current_chunk))
            current_chunk = [sentence]
            current_size = sentence_size
        else:
            current_chunk.append(sentence)
            current_size += sentence_size
    
    if current_chunk:
        chunks.append(' '.join(current_chunk))
    
    return chunks
```

### Paragraph-Based Chunking

**Für strukturierte Dokumente**:

```python
def chunk_text_paragraphs(text, max_chunk_size=500):
    """Chunks basierend auf Paragraphen"""
    paragraphs = text.split('\n\n')
    
    chunks = []
    current_chunk = []
    current_size = 0
    
    for paragraph in paragraphs:
        paragraph = paragraph.strip()
        if not paragraph:
            continue
        
        para_size = len(paragraph.split())
        
        if para_size > max_chunk_size:
            # Paragraph zu groß → weiter chunken
            if current_chunk:
                chunks.append('\n\n'.join(current_chunk))
                current_chunk = []
                current_size = 0
            
            # Chunk großen Paragraph
            para_chunks = chunk_text_semantic(paragraph, max_chunk_size)
            chunks.extend(para_chunks)
        
        elif current_size + para_size > max_chunk_size and current_chunk:
            chunks.append('\n\n'.join(current_chunk))
            current_chunk = [paragraph]
            current_size = para_size
        
        else:
            current_chunk.append(paragraph)
            current_size += para_size
    
    if current_chunk:
        chunks.append('\n\n'.join(current_chunk))
    
    return chunks
```

## Embedding-Management

### Speicherung in ThemisDB

```python
class EmbeddingStore:
    def __init__(self, client):
        self.client = client
    
    def store_document_embedding(self, doc_id, title, content, embedding):
        """Speichert Dokument mit Embedding"""
        document = {
            'id': doc_id,
            'title': title,
            'content': content,
            'embedding': embedding,  # Array von Floats
            'embedding_dimensions': len(embedding),
            'embedding_model': 'all-MiniLM-L6-v2',
            'created_at': datetime.now().isoformat()
        }
        
        return self.client.insert('documents', document)
    
    def get_all_embeddings(self):
        """Lädt alle Embeddings für Vector Search"""
        docs = self.client.query('SELECT id, embedding FROM documents')
        
        return {
            doc['id']: np.array(doc['embedding'])
            for doc in docs
        }
```

### Chunked Document Storage

```python
def store_chunked_document(doc_id, title, content, chunk_size=512):
    """Speichert Dokument als mehrere Chunks"""
    # 1. Chunk Text
    chunks = chunk_text_semantic(content, max_chunk_size=chunk_size)
    
    # 2. Generate Embeddings
    chunk_embeddings = generate_embeddings_batch(chunks)
    
    # 3. Store Chunks
    for i, (chunk, embedding) in enumerate(zip(chunks, chunk_embeddings)):
        chunk_doc = {
            'id': f"{doc_id}_chunk_{i}",
            'parent_id': doc_id,
            'title': title,
            'content': chunk,
            'chunk_index': i,
            'chunk_total': len(chunks),
            'embedding': embedding.tolist()
        }
        
        client.insert('document_chunks', chunk_doc)
    
    # 4. Store Parent Metadata
    parent_doc = {
        'id': doc_id,
        'title': title,
        'full_content': content,
        'chunk_count': len(chunks),
        'created_at': datetime.now().isoformat()
    }
    
    client.insert('documents', parent_doc)
```

## Embedding-Qualität

### Evaluation Metrics

**1. Precision@K**:
```python
def precision_at_k(query, results, relevant_docs, k=10):
    """Misst Precision der Top-K Ergebnisse"""
    top_k_results = [r['id'] for r in results[:k]]
    relevant_found = sum(1 for doc_id in top_k_results if doc_id in relevant_docs)
    
    return relevant_found / k
```

**2. Mean Reciprocal Rank (MRR)**:
```python
def mean_reciprocal_rank(queries, results_lists, relevant_docs_lists):
    """Durchschnittliche Position des ersten relevanten Dokuments"""
    reciprocal_ranks = []
    
    for results, relevant_docs in zip(results_lists, relevant_docs_lists):
        for rank, result in enumerate(results, start=1):
            if result['id'] in relevant_docs:
                reciprocal_ranks.append(1.0 / rank)
                break
        else:
            reciprocal_ranks.append(0.0)
    
    return sum(reciprocal_ranks) / len(reciprocal_ranks)
```

### Quality Testing

```python
def test_embedding_quality():
    """Test-Suite für Embedding-Qualität"""
    test_cases = [
        # (query, expected_similar, expected_different)
        ("Python Programmierung", ["Python Tutorial", "Code lernen"], ["Auto kaufen"]),
        ("Datenbank Administration", ["SQL Server", "DBA Guide"], ["Kochen"]),
    ]
    
    for query, similar, different in test_cases:
        query_emb = generate_embedding(query)
        
        # Teste ähnliche
        for sim_text in similar:
            sim_emb = generate_embedding(sim_text)
            score = cosine_similarity(query_emb, sim_emb)
            assert score > 0.5, f"Low similarity: {query} <-> {sim_text} = {score}"
            print(f"✓ {query} <-> {sim_text}: {score:.3f}")
        
        # Teste unähnliche
        for diff_text in different:
            diff_emb = generate_embedding(diff_text)
            score = cosine_similarity(query_emb, diff_emb)
            assert score < 0.5, f"High similarity: {query} <-> {diff_text} = {score}"
            print(f"✓ {query} <-> {diff_text}: {score:.3f}")
```

## Erweiterte Techniken

### Domain-Specific Fine-Tuning

**Für spezialisierte Anwendungen**:

```python
from sentence_transformers import SentenceTransformer, InputExample, losses
from torch.utils.data import DataLoader

def fine_tune_model(base_model, training_data, output_path):
    """Fine-tuned Modell für spezifische Domain"""
    model = SentenceTransformer(base_model)
    
    # Training Examples: (text1, text2, similarity_score)
    examples = [
        InputExample(texts=[ex['text1'], ex['text2']], label=ex['score'])
        for ex in training_data
    ]
    
    # DataLoader
    train_dataloader = DataLoader(examples, shuffle=True, batch_size=16)
    
    # Loss Function
    train_loss = losses.CosineSimilarityLoss(model)
    
    # Training
    model.fit(
        train_objectives=[(train_dataloader, train_loss)],
        epochs=3,
        warmup_steps=100,
        output_path=output_path
    )
    
    return model
```

### Cross-Encoder Re-Ranking

**Höhere Präzision**:

```python
from sentence_transformers import CrossEncoder

class ReRankingPipeline:
    def __init__(self):
        self.bi_encoder = SentenceTransformer('all-MiniLM-L6-v2')
        self.cross_encoder = CrossEncoder('cross-encoder/ms-marco-MiniLM-L-12-v2')
    
    def search_and_rerank(self, query, documents, top_k=10, rerank_top=100):
        """Bi-Encoder für Recall, Cross-Encoder für Precision"""
        # 1. Bi-Encoder: Schnelle Initial-Suche
        query_emb = self.bi_encoder.encode(query)
        doc_embs = self.bi_encoder.encode([doc.content for doc in documents])
        
        # Initial Top-100
        similarities = cosine_similarity_batch(query_emb, doc_embs)
        top_indices = np.argsort(similarities)[::-1][:rerank_top]
        
        # 2. Cross-Encoder: Präzises Re-Ranking
        pairs = [[query, documents[idx].content] for idx in top_indices]
        scores = self.cross_encoder.predict(pairs)
        
        # Final Top-K
        reranked_indices = np.argsort(scores)[::-1][:top_k]
        
        return [
            (documents[top_indices[idx]], float(scores[idx]))
            for idx in reranked_indices
        ]
```

### Multi-Vector Embeddings

**Für komplexe Dokumente**:

```python
def generate_multi_vector_embedding(document):
    """Mehrere Embeddings pro Dokument für bessere Repräsentation"""
    embeddings = []
    
    # 1. Title Embedding
    if document.title:
        title_emb = generate_embedding(document.title)
        embeddings.append(('title', title_emb, 2.0))  # Weight
    
    # 2. Summary Embedding
    if document.summary:
        summary_emb = generate_embedding(document.summary)
        embeddings.append(('summary', summary_emb, 1.5))
    
    # 3. Content Chunks
    chunks = chunk_text_semantic(document.content, max_chunk_size=256)
    for i, chunk in enumerate(chunks[:5]):  # Max 5 chunks
        chunk_emb = generate_embedding(chunk)
        embeddings.append((f'chunk_{i}', chunk_emb, 1.0))
    
    return embeddings

def search_multi_vector(query_embedding, documents, top_k=10):
    """Sucht über mehrere Vektoren pro Dokument"""
    doc_scores = {}
    
    for doc in documents:
        max_score = 0
        
        for vector_type, embedding, weight in doc.embeddings:
            similarity = cosine_similarity(query_embedding, embedding)
            weighted_score = similarity * weight
            max_score = max(max_score, weighted_score)
        
        doc_scores[doc.id] = max_score
    
    # Sort und return Top-K
    sorted_docs = sorted(doc_scores.items(), key=lambda x: x[1], reverse=True)
    return sorted_docs[:top_k]
```

## Troubleshooting

### Problem: Schlechte Suchergebnisse

**Lösungen**:

1. **Anderes Modell testen**:
   ```python
   # Von all-MiniLM → all-mpnet (bessere Qualität)
   model = SentenceTransformer('all-mpnet-base-v2')
   ```

2. **Text-Preprocessing verbessern**:
   ```python
   text = preprocess_text(text)
   text = remove_special_chars(text)
   ```

3. **Hybrid Search**:
   ```python
   # Kombiniere Vector + Keyword
   results = hybrid_search(query, vector_weight=0.7)
   ```

### Problem: Zu langsam

**Lösungen**:

1. **Batch Processing**:
   ```python
   embeddings = model.encode(texts, batch_size=64)
   ```

2. **GPU nutzen**:
   ```python
   model = SentenceTransformer('all-MiniLM-L6-v2', device='cuda')
   ```

3. **Kleineres Modell**:
   ```python
   model = SentenceTransformer('all-MiniLM-L6-v2')  # Statt all-mpnet
   ```

### Problem: Zu viel RAM

**Lösungen**:

1. **Quantisierung**:
   ```python
   embeddings_int8 = (embeddings * 127).astype(np.int8)
   ```

2. **Streaming**:
   ```python
   for batch in batch_generator(documents, batch_size=1000):
       process_batch(batch)
   ```

## Best Practices

### ✅ DO

1. **Batch Processing** - Für Effizienz
2. **Text Preprocessing** - Für Konsistenz
3. **Chunking** - Für lange Dokumente
4. **Caching** - Für wiederkehrende Texte
5. **Evaluation** - Mit Test-Cases
6. **Monitoring** - Qualität tracken

### ❌ DON'T

1. **Embeddings mischen** - Verschiedene Modelle inkompatibel
2. **Zu große Chunks** - Max 512 Tokens
3. **Keine Normalisierung** - Für Cosine Similarity
4. **Modell pro Query laden** - Einmal laden, oft nutzen
5. **Ignore Performance** - Profile und optimiere

---

**Letzte Aktualisierung**: 2025-12-22
