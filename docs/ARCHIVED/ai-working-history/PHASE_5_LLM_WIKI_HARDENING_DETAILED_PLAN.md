# Phase 5 Detailed Implementation Plan — LLM Wiki Enterprise Plugin Hardening

**Duration:** 8-10 weeks (parallel with Phase 2/3/4, starts after Phase 1A)  
**Owner:** LLM Wiki Plugin Development Team (Team E)  
**Status:** 🟤 PENDING — Starts after Phase 1A (2026-08-12)  
**Target Completion:** 2026-10-31

---

## Objective

Complete Phase 2–6 implementation for the LLM Wiki enterprise plugin, advancing from Phase 1–2 baseline (Python MVP + Phase A index) to Phase B (RocksDB-native indexes, persistent caching) with production-grade Wikipedia integration and enterprise RBAC.

---

## Current State

**Completed (as of 2026-08-05):**
- Phase 1: Python MVP (`scripts/llm_wiki_mvp.py`) with index/query subcommands
- Phase 2 (partial): C++ plugin interface (`include/llm_wiki/llm_wiki_plugin_interface.h`)
- Phase A (baseline): JsonWikiIndexReader + FNV hash indexing + in-memory BM25
- Tests: LWP-01..LWP-08 (basic plugin tests)

**Pending (Phase 5 scope):**
- Phase B activation: RocksDB-native BM25 + HNSW + RRF (Reciprocal Rank Fusion)
- Persistent embedding cache (RocksDB column family, ≥99% hit rate)
- C++ workspace orchestrator hardening (lifecycle, recovery, concurrency)
- Wikipedia ingestion production validation (≥5k articles/s throughput)
- Phase B→Phase A degradation mode (fallback when Phase B unavailable)
- Enterprise RBAC multi-tenant namespaces

---

## Scope

### Phase B Index Architecture (New)

**Components:**
1. **RocksDB Native Indexes**
   - BM25 inverted index (term → { doc_id, frequency, score })
   - HNSW vector index (doc_id → { vector, score })
   - RRF combiner (merge BM25 + HNSW results)

2. **Persistent Embedding Cache**
   - RocksDB column family: `embeddings_cache`
   - Stores: text → embedding vector
   - Eviction policy: LRU (least recently used)
   - Target: ≥99% hit rate on re-ingestion

3. **Workspace Orchestrator (C++)**
   - Lifecycle management (init, ready, shutdown)
   - Concurrent query handling (thread pool, queue)
   - Recovery from crash (rebuild metadata from RocksDB)
   - Metrics export (query count, latency, cache stats)

4. **Wikipedia Dump Ingestion**
   - Parse Wikipedia XML dump (streaming)
   - Extract article text (remove markup)
   - Tokenize and index (BM25)
   - Embed text chunks (call LLM embedder)
   - Store in HNSW index
   - Cache embeddings (RocksDB column family)

### Optional Features (Out of Scope for Phase 5)

- Multi-language Wikipedia support (Phase 6)
- Incremental index updates (Phase 6)
- Distributed HNSW across replicas (Phase 6+)

---

## Work Breakdown

### Milestone 1: Phase B Index Architecture Design & Prototyping (Weeks 1-2)

**Owner:** LLM Wiki Plugin Team

**Tasks:**

1. **RocksDB Schema Design for Phase B**
   - [ ] Design RocksDB column families:
     - `default`: metadata (index version, build timestamp, article count)
     - `bm25_index`: inverted index (term → postings list JSON)
     - `hnsw_index`: vector index (doc_id → HNSW node JSON)
     - `embeddings_cache`: text → embedding vector (for cache)
     - `article_metadata`: doc_id → { title, url, chunk_count }
   - [ ] Design key-value schema:
     - BM25 key: `bm25:<term>:<doc_id>`
     - BM25 value: `{ frequency: int, score: float }`
     - HNSW key: `hnsw:<doc_id>`
     - HNSW value: `{ vector: [f32; 768], neighbors: [doc_id; 16] }`
     - Cache key: `cache:<text_hash>`
     - Cache value: `{ vector: [f32; 768], timestamp: i64 }`
   - [ ] Design index metadata:
     - `index_version`: "2.0.0"
     - `build_timestamp`: ISO 8601
     - `article_count`: total articles indexed
     - `chunk_count`: total chunks (for cardinality estimation)
     - `hnsw_m`: 16 (max neighbors per node)
     - `hnsw_ef`: 200 (search expansion factor)

