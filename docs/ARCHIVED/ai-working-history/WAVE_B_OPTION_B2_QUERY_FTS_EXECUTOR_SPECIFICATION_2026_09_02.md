# Wave B Option B2: Query FTS Executor Backend — Specification + Phase 1 Delivery Planning
## Status: ✅ READY FOR EXECUTION

---

## Executive Summary

**Module:** Query (full-text search executor backend)  
**Scope:** FTS executor architecture design + Phase 1 implementation (index loading)  
**Timeline:** Sept 16-30 design + Phase 1 start (15 days planning + 5 days Phase 1 start)  
**Critical Blocker:** ROADMAP line 63 — "Query FTS executor backend wiring | 0% implementation progress | Begin immediately; 4-6 weeks estimated"  
**Gate:** Q4 2026 deadline at risk; must start immediately to meet Oct 30 Phase 1 target  
**Outcome:** Complete design spec + Phase 1 (index loading layer) implementation + tests

---

## Current State (2026-09-02 Baseline)

### Codebase Status
- ✅ **Parser Complete:** AQL lexer/parser for FTS queries (Phase A-B)
- ✅ **Physical Query Optimizer:** Query planning framework operational
- ⚠️ **FTS Executor Backend:** 0% implementation (blocking Q4 deadline)
- ❌ **Missing:** Index data structure, scoring layer, thread-safety harness

### FTS Query Requirements (from `src/query/ROADMAP.md`)
- Full-text search via AQL `SELECT ... WHERE text.search('query')`
- BM25 scoring algorithm + configurable weights (k1, b, avgdl)
- Multiple index formats: RocksDB prefix-tree, in-memory bitmap, SIMD vectorized
- Distributed execution: per-shard index search + distributed ranking
- Performance gate: ≤100ms on 100K documents (p95)

### Known Limitations
- No FTS executor implementation (parser exists only)
- No index layout design (proposal: RocksDB + in-memory cache)
- No BM25 scoring harness
- No thread-safety guarantees documented
- No test strategy defined

---

## Execution Plan (Sept 16-30 Design + Phase 1 Start)

### Phase 1: Architecture Design & Specification (Sept 16-23, 8 days)

**Objective:** Complete FTS executor design spec + lock API contracts

**Tasks:**

1. **Review Existing Design Documents** (Sept 16, 09:00 UTC, 4 hours)
   - Read: `src/query/DESIGN_FTS_EXECUTOR_2026-09-10.md` (existing draft)
   - Review: Parser AST structure for FTS queries
   - Inventory: Available index formats (RocksDB, in-memory, SIMD)
   - Document: Current design gaps + blockers

2. **Design Core Data Structures** (Sept 16-17, 2 days)
   - **FTS Index Interface:**
     ```cpp
     class FTSIndex {
       virtual ~FTSIndex() = default;
       virtual SearchResults search(const Query& q) = 0;
       virtual void build(const Documents& docs) = 0;
       virtual size_t memoryUsage() const = 0;
     };
     ```
   
   - **Index Implementations:**
     - `RocksDBFTSIndex`: Persistent prefix-tree index (production)
     - `InMemoryBitmapIndex`: Fast in-memory index (test environment)
     - `SIMDVectorizedIndex`: SIMD scoring (future optimization)
   
   - **Scoring Result:**
     ```cpp
     struct ScoringResult {
       DocumentID doc_id;
       float bm25_score;     // Raw BM25 value
       float normalized_score; // [0, 1] range
       std::vector<int> term_positions; // For highlighting
     };
     ```

3. **BM25 Scoring Algorithm Specification** (Sept 17-18, 2 days)
   - Parameterization: k1 (default 1.2), b (default 0.75), avgdl (computed)
   - Formula: `BM25(q, D) = Σ IDF(qi) * (k1+1)*freq(qi,D) / (k1*(1-b+b*|D|/avgdl) + freq(qi,D))`
   - IDF variant: Standard log-odds (saturate at max to avoid zero-division)
   - Thread-safe parameter updates: Store params in atomic<ConfigSnapshot>
   - Query normalization: Stemming, stop-word removal, term weighting
   - Edge cases: Empty queries, stop-word-only queries, zero-frequency terms

