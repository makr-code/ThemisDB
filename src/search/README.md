> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Search Module

<!-- Status: current | validated: 2026-04-06 | commit: a14cdb2 -->
<!-- Primary: src/search/ | Secondary: docs/de/src/search/ -->

Full-text and semantic search capabilities for ThemisDB.

## Module Purpose

Implements full-text and hybrid search for ThemisDB, providing inverted index management, BM25 relevance scoring, and hybrid vector+keyword search capabilities.

## Subsystem Scope

**In scope:** Inverted index construction and management, BM25/TF-IDF scoring, hybrid search (vector similarity + keyword), search result ranking and pagination.

**Out of scope:** Vector index construction (handled by index module), language-specific stemming/stopwords (handled by utils module).

## Relevant Interfaces

- `hybrid_search.cpp` — RRF-based fusion of BM25 and vector search (main entry point)
- `query_expander.cpp` — synonym expansion, spelling correction, query relaxation
- `fuzzy_matcher.cpp` — Levenshtein, Soundex, Metaphone, N-gram similarity
- `llm_query_rewriter.cpp` — LLM-based query rewriting for improved recall
- `llm_reranker.cpp` — LLM-based re-ranking of top-N results
- `learning_to_rank.cpp` — online pairwise gradient-descent ranking model

## Current Delivery Status

**Maturity:** 🟢 Production-Ready — All 16 components production-ready as of v2.2.0.

## Components

- **Hybrid Search**: Combines BM25 (lexical) and vector (semantic) search with Reciprocal Rank Fusion (RRF)
- **Full-Text Search**: BM25 ranking algorithm for keyword matching
- **Fuzzy Matching**: Phonetic search and edit distance matching
- **Search Indexing**: Tokenization, stemming, and inverted index management
- **Query Parsing**: Natural language query parsing and expansion
- **Result Ranking**: Configurable scoring and ranking algorithms
- **LLM Query Rewriting**: LLM-based alternative query generation for improved recall (`LlmQueryRewriter`)
- **LLM Re-ranking**: Configurable re-ranking with LLM feedback loop (`LlmReranker`)

## Features

### Full-Text Search
- **BM25 Ranking**: Industry-standard probabilistic ranking function
- **Term frequency analysis**: Boost relevance based on term occurrence
- **Document frequency**: Penalize common terms
- **Field length normalization**: Account for document length
- **Configurable parameters**: Tune k1 and b for optimal results

### Semantic Search
- **Vector embeddings**: Dense vector representations for semantic similarity
- **HNSW indexing**: Fast approximate nearest neighbor search
- **Multiple distance metrics**: Cosine, dot product, L2 distance
- **Hybrid search**: Combine lexical and semantic matching

### Search Enhancement
- **Fuzzy matching**: Handle typos and misspellings
- **Phonetic search**: Soundex, Metaphone algorithms
- **Stemming**: Reduce words to root forms
- **Stop word filtering**: Remove common words
- **Synonym expansion**: Query expansion with synonyms
- **Phrase search**: Exact phrase matching

### Hybrid Search (v1.2.0+)
- **Reciprocal Rank Fusion (RRF)**: Optimal result merging
- **Configurable weights**: Balance between BM25 and vector search
- **Score normalization**: Unified scoring across search types
- **Multiple search modes**: Text-only, vector-only, or hybrid

## Architecture

```
SearchModule
├─→ HybridSearch (RRF-based result fusion)
│   ├─→ SecondaryIndexManager (BM25 full-text search)
│   └─→ VectorIndexManager (Semantic vector search)
├─→ QueryParser (Natural language processing)
├─→ Tokenizer (Text analysis and stemming)
├─→ FuzzyMatcher (Typo tolerance)
└─→ ResultRanker (Score aggregation and ranking)
```

## Use Cases

### RAG (Retrieval-Augmented Generation)
- Retrieve relevant context for LLM prompts
- Combine keyword and semantic search for best results
- High recall@10 (85%+) with hybrid search
- Support for reranking with LLM feedback

### Document Search
- Full-text search across document collections
- Semantic similarity for conceptual matching
- Faceted search with filters
- Highlight matching terms

### Question Answering
- Find answers to natural language questions
- Semantic matching for paraphrased questions
- Context retrieval for answer generation
- Confidence scoring

### E-commerce Search
- Product search with text and attributes
- Fuzzy matching for misspellings
- Faceted navigation (brand, price, category)
- Personalized ranking

## Performance Characteristics

### Full-Text Search (BM25)
- **Query latency**: 1-10ms for typical queries
- **Index size**: ~20-30% of document size
- **Throughput**: 1K-10K queries/second
- **Scalability**: Handles millions of documents

### Vector Search (HNSW)
- **Query latency**: 1-10ms for k=10 on 1M vectors
- **Index size**: ~1.5x vector data size
- **Throughput**: 1K-5K queries/second
- **Scalability**: Handles 10M+ vectors

