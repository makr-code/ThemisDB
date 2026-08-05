# AQL Geospatial Parser Integration Roadmap

<!-- Status: [~] in progress | [ ] open | [x] done | [I] issue | [P] PR | [?] blocked -->

**Status:** ✅ Phase 1 COMPLETE — Phases 2–4 pending (Target: Q3 2026)
**Target Release:** v2.0.0 (Q3 2026)
**Owner:** query module / Team C
**Last Updated:** 2026-07-27

> **Note:** The canonical roadmap reference path in `AQL_V2_0_0_COMPLETE_ROADMAP.md` is
> `src/geospatial/AQL_GEOSPATIAL_ROADMAP.md`. Since `src/geospatial/` does not exist as a
> standalone module, this roadmap lives at `src/query/AQL_GEOSPATIAL_ROADMAP.md`.

---

## Executive Summary

Wire the already-implemented ST_* geospatial functions into the AQL parser so they can be used
in **FILTER, SORT, and RETURN** contexts, not just LET. The functions exist in production in
`src/query/let_evaluator.cpp`; this roadmap covers only the AQL parser integration.

**Effort:** Revised down to 2–3 weeks (vs. original 3–4 weeks) because 70% of implementation
is already done — only parser wiring and query-optimizer hints are needed.

---

## Current State

| Feature | Status | Evidence |
|---------|--------|----------|
| ST_Distance(geom1, geom2) — Haversine | ✅ Production | `src/query/let_evaluator.cpp:513-596` |
| ST_Within(geom1, geom2) | ✅ Production | `src/query/let_evaluator.cpp:597-721` |
| ST_Contains(g1, g2) | ✅ Production | `src/query/let_evaluator.cpp:762+` |
| ST_Intersects(g1, g2) | ✅ Production | `src/query/let_evaluator.cpp` |
| ST_GeomFromGeoJSON(json) | ✅ Production | `src/query/let_evaluator.cpp` |
| ST_Point(lon, lat) | ✅ Production | `src/query/let_evaluator.cpp` |
| ST_AsGeoJSON(geom) | ✅ Production | `src/query/let_evaluator.cpp` |
| LET context: `LET d = ST_Distance(...)` | ✅ Production | `tests/aql/test_aql_let_st.cpp` |
| FILTER context: `FILTER ST_Within(doc.loc, poly)` | ❌ Not Implemented | Parser does not evaluate ST_* in FILTER |
| SORT context: `SORT ST_Distance(...) ASC` | ❌ Not Implemented | |
| RETURN context: `RETURN ST_Distance(...)` | ❌ Not Implemented | |
| Query optimizer: spatial index selection hints | ❌ Not Implemented | |

---

## Root Cause of Gap

The AQL parser dispatches FILTER/SORT/RETURN expressions through a different evaluation path
than LET. `let_evaluator.cpp` is only called for LET-assigned variables. Function calls in
FILTER/RETURN predicates go through a separate expression evaluator that does not currently
delegate to `LetEvaluator::evalFunction()` for ST_* names.

**Fix scope:** Extend the FILTER/SORT/RETURN expression evaluator to call into `LetEvaluator`
(or share the function dispatch table) when it encounters a function call node whose name starts
with `ST_`.

---

## Implementation Phases

### Phase 1: Parser + Expression Evaluator Wiring (1 week) — [x] COMPLETE (2026-07-27)

**Finding:** ST_* functions already evaluate in FILTER/SORT/RETURN contexts via `qe_evalFunction`
in `query_engine.cpp` (lines 1676–2330). No parser grammar changes were required — the parser
already handles any `IDENTIFIER(...)` as a generic `FunctionCallExpr`. The gap was solely missing
test coverage.

**Delivered:**
- [x] Verified ST_* function names parse without errors in FILTER/SORT/RETURN (Target: Q3 2026)
- [x] Verified `LetEvaluator::evaluateExpression` dispatches ST_* in FILTER context (Target: Q3 2026)
- [x] `tests/aql/test_aql_st_predicates.cpp` — 26 tests across 4 suites (GEO-FILTER/CONTEXT/EVAL/PARSEEVAL) (Target: Q3 2026)
- [x] LET context regression confirmed unaffected (`test_aql_let_st.cpp`) (Target: Q3 2026)

**Acceptance:** FILTER/SORT/RETURN with ST_* parse and evaluate without errors. ✅

### Phase 2: Query Optimizer Integration (Phase 6C) — [x] COMPLETE (2026-08-05)

**Goal:** Optimizer recognises spatial predicates and selects geo index when available.

**Delivered:**
- [x] Geospatial cost model with histogram-based selectivity (`geospatial_cost_model.h/cpp`)
  - `GeospatialCostEstimator` class with ST_DISTANCE, ST_CONTAINS, ST_INTERSECTS costs
  - `SpatialHistogram` for selectivity estimation
  - R-tree, grid, and full-scan cost modeling
  - Cost accuracy: ±20% vs actual execution

- [x] Optimizer hint directives (`geospatial_optimizer_hints.h/cpp`)
  - `USE_INDEX(field, "index_name")` — Force specific index
  - `FORCE_SCAN(field)` — Disable indexing
  - `INDEX_PRIORITY(field, factor)` — Adjust selection priority
  - `DISTANCE_ORDER(field, direction)` — Pre-sort by distance
  - Complete hint parser and validator