4. **Index Loading & Initialization** (Sept 18-19, 2 days)
   - Index path resolution (local disk, S3/GCS for distributed)
   - Lazy loading: Build on-demand or pre-warm at startup
   - Incremental updates: Handle new documents post-initialization
   - Invalidation strategy: TTL or explicit invalidate() call
   - Error handling: Missing index file → fallback to sequential scan (degraded mode)

5. **Thread-Safety & Concurrency Model** (Sept 19-20, 2 days)
   - Readers: Multiple concurrent search() calls (lock-free reads via snapshot)
   - Writers: build() / update() serialized (mutex-protected)
   - Coordination: RwLock or SeqLock for read-heavy workload
   - Memory ordering: Acquire/release semantics for index swap
   - Test strategy: Concurrent reader/writer stress test (32 threads, 60 sec)

6. **Distributed Query Coordination** (Sept 20-21, 2 days)
   - Per-shard index search (each replica searches local index)
   - Ranking aggregation (coordinator collects per-shard results)
   - Score normalization (account for different index sizes per shard)
   - Distributed pagination: limit/offset with per-shard ranking
   - Timeout strategy: Partial results OK (fail-open on slow shard)

7. **Final Specification Document** (Sept 21-23, 3 days)
   - Generate: `FTS_EXECUTOR_SPECIFICATION_FINAL_2026_09_23.md`
   - Sections:
     1. Executive summary (architecture, components, dependencies)
     2. Data structures (FTSIndex interface, ScoringResult, IndexMetadata)
     3. BM25 scoring algorithm (formulas, parameters, edge cases)
     4. Index loading + initialization (lifecycle, error handling, fallback)
     5. Thread-safety model (concurrency strategy, synchronization primitives)
     6. Distributed execution (per-shard search, aggregation, pagination)
     7. API contracts (function signatures, error codes, exceptions)
     8. Test strategy (unit, integration, chaos, performance gates)
   
   - Sign-off checklist:
     - [ ] All data structures defined + documented
     - [ ] API contracts locked (no breaking changes)
     - [ ] Thread-safety guarantees proven (or marked [?] for design review)
     - [ ] Distributed execution flow validated against sharding contracts
     - [ ] Performance assumptions documented (latency targets, memory budgets)
     - [ ] Error handling strategy complete (15+ error scenarios covered)

### Phase 2: Phase 1 Implementation Kick-Off (Sept 24-30, 7 days)

**Objective:** Start Phase 1 (index loading layer) + demonstrate forward progress

**Tasks:**

1. **Header Files Creation** (Sept 24, 09:00 UTC, 8 hours)
   - Create: `include/query/fts_executor.h`
     ```cpp
     namespace query::fts {
       class FTSExecutor {
         // Query execution interface
         FutureResult<SearchResults> execute(const FTSQuery& q) const;
         
         // Index lifecycle
         Status rebuild(const Documents& docs);
         Status update(DocumentID id, const Document& doc);
         
         // Configuration
         void setScoreWeights(float k1, float b);
       };
     }
     ```
   
   - Create: `include/query/fts_index.h` (abstract interface)
   - Create: `include/query/bm25_scorer.h` (scoring implementation)

2. **Implementation Foundation** (Sept 24-25, 2 days)
   - Implement: `BM25Scorer` class (scoring algorithm + parameterization)
   - Implement: `InMemoryBitmapIndex` (test index, no RocksDB dependency)
   - Unit tests: 12+ tests for BM25 scorer (edge cases, term weighting)
   - Unit tests: 8+ tests for in-memory index (add, search, clear)
   - Pass rate target: 20/20 tests green

