# AQL Full-Text Search (FTS) Extension Contract v1.0 (Frozen)

**Status**: FROZEN — Authoritative specification for Phase 2 implementation  
**Created**: 2026-08-06  
**Target Release**: v2.0.0 (Q4 2026)  
**Dependencies**: None (parallel track; BasicTextIndex already exists)  
**Blocking Items**: None

---

## 1. Overview

This contract defines the AQL Full-Text Search (FTS) extension, enabling SEARCH syntax for text queries. The `BasicTextIndex` backend already exists; Phase 2 focuses on **SEARCH syntax parsing** and **FTS operator support** (phrase queries, field boosting, wildcards, boolean operators).

**Key Property**: Current CONTAINS() function works for basic text search; Phase 2 adds first-class SEARCH syntax + advanced operators.

---

## 2. SEARCH Syntax Specification

### 2.1 Basic SEARCH Query

```sql
-- Basic syntax
FOR doc IN collection 
  SEARCH doc.text IN "search_term"
  RETURN doc

-- With collection filtering
FOR doc IN collection
  FILTER doc.type = "article"
  SEARCH doc.text IN "query"
  RETURN doc

-- Multiple fields
FOR doc IN collection
  SEARCH doc.title IN "term" OR doc.body IN "term"
  RETURN doc
```

### 2.2 Advanced Operators

```sql
-- Phrase queries (exact sequence)
SEARCH doc.text IN "exact phrase"

-- Field boosting (increase relevance weight)
SEARCH (doc.title IN "term")^2 OR doc.body IN "term"

-- Wildcards (prefix/suffix/infix matching)
SEARCH doc.text IN "star*"      // startswith
SEARCH doc.text IN "*term"      // endswith
SEARCH doc.text IN "t*rm"       // contains (optional, may not implement)

-- Boolean operators
SEARCH doc.text IN "term1 AND term2"      // both must match
SEARCH doc.text IN "term1 OR term2"       // either can match
SEARCH doc.text IN "term1 NOT term2"      // must have term1, must not have term2

-- Negation
SEARCH !(doc.text IN "forbidden_term")

-- Grouping
SEARCH (doc.text IN "term1" OR doc.text IN "term2") AND doc.published = true
```

### 2.3 WITH Clause Options

```sql
-- Relevance scoring
FOR doc IN collection
  SEARCH doc.text IN "query" WITH { score: "bm25" }
  RETURN {doc, score: doc._score}

-- Stopword filtering
SEARCH doc.text IN "query" WITH { stopwords: true }

-- Case sensitivity
SEARCH doc.text IN "Query" WITH { case_sensitive: false }
```

---

## 3. AST Node Types

```cpp
// include/query/aql_fts_ast.h

class SearchNode : public AQLNode {
  public:
    enum class SearchOperator {
        IN,                // doc.field IN "term"
        AND,               // term1 AND term2
        OR,                // term1 OR term2
        NOT,               // NOT term
        PHRASE,            // exact sequence
        PREFIX,            // term*
        WILDCARD,          // t*rm
        BOOST,             // (query)^weight
    };

    virtual ~SearchNode() = default;
    
    SearchOperator getOperator() const;
    std::string getFieldName() const;     // for IN operator
    std::string getSearchTerm() const;    // literal string to search
    double getBoostWeight() const;        // for BOOST operator
};

// Specific node types
class SearchINNode : public SearchNode {
  public:
    SearchINNode(std::string field, std::string term);
    std::string getField() const;
    std::string getTerm() const;
};

class SearchBooleanNode : public SearchNode {
  public:
    SearchBooleanNode(SearchOperator op, SearchNode* left, SearchNode* right);
    SearchNode* getLeft() const;
    SearchNode* getRight() const;
};

class SearchPhrasNode : public SearchNode {
  public:
    SearchPhrasNode(std::string field, std::vector<std::string> words);
    std::vector<std::string> getPhraseWords() const;
};
```

