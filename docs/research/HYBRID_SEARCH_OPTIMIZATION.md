# Hybrid Search Optimization - Research Report

**Projekt:** ThemisDB  
**Kategorie:** Research Documentation  
**Erstellt:** 27. Januar 2026  
**Version:** 1.0  
**Status:** ✅ Abgeschlossen

---

## 📋 Executive Summary

Dieser Forschungsbericht untersucht aktuelle Hybrid Search Methoden zur Kombination von Dense- und Sparse-Ansätzen sowie multi-modaler Recherche. Der Fokus liegt auf der Analyse vielversprechender Verfahren für eine flexible und performante Suche in ThemisDB.

**Kernerkenntnisse:**
- Hybride Suchsysteme kombinieren das Beste aus lexikalischer (Sparse) und semantischer (Dense) Suche
- Dense-Sparse Fusion mit Reciprocal Rank Fusion (RRF) ist einfach implementierbar und sehr effektiv
- Late Interaction Modelle (ColBERT) bieten überlegene Präzision bei moderatem Overhead
- Multi-Vector Representations ermöglichen feinkörnigere semantische Matches
- Cross-Modal Retrieval eröffnet neue Anwendungsszenarien für ThemisDB

---

## 🎯 Forschungsziele

1. **Dense-Sparse Hybridmethoden:** Kombination von Keyword-basierter (BM25) und semantischer (Vector) Suche
2. **Cross-Modal Retrieval:** Text-zu-Bild, Bild-zu-Text, und andere multi-modale Suchszenarien
3. **Multi-Vector Representations:** Techniken zur Verwendung mehrerer Vektoren pro Dokument
4. **ThemisDB-Integration:** Bewertung der Anwendbarkeit für ThemisDB

---

## 1. Dense-Sparse Hybridmethoden

### 1.1 Grundkonzepte

**Sparse Retrieval (Lexikalisch):**
- Basiert auf exakter Keyword-Übereinstimmung
- BM25, TF-IDF als klassische Vertreter
- ✅ Vorteile: Schnell, interpretierbar, keine false positives bei Eigennamen
- ❌ Nachteile: Synonyme nicht erkannt, keine semantische Ähnlichkeit

**Dense Retrieval (Semantisch):**
- Basiert auf gelernten Embeddings (z.B. BERT, Sentence Transformers)
- Vektorsuche mit HNSW, FAISS, etc.
- ✅ Vorteile: Semantische Ähnlichkeit, Synonyme, mehrsprachig
- ❌ Nachteile: Rechenintensiv, kann exakte Matches "vergessen"

**Hybrid Approach:**
Kombiniert beide Methoden, um die jeweiligen Stärken zu nutzen und Schwächen zu kompensieren.

---

### 1.2 Fusion-Strategien

#### 1.2.1 Reciprocal Rank Fusion (RRF)

**Konzept:**
RRF kombiniert Rankings aus verschiedenen Retrieval-Systemen ohne Normalisierung der Scores.

**Formel:**
```
RRF_score(d) = Σ 1 / (k + rank_i(d))
```
wobei:
- `d` = Dokument
- `rank_i(d)` = Rang des Dokuments in Ranking i
- `k` = Konstante (typisch: 60)

**Eigenschaften:**
- ✅ Score-agnostisch (keine Normalisierung nötig)
- ✅ Einfach implementierbar
- ✅ Robust gegenüber outlier scores
- ✅ Empirisch sehr effektiv (Cormack et al., 2009)

**Implementierungsskizze für ThemisDB:**
```cpp
// Pseudo-Code
struct SearchResult {
    std::string document_id;
    float score;
    int rank;
};

std::vector<SearchResult> hybrid_search_rrf(
    const std::string& query,
    const std::vector<float>& query_embedding,
    int k_results = 20,
    float k_constant = 60.0f
) {
    // 1. Sparse retrieval (BM25)
    auto sparse_results = bm25_search(query, k_results * 2);
    
    // 2. Dense retrieval (Vector Search)
    auto dense_results = vector_search(query_embedding, k_results * 2);
    
    // 3. RRF Fusion
    std::map<std::string, float> rrf_scores;
    
    for (size_t i = 0; i < sparse_results.size(); ++i) {
        rrf_scores[sparse_results[i].document_id] += 
            1.0f / (k_constant + i + 1);
    }
    
    for (size_t i = 0; i < dense_results.size(); ++i) {
        rrf_scores[dense_results[i].document_id] += 
            1.0f / (k_constant + i + 1);
    }
    
    // 4. Sort by RRF score
    // ... sort and return top-k
}
```

