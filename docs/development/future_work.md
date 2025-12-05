# Search & Relevance – Future Work

**Stand:** 5. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** Development

---


**Status:** v1 Complete (BM25 HTTP + Hybrid Fusion) – v2 Planning

<<<<<<< Updated upstream
=======
> Verification – 16. November 2025
> - Kurze Überprüfung gegen den Quellcode:
>   - Gefunden/implementiert: BM25 + FULLTEXT AQL Integration, Hybrid Text+Vector Fusion, Stemming/Analyzer, VectorIndex (HNSW optional), SemanticCache, HKDFCache, TSStore + Gorilla Codec, ContentManager ZSTD Wrapper.
>   - Fehlend / nur dokumentiert: CDC/Changefeed HTTP Endpoints (GET /changefeed, SSE), FieldEncryption batch API (`encryptEntityBatch`) und PKI/eIDAS Signaturen (Design vorhanden, produktive Implementierung fehlt).
>   - Empfehlung: Nächster Implementierungsschritt: CDC/Changefeed (MVP) — siehe `docs/development/todo.md` für Details.

>>>>>>> Stashed changes
## Implemented Features (v1)

### ✅ BM25 Fulltext Search (Commit 94af141)
- **API:** POST /search/fulltext
- **Scoring:** Okapi BM25 (k1=1.2, b=0.75)
- **Index:** TF/DocLength automatic maintenance
- **Response:** {pk, score} sorted by relevance
- **Tests:** 10/10 passed

### ✅ Hybrid Text+Vector Fusion (Commit e55508a)
- **API:** POST /search/fusion
- **Modes:** RRF (rank-based) and Weighted (score-based)
- **Flexibility:** Text-only, Vector-only, or combined
- **Normalization:** Min-Max for weighted, reciprocal rank for RRF
- **Tests:** No regressions in fulltext suite

### ✅ Stemming & Analyzer Extensions (v1.2)
- **Implementation:** Porter-Subset (EN), simplified suffix removal (DE)
- **Configuration:** Per-index via `POST /index/create` with:
  ```json
  {
    "type": "fulltext",
    "config": {
      "stemming_enabled": true,
      "language": "en"  // en | de | none
    }
  }
  ```
- **Index Maintenance:** Consistent tokenization in Put/Delete/Rebuild
- **Query-Time:** Automatically uses index config for query tokens
- **Storage:** Config persisted in `ftidxmeta:table:column` as JSON
- **Backward Compatible:** Default `{stemming_enabled: false, language: "none"}`
- **Tests:** 16/16 stemming tests passed + 10/10 fulltext regression tests
- **HTTP API:** `/index/create` with `type: "fulltext"` and optional `config`
- **OpenAPI:** Documented in openapi.yaml with examples
- **Stopwords:** Pro-Index konfigurierbar (Default-Listen EN/DE, Custom-Liste)

### ✅ AQL Integration: FULLTEXT Operator (v1.3)

**Goal:** Implement FULLTEXT(field, query) operator in AQL

**Status:** ✅ Implementiert (aql_translator.cpp lines 101-174)

**Features:**
- Syntax: `FULLTEXT(doc.field, "query" [, limit])`
- Standalone FULLTEXT queries
- FULLTEXT + AND Kombinationen (hybride Suche)
- FULLTEXT + OR via DisjunctiveQuery
- Integration mit BM25() Scoring

**Beispiel-Queries:**
```aql
-- Simple FULLTEXT
FOR doc IN articles
  FILTER FULLTEXT(doc.content, "machine learning")
  RETURN doc

-- FULLTEXT + BM25 scoring
FOR doc IN articles
  FILTER FULLTEXT(doc.content, "machine learning")
  SORT BM25(doc) DESC
  LIMIT 10
  RETURN {title: doc.title, score: BM25(doc)}

-- FULLTEXT + AND (hybrid)
FOR doc IN articles
  FILTER FULLTEXT(doc.content, "neural networks") AND doc.year == "2024"
  RETURN doc

-- FULLTEXT + OR (disjunctive)
FOR doc IN articles
  FILTER FULLTEXT(doc.content, "AI") OR doc.category == "research"
  RETURN doc
```

