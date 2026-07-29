# AQL v2.0.0 — Complete Language Standard Roadmap

**Status**: In Progress (2026-07-27)  
**Target Release**: v2.0.0 (Q4 2026, November 2026)  
**Scope**: Full ArangoDB AQL feature parity + ThemisDB extensions  
**Effort**: 20-25 calendar weeks (5-6 months)  
**Teams Required**: 2-3 parallel workstreams  

> ⚠️ **PREREQUISITE**: [AQL Architecture Consolidation Audit](../../AQL_CONSOLIDATION_AUDIT_2026_06_18.md) — 70 hours, must complete BEFORE Phase 1 kickoff. See: src/query ↔ src/aql integration boundary formalization.

---

## 📋 Executive Summary

v2.0.0 delivers **production-ready AQL with complete DML, DDL, and spatial query support**:

| Feature | Effort | Duration | Critical Path | Team |
|---------|--------|----------|---|------|
| **Mutations** (INSERT/UPDATE/REPLACE/REMOVE/UPSERT) | 500-700 LOC parser + 1000-1500 LOC executor | 12-15 weeks | 🔴 Blocker | A |
| **DDL** (CREATE/DROP COLLECTION/INDEX/VIEW) | 300-400 LOC parser + 800-1000 LOC executor | 4-6 weeks | 🟠 After Mutations | B |
| **Geospatial** (ST_Distance, ST_Contains, ST_Within, ST_DWithin) | **200-300 LOC parser integration** (functions exist) | **2-3 weeks** (revised) | 🟢 Parallel | C |
| **Full-Text Search** (SEARCH syntax + FTS operators) | 300-400 LOC parser + 600-800 LOC executor | 2-3 weeks | 🟢 Parallel | B |
| **Integration & Testing** (cross-feature tests, perf bench) | 2000+ LOC tests + benchmarks | 3-4 weeks | 🟡 After all | A+B+C |

**Total**: ~4500-5500 lines of production code + ~3000 lines of tests  
**Revision Note**: Geospatial effort -15% due to function library already implemented; DDL effort unchanged; Mutations effort unchanged; FTS effort unchanged

---

## 🎯 Feature Breakdown

### 1. MUTATIONS (INSERT/UPDATE/REPLACE/REMOVE/UPSERT)

**Status**: 📋 Roadmap created (AQL_MUTATIONS_ROADMAP.md)

**Phases**:
- Phase 1: Parser & Tokenizer (2-3 weeks)
- Phase 2: Safety & Validation (1-2 weeks)
- Phase 3: Translation & Execution (3-4 weeks)
- Phase 4: Transaction Integration (2-3 weeks)
- Phase 5: Testing & Documentation (2-3 weeks)

**Files to Create**:
- `src/query/mutation_executor.cpp` (executor)
- `src/query/aql_mutation_validator.cpp` (validation)
- `tests/aql/test_aql_mutations_*.cpp` (7 test suites)
- `docs/de/aql/aql_mutations_guide.md` (user guide)

**Key Dependencies**: Transaction support (BEGIN/COMMIT) — **already implemented 2026-06-18** ✅

---

### 2. DDL (CREATE/DROP COLLECTION/INDEX/VIEW)

**Status**: 🔴 NOT STARTED (requires Mutations parser foundation)

**Phases**:
- Phase 1: Parser Extension (1-2 weeks)
  - Add DDL tokens: CREATE, DROP, COLLECTION, INDEX, VIEW, UNIQUE, SPARSE, GEO, FULLTEXT, VECTOR
  - Extend ASTNodeType enum: CreateCollectionNode, CreateIndexNode, CreateViewNode, DropNode
  - Implement parseCreateCollection(), parseCreateIndex(), parseCreateView(), parseDrop()
  - AST structure for index types (HASH, SKIPLIST, GEO, VECTOR, FULLTEXT)
  
- Phase 2: Metadata & Catalog (1-2 weeks)
  - Extend CatalogManager with DDL execution
  - Create index registry
  - Collection metadata persistence

- Phase 3: Execution & Validation (1-2 weeks)
  - DDLExecutor class for creation/deletion
  - Schema validation
  - Conflict detection (collection exists, index duplicate, etc.)

- Phase 4: Testing & Documentation (1-2 weeks)

