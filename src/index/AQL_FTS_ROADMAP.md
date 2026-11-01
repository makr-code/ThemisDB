---
Author: "ThemisDB Team"
Created: "2026-07-27"
Last Updated: "2026-10-31"
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# AQL FTS (Full-Text Search) Implementation Roadmap

<!-- Status: [ ] open | [~] in progress | [x] done | [I] issue | [P] PR | [?] blocked -->

**Status:** Parser work is present; translator/execution integration and end-to-end status require separate validation.
**Target Release:** v2.0.0 (Q4 2026)
**Owner:** query module / Team B/C
**Last Updated:** 2026-10-31

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
| `InvertedIndex` search interfaces | Present in module header/implementation; production readiness is not assessed here | `include/index/inverted_index.h`, `src/index/inverted_index.cpp` |
| `BM25(doc)` AQL function | ✅ Production | `tests/aql/test_aql_bm25.cpp` + parser |
| FULLTEXT() AQL function | ✅ Production | `tests/aql/test_aql_fulltext_hybrid.cpp` |
| AQL SEARCH keyword / clause | Parser and AST support present; execution wiring not confirmed | `include/query/aql_parser.h`, `src/query/aql_parser.cpp`; no `search_clause` translator/executor reference found |
| PHRASE / NEAR / STARTS_WITH predicates | Parser forms present; end-to-end execution not confirmed | `src/query/aql_parser.cpp`; translator has a separate `PHRASE()` function path |
| Spatial + FTS combined queries | ❌ Not Implemented | Requires geospatial + FTS both wired |

---

> **Status conflict to resolve:** Root `ROADMAP.md` states that FTS executor
> wiring, phrase/proximity matching, and the 100K-document benchmark gate were
> delivered (query Phase B). This scoped source check found parser structures
> and parsing, but did not find a use of `search_clause` outside the parser and
> AST declarations. Reconcile this roadmap with the query implementation and
> current test/benchmark evidence before marking the integration complete.

## Architecture

### Previously documented BM25 path (executor status not independently confirmed)

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

### Phase 1: Parser Integration — [x] Parser structures present; end-to-end parser tests not confirmed

**Goal:** AQL SEARCH keyword + PHRASE()/NEAR/STARTS_WITH predicates parse correctly.

- [x] Add `SEARCH` token and AST node declarations to `include/query/aql_parser.h`.
- [x] Implement tokenization and `parseSearchClause()` in `src/query/aql_parser.cpp`.
- [x] Parse PHRASE, NEAR, STARTS_WITH, BOOST, and ANALYZER forms in the SEARCH clause.
- [ ] Confirm focused parser tests cover valid and invalid forms (Target: Q4 2026).

**Acceptance:** SEARCH clause with PHRASE/NEAR/STARTS_WITH parses without errors; AST contains `SearchClauseNode`.

### Phase 2: Translator Wiring — [?] Reconcile source and roadmap status; Q3 2026 target elapsed

**Goal:** `SearchClauseNode` translates to an `FtsExecutionPlan` that calls the correct `InvertedIndex` method.

- [?] Verify whether SEARCH AST translation/execution is implemented; the root roadmap and source search disagree.
  - `PHRASE(field, text)` → `InvertedIndex::searchPhrase(table, column, phrase, limit)`
  - `NEAR(field, text, dist)` → `InvertedIndex::searchFuzzy()` with distance cap
  - `STARTS_WITH(field, prefix)` → `InvertedIndex::search()` with prefix flag
- [?] Reconcile the executor-dispatch status with the root query roadmap and current source.
- [?] Confirm BM25 behavior without SEARCH and add regression evidence after executor status is resolved.

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

- [AQL v2.0 complete roadmap](../query/AQL_V2_0_0_COMPLETE_ROADMAP.md) — Master v2.0.0 language standard roadmap (Gate 5)
- `include/index/inverted_index.h` — Production FTS backend (searchPhrase, search, searchFuzzy)
- `tests/aql/test_aql_bm25.cpp` — Existing BM25 integration tests
- `tests/aql/test_aql_fulltext_hybrid.cpp` — Existing FULLTEXT integration tests
