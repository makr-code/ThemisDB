# AQL FTS (Full-Text Search) Implementation Roadmap

<!-- Status: [ ] open | [~] in progress | [x] done | [I] issue | [P] PR | [?] blocked -->

**Status:** 📋 PLANNED — Phases 1–4 pending (Target: Q3–Q4 2026)
**Target Release:** v2.0.0 (Q4 2026)
**Owner:** query module / Team B/C
**Last Updated:** 2026-07-27

---

## Executive Summary

Extend AQL with a native **SEARCH** syntax that wraps the existing `InvertedIndex::search()`,
`InvertedIndex::searchPhrase()`, and `InvertedIndex::searchFuzzy()` back-ends. The index
infrastructure is already production-ready (`include/index/inverted_index.h`); this roadmap
covers only the AQL-layer integration.

**Goal:** `FOR doc IN collection SEARCH PHRASE(doc.content, "machine learning") RETURN doc`
parses and executes within ≤ 100 ms on 100 000 documents (Gate 5).

---

## Current State

| Feature | Status | Evidence |
|---------|--------|----------|
| `InvertedIndex::search()` — BM25-scored keyword search | ✅ Production | `include/index/inverted_index.h:searchPhrase`, `src/index/inverted_index.cpp` |
| `InvertedIndex::searchPhrase()` — exact phrase matching | ✅ Production | `include/index/inverted_index.h:170-178` |
| `InvertedIndex::searchFuzzy()` — fuzzy matching | ✅ Production | `include/index/inverted_index.h` |
| `BM25(doc)` AQL function | ✅ Production | `tests/aql/test_aql_bm25.cpp` + parser |
| FULLTEXT() AQL function | ✅ Production | `tests/aql/test_aql_fulltext_hybrid.cpp` |
| AQL SEARCH keyword / clause | ❌ Not Implemented | Keywords not wired in parser |
| PHRASE() AQL function in FILTER | ❌ Not Implemented | No parser node; searchPhrase() not callable from AQL |
| PROXIMITY() / NEAR() AQL function | ❌ Not Implemented | No parser node |
| Spatial + FTS combined queries | ❌ Not Implemented | Requires geospatial + FTS both wired |

---

## Architecture

### Current (BM25-Only Path)

```
AQL: FOR doc IN col FILTER BM25(doc) > threshold RETURN doc
  ↓
Parser: FunctionCallNode("BM25") → QueryEngine
  ↓
QueryEngine: calls InvertedIndex::search()
```

### Target (SEARCH clause + PHRASE/PROXIMITY)

```
AQL: FOR doc IN col SEARCH PHRASE(doc.content, "machine learning") RETURN doc
  ↓
Parser: SearchClauseNode → PHRASE(FunctionCallNode) → FtsPredicateNode
  ↓
Translator: FtsExecutionPlan → InvertedIndex::searchPhrase()
  ↓
QueryEngine: executes FTS plan, returns scored results
```

---

## Implementation Phases

### Phase 1: Parser Integration (1–2 weeks) — [ ] PLANNED (Target: Q3 2026)

**Goal:** AQL SEARCH keyword + PHRASE()/NEAR()/STARTS_WITH() function family parse correctly.

- [ ] Add `SEARCH` token to `TokenType` enum in `include/query/aql_parser.h` (Target: Q3 2026)
- [ ] Add `SearchClauseNode` to `ASTNodeType` enum (Target: Q3 2026)
- [ ] Add `PHRASE`, `NEAR`, `STARTS_WITH`, `BOOST`, `ANALYZER` function tokens (Target: Q3 2026)
- [ ] Implement `parseSearchClause()` in `src/query/aql_parser.cpp` (Target: Q3 2026)
- [ ] Define `FtsPredicateNode` struct with: `field`, `term`, `type` (phrase/proximity/prefix), `boost`, `analyzer` (Target: Q3 2026)
- [ ] Register SEARCH as optional clause after FOR…IN (alongside FILTER, SORT, LIMIT) (Target: Q3 2026)

**Acceptance:** SEARCH clause with PHRASE/NEAR/STARTS_WITH parses without errors; AST contains `SearchClauseNode`.

### Phase 2: Translator Wiring (1–2 weeks) — [ ] PLANNED (Target: Q3 2026)

**Goal:** `SearchClauseNode` translates to an `FtsExecutionPlan` that calls the correct `InvertedIndex` method.