3. **Index Loading Layer** (Sept 26-27, 2 days)
   - Implement: `FTSIndexLoader` class (file I/O, metadata validation)
   - Implement: Lazy loading strategy (build on first query if not initialized)
   - Implement: Error handling (missing file → degraded sequential scan fallback)
   - Integration tests: 6+ tests for loading, invalidation, fallback
   - Pass rate target: 6/6 tests green

4. **Thread-Safety Harness** (Sept 28, 1 day)
   - Implement: Basic concurrency (RwLock over in-memory index)
   - Add: Concurrent reader stress test (16 threads, 10K queries each)
   - Add: Reader-writer contention test (8 readers + 1 writer, 60 sec sustained)
   - Pass rate target: Concurrent tests green, no data races (TSAN clean)

5. **Query AST Integration** (Sept 29, 1 day)
   - Integrate: Parser AST → FTSQuery translation layer
   - Add: Query normalization (stemming, stop-word removal)
   - Add: Parameter extraction (k1, b, weights from query hints)
   - Test: 8+ tests for AST integration + parameter extraction

6. **Phase 1 Acceptance Checklist** (Sept 30, 1 day)
   - [ ] FTSExecutor header + stub implementation complete
   - [ ] BM25Scorer algorithm fully implemented + 12+ tests green
   - [ ] InMemoryBitmapIndex working + 8+ tests green
   - [ ] FTSIndexLoader error handling + 6+ tests green
   - [ ] Thread-safety harness + concurrent stress tests green (TSAN clean)
   - [ ] Parser AST integration + 8+ tests green
   - [ ] Total: 42+ tests passing, 0 blockers for Phase 2
   - [ ] Documentation: Phase 1 completion summary + next-phase blockers
   - [ ] Code review: All code passes linting + style checks

---

## Phase 2-6 Implementation Plan (Post-Wave B)

### Phase 2: Core Scoring & Index Persistence (Oct 1-14, 2 weeks)
- Implement: RocksDBFTSIndex (persistent prefix-tree)
- Implement: Full BM25 with configurable weights
- Tests: 20+ persistence + scoring integration tests
- Target: Index persist/load round-trip green

### Phase 3: Distributed Query Execution (Oct 15-28, 2 weeks)
- Implement: Per-shard index search coordination
- Implement: Distributed ranking aggregation
- Implement: Pagination with per-shard limits
- Tests: 15+ distributed execution tests
- Target: Sharding module integration green

### Phase 4: Performance Optimization (Oct 29-Nov 11, 2 weeks)
- Implement: SIMD vectorized scoring (optional, performance gate only)
- Benchmark: p95 latency ≤100ms on 100K documents
- Optimize: Index size, memory footprint, search throughput
- Tests: 8+ performance gate tests

### Phase 5: Error Handling & Edge Cases (Nov 12-25, 2 weeks)
- Error codes: 6000-6099 (Query module range)
- Error scenarios: Invalid index format, OOM, corrupted documents
- Edge cases: Empty queries, special characters, very large result sets
- Tests: 20+ error handling + edge case tests

### Phase 6: Documentation & Integration (Nov 26-30, 1 week)
- API documentation: Doxygen comments + README
- Integration guide: How to enable FTS in AQL
- Operational runbook: Index rebuild, troubleshooting
- Target: Full documentation + operational readiness

---

## Resource Allocation

**Team:** 2 FTE (Sept 16-30 design + Phase 1 start)
- **Query Architect** (1 FTE): Specification design, API contracts, design reviews
- **Query Engineer** (1 FTE): Phase 1 implementation, testing, integration
- **DevOps/CI** (0.25 FTE, ad-hoc): Benchmark setup for Phase 4

**Time Breakdown (Sept 16-30):**
- Design + specification: 56 hours (8 days × 7 hours/day)
- Phase 1 implementation: 40 hours (5 days × 8 hours/day)
- Testing + code review: 32 hours (4 days × 8 hours/day)
- Documentation + sign-off: 16 hours
- **Total:** 144 hours (~18 FTE-days)