**Evaluation (aus Literatur):**
- Verbesserung von 5-15% MRR vs. einzelne Methoden (Cormack et al., 2009)
- Besonders effektiv bei multi-aspekt Queries

---

#### 1.2.2 Linear Combination

**Konzept:**
Gewichtete Kombination von normalisierten Scores.

**Formel:**
```
hybrid_score(d) = α · score_dense(d) + (1-α) · score_sparse(d)
```

**Eigenschaften:**
- ✅ Intuitive Gewichtung (α zwischen 0 und 1)
- ❌ Erfordert Score-Normalisierung
- ❌ α-Wert muss pro Use Case tuned werden

**Normalisierungsoptionen:**
1. **Min-Max:** `(score - min) / (max - min)`
2. **Z-Score:** `(score - mean) / std`
3. **Sigmoid:** `1 / (1 + exp(-score))`

**Empfehlung:**
- RRF bevorzugen für Robustheit
- Linear Combination wenn domänen-spezifische Gewichtung wichtig ist

---

#### 1.2.3 Late Interaction Models (ColBERT)

**Konzept:**
Statt ein einzelnes Embedding pro Dokument, wird jedes Token im Dokument zu einem Vektor.

**ColBERT-Architektur:**
```
Query:    [Token1] [Token2] ... [TokenN]  →  [Vec1] [Vec2] ... [VecN]
Document: [Token1] [Token2] ... [TokenM]  →  [Vec1] [Vec2] ... [VecM]

Score = Σ_i max_j (Vec_qi · Vec_dj)
```

**Eigenschaften:**
- ✅ Feinkörnigere semantische Übereinstimmung
- ✅ Höhere Präzision als Einzelvektor-Ansätze (+5-10% MRR)
- ❌ N×M Vektoren zu speichern (Speicher-Overhead)
- ❌ Komplexere Indexierung

**Performance:**
- MS MARCO Passage Ranking: MRR@10 = 0.36 (vs. 0.33 für DPR)
- TREC-COVID: NDCG@10 = 0.69 (vs. 0.65 für DPR)

**Implementierungsoptionen für ThemisDB:**
1. **ColBERTv1:** Alle Token-Vektoren speichern
2. **ColBERTv2:** Kompressions-Techniken (Residual Compression)
3. **Approximations:** Nur Top-K wichtigste Token-Vektoren speichern

**Speicherkosten-Beispiel:**
```
Document: 512 tokens × 128 dimensions × 4 bytes (float32) = 256 KB pro Dokument
vs. Single Vector: 768 dimensions × 4 bytes = 3 KB pro Dokument

→ 85× Speicher-Overhead
```

**Empfehlung für ThemisDB:**
- Für High-Precision Use Cases (Legal, Medical) evaluieren
- Komprimierte Variante (ColBERTv2) verwenden
- Optional: Als Enterprise-Feature anbieten

---

### 1.3 BM25-Implementierung für ThemisDB

BM25 ist der de-facto Standard für Sparse Retrieval.

**BM25-Formel:**
```
BM25(d, q) = Σ IDF(qi) · f(qi, d) · (k1 + 1) / 
             (f(qi, d) + k1 · (1 - b + b · |d|/avgdl))
```

**Parameter:**
- `k1`: Term frequency saturation (Standard: 1.2)
- `b`: Length normalization (Standard: 0.75)
- `IDF(qi)`: Inverse Document Frequency
- `f(qi, d)`: Term frequency in document d

**Integration in ThemisDB:**

```cpp
// Neue Komponente: SparseIndexManager
class SparseIndexManager {
public:
    struct BM25Config {
        float k1 = 1.2f;
        float b = 0.75f;
    };
    
    struct TermStats {
        uint64_t doc_freq;      // Dokumente mit diesem Term
        uint64_t total_freq;    // Gesamte Vorkommen
    };
    
    // Index-Struktur: Inverted Index
    // term -> [(doc_id, term_freq), ...]
    std::unordered_map<std::string, 
        std::vector<std::pair<std::string, uint32_t>>> inverted_index_;
    
    // Document stats
    std::unordered_map<std::string, uint32_t> doc_lengths_;
    uint64_t total_docs_ = 0;
    float avg_doc_length_ = 0.0f;
    
    void index_document(const std::string& doc_id, 
                       const std::vector<std::string>& tokens);
    
    std::vector<SearchResult> search(
        const std::vector<std::string>& query_tokens,
        int top_k,
        const BM25Config& config = BM25Config()
    );
    
private:
    float compute_idf(const std::string& term) const;
    float compute_bm25_score(const std::string& doc_id,
                            const std::vector<std::string>& query_tokens,
                            const BM25Config& config) const;
};
```

