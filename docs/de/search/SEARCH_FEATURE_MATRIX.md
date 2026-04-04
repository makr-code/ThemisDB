# Search Feature Matrix - ThemisDB v1.4

**Version:** 1.4.0  
**Status:** ✅ Produktionsreif  
**Aktualisiert:** Januar 2026

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Feature-Vergleich](#feature-vergleich)
- [Performance-Vergleich](#performance-vergleich)
- [Use Case Matrix](#use-case-matrix)
- [Limitations](#limitations)
- [Entscheidungshilfe](#entscheidungshilfe)

---

## Übersicht

ThemisDB bietet drei primäre Search-Methoden: **Full-Text Search**, **Vector Search** und **Hybrid Search**. Diese Matrix hilft bei der Auswahl der richtigen Methode.

---

## Feature-Vergleich

| Feature | Full-Text | Vector | Hybrid |
|---------|-----------|--------|--------|
| **Exact Keyword Match** | ✅ Exzellent | ❌ Nicht unterstützt | ✅ Gut |
| **Semantic Understanding** | ❌ Begrenzt | ✅ Exzellent | ✅ Exzellent |
| **Typo Tolerance** | ✅ Mit Fuzzy | ❌ Begrenzt | ✅ Gut |
| **Phrase Search** | ✅ Native | ❌ Nicht unterstützt | ✅ Via Full-Text |
| **Synonym Handling** | ⚠️ Manual | ✅ Automatisch | ✅ Automatisch |
| **Multi-Language** | ✅ Analyzer-basiert | ✅ Embedding-basiert | ✅ Beides |
| **Boolean Operators** | ✅ AND/OR/NOT | ❌ Nicht unterstützt | ✅ Via Full-Text |
| **Ranking Quality** | ⚠️ BM25 | ⚠️ Similarity | ✅ Multi-Signal |
| **Query Latency** | ⚡ Schnell (10-50ms) | ⚡ Schnell (15-80ms) | ⚠️ Medium (40-100ms) |
| **Index Size** | 📊 Klein (10-30% von Daten) | 📊 Groß (50-100% von Daten) | 📊 Groß (60-130% von Daten) |
| **Setup Complexity** | 🟢 Einfach | 🟡 Medium | 🔴 Komplex |
| **Update Performance** | ⚡ Schnell | ⚡ Schnell | ⚠️ Medium |

**Legende:**
- ✅ Exzellent / Vollständig unterstützt
- ⚠️ Begrenzt / Mit Einschränkungen
- ❌ Nicht unterstützt / Nicht empfohlen
- ⚡ Schnell
- 📊 Ressourcen-Nutzung
- 🟢 Niedrig / 🟡 Medium / 🔴 Hoch

---

## Performance-Vergleich

### Latency Benchmarks

**Test Setup:**
- Dataset: 1M Dokumente
- Durchschnittliche Doc-Größe: 2KB
- Hardware: 16 CPU cores, 64GB RAM

| Metric | Full-Text | Vector | Hybrid |
|--------|-----------|--------|--------|
| **Avg Latency** | 25ms | 35ms | 55ms |
| **P50 Latency** | 20ms | 30ms | 50ms |
| **P95 Latency** | 45ms | 75ms | 95ms |
| **P99 Latency** | 85ms | 140ms | 180ms |
| **Throughput** | 600 QPS | 400 QPS | 250 QPS |

### Scaling Characteristics

| Dataset Size | Full-Text Latency | Vector Latency | Hybrid Latency |
|--------------|-------------------|----------------|----------------|
| 100K docs | 8ms | 12ms | 18ms |
| 1M docs | 25ms | 35ms | 55ms |
| 10M docs | 65ms | 80ms | 130ms |
| 100M docs | 180ms | 250ms | 380ms |

### Index Size

| Method | Index Overhead | Build Time (1M docs) |
|--------|----------------|----------------------|
| Full-Text | ~15-30% | 2-5 min |
| Vector | ~50-100% | 8-12 min |
| Hybrid | ~65-130% | 10-17 min |

---

## Use Case Matrix

### Empfohlene Methode nach Use Case

| Use Case | Best Method | Reason |
|----------|-------------|--------|
| **E-Commerce Product Search** | 🟡 Hybrid | Keywords + Semantik wichtig |
| **Documentation Search** | 🟡 Hybrid | Exakte Begriffe + Context |
| **Code Search** | 🔵 Full-Text | Exakte Syntax-Matches |
| **Question Answering** | 🟢 Vector | Semantisches Verständnis |
| **News Search** | 🟡 Hybrid | Keywords + Relevanz |
| **Academic Papers** | 🟡 Hybrid | Technical Terms + Semantik |
| **Image Search** | 🟢 Vector | Keine Keywords, visuelle Ähnlichkeit |
| **Chat/Message Search** | 🔵 Full-Text | Exakte Phrasen wichtig |
| **Blog/CMS** | 🟡 Hybrid | Title-Boost + Content-Semantik |
| **Legal Documents** | 🔵 Full-Text | Exakte Legal Terms |
| **Medical Records** | 🟡 Hybrid | Medical Terms + Semantik |
| **Customer Support KB** | 🟡 Hybrid | Keywords + Intent |
| **Social Media** | 🟢 Vector | Trending Topics, Semantik |
| **Music/Audio Search** | 🟢 Vector | Audio Embeddings |
| **Recipe Search** | 🟡 Hybrid | Ingredients (exact) + Style (semantic) |

**Legende:**
- 🔵 Full-Text Only
- 🟢 Vector Only
- 🟡 Hybrid (kombiniert)

---

## Detailed Comparison

### Full-Text Search

**Stärken:**
- ✅ Schnell (10-50ms durchschnittlich)
- ✅ Exakte Keyword-Matches
- ✅ Boolean Queries (AND/OR/NOT)
- ✅ Phrase Search
- ✅ Fuzzy Matching
- ✅ Kleiner Index-Footprint
- ✅ Einfaches Setup

**Schwächen:**
- ❌ Kein semantisches Verständnis
- ❌ Synonyme müssen manuell definiert werden
- ❌ Keine Cross-Lingual Search (ohne Übersetzung)
- ❌ Ranking basiert nur auf Term-Frequenz

**Best For:**
- Exakte Keyword-Suche
- Code/Technical Documentation
- Legal/Compliance Documents
- Logs und Error Messages

### Vector Search

**Stärken:**
- ✅ Semantisches Verständnis
- ✅ Automatische Synonym-Behandlung
- ✅ Cross-Lingual (mit Multi-Lingual Embeddings)
- ✅ Findet ähnliche Konzepte
- ✅ Funktioniert mit verschiedenen Modalitäten (Text, Image, Audio)

**Schwächen:**
- ❌ Keine exakten Keyword-Matches
- ❌ Größerer Index (Embedding-Vektoren)
- ❌ Langsamer als Full-Text (bei gleicher Datenmenge)
- ❌ Embedding-Generierung erforderlich
- ❌ Schwierig zu debuggen

**Best For:**
- Semantic Search
- Recommendation Systems
- Question Answering
- Image/Audio Search
- Multi-Language Content

### Hybrid Search

**Stärken:**
- ✅ Best of Both Worlds
- ✅ Bessere Recall und Precision
- ✅ Robust gegen verschiedene Query-Typen
- ✅ Flexibles Ranking

**Schwächen:**
- ❌ Höhere Latenz (kombiniert beide Methoden)
- ❌ Größerer Index-Footprint
- ❌ Komplexeres Setup
- ❌ Mehr Tuning erforderlich (Weights)
- ❌ Höhere Compute-Kosten

**Best For:**
- E-Commerce
- Content Management
- Enterprise Search
- Knowledge Bases
- Academic Search

---

## Limitations

### Full-Text Search

| Limitation | Value | Impact |
|------------|-------|--------|
| Max Token Length | 40 chars (default) | Lange Wörter werden gekürzt |
| Max Document Size | 16MB | Sehr große Dokumente müssen gesplittet werden |
| Analyzer Complexity | Medium | Custom Analyzers erfordern Expertise |
| Cross-Field Scoring | Limited | Multi-Field Queries sind komplex |

### Vector Search

| Limitation | Value | Impact |
|------------|-------|--------|
| Max Dimensions | 2048 | Limitiert Embedding-Modell-Wahl |
| Index Build Time | Hours (für 100M) | Lange Initial Build Time |
| Memory Requirements | High | RAM für gesamten Index benötigt |
| Exact Match Recall | Low | Findet keine exakten Keywords |

### Hybrid Search

| Limitation | Value | Impact |
|------------|-------|--------|
| Query Latency | 2x Single Method | Höhere Response Time |
| Configuration Complexity | High | Viele Parameter zu tunen |
| Score Normalization | Required | Zusätzliche Verarbeitung |
| Cache Invalidation | Complex | Cache-Strategie nicht trivial |

---

## Entscheidungshilfe

### Decision Tree

```
START
  │
  ├─ Brauchen Sie exakte Keyword-Matches? ─── JA ──┐
  │                                                  │
  │  Brauchen Sie auch semantisches Verständnis? ─ JA ─→ HYBRID
  │                                                  │
  │                                                 NEIN
  │                                                  │
  │                                                  └─→ FULL-TEXT
  │
  └─ NEIN
     │
     Ist Ihre Suche primär semantisch?
     (z.B. ähnliche Bilder, Bedeutung wichtiger als Wortlaut)
     │
     JA ─→ VECTOR
     │
     NEIN ─→ HYBRID (zur Sicherheit)
```

### Quick Guide

**Wähle Full-Text wenn:**
- ✅ Exakte Keywords kritisch sind
- ✅ Low Latency erforderlich (< 50ms)
- ✅ Dokumente haben klare Keywords
- ✅ Boolean Logic benötigt wird
- ✅ Budget/Ressourcen begrenzt sind

**Wähle Vector wenn:**
- ✅ Semantik wichtiger als Keywords
- ✅ Multi-Language Support benötigt
- ✅ Recommendation System
- ✅ Nicht-Text-Daten (Images, Audio)
- ✅ Synonyme automatisch erkannt werden sollen

**Wähle Hybrid wenn:**
- ✅ Beide Aspekte wichtig sind
- ✅ Höchste Search Quality benötigt
- ✅ E-Commerce oder Enterprise Search
- ✅ Budget für höhere Infrastruktur vorhanden
- ✅ Bereit, Zeit in Tuning zu investieren

---

## Configuration Examples

### Full-Text Setup

```aql
// 1. Create Index
CREATE FULLTEXT INDEX idx_content ON articles(title, content)
  ANALYZER "text_en"

// 2. Query
FOR doc IN articles
  SEARCH ANALYZER(
    BOOST(doc.title IN TOKENS(@query, "text_en"), 3.0) OR
    doc.content IN TOKENS(@query, "text_en"),
    "text_en"
  )
  LET score = BM25(doc)
  SORT score DESC
  LIMIT 20
  RETURN {doc, score}
```

### Vector Setup

```aql
// 1. Create Index
CREATE VECTOR INDEX idx_embedding ON articles(embedding)
  DIMENSIONS 768
  METRIC cosine
  OPTIONS {M: 16, efConstruction: 200}

// 2. Generate Embeddings (Python)
from sentence_transformers import SentenceTransformer
model = SentenceTransformer('all-MiniLM-L6-v2')
embedding = model.encode(text)

// 3. Query
FOR doc IN articles
  LET similarity = COSINE_SIMILARITY(doc.embedding, @queryEmbedding)
  FILTER similarity > 0.7
  SORT similarity DESC
  LIMIT 20
  RETURN {doc, similarity}
```

### Hybrid Setup

```aql
// 1. Create Both Indexes
CREATE FULLTEXT INDEX idx_ft ON articles(title, content)
CREATE VECTOR INDEX idx_vec ON articles(embedding) DIMENSIONS 768

// 2. Hybrid Query
LET ftResults = (
  FOR doc IN articles
    SEARCH ANALYZER(doc.content IN TOKENS(@query, "text_en"), "text_en")
    LET ftScore = BM25(doc)
    RETURN {doc, ftScore}
)

LET vecResults = (
  FOR doc IN articles
    LET vecScore = COSINE_SIMILARITY(doc.embedding, @queryEmbedding)
    FILTER vecScore > 0.7
    RETURN {doc, vecScore}
)

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

## Benchmarking Your Use Case

### Test Framework

```javascript
async function benchmarkSearchMethods(testQueries) {
  const results = {
    fulltext: [],
    vector: [],
    hybrid: []
  };
  
  for (const query of testQueries) {
    // Full-Text
    const ftStart = Date.now();
    const ftResults = await fulltextSearch(query);
    results.fulltext.push({
      query,
      latency: Date.now() - ftStart,
      resultCount: ftResults.length
    });
    
    // Vector
    const vecStart = Date.now();
    const vecResults = await vectorSearch(query);
    results.vector.push({
      query,
      latency: Date.now() - vecStart,
      resultCount: vecResults.length
    });
    
    // Hybrid
    const hybStart = Date.now();
    const hybResults = await hybridSearch(query);
    results.hybrid.push({
      query,
      latency: Date.now() - hybStart,
      resultCount: hybResults.length
    });
  }
  
  // Calculate statistics
  for (const method of ['fulltext', 'vector', 'hybrid']) {
    const latencies = results[method].map(r => r.latency);
    console.log(`${method}:`);
    console.log(`  Avg Latency: ${avg(latencies)}ms`);
    console.log(`  P95 Latency: ${percentile(latencies, 95)}ms`);
    console.log(`  P99 Latency: ${percentile(latencies, 99)}ms`);
  }
  
  return results;
}
```

---

## Siehe auch

- [Full-Text Search Guide](FULLTEXT_SEARCH_GUIDE.md)
- [Vector Search Guide](VECTOR_SEARCH_GUIDE.md)
- [Hybrid Search Guide](HYBRID_SEARCH_GUIDE.md)
- [Performance Tuning](performance_tuning.md)
- [Search API Documentation](../apis/REST_API_SPECIFICATION.md)
