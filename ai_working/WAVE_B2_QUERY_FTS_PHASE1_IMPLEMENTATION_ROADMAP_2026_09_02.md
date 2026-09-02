# Wave B Option B2: Query FTS Executor — Phase 1 Implementation Roadmap
## Status: ✅ READY FOR EXECUTION

**Module:** Query (FTS Executor Backend)  
**Phase:** Phase 1 (Index Loading + BM25 Scoring + Thread-Safety)  
**Timeline:** Sept 16-30, 2026 (15 days design+implementation sprint)  
**Gate:** ROADMAP line 63 — "Query FTS executor backend | 0% progress | BEGIN IMMEDIATELY; 4-6 weeks"  
**Criticality:** 🔴 **CRITICAL PATH** (0% implementation, Q4 deadline at risk)  
**Success Criteria:** 42+ unit/integration tests passing + Phase 1 acceptance checklist signed off

---

## Current State (2026-09-02 Baseline)

### ✅ What's Done
- Design document: `src/query/DESIGN_FTS_EXECUTOR_2026-09-10.md` (350+ lines, comprehensive)
- Tokenizer + Parser: `src/query/fts_tokenizer.cpp` + `src/query/fts_parser.cpp` (feature-complete)
- AST structures: `SearchNode`, `Token`, parser output (ready)

### ❌ What's Missing (Phase 1 Scope)
1. **FtsExecutor class** — stub exists, execute() returns NOT_IMPLEMENTED
2. **BM25Scorer** — stub, no score computation
3. **IndexCache** — stub, no LRU/Bloom filter logic
4. **Thread-safety** — no shared_mutex annotations, no concurrent test harness
5. **Error handling** — no FtsError enum implementation, no recovery paths
6. **Parser integration** — AST nodes not wired to executor

### ⚠️ Known Gaps
- No RocksDB persistence (Phase 2 scope)
- No distributed query coordination (Phase 3 scope)
- No performance optimization/early termination (Phase 4 scope)

---

## Phase 1 Execution Plan (Sept 16-30)

### Week 1 (Sept 16-23): Architecture Design + Header Files Lock

**Objective:** Finalize specifications, lock header files, unblock implementation

#### Task 1.1: Architecture Review & Specification Lockdown (Sept 16-17, 2 days)
- **Lead:** Query Architect
- **Deliverables:**
  - [ ] Review DESIGN_FTS_EXECUTOR_2026-09-10.md with team
  - [ ] Document decision points (Bloom filter false-positive rate, LRU size, BM25 parameters)
  - [ ] Lock design: No further changes after Sept 18 23:59 UTC
  - [ ] Create: `DESIGN_DECISIONS_2026_09_16.md` (recorded decisions + rationale)
- **Output:** Design locked + team aligned

#### Task 1.2: Header File Generation (Sept 18-19, 2 days)
- **Lead:** Query Engineer
- **Scope:**
  1. **`include/query/fts_executor.h`** — Main executor interface
     ```cpp
     namespace themis::query::fts {
       class FtsExecutor { /* public API */ };
       struct SearchResult { /* defined in design */ };
       struct ExecutionOptions { /* defined in design */ };
       enum class FtsError { /* 7200-7299 error codes */ };
       Result<T> type (using Outcome library or std::expected equivalent)
     }
     ```
  
  2. **`include/query/bm25_scorer.h`** — BM25 scoring
     ```cpp
     class BM25Scorer {
       float compute(DocumentId doc_id, const SearchNode& query, 
                     const IndexStatistics& stats);
     };
     ```
  
  3. **`include/query/index_cache.h`** — LRU + Bloom filter cache
     ```cpp
     class IndexCache {
       std::optional<PostingList> lookup(const std::string& term);
       void insert(const std::string& term, PostingList&& list);
       void evict_lru();
     };
     ```
  
  4. **`include/query/fts_index.h`** — Index abstraction
     ```cpp
     class FtsIndex {
       std::vector<DocumentId> lookup_term(const std::string& term);
       DocumentStatistics get_doc_stats(DocumentId doc_id);
     };
     ```

- **Deliverables:**
  - [ ] All 4 headers created + peer-reviewed
  - [ ] Doxygen comments complete (all public APIs documented)
  - [ ] Thread-safety contracts documented (@thread-safe, @not-thread-safe)
  - [ ] Error handling patterns documented (Result<T>, FtsError enum)
- **Output:** Headers locked, implementation can proceed in parallel