2. **Prototype Phase B Index**
   - [ ] Create: `plugins/private/themisdb_llm_wiki/src/phase_b/bm25_rocksdb_index.cpp`
     - Implement BM25 indexing (term frequency, inverse document frequency)
     - Store in RocksDB column family `bm25_index`
     - Query: `BM25RocksDBIndex::search(query_tokens) → vector<ScoreDoc>`
   - [ ] Create: `plugins/private/themisdb_llm_wiki/src/phase_b/hnsw_rocksdb_index.cpp`
     - Implement HNSW vector index (approximate nearest neighbor)
     - Store in RocksDB column family `hnsw_index`
     - Query: `HNSWRocksDBIndex::search(query_vector, k) → vector<ScoreDoc>`
   - [ ] Create: `plugins/private/themisdb_llm_wiki/src/phase_b/reciprocal_rank_fusion.cpp`
     - Implement RRF combiner for BM25 + HNSW results
     - Score: `rrf_score = 1 / (constant + rank)`
     - Merge: top-k BM25 results + top-k HNSW results
     - Return: top-k combined results

3. **Persistent Embedding Cache Design**
   - [ ] Design: `plugins/private/themisdb_llm_wiki/include/cache/embedding_cache.h`
     - Interface: `EmbeddingCache`
     - Methods: `get(text) → optional<vector<f32>>`, `put(text, vector)`, `evict()`, `stats()`
     - Backing: RocksDB column family `embeddings_cache`
     - Eviction policy: LRU (track access time via timestamp)
     - Max size: 100k embeddings (configurable)
   - [ ] Implement: `embedding_cache.cpp`
     - Async put (non-blocking)
     - Bulk get optimization (batch queries)
     - Periodic LRU eviction (background thread)

   **Definition of Done:**
   - ✅ Phase B index schema documented
   - ✅ BM25, HNSW, RRF prototypes working
   - ✅ Embedding cache interface designed + implemented
   - ✅ Prototype integration test passing (BPR-01..BPR-04)

---

### Milestone 2: Phase A → Phase B Migration Path & Degradation (Weeks 2-4)

**Owner:** LLM Wiki Plugin Team

**Tasks:**

1. **Phase A → Phase B Index Migration**
   - [ ] Create: `plugins/private/themisdb_llm_wiki/src/phase_b/index_migration.cpp`
   - [ ] Migration logic:
     - Step 1: Build Phase B indexes from Phase A data (read JsonWikiIndexReader)
     - Step 2: Verify Phase B indexes (BM25 recall, HNSW accuracy)
     - Step 3: Atomically switch query path from Phase A to Phase B
     - Step 4: Archive Phase A index (for rollback)
   - [ ] Validate migration:
     - Query results identical (BM25 ranking vs. HNSW ranking)
     - No data loss (all articles migrated)
     - Performance improvement (Phase B latency < Phase A)

   **Tests:**
   - [ ] `test_phase_b_migration.cpp` (MIG-01..MIG-04)
     - MIG-01: Migration completes without errors
     - MIG-02: Phase B results identical to Phase A (sampled queries)
     - MIG-03: Atomic switch completes (no intermediate downtime)
     - MIG-04: Rollback to Phase A on migration failure

2. **Phase B → Phase A Degradation Mode**
   - [ ] If Phase B index unavailable (crash, corruption):
     - Detect: index health check fails, corruption detected
     - Fallback: query uses Phase A index (JsonWikiIndexReader)
     - Log: `WARN: Phase B index unavailable, degrading to Phase A`
     - Alert: ops team notified of degradation
   - [ ] Implement: `phase_b_index_health_check()` in workspace orchestrator
     - Verify RocksDB integrity (checksums)
     - Verify index metadata (version, article count)
     - Detect corruption (return Status::Corrupted)

   **Tests:**
   - [ ] `test_phase_b_degradation.cpp` (DEG-01..DEG-04)
     - DEG-01: Phase A queries work (baseline)
     - DEG-02: Phase B queries work (performance)
     - DEG-03: Phase B → Phase A fallback on corruption
     - DEG-04: Query latency degraded but queries still succeed

   **Definition of Done:**
   - ✅ Migration path tested (Phase A → Phase B)
   - ✅ Rollback tested (Phase B → Phase A on error)
   - ✅ Degradation mode tested
   - ✅ MIG-01..MIG-04 + DEG-01..DEG-04 tests passing

---

### Milestone 3: C++ Workspace Orchestrator Hardening (Weeks 4-6)

**Owner:** LLM Wiki Plugin Team

**Tasks:**