**RocksDB Schema für BM25:**
```
# Inverted Index
"sparse:term:{term}" → JSON: [{"doc_id": "...", "freq": 5}, ...]

# Document Metadata
"sparse:doc:{doc_id}" → JSON: {"length": 512, "indexed_at": "..."}

# Global Stats
"sparse:stats:total_docs" → uint64
"sparse:stats:avg_doc_length" → float
```

---

### 1.4 Evaluation Metrics

**Wichtige Metriken für Hybrid Search:**

1. **Mean Reciprocal Rank (MRR):**
   ```
   MRR = (1/|Q|) Σ 1/rank_i
   ```
   - Fokus auf erstes relevantes Ergebnis

2. **Normalized Discounted Cumulative Gain (NDCG@k):**
   ```
   NDCG@k = DCG@k / IDCG@k
   DCG@k = Σ (2^rel_i - 1) / log2(i + 1)
   ```
   - Berücksichtigt Position und Relevanz

3. **Recall@k:**
   ```
   Recall@k = (Relevante Docs in Top-k) / (Alle relevanten Docs)
   ```
   - Wichtig für Retrieval-Systeme

**Benchmark-Datasets:**
- **MS MARCO:** 8.8M passages, 1M queries
- **BEIR:** 18 diverse Datasets (Zero-shot Evaluation)
- **TREC:** Deep Learning Track

---

## 2. Cross-Modal Retrieval

### 2.1 Grundkonzepte

Cross-Modal Retrieval ermöglicht Suche über verschiedene Modalitäten hinweg:
- Text → Bild
- Bild → Text
- Text → Audio
- Video → Text

**Kernidee:**
Gemeinsamer Embedding-Space für verschiedene Modalitäten.

---

### 2.2 Vision-Language Models

#### 2.2.1 CLIP (Contrastive Language-Image Pre-training)

**Architektur:**
```
Image Input → Image Encoder (Vision Transformer) → Image Embedding (512D)
Text Input  → Text Encoder (Transformer)         → Text Embedding (512D)

Contrastive Loss: Images und Texts im gleichen Raum
```

**Training:**
- 400M (Image, Text)-Paare aus dem Internet
- Contrastive Learning: Matching pairs zusammenbringen, non-matching auseinander

**Eigenschaften:**
- ✅ Zero-shot Image Classification
- ✅ Text-to-Image und Image-to-Text Retrieval
- ✅ Multilinguale Varianten verfügbar
- ❌ Rechenintensiv (große Vision Transformer)

**Performance:**
- ImageNet Zero-shot: 76.2% Top-1 Accuracy
- COCO Image-Text Retrieval: Recall@1 = 58.4%

**Varianten:**
- **OpenCLIP:** Open-Source Re-Implementierung
- **CLIP-ViT-L/14:** Large Model (428M Parameter)
- **CLIP-ViT-B/32:** Base Model (151M Parameter)

---

#### 2.2.2 ALIGN (A Large-scale ImaGe and Noisy-text embedding)

**Unterschiede zu CLIP:**
- Training auf 1.8B (Image, Alt-text)-Paare
- Noisy data (automatisch gescraped)
- Keine manuelle Curation

**Performance:**
- ImageNet: 76.4% (leicht besser als CLIP)
- Mehr Robustheit gegenüber noisy Queries

---

#### 2.2.3 BLIP (Bootstrapping Language-Image Pre-training)

**Innovation:**
- Caption Generation zusätzlich zu Contrastive Learning
- Synthetic Captions für bessere Datenqualität

**Architektur:**
```
Image → Vision Transformer → Shared Multimodal Encoder
                              ↓
                         [Caption Generator] + [Contrastive Loss]
```

**Eigenschaften:**
- ✅ Bessere Caption Quality
- ✅ Image-Text Matching + Caption Generation
- ❌ Größeres Modell

---

### 2.3 Integration in ThemisDB

**Speicher-Schema für Multi-Modal Embeddings:**

```
# Image Embeddings
"image:{image_id}:embedding" → [512D CLIP vector]
"image:{image_id}:metadata" → JSON: {
    "url": "...",
    "caption": "...",
    "indexed_at": "...",
    "model": "CLIP-ViT-B/32"
}

# Text-to-Image Index
"crossmodal:text2image:{text_embedding_hash}" → [image_id1, image_id2, ...]
```

