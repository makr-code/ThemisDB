// ==============================================================================
// DESIGN_FTS_EXECUTOR_2026-09-10.md
// ==============================================================================
// CRITICAL MODULE: QUERY
// Gap 3.1: FTS Executor Backend Design
//
// Status: Design phase (Sept 2-10, 2026)
// Target Completion: Sept 10, 2026
// Implementation Start: Sept 15, 2026 (blocked on design approval)
// Implementation Duration: 30 days (Sept 15 - Oct 30)
// Effort Estimate: 30 days (team of 3)
//
// Acceptance Criteria (Design Phase):
// 1. Algorithm documented: BM25 scoring + index lookup strategy
// 2. Data structures specified: FTS index on-disk layout, in-memory cache strategy
// 3. API surface defined: FtsExecutor interface, error cases, thread-safety model
// 4. Performance targets: ≤100ms query on 100K documents
// 5. Test strategy drafted: 25 total tests (unit/integration/performance/determinism)
// 6. Technical committee approval obtained
//
// ==============================================================================

# DESIGN: Full-Text Search (FTS) Executor Backend

## 1. Current State Analysis

### What's Implemented
- **Lexer/Tokenizer** (`src/query/fts_tokenizer.cpp`):
  - Token types defined: TERM, PHRASE, WILD_PREFIX, BOOL_AND, BOOL_OR, BOOL_NOT
  - Tokenizer produces sequence of Token objects with position/length metadata
  - Edge cases handled: UTF-8 normalization, case-insensitive folding, accent removal

- **Parser** (`src/query/fts_parser.cpp`):
  - parseSearchClause() consumes token sequence → AST
  - AST structure: `SearchNode` with type (TERM|PHRASE|AND|OR|NOT|RANGE)
  - All standard boolean combinations supported
  - Example: `"machine learning" AND (gpu OR cuda)` → correct AST

- **Stubs/Placeholders**:
  - FtsExecutor class exists but execute() not implemented (returns NOT_IMPLEMENTED)
  - FtsIndexLookup not implemented (returns empty results)
  - BM25Scorer stub exists but doesn't compute scores
  - IndexCache stub (no eviction/LRU logic)

### What's Missing
1. **Index Lookup Engine**: No mechanism to query FTS index for matching documents
2. **Scoring Function**: BM25 implementation incomplete (missing term frequency, IDF calculations)
3. **Index Data Structures**: On-disk index layout not defined; in-memory index cache not implemented
4. **Performance Optimization**: No query planning, no early termination, no caching
5. **Thread Safety**: FtsExecutor not annotated as thread-safe; index updates and queries not synchronized
6. **Error Handling**: No recovery from corrupted index, malformed queries, or resource exhaustion