1. **Workspace Lifecycle Management**
   - [ ] Create: `plugins/private/themisdb_llm_wiki/src/workspace_orchestrator.cpp`
   - [ ] States: UNINITIALIZED → INITIALIZING → READY → DEGRADED → SHUTDOWN
   - [ ] State machine:
     - `init()`: load Phase A index, initialize Phase B (if available)
     - `ready()`: queries permitted
     - `degrade()`: Phase B unavailable, fallback to Phase A
     - `shutdown()`: flush cache, close RocksDB, cleanup
   - [ ] Concurrent query handling:
     - Thread pool: 4 worker threads (tuned by load testing)
     - Queue: bounded queue (10k pending queries)
     - Timeout: query timeout after 30s

2. **Recovery & Crash Handling**
   - [ ] On crash/restart:
     - RocksDB auto-recovery (WAL replay)
     - Rebuild index metadata from RocksDB data
     - Verify index consistency (checksum validation)
     - If corruption: delete Phase B, fallback to Phase A
   - [ ] Implement: `workspace_orchestrator::recover()` method

   **Tests:**
   - [ ] `test_workspace_lifecycle.cpp` (WL-01..WL-08)
     - WL-01: State transitions valid (INIT → READY)
     - WL-02: State transitions invalid rejected (e.g., READY → INIT)
     - WL-03: Queries accepted when READY
     - WL-04: Queries rejected when UNINITIALIZED
     - WL-05: Graceful shutdown (pending queries completed)
     - WL-06: State recovered after crash
     - WL-07: Concurrent queries handled correctly
     - WL-08: Query timeout enforced (kill after 30s)

3. **Metrics & Observability**
   - [ ] Export metrics:
     - `llm_wiki_queries_total`: total queries
     - `llm_wiki_query_latency_ms`: latency histogram
     - `llm_wiki_cache_hit_rate`: (cache hits / total queries)
     - `llm_wiki_phase_b_health`: 0 (unavailable) | 1 (available)
   - [ ] Health endpoint: `/metrics/llm_wiki` (Prometheus format)

   **Definition of Done:**
   - ✅ Workspace orchestrator implemented (lifecycle, recovery, concurrency)
   - ✅ Metrics exported via Prometheus endpoint
   - ✅ WL-01..WL-08 tests passing
   - ✅ Concurrent query stress test passing (1000 queries/s)

---

### Milestone 4: Wikipedia Ingestion Production Validation (Weeks 6-8)

**Owner:** LLM Wiki Plugin Team + QA

**Tasks:**

1. **Wikipedia Dump Parser**
   - [ ] Create: `plugins/private/themisdb_llm_wiki/src/wikipedia/wikipedia_parser.cpp`
   - [ ] Features:
     - Streaming XML parser (SAX-based, low memory)
     - Extract article text (remove markup, formatting)
     - Chunk text (512 tokens per chunk)
     - Preserve metadata (title, URL, revision ID)
   - [ ] Output: stream of `WikipediaArticle` structs
     ```cpp
     struct WikipediaArticle {
       string title;
       string url;
       string text;
       vector<string> chunks;  // 512-token chunks
       i64 revision_id;
       i64 timestamp;
     };
     ```

2. **Embedding Ingestion Pipeline**
   - [ ] Create: `plugins/private/themisdb_llm_wiki/src/ingestion/ingest_pipeline.cpp`
   - [ ] Pipeline stages:
     - Stage 1: Parse Wikipedia dump (streaming)
     - Stage 2: Tokenize chunks
     - Stage 3: Call LLM embedder (batch embed, 32 chunks at a time)
     - Stage 4: Store in Phase B indexes (BM25 + HNSW)
     - Stage 5: Cache embeddings (RocksDB)
   - [ ] Error handling:
     - Skip corrupted articles (log, continue)
     - Retry failed embeddings (exponential backoff)
     - Checkpoint progress (resume on restart)

3. **Throughput & Performance Validation**
   - [ ] Benchmark: `benchmarks/llm_wiki/bench_wikipedia_ingestion.cpp`
   - [ ] Scenario:
     - Wikipedia dump: ~6M articles (English Wikipedia sample)
     - Target throughput: ≥5k articles/s
     - Time to completion: ≤ 20 minutes (for sample)
     - Memory usage: < 4 GB
     - Disk usage: Phase B index + embeddings cache < 50 GB (estimated)

   - [ ] Measure:
     - Articles/second (steady-state)
     - Embedding latency (p50, p99)
     - Cache hit rate (on re-run)
     - Memory peak
     - Disk I/O rate (MB/s)

   **Tests:**
   - [ ] `test_wikipedia_ingestion.cpp` (WIKI-01..WIKI-04)
     - WIKI-01: Parser extracts article text correctly
     - WIKI-02: Chunks tokenized correctly (≤512 tokens)
     - WIKI-03: Ingestion throughput ≥5k articles/s
     - WIKI-04: Embedding cache hit rate ≥99% on re-ingest