**API-Erweiterung:**

```cpp
// Neue Endpoints
POST /crossmodal/image/index
{
    "image_id": "img123",
    "image_url": "https://...",
    "caption": "A cat sitting on a couch",
    "image_data": "base64..."  // Optional
}

POST /crossmodal/search/text-to-image
{
    "query_text": "red car in the sunset",
    "top_k": 10,
    "model": "CLIP-ViT-B/32"
}

POST /crossmodal/search/image-to-text
{
    "query_image_id": "img456",
    "top_k": 10
}
```

**Embedding-Generation:**

ThemisDB hat bereits llama.cpp integriert. Für CLIP:

**Option 1: llama.cpp mit CLIP support**
```bash
# llama.cpp unterstützt CLIP models
llama-cli --model clip-vit-b-32.gguf --image input.jpg
```

**Option 2: ONNX Runtime (wie GNN-Forschung)**
```cpp
// CLIP ONNX Inference
#include <onnxruntime/core/session/onnxruntime_cxx_api.h>

class CLIPEmbedder {
    Ort::Session image_session_;
    Ort::Session text_session_;
    
    std::vector<float> encode_image(const cv::Mat& image);
    std::vector<float> encode_text(const std::string& text);
};
```

**Option 3: Python Microservice**
```python
# Separater CLIP Service
from transformers import CLIPModel, CLIPProcessor

model = CLIPModel.from_pretrained("openai/clip-vit-base-patch32")
processor = CLIPProcessor.from_pretrained("openai/clip-vit-base-patch32")

@app.post("/encode/image")
def encode_image(image_data: bytes):
    image = Image.open(io.BytesIO(image_data))
    inputs = processor(images=image, return_tensors="pt")
    embeddings = model.get_image_features(**inputs)
    return embeddings.tolist()
```

**Empfehlung:**
- **Phase 1:** Python Microservice (schnellste Integration)
- **Phase 2:** ONNX Runtime (bessere Performance)
- **Phase 3:** llama.cpp CLIP support (einheitliche LLM-Infrastruktur)

---

### 2.4 Use Cases für Cross-Modal Retrieval in ThemisDB

**1. E-Commerce:**
- Produktsuche: "Rote Lederjacke" → Produktbilder
- Reverse Image Search: Foto hochladen → ähnliche Produkte

**2. Medien & Content Management:**
- Stock Photo Suche: Textbeschreibung → Stockfotos
- Video Indexing: Frame Embeddings → Szenen finden

**3. Compliance & Legal:**
- Dokumentensuche mit Bildbelegen: "Vertrag mit Unterschrift" → PDF-Seiten
- Beweismittel-Suche: Beschreibung → relevante Fotos/Videos

**4. Behörden & Public Sector:**
- Baupläne: Textbeschreibung → Technische Zeichnungen
- Kataster: Grundstücksbeschreibung → Karten

---

## 3. Multi-Vector Representations

### 3.1 Konzepte

Statt einem Embedding pro Dokument, mehrere Embeddings für verschiedene Aspekte:

**Ansätze:**
1. **ColBERT:** Token-Level Embeddings (bereits diskutiert)
2. **Poly-Encoders:** Mehrere "Attention Heads" Embeddings
3. **Aspect-based Embeddings:** Embeddings für verschiedene Aspekte (Title, Abstract, Body)

---

### 3.2 Poly-Encoders

**Architektur:**
```
Document → BERT → [CLS] Token
                   ↓
            M Attention Codes → M Context Vectors
                                ↓
Query → BERT → Query Embedding → Dot Product with M Vectors → Score
```

**Parameter:**
- `M`: Anzahl der Poly-Codes (typisch: 64, 128)

**Eigenschaften:**
- ✅ Bessere Präzision als Bi-Encoder (+3-5% Recall@1)
- ✅ Schneller als Cross-Encoder
- ❌ Komplexere Architektur

**Performance (ConvAI2 Dataset):**
- Bi-Encoder: Recall@1 = 79.3%
- Poly-Encoder (M=64): Recall@1 = 86.7%
- Cross-Encoder: Recall@1 = 89.2% (aber 100× langsamer)

---

### 3.3 Aspect-based Multi-Vector

**Konzept:**
Verschiedene Embeddings für verschiedene Dokument-Teile:

```
Document:
  - Title Embedding (384D)
  - Abstract Embedding (384D)
  - Body Embedding (384D)
  
Query Embedding (384D)

Scores:
  - score_title = cosine(query, title)
  - score_abstract = cosine(query, abstract)
  - score_body = cosine(query, body)
  
Final Score = α · score_title + β · score_abstract + γ · score_body
```

**Gewichtung:**
- Title: α = 0.5 (sehr wichtig)
- Abstract: β = 0.3
- Body: γ = 0.2

**Eigenschaften:**
- ✅ Interpretierbar
- ✅ Einfach implementierbar
- ❌ Erfordert strukturierte Dokumente

---

### 3.4 Storage-Optionen in ThemisDB

**Variante 1: Multi-Column in Vector Index**
```cpp
struct MultiVectorDocument {
    std::string doc_id;
    std::vector<float> title_embedding;
    std::vector<float> abstract_embedding;
    std::vector<float> body_embedding;
};

// RocksDB Schema
"mvec:{doc_id}:title" → [384D vector]
"mvec:{doc_id}:abstract" → [384D vector]
"mvec:{doc_id}:body" → [384D vector]
```

**Variante 2: Separate HNSW Indices**
```cpp
VectorIndexManager title_index;    // Separate HNSW für Titles
VectorIndexManager abstract_index; // Separate HNSW für Abstracts
VectorIndexManager body_index;     // Separate HNSW für Body

// Suche in allen drei, kombiniere Ergebnisse mit RRF
```

**Empfehlung:**
- Separate Indices für bessere Tuning-Möglichkeiten
- Shared Storage Layer (RocksDB) für Metadaten

---

## 4. State-of-the-Art & Best Practices

### 4.1 Aktuelle SOTA-Modelle (2026)

**Dense Retrievers:**
1. **E5-Mistral-7B:** 7B Parameter, beste Zero-shot Performance
   - BEIR Average: NDCG@10 = 0.548
2. **BGE-M3:** Multi-lingual, Multi-granularity
   - BEIR Average: NDCG@10 = 0.540
3. **Instructor-XL:** Instruction-following Embeddings
   - Flexibel durch Task-Instructions

**Hybrid Models:**
1. **SPLADE:** Sparse + Dense in einem Modell
   - Learned Sparse Representations
   - BEIR Average: NDCG@10 = 0.520
2. **uniCOIL:** Universal Contextualized Inverted List
   - Effiziente Sparse Representations

---

### 4.2 Best Practices für Production

**1. Index-Strategie:**
- Dense Vector Index (HNSW): Semantische Ähnlichkeit
- Sparse Index (BM25): Exakte Keyword-Matches
- Hybrid Fusion (RRF): Bestes aus beiden Welten

**2. Query Processing Pipeline:**
```
User Query
    ↓
[Query Analysis] → Keyword Extraction + Embedding Generation
    ↓
[Parallel Retrieval]
    ├─→ BM25 Search (Fast)
    └─→ Vector Search (Accurate)
    ↓
[Fusion Layer] → RRF or Linear Combination
    ↓
[Re-Ranking] → Optional: Cross-Encoder für Top-K
    ↓
Results
```

**3. Performance Optimization:**
- **Caching:** Query Embeddings cachen (häufige Queries)
- **Quantization:** INT8 statt FP32 für Vektoren (-75% Speicher)
- **Pruning:** HNSW Parameter tunen (efConstruction, efSearch)
- **Batch Processing:** Mehrere Queries parallel verarbeiten

**4. Evaluation:**
- A/B Testing mit echten Usern
- Offline Metrics: MRR, NDCG, Recall@k
- Query Latency Monitoring
- User Satisfaction Scores (CTR, Dwell Time)

---

## 5. Empfehlungen für ThemisDB

### 5.1 Roadmap-Vorschlag

#### Phase 1: Basic Hybrid Search (3 Monate)
**Ziel:** BM25 + Dense Vector Fusion mit RRF

**Deliverables:**
1. `SparseIndexManager` Implementation
   - BM25 Scoring
   - Inverted Index in RocksDB
   - Tokenization (Whitespace, Unicode)
2. Hybrid Search API
   - `POST /search/hybrid` Endpoint
   - RRF Fusion Implementation
3. Documentation & Examples

**Aufwand:** ~2000 LOC

---

#### Phase 2: Cross-Modal Retrieval (4 Monate)
**Ziel:** Text-Image Search mit CLIP

**Deliverables:**
1. CLIP Integration
   - Python Microservice (Phase 2a)
   - ONNX Runtime (Phase 2b)