#### Task 1.3: Test Skeleton + Acceptance Checklist (Sept 20-23, 4 days)
- **Lead:** Test Lead
- **Test Files to Create:**
  1. **`tests/query/test_bm25_scorer.cpp`** — 12+ tests
     - [ ] BM25 score computation with known documents
     - [ ] Edge cases: empty query, single term, phrase
     - [ ] Parameter sensitivity (k1, b values)
     - [ ] IDF computation correctness
     - [ ] Score normalization [0, ∞)
  
  2. **`tests/query/test_index_cache.cpp`** — 8+ tests
     - [ ] LRU eviction FIFO order
     - [ ] Bloom filter false-positive rate ≤1%
     - [ ] Cache hit/miss statistics
     - [ ] Memory bounds enforcement
     - [ ] Concurrent access (stress test)
  
  3. **`tests/query/test_fts_executor_threading.cpp`** — 8+ tests
     - [ ] Concurrent reads (N parallel threads)
     - [ ] TSAN race condition detection
     - [ ] Shared lock allows concurrent queries
     - [ ] Unique lock enforces exclusive index updates
     - [ ] No deadlock scenarios
  
  4. **`tests/query/test_fts_parser_integration.cpp`** — 8+ tests
     - [ ] AST nodes → executor input
     - [ ] Simple term query
     - [ ] Phrase query (position matching)
     - [ ] Boolean AND/OR/NOT
     - [ ] Parser error handling
  
  5. **`tests/query/test_fts_executor_basic.cpp`** — 8+ tests
     - [ ] execute() on mock index
     - [ ] Smoke test (execute simple query)
     - [ ] Result ordering (highest score first)
     - [ ] Limit parameter respected
     - [ ] Timeout mechanism working

- **Deliverables:**
  - [ ] All test files created with skeleton (tests compile, placeholders pass)
  - [ ] Create: `PHASE_1_ACCEPTANCE_CHECKLIST.md` (42+ test targets documented)
  - [ ] Test count target: 42+ tests (distributed across files)
- **Output:** Test framework ready, tests can be written in parallel with implementation

---

### Week 2 (Sept 24-30): Implementation + Testing

**Objective:** Full Phase 1 implementation + 42+ tests passing

#### Task 2.1: BM25Scorer Implementation (Sept 24-25, 2 days)
- **Lead:** Query Engineer
- **Implementation:**
  ```cpp
  // File: src/query/bm25_scorer.cpp
  float BM25Scorer::compute(DocumentId doc_id, const SearchNode& query, 
                             const IndexStatistics& stats) {
    // 1. Fetch term frequencies + doc stats from index
    // 2. For each TERM in query:
    //    a. Compute IDF = log((N - df + 0.5) / (df + 0.5))
    //    b. Compute TF component = (tf * (k1 + 1)) / (tf + k1 * (1 - b + b * |D| / avgDL))
    //    c. Score += IDF * TF component
    // 3. For AND/OR/NOT nodes: combine recursively
    // 4. Return total score
  }
  ```
  
  - Constants: k1=1.5, b=0.75 (configurable)
  - Edge cases: zero frequency, missing term (return 0)
  - Performance: O(query_terms) per document

- **Tests to Pass:** `test_bm25_scorer.cpp` (12+ tests)
- **Deliverables:**
  - [ ] BM25Scorer fully implemented
  - [ ] 12+ tests passing with no warnings
  - [ ] Scoring output validated against reference implementation (Python benchmark)

#### Task 2.2: IndexCache Implementation (Sept 25-26, 2 days)
- **Lead:** Query Engineer
- **Implementation:**
  ```cpp
  // File: src/query/index_cache.cpp
  class IndexCache {
    LRUCache<std::string, PostingList> cache_;  // Hot posting lists
    BloomFilter term_filter_;                   // Quick negative lookup
    std::shared_mutex lock_;                    // Thread-safe access
    
    std::optional<PostingList> lookup(const std::string& term);  // Uses lock
    void insert(const std::string& term, PostingList&& list);    // Uses lock
  };
  ```
  
  - LRU: intrusive linked-list + hash map for O(1) access
  - Bloom filter: 1% false-positive rate (8 hash functions, sized for N terms)
  - Memory budget: 100 MB default (configurable)
  - Thread safety: shared_lock for reads, unique_lock for writes

