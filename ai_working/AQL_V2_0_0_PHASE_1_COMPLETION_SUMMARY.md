# AQL 2.0.0 Completion — Phase 1 Summary

**Completion Date**: 2026-08-06  
**Status**: ✅ COMPLETE & FROZEN  
**Target Release**: v2.0.0 (Q4 2026, October 2026)  
**Teams Ready**: 5-7 engineers across 4 parallel tracks

---

## Phase 1 Deliverables (Design / API Contracts)

All four frozen API contracts have been created as authoritative specifications for Phase 2-6 implementation:

### 1. ✅ MUTATIONS_API_CONTRACT.md (13.3 KB)

**Scope**: INSERT, UPDATE, REPLACE, REMOVE, UPSERT operations  
**Key Specifications**:
- AST Node Types: InsertNode, UpdateNode, ReplaceNode, RemoveNode, UpsertNode
- TokenType Extensions: INSERT, UPDATE, REPLACE, REMOVE, UPSERT, SET, VALUES, IGNORE, DUPLICATES
- Parser Methods: parseInsert(), parseUpdate(), parseReplace(), parseRemove(), parseUpsert()
- Error Codes: 400-level (validation), 500-level (runtime)
- Performance Targets: Insert ≤10ms, Update ≤15ms, Delete ≤8ms per doc
- Reference Implementation Pattern: Follows sql_parser.cpp recursive-descent style
- Integration Points: AQLParser, MutationExecutor, RocksDB storage, Transaction context, Index manager

**Test Requirements**: 50+ parser tests, 30+ validator tests, 50+ executor tests, 100+ transaction tests

---

### 2. ✅ DDL_API_CONTRACT.md (17.1 KB)

**Scope**: CREATE/DROP COLLECTION, INDEX, VIEW operations  
**Key Specifications**:
- AST Node Types: CreateCollectionNode, CreateIndexNode, CreateViewNode, DropCollectionNode, DropIndexNode, DropViewNode
- TokenType Extensions: CREATE, DROP, COLLECTION, INDEX, VIEW, TYPE, HASH, SKIPLIST, GEO, FULLTEXT, VECTOR, UNIQUE, SPARSE, WITH, IF, NOT, EXISTS
- Parser Methods: parseCreateCollection(), parseCreateIndex(), parseCreateView(), parseDrop*()
- CatalogManager Extensions: createCollection(), createIndex(), createView(), dropCollection(), dropIndex(), dropView(), getIndexes()
- Index Metadata Schema: RocksDB keys `__index_{collection}_{name}` with JSON metadata
- Error Codes: 400-level (not found, duplicate, invalid), 403-level (protected, in-use), 500-level (runtime)
- Performance Targets: CREATE COLLECTION ≤100ms, CREATE INDEX (1M docs) ≤30s, DROP ≤5s
- Transactionality: DDL operations are atomic (all-or-nothing)

**Test Requirements**: 30+ parser tests, 20+ catalog tests, 25+ executor tests, 50+ integration tests

---

### 3. ✅ GEOSPATIAL_INTEGRATION_CONTRACT.md (14.7 KB)

**Scope**: ST_* function integration into FILTER/SORT/RETURN query contexts  
**Key Specifications**:
- 14 Existing Functions (already implemented in let_evaluator.cpp):
  - Coordinate: ST_Point, ST_AsGeoJSON, ST_GeomFromGeoJSON
  - Distance: ST_Distance (Haversine), ST_DWithin
  - Containment: ST_Contains, ST_Within, ST_Intersects
  - Area/Boundary: ST_Area, ST_Boundary, ST_Centroid
  - Utility: ST_Length, ST_IsValid, ST_GeometryType
- No Parser Changes Required: Functions flow through existing qe_evalFunction() lookup path
- Phase 2 Enhancements:
  - GeoFunctionRegistry: register spatial functions, check if function is spatial
  - GeoQueryOptimizer: recognize spatial predicates, generate optimization hints
  - Index Selection: suggest GEO index when FILTER ST_Within(...) detected
- Performance Target: ST_Distance on 100K geometries in <100ms (≥50x speedup vs. naive O(n²))
- Error Handling: input validation, null propagation for invalid geometries, WGS84 coordinate validation

**Test Requirements**: 27 existing parser tests (PASS), 15+ optimizer tests, 30+ integration tests

---

### 4. ✅ FTS_EXTENSION_CONTRACT.md (14.3 KB)