---

## 4. TokenType Extensions

```cpp
// include/query/aql_token.h (extensions)

enum class TokenType {
    // ... existing tokens ...
    
    // FTS keywords
    SEARCH,          ///< SEARCH keyword
    IN,              ///< IN keyword (reused)
    AND,             ///< AND boolean operator
    OR,              ///< OR boolean operator
    NOT,             ///< NOT boolean operator
    WITH,            ///< WITH clause (options)
    STOPWORDS,       ///< STOPWORDS option
    SCORE,           ///< SCORE field option
    BM25,            ///< BM25 scoring algorithm
    CASE_SENSITIVE,  ///< case sensitivity option
    
    // FTS operators
    BOOST,           ///< ^ operator for boosting (e.g., term^2)
    WILDCARD,        ///< * for wildcards
};
```

---

## 5. Parser Method Signatures

```cpp
// include/query/aql_fts_parser.h

class FTSParser {
  public:
    /// Parse SEARCH clause from FOR...SEARCH syntax
    /// @return SearchNode* or nullptr
    SearchNode* parseSearch();
    
    /// Parse SEARCH term (may be complex: "term1 AND term2" etc.)
    /// @return SearchNode tree
    SearchNode* parseSearchExpression();
    
    /// Parse basic search term with optional wildcards
    /// @return SearchINNode
    SearchINNode* parseSearchTerm(const std::string& field);
    
    /// Parse phrase query ("term1 term2 ...")
    /// @return SearchPhraseNode
    SearchPhraseNode* parsePhrase(const std::string& field);
    
    /// Parse WITH clause options
    /// @return map of {key: value} options
    std::map<std::string, AQLValue> parseSearchOptions();
};
```

---

## 6. BasicTextIndex Extension Contract

`BasicTextIndex` (existing class) must be extended to support advanced operators:

```cpp
// include/index/fulltext_index.h (extensions)

class BasicTextIndex {
  public:
    // Existing method
    std::vector<DocumentId> search(const std::string& query);
    
    // NEW: Phrase query support
    /// Search for exact phrase (all words in sequence)
    /// @param words Vector of words in sequence
    /// @return document IDs containing phrase
    std::vector<DocumentId> searchPhrase(
        const std::vector<std::string>& words);
    
    // NEW: Field boosting
    /// Search with per-field weight multipliers
    /// @param query Search query
    /// @param fieldWeights {field_name: boost_factor}
    /// @return documents ranked by relevance
    std::vector<DocumentId> searchWithBoosting(
        const std::string& query,
        const std::map<std::string, double>& fieldWeights);
    
    // NEW: Wildcard support
    /// Search with wildcard patterns (term*, *term, t*rm)
    /// @param pattern Pattern with * wildcards
    /// @return matching documents
    std::vector<DocumentId> searchWildcard(
        const std::string& pattern);
    
    // NEW: Relevance scoring (BM25)
    /// Score results by relevance (BM25 algorithm)
    /// @param query Search query
    /// @return vector of {document, score} sorted by score DESC
    std::vector<std::pair<DocumentId, double>> searchWithScoring(
        const std::string& query);
};
```

---

## 7. Query Executor Integration

```cpp
// include/query/fts_query_executor.h (NEW)

class FTSQueryExecutor {
  public:
    /// Execute SEARCH query
    /// @param searchNode AST from parser
    /// @param collection Documents to search
    /// @param options WITH clause options
    /// @return result stream of matching documents
    ResultStream* executeSearch(
        const SearchNode* searchNode,
        const Collection* collection,
        const std::map<std::string, AQLValue>& options);
};
```

---

## 8. Index Selection Rules

```
If query contains SEARCH doc.text IN "term":
   ├─ Check if FULLTEXT index exists on "text"
   ├─ If YES → use index (text scan instead of full table scan)
   └─ If NO → fallback to sequential scan (still works)

If SEARCH contains phrase or wildcards:
   ├─ Must use FULLTEXT index (sequential scan incompatible)
   └─ Error if no FULLTEXT index + complex query
```

