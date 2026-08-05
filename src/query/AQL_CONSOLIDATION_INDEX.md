# AQL Feature Consolidation Index
## Cross-Module Documentation Reference Guide

**Status**: Phase 6 Consolidation (2026-08-05)  
**Purpose**: Single source of truth for each AQL v2.0.0 feature across src/query/ and src/aql/ modules

---

## Feature Roadmap Mapping

### 1. Mutations (INSERT, UPDATE, REPLACE, REMOVE, UPSERT)

| Aspect | Source of Truth | Cross-References |
|--------|-----------------|------------------|
| **Roadmap** | src/query/AQL_MUTATIONS_ROADMAP.md | src/aql/ROADMAP.md (reference) |
| **Status** | ✅ COMPLETE (2026-07-15, Phases 1-5) | All phases delivered |
| **Parser** | src/query/aql_parser.cpp | aql_mutation_translator.cpp, aql_mutation_validator.cpp |
| **Executor** | src/query/mutation_executor.cpp | Transaction coordinator integration |
| **Tests** | tests/query/test_aql_ddl_phase2.cpp (32+ cases) | Integration tests (40+ additional) |
| **Migration Guide** | (Pending Q4 2026) | Link in AQL_MUTATIONS_ROADMAP.md §Migration |

**Key Decision**: Mutations feature-specific roadmap is canonical. src/aql/ROADMAP.md references this for unified rollout status.

---

### 2. DDL (CREATE/DROP COLLECTION/INDEX/VIEW)

| Aspect | Source of Truth | Cross-References |
|--------|-----------------|------------------|
| **Roadmap** | src/query/AQL_DDL_ROADMAP.md | src/aql/ROADMAP.md (reference) |
| **Status** | ✅ COMPLETE (2026-07-22, Phase 1) | Parser + Executor + 32 tests |
| **Parser** | src/query/aql_parser.cpp (DDL branch) | ddl_executor.cpp |
| **Executor** | src/query/ddl_executor.cpp | Schema manager integration |
| **Tests** | tests/query/test_aql_ddl_phase2.cpp | Comprehensive coverage (CREATE/DROP all types) |
| **Phase 2** | (Pending Q3 2026) | Optimizer hints for DDL statements |

**Key Decision**: DDL feature-specific roadmap is canonical. Phase 2 (optimizer hints) will coordinate with Phase 2 Optimizer hardening work.

---

### 3. Geospatial (ST_Distance, ST_Contains, ST_Intersects, etc.)

| Aspect | Source of Truth | Cross-References |
|--------|-----------------|------------------|
| **Roadmap** | src/query/AQL_GEOSPATIAL_ROADMAP.md | src/aql/ROADMAP.md (reference) |
| **Status** | ✅ PHASE 1 COMPLETE (2026-07-27) | ST_* functions working in FILTER/SORT/RETURN |
| **Parser** | src/query/aql_parser.cpp (geospatial) | Geospatial function parsing |
| **Executor** | src/query/query_engine.cpp::qe_evalFunction | ST_* function implementations |
| **Tests** | tests/query/test_aql_st_predicates.cpp (26 cases) | Phase 1 validation complete |
| **Phase 2** | 📋 PENDING (Target: Q3 2026) | Optimizer hints for spatial index selection |
| **Phase 3** | 📋 PENDING (Q4 2026) | Advanced spatial operations (buffer, simplify, etc.) |

**Key Decision**: Geospatial feature-specific roadmap is canonical. Phase 2 implementation will be driven by Phase 2 Optimizer hardening agent.

---

### 4. Full-Text Search (FTS) Phrase & Proximity Queries

| Aspect | Source of Truth | Cross-References |
|--------|-----------------|------------------|
| **Roadmap** | src/query/AQL_V2_0_0_COMPLETE_ROADMAP.md §FTS | src/aql/ROADMAP.md (reference) |
| **Status** | 📋 PENDING (Target: Q3-Q4 2026) | Basic FTS working; phrase/proximity enhancement needed |
| **Parser** | src/query/aql_parser.cpp (FTS branch) | PHRASE(), PROXIMITY() syntax |
| **Executor** | src/query/query_engine.cpp + synopsis_store.cpp | FTS query evaluation |
| **Tests** | (To create) | 40+ test cases (20 phrase, 20 proximity) |
| **Performance Target** | ≤100ms on 100K documents | Benchmark in tests/query/test_aql_fts_phrase_proximity.cpp |