**Tests:** 23/23 green (`test_aql_fulltext.cpp`, `test_aql_fulltext_hybrid.cpp`)

### ✅ AQL Integration: BM25(doc) Function (v1.3)

**Goal:** Enable BM25 scoring in AQL queries with SORT support

**Status:** ✅ Implementiert

**Implementation Details:**
1. Query Engine Extension (query_engine.cpp)
   - Neue Methode: `executeAndKeysWithScores()` liefert `KeysWithScores`
   - Score-Map aus `scanFulltextWithScores()`
   - Scores bleiben über AND-Intersections mit Strukturprädikaten erhalten

2. Function Evaluation (query_engine.cpp lines 963-982)
   - `BM25(doc)` liest Score aus `ctx.getBm25ScoreForPk(pk)`
   - 0.0 Fallback, wenn kein Score vorhanden
   - Extrahiert `_key` oder `_pk` aus dem Dokumentobjekt

3. SORT Integration
   - `SORT BM25(doc) DESC` nutzt Score aus EvaluationContext
   - Automatische Befüllung via `ctx.setBm25Scores()` bei FULLTEXT

**Beispiel-Query:**
```aql
FOR doc IN articles
  FILTER FULLTEXT(doc.content, "machine learning")
  SORT BM25(doc) DESC
  LIMIT 10
  RETURN {title: doc.title, score: BM25(doc)}
```

**Tests:** 4/4 grün (`test_aql_bm25.cpp`)
- BasicBM25FunctionParsing
- ExecuteAndKeysWithScores
- BM25ScoresDecreaseWithRelevance
- NoScoresForNonFulltextQuery

## Future Work (v2+)

### ✅ Advanced Analyzer Extensions

**Goal:** Extend stemming with additional linguistic features

**Potential Enhancements:**
1. ~~Stopword Filtering~~
  - Implemented in v1.2 (Default EN/DE + Custom per Index)

2. ~~**Umlaut Normalization (German)**~~
   - ✅ **Implemented in v1.2** (normalize_umlauts config option)
   - Normalize "ä→a", "ö→o", "ü→u", "ß→ss"
   - Improves matching for search queries without special chars
   - Example: "läuft" → "lauft" (stems to "lauf")
   - Implementation: `utils::Normalizer::normalizeUmlauts()`
   - Tests: `test_normalization.cpp` (2/2 passing)

3. **Compound Word Splitting (German)**
   - Split "Fußballweltmeisterschaft" → "fußball welt meisterschaft"
   - Critical for German precision/recall
   - Requires dictionary or ML-based approach

4. **Lemmatization (vs. Stemming)**
   - More accurate morphological analysis
   - "running" → "run", "better" → "good"
   - Requires POS tagging and lexicon

**Effort Estimate:** 2-5 days (depending on scope)
- Stopwords: 4-6 hours
- Umlaut normalization: 2-3 hours
- Compound splitting: 1-2 days (complex)
- Lemmatization: 2-3 days (requires NLP library)

**Complexity:** Medium-High
- Stopwords: Low
- Normalization: Low
- Compound splitting: High (ambiguity resolution)
- Lemmatization: High (dependency on NLP toolkit)

**Priority:** Medium
- Stopwords: High value/effort ratio
- Umlaut normalization: High for German content
- Compound splitting: Nice-to-have (complex)
- Lemmatization: Overkill for most use cases (stemming sufficient)

**Alternative Analyzers (Future):**
- N-Grams (for partial matching, typo tolerance)
- Phonetic matching (Soundex, Metaphone for fuzzy search)
- Synonym expansion
- Stop-word removal