### False Positives
- Multiple TODO comments in parser refer to "index integration" but parser is feature-complete
- Comments are documentation leaks (should be removed per memory #gap-audit)

## 2. Design: FTS Executor Architecture

### 2.1 Index Data Structures

#### On-Disk Index Format

```
FTS Index File Layout (binary format):
├─ Header (48 bytes)
│  ├─ Magic: "THEMISFTS" (8 bytes)
│  ├─ Version: u32 (4 bytes, current: 1)
│  ├─ Index size: u64 (8 bytes)
│  ├─ Doc count: u32 (4 bytes)
│  ├─ Posting list offset: u64 (8 bytes)
│  ├─ Metadata offset: u64 (8 bytes)
│  └─ Checksum (CRC32): u32 (4 bytes)
├─ Vocabulary Section (variable)
│  ├─ Term 1: "machine"
│  │  ├─ Term length: u16
│  │  ├─ Term bytes: UTF-8 encoded
│  │  ├─ Document frequency (df): u32
│  │  └─ Posting list offset: u64
│  ├─ Term 2: "learning"
│  │ ...
├─ Posting Lists Section (variable)
│  ├─ Term "machine":
│  │  ├─ Doc ID 1, tf: 2, positions: [0, 15]
│  │  ├─ Doc ID 5, tf: 1, positions: [8]
│  │  └─ Doc ID 42, tf: 3, positions: [0, 9, 22]
│  ├─ Term "learning":
│  │ ...
├─ Document Statistics Section (variable)
│  ├─ Doc 1: length: 512, avg_tf: 1.2, language: "en"
│  ├─ Doc 5: length: 256, avg_tf: 0.8, language: "en"
│  └─ Doc 42: length: 2048, avg_tf: 1.5, language: "en"
└─ Metadata Section (variable)
   ├─ Index creation timestamp: u64
   ├─ Last update: u64
   └─ Reserved for future use
```

#### In-Memory Index Cache (LRU + Bloom Filter)

```cpp
// Cache strategy for large indexes (>100K documents)
struct IndexCache {
  // Bloom filter for quick negative lookups
  std::unique_ptr<BloomFilter> term_existence_filter;  // 1% false-positive rate
  
  // LRU cache for hot posting lists (most frequently queried terms)
  LRUCache<std::string, PostingList> hot_posting_lists;
  
  // Memory budget: 100 MB default, configurable
  static constexpr size_t DEFAULT_CACHE_SIZE_MB = 100;
  
  // Lock for thread-safe access
  mutable std::shared_mutex cache_lock_;
};
```

### 2.2 BM25 Scoring Algorithm

BM25 formula (Okapi BM25):
```
score(D, Q) = Σ(i=1 to n) IDF(qi) * (tf(qi, D) * (k1 + 1)) / 
                                    (tf(qi, D) + k1 * (1 - b + b * |D| / avgDL))
```

Parameters:
- **k1** = 1.5 (controls term frequency saturation; higher = more TF weight)
- **b** = 0.75 (controls length normalization; 0 = no length norm, 1 = full norm)
- **IDF(qi)** = log((N - df(qi) + 0.5) / (df(qi) + 0.5))
  - N = total documents
  - df(qi) = document frequency of term i

Scoring steps:
1. Parse query AST (already done by parser)
2. For each TERM or PHRASE node:
   a. Lookup posting list from index
   b. For each matching document, compute BM25 score
   c. Cache score in result set
3. For AND/OR/NOT nodes:
   a. Recursively score child nodes
   b. Combine scores (AND = min, OR = max, NOT = invert)
4. Sort results by score descending
5. Return top-K (default: 100)

### 2.3 Query Execution Flow

```cpp
// Conceptual flow (pseudo-code)
ResultSet execute(const SearchNode& ast, const ExecutionContext& ctx) {
  ResultSet results;
  
  // 1. Query planning: Estimate selectivity, determine optimal traversal
  QueryPlan plan = planQuery(ast);
  
  // 2. Traverse AST bottom-up (postorder)
  results = traverseAST(ast);
  
  // 3. Score matching documents
  for (auto& doc : results) {
    doc.score = computeBM25Score(ast, doc.id);
  }
  
  // 4. Sort by score
  std::sort(results.begin(), results.end(), 
            [](const Result& a, const Result& b) {
              return a.score > b.score;
            });
  
  // 5. Early termination if K results found with acceptable score
  if (results.size() >= ctx.limit && 
      results[K].score > ctx.score_threshold) {
    results.erase(results.begin() + K, results.end());
  }
  
  return results;
}
```

### 2.4 Thread Safety Model

```cpp
class FtsExecutor {
 public:
  // Query execution is read-only → concurrent reads allowed
  // Must use std::shared_lock for concurrent query execution
  // Index updates require std::unique_lock
  
 private:
  mutable std::shared_mutex index_lock_;  // Protects: vocab, posting_lists, doc_stats
  IndexCache cache_;  // Has internal locking
  
  // THREAD SAFETY CONTRACT:
  // - execute() acquires shared_lock (allows N concurrent readers)
  // - updateIndex() acquires unique_lock (exclusive write)
  // - No deadlocks by lock ordering: always acquire index_lock_ before cache_lock_
};
```

### 2.5 Error Handling

```cpp
enum class FtsError {
  OK = 0,
  INDEX_NOT_FOUND = 7200,        // Index file doesn't exist
  INVALID_INDEX_FORMAT = 7201,   // Checksum mismatch or corrupted magic
  INDEX_LOCKED = 7202,           // Another process holding index lock
  PARSING_ERROR = 7203,          // Malformed query
  EXECUTION_TIMEOUT = 7204,      // Query exceeded time limit
  OUT_OF_MEMORY = 7205,          // Cache or index too large
  TERM_NOT_FOUND = 7206,         // Term not in index (not an error, empty results)
  INTERNAL_ERROR = 7299,         // Unexpected state
};

// Error recovery strategy:
// - If index corrupted: Rebuild from raw documents
// - If cache OOM: Evict LRU entries + retry
// - If query timeout: Return partial results with timeout flag
// - If lock contention: Retry with exponential backoff (up to 3 retries)
```

## 3. API Surface Definition

### 3.1 FtsExecutor Public Interface

```cpp
namespace themis::query::fts {

class FtsExecutor {
 public:
  // Construction + initialization
  explicit FtsExecutor(const FtsIndexPath& index_path);
  ~FtsExecutor();
  
  // Executor contract: must be thread-safe for reads (shared_lock allowed)
  // THREAD SAFETY: Multiple readers allowed; writers exclusive
  
  // Query execution API
  // @param query: search query (already parsed to AST by FtsParser)
  // @param limit: max results to return (default: 100)
  // @param timeout_ms: query time budget (default: 1000ms)
  // @return: sorted results by BM25 score, or error
  Result<std::vector<SearchResult>> execute(
      const SearchNode& query,
      const ExecutionOptions& options);
  
  // Batch query execution (optimization for multiple queries)
  Result<std::vector<std::vector<SearchResult>>> executeBatch(
      const std::vector<SearchNode>& queries,
      const ExecutionOptions& options);
  
  // Index update API (exclusive lock required)
  Result<void> updateIndex(const IndexUpdateBatch& updates);
  
  // Diagnostic API
  IndexStatistics getStatistics() const;  // read-only
  bool isIndexHealthy() const;
  
 private:
  std::shared_mutex index_lock_;
  IndexCache cache_;
  std::unique_ptr<FtsIndex> index_;
  ExecutionMetrics metrics_;  // Latency, hit rate, etc.
};

// Query result structure
struct SearchResult {
  DocumentId doc_id;
  float score;           // BM25 score [0, ∞)
  std::vector<uint32_t> term_positions;  // For highlighting
  std::string snippet;   // First 200 chars with query term context
};

// Execution options
struct ExecutionOptions {
  size_t limit = 100;
  std::chrono::milliseconds timeout{1000};
  bool include_snippets = true;
  bool parallel_merge = true;  // Use N threads for large result sets
};

}  // namespace themis::query::fts
```

### 3.2 Error Handling Patterns

```cpp
// Example usage:
auto executor = FtsExecutor(index_path);
auto result = executor.execute(search_ast, {.limit = 50, .timeout_ms = 500});

if (!result.ok()) {
  switch (result.error().code()) {
    case FtsError::INDEX_NOT_FOUND:
      LOG(ERROR) << "FTS index missing; rebuild required";
      // Fallback: rebuild or return empty results
      break;
    case FtsError::EXECUTION_TIMEOUT:
      LOG(WARNING) << "Query timeout; returning partial results";
      // Handle partial results gracefully
      break;
    default:
      return result;
  }
}

auto& search_results = result.value();
for (const auto& res : search_results) {
  printf("Doc %u: score %.2f\n", res.doc_id, res.score);
}
```

## 4. Performance Targets & Optimization Strategy

### 4.1 Performance Gates

| Metric | Target | Tolerance |
|--------|--------|-----------|
| Single-term query (100K docs) | ≤ 50ms | ± 10ms |
| Phrase query (100K docs) | ≤ 100ms | ± 20ms |
| Boolean AND (100K docs, 3 terms) | ≤ 75ms | ± 15ms |
| Boolean OR (100K docs, 3 terms) | ≤ 150ms | ± 30ms |
| Top-1000 results aggregation | ≤ 200ms | ± 40ms |

### 4.2 Optimization Techniques

1. **Posting List Compression**: Use VByte encoding for Doc IDs + TF values
2. **Bloom Filter**: Pre-filter non-existent terms before posting list lookup
3. **LRU Cache**: Cache hot posting lists in RAM (default 100 MB)
4. **Early Termination**: Stop scanning if score can't improve top-K
5. **Parallel Execution**: Use thread pool for large OR queries (merge partial results)
6. **Query Planning**: Estimate selectivity → traverse smallest posting lists first (AND optimization)

### 4.3 Determinism Guarantee

- Sorting stability: Secondary sort by document ID to ensure reproducible top-K
- Score precision: Use float64 internally, round to float32 for output consistency
- No random sampling or Monte Carlo approximations in scoring

## 5. Test Strategy (25 Total Tests)

### 5.1 Unit Tests (6)
- [ ] BM25 score computation with known IDF + TF values
- [ ] Posting list compression/decompression round-trip
- [ ] Bloom filter false-positive rate < 1%
- [ ] Term normalization (case folding, accent removal)
- [ ] Query plan selectivity estimation
- [ ] Score sorting stability (by document ID)

### 5.2 Integration Tests (10)
- [ ] Single-term query correctness (verify result set)
- [ ] Phrase query with position matching
- [ ] Boolean AND optimization (shortest posting list first)
- [ ] Boolean OR result set union correctness
- [ ] Boolean NOT result set complement
- [ ] Complex query: AND(OR(t1, t2), NOT(t3))
- [ ] Cache coherence: query before/after cache eviction → same results
- [ ] Large result set (>1000 docs) pagination
- [ ] Query timeout with partial results
- [ ] Index corruption recovery

### 5.3 Performance Tests (5)
- [ ] Single-term latency benchmark (target: ≤50ms)
- [ ] Phrase query latency (target: ≤100ms)
- [ ] Boolean AND optimization gains (expect 2-3x speedup vs naive)
- [ ] Cache hit rate under repeated queries
- [ ] Parallel execution speedup on large OR queries

### 5.4 Determinism Tests (4)
- [ ] Same query 100 times → identical result order and scores
- [ ] Query with contention (concurrent updates + reads)
- [ ] Score precision consistency (no rounding errors)
- [ ] Result set determinism under thread pool scheduling variation

## 6. Implementation Phases (Sept 15 - Oct 30)

### Phase 1: Index Loading + Basic Lookup (Sept 15-22, 5 days)
- [ ] Implement FtsIndex file I/O (load header, vocabulary, posting lists)
- [ ] Implement FtsIndexLookup: keyword → posting list
- [ ] Unit tests: file I/O round-trip, posting list parsing
- [ ] Benchmark: load time for 100K-document index

### Phase 2: BM25 Scoring + Merge (Sept 22-28, 6 days)
- [ ] Implement BM25 score computation
- [ ] Implement result set merge for AND/OR/NOT
- [ ] Integration tests: single-term, phrase, boolean queries
- [ ] Performance gate validation

### Phase 3: Optimization + Caching (Sept 28 - Oct 8, 10 days)
- [ ] Implement LRU cache for hot posting lists
- [ ] Implement Bloom filter for term existence checks
- [ ] Implement query planning (selectivity estimation)
- [ ] Early termination for top-K queries
- [ ] Performance benchmarks; validate gates

### Phase 4: Thread Safety + Error Handling (Oct 8-18, 10 days)
- [ ] Implement shared_mutex for concurrent read access
- [ ] Implement index update locking (exclusive write)
- [ ] Error handling + recovery paths
- [ ] Integration tests under contention
- [ ] Determinism tests

### Phase 5: Advanced Features (Oct 18-28, 10 days)
- [ ] Parallel query execution for large OR queries
- [ ] Partial result handling + timeout recovery
- [ ] Query timeout with exponential backoff
- [ ] Performance tests under concurrent load
- [ ] Snippet generation

### Phase 6: Integration + Verification (Oct 28-30, 2 days)
- [ ] End-to-end integration tests with AQL engine
- [ ] Production readiness verification
- [ ] Final performance gate validation
- [ ] Evidence bundle preparation

## 7. Definition of Done (Design Phase)

- [x] Algorithm documented (BM25 scoring, index layout)
- [x] Data structures specified (on-disk index, in-memory cache)
- [x] API surface defined (FtsExecutor interface, error cases)
- [x] Performance targets established (≤100ms single-term query)
- [x] Test strategy drafted (25 total tests)
- [ ] **GATE: Technical committee approval**
- [ ] Implementation schedule committed (Sept 15 - Oct 30)
- [ ] Team capacity confirmed (3 FTE required)

## 8. Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| Bloom filter false-positive rate > 1% | Low | Medium | Use proven library (e.g., Boost.Bloom) |
| BM25 score precision issues | Low | Low | Unit test with known IDF/TF values |
| Index file corruption | Medium | High | Implement checksums + rebuild from raw docs |
| Cache thrashing under adversarial queries | Low | Medium | LRU eviction policy + memory limits |
| Lock contention under concurrent queries | Low | Medium | Shared_lock for reads; profile under 100+ concurrent clients |
| Performance gate miss on large result sets | Medium | High | Invest in early termination + query planning (Phase 3) |

## 9. Next Steps

1. **Design Approval** (Sept 2-10):
   - [ ] Technical committee review this design document
   - [ ] Obtain written approval from: @query-owner @performance-owner @thread-safety-owner
   - [ ] Resolve any design concerns before Sept 10

2. **Implementation Kickoff** (Sept 15):
   - [ ] Create feature branch: `feature/query-fts-executor`
   - [ ] Assign team members to phases 1-6
   - [ ] Set up performance benchmarking environment
   - [ ] Create GitHub issues for each phase

3. **Weekly Checkpoints**:
   - Mondays 09:00 UTC: Phase progress sync
   - Wednesdays 14:00 UTC: Performance gate review
   - Fridays 16:00 UTC: Integration testing status

---

**Design Document Version**: 1.0 (Draft)
**Date**: 2026-09-02
**Status**: Pending Technical Committee Approval
**Owner**: @query-module-owner