### Hybrid Search (RRF)
- **Query latency**: 5-20ms (combines both searches)
- **Recall@10**: 85%+ with optimal tuning
- **Throughput**: 500-2K queries/second
- **Memory overhead**: Minimal (result merging only)

## Configuration

### Hybrid Search Setup
```cpp
#include "search/hybrid_search.h"

using namespace themis;

// Configure hybrid search
HybridSearch::Config config;
config.bm25_weight = 0.5;          // Balance BM25 vs vector
config.vector_weight = 0.5;
config.k = 10;                     // Final result count
config.k_bm25 = 50;                // BM25 candidates
config.k_vector = 50;              // Vector candidates
config.use_rrf = true;             // Use Reciprocal Rank Fusion
config.rrf_k = 60.0;               // RRF constant
config.normalize_scores = true;
config.max_k = 10000;              // Hard upper bound for k
config.max_candidates = 10000;     // Hard upper bound for k_bm25 / k_vector
config.default_table  = "documents";
config.default_column = "content";
config.vector_metric = VectorIndexManager::Metric::COSINE; // or DOT / L2

// Constructor throws std::invalid_argument for invalid config
HybridSearch hybrid_search(
    fulltext_index_manager,
    vector_index_manager,
    config
);

// Perform hybrid search (never throws)
std::string text_query = "machine learning algorithms";
std::vector<float> query_vector = embeddings_model.encode(text_query);

HybridSearch::SearchStats stats;
auto results = hybrid_search.search(
    text_query,
    query_vector.data(),
    query_vector.size(),
    &stats  // optional: receive per-source diagnostics
);

if (stats.partial_result) {
    // One backend failed; results are degraded but not empty
}

// Process results
for (const auto& result : results) {
    std::cout << "Doc: " << result.document_id
              << " Score: " << result.hybrid_score
              << " (BM25: " << result.bm25_score
              << ", Vector: " << result.vector_score << ")" << std::endl;
}
```

### Full-Text Search
```cpp
// Create full-text index
SecondaryIndexManager index_mgr(db);
index_mgr.createIndex(
    "documents",
    "content",
    SecondaryIndexType::FULLTEXT
);

// Search documents
auto results = index_mgr.search(
    "documents",
    "content",
    "machine learning",
    10  // limit
);
```

### Vector Search
```cpp
// Create vector index
VectorIndexManager vector_mgr(db);
vector_mgr.createIndex(
    "documents",
    "embedding",
    768,  // dimension
    VectorIndexManager::Metric::COSINE
);

// Search by vector
std::vector<float> query_vector = {...};
auto results = vector_mgr.search(
    "documents",
    "embedding",
    query_vector,
    10  // k
);
```

## Integration Points

- **Index Module**: Secondary index for full-text, vector index for semantic search
- **Storage Layer**: Document retrieval and metadata
- **Utils Module**: Stemmer, tokenizer, stopwords
- **LLM Module**: Embedding generation for semantic search
- **Query Module**: Integration with AQL for search queries

## Thread Safety

A single `HybridSearch` instance is **not thread-safe**. `search()` and `setConfig()`
must not be called concurrently on the same instance. Callers that share an instance
across threads must provide their own synchronization (e.g. a mutex). The recommended
pattern is one `HybridSearch` instance per thread; the class is lightweight (Config +
two non-owning index pointers) and cheap to construct.

The underlying index managers (`SecondaryIndexManager`, `VectorIndexManager`) may have
their own thread-safety guarantees — consult their documentation.

## Dependencies

- **Secondary Index Manager**: BM25 full-text search
- **Vector Index Manager**: HNSW vector search
- **HNSWLIB**: Approximate nearest neighbor library
- **Utils**: Stemmer, stopwords, text processing

## Documentation

For detailed implementation documentation, see:
- [Architecture Guide](ARCHITECTURE.md) - Component diagram, data flows, threading, security
- [Roadmap](ROADMAP.md) - Implementation status, known limitations
- [Future Enhancements](FUTURE_ENHANCEMENTS.md) - Planned features and IEEE references
- [Secondary Docs (DE)](../../docs/de/src/search/README.md) - German overview and reality-check

## Version History

- **v1.2.0**: Hybrid search with RRF for RAG optimization
- **v1.3.0**: Real BM25 and vector index integration
- **v1.4.0**: Configurable vector metric (COSINE/DOT/L2), strict config validation,
  resource limits (`max_k`, `max_candidates`), score normalization edge-case fixes,
  linear-combination pre-normalization, `SearchStats` for partial-result detection,
  thread-safety and exception-safety documentation, expanded test coverage
- **v1.5.0**: `QueryExpander` (synonym expansion, spelling correction, query relaxation),
  `FuzzyMatcher` (Levenshtein, Soundex, Metaphone, N-gram similarity),
  `FacetedSearch` (value-count facets, range buckets, drill-down filter intersection),
  `SearchAnalytics` (thread-safe query logging, p95/p99 latency, zero-result detection),
  `AutocompleteEngine` (prefix-index suggestions, popular-query suggestions),
  `LearningToRank` (linear re-ranker, click-through training, A/B variant selector),
  `MultiModalSearch` (text + embedding RRF fusion, searchTextAndImage convenience)