---

### 🔲 Position-based Phrase Search

**Goal:** Replace substring-based phrases with true position-aware phrase matching

**Example:**
```json
{
  "query": "\"machine learning\"",
  "match": "exact phrase only, not 'machine' and 'learning' separately"
}
```

**Requirements:**
- Extend index to store token positions (position arrays alongside TF)
- Phrase query parser: detect quoted strings
- Proximity verification: ensure tokens appear consecutively (or within k-window)

**Effort:** 2-3 days (incremental over current substring approach)

---

### 🔲 Query Highlighting

**Goal:** Return matched terms/snippets in response

**Example Response:**
```json
{
  "pk": "doc123",
  "score": 8.5,
  "highlights": {
    "content": "...with <em>machine learning</em> algorithms..."
  }
}
```

**Requirements:**
- Extract matched tokens from query
- Locate occurrences in document text
- Generate snippets with highlighting markup

**Effort:** 1-2 days

---

### 🔲 Learned Fusion (ML-based Ranking)

**Goal:** Replace hand-tuned fusion with learned weights

**Approach:**
- Collect query logs with relevance judgments
- Train LambdaMART/LightGBM ranker
- Features: BM25 score, Vector similarity, metadata signals
- Online serving: predict fusion weights per query

**Effort:** 1-2 weeks (requires ML infrastructure)

---

### 🔲 Multi-Stage Retrieval Pipeline

**Goal:** Efficient retrieval → reranking architecture

**Stages:**
1. **Retrieval** (fast, high recall): Fusion search with k=1000
2. **Reranking** (slow, high precision): Cross-encoder on top-100
3. **Diversification** (optional): MMR for result diversity

**Effort:** 2-3 days (without Cross-Encoder integration)

---

## Implementation Priority

**High Priority (v2):**
1. ✅ BM25 HTTP API (DONE)
2. ✅ Hybrid Fusion (DONE)
3. 🔲 Stemming (DE/EN) – **Next**
4. 🔲 AQL Integration – **After Stemming**

**Medium Priority (v3):**
5. 🔲 Phrase Search
6. 🔲 Query Highlighting
7. 🔲 Advanced Analyzers (N-Grams, Synonyms)

**Low Priority (v4+):**
8. 🔲 Learned Fusion
9. 🔲 Multi-Stage Reranking
10. 🔲 Query Expansion

## Testing Strategy

**Unit Tests:**
- Stemmer: token → stem mappings for DE/EN
- AQL Parser: BM25(doc) function parsing
- Query Engine: Score context propagation

**Integration Tests:**
- End-to-end AQL queries with FULLTEXT + SORT BM25
- Stemming: Query "running" matches docs with "run"
- Phrase search: Quoted vs. unquoted queries

**Performance Tests:**
- BM25 latency: 100k docs, 5-token queries (target: <50ms)
- Fusion overhead: Text+Vector vs. separate (target: <2× slowdown)
- Stemming impact: Index size increase (expect: +10-20%)

## Documentation TODOs

- [x] **AQL Syntax Guide: FULLTEXT operator, BM25(doc) function** ✅ COMPLETE
  - Dokumentiert in `docs/aql_syntax.md` (Zeilen 172-195, 491-577)
  - FULLTEXT operator vollständig dokumentiert mit Beispielen
  - BM25(doc) Funktion für Score-Zugriff dokumentiert
  - Hybrid Search (FULLTEXT + AND) dokumentiert
  
- [x] **Index Configuration: Stemming options, language codes** ✅ COMPLETE
  - Dokumentiert in `docs/search/fulltext_api.md` (Zeilen 1-150)
  - Stemming: `stemming_enabled`, `language` (en/de/none)
  - Stopwords: `stopwords_enabled`, custom `stopwords` array
  - Umlaut-Normalisierung: `normalize_umlauts` für DE
  - Vollständige API-Beispiele mit Konfiguration