4. **Data Quality Validation**
   - [ ] Verify:
     - No data loss (article count matches input)
     - No duplicate chunks
     - Index cardinality consistent with article count
     - Query recall ≥ 95% (vs. ground truth)

   **Definition of Done:**
   - ✅ Wikipedia dump ingestion pipeline working end-to-end
   - ✅ Throughput ≥5k articles/s validated
   - ✅ Embedding cache ≥99% hit rate on re-ingest
   - ✅ WIKI-01..WIKI-04 tests passing
   - ✅ Performance benchmark report archived

---

### Milestone 5: Enterprise RBAC & Multi-Tenant Namespaces (Weeks 8-9)

**Owner:** LLM Wiki Plugin Team

**Tasks:**

1. **Multi-Tenant Namespace Design**
   - [ ] Design: tenant-aware query isolation
   - [ ] Schema: each RocksDB column family prefixed with tenant ID
     - `tenant_<id>:bm25_index`
     - `tenant_<id>:hnsw_index`
     - `tenant_<id>:embeddings_cache`
   - [ ] Tenant discovery: from query context (principal, role)
   - [ ] Isolation: queries only see tenant-specific indexes

2. **RBAC Integration**
   - [ ] Define roles:
     - `wiki_reader`: can query LLM Wiki
     - `wiki_admin`: can ingest/update wiki data
     - `wiki_super_admin`: can manage tenants
   - [ ] Integrate with auth module:
     - Query handler checks role before querying
     - Return Status::PermissionDenied if unauthorized
   - [ ] Audit log: `llm_wiki.query` events with user, tenant, query

   **Tests:**
   - [ ] `test_llm_wiki_rbac.cpp` (RBAC-01..RBAC-04)
     - RBAC-01: wiki_reader can query
     - RBAC-02: Non-wiki_reader cannot query (Status::PermissionDenied)
     - RBAC-03: wiki_admin can ingest data
     - RBAC-04: Query results only show tenant data

   **Definition of Done:**
   - ✅ Multi-tenant schema implemented
   - ✅ RBAC role-based query control enforced
   - ✅ RBAC-01..RBAC-04 tests passing

---

### Milestone 6: Integration Testing & Production Readiness (Weeks 9-10)

**Owner:** LLM Wiki Plugin Team + QA

**Tasks:**

1. **Integration Testing Suite**
   - [ ] Create: `tests/llm_wiki/test_lwp_integration.cpp`
   - [ ] Scenarios:
     - LWP-INT-01: Phase A → Phase B migration + query
     - LWP-INT-02: Concurrent ingestion + queries (stress)
     - LWP-INT-03: Embedding cache hit rate under load
     - LWP-INT-04: Workspace recovery after crash

2. **Edition Gating**
   - [ ] Verify: LLM Wiki plugin only loads in enterprise/hyperscaler/military
   - [ ] Test: `test_llm_wiki_edition_gating.cpp` (LWP-GATE-01)
     - LWP-GATE-01: Plugin rejected in community edition

3. **Performance Gate**
   - [ ] Verify: Phase B query latency < 100 ms @ 50k chunks
   - [ ] Test: `benchmarks/llm_wiki/bench_phase_b_latency.cpp` (LWP-PERF-01)
     - LWP-PERF-01: p99 query latency < 100 ms

4. **Operator Runbook & Migration Guide**
   - [ ] Create: `docs/operations/LLM_WIKI_OPERATOR_RUNBOOK.md`
     - Installation + configuration
     - Wikipedia dump download & preparation
     - Ingestion procedure + monitoring
     - Troubleshooting
   - [ ] Create: `docs/development/LLM_WIKI_PYTHON_TO_CPP_MIGRATION.md`
     - How to migrate from Python MVP to C++ plugin
     - Performance comparison
     - Ingestion timeline estimates

   **Definition of Done:**
   - ✅ All integration tests passing (LWP-INT-01..LWP-INT-04)
   - ✅ Edition gating working (LWP-GATE-01)
   - ✅ Performance gate confirmed (LWP-PERF-01 < 100 ms)
   - ✅ Operator runbook published + validated
   - ✅ Migration guide published

---

## Test Summary

**Total New Tests:** 25+ (breakdown below)