---

## 9. Error Handling

All FTS errors are categorized:

### 4xx Validation Errors

| Code | Name | Meaning | Recovery |
|------|------|---------|----------|
| 400 | INVALID_SEARCH_SYNTAX | Malformed SEARCH clause | Fix query syntax |
| 400 | INVALID_QUERY_STRING | Query string not parseable | Escape special chars |
| 400 | EMPTY_SEARCH_QUERY | No search terms provided | Provide at least one term |
| 400 | UNSUPPORTED_OPERATOR | Operator not supported (e.g., FUZZY) | Use supported operators |

### 5xx Runtime Errors

| Code | Name | Meaning | Recovery |
|------|------|---------|----------|
| 500 | INDEX_NOT_AVAILABLE | FULLTEXT index unavailable | Create index or use sequential |
| 500 | SEARCH_TIMEOUT | Query execution exceeded timeout | Simplify query or add indexes |

---

## 10. Performance Targets (Phase 5 Gates)

FTS queries MUST meet these targets:

| Operation | Target | Basis |
|-----------|--------|-------|
| SEARCH basic term (10K docs) | ≤ 10ms | index lookup + result streaming |
| SEARCH phrase (10K docs) | ≤ 25ms | phrase matching on index |
| SEARCH with wildcard (10K docs) | ≤ 50ms | pattern matching |
| SEARCH with boosting (10K docs) | ≤ 30ms | relevance scoring |
| SEARCH with BM25 scoring (100K docs) | ≤ 100ms | score computation |
| SEARCH boolean query (100K docs) | ≤ 75ms | operator evaluation |

---

## 11. Test Requirements (Phase 4)

Phase 2 implementation must include:

### Parser Tests (20+)
- Valid SEARCH syntax
- Field references (doc.field)
- Term expressions (strings, wildcards)
- Boolean operators (AND, OR, NOT)
- Boost syntax (^weight)
- WITH clause options
- Error cases (missing keywords, malformed)

### Index Tests (25+)
- FULLTEXT index creation
- Phrase query execution
- Wildcard pattern matching
- Field boosting
- BM25 relevance scoring
- Stopword filtering