- [x] **Performance Tuning Guide** ✅ COMPLETE (07.11.2025)
  - Neu erstellt: `docs/search/performance_tuning.md`
  - BM25 Parameter Tuning (k1, b) mit Use-Case-Matrix
  - efSearch für Vector-Queries (20-200 mit Recall/Latency trade-offs)
  - k_rrf für Hybrid Search Fusion (20-100 Empfehlungen)
  - weight_text/weight_vector für Weighted Fusion
  - Index Rebuild Strategy & Maintenance
  - Performance Benchmarks und Monitoring
  - Production Checklist
  
- [x] **Migration Guide: v1 → v2** ✅ COMPLETE (07.11.2025)
  - Neu erstellt: `docs/search/migration_guide.md`
  - Zero-Downtime Migration Strategy (Dual Index)
  - Maintenance Window Strategy (In-Place)
  - Incremental Migration für große Datasets (>10M docs)
  - Rollback Procedures mit Timelines
  - Backward Compatibility Matrix
  - Testing Checklist (Pre/During/Post-Migration)
  - Migration Examples: Stemming, Umlaut-Norm, Vector-Dim-Change
  - Performance Impact & Monitoring
  - FAQ & Troubleshooting

## References

- Snowball Stemmer: https://snowballstem.org/
- Okapi BM25: Robertson & Zaragoza (2009)
- RRF: Cormack, Clarke, Büttcher. SIGIR 2009
- LambdaMART: Burges (2010)

## Implementation Status (November 2025)

### ✅ Completed Features

1. **BM25 Fulltext Search** - Production-ready
   - HTTP API: POST /search/fulltext mit Score-Ranking
   - Index API: POST /index/create mit config options
   - Query semantics: AND-logic, optional limit
   
2. **Stemming & Normalization** - Production-ready
   - Languages: EN (Porter subset), DE (suffix stemming)
   - Stopwords: Built-in lists + custom stopwords
   - Umlaut normalization: ä→a, ö→o, ü→u, ß→ss (optional)
   
3. **Phrase Search** - Production-ready (v1)
   - Quoted phrases: "exact match" queries
   - Case-insensitive substring matching
   - Works with normalize_umlauts
   
4. **AQL Integration** - Production-ready (v1.3)
   - FILTER FULLTEXT(field, query [, limit])
   - SORT BM25(doc) DESC/ASC
   - RETURN {doc, score: BM25(doc)}
   - Hybrid: FULLTEXT + AND predicates
   - OR combinations: FULLTEXT(...) OR ...

5. **Hybrid Search (Text + Vector)** - Production-ready
   - RRF fusion (Reciprocal Rank Fusion)
   - Weighted fusion (configurable text/vector balance)
   - HTTP API: POST /search/hybrid

### 🟡 Planned Enhancements

**Near-term (Q1 2026):**
- Highlighting: Mark matched terms in response
- ~~Performance tuning guide with benchmarks~~ ✅ IMPLEMENTED → siehe `docs/search/performance_tuning.md`
- ~~Migration guide for index rebuilds~~ ✅ IMPLEMENTED → siehe `docs/search/migration_guide.md`

**Long-term (Q2+ 2026):**
- Position-based phrase search (faster than substring)
- Advanced analyzers: n-grams, phonetic matching
- Query expansion with synonyms
- LambdaMART learning-to-rank

### Nächste sinnvolle Schritte

1. ~~Umlaut-/ß-Normalisierung~~ ✅ IMPLEMENTED
2. ~~Phrase Queries~~ ✅ IMPLEMENTED (v1 substring-based)
3. ~~AQL-Integration: FULLTEXT-Operator + BM25~~ ✅ IMPLEMENTED (v1.3)
4. Highlighting für matched terms (v2 planned)
5. ~~Performance Tuning Guide mit Benchmarks~~ ✅ IMPLEMENTED → `docs/search/performance_tuning.md`