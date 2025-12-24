# 🔎 ThemisDB AQL - Phases 1-3 Consolidated Guide

**Category:** 🔎 Advanced Queries  
**Version:** v1.3.0  
**Status:** ✅ Production Ready  
**Datum:** 24. Dezember 2025

---

## 📑 Inhaltsverzeichnis

- [📋 Executive Summary](#-executive-summary)
- [🎯 Phase Overview](#-phase-overview)
  - [Phase 1 & 1.5: Hybrid Query Optimizations](#phase-1--15-hybrid-query-optimizations)
  - [Phase 2 & 2.5: AQL Syntax Sugar](#phase-2--25-aql-syntax-sugar)
  - [Phase 3: Subqueries & CTEs](#phase-3-subqueries--ctes)
- [✨ Unified Feature Set](#-unified-feature-set)
- [🚀 Quick Start Examples](#-quick-start-examples)
- [📖 Detailed Documentation](#-detailed-documentation)
- [💡 Best Practices](#-best-practices)
- [🔧 Performance Tuning](#-performance-tuning)
- [📚 References](#-references)

---

## 📋 Executive Summary

Dieses Dokument konsolidiert die Implementierung und Dokumentation der **AQL-Erweiterungen Phases 1-3** für ThemisDB v1.3.0. Diese drei Phasen bilden zusammen ein umfassendes System für fortgeschrittene Multi-Model-Queries mit optimaler Performance.

### 🎉 Achievements Across All Phases

| Phase | Hauptfeatures | Status | LOC | Tests |
|-------|--------------|--------|-----|-------|
| **Phase 1 & 1.5** | Hybrid Query C++ API & Optimizations | ✅ Complete | ~2,500 | 7+ |
| **Phase 2 & 2.5** | AQL Syntax Sugar (SIMILARITY, PROXIMITY, SHORTEST_PATH) | ✅ Complete | ~3,200 | 20+ |
| **Phase 3** | Subqueries & Common Table Expressions | ✅ Complete | ~2,800 | 35+ |
| **Total** | **Unified Multi-Model Query System** | ✅ **Production Ready** | **~8,500** | **62+** |

### 🚀 Key Capabilities

1. **Hybrid Multi-Model Queries**: Vector + Geo + Graph + Content in einer Query
2. **Performance Optimization**: 4-25× Speedup durch Index-Integration
3. **Intuitive AQL Syntax**: SIMILARITY(), PROXIMITY(), SHORTEST_PATH keywords
4. **Advanced Query Features**: CTEs, Subqueries, Correlated Queries
5. **Cost-Based Optimization**: Automatische Planwahl für optimale Performance
6. **Production Ready**: Comprehensive testing, benchmarks, documentation

---

## 🎯 Phase Overview

### Phase 1 & 1.5: Hybrid Query Optimizations

**Branch:** `feature/aql-st-functions`  
**Released:** 17. November 2025  
**Status:** ✅ VOLLSTÄNDIG IMPLEMENTIERT & COMMITTED  

#### Implementierte Features

##### 1. HNSW Integration für Vector+Geo ✅
- **Performance-Ziel:** <5ms @ 1000 candidates → **ERREICHT (4ms)**
- **Code:** ~150 LOC in `query_engine.cpp`
- **Speedup:** 10× vs. brute-force
- **Test:** `VectorGeo_WithVectorIndexManager_UsesHNSW`

##### 2. Spatial Index Integration für Vector+Geo ✅
- **Performance-Ziel:** <5ms mit R-Tree → **ERREICHT**
- **Code:** ~120 LOC (inkl. `extractBBoxFromFilter()` helper)
- **Speedup:** 100× vs. full table scan
- **Fallback:** Graceful degradation zu full scan

##### 3. Batch Entity Loading für Graph+Geo ✅
- **Performance-Ziel:** 20-50ms @ BFS depth 5 → **ERREICHT (35ms)**
- **Code:** ~80 LOC für beide Cases (Dijkstra + BFS)
- **Speedup:** 5× vs. sequential loading
- **Observability:** Trace attributes hinzugefügt

#### Performance-Ergebnisse Phase 1.5

| Query Type | Vorher | Nachher | Speedup | Status |
|------------|--------|---------|---------|--------|
| **Vector+Geo (HNSW+Spatial)** | 100ms | **4ms** | **25×** | ✅✅ |
| **Vector+Geo (Spatial only)** | 100ms | **18ms** | **5.5×** | ✅ |
| **Graph+Geo (Batch)** | 160ms | **35ms** | **4.5×** | ✅ |
| **Content+Geo** | 20-80ms | 20-80ms | - | ✅ Bereits effizient |

**Alle Performance-Ziele erreicht oder übertroffen!** 🎯

#### Key Files Modified (Phase 1.5)
1. `include/query/query_engine.h` (+64 lines) - Optional index parameters
2. `src/query/query_engine.cpp` (+1015 lines) - HNSW, Spatial, Batch Loading
3. `tests/test_hybrid_queries.cpp` (549 lines) - 7 Integration Tests
4. `docs/de/aql/aql_hybrid_queries_phase15.md` (678 lines) - Documentation

---

### Phase 2 & 2.5: AQL Syntax Sugar

**Branch:** `feature/aql-st-functions`  
**Status:** ✅ Phase 2 + 2.5 abgeschlossen  

Phase 2 erweitert AQL mit Syntax-Zucker für Hybrid Queries, sodass diese elegant und intuitiv in AQL geschrieben werden können.

#### Implementierte Features

##### Phase 2.1: SIMILARITY() Function ✅

**Syntax:**
```aql
FOR doc IN entities
  FILTER ST_Within(doc.location, @region)
  SORT SIMILARITY(doc.embedding, @queryVector) DESC
  LIMIT 10
  RETURN doc
```

**Implementation:**
- Keyword SIMILARITY im Tokenizer
- Parser erkennt SIMILARITY als FunctionCall in SORT
- SimilarityCallExpr spezialisierter AST Node
- Translator: Erkennung + Erzeugung VectorGeoQuery
- Dispatcher: executeAql() ruft executeVectorGeoQuery()
- Tests: Parsing / Übersetzung / Dispatch

**Mit zusätzlichen Prädikaten (Composite Index Prefilter):**
```aql
FOR doc IN hotels
  FILTER ST_Within(doc.location, POLYGON(...))
  FILTER doc.city == "Berlin"
  SORT SIMILARITY(doc.description_embedding, @queryVec) DESC
  LIMIT 10
  RETURN doc
```

##### Phase 2.2: Graph Spatial Constraints ✅

**Syntax:**
```aql
FOR v IN 1..10 OUTBOUND @start edges
  FILTER ST_Within(v.location, @boundary)
  SHORTEST_PATH TO @target
  RETURN v
```

**Implementation:**
- Add SHORTEST_PATH keyword
- Extend parser for Graph + FILTER pattern
- Implement spatial constraint extraction
- Generate executeRecursivePathQuery() with constraints
- Add integration tests

##### Phase 2.3: PROXIMITY() Function ✅

**Syntax:**
```aql
FOR doc IN restaurants
  FILTER FULLTEXT(doc.menu, "vegan")
  SORT PROXIMITY(doc.location, ST_Point(13.4, 52.5)) ASC
  LIMIT 20
  RETURN doc
```

**Implementation:**
- Add PROXIMITY keyword
- Implement ProximityExpr AST node
- Detect FULLTEXT + PROXIMITY pattern
- Generate executeContentGeoQuery()
- Add distance calculation
- Add integration tests

##### Phase 2.5: Advanced Optimizations ✅

1. **Erweiterte Predicate-Normalisierung**
   - Equality + Range + Composite Index Prefiltering
   - `scanKeysEqualComposite()` Integration in `executeVectorGeoQuery`
   - Automatische Erkennung von AND-Ketten für Composite Indizes

2. **Content+Geo Erweitertes Kostenmodell**
   - Planwahl zwischen Fulltext-first und Spatial-first
   - Heuristisches Modell mit `bboxRatio` und geschätzten FT-Hits
   - Naive Token-AND Evaluation im Spatial-first Pfad

3. **Graph-Pfad Optimierung**
   - Dynamische Branching-Faktor-Schätzung (Sampling über erste 2 Tiefen)
   - Frühabbruch bei geschätzter Expansion >1M Vertices
   - Räumliche Selektivität in Kostenmodell integriert

4. **Benchmark Suite Hybrid Sugar**
   - `benchmarks/bench_hybrid_aql_sugar.cpp` erstellt
   - Vergleich: AQL Sugar vs C++ API (Vector+Geo, Content+Geo)
   - Parse+Translate Overhead isoliert gemessen

#### Backwards Compatibility

**CRITICAL:** Alle Phase 2 Features sind **100% backwards compatible**:

1. ✅ Alte Queries funktionieren weiterhin
2. ✅ Neue Syntax ist **optional** (C++ API bleibt verfügbar)
3. ✅ Fallback zu unoptimierter Ausführung wenn Syntax nicht erkannt
4. ✅ Keine Breaking Changes in Parser/Translator

---

### Phase 3: Subqueries & CTEs

**Branch:** `feature/aql-subqueries` → `feature/aql-st-functions` (Implementierung)  
**Status:** ✅ **ABGESCHLOSSEN** (17. November 2025)  
**Aufwand:** 16-21 Stunden geplant → ~12 Stunden tatsächlich

Phase 3 erweitert AQL um **Subqueries** und **Common Table Expressions (CTEs)**, um komplexe Queries eleganter und performanter zu machen.

#### ✅ Implementierte Sub-Phasen

1. ✅ **Phase 3.1: WITH Clause** - Parser, AST, Tests
2. ✅ **Phase 3.2: Scalar Subqueries** - Expression-Context Parsing
3. ✅ **Phase 3.3: Array Subqueries** - ANY/ALL Quantifiers
4. ✅ **Phase 3.4: Correlated Subqueries** - Parent Context Chain
5. ✅ **Phase 3.5: Optimization** - Materialization Heuristics

#### Feature 1: Common Table Expressions (WITH Clause) ✅

**Einfaches CTE:**
```aql
WITH berlin_hotels AS (
  FOR hotel IN hotels
  FILTER hotel.city == "Berlin"
  RETURN hotel
)
FOR h IN berlin_hotels
  SORT h.stars DESC
  LIMIT 10
  RETURN h
```

**Mehrere CTEs:**
```aql
WITH 
  expensive_hotels AS (
    FOR h IN hotels FILTER h.price > 150 RETURN h
  ),
  top_rated AS (
    FOR h IN expensive_hotels FILTER h.rating >= 4.5 RETURN h
  )
FOR h IN top_rated
  RETURN h
```

**CTE mit Aggregation:**
```aql
WITH avg_price_by_city AS (
  FOR h IN hotels
  COLLECT city = h.city
  AGGREGATE avg_price = AVG(h.price)
  RETURN {city, avg_price}
)
FOR stat IN avg_price_by_city
  FILTER stat.avg_price > 100
  RETURN stat
```

#### Feature 2: Scalar Subqueries ✅

Subquery die genau einen Wert zurückgibt:

```aql
FOR hotel IN hotels
  LET avg_rating = (
    FOR review IN reviews
    FILTER review.hotel_id == hotel._id
    RETURN AVG(review.rating)
  )[0]
  FILTER avg_rating > 4.5
  RETURN {hotel, avg_rating}
```

#### Feature 3: Array Subqueries ✅

**IN Operator:**
```aql
FOR product IN products
  FILTER product.category_id IN (
    FOR cat IN categories
    FILTER cat.active == true
    RETURN cat._id
  )
  RETURN product
```

**ANY Operator:**
```aql
FOR hotel IN hotels
  FILTER ANY review IN (
    FOR r IN reviews 
    FILTER r.hotel_id == hotel._id 
    RETURN r
  ) SATISFIES review.rating >= 4
  RETURN hotel
```

**ALL Operator:**
```aql
FOR hotel IN hotels
  FILTER ALL review IN (
    FOR r IN reviews 
    FILTER r.hotel_id == hotel._id 
    RETURN r
  ) SATISFIES review.rating >= 3
  RETURN hotel
```

#### Feature 4: Correlated Subqueries ✅

Subquery mit Zugriff auf äußere Variablen:

```aql
FOR hotel IN hotels
  LET review_count = (
    FOR review IN reviews
    FILTER review.hotel_id == hotel._id  -- Correlation!
    RETURN COUNT(1)
  )[0]
  FILTER review_count > 10
  RETURN {hotel, review_count}
```

**Implementation: Context Chaining**
- `EvaluationContext.parent` pointer für parent scope lookup
- `createChild()` helper für child context creation
- `get()` mit parent chain lookup für outer variables

#### Feature 5: Optimization Strategies ✅

**Materialization Heuristics:**
- Materialisiere wenn mehrfach verwendet (>1 Reference)
- Materialisiere wenn Aggregation enthalten (teuer neu zu berechnen)
- Materialisiere wenn geschätzte Größe > Threshold (1000 rows)
- Sonst: Inline

**Subquery to JOIN Conversion:**
- Correlated existence check → SEMI JOIN
- Kostenbasierte Entscheidung basierend auf Query-Struktur

#### Key Files Modified (Phase 3)

- `src/query/aql_parser.cpp` - WITH/AS/ALL/SATISFIES Keywords, parseWithClause(), Subquery/ANY/ALL Parsing
- `include/query/aql_parser.h` - WithNode, CTEDefinition, SubqueryExpr, AnyExpr, AllExpr AST
- `include/query/query_engine.h` - EvaluationContext mit CTE storage, parent chain, createChild()
- `src/query/query_engine.cpp` - SubqueryExpr/AnyExpr/AllExpr Evaluation
- `include/query/subquery_optimizer.h` - shouldMaterializeCTE(), canConvertToJoin(), estimateQueryCost()
- `tests/test_aql_with_clause.cpp` - 15 Unit Tests für WITH
- `tests/test_aql_subqueries.cpp` - 20+ Unit Tests für Subqueries/ANY/ALL/Optimization
- `CMakeLists.txt` - Test targets hinzugefügt

---

## ✨ Unified Feature Set

Die Kombination aller drei Phasen ermöglicht hochkomplexe, performante Multi-Model-Queries:

### 🔥 Advanced Combined Example

```aql
-- Kombiniere CTEs (Phase 3), Hybrid Queries (Phase 1+2), und Spatial Constraints
WITH 
  -- CTE 1: Relevante Restaurants mit Fulltext + Geo
  nearby_restaurants AS (
    FOR place IN places
    FILTER FULLTEXT(place.menu, "vegan organic")
    FILTER ST_Within(place.location, @berlinBoundary)
    SORT PROXIMITY(place.location, @myPosition) ASC
    LIMIT 50
    RETURN place
  ),
  
  -- CTE 2: Enrichment mit Review-Statistiken (Correlated Subquery)
  enriched_restaurants AS (
    FOR restaurant IN nearby_restaurants
    LET avg_rating = (
      FOR review IN reviews
      FILTER review.place_id == restaurant._id
      RETURN AVG(review.rating)
    )[0]
    LET review_count = (
      FOR review IN reviews
      FILTER review.place_id == restaurant._id
      RETURN COUNT(1)
    )[0]
    FILTER avg_rating >= 4.0 AND review_count > 5
    RETURN MERGE(restaurant, {avg_rating, review_count})
  )

-- Hauptquery: Finde ähnliche Restaurants via Vector-Ähnlichkeit
FOR restaurant IN enriched_restaurants
  SORT SIMILARITY(restaurant.cuisine_embedding, @myCuisinePrefs) DESC
  LIMIT 10
  RETURN {
    name: restaurant.name,
    distance: DISTANCE(restaurant.location, @myPosition),
    avg_rating: restaurant.avg_rating,
    review_count: restaurant.review_count,
    similarity_score: SIMILARITY(restaurant.cuisine_embedding, @myCuisinePrefs)
  }
```

**Features Used:**
- ✅ WITH clause (Phase 3) für CTE definition
- ✅ FULLTEXT + PROXIMITY (Phase 2) für Content+Geo
- ✅ Correlated Subqueries (Phase 3) für Review-Statistiken
- ✅ SIMILARITY (Phase 2) für Vector-Ranking
- ✅ HNSW + Spatial Index Optimization (Phase 1.5) - automatisch
- ✅ Cost-Based Optimizer (Phase 2.5) - automatisch
- ✅ CTE Materialization Optimization (Phase 3.5) - automatisch

---

## 🚀 Quick Start Examples

### Example 1: Vector+Geo with Index Prefilter

```aql
-- Finde ähnliche Hotels in Berlin mit hoher Bewertung
FOR hotel IN hotels
  FILTER hotel.city == "Berlin"           -- Equality Predicate (Index)
  FILTER hotel.stars >= 4                 -- Range Predicate (Index)
  FILTER ST_Within(hotel.location, @bbox) -- Spatial Constraint
  SORT SIMILARITY(hotel.description_vec, @query_vec) DESC
  LIMIT 10
  RETURN hotel
```

**Optimizer wählt automatisch:** Composite Index Prefilter → Spatial Filter → Vector k-NN

### Example 2: Content+Geo with Distance Ranking

```aql
-- Finde "coffee shops" in der Nähe, sortiert nach Distanz
FOR place IN places
  FILTER FULLTEXT(place.description, "coffee shop")
  FILTER ST_Within(place.location, @searchArea)
  SORT PROXIMITY(place.location, @myLocation) ASC
  LIMIT 20
  RETURN {
    name: place.name,
    distance_meters: DISTANCE(place.location, @myLocation),
    description: place.description
  }
```

**Optimizer wählt automatisch:** Fulltext-first vs Spatial-first basierend auf Selektivität

### Example 3: Graph+Geo Shortest Path

```aql
-- Finde kürzesten Pfad von Berlin nach Dresden, nur durch Deutschland
FOR v, e, p IN 1..10 OUTBOUND "city:berlin" road_network
  FILTER ST_Within(v.location, @germanyPolygon)
  SHORTEST_PATH TO "city:dresden"
  RETURN {
    path: p,
    total_distance: SUM(e[*].distance),
    waypoints: v[*].name
  }
```

**Optimizer verwendet:** Batch Entity Loading + Spatial Constraint Pruning

### Example 4: CTE-based Data Pipeline

```aql
-- Multi-Stage Data Processing mit CTEs
WITH
  -- Stage 1: Filter active users
  active_users AS (
    FOR u IN users
    FILTER u.status == "active"
    FILTER u.last_login > @cutoffDate
    RETURN u
  ),
  
  -- Stage 2: Join mit Orders
  user_orders AS (
    FOR u IN active_users
    FOR o IN orders
    FILTER o.user_id == u._id
    RETURN {user: u, order: o}
  ),
  
  -- Stage 3: Aggregate
  user_stats AS (
    FOR uo IN user_orders
    COLLECT user = uo.user
    AGGREGATE 
      total_spent = SUM(uo.order.amount),
      order_count = COUNT(1)
    RETURN {user, total_spent, order_count}
  )

-- Main Query: Top spenders
FOR stat IN user_stats
  FILTER stat.total_spent > 1000
  SORT stat.total_spent DESC
  LIMIT 10
  RETURN stat
```

---

## 📖 Detailed Documentation

### Architecture Overview

#### 1. Parsing Layer (Phase 2 & 3)

**Extended Keywords:**
- `SIMILARITY` - Vector similarity in SORT clause
- `PROXIMITY` - Geo proximity in SORT clause
- `SHORTEST_PATH` - Graph shortest path directive
- `WITH`, `AS` - CTE definition
- `ANY`, `ALL`, `SATISFIES` - Quantified predicates

**Extended AST Nodes:**
- `SimilarityCallExpr` - Vector similarity function
- `ProximityExpr` - Geo proximity function
- `WithNode` - CTE container
- `CTEDefinition` - Individual CTE
- `SubqueryExpr` - Subquery in expression
- `AnyExpr`, `AllExpr` - Quantifier expressions

#### 2. Translation Layer (Phase 2 & 3)

**Hybrid Query Detection:**
- `isVectorGeoQuery()` - Detects FILTER ST_* + SORT SIMILARITY
- `isGraphGeoQuery()` - Detects Graph Traversal + FILTER ST_* on vertex
- `isContentGeoQuery()` - Detects FULLTEXT + SORT PROXIMITY

**CTE Processing:**
- `countCTEReferences()` - Counts CTE usage recursively
- `SubqueryOptimizer::shouldMaterializeCTE()` - Materialization decision
- `attachCTEs()` - Attaches CTE metadata to TranslationResult

#### 3. Optimization Layer (Phase 1.5, 2.5, 3.5)

**Index Selection:**
- HNSW for Vector k-NN (Phase 1.5)
- R-Tree for Spatial Filter (Phase 1.5)
- Composite Indexes for Equality/Range Predicates (Phase 2.5)

**Cost-Based Planning:**
- Vector+Geo: Spatial-first vs Vector-first (Phase 2.5)
- Content+Geo: Fulltext-first vs Spatial-first (Phase 2.5)
- Graph+Geo: Branching factor estimation + early abort (Phase 2.5)
- CTE: Materialization vs Inline (Phase 3.5)

**Observability:**
- Tracer span attributes für Plan & Costs
- `optimizer.vg.plan`, `optimizer.cg.plan`, `optimizer.graph.plan`
- `composite_prefilter_applied`, `branching_estimate`

#### 4. Execution Layer (Phase 1, 2, 3)

**Hybrid Query Execution:**
- `executeVectorGeoQuery()` - Vector+Geo with HNSW + Spatial Index
- `executeContentGeoQuery()` - Content+Geo with BM25 + Distance Ranking
- `executeRecursivePathQuery()` - Graph+Geo with Batch Loading + Spatial Constraints

**CTE Execution:**
- `executeCTEs()` - Führt CTE-Liste sequentiell aus
- Stores results in `EvaluationContext`
- Supports all query types: Join, Conjunctive, Disjunctive, VectorGeo, ContentGeo

**Subquery Execution:**
- `evaluateExpression()` SubqueryExpr case - recursive translation & execution
- Creates child context via `ctx.createChild()` for correlation
- Returns scalar (single), null (empty), or array (multiple) results

---

## 💡 Best Practices

### 1. Hybrid Query Optimization

**DO:**
- ✅ Use SIMILARITY/PROXIMITY keywords for automatic optimization
- ✅ Add equality/range predicates before spatial filters for index prefilter
- ✅ Provide realistic k values for vector search (10-100 typical)
- ✅ Use ST_Within with tight bounding boxes for better spatial selectivity

**DON'T:**
- ❌ Don't use SIMILARITY without spatial constraints (use pure vector search instead)
- ❌ Don't use very large k values (>1000) without good reason
- ❌ Don't forget to create indexes (secondary, vector, spatial)

### 2. CTE Usage

**DO:**
- ✅ Use CTEs for repeated subquery patterns
- ✅ Use CTEs for complex data transformations
- ✅ Use CTEs for aggregations that are expensive to recompute
- ✅ Name CTEs descriptively (e.g., `active_users`, `top_rated_hotels`)

**DON'T:**
- ❌ Don't create CTEs for simple filters (inline them instead)
- ❌ Don't create CTEs with millions of rows (use streaming or chunking)
- ❌ Don't over-nest CTEs (3-4 levels max for readability)

### 3. Subquery Optimization

**DO:**
- ✅ Use correlated subqueries for row-by-row calculations
- ✅ Use ANY/ALL for existence checks (more readable than COUNT)
- ✅ Use scalar subqueries in LET for enrichment
- ✅ Consider converting to JOINs if subquery is expensive

**DON'T:**
- ❌ Don't use subqueries for simple lookups (use JOIN instead)
- ❌ Don't use uncorrelated subqueries without good reason (use CTE instead)
- ❌ Don't nest subqueries more than 2-3 levels deep

### 4. Performance Tuning

**Indexes:**
- Create composite indexes for common equality/range predicates
- Create vector indexes (HNSW) for similarity search
- Create spatial indexes (R-Tree) for geo queries
- Monitor index usage via tracer attributes

**Query Planning:**
- Use EXPLAIN to understand query plans
- Check tracer attributes for cost model decisions
- Monitor query execution time
- Profile slow queries for optimization opportunities

**Memory Management:**
- Large CTEs may materialize in memory (monitor memory usage)
- Consider spill-to-disk for very large CTEs (future feature)
- Use LIMIT early to reduce intermediate result sizes
- Stream results when possible (avoid COLLECT on huge datasets)

---

## 🔧 Performance Tuning

### Tracer Attributes for Observability

#### Vector+Geo Queries

```
optimizer.vg.plan = "spatial_first" | "vector_first"
optimizer.vg.cost_spatial_first = 245.3
optimizer.vg.cost_vector_first = 180.7
optimizer.vg.spatial_selectivity = 0.15
optimizer.vg.composite_index_selectivity = 0.05
composite_prefilter_applied = true
composite_prefilter_keys = 3
hnsw_used = true
spatial_candidates = 847
```

#### Content+Geo Queries

```
optimizer.cg.plan = "fulltext_first" | "spatial_first"
optimizer.cg.cost_fulltext_first = 320.5
optimizer.cg.cost_spatial_first = 450.2
optimizer.cg.fulltext_hits_estimate = 150
optimizer.cg.spatial_selectivity = 0.25
```

#### Graph+Geo Queries

```
optimizer.graph.branching_estimate = 3.2
optimizer.graph.expanded_estimate = 850000
optimizer.graph.spatial_selectivity = 0.18
optimizer.graph.aborted = false
batch_load_count = 12
batch_load_total_entities = 847
```

### Tuning Guidelines

#### When spatial_first is chosen but slow:
- Tighten bounding box (reduce spatial_selectivity)
- Add more equality/range predicates for composite index prefilter
- Consider reducing k (fewer vector candidates needed)

#### When vector_first is chosen but slow:
- Increase spatial selectivity (tighten bbox)
- Add HNSW index if not present
- Check composite_index_selectivity (add more indexed predicates)

#### When Graph queries expand too much:
- Add tighter spatial constraints
- Reduce max depth
- Add additional vertex filters
- Check branching_estimate (should be <5 for good performance)

#### When CTEs materialize large datasets:
- Check if CTE is reused (reference_count > 1)
- Consider streaming instead of materialization
- Split into smaller CTEs
- Add LIMIT where appropriate

---

## 📚 References

### Related Documentation

- [AQL Syntax Reference](aql_syntax.md) - Complete AQL language reference
- [AQL Functions Reference](aql_functions_reference.md) - All available functions
- [Query Engine Architecture](aql_query_engine.md) - Engine internals
- [Vector Index Guide](../features/vector_index.md) - HNSW index details
- [Spatial Index Guide](../features/spatial_index.md) - R-Tree index details
- [Cost Models Documentation](../development/cost-models.md) - Optimizer cost models

### Individual Phase Documentation

For detailed implementation information on each phase:

- [Phase 1.5 Completion Report](../reports/phase_1.5_completion.md) - Detailed completion report
- [Phase 2 Implementation Plan](../reports/phase_2_plan.md) - Implementation details
- [Phase 3 Implementation Plan](../reports/phase_3_plan.md) - Implementation details

### Code Files

**Core Query Engine:**
- `include/query/query_engine.h` - Query engine interface
- `src/query/query_engine.cpp` - Query execution logic
- `include/query/aql_parser.h` - AQL parser interface
- `src/query/aql_parser.cpp` - Parser implementation
- `include/query/aql_translator.h` - Translator interface
- `src/query/aql_translator.cpp` - Translation logic

**Optimization:**
- `include/query/subquery_optimizer.h` - Subquery optimization heuristics
- Cost model logic embedded in `query_engine.cpp`

**Tests:**
- `tests/test_hybrid_queries.cpp` - Hybrid query tests (Phase 1.5)
- `tests/test_aql_with_clause.cpp` - CTE tests (Phase 3.1)
- `tests/test_aql_subqueries.cpp` - Subquery tests (Phase 3.2-3.5)

**Benchmarks:**
- `benchmarks/bench_hybrid_aql_sugar.cpp` - Phase 2 AQL syntax benchmarks

---

## 📝 Changelog

### v1.3.0 - December 24, 2025

**Phase 1 & 1.5: Hybrid Query Optimizations**
- ✅ HNSW Integration für Vector+Geo (10× speedup)
- ✅ Spatial Index Integration (100× speedup)
- ✅ Batch Entity Loading für Graph+Geo (5× speedup)
- ✅ Performance goals achieved: 4ms Vector+Geo, 35ms Graph+Geo

**Phase 2 & 2.5: AQL Syntax Sugar**
- ✅ SIMILARITY() function für Vector+Geo queries
- ✅ PROXIMITY() function für Content+Geo queries
- ✅ SHORTEST_PATH keyword für Graph queries
- ✅ Composite Index Prefilter für Equality/Range predicates
- ✅ Cost-based optimizer für alle Hybrid-Typen
- ✅ Comprehensive benchmarks and tracer attributes

**Phase 3: Subqueries & CTEs**
- ✅ WITH clause für Common Table Expressions
- ✅ Scalar subqueries in LET and RETURN
- ✅ Array subqueries mit ANY/ALL quantifiers
- ✅ Correlated subqueries mit parent context chain
- ✅ CTE materialization heuristics
- ✅ Subquery to JOIN conversion

**Documentation:**
- ✅ Consolidated documentation combining all three phases
- ✅ Comprehensive examples showing feature combinations
- ✅ Best practices and performance tuning guides
- ✅ Tracer attributes for observability

---

**Version:** v1.3.0  
**Status:** ✅ Production Ready  
**Total Lines of Code:** ~8,500  
**Total Tests:** 62+  
**Performance Improvement:** 4-25× across different query types

**Nächste Schritte:** Phase 4 Kandidaten - JOINs, Window Functions, Query Plan Caching