- [ ] Add `FtsExecutionPlan` to `include/query/aql_translator.h` (Target: Q3 2026)
- [ ] Implement `translateSearchClause()` in `src/query/aql_translator.cpp` (Target: Q3 2026)
  - `PHRASE(field, text)` → `InvertedIndex::searchPhrase(table, column, phrase, limit)`
  - `NEAR(field, text, dist)` → `InvertedIndex::searchFuzzy()` with distance cap
  - `STARTS_WITH(field, prefix)` → `InvertedIndex::search()` with prefix flag
- [ ] Integrate `FtsExecutionPlan` into `QueryEngine::execute()` dispatch (Target: Q3 2026)
- [ ] Preserve BM25 fallback when no SEARCH clause (no regression) (Target: Q3 2026)

**Acceptance:** `FOR doc IN col SEARCH PHRASE(doc.content, "machine learning") RETURN doc` executes end-to-end.

### Phase 3: Performance Hardening (1 week) — [ ] PLANNED (Target: Q3–Q4 2026)

**Goal:** Gate 5 — phrase queries ≤ 100 ms on 100 000 documents.

- [ ] Benchmark `InvertedIndex::searchPhrase()` at 100K document scale (Target: Q3 2026)
  - Inputs: 100 000 documents, 2-gram phrases, concurrent 10 requests
  - Target: p99 ≤ 100 ms at 10 concurrent queries
- [ ] Add plan-caching entry point for repeated SEARCH queries (Target: Q4 2026)
- [ ] ANALYZER hint: route queries to configured tokeniser (simple, snowball, language-specific) (Target: Q4 2026)
- [ ] Add `fts_search_duration_ms` Prometheus histogram (Target: Q4 2026)

**Acceptance (Gate 5):** Benchmark confirms phrase p99 ≤ 100 ms at 100 000 documents.

### Phase 4: Testing & Documentation (1 week) — [ ] PLANNED (Target: Q4 2026)

- [ ] `tests/aql/test_aql_fts_search_clause.cpp` — 25+ tests (Target: Q4 2026)
  - FTS-01..FTS-08: PHRASE parsing + execution correctness
  - FTS-09..FTS-16: NEAR/STARTS_WITH parsing + execution correctness
  - FTS-17..FTS-20: ANALYZER hint routing
  - FTS-21..FTS-25: error paths (unknown field, empty phrase, missing index)
- [ ] Perf test: `bench_aql_fts_phrase.cpp` — GATE-FTS-01 (p99 ≤ 100 ms / 100K docs) (Target: Q4 2026)
- [ ] Doxygen: `SearchClauseNode`, `FtsPredicateNode`, `FtsExecutionPlan` (Target: Q4 2026)
- [ ] User guide: `docs/de/aql/aql_fts_guide.md` (Target: Q4 2026)
- [ ] Update `AQL_V2_0_0_COMPLETE_ROADMAP.md` Gate 5 status (Target: Q4 2026)

---

## Performance Targets

| Metric | Target | Gate |
|--------|--------|------|
| PHRASE query p99 (100K docs, 10 concurrent) | ≤ 100 ms | GATE-FTS-01 (Gate 5 in AQL v2 roadmap) |
| NEAR query p99 (100K docs) | ≤ 150 ms | GATE-FTS-02 |
| STARTS_WITH p99 (100K docs) | ≤ 50 ms | GATE-FTS-03 |
| BM25 regression: no regression vs. baseline | Same as v1.x | Regression gate |

---

## Production Readiness Checklist

- [ ] SEARCH clause parses correctly for all supported functions
- [ ] PHRASE/NEAR/STARTS_WITH execute end-to-end via `InvertedIndex`
- [ ] Phrase p99 ≤ 100 ms at 100K documents (Gate 5)
- [ ] BM25 path unaffected: no regression in existing FULLTEXT / BM25 queries
- [ ] 25+ unit + integration tests passing
- [ ] Prometheus histogram wired
- [ ] Documentation complete (user guide + Doxygen)

---

## Known Limitations / Non-Scope for v2.0.0

- Multi-field SEARCH (across multiple document fields in one SEARCH clause) — deferred to v2.1.0
- Custom ANALYZER definitions via DDL — deferred (requires DDL extension)
- Relevance score injection into AQL RETURN (score as field) — deferred to v2.1.0
- Distributed FTS across shards — tracked in Track 2 Phase 3.3

---

## References

- [AQL_V2_0_0_COMPLETE_ROADMAP.md](./AQL_V2_0_0_COMPLETE_ROADMAP.md) — Master v2.0.0 language standard roadmap (Gate 5)
- `include/index/inverted_index.h` — Production FTS backend (searchPhrase, search, searchFuzzy)
- `tests/aql/test_aql_bm25.cpp` — Existing BM25 integration tests
- `tests/aql/test_aql_fulltext_hybrid.cpp` — Existing FULLTEXT integration tests