- [x] Spatial index selector (`geospatial_index_selector.h/cpp`)
  - Automatic index selection based on data distribution
  - Support for R-tree, Grid, Quadtree indexes
  - Index ranking and efficiency scoring
  - Selectivity gain calculation

- [x] Query plan rewriting (`geospatial_query_rewrite.h/cpp`)
  - 5 rewrite rules for spatial optimization
  - Rule 1: Index path reordering
  - Rule 2: Distance ordering optimization
  - Rule 3: Intersection optimization (bbox + refinement)
  - Rule 4: Redundant predicate elimination
  - Rule 5: Predicate pushdown

- [x] Performance benchmarks (`bench_geospatial_phase2.cpp`)
  - BENCH_GEO_01: Distance nearest-neighbor (≤120µs gate)
  - BENCH_GEO_02: Contains point-in-polygon (≤180µs gate)
  - BENCH_GEO_03: Intersects bounding box (≤240µs gate)
  - BENCH_GEO_04: Complex 3-way filter (≤600µs gate)
  - BENCH_GEO_05: Throughput (≥800 q/s gate)

- [x] Test suite (40+ tests in `test_geospatial_optimizer.cpp`)
  - GEO_OPT_01-08: Cost estimation accuracy
  - GEO_OPT_09-16: Hint parsing and validation
  - GEO_OPT_17-24: Index selection logic
  - GEO_OPT_25-32: Query rewrite rules
  - GEO_OPT_33-40: Regression gates

- [x] Integration tests (10+ in `test_geospatial_phase2_integration.cpp`)
  - INT_GEO_01-10: End-to-end scenarios

- [x] Documentation (`AQL_GEOSPATIAL_OPTIMIZATION_GUIDE.md`)
  - User guide with examples
  - Performance expectations
  - Best practices

**Acceptance:** All 50+ tests passing, cost model accurate to ±20%, 5 rewrite rules implemented.

### Phase 3: Performance Hardening (0.5 weeks) — [ ] PLANNED (Target: Q3 2026)

**Goal:** Spatial query performance gate — ST_Within on 100K point dataset ≤ 50 ms p99.

- [~] Benchmark `FILTER ST_Within` at 100 000 point documents (Target: Q3 2026)
  - No spatial index: sequential scan baseline
  - With R-tree geo index: ≤ 50 ms p99
  - Implemented 2026-07-27: `benchmarks/bench_aql_geo_filter.cpp`
    (index path + sequential baseline with p99 counters)
- [ ] Optional GPU acceleration hint: `FILTER ST_Distance(...) < r USE INDEX geo_cuda` (Target: Q4 2026)

**Acceptance (Gate 4):** Spatial query with geo index ≥ 50× speedup vs. sequential scan on 100K geometries.

### Phase 4: Testing & Documentation (0.5 weeks) — [ ] PLANNED (Target: Q3 2026)

- [ ] `tests/aql/test_aql_geo_filter.cpp` — 20+ tests (Target: Q3 2026)
  - GEO-01..GEO-08: FILTER ST_Within/ST_Contains/ST_Intersects
  - GEO-09..GEO-12: SORT ST_Distance ASC/DESC
  - GEO-13..GEO-16: RETURN with ST_Distance computed value
  - GEO-17..GEO-20: error paths (invalid GeoJSON, null coordinates, missing index)
- [ ] Perf test: `bench_aql_geo_filter.cpp` — GATE-GEO-01 (≤ 50 ms / 100K with geo index) (Target: Q3 2026)
- [ ] Doxygen: update `let_evaluator.h` for FILTER-context usage (Target: Q3 2026)
- [ ] User guide: `docs/de/aql/aql_geospatial_guide.md` (Target: Q3 2026)
- [ ] Update `AQL_V2_0_0_COMPLETE_ROADMAP.md` Gate 4 status (Target: Q3 2026)

---

## Performance Targets

| Metric | Target | Gate |
|--------|--------|------|
| ST_Within p99 with geo R-tree index, 100K points | ≤ 50 ms | GATE-GEO-01 (Gate 4 in AQL v2 roadmap) |
| ST_Within p99 without index (sequential) | Baseline documented | GATE-GEO-02 (no blocker) |
| ST_Distance sort p99, 100K docs | ≤ 100 ms | GATE-GEO-03 |
| LET regression: no regression vs. baseline | Same as v1.x | Regression gate |

---

## Production Readiness Checklist

- [ ] ST_* functions evaluate correctly in FILTER, SORT, RETURN contexts
- [ ] LET-based ST_* usage unaffected (no regression)
- [ ] Optimizer selects geo index for ST_Within/ST_Distance predicates when available
- [ ] ST_Within p99 ≤ 50 ms at 100K documents with geo index (Gate 4)
- [ ] 20+ unit + integration tests passing
- [ ] Documentation complete

---

## Dependencies

- `include/query/let_evaluator.h` — existing ST_* function implementations (no changes needed)
- `src/query/aql_parser.cpp` — expression evaluator extension point
- `src/query/adaptive_optimizer.cpp` — geo predicate detection
- `include/index/` — geo index (R-tree) for spatial index selection hint

---

## References

- [AQL_V2_0_0_COMPLETE_ROADMAP.md](./AQL_V2_0_0_COMPLETE_ROADMAP.md) — Master v2.0.0 language standard roadmap (Gate 4)
- `tests/aql/test_aql_let_st.cpp` — Existing LET-based ST_* tests (regression baseline)
- `src/query/let_evaluator.cpp:513+` — ST_* function implementations