**Files to Create**:
- `src/query/ddl_executor.cpp` (executor)
- `src/query/ddl_validator.cpp` (validation)
- `tests/aql/test_aql_ddl_*.cpp` (4 test suites)
- `docs/de/aql/aql_ddl_guide.md` (user guide)

**Key Dependencies**: 
- ✅ Mutations parser (Phase 1) — provides foundation
- ✅ CatalogManager (exists)

**Parallelization**: Can start 1-2 weeks after Mutations Phase 1 ✅

---

### 3. GEOSPATIAL (ST_Distance, ST_Contains, ST_Within, ST_DWithin)

**Status**: 🔄 **In Progress** — Phase 1 (Parser wiring + tests, 2026-07-27)

**Current** (2026-06-18 audit):
- ✅ **Functions implemented** in `src/query/let_evaluator.cpp`:
  - ST_Point, ST_AsGeoJSON, ST_Distance (Haversine formula), ST_Intersects, ST_Within, ST_Contains, ST_GeomFromGeoJSON
  - Already callable as LET expressions: `LET dist = ST_Distance(...)`
- ❌ **NOT wired into AQL parser**:
  - Cannot use as FILTER predicates: `FILTER ST_Within(doc.location, polygon)` → not recognized
  - No query optimizer integration (spatial index selection)
  - No batch evaluation hints

**Effort Impact**: Roadmap can focus on **Integration + Optimization** instead of reimplementation

**Phases** (revised - leverage existing functions):
- ✅ Phase 1: Parser Integration — COMPLETE (2026-07-27)
  - Confirmed: ST_* functions wire into FILTER/SORT/RETURN via generic IDENTIFIER() function-call path
  - No parser tokenizer/grammar changes required — parser already handles all function call forms
  - `qe_evalFunction` in `query_engine.cpp` already handles all ST_* at runtime (lines 1676–2330)
  - LetEvaluator::evaluateExpression handles ST_* in LET and FILTER contexts
  - Added 27 production tests in `tests/aql/test_aql_st_predicates.cpp`:
    - 7 parser acceptance tests (FILTER context, incl. nested ST_GeomFromGeoJSON)
    - 3 parser context tests (SORT, RETURN, LET+FILTER combined)
    - 12 evaluation tests (ST_Distance, ST_Within, ST_Contains, ST_Intersects, ST_DWithin, ST_GeomFromGeoJSON)
    - 5 parse-then-evaluate integration tests

- Phase 2: Query Optimization (1 week)
  - Optimizer recognition of spatial predicates (e.g., `FILTER ST_Within(...)`)
  - Spatial index selection hints
  - Plan caching for repeated queries

- Phase 3: Performance Hardening (1 week)
  - Optional GPU acceleration (CUDA kernels for batch distance matrix)
  - Vectorized polygon containment (batch 1000+ geometries)
  - Query caching + early termination

- Phase 4: Testing & Benchmarks (1-2 weeks)
  - Performance validation vs. PostGIS baseline
  - Accuracy tests for edge cases (antimeridian, poles, etc.)
  - Load testing (100K+ geometries)

**Files to Create**:
- `src/geospatial/geo_kernel.cpp` (CUDA + CPU kernels)
- `src/geospatial/geo_validator.cpp` (coordinate validation)
- `src/query/geo_query_executor.cpp` (query layer)
- `tests/geospatial/test_geo_*.cpp` (5+ test suites)
- `benchmarks/benchmark_geo_queries.cpp` (perf validation)
- `docs/de/aql/aql_geospatial_guide.md` (user guide)

**Key Dependencies**: 
- ✅ Query parser (can add ST_* functions without parser changes)
- ⚠️ Geospatial index (RocksDB prefix compression for spatial data)

**Parallelization**: Can start **immediately in parallel** with Mutations ✅  
**Go/No-Go Gate**: GPU kernel performance must achieve >= 50x speedup vs. CPU on RTX-class GPU

---

### 4. FULL-TEXT SEARCH (SEARCH + FTS Operators)

**Status**: 🟡 PARTIAL (BasicTextIndex exists, but no SEARCH syntax)

**Current**: CONTAINS() function works, but no FTS-specific syntax