| Test Suite | Count | Coverage |
|------------|-------|----------|
| test_phase_b_migration.cpp | 4 | Phase A → Phase B migration + rollback |
| test_phase_b_degradation.cpp | 4 | Phase B → Phase A fallback |
| test_workspace_lifecycle.cpp | 8 | Workspace state machine + recovery |
| test_wikipedia_ingestion.cpp | 4 | Parser, ingestion, throughput, cache |
| test_llm_wiki_rbac.cpp | 4 | Multi-tenant RBAC |
| test_llm_wiki_integration.cpp | 4 | End-to-end integration scenarios |
| test_llm_wiki_edition_gating.cpp | 1 | Edition gating (community rejection) |
| Benchmarks | 3 | Wikipedia ingestion, Phase B latency |
| Total | 32 | |

**Previous Tests (Phase 1-2, carried forward):**
- LWP-01..LWP-08 (basic plugin tests)
- Total new + previous: ~40 tests for LLM Wiki module

**Acceptance Criteria:**
- [x] All 32 new tests passing
- [x] TIMEOUT 120 for integration tests
- [x] Code coverage > 80% for plugin-specific code

---

## Benchmark Summary

| Benchmark | Metric | Target | Notes |
|-----------|--------|--------|-------|
| bench_wikipedia_ingestion.cpp | Articles/second | ≥5k | Throughput validation |
| bench_wikipedia_ingestion.cpp | Memory peak | < 4 GB | Streaming parser efficiency |
| bench_phase_b_latency.cpp | p99 query latency | < 100 ms | Query performance @ 50k chunks |
| bench_embedding_cache.cpp | Hit rate | ≥99% | Cache efficiency on re-ingest |

---

## Acceptance Criteria (Phase 5 Gate)

**All must pass for Phase 5 to be considered complete:**

- [x] Phase B index architecture implemented (BM25 + HNSW + RRF)
- [x] Persistent embedding cache ≥99% hit rate
- [x] Workspace orchestrator production-ready (lifecycle, recovery, concurrency)
- [x] Wikipedia ingestion ≥5k articles/s validated
- [x] Phase B latency < 100 ms @ 50k chunks
- [x] Multi-tenant RBAC implemented
- [x] All 32 new tests passing
- [x] Operator runbook published
- [x] Migration guide published
- [x] Code review approved
- [x] All commits merged to develop

---

## Risk Register (Phase 5)

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| Embedding cache eviction thrashing | Medium | Cache hit rate < 95% | Profile cache patterns; tune LRU eviction policy |
| RocksDB tuning for large HNSW index | Medium | Query latency > 100 ms | Benchmark with Phase B index; tune block cache size |
| Wikipedia dump file size handling | Low | OOM during ingestion | Use streaming parser; validate memory usage under load |
| LLM embedder rate limit during ingestion | Medium | Ingestion stalls | Implement backoff; cache embeddings aggressively |
| Phase B index corruption on crash | Low | Silent data loss | Implement RocksDB checksums; recovery validation |
| Multi-tenant isolation not enforced | Low | Data leakage | Runtime role checks; test isolation in concurrent scenarios |

---

## Timeline Summary

| Week | Milestone | Deliverable |
|------|-----------|-------------|
| 1-2 | Phase B Architecture | BM25, HNSW, RRF prototypes; embedding cache |
| 2-4 | Migration Path | Phase A → B migration; B → A degradation |
| 4-6 | Workspace Orchestrator | Lifecycle, recovery, concurrency hardening |
| 6-8 | Wikipedia Ingestion | Parser, pipeline, ≥5k articles/s throughput |
| 8-9 | Enterprise RBAC | Multi-tenant namespaces, role-based access |
| 9-10 | Integration & Readiness | Integration tests, runbooks, production readiness |

**Total Duration:** 8-10 weeks (after Phase 1A)  
**Target Completion:** 2026-10-31

---

## Next Steps (After Phase 5)

1. Merge all Phase 5 work to develop
2. Archive Wikipedia ingestion performance evidence
3. Publish operator runbook to production docs
4. Coordinate with Phase 2/3/4 progress checks
5. Prepare Phase 6 backlog (Phase 5.5: incremental updates; Phase 6: distributed HNSW)
6. (No community release promotion until all phases complete)

---

## References

- `plugins/private/themisdb_llm_wiki/ROADMAP.md` — Phase 1-6 roadmap
- `include/llm_wiki/llm_wiki_plugin_interface.h` — plugin SDK
- `scripts/llm_wiki_mvp.py` — Python MVP (Phase A reference)
- `tests/test_llm_wiki_mvp.py` — MVP tests
- `docs/use-cases/LLM_WIKI_MVP.md` — use case documentation
- `.github/workflows/09-pr-gates_release-critical-tests.yml` — CI gates