**Scope**: Full-Text Search (SEARCH syntax) and advanced text operators  
**Key Specifications**:
- SEARCH Syntax: `FOR doc IN collection SEARCH doc.text IN "term"`
- SearchNode AST Types: SearchINNode, SearchBooleanNode, SearchPhraseNode
- TokenType Extensions: SEARCH, IN, AND, OR, NOT, WITH, BOOST, WILDCARD, STOPWORDS, SCORE, BM25, CASE_SENSITIVE
- Operators:
  - Phrase queries: "exact phrase" (all words in sequence)
  - Field boosting: (query)^2 (increase field weight)
  - Wildcards: term*, *term, t*rm (prefix/suffix/infix)
  - Boolean: AND, OR, NOT operators
  - Negation: !(condition)
  - WITH clause: scoring algorithm, stopwords, case sensitivity
- BasicTextIndex Extensions: searchPhrase(), searchWithBoosting(), searchWildcard(), searchWithScoring()
- BM25 Relevance Scoring: tunable parameters (k1=1.5, b=0.75, k2=100)
- Stopword Filtering: common English stopwords, language-aware
- Performance Target: phrase query on 100K docs in ≤100ms

**Test Requirements**: 20+ parser tests, 25+ index tests, 20+ executor tests, 40+ integration tests

---

## Phase 1 Approval Status

All 4 contracts are **FROZEN** (locked against further changes without explicit amendment approval).

**Approval Checklist**:
- [ ] Architecture Review: Tech Lead approval (AST design backward-compatible)
- [ ] Parser Lead approval (TokenType + parser methods follow existing patterns)
- [ ] Storage Lead approval (RocksDB integration, CatalogManager extension)
- [ ] Query Optimization Lead approval (optimizer hooks, index selection)
- [ ] Performance Lead approval (performance targets achievable)
- [ ] PM approval (team assignment, timeline, dependencies)

**Current Status**: PENDING HUMAN SIGN-OFF (can proceed to Phase 2 awaiting approval)

---

## Phase 2 Readiness (Next: Weeks 3-18)

### Track A: Mutations (Weeks 3-15, 12-week critical path)
- **Owner**: Team A (2-3 engineers)
- **Prerequisite**: Phase 1 contract frozen ✅
- **Deliverable**: Weeks 3-4 parser, weeks 5-6 validator, weeks 7-10 executor, weeks 11-13 transaction, weeks 14-15 hardening
- **Test Target**: 50+ parser, 30+ validator, 50+ executor, 100+ transaction tests

### Track B: DDL (Weeks 8-20, depends on Mutations Phase 1)
- **Owner**: Team A (same team, Weeks 8+ after Mutations Phase 1)
- **Prerequisite**: Mutations Phase 1 parser foundation complete
- **Deliverable**: Weeks 8-9 parser, weeks 10-11 catalog, weeks 12-13 executor, weeks 14-20 integration
- **Test Target**: 30+ parser, 20+ catalog, 25+ executor, 50+ integration tests

### Track C: Geospatial (Weeks 3-11, parallel, no dependencies)
- **Owner**: Team B (1 engineer)
- **Prerequisite**: Phase 1 contract frozen ✅
- **Deliverable**: Weeks 3-4 parser integration, weeks 5-6 optimizer, weeks 7-8 performance, weeks 9-11 testing/docs
- **Test Target**: 27 existing tests, 15+ optimizer, 30+ integration, >= 50x speedup gate

### Track D: FTS (Weeks 3-12, parallel, no dependencies)
- **Owner**: Team C (1 engineer)
- **Prerequisite**: Phase 1 contract frozen ✅
- **Deliverable**: Weeks 3-4 parser, weeks 5-6 index hardening, weeks 7-9 optimizer+scoring, weeks 10-12 testing/docs
- **Test Target**: 20+ parser, 25+ index, 20+ executor, 40+ integration tests

### Integration/QA (Weeks 16-25, overlaps with Phase 2 later stages)
- **Owner**: Team D-F (1-2 engineers rotating)
- **Deliverable**: Cross-feature tests, performance benchmarks, documentation

---

## Key Dependencies

```
Start: Week 1 (Phase 1 complete) ✅
   │
   ├─ MUTATIONS Phase 1 (Weeks 3-4)
   │  └─ UNBLOCKS: DDL Phase 1 (Week 8)
   │     └─ UNBLOCKS: Full production build test
   │
   ├─ GEOSPATIAL Phase 1 (Weeks 3-4) [PARALLEL]
   ├─ FTS Phase 1 (Weeks 3-4) [PARALLEL]
   │
   └─ Week 8: Mutations Phase 1 + DDL Phase 1 both start
      (3 parallel tracks running: Mutations+DDL, Geospatial, FTS)
      
End: Week 25 → v2.0.0 GA ready for release
```

---

## Success Metrics (Phase 1 ✅ → Phase 2-6 🔵)

### Phase 2 Exit Gates
- [ ] 300+ tests PASS (50 parser + 30 validator + 50 executor + 100 transaction across tracks)
- [ ] Zero CRITICAL scanner findings
- [ ] All 4 track deliverables complete