**Phases**:
- Phase 1: Parser Extension (1 week)
  - Add SEARCH token
  - Extend FOR syntax: `FOR doc IN collection SEARCH doc.text IN "query_string"`
  - Optional operators: WITH (options), LIMIT, SCORE()

- Phase 2: FTS Engine Enhancement (1 week)
  - Extend BasicTextIndex with phrase queries
  - Add field boosting (field_name^weight)
  - Boolean operators (AND, OR, NOT in FTS context)
  - Wildcard support (term*, *term, term*)

- Phase 3: Query Optimization (1 week)
  - FTS index selection in optimizer
  - Query plan pruning for text-heavy predicates
  - Result ranking by relevance score

- Phase 4: Testing & Documentation (1 week)

**Files to Create**:
- `src/query/fts_query_executor.cpp` (query layer)
- `src/index/fulltext_query_parser.cpp` (FTS syntax parser)
- `tests/aql/test_aql_fts_*.cpp` (3 test suites)
- `docs/de/aql/aql_fts_guide.md` (user guide)

**Key Dependencies**: 
- ✅ BasicTextIndex (already exists)
- ⚠️ Query optimizer (may need minor extension)

**Parallelization**: Can start **immediately in parallel** with Mutations ✅

---

## 📊 Timeline & Critical Path

```
                    Week 1-2           Week 3-4           Week 5-6           Week 7-8           Week 9-10
                    --------           --------           --------           --------           ---------

MUTATIONS:
Phase 1 (Parser)    [████████]
Phase 2 (Safety)               [████]
Phase 3 (Executor)                    [████████████]
Phase 4 (Txn)                                          [████████]
Phase 5 (Testing)                                                 [████████]

DDL:
Phase 1 (Parser)                      [████]  ← (waits for Mutations Phase 1)
Phase 2 (Catalog)                          [████]
Phase 3 (Execution)                           [████]
Phase 4 (Testing)                               [████]

GEOSPATIAL:      [PARALLEL - starts immediately]
Phase 1-2         [████████]
Phase 3-4                  [████████]
Phase 5                            [████████]

FTS:             [PARALLEL - starts immediately]
Phase 1-2         [██████]
Phase 3-4              [██████]
Phase 4-5                  [████]

INTEGRATION & TESTING:                                                                   [████████████████]
Cross-feature tests, benchmarks, performance validation

─────────────────────────────────────────────────────────────────────────────────────────────────────
Week 1         Week 5         Week 10         Week 15         Week 20         Week 25
Q3 2026        Q3 2026        Q4 2026         Q4 2026         Q4 2026         Q4 2026
                                                                               ↑
                                                                         v2.0.0 Release Target
```

**Critical Path** (determines release date):
1. Mutations Phases 1-5 (12-15 weeks)
2. DDL Phases 1-4 (4-6 weeks, sequential after Mutations Phase 1)
3. Integration Testing (3-4 weeks)

**Total**: ~18-23 weeks (4-5.5 calendar months) — **revised down 2 weeks due to geospatial reuse**

**Fast-Track Optimizations**:
- Geospatial & FTS can run 100% in parallel (no blocking)
- Mutations Phase 3-4 parallelizable with Phase 2
- Integration testing starts Week 15 (partial, incremental)

---

## 👥 Team Assignments

### **Team A: Mutations + DDL (Core Language)**
- **Primary**: Parser extensions, transaction integration
- **Weeks**: 0-20 (sequential: Mutations 0-12, DDL 8-20)
- **People**: 2-3 engineers
- **Key Skills**: Parser design, AST manipulation, RocksDB integration, transaction semantics

### **Team B: Geospatial (Spatial Queries) — REVISED**
- **Primary**: Parser integration, query optimization, performance hardening
- **Weeks**: 0-8 (parallel, **2 weeks shorter due to function reuse**)
- **People**: 1 engineer (can overlap with Team A in weeks 8+)
- **Key Skills**: Query parser, optimizer, optional CUDA optimization
- **Note**: Existing ST_* functions in let_evaluator.cpp are production-ready; focus on query integration + perf

### **Team C: Full-Text Search (Text Queries)**
- **Primary**: FTS engine, query parser
- **Weeks**: 0-9 (parallel)
- **People**: 1 engineer
- **Key Skills**: Information retrieval, text indexing, query parsing