**Key Decision**: FTS will be tracked as part of AQL_V2_0_0_COMPLETE_ROADMAP.md. Phase 6 agent will implement phrase/proximity queries.

---

### 5. LLM Integration (Phases 1-3)

| Aspect | Source of Truth | Cross-References |
|--------|-----------------|------------------|
| **Roadmap** | src/query/ROADMAP.md line 22-44 | src/aql/ROADMAP.md (LLM orchestration coordination) |
| **Status** | ✅ COMPLETE (2026-08-05, Phases 1-3 documentation) | Phase 4 SLA tests pending |
| **Phase 1: Boundary** | src/query/AQL_LLM_INTEGRATION_CONTRACT.md | Parser validation contract definition |
| **Phase 2: Metrics** | src/query/AQL_LLM_INTEGRATION_PHASE3_METRICS.md | Comprehensive metrics interpretation |
| **Phase 3: Documentation** | src/query/AQL_LLM_INTEGRATION_PHASE3_PARSER_CHANGES.md | Parser enhancements explained |
| **API Contract** | src/query/AQL_LLM_INTEGRATION_PHASE3_API_CONTRACT.md | Public API definition for consumers |
| **Migration Guide** | docs/aql/AQL_LLM_INTEGRATION_MIGRATION_GUIDE.md | User-facing adoption guide |
| **Tests** | tests/query/test_aql_llm_integration.cpp (16 test cases) | Integration test suite (Phase 2) |
| **Performance Tests** | tests/query/test_aql_validation_performance.cpp (8 test cases) | SLA verification (Phase 4, pending build) |

**Key Decision**: LLM Integration documentation is canonical. Phase 1-3 complete; Phase 4 SLA tests in build verification.

---

### 6. Cross-Feature Integration Tests

| Aspect | Source of Truth | Cross-References |
|--------|-----------------|------------------|
| **Roadmap** | src/query/ROADMAP.md line 60 | src/aql/ROADMAP.md (master schedule) |
| **Status** | 📋 PENDING (Target: Q4 2026) | 1000+ tests, zero v1.x regressions |
| **Scope** | Comprehensive integration across all v2.0.0 features | Mutations + DDL + Geospatial + FTS + LLM |
| **Test Location** | tests/query/test_aql_integration_v2_0_0_suite.cpp | To be created |
| **Regression Baseline** | tests/query/test_query_*.cpp (existing 30+ suites) | Ensure no v1.x feature breakage |

**Key Decision**: Integration test roadmap consolidated in ROADMAP.md. Individual feature-specific tests in respective roadmaps.

---

## Cross-Module Dependency Matrix

### src/query/ → src/aql/ Dependencies

| Interface | Purpose | Linkage |
|-----------|---------|---------|
| AQLParser | Parse NL text to AQL | aql_parser_service.cpp calls validateAQLWithParser() |
| QueryOptimizer | Optimize LLM-generated AQL | Rewrite rules, cost model aware of LLM patterns |
| QueryEngine::execute() | Execute validated AQL | Unified entry point for LLM and user-generated queries |

### src/aql/ → src/query/ Dependencies

| Interface | Purpose | Linkage |
|-----------|---------|---------|
| LLMMetricsCollector | Track validation metrics | Integration contract (AQL_LLM_INTEGRATION_CONTRACT.md) |
| Parser Validation | Safety gates for LLM output | validateAQLWithParser() in llm_aql_handler.cpp:1553 |
| Fallback Executor | Guaranteed baseline execution | Query engine provides JIT fallback via QueryCompiler |

**No circular dependencies detected.** Dependency flow: aql/ → query/ is one-directional.

---

## Documentation Organization

### Canonical Roadmaps (by feature)
- **Mutations**: src/query/AQL_MUTATIONS_ROADMAP.md
- **DDL**: src/query/AQL_DDL_ROADMAP.md
- **Geospatial**: src/query/AQL_GEOSPATIAL_ROADMAP.md
- **v2.0.0 Complete**: src/query/AQL_V2_0_0_COMPLETE_ROADMAP.md
- **LLM Integration**: src/query/AQL_LLM_INTEGRATION_CONTRACT.md (Phase 1 boundary)
  - Phase 3 Documentation: 
    - src/query/AQL_LLM_INTEGRATION_PHASE3_PARSER_CHANGES.md
    - src/query/AQL_LLM_INTEGRATION_PHASE3_METRICS.md
    - src/query/AQL_LLM_INTEGRATION_PHASE3_API_CONTRACT.md
    - docs/aql/AQL_LLM_INTEGRATION_MIGRATION_GUIDE.md