- **v1.6.0**: `LlmQueryRewriter` — LLM-based query rewriting for improved recall (injected backend,
  multi-strategy prompt, numbered-line parsing, deduplication, fallback)
- **v1.7.0**: Ranked spelling correction suggestions — `SpellingCorrection` struct,
  `QueryExpander::suggestSpellingCorrections()` (ranked multi-candidate word corrections with
  confidence scores), `QueryExpander::suggestQueryCorrections()` (ranked full-query correction
  suggestions with per-token substitution and all-corrected variants)
- **v1.8.0**: `LlmReranker` — Configurable re-ranking with LLM feedback loop (batched prompting,
  per-document 0–10 score parsing, configurable score blending, `toClickEvents()` bridge to
  `LearningToRank` for closed-loop LTR training from LLM relevance judgments)

## Examples

### Basic Hybrid Search
```cpp
// Initialize
HybridSearch hybrid(fulltext_idx, vector_idx, config);

// Search
auto results = hybrid.search(
    "neural networks",
    embedding_vector,
    embedding_dim
);
```

### BM25-Only Search
```cpp
HybridSearch::Config config;
config.bm25_weight = 1.0;
config.vector_weight = 0.0;

HybridSearch bm25_only(fulltext_idx, vector_idx, config);
```

### Vector-Only Search
```cpp
HybridSearch::Config config;
config.bm25_weight = 0.0;
config.vector_weight = 1.0;

HybridSearch vector_only(fulltext_idx, vector_idx, config);
```

### With Filters
```cpp
// Post-filter results in application code after search()
auto results = hybrid.search("restaurants", query_vector, vector_dim);
results.erase(std::remove_if(results.begin(), results.end(),
    [](const HybridSearch::Result& r) {
        return r.hybrid_score < 0.5; // keep only high-confidence results
    }), results.end());
```

## Best Practices

### Tuning Hybrid Search
1. **Start with balanced weights**: 0.5/0.5 for BM25/vector
2. **Use RRF**: Better than linear combination in most cases
3. **Adjust k parameter**: Higher k_bm25 and k_vector improve recall
4. **Normalize scores**: Enable for fair comparison

### Performance Optimization
1. **Cache embeddings**: Avoid recomputing query vectors
2. **Batch searches**: Process multiple queries together
3. **Warm up indexes**: Load into memory before serving
4. **Use appropriate k**: Don't retrieve more results than needed

### Quality Improvement
1. **Fine-tune embeddings**: Domain-specific embeddings work better
2. **Optimize tokenization**: Remove noise, handle special characters
3. **Use synonyms**: Expand queries for better recall
4. **Rerank results**: Use LLM or custom logic for final ranking

## See Also

- [Header Documentation](../../include/search/README.md) - Public API definitions
- [Index Module](../index/README.md) - Underlying index implementations
- [LLM Module](../llm/README.md) - Embedding generation
- [Utils Module](../utils/README.md) - Text processing utilities
- [Architecture Guide](../../ARCHITECTURE.md) - Search architecture

## Scientific References

1. Robertson, S., & Zaragoza, H. (2009). **The Probabilistic Relevance Framework: BM25 and Beyond**. *Foundations and Trends in Information Retrieval*, 3(4), 333–389. https://doi.org/10.1561/1500000019

2. Vaswani, A., Shazeer, N., Parmar, N., Uszkoreit, J., Jones, L., Gomez, A. N., … Polosukhin, I. (2017). **Attention Is All You Need**. *Advances in Neural Information Processing Systems (NeurIPS)*, 30, 5998–6008. https://arxiv.org/abs/1706.03762

3. Karpukhin, V., Oğuz, B., Min, S., Lewis, P., Wu, L., Edunov, S., … Yih, W.-t. (2020). **Dense Passage Retrieval for Open-Domain Question Answering**. *Proceedings of the 2020 Conference on Empirical Methods in Natural Language Processing (EMNLP)*, 6769–6781. https://doi.org/10.18653/v1/2020.emnlp-main.550

4. Manning, C. D., Raghavan, P., & Schütze, H. (2008). **Introduction to Information Retrieval**. Cambridge University Press. https://doi.org/10.1017/CBO9780511809071

5. Luan, Y., Eisenstein, J., Toutanova, K., & Collins, M. (2021). **Sparse, Dense, and Attentional Representations for Text Retrieval**. *Transactions of the Association for Computational Linguistics*, 9, 329–345. https://doi.org/10.1162/tacl_a_00369

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.

## Usage

The implementation files in this module are compiled into the ThemisDB library.
See [`../../include/search/README.md`](../../include/search/README.md) for the public API.