2. Image Index Manager
   - Image Embedding Storage
   - HNSW Index für Images
3. Cross-Modal API
   - `POST /crossmodal/search/text-to-image`
   - `POST /crossmodal/search/image-to-text`
4. Image Management
   - Upload, Storage, Retrieval

**Aufwand:** ~3000 LOC + Microservice

---

#### Phase 3: Multi-Vector Representations (3 Monate)
**Ziel:** Aspect-based Search

**Deliverables:**
1. Multi-Vector Storage Schema
2. Aspect-based Indexing
   - Title, Abstract, Body Embeddings
3. Advanced Fusion Strategies
   - Weighted Aspect Scoring
   - Poly-Encoder Support (optional)

**Aufwand:** ~1500 LOC

---

#### Phase 4: Advanced Features (4 Monate)
**Ziel:** Production Optimizations

**Deliverables:**
1. Query Performance Optimization
   - Embedding Cache
   - Batch Processing
2. Re-Ranking Module
   - Cross-Encoder Integration
   - LLM-based Re-Ranking
3. Monitoring & Analytics
   - Search Quality Metrics
   - Performance Dashboards

**Aufwand:** ~2000 LOC

---

### 5.2 Prioritäten

**MUST-HAVE (P0):**
1. ✅ Hybrid Search (BM25 + Dense Vector + RRF)
   - Direkter Nutzen für alle Textsuche-Anwendungen
   - Relativ einfach zu implementieren
   - Industry Standard

**SHOULD-HAVE (P1):**
2. ✅ Cross-Modal Retrieval (CLIP Integration)
   - Differenzierung gegenüber Wettbewerb
   - Neue Use Cases (E-Commerce, Media)
   - Moderate Komplexität

**NICE-TO-HAVE (P2):**
3. ⏰ Multi-Vector Representations
   - Inkrementelle Verbesserung (+3-5% Recall)
   - Höhere Komplexität
   - Speicher-Overhead

**OPTIONAL (P3):**
4. 🔮 Late Interaction (ColBERT)
   - Höchste Präzision
   - Hoher Speicher-Overhead (85×)
   - Enterprise Feature

---

### 5.3 Architektur-Integration

**Neue Komponenten in ThemisDB:**

```
src/
├── search/
│   ├── sparse/
│   │   ├── sparse_index_manager.h/cpp    [NEU]
│   │   ├── bm25_scorer.h/cpp             [NEU]
│   │   └── tokenizer.h/cpp               [NEU]
│   ├── hybrid/
│   │   ├── hybrid_search.h/cpp           [NEU]
│   │   ├── rrf_fusion.h/cpp              [NEU]
│   │   └── score_normalizer.h/cpp        [NEU]
│   ├── crossmodal/
│   │   ├── clip_embedder.h/cpp           [NEU]
│   │   ├── image_index_manager.h/cpp     [NEU]
│   │   └── crossmodal_search.h/cpp       [NEU]
│   └── multivec/
│       ├── multi_vector_index.h/cpp      [NEU]
│       └── aspect_scorer.h/cpp           [NEU]
```

**API-Erweiterungen:**
```cpp
// HTTP Endpoints
POST /api/v1/search/hybrid         // Hybrid Search (BM25 + Vector)
POST /api/v1/search/crossmodal     // Cross-Modal Search
POST /api/v1/search/multivec       // Multi-Vector Search

// Configuration
GET  /api/v1/search/config
PUT  /api/v1/search/config

// Index Management
POST /api/v1/search/sparse/index   // Index document for BM25
POST /api/v1/search/image/index    // Index image for CLIP
```

---

## 6. Competitive Landscape

### 6.1 Vergleich mit anderen Systemen

| Feature | ThemisDB (geplant) | Weaviate | Vespa | Milvus | Qdrant |
|---------|------------|----------|-------|--------|--------|
| Dense Vector Search | ✅ (HNSW) | ✅ | ✅ | ✅ | ✅ |
| Sparse BM25 | 🔄 Geplant | ✅ | ✅ | ❌ | ❌ |
| Hybrid Search | 🔄 Geplant | ✅ | ✅ | ❌ | ⚠️ Limited |
| Cross-Modal (CLIP) | 🔄 Geplant | ✅ | ⚠️ Limited | ❌ | ❌ |
| Multi-Vector | 🔄 Geplant | ❌ | ⚠️ Limited | ❌ | ❌ |
| Native LLM | ✅ (llama.cpp) | ❌ | ❌ | ❌ | ❌ |
| Multi-Model DB | ✅ | ❌ | ⚠️ Limited | ❌ | ❌ |
| ACID Transactions | ✅ | ❌ | ⚠️ Limited | ❌ | ❌ |