### **Integration Team (overlaps with all)**
- **Primary**: Cross-feature testing, performance benchmarks, documentation
- **Weeks**: 12-25 (final 8-10 weeks)
- **People**: 1-2 engineers (could be from Teams A/B/C rotating)
- **Key Skills**: Test automation, performance profiling, doc writing

**Total Headcount**: 5-7 engineers (with parallelization: 3 concurrent, 2 sequential)

---

## � Codebase Audit Findings (2026-06-18)

**Disclaimer**: This roadmap is based on comprehensive source code audit. Key findings that reduce scope/effort:

### **Finding 1: Geospatial Functions Already Implemented** 🟢
- **Location**: `src/query/let_evaluator.cpp`
- **Functions**: ST_Point, ST_AsGeoJSON, ST_Distance (Haversine), ST_Intersects, ST_Within, ST_Contains, ST_GeomFromGeoJSON
- **Status**: 🟢 Functional and tested
- **What's Missing**: AQL parser integration (currently only usable in LET expressions)
- **Impact**: Geospatial roadmap effort **-40%** (2 weeks → 4-5 weeks total, focused on integration + optimization)
- **Action**: Phase 1 focuses on **wiring** existing functions into parser, not reimplementation

### **Finding 2: SQL DML Parser Exists** 🟡
- **Location**: `src/query/sql_parser.cpp`
- **Coverage**: parseInsert(), parseUpdate(), parseDelete() with full statement parsing
- **Purpose**: SQL → AQL transpilation (legacy importer compatibility)
- **Status**: 🟢 Production-ready for SQL compatibility
- **Caveat**: NOT integrated into native AQL mutations; uses separate tokenizer
- **Impact**: Can reference SQL parser design + testing patterns for AQL DML roadmap
- **Action**: Use sql_parser.cpp as reference architecture (patterns for AST construction, error handling, etc.)

### **Finding 3: Transactions Foundation Exists** 🟢
- **Location**: `src/query/aql_runner.cpp` (added 2026-06-18)
- **Status**: BEGIN/COMMIT/ROLLBACK tokenization + multi-statement execution
- **Completeness**: 🟡 Multi-statement batching works, but mutations not yet enabled
- **Impact**: Mutations Phase 4 (Transaction Integration) is **NOT a blocker**—foundation ready
- **Action**: Phase 4 focuses on **mutation semantics** (atomicity checks, rollback logic), not transaction engine

### **Finding 4: Documentation-Implementation Gap**
- **Status**: Documented features (aql/README.md, docs/de/aql/) describe DML/DDL/Geospatial but **not implemented in parser**
- **Root Cause**: Deliberate read-only architecture decision (v1.x focus)
- **Resolution**: v2.0.0 roadmap closes this gap intentionally
- **No Action Needed**: This is expected and planned

### **Revised Effort Estimates (post-audit)**
| Feature | Original Estimate | Audit Findings | Revised Estimate | Reduction |
|---------|-------------------|-----------------|------------------|-----------|
| Mutations | 12-15 weeks | Uses reference from sql_parser.cpp | 12-15 weeks (same) | 0% |
| DDL | 4-6 weeks | Parallel effort, no blockers | 4-6 weeks (same) | 0% |
| Geospatial | 3-4 weeks | 70% functions exist | **2-3 weeks** | -40% ✅ |
| FTS | 2-3 weeks | Index exists, parser add-on | 2-3 weeks (same) | 0% |
| Integration | 3-4 weeks | Cross-feature tests | 3-4 weeks (same) | 0% |
| **TOTAL** | **20-25 weeks** | Geospatial 2 weeks saved | **18-23 weeks** | **-8-15% ✅** |

**Bottom Line**: v2.0.0 can be delivered **2 weeks earlier** due to existing geospatial implementation. Revised target: **October 2026 (Q4 early)** instead of November 2026.

---

## �🔄 Dependencies & Go/No-Go Gates

### **Gate 1: Mutations Phase 1 Complete** (Week 2-3)
- ✅ TokenType enum extended
- ✅ Parser recognizes INSERT/UPDATE/DELETE/REPLACE/REMOVE/UPSERT
- ✅ MutationNode AST types defined
- ✅ 50+ parser tests PASS
- **Blocker Trigger**: If 10+ parser tests FAIL → schedule architecture review
- **Go**: Proceed to Phase 2, unblock DDL Phase 1

