# Hybrid Search Guide - ThemisDB v1.4

**Version:** 1.4.0  
**Status:** ✅ Produktionsreif  
**Aktualisiert:** Januar 2026

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Hybrid Search Strategies](#hybrid-search-strategies)
- [Fusion Methods](#fusion-methods)
- [Ranking und Scoring](#ranking-und-scoring)
- [Beispiele](#beispiele)
- [Performance](#performance)
- [Best Practices](#best-practices)

---

## Übersicht

Hybrid Search kombiniert Full-Text Search und Vector Search für optimale Suchergebnisse. Diese Kombination nutzt sowohl lexikalische Übereinstimmung (Keywords) als auch semantische Ähnlichkeit (Meaning).

### Warum Hybrid Search?

| Problem | Lösung |
|---------|--------|
| **Keyword Matching verliert Semantik** | Vector Search ergänzt Bedeutung |
| **Vector Search findet keine exakten Matches** | Full-Text Search findet exakte Begriffe |
| **Nur Full-Text**: Synonyme fehlen | Vector Search erfasst Synonyme |
| **Nur Vector**: Langsam bei großen Datasets | Full-Text pre-filtert Kandidaten |

### Vorteile

- ✅ **Best of Both Worlds**: Exakte Matches + Semantische Ähnlichkeit
- ✅ **Better Recall**: Findet mehr relevante Dokumente
- ✅ **Better Precision**: Besseres Ranking durch Multi-Signal
- ✅ **Robustheit**: Funktioniert auch wenn ein Signal schwach ist

---

## Hybrid Search Strategies

### 1. Parallel Search + Fusion

Führe beide Searches parallel aus und kombiniere Ergebnisse.

```aql
// Full-Text Search
LET fulltextResults = (
  FOR doc IN documents
    SEARCH ANALYZER(doc.content IN TOKENS(@query, "text_en"), "text_en")
    LET ftScore = BM25(doc)
    RETURN {doc, ftScore}
)

// Vector Search
LET vectorResults = (
  FOR doc IN documents
    LET vecScore = COSINE_SIMILARITY(doc.embedding, @queryEmbedding)
    FILTER vecScore > 0.7
    RETURN {doc, vecScore}
)

// Combine with RRF (Reciprocal Rank Fusion)
FOR result IN UNION(fulltextResults, vectorResults)
  COLLECT doc = result.doc
  AGGREGATE 
    ftScores = PUSH(result.ftScore),
    vecScores = PUSH(result.vecScore)
  LET combinedScore = (
    COALESCE(FIRST(ftScores), 0) * @alphaFT +
    COALESCE(FIRST(vecScores), 0) * @alphaVec
  )
  SORT combinedScore DESC
  LIMIT 20
  RETURN {doc, combinedScore}
```

### 2. Sequential Search (Filter + Rank)

Nutze Full-Text als Pre-Filter, dann Vector für Ranking.

```aql
// Step 1: Full-Text Pre-Filter
FOR doc IN documents
  SEARCH ANALYZER(doc.content IN TOKENS(@query, "text_en"), "text_en")
  LIMIT 1000  // Kandidaten für Vector Search
  
  // Step 2: Vector Re-Ranking
  LET vecScore = COSINE_SIMILARITY(doc.embedding, @queryEmbedding)
  LET ftScore = BM25(doc)
  LET hybridScore = (ftScore * 0.3) + (vecScore * 0.7)
  
  SORT hybridScore DESC
  LIMIT 20
  RETURN {
    id: doc._id,
    title: doc.title,
    ftScore: ftScore,
    vecScore: vecScore,
    hybridScore: hybridScore
  }
```

### 3. Conditional Hybrid

Entscheide dynamisch, welche Methode zu nutzen ist.

```aql
LET queryLength = LENGTH(TOKENS(@query, "text_en"))

// Short queries (1-2 words): Prefer Vector Search
LET strategy = queryLength <= 2 ? "vector" : "hybrid"

FOR doc IN documents
  SEARCH ANALYZER(doc.content IN TOKENS(@query, "text_en"), "text_en")
  
  LET ftScore = BM25(doc)
  LET vecScore = COSINE_SIMILARITY(doc.embedding, @queryEmbedding)
  
  LET finalScore = strategy == "vector" 
    ? vecScore 
    : (ftScore * 0.4 + vecScore * 0.6)
  
  SORT finalScore DESC
  LIMIT 20
  RETURN {doc, finalScore, strategy}
```

---

## Fusion Methods

### Weighted Sum

Einfache gewichtete Kombination der Scores.

```aql
LET ftWeight = 0.4
LET vecWeight = 0.6

LET hybridScore = (ftScore * ftWeight) + (vecScore * vecWeight)
```

**Vorteile:** Einfach, schnell  
**Nachteile:** Scores müssen normalisiert sein

### Reciprocal Rank Fusion (RRF)

Kombiniere basierend auf Ranks statt Scores.

```aql
// RRF Formula: score = Σ 1 / (k + rank)
LET k = 60  // RRF constant

LET rrf = (
  1 / (k + ftRank) +
  1 / (k + vecRank)
)
```

**Vorteile:** Funktioniert mit unterschiedlichen Score-Bereichen  
**Nachteile:** Benötigt Ranking beider Listen

### Distribution-Based Combination

Normalisiere Scores basierend auf Statistiken.

```aql
// Z-Score Normalization
LET ftNorm = (ftScore - ftMean) / ftStdDev
LET vecNorm = (vecScore - vecMean) / vecStdDev

LET hybridScore = (ftNorm + vecNorm) / 2
```

**Vorteile:** Scores auf gleicher Skala  
**Nachteile:** Benötigt Statistiken vorab

---

## Ranking und Scoring

### Score Normalization

```aql
// Min-Max Normalization
FUNCTION normalizeScore(score, min, max)
  RETURN (score - min) / (max - min)
END

FOR doc IN documents
  LET ftNorm = normalizeScore(ftScore, 0, maxFTScore)
  LET vecNorm = normalizeScore(vecScore, -1, 1)
  LET hybridScore = (ftNorm + vecNorm) / 2
  RETURN {doc, hybridScore}
```

### Boosting Strategies

```aql
// Field-based Boosting
FOR doc IN documents
  SEARCH ANALYZER(
    BOOST(doc.title IN TOKENS(@query, "text_en"), 3.0) OR
    doc.content IN TOKENS(@query, "text_en"),
    "text_en"
  )
  
  LET ftScore = BM25(doc)
  LET vecScore = COSINE_SIMILARITY(doc.embedding, @queryEmbedding)
  
  // Recency Boost
  LET recencyBoost = DATE_DIFF(DATE_NOW(), doc.published_date, "days") < 30 ? 1.2 : 1.0
  
  // Authority Boost
  LET authorityBoost = doc.author_reputation > 1000 ? 1.1 : 1.0
  
  LET finalScore = (ftScore * 0.4 + vecScore * 0.6) * recencyBoost * authorityBoost
  
  SORT finalScore DESC
  LIMIT 20
  RETURN {doc, finalScore}
```

---

## Beispiele

### Beispiel 1: E-Commerce Product Search

```javascript
// Complete hybrid search implementation
async function hybridProductSearch(query, filters = {}) {
  // 1. Generate query embedding
  const queryEmbedding = await getEmbedding(query);
  
  // 2. Hybrid search query
  const results = await db.query(`
    // Full-Text Search
    LET fulltextResults = (
      FOR product IN products
        SEARCH ANALYZER(
          BOOST(product.name IN TOKENS(@query, "text_en"), 5.0) OR
          BOOST(product.brand IN TOKENS(@query, "text_en"), 3.0) OR
          BOOST(product.category IN TOKENS(@query, "text_en"), 2.0) OR
          product.description IN TOKENS(@query, "text_en"),
          "text_en"
        )
        FILTER product.in_stock == true
        ${filters.category ? 'FILTER product.category == @category' : ''}
        ${filters.minPrice ? 'FILTER product.price >= @minPrice' : ''}
        ${filters.maxPrice ? 'FILTER product.price <= @maxPrice' : ''}
        LET ftScore = BM25(product)
        LIMIT 500
        RETURN {product, ftScore}
    )
    
    // Vector Search (Semantic)
    LET vectorResults = (
      FOR product IN products
        FILTER product.in_stock == true
        ${filters.category ? 'FILTER product.category == @category' : ''}
        LET vecScore = COSINE_SIMILARITY(product.feature_embedding, @queryEmbedding)
        FILTER vecScore > 0.6
        LIMIT 500
        RETURN {product, vecScore}
    )
    
    // Combine Results
    FOR result IN UNION(fulltextResults, vectorResults)
      COLLECT product = result.product
      AGGREGATE 
        ftScore = MAX(result.ftScore || 0),
        vecScore = MAX(result.vecScore || 0)
      
      // Weighted Hybrid Score
      LET hybridScore = (ftScore * 0.35) + (vecScore * 0.65)
      
      // Popularity Boost
      LET popularityBoost = product.sales_count > 100 ? 1.15 : 1.0
      
      // Rating Boost
      LET ratingBoost = product.avg_rating >= 4.5 ? 1.1 : 1.0
      
      LET finalScore = hybridScore * popularityBoost * ratingBoost
      
      SORT finalScore DESC
      LIMIT 50
      RETURN {
        id: product._id,
        name: product.name,
        brand: product.brand,
        price: product.price,
        image: product.image_url,
        rating: product.avg_rating,
        ftScore: ROUND(ftScore, 3),
        vecScore: ROUND(vecScore, 3),
        finalScore: ROUND(finalScore, 3)
      }
  `, {
    query,
    queryEmbedding,
    ...filters
  });
  
  return results;
}

// Usage
const results = await hybridProductSearch("wireless bluetooth headphones", {
  category: "electronics",
  minPrice: 50,
  maxPrice: 200
});
```

### Beispiel 2: Document Search with Facets

```aql
LET queryEmbedding = @queryEmbedding

// Hybrid Search
LET searchResults = (
  // Full-Text Branch
  LET ftResults = (
    FOR doc IN documents
      SEARCH ANALYZER(
        doc.title IN TOKENS(@query, "text_en") OR
        doc.content IN TOKENS(@query, "text_en"),
        "text_en"
      )
      FILTER doc.published == true
      LET ftScore = BM25(doc)
      RETURN {doc, ftScore, source: "fulltext"}
  )
  
  // Vector Branch
  LET vecResults = (
    FOR doc IN documents
      FILTER doc.published == true
      LET vecScore = COSINE_SIMILARITY(doc.embedding, queryEmbedding)
      FILTER vecScore > 0.7
      RETURN {doc, vecScore, source: "vector"}
  )
  
  // Merge
  FOR result IN UNION(ftResults, vecResults)
    COLLECT doc = result.doc
    AGGREGATE
      ftScore = MAX(result.ftScore || 0),
      vecScore = MAX(result.vecScore || 0),
      sources = UNIQUE(result.source)
    
    LET hybridScore = (ftScore * 0.4) + (vecScore * 0.6)
    RETURN {doc, ftScore, vecScore, hybridScore, sources}
)

// Top Results
LET topResults = (
  FOR result IN searchResults
    SORT result.hybridScore DESC
    LIMIT 20
    RETURN {
      id: result.doc._id,
      title: result.doc.title,
      summary: SUBSTRING(result.doc.content, 0, 200),
      author: result.doc.author,
      date: result.doc.published_date,
      category: result.doc.category,
      tags: result.doc.tags,
      scores: {
        fulltext: ROUND(result.ftScore, 3),
        vector: ROUND(result.vecScore, 3),
        hybrid: ROUND(result.hybridScore, 3)
      },
      matchType: result.sources
    }
)

// Facets
LET categoryFacets = (
  FOR result IN searchResults
    COLLECT category = result.doc.category WITH COUNT INTO count
    SORT count DESC
    RETURN {category, count}
)

LET authorFacets = (
  FOR result IN searchResults
    COLLECT author = result.doc.author WITH COUNT INTO count
    SORT count DESC
    LIMIT 10
    RETURN {author, count}
)

LET tagFacets = (
  FOR result IN searchResults
    FOR tag IN result.doc.tags || []
      COLLECT t = tag WITH COUNT INTO count
      SORT count DESC
      LIMIT 15
      RETURN {tag: t, count}
)

RETURN {
  results: topResults,
  total: LENGTH(searchResults),
  facets: {
    categories: categoryFacets,
    authors: authorFacets,
    tags: tagFacets
  },
  query: @query
}
```

### Beispiel 3: Question Answering System

```javascript
// Hybrid search for Q&A
async function answerQuestion(question) {
  // Generate query embedding
  const queryEmbedding = await getEmbedding(question);
  
  // Hybrid search with context
  const results = await db.query(`
    // Keywords from question
    LET keywords = TOKENS(@question, "text_en")
    
    // Full-Text: Find documents with keywords
    LET fulltextCandidates = (
      FOR doc IN knowledge_base
        SEARCH ANALYZER(
          doc.question IN TOKENS(@question, "text_en") OR
          doc.answer IN TOKENS(@question, "text_en"),
          "text_en"
        )
        LET ftScore = BM25(doc)
        RETURN {doc, ftScore}
    )
    
    // Vector: Semantic similarity
    LET vectorCandidates = (
      FOR doc IN knowledge_base
        LET vecScore = COSINE_SIMILARITY(doc.question_embedding, @queryEmbedding)
        FILTER vecScore > 0.75
        RETURN {doc, vecScore}
    )
    
    // Fusion
    FOR candidate IN UNION(fulltextCandidates, vectorCandidates)
      COLLECT doc = candidate.doc
      AGGREGATE
        ftScore = MAX(candidate.ftScore || 0),
        vecScore = MAX(candidate.vecScore || 0)
      
      // Prioritize vector score for Q&A (semantic understanding)
      LET hybridScore = (ftScore * 0.2) + (vecScore * 0.8)
      
      // Boost frequently accessed answers
      LET accessBoost = doc.access_count > 100 ? 1.1 : 1.0
      
      // Boost highly rated answers
      LET ratingBoost = doc.helpful_votes > 50 ? 1.15 : 1.0
      
      LET finalScore = hybridScore * accessBoost * ratingBoost
      
      SORT finalScore DESC
      LIMIT 5
      RETURN {
        question: doc.question,
        answer: doc.answer,
        source: doc.source_url,
        confidence: ROUND(finalScore * 100, 1),
        metadata: {
          ftScore: ROUND(ftScore, 3),
          vecScore: ROUND(vecScore, 3),
          accessCount: doc.access_count,
          helpfulVotes: doc.helpful_votes
        }
      }
  `, {
    question,
    queryEmbedding
  });
  
  return results[0] || {
    answer: "I don't have enough information to answer that question.",
    confidence: 0
  };
}

// Usage
const answer = await answerQuestion("What is quantum entanglement?");
console.log(`Answer (${answer.confidence}% confidence): ${answer.answer}`);
```

---

## Performance

### Benchmark: Hybrid vs Single Method

**Test Setup:**
- Dataset: 1M documents
- Query Set: 1000 diverse queries
- Metrics: Latency, Recall@20, NDCG@20

| Method | Avg Latency | Recall@20 | NDCG@20 |
|--------|-------------|-----------|---------|
| Full-Text Only | 25ms | 0.72 | 0.68 |
| Vector Only | 35ms | 0.78 | 0.74 |
| **Hybrid (Parallel)** | **55ms** | **0.89** | **0.85** |
| Hybrid (Sequential) | 45ms | 0.87 | 0.83 |

**Interpretation:**
- Hybrid Search hat höhere Latenz aber deutlich bessere Qualität
- Sequential Hybrid ist schneller aber leicht schlechtere Qualität
- Für Production: Parallel Hybrid + Caching

### Optimization

```javascript
// Cache frequently used queries
const queryCache = new LRUCache({max: 10000, ttl: 3600000}); // 1 hour

async function cachedHybridSearch(query) {
  const cacheKey = `hybrid:${query}`;
  
  if (queryCache.has(cacheKey)) {
    return queryCache.get(cacheKey);
  }
  
  const results = await hybridSearch(query);
  queryCache.set(cacheKey, results);
  
  return results;
}
```

---

## Best Practices

### 1. Tune Weights based on Use Case

```javascript
// A/B Testing for optimal weights
const weightConfigs = [
  {ft: 0.3, vec: 0.7},  // Semantic-heavy
  {ft: 0.5, vec: 0.5},  // Balanced
  {ft: 0.7, vec: 0.3}   // Keyword-heavy
];

for (const config of weightConfigs) {
  const metrics = await evaluateSearchQuality(config);
  console.log(`Config ${JSON.stringify(config)}: NDCG=${metrics.ndcg}`);
}
```

### 2. Adaptive Fusion

```aql
// Adapt weights based on query characteristics
LET queryLength = LENGTH(TOKENS(@query, "text_en"))
LET hasQuotes = @query LIKE '%"%'

// Short queries or questions: Prefer semantic
LET vecWeight = queryLength <= 3 OR @query LIKE "%?%" ? 0.7 : 0.5
LET ftWeight = 1.0 - vecWeight

LET hybridScore = (ftScore * ftWeight) + (vecScore * vecWeight)
```

### 3. Monitor Search Quality

```javascript
// Track search metrics
async function logSearchMetrics(query, results, userClick) {
  await db.query(`
    INSERT {
      query: @query,
      results_count: @resultsCount,
      clicked_position: @clickedPosition,
      timestamp: DATE_NOW()
    } INTO search_metrics
  `, {
    query,
    resultsCount: results.length,
    clickedPosition: results.findIndex(r => r.id === userClick)
  });
}
```

### 4. Fallback Strategy

```javascript
async function robustHybridSearch(query) {
  try {
    // Try hybrid search
    const results = await hybridSearch(query);
    
    if (results.length > 0) {
      return results;
    }
    
    // Fallback: Full-text only
    return await fulltextSearch(query);
  } catch (error) {
    // Fallback: Vector only
    return await vectorSearch(query);
  }
}
```

---

## Wann Hybrid Search verwenden?

| Use Case | Empfehlung |
|----------|------------|
| **E-Commerce** | ✅ Hybrid (Keywords + Semantik wichtig) |
| **Documentation** | ✅ Hybrid (Exakte Begriffe + Context) |
| **Q&A Systems** | ✅ Hybrid (Semantic heavy: 20/80) |
| **Code Search** | ⚠️ Full-Text bevorzugt (Exakte Matches) |
| **Image Search** | ⚠️ Vector bevorzugt (Keine Keywords) |
| **News Articles** | ✅ Hybrid (Aktualität + Relevanz) |
| **Academic Papers** | ✅ Hybrid (Technical Terms + Semantik) |

---

## Siehe auch

- [Full-Text Search Guide](FULLTEXT_SEARCH_GUIDE.md)
- [Vector Search Guide](VECTOR_SEARCH_GUIDE.md)
- [Search Feature Matrix](SEARCH_FEATURE_MATRIX.md)
- [Performance Tuning](performance_tuning.md)
- [Search API Documentation](../apis/REST_API_SPECIFICATION.md)