**Differenzierung:**
- ✅ ThemisDB's **Multi-Model Architektur** (Graph + Vector + Relational + Document)
- ✅ **Native LLM Integration** mit llama.cpp
- ✅ **ACID Transactions** über alle Modelle
- 🔄 Hybrid Search **gleicht Feature-Gap** zu Weaviate/Vespa

---

### 6.2 Market Positioning

**ThemisDB Unique Value Proposition:**

> "The only multi-model database with native LLM integration, ACID transactions, and hybrid search — from edge to cloud."

**Zielgruppen:**
1. **Enterprise:** ACID + Multi-Model + Hybrid Search
2. **Behörden:** On-Premise LLM + Compliance + Hybrid Search
3. **Edge/IoT:** Minimal Edition mit Hybrid Search für Edge Devices

---

## 7. Literatur & Referenzen

### 7.1 Wissenschaftliche Papers

**Hybrid Search:**
1. Cormack, G.V. et al. (2009). "Reciprocal Rank Fusion outperforms Condorcet and individual Rank Learning Methods." SIGIR.
2. Luan, Y. et al. (2021). "Sparse, Dense, and Attentional Representations for Text Retrieval." TACL.
3. Formal, T. et al. (2021). "SPLADE: Sparse Lexical and Expansion Model for First Stage Ranking." SIGIR.

**Late Interaction:**
4. Khattab, O. & Zaharia, M. (2020). "ColBERT: Efficient and Effective Passage Search via Contextualized Late Interaction over BERT." SIGIR.
5. Khattab, O. et al. (2021). "ColBERTv2: Effective and Efficient Retrieval via Lightweight Late Interaction." arXiv:2112.01488.

**Cross-Modal:**
6. Radford, A. et al. (2021). "Learning Transferable Visual Models From Natural Language Supervision." (CLIP) ICML.
7. Jia, C. et al. (2021). "Scaling Up Visual and Vision-Language Representation Learning With Noisy Text Supervision." (ALIGN) ICML.
8. Li, J. et al. (2022). "BLIP: Bootstrapping Language-Image Pre-training for Unified Vision-Language Understanding and Generation." ICML.

**Multi-Vector:**
9. Humeau, S. et al. (2020). "Poly-encoders: Architectures and Pre-training Strategies for Fast and Accurate Multi-sentence Scoring." ICLR.
10. Reimers, N. & Gurevych, I. (2019). "Sentence-BERT: Sentence Embeddings using Siamese BERT-Networks." EMNLP.

**Evaluation:**
11. Thakur, N. et al. (2021). "BEIR: A Heterogeneous Benchmark for Zero-shot Evaluation of Information Retrieval Models." NeurIPS.
12. Nguyen, T. et al. (2016). "MS MARCO: A Human Generated MAchine Reading COmprehension Dataset." NeurIPS.

---

### 7.2 Open-Source Projekte

**Embedding Models:**
- **Sentence Transformers:** https://www.sbert.net/
- **OpenCLIP:** https://github.com/mlfoundations/open_clip
- **E5-Mistral:** https://huggingface.co/intfloat/e5-mistral-7b-instruct

**Retrieval Frameworks:**
- **Haystack:** https://haystack.deepset.ai/ (Hybrid Search Pipeline)
- **LlamaIndex:** https://www.llamaindex.ai/ (RAG mit Hybrid Search)
- **LangChain:** https://www.langchain.com/ (Retrieval Chains)

**Benchmark Tools:**
- **BEIR:** https://github.com/beir-cellar/beir
- **MTEB:** https://github.com/embeddings-benchmark/mteb

---

### 7.3 Industrie-Ressourcen

**Blogs & Tutorials:**
1. Pinecone: "Hybrid Search Explained" - https://www.pinecone.io/learn/hybrid-search/
2. Weaviate: "Hybrid Search in Weaviate" - https://weaviate.io/developers/weaviate/search/hybrid
3. Vespa: "Approximate Nearest Neighbor Search" - https://docs.vespa.ai/en/approximate-nn-hnsw.html

**Benchmarks:**
- Vector Database Benchmark: https://github.com/erikbern/ann-benchmarks
- MTEB Leaderboard: https://huggingface.co/spaces/mteb/leaderboard

---

## 8. Anhang

### 8.1 Glossar