### Executor Tests (20+)
- Basic SEARCH execution
- Multiple fields (OR'ed searches)
- Boolean operator combinations
- Score result ranking
- Option propagation (case sensitivity, stopwords)

### Integration Tests (40+)
- SEARCH + FILTER combined
- SEARCH + SORT by score
- SEARCH with pagination (LIMIT/OFFSET)
- SEARCH on mixed data types
- Performance regression tests vs. CONTAINS() function

---

## 12. Stopword Handling

FTS must support common stopwords (language-dependent):

```cpp
// src/index/fts_stopwords.h (NEW)

class FTSStopwordFilter {
  public:
    /// Check if word is a stopword
    static bool isStopword(const std::string& word, const std::string& lang = "en");
    
    /// Remove stopwords from term list
    static std::vector<std::string> filterStopwords(
        const std::vector<std::string>& words);
};

// Common English stopwords
const std::set<std::string> kEnglishStopwords = {
    "the", "a", "an", "and", "or", "but", "in", "on", "at", "to", "for",
    "of", "with", "by", "from", "up", "about", "out", "if", "is", "are",
    "was", "were", "be", "been", "being", "have", "has", "had", "do",
    "does", "did", "will", "would", "could", "should", "may", "might",
    "can", "must", "shall", ...
};
```

---

## 13. BM25 Relevance Scoring

FTS must support BM25 ranking algorithm:

```cpp
// src/index/bm25_scorer.h (NEW)

class BM25Scorer {
  public:
    /// Compute BM25 score for document
    /// @param query Search query terms
    /// @param doc Document text
    /// @param avgDocLength Average document length in corpus
    /// @return Score (higher = more relevant)
    double computeScore(
        const std::vector<std::string>& queryTerms,
        const std::string& doc,
        double avgDocLength);
};

// BM25 parameters (tunable)
const double k1 = 1.5;      // term frequency saturation
const double b = 0.75;      // field length normalization
const double k2 = 100;      // query term frequency
```

---

## 14. Integration Points

### 14.1 Parser Integration
- `AQLParser::parseStatement()` — recognizes SEARCH keyword
- Returns `SearchNode*` (part of query AST)

### 14.2 Executor Integration
- `QueryExecutor::execute(SearchNode*)` → delegates to `FTSQueryExecutor`
- `FTSQueryExecutor::executeSearch()` → FTS logic

### 14.3 Index Integration
- `BasicTextIndex` — extended with phrase/wildcard/scoring methods
- Index manager recognizes FULLTEXT index type
- Optimizer uses FULLTEXT index hints

### 14.4 Optimizer Integration
- Query optimizer recognizes SEARCH clauses
- Suggests FULLTEXT index if available
- Estimates selectivity for SEARCH predicates

---

## 15. Dependency Tree

```
FTS Phase 1-6
├── Phase 1: Design / API Contract (THIS DOCUMENT) ✓
├── Phase 2: Core Implementation (Weeks 3-12, parallel)
│   ├── Week 3-4: Parser extension (SEARCH syntax + WITH clause)
│   │   └── Files: aql_fts_parser.cpp, SEARCH token + AST nodes
│   ├── Week 5-6: BasicTextIndex extension (phrase, wildcard, boosting)
│   │   └── Files: fulltext_query_parser.cpp, BasicTextIndex methods
│   ├── Week 7-9: Query optimization + BM25 scoring
│   │   └── Files: bm25_scorer.cpp, fts_query_executor.cpp
│   └── Week 10-12: Testing + documentation
│       └── Benchmarks, user guide (docs/de/aql/aql_fts_guide.md)
├── Phase 3: Error Handling (Week 17-18)
├── Phase 4: Tests (1000+ total, incremental)
├── Phase 5: Performance Hardening (<= 100ms gates)
└── Phase 6: Documentation & Acceptance
```

**No blocking dependencies** — can run in parallel with Mutations, DDL, Geospatial.

---

## 16. Acceptance Criteria

Phase 2 implementation is COMPLETE when:

- [ ] SEARCH syntax parses correctly (all operators)
- [ ] SearchNode AST structures build correctly
- [ ] BasicTextIndex supports phrases, wildcards, boosting
- [ ] FULLTEXT index selection works in query planner
- [ ] BM25 scoring implemented and tested
- [ ] 20+ parser tests PASS
- [ ] 25+ index tests PASS
- [ ] 20+ executor tests PASS
- [ ] 40+ integration tests PASS
- [ ] Performance: phrase query on 100K docs in <= 100ms
- [ ] Zero new CRITICAL scanner findings
- [ ] Doxygen 100% on fts_*.cpp/h files

---

## 17. Approval & Sign-Off

This contract MUST be approved by:

- [ ] **Parser Lead**: SEARCH syntax + TokenType additions
- [ ] **FTS Engine Lead**: BasicTextIndex extension feasibility
- [ ] **Query Optimizer Lead**: Spatial index hints + selectivity estimation
- [ ] **Performance Lead**: BM25 scoring + benchmark targets
- [ ] **PM**: FTS track can proceed in parallel

**Approval Status**: PENDING SIGN-OFF

**Approved By**: _______________  
**Date**: _______________  
**Changes Since Frozen**: None (this is the frozen version)

---

## 18. Amendments Log

| Date | Amendment | Approved By |
|------|-----------|-------------|
| 2026-08-06 | Initial contract created | PENDING |

(Amendments require explicit written approval and update to this section)