- **Tests to Pass:** `test_index_cache.cpp` (8+ tests)
- **Deliverables:**
  - [ ] IndexCache fully implemented
  - [ ] LRU eviction policy verified
  - [ ] 8+ tests passing
  - [ ] Memory bounds enforced

#### Task 2.3: FtsExecutor Core Implementation (Sept 27-28, 2 days)
- **Lead:** Query Architect
- **Implementation:**
  ```cpp
  // File: src/query/fts_executor.cpp
  Result<std::vector<SearchResult>> FtsExecutor::execute(
      const SearchNode& query, 
      const ExecutionOptions& options) {
    // 1. Acquire shared_lock (allows N concurrent readers)
    // 2. Query planning: estimate selectivity, build traversal plan
    // 3. Traverse AST bottom-up (postorder traversal)
    // 4. For each matching document: compute BM25 score (using BM25Scorer)
    // 5. Sort by score descending
    // 6. Early termination if K results found with acceptable score
    // 7. Return top-K results
  }
  ```
  
  - Thread safety: shared_lock for concurrent execution
  - Error handling: timeout, OOM, index corruption (return FtsError)
  - Query execution flow (pseudo-code above; implement step-by-step)

- **Tests to Pass:** `test_fts_executor_basic.cpp` (8+ tests)
- **Deliverables:**
  - [ ] FtsExecutor::execute() fully implemented
  - [ ] 8+ basic tests passing
  - [ ] Thread safety verified (no TSAN warnings)

#### Task 2.4: Thread-Safety Hardening (Sept 28-29, 2 days)
- **Lead:** Test Lead + Query Engineer
- **Implementation:**
  - Annotate all shared data members with thread-safety contracts
  - Implement RAII lock guards for all critical sections
  - Document lock ordering: always index_lock_ before cache_lock_
  - No deadlock scenarios (verify by code inspection + stress testing)

- **Tests to Pass:** `test_fts_executor_threading.cpp` (8+ tests)
- **Deliverables:**
  - [ ] All thread-safety contracts documented in headers
  - [ ] TSAN clean: zero race condition warnings
  - [ ] 8+ concurrency tests passing (stress test: 16 threads, 1000 ops each)
  - [ ] Deadlock detection: zero timeouts in concurrent scenarios

#### Task 2.5: Parser Integration (Sept 29-30, 2 days)
- **Lead:** Query Engineer
- **Implementation:**
  - Wire FtsParser output (SearchNode AST) → FtsExecutor::execute() input
  - Implement error propagation: parse errors → FtsError::PARSING_ERROR
  - Add example integration tests combining parser → executor

- **Tests to Pass:** `test_fts_parser_integration.cpp` (8+ tests)
- **Deliverables:**
  - [ ] Parser → Executor pipeline working end-to-end
  - [ ] 8+ integration tests passing
  - [ ] Error cases (malformed query) handled gracefully

#### Task 2.6: Test Execution + Coverage (Sept 30, Final Day)
- **Lead:** Test Lead
- **Execution:**
  - Run all 42+ tests in parallel: `ctest -j 4 --verbose`
  - Coverage target: ≥85% of Phase 1 code (src/query/fts_* functions)
  - Document any flaky tests (re-run 5x each)

- **Deliverables:**
  - [ ] All 42+ tests passing (100% pass rate)
  - [ ] Code coverage ≥85%
  - [ ] Zero flaky tests
  - [ ] Create: `PHASE_1_FINAL_RESULTS_2026_09_30.md` (test results summary)

---

## Resource Allocation

**Team Size:** 2 FTE (Sept 16-30)
- **Query Architect** (1 FTE): Design, execution flow, integration
- **Query Engineer** (1 FTE): BM25, cache, thread-safety, parser integration
- **Test Lead** (0.5 FTE, shared): Test framework, coverage, TSAN

**Time Breakdown:**
- Architecture + design lockdown: 8 hours (Sept 16-18)
- Header files: 16 hours (Sept 18-20)
- Test skeleton: 32 hours (Sept 20-23)
- BM25Scorer: 16 hours (Sept 24-25)
- IndexCache: 16 hours (Sept 25-26)
- FtsExecutor core: 16 hours (Sept 27-28)
- Thread-safety: 16 hours (Sept 28-29)
- Parser integration: 16 hours (Sept 29-30)
- Test execution + documentation: 8 hours (Sept 30)

**Total:** 144 hours (18 FTE-days)

---

## Success Criteria (Sept 30, 18:00 UTC)

✅ **ALL must be true:**