| Begriff | Bedeutung |
|---------|-----------|
| **Dense Retrieval** | Semantische Suche mit gelernten Embeddings |
| **Sparse Retrieval** | Keyword-basierte Suche (BM25, TF-IDF) |
| **Hybrid Search** | Kombination von Dense und Sparse Retrieval |
| **RRF** | Reciprocal Rank Fusion - Score-agnostische Fusion-Methode |
| **HNSW** | Hierarchical Navigable Small World - Algorithmus für ANN |
| **ColBERT** | Late Interaction Model mit Token-Level Embeddings |
| **CLIP** | Vision-Language Model für Cross-Modal Retrieval |
| **MRR** | Mean Reciprocal Rank - Evaluation Metrik |
| **NDCG** | Normalized Discounted Cumulative Gain - Ranking Metrik |
| **ANN** | Approximate Nearest Neighbor - Effiziente Vector Search |

---

### 8.2 Beispiel-Queries

**Hybrid Search Beispiel:**
```bash
# API Request
curl -X POST http://localhost:8080/api/v1/search/hybrid \
  -H "Content-Type: application/json" \
  -d '{
    "query": "machine learning optimization techniques",
    "top_k": 10,
    "fusion_method": "rrf",
    "rrf_k": 60,
    "sparse_weight": 0.5,
    "dense_weight": 0.5
  }'

# Response
{
  "results": [
    {
      "doc_id": "doc123",
      "score": 0.89,
      "sparse_score": 4.2,
      "dense_score": 0.87,
      "rank_sparse": 2,
      "rank_dense": 1,
      "title": "Advanced Optimization in ML"
    },
    ...
  ]
}
```

**Cross-Modal Search Beispiel:**
```bash
# Text-to-Image Search
curl -X POST http://localhost:8080/api/v1/search/crossmodal \
  -H "Content-Type: application/json" \
  -d '{
    "query_text": "sunset over mountains",
    "modality": "text-to-image",
    "top_k": 5,
    "model": "CLIP-ViT-B/32"
  }'

# Response
{
  "results": [
    {
      "image_id": "img456",
      "score": 0.92,
      "url": "https://...",
      "caption": "Beautiful sunset in the Alps"
    },
    ...
  ]
}
```

---

## 9. Zusammenfassung & Handlungsempfehlungen

### 9.1 Key Takeaways

1. **Hybrid Search ist essentiell:**
   - Kombiniert Stärken von Keyword- und Semantischer Suche
   - RRF als robuste, einfache Fusion-Methode
   - 5-15% Verbesserung gegenüber einzelnen Methoden

2. **Cross-Modal Retrieval eröffnet neue Use Cases:**
   - CLIP für Text-Image Search
   - Einfache Integration via Python Microservice oder ONNX
   - Differenzierung im Markt

3. **Multi-Vector Representations für High-Precision:**
   - Aspect-based Embeddings als einfacher Einstieg
   - ColBERT für höchste Präzision (aber hoher Overhead)

4. **ThemisDB hat ideale Basis:**
   - ✅ Vector Search (HNSW) vorhanden
   - ✅ LLM Integration (llama.cpp)
   - ✅ Multi-Model Architektur
   - 🔄 BM25 und Hybrid Search fehlen

---

### 9.2 Konkrete Next Steps

**Für Product Team:**
1. ✅ **Priorisierung:** Hybrid Search als P0 Feature für v1.5
2. ✅ **Roadmap:** 3-4 Monate für Phase 1 (BM25 + RRF)
3. ⏰ **Planung:** Cross-Modal Retrieval als P1 für v1.6

**Für Engineering Team:**
1. ✅ **Spike:** Proof-of-Concept für BM25 Index in RocksDB (1 Sprint)
2. ✅ **Design Review:** Hybrid Search API Design
3. ✅ **Implementation:** SparseIndexManager und RRF Fusion

**Für Research Team:**
1. ✅ **Benchmark:** BEIR Evaluation Setup
2. ✅ **Evaluation:** Baseline mit aktuellen Vector Search
3. ⏰ **Exploration:** CLIP Integration Optionen

---

**Fragen? Feedback?**
- 📧 Email: research@themisdb.com
- 💬 Discussions: https://github.com/makr-code/ThemisDB/discussions
- 🐛 Issues: https://github.com/makr-code/ThemisDB/issues

---

**Erstellt:** 27. Januar 2026  
**Version:** 1.0  
**Status:** ✅ Abgeschlossen  
**Nächste Review:** Q2 2026