### Module-Level Roadmaps (orchestration)
- **Query Module**: src/query/ROADMAP.md (master scheduling)
- **AQL Module**: src/aql/ROADMAP.md (references feature roadmaps)

### Architecture & Design
- **Query Architecture**: src/query/ARCHITECTURE.md (section: LLM Integration)
- **AQL Architecture**: src/aql/ARCHITECTURE.md (section: Query Engine Dependency)

---

## Scheduling Coordination

### Q3 2026 Priorities (Overlapping)

| Initiative | Owner | Timeline | Status | Blocking Factors |
|-----------|-------|----------|--------|-----------------|
| Phase 1: Parser Safety | query-phase-1-safety | Week 1-2 Q3 | ✅ COMPLETE 2026-06-18 | None |
| Phase 2: Optimizer Hardening | query-phase-2-optimizer | Week 1-3 Q3 | ✅ COMPLETE 2026-06-18 | None |
| Phase 3: Documentation | query-phase-3-documentation | Week 1-2 Q3 | ✅ COMPLETE 2026-08-05 | None |
| Geospatial Phase 2 (Optimizer Hints) | Phase 6 agent (part of Phase 2 work) | Week 2-4 Q3 | 📋 PENDING | Phase 2 Optimizer framework |
| Query Hardening Wave (Reliability) | Phase 6 agent | Week 1-4 Q3 | 🔄 IN PROGRESS | None |

### Q4 2026 Priorities

| Initiative | Owner | Timeline | Blocking Factors |
|-----------|-------|----------|-----------------|
| AQL LLM Phase 4 (SLA Tests) | Phase 6 agent | Week 1 Q4 | fmt library dependency (2 hrs) |
| Geospatial Phase 2 (Completion) | Phase 6 agent (if not complete) | Week 1-2 Q4 | Phase 2 Optimizer completion |
| FTS Phrase/Proximity | Phase 6 agent | Week 2-4 Q4 | None (independent feature) |
| Cross-Feature Integration (1000+ tests) | To schedule | Week 3-4 Q4 | All phase completions |

**Observation**: No critical blocking path identified. All features can progress in parallel with minimal dependencies.

---

## Consolidation Validation Checklist

- [x] Feature-specific roadmaps identified (6 roadmaps)
- [x] Canonical source of truth designated for each feature
- [x] Cross-references documented (bidirectional links)
- [x] No conflicting task scheduling detected
- [x] Dependency matrix complete (src/query/ ↔ src/aql/)
- [x] Q3-Q4 scheduling coordinated without blocking
- [x] Documentation locations centralized (this index)
- [x] Phase 3 (LLM Documentation) complete ✅ 2026-08-05
  - [x] Parser changes documented (AQL_LLM_INTEGRATION_PHASE3_PARSER_CHANGES.md)
  - [x] Metrics guide created (AQL_LLM_INTEGRATION_PHASE3_METRICS.md)
  - [x] API contract finalized (AQL_LLM_INTEGRATION_PHASE3_API_CONTRACT.md)
  - [x] Migration guide for users (AQL_LLM_INTEGRATION_MIGRATION_GUIDE.md)
  - [x] All cross-references verified (no broken links)

**Status**: ✅ Consolidation index complete. Phase 3 documentation complete. Ready for Phase 4 SLA tests and Phase 5 documentation orchestration.

---

## Next Steps

1. **Phase 5 (doc-orchestrator)**: 
   - Update src/aql/ROADMAP.md to reference this index and feature roadmaps
   - Consolidate duplicate definitions (if any found during index validation)
   - Create cross-module reference guide in docs/architecture/AQL_MODULE_INTEGRATION.md

2. **Phase 6 Agents**:
   - Geospatial Phase 2: Coordinate with Phase 2 Optimizer agent
   - FTS Phrase/Proximity: Begin implementation (independent)
   - Query Hardening: Begin reliability hardening (independent)

3. **Ongoing Maintenance**:
   - Update this index as features complete
   - Move completed items to CHANGELOG.md with evidence links
   - Maintain bidirectional cross-references

---

**Index Created**: 2026-08-05  
**Status**: Phase 6 Consolidation (concurrent with Phases 1-4)  
**Next Review**: When Phase 5 (doc-orchestrator) begins