1. **Architecture:**
   - [ ] Design document finalized + locked (no changes Sept 19+)
   - [ ] All 4 header files created + peer-reviewed
   - [ ] Thread-safety contracts documented

2. **Implementation:**
   - [ ] BM25Scorer fully implemented (compute score correctly)
   - [ ] IndexCache fully implemented (LRU + Bloom filter working)
   - [ ] FtsExecutor::execute() fully implemented (query execution end-to-end)
   - [ ] Parser → Executor integration complete

3. **Testing:**
   - [ ] All 42+ tests pass (100% pass rate)
   - [ ] Code coverage ≥85%
   - [ ] Zero TSAN race conditions
   - [ ] Zero flaky tests

4. **Documentation:**
   - [ ] Doxygen comments complete for all public APIs
   - [ ] Thread-safety contracts documented
   - [ ] Design decisions recorded (DESIGN_DECISIONS_2026_09_16.md)
   - [ ] Phase 1 acceptance checklist signed off

5. **Quality Gates:**
   - [ ] No compiler warnings
   - [ ] Clang-format clean
   - [ ] Clang-tidy clean (or justified exceptions)
   - [ ] All tests compile + run without errors

**FINAL GATE:** 42+ tests passing + design locked + 0 blockers for Phase 2 → Phase 1 COMPLETE ✅

---

## Phase 2 Kickoff (Oct 1, 2026)

**Not in Phase 1 scope, but critical for Q4 deadline:**

- **Phase 2:** RocksDB persistence (Oct 1-14, 80 hours)
  - Index file I/O + serialization
  - On-disk format implementation
  - Recovery from corrupted index
  
- **Phase 3:** Distributed execution (Oct 15-28, 80 hours)
  - Cross-shard query coordination
  - Merge/ranking aggregation
  
- **Phase 4:** Performance optimization (Oct 29-Nov 11, 60 hours)
  - Query planning improvements
  - Early termination
  - Cache tuning
  - Performance target: p95 ≤100ms on 100K documents

- **Phase 5:** Error handling + edge cases (Nov 12-25, 80 hours)
  - Malformed queries
  - Resource exhaustion
  - Timeout handling
  
- **Phase 6:** Documentation + GA readiness (Nov 26-30, 40 hours)
  - API documentation
  - Operational runbooks
  - Migration guide

---

## Known Risks & Mitigations

| Risk | Probability | Impact | Mitigation |
|---|---|---|---|
| Design review uncovers architectural issues (Sept 16-18) | MEDIUM | HIGH | Schedule review Sept 16-17, escalate for decision if needed |
| BM25 parameter tuning requires reference benchmarks unavailable | MEDIUM | MEDIUM | Use industry-standard k1=1.5, b=0.75; adjust post-Phase 4 if needed |
| Thread-safety bugs surface late (TSAN detects race in Week 2) | MEDIUM | MEDIUM | Implement thread-safety first (Task 2.4), test continuously |
| Test flakiness (timing-sensitive concurrency tests) | MEDIUM | LOW | Re-run flaky tests 5x each; document and flag for Oct hardening |
| Parser AST incompatibility discovered (incompatible node types) | MEDIUM | HIGH | Integration test early (Task 2.5 Sept 29); adjust AST if needed |

---

## Document Status

**Prepared By:** Copilot Coding Agent  
**Date:** Sept 2, 2026, 14:59 UTC  
**Session:** Wave B Implementation — Option B2 (Query FTS Executor Phase 1)  
**Gate:** Critical blocker — "0% progress, Q4 deadline at risk"

**Status:** ✅ READY FOR EXECUTION (Sept 16, 2026, 09:00 UTC)

---

## Next Steps

**Pre-Execution (Sept 2-15):**
- [ ] Review this Phase 1 roadmap with Query team
- [ ] Allocate resources (2 FTE, Query Architect + Query Engineer)
- [ ] Prepare development environment (sccache, build tools)
- [ ] Identify reference implementations for BM25 benchmarking

**Execution (Sept 16-30):**
- [ ] Sept 16: Kick-off meeting + design review
- [ ] Sept 18: Header files locked
- [ ] Sept 23: Test skeleton complete
- [ ] Sept 30: Phase 1 complete (42+ tests passing)

**Post-Execution (Oct 1+):**
- [ ] Oct 1: Phase 2 kickoff (RocksDB persistence)
- [ ] Oct 30: All 6 phases complete (Q4 deadline)