### Phase 3 Exit Gates
- [ ] 125+ error/edge case tests PASS
- [ ] Fallback paths tested (graceful degradation, retry logic)

### Phase 4 Exit Gates
- [ ] 1000+ total tests PASS (0 flakes on 3 consecutive runs)
- [ ] 95%+ code coverage on production code

### Phase 5 Exit Gates
- [ ] Performance gates met:
  - Mutations: Insert ≤10ms, Update ≤15ms, Delete ≤8ms per doc
  - DDL: CREATE INDEX (1M) ≤30s, DROP ≤5s
  - Geospatial: ST_Distance (100K) <100ms, ≥50x speedup
  - FTS: phrase query (100K) ≤100ms
- [ ] Benchmarks PASS (Wave 7 integration)
- [ ] CodeQL + sanitizer PASS

### Phase 6 Exit Gates
- [ ] All user guides complete (4 docs/de/aql/*.md files)
- [ ] Doxygen 100% on all new .cpp/.h files
- [ ] All ADRs approved
- [ ] ROADMAP/CHANGELOG/VERSIONING synced
- [ ] Human approval: v2.0.0 GA ready

---

## Timeline Summary

| Phase | Weeks | Status | Key Deliverable |
|-------|-------|--------|-----------------|
| **1** | 1-2 | ✅ COMPLETE | 4 Frozen API Contracts |
| **2** | 3-18 | 🔵 READY | Production Logic (no stubs) |
| **3** | 16-20 | ⏳ PENDING | Error Handling + Edge Cases |
| **4** | 16-22 | ⏳ PENDING | 1000+ Test Suite |
| **5** | 20-24 | ⏳ PENDING | Performance Gates |
| **6** | 23-25 | ⏳ PENDING | Docs + Acceptance Sign-Off |

**Total**: 25 weeks (late July 2026 → October 2026)  
**Target Release**: v2.0.0 GA (October 2026)

---

## Files Created

1. `/src/query/MUTATIONS_API_CONTRACT.md` — Mutations contract (13.3 KB)
2. `/src/query/DDL_API_CONTRACT.md` — DDL contract (17.1 KB)
3. `/src/query/GEOSPATIAL_INTEGRATION_CONTRACT.md` — Geospatial contract (14.7 KB)
4. `/src/query/FTS_EXTENSION_CONTRACT.md` — FTS contract (14.3 KB)

**Total Phase 1 Deliverable**: ~59 KB of frozen specifications

---

## Next Steps

### Immediate (Awaiting Approval)
1. **Tech Lead** reviews all 4 contracts for architecture compliance
2. **Parser Lead** confirms TokenType + parser methods follow existing patterns
3. **Storage/Query Leads** approve integration points
4. **PM** assigns teams and confirms timeline

### Upon Approval (Week 3)
1. **Team A** starts Mutations Phase 1 (parser + tokenizer)
2. **Team B** starts Geospatial Phase 1 (parser integration + optimizer hints)
3. **Team C** starts FTS Phase 1 (parser extension + SEARCH syntax)
4. **All teams** create GitHub issues + feature branches per contract specs

### Ongoing (Weeks 3-25)
1. Weekly sync: Fri 15:00 UTC (status, blockers, metrics)
2. Gate verification: end of each phase (build, test, perf, security, docs)
3. Evidence tracking: ai_working/phase2_evidence/, ai_working/phase3_evidence/, etc.

---

## Risks & Mitigations

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| Mutations parser too complex | Medium (30%) | High | Reference sql_parser.cpp design; architecture review early |
| DDL CatalogManager extension blocked by storage lead | Low (15%) | High | Pre-approval from storage lead before Phase 1 ends |
| Geospatial function reuse insufficient (reimplementation needed) | Low (10%) | Medium | Verify let_evaluator.cpp functions in Phase 2 week 1 |
| FTS BM25 scoring performance miss | Medium (25%) | Medium | Prototype BM25 scoring in Phase 2 week 5; optimize in Phase 5 |
| Team capacity / resource constraints | Low (10%) | High | Parallel track model allows 1-2 engineer per track |

---

## Document Governance

All 4 API contracts are **FROZEN** as of **2026-08-06**. Future amendments require:

1. **Requester** raises issue with specific contract section + reason
2. **Architecture Lead** reviews + approves/rejects amendment
3. **All affected team leads** sign off on change
4. **Amendment logged** in contract's "Amendments Log" section
5. **New version** communicated to all teams before next phase

**No amendments allowed during Phase 2-6 without this approval process.**

---

**Phase 1 Complete. Ready for Phase 2 kickoff.**

*Last Updated: 2026-08-06*  
*Status: ✅ FROZEN & READY FOR TEAM ASSIGNMENT*