### **Gate 2: Mutations Phase 3 Complete** (Week 6-8)
- ✅ MutationExecutor integrated with RocksDB
- ✅ Index update pipeline working
- ✅ Single-document mutations PASS
- ✅ Performance: Insert <= 10ms, Update <= 15ms, Delete <= 8ms per document
- **Blocker Trigger**: If perf > 50ms → investigate lock contention
- **Go**: Proceed to Phase 4, unblock DDL Phase 2

### **Gate 3: Transaction Integration** (Week 10-11)
- ✅ Multi-statement batches atomic (all-or-nothing)
- ✅ Rollback semantics verified
- ✅ WAL recovery validated
- ✅ 100+ transaction scenario tests PASS
- **Blocker Trigger**: If rollback loses data → investigate WAL implementation
- **Go**: Release v2.0.0-beta

### **Gate 4: Geospatial Performance** (Week 8-9)
- ✅ ST_Distance: >= 50x speedup vs. naive O(n²) on 100K geometries
- ✅ ST_Contains: polygon containment <= 1ms per point batch (1000 points)
- ✅ Memory: < 500MB for 1M point dataset
- **No-Go Trigger**: If speedup < 10x or memory > 2GB → consider simplified approach
- **Go**: Include in v2.0.0, else defer to v2.0.1

### **Gate 5: FTS Performance** (Week 8-9)
- ✅ Phrase query: <= 100ms for 100K documents
- ✅ Boolean operators: >= 95% relevance accuracy vs. Elasticsearch
- ✅ Index size: < 150% of source document size
- **No-Go Trigger**: If index size > 200% → consider compression strategy
- **Go**: Include in v2.0.0

### **Gate 6: Integration Testing** (Week 20-22)
- ✅ 1000+ cross-feature tests PASS
- ✅ Performance regression: < 5% vs. read-only baseline
- ✅ Security audit: 0 CVEs in new code
- ✅ Documentation: 100% API coverage
- **Blocker Trigger**: If regression > 10% → investigate executor bottleneck
- **Go/Final**: Release v2.0.0

---

## 📈 Success Metrics (v2.0.0)

### **Completeness**
- ✅ 100% ArangoDB AQL feature parity (mutations, DDL, geospatial query support)
- ✅ 95%+ test coverage for all 4 features
- ✅ 0 known blockers in mutation/transaction/spatial paths

### **Performance**
- ✅ Insert performance: <= 10ms/document (batch) or <= 50ms (single)
- ✅ Query performance: <= 5% regression vs. v1.3.0 read-only
- ✅ Geospatial speedup: >= 50x on GPU hardware (CUDA)
- ✅ FTS performance: >= 95% parity with Elasticsearch on phrase queries

### **Reliability**
- ✅ Transaction atomicity: 0 data loss, 0 orphaned documents in 10K scenario tests
- ✅ Rollback correctness: 100% recovery to pre-transaction state
- ✅ WAL durability: Crash safety verified via fault-injection tests
- ✅ Concurrent mutation safety: 0 race conditions in parallel stress tests

### **Documentation**
- ✅ User guides for all 4 features (Mutations, DDL, Geospatial, FTS)
- ✅ API reference with 50+ examples
- ✅ Migration guide from v1.3.0 (read-only) to v2.0.0 (with mutations)
- ✅ Performance tuning guide

### **Production Readiness**
- ✅ Security review: 0 CVEs, no SQL injection vectors
- ✅ Load testing: 1000+ concurrent users, 10K mutations/second
- ✅ Chaos engineering: 99.5% uptime under random node failures
- ✅ Changelog: Comprehensive entry documenting all v2.0.0 features + breaking changes (none)

---

## 🚨 Known Risks & Mitigation