**Effort Estimate (Phase 2-6):**
- Phase 2 (persistence): 80 hours
- Phase 3 (distributed): 80 hours
- Phase 4 (optimization): 60 hours
- Phase 5 (error handling): 80 hours
- Phase 6 (documentation): 40 hours
- **Total Q4 2026:** 340 hours (~42 FTE-days)

---

## Success Criteria (Sept 30, 18:00 UTC)

✅ **Phase 1 Acceptance (Phase 1 Checkpoint):**
- [ ] FTS Executor specification complete + locked (no further design changes)
- [ ] 42+ unit/integration tests passing (BM25 + Index + Thread-safety)
- [ ] TSAN/sanitizer clean (no data races, memory leaks)
- [ ] Code review approved (style, error handling, documentation)
- [ ] Phase 2 blockers identified (0 blockers = ready to start Oct 1)

✅ **Phase 1 Implementation (Phase 1 Target):**
- [ ] BM25Scorer: 12+ tests, all edge cases covered ✅
- [ ] InMemoryBitmapIndex: 8+ tests, search/add/clear working ✅
- [ ] FTSIndexLoader: 6+ tests, fallback path working ✅
- [ ] Thread-safety: Concurrent tests + TSAN green ✅
- [ ] Parser integration: 8+ tests, normalization working ✅
- [ ] Total: 42+ tests green, 0 failures

---

## Known Risks & Mitigations

| Risk | Probability | Impact | Mitigation |
|---|---|---|---|
| Design review reveals architectural issues | MEDIUM | HIGH | Lock spec by Sept 23; escalate for decision if needed Sept 24 |
| RocksDB integration more complex than expected | MEDIUM | MEDIUM | Plan for Sept 15 pre-spike; defer RocksDB to Phase 2 if needed |
| Performance gate (≤100ms) not achievable | LOW | HIGH | Benchmark during Phase 4; if miss, investigate indexing strategy |
| Thread-safety bugs found late | MEDIUM | MEDIUM | Concurrent stress testing Sept 28; TSAN on all commits |
| Parser AST format doesn't support FTS queries | LOW | HIGH | Verify parser capabilities Sept 16; escalate if incompatible |
| Phase 2-6 effort underestimated | MEDIUM | MEDIUM | Re-estimate after Phase 1; consider splitting into Q1 2027 if overrun |

---

## Next Steps

**Pre-Execution (Sept 2-15):**
- [ ] Review existing DESIGN_FTS_EXECUTOR_2026-09-10.md (identify gaps)
- [ ] Verify parser AST supports FTS query syntax
- [ ] Pre-spike RocksDB integration (1 hour exploration)
- [ ] Set up benchmark infrastructure for Phase 4 (performance gates)

**Execution (Sept 16-30):**
- [ ] Sept 16-23: Specification design + API contracts locked
- [ ] Sept 24: Header files + foundation implementation started
- [ ] Sept 25-30: Phase 1 (index loading) implementation + tests
- [ ] Sept 30: Phase 1 acceptance checklist + sign-off

**Post-Execution (Oct 1+):**
- [ ] Oct 1-14: Phase 2 (persistence + RocksDB)
- [ ] Oct 15-28: Phase 3 (distributed execution)
- [ ] Oct 29-Nov 11: Phase 4 (performance optimization)
- [ ] Nov 12-25: Phase 5 (error handling)
- [ ] Nov 26-30: Phase 6 (documentation + GA readiness)
- [ ] Target: FTS executor fully operational by Q4 2026 end

---

## Document Status

**Prepared By:** Copilot Coding Agent  
**Date:** Sept 2, 2026, 14:35 UTC  
**Session:** Wave B Planning — Option B2 (Query FTS Executor)  
**Gate:** Critical Q4 2026 deadline (0% progress must begin immediately)

**Status:** ✅ READY FOR EXECUTION (Sept 16, 2026, 09:00 UTC)

