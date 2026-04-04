# Full-Text Search Guide - ThemisDB v1.4

**Version:** 1.4.0  
**Status:** ✅ Produktionsreif  
**Aktualisiert:** Januar 2026

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Full-Text Index](#full-text-index)
- [Search Syntax](#search-syntax)
- [Analyzers](#analyzers)
- [Ranking und Scoring](#ranking-und-scoring)
- [Beispiele](#beispiele)
- [Performance](#performance)
- [Best Practices](#best-practices)

---

## Übersicht

ThemisDB bietet leistungsstarke Full-Text Search Funktionen basierend auf invertierten Indizes. Full-Text Search ermöglicht schnelle Textsuchen in großen Dokumentensammlungen mit Unterstützung für Ranking, Fuzzy Search und mehrere Sprachen.

### Features

- ✅ **Inverted Index**: Schnelle Token-basierte Suche
- ✅ **Multi-Language Support**: Analyzer für Deutsch, Englisch, Französisch, etc.
- ✅ **Fuzzy Matching**: Tippfehlertoleranz mit Levenshtein-Distanz
- ✅ **Phrase Search**: Exakte Phrasen-Suche
- ✅ **BM25 Ranking**: Relevanz-Scoring nach BM25-Algorithmus
- ✅ **Boolean Queries**: AND, OR, NOT Operatoren
- ✅ **Highlighting**: Treffer-Hervorhebung

---

## Full-Text Index

### Index erstellen

```aql
// Einfacher Full-Text Index
CREATE FULLTEXT INDEX idx_content ON articles(content)

// Mit Analyzer
CREATE FULLTEXT INDEX idx_content_de ON articles(content)
  ANALYZER "text_de"

// Multi-Field Index
CREATE FULLTEXT INDEX idx_article_search ON articles(title, content, tags)
  ANALYZER "text_en"
```

### Index-Optionen

```aql
CREATE FULLTEXT INDEX idx_advanced ON articles(content)
  ANALYZER "text_en"
  OPTIONS {
    minLength: 2,           // Minimale Token-Länge
    maxLength: 40,          // Maximale Token-Länge
    includeAllFields: false,
    trackListPositions: true,
    storeValues: "full"
  }
```

---

## Search Syntax

### Einfache Suche

```aql
// Suche nach "quantum computing"
FOR doc IN articles
  SEARCH ANALYZER(doc.content IN TOKENS("quantum computing", "text_en"), "text_en")
  RETURN doc
```

### Boolean Operators

```aql
// AND: Beide Begriffe müssen vorkommen
FOR doc IN articles
  SEARCH ANALYZER(
    doc.content IN TOKENS("quantum", "text_en") AND
    doc.content IN TOKENS("computing", "text_en"),
    "text_en"
  )
  RETURN doc
```

```aql
// OR: Mindestens ein Begriff
FOR doc IN articles
  SEARCH ANALYZER(
    doc.content IN TOKENS("quantum", "text_en") OR
    doc.content IN TOKENS("physics", "text_en"),
    "text_en"
  )
  RETURN doc
```

```aql
// NOT: Exkludiere Begriffe
FOR doc IN articles
  SEARCH ANALYZER(
    doc.content IN TOKENS("quantum", "text_en") AND
    NOT doc.content IN TOKENS("mechanics", "text_en"),
    "text_en"
  )
  RETURN doc
```

### Phrase Search

```aql
// Exakte Phrase
FOR doc IN articles
  SEARCH PHRASE(doc.content, "quantum computing", "text_en")
  RETURN doc
```

### Fuzzy Search

```aql
// Tippfehlertoleranz (Levenshtein-Distanz)
FOR doc IN articles
  SEARCH LEVENSHTEIN_MATCH(
    doc.content,
    "quantm",  // Typo
    2,         // Max distance
    false,     // Case sensitive
    "text_en"
  )
  RETURN doc
```

### Prefix Search

```aql
// Suche nach Wörtern, die mit "comp" beginnen
FOR doc IN articles
  SEARCH STARTS_WITH(doc.content, "comp", "text_en")
  RETURN doc
```

---

## Analyzers

### Verfügbare Analyzers

| Analyzer | Sprache | Funktionen |
|----------|---------|------------|
| `text_de` | Deutsch | Stemming, Stopwords, Normalisierung |
| `text_en` | Englisch | Stemming, Stopwords, Normalisierung |
| `text_fr` | Französisch | Stemming, Stopwords, Normalisierung |
| `text_es` | Spanisch | Stemming, Stopwords, Normalisierung |
| `identity` | Keine | Keine Transformation |
| `delimiter` | Keine | Token-Splitting nach Delimiter |
| `stem` | Configurable | Nur Stemming |
| `norm` | Configurable | Nur Normalisierung |

### Custom Analyzer

```javascript
// Server-seitig: Custom Analyzer definieren
db.analyzer.create({
  name: "custom_analyzer",
  type: "text",
  properties: {
    locale: "en",
    case: "lower",
    stopwords: ["the", "a", "an"],
    accent: true,
    stemming: true
  }
});
```

### Analyzer-Eigenschaften

```aql
// Case-insensitive Search
FOR doc IN articles
  SEARCH ANALYZER(
    doc.content IN TOKENS("QUANTUM", "text_en"),
    "text_en"
  )
  RETURN doc
// Findet "quantum", "Quantum", "QUANTUM"
```

```aql
// Stemming (Wortformen)
FOR doc IN articles
  SEARCH ANALYZER(
    doc.content IN TOKENS("running", "text_en"),
    "text_en"
  )
  RETURN doc
// Findet "run", "running", "runs", "ran"
```

---

## Ranking und Scoring

### BM25 Scoring

ThemisDB verwendet BM25 (Best Matching 25) für Relevanz-Scoring.

**BM25 Formula:**
```
score(D,Q) = Σ IDF(qi) · (f(qi,D) · (k1 + 1)) / (f(qi,D) + k1 · (1 - b + b · |D| / avgdl))
```

Wo:
- `D` = Dokument
- `Q` = Query
- `f(qi,D)` = Term-Frequenz von Query-Term `qi` in Dokument `D`
- `|D|` = Dokument-Länge
- `avgdl` = Durchschnittliche Dokument-Länge
- `k1`, `b` = Tuning-Parameter

### Scoring in Queries

```aql
// Mit BM25 Score
FOR doc IN articles
  SEARCH ANALYZER(doc.content IN TOKENS("quantum computing", "text_en"), "text_en")
  LET score = BM25(doc)
  SORT score DESC
  RETURN {doc, score}
```

### Score Boosting

```aql
// Field Boosting: Titel wichtiger als Content
FOR doc IN articles
  SEARCH ANALYZER(
    BOOST(doc.title IN TOKENS("quantum", "text_en"), 2.0) OR
    doc.content IN TOKENS("quantum", "text_en"),
    "text_en"
  )
  LET score = BM25(doc)
  SORT score DESC
  RETURN {doc, score}
```

---

## Beispiele

### Beispiel 1: Einfache Suche mit Highlighting

```aql
FOR doc IN articles
  SEARCH ANALYZER(doc.content IN TOKENS(@query, "text_en"), "text_en")
  LET score = BM25(doc)
  LET snippet = SUBSTRING(doc.content, 0, 200)
  SORT score DESC
  LIMIT 10
  RETURN {
    id: doc._id,
    title: doc.title,
    snippet: snippet,
    score: score
  }
```

**Bind Variables:**
```json
{
  "query": "quantum computing"
}
```

**Result:**
```json
{
  "id": "articles/123",
  "title": "Introduction to Quantum Computing",
  "snippet": "Quantum computing is a rapidly-advancing technology that harnesses the laws of quantum mechanics to solve problems too complex for classical computers...",
  "score": 8.45
}
```

### Beispiel 2: Multi-Field Search

```aql
FOR doc IN articles
  SEARCH ANALYZER(
    doc.title IN TOKENS(@query, "text_en") OR
    doc.content IN TOKENS(@query, "text_en") OR
    doc.tags ANY IN TOKENS(@query, "text_en"),
    "text_en"
  )
  LET score = BM25(doc)
  SORT score DESC
  LIMIT 20
  RETURN {
    title: doc.title,
    author: doc.author,
    date: doc.published_date,
    score: score
  }
```

### Beispiel 3: Faceted Search

```aql
// Haupt-Suche
LET searchResults = (
  FOR doc IN articles
    SEARCH ANALYZER(doc.content IN TOKENS(@query, "text_en"), "text_en")
    RETURN doc
)

// Facetten berechnen
LET categoryFacets = (
  FOR doc IN searchResults
    COLLECT category = doc.category WITH COUNT INTO count
    RETURN {category, count}
)

LET authorFacets = (
  FOR doc IN searchResults
    COLLECT author = doc.author WITH COUNT INTO count
    SORT count DESC
    LIMIT 10
    RETURN {author, count}
)

// Top-Ergebnisse
LET topResults = (
  FOR doc IN searchResults
    LET score = BM25(doc)
    SORT score DESC
    LIMIT 10
    RETURN {doc, score}
)

RETURN {
  results: topResults,
  facets: {
    categories: categoryFacets,
    authors: authorFacets
  },
  total: LENGTH(searchResults)
}
```

---

## Performance

### Benchmark-Ergebnisse

**Test Setup:**
- Collection: 1M Dokumente
- Durchschnittliche Dokument-Größe: 2KB
- Index-Größe: ~500MB
- Hardware: 16 CPU cores, 64GB RAM

| Query Type | Avg. Latency | P95 Latency | Throughput |
|------------|--------------|-------------|------------|
| Single Term | 15ms | 25ms | 1000 QPS |
| Multi-Term (AND) | 25ms | 45ms | 600 QPS |
| Phrase Search | 35ms | 65ms | 400 QPS |
| Fuzzy Search | 50ms | 90ms | 250 QPS |
| Complex Boolean | 45ms | 85ms | 300 QPS |

### Performance-Optimierung

**1. Index nur benötigte Felder:**
```aql
// ✅ Gut: Nur title und content
CREATE FULLTEXT INDEX idx_search ON articles(title, content)

// ❌ Schlecht: Alle Felder
CREATE FULLTEXT INDEX idx_all ON articles(*)
```

**2. Analyzer-Wahl:**
```aql
// ✅ Schnell: Einfacher Analyzer
ANALYZER "identity"

// ⚠️ Langsamer: Full-Text Analyzer
ANALYZER "text_en"  // Mit Stemming, Stopwords
```

**3. Query-Komplexität reduzieren:**
```aql
// ✅ Schnell: Einfache Query
SEARCH doc.content IN TOKENS("quantum", "text_en")

// ❌ Langsam: Komplexe Boolean Query
SEARCH (doc.title IN TOKENS("quantum", "text_en") OR
        doc.content IN TOKENS("quantum", "text_en")) AND
       NOT doc.tags ANY IN TOKENS("draft", "text_en")
```

### Limitations

- **Max Token Length**: 40 Zeichen (default)
- **Max Document Size**: 16MB per document
- **Max Index Size**: Limitiert durch verfügbaren RAM
- **Query Timeout**: 30 Sekunden (default, configurable)

---

## Best Practices

### 1. Analyzer richtig wählen

```aql
// ✅ Sprachspezifisch
FOR doc IN articles
  FILTER doc.language == "de"
  SEARCH ANALYZER(doc.content IN TOKENS(@query, "text_de"), "text_de")
  RETURN doc
```

### 2. Index Maintenance

```javascript
// Regelmäßige Index-Optimierung
db.query(`
  FOR doc IN articles
    OPTIMIZE INDEX idx_content
`);
```

### 3. Pagination implementieren

```aql
FOR doc IN articles
  SEARCH ANALYZER(doc.content IN TOKENS(@query, "text_en"), "text_en")
  LET score = BM25(doc)
  SORT score DESC
  LIMIT @offset, @limit
  RETURN {doc, score}
```

### 4. Query Caching nutzen

```aql
FOR doc IN articles
  OPTIONS {cache: true}
  SEARCH ANALYZER(doc.content IN TOKENS(@query, "text_en"), "text_en")
  RETURN doc
```

### 5. Error Handling

```javascript
try {
  const results = await db.query(query, bindVars);
  return results;
} catch (error) {
  if (error.code === 'FULLTEXT_QUERY_TIMEOUT') {
    console.error('Query timeout - reduce complexity');
  } else if (error.code === 'FULLTEXT_INDEX_NOT_FOUND') {
    console.error('Index missing - create fulltext index');
  }
  throw error;
}
```

---

## Use Cases

### 1. Blog/CMS Search

```aql
// Blog-Artikel durchsuchen
FOR doc IN blog_posts
  SEARCH ANALYZER(
    BOOST(doc.title IN TOKENS(@query, "text_en"), 3.0) OR
    BOOST(doc.excerpt IN TOKENS(@query, "text_en"), 2.0) OR
    doc.content IN TOKENS(@query, "text_en"),
    "text_en"
  )
  FILTER doc.published == true
  LET score = BM25(doc)
  SORT score DESC
  LIMIT 20
  RETURN {
    title: doc.title,
    excerpt: doc.excerpt,
    url: doc.url,
    score: score
  }
```

### 2. E-Commerce Product Search

```aql
// Produktsuche
FOR product IN products
  SEARCH ANALYZER(
    BOOST(product.name IN TOKENS(@query, "text_en"), 5.0) OR
    BOOST(product.brand IN TOKENS(@query, "text_en"), 3.0) OR
    BOOST(product.category IN TOKENS(@query, "text_en"), 2.0) OR
    product.description IN TOKENS(@query, "text_en"),
    "text_en"
  )
  FILTER product.in_stock == true
  LET score = BM25(product)
  SORT score DESC
  LIMIT 50
  RETURN {
    id: product._id,
    name: product.name,
    brand: product.brand,
    price: product.price,
    image: product.image_url,
    score: score
  }
```

### 3. Documentation Search

```aql
// Dokumentations-Suche mit Highlighting
FOR doc IN documentation
  SEARCH ANALYZER(
    doc.title IN TOKENS(@query, "text_en") OR
    doc.content IN TOKENS(@query, "text_en"),
    "text_en"
  )
  LET score = BM25(doc)
  LET matchedTerms = TOKENS(@query, "text_en")
  LET contentSnippet = SUBSTRING(doc.content, 0, 300)
  SORT score DESC
  LIMIT 15
  RETURN {
    title: doc.title,
    url: doc.url,
    snippet: contentSnippet,
    version: doc.version,
    score: score,
    matchedTerms: matchedTerms
  }
```

---

## Siehe auch

- [Vector Search Guide](VECTOR_SEARCH_GUIDE.md)
- [Hybrid Search Guide](HYBRID_SEARCH_GUIDE.md)
- [Search Feature Matrix](SEARCH_FEATURE_MATRIX.md)
- [AQL Syntax Guide](../aql/AQL_SYNTAX_GUIDE.md)
- [Performance Tuning](performance_tuning.md)