| Risk | Impact | Probability | Mitigation |
|------|--------|-------------|-----------|
| **Mutations lock contention** | Performance regression (50%+ slower) | 🟠 MEDIUM | Separate index structures, fine-grained locking, early benchmarking (Week 8) |
| **Geospatial GPU memory overflow** | OOM on large datasets (>100M points) | 🟠 MEDIUM | Streaming/batching algorithms, early profiling (Week 6) |
| **FTS index size explosion** | Storage cost 300%+ of source | 🟡 LOW | Compression (zstd), early indexing perf test (Week 5) |
| **Rollback WAL corruption** | Data loss on crash during rollback | 🔴 CRITICAL | Extensive crash-recovery testing, fsync on critical boundaries (Week 12) |
| **Cross-feature interaction bugs** | Late discovery of DDL+Mutations conflicts | 🟠 MEDIUM | Integration testing starts Week 15, incremental merges (not Big Bang Week 22) |
| **Team resource contention** | Delayed geospatial/FTS if mutation work overruns | 🟡 LOW | Hire contractors for geospatial if weeks 8-10 slip (contingency) |

---

## 📋 Backlog of Roadmaps to Create

### **Existing** ✅
- [x] `src/query/AQL_MUTATIONS_ROADMAP.md` (created 2026-06-18)

### **To Create** (This Week)
- [x] `src/query/AQL_DDL_ROADMAP.md` — CREATED 2026-07-27 (DDL Phases 1-4 complete, delivered 2026-07-22)
- [x] `src/query/AQL_GEOSPATIAL_ROADMAP.md` — CREATED 2026-07-27 (note: module at src/query/, not src/geospatial/)
- [x] `src/index/AQL_FTS_ROADMAP.md` — CREATED 2026-07-27 (was `AQL_FULLTEXT_ROADMAP.md`)
- [ ] `src/query/AQL_V2_0_0_INTEGRATION_ROADMAP.md` (testing, perf validation, docs)

### **Reference Documents** (Update existing)
- [ ] `ROADMAP.md` (root): Link to AQL v2.0.0 master roadmap
- [ ] `FUTURE_ENHANCEMENTS.md` (root): Update AQL mutation/DDL/geospatial status
- [ ] `src/query/ROADMAP.md` (module): Reference v2.0.0 feature phases

---

## 🎯 Recommended Next Steps

1. **Week 1 (This Week)**: 
   - ✅ Review AQL_MUTATIONS_ROADMAP.md
   - ✅ Approve v2.0.0 scope (Mutations + DDL + Geospatial + FTS)
   - [x] Create detailed DDL, Geospatial, FTS roadmaps — DONE 2026-07-27
   - [ ] Assign team leads (Team A, B, C)
   - [ ] Schedule Gate 1 review (Week 3)

2. **Week 2-3 (Start of Q3)**:
   - [ ] Mutations Phase 1 (parser) — Team A
   - [ ] Geospatial Phase 1 (geometry lib) — Team B (parallel)
   - [ ] FTS Phase 1 (parser) — Team C (parallel)

3. **Week 8-9 (Mid Q3)**:
   - [ ] Gate 2 & 4 & 5 reviews
   - [ ] DDL Phase 1 starts (Team A, after Mutations Phase 1)
   - [ ] Mutation Phase 3 executor mid-testing

4. **Week 15-16 (Late Q3)**:
   - [ ] Integration testing starts (Team A+B+C together)
   - [ ] Performance regression testing
   - [ ] Documentation consolidation

5. **Week 22-24 (Early Q4)**:
   - [ ] Gate 6 final review
   - [ ] v2.0.0 release candidate build
   - [ ] Changelog + migration guide finalized

6. **Week 25 (November 2026)**:
   - [ ] **v2.0.0 General Availability (GA) Release** 🎉

---

## 📚 Reference

- **Mutations Roadmap**: `src/query/AQL_MUTATIONS_ROADMAP.md`
- **ArangoDB AQL Spec**: https://www.arangodb.com/docs/stable/aql/ (reference)
- **ThemisDB Architecture**: `ARCHITECTURE.md`, `docs/de/query_engine.md`
- **Test Strategy**: `tests/aql/INTEGRATION_PLAN.md` (to be created)
- **Performance Baseline**: Run `benchmarks/benchmark_aql_v1.3.0.cpp` before v2.0.0 changes

---

**Status**: PLANNING PHASE (2026-06-18)  
**Next Review**: Week 2-3 (after Gate 1 prep) | **Target**: v2.0.0 GA (November 2026)  
**Owner**: @copilot-aql-team | **Approver**: @architecture-board  
