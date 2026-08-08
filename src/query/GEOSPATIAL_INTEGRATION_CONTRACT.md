# AQL Geospatial Integration Contract v1.0 (Frozen)

**Status**: FROZEN — Authoritative specification for Phase 2 implementation  
**Created**: 2026-08-06  
**Target Release**: v2.0.0 (Q4 2026)  
**Dependencies**: None (parallel track; functions already implemented in let_evaluator.cpp)  
**Blocking Items**: None

---

## 1. Overview

This contract defines the integration of existing ST_* geospatial functions into AQL query contexts (FILTER, SORT, RETURN). The functions themselves are production-ready (implemented in `src/query/let_evaluator.cpp`). Phase 2 focuses on **parser wiring** and **query optimization**, not reimplementation.

**Key Property**: All 14 ST_* functions are callable in AQL without parser changes—they flow through `qe_evalFunction()` lookup path. Phase 2 adds **optimization hints** and **index selection** for spatial queries.

---

## 2. Existing ST_* Function Signatures

All functions are already implemented and tested. This contract documents their API and integration points.

### 2.1 Coordinate Functions

```cpp
// src/query/let_evaluator.cpp (existing)

/// ST_Point: create a GeoJSON point
/// @param lon Longitude (-180 to 180)
/// @param lat Latitude (-90 to 90)
/// @return {type: "Point", coordinates: [lon, lat]}
/// @example ST_Point(-73.97, 40.78)  // New York
AQLValue ST_Point(double lon, double lat);

/// ST_AsGeoJSON: convert geometry to GeoJSON string
/// @param geom Geometry object (Point, Polygon, etc.)
/// @return JSON string representation
AQLValue ST_AsGeoJSON(const AQLValue& geom);

/// ST_GeomFromGeoJSON: parse GeoJSON string to geometry
/// @param jsonStr GeoJSON string (must be valid JSON)
/// @return Parsed geometry or null on parse error
AQLValue ST_GeomFromGeoJSON(const std::string& jsonStr);
```

### 2.2 Distance Functions

```cpp
/// ST_Distance: haversine distance between two points
/// @param point1 GeoJSON Point (or [lon, lat] array)
/// @param point2 GeoJSON Point (or [lon, lat] array)
/// @return Distance in meters (double)
/// @note Uses WGS84 ellipsoid; accuracy ±0.5%
AQLValue ST_Distance(const AQLValue& point1, const AQLValue& point2);

/// ST_DWithin: test if two geometries within distance threshold
/// @param geom1 Geometry
/// @param geom2 Geometry
/// @param distance Threshold distance in meters
/// @return true if distance(geom1, geom2) <= distance
AQLValue ST_DWithin(const AQLValue& geom1, const AQLValue& geom2, 
                     double distance);
```

### 2.3 Containment Functions

```cpp
/// ST_Contains: test if polygon contains point
/// @param polygon Polygon geometry (must be closed ring)
/// @param point Point geometry or [lon, lat] array
/// @return true if point inside polygon (or on boundary)
AQLValue ST_Contains(const AQLValue& polygon, const AQLValue& point);

/// ST_Within: test if point is within polygon
/// @param point Point geometry or [lon, lat] array
/// @param polygon Polygon geometry
/// @return true if point inside polygon
/// @note Inverse of ST_Contains; more intuitive for queries
AQLValue ST_Within(const AQLValue& point, const AQLValue& polygon);

/// ST_Intersects: test if two geometries intersect
/// @param geom1 Geometry (Point, Polygon, etc.)
/// @param geom2 Geometry
/// @return true if geometries overlap or touch
AQLValue ST_Intersects(const AQLValue& geom1, const AQLValue& geom2);
```

### 2.4 Area/Boundary Functions

```cpp
/// ST_Area: compute area of polygon in square meters
/// @param polygon Polygon geometry
/// @return Area (double), or 0 for Point/LineString
AQLValue ST_Area(const AQLValue& polygon);

/// ST_Boundary: get boundary of polygon/multipolygon
/// @param geom Geometry
/// @return Boundary as LineString or MultiLineString
AQLValue ST_Boundary(const AQLValue& geom);

/// ST_Centroid: compute centroid of polygon
/// @param polygon Polygon geometry
/// @return Point at centroid
AQLValue ST_Centroid(const AQLValue& polygon);
```

### 2.5 Utility Functions

```cpp
/// ST_Length: compute length of linestring in meters
/// @param line LineString geometry
/// @return Length (double)
AQLValue ST_Length(const AQLValue& line);

/// ST_IsValid: validate geometry
/// @param geom Any geometry
/// @return true if geometry is valid (well-formed, non-self-intersecting)
AQLValue ST_IsValid(const AQLValue& geom);

/// ST_GeometryType: get type of geometry
/// @param geom Any geometry
/// @return String: "Point", "Polygon", "LineString", etc.
AQLValue ST_GeometryType(const AQLValue& geom);
```

---

## 3. Parser Wiring: How ST_* Works in FILTER/SORT/RETURN

All ST_* functions are callable via the existing `qe_evalFunction()` mechanism. **No parser changes required.**

### 3.1 Current Path (Working)

```
User Query: FILTER ST_Within(doc.location, polygon)
   ↓
Lexer: tokenizes as IDENTIFIER("ST_Within") + "(" + ...
   ↓
Parser: recognizes function call pattern (generic IDENTIFIER + parentheses)
   ↓
LetEvaluator::evaluateExpression()
   ├─ Detects function call
   ├─ Looks up "ST_Within" in qe_funcRegistry
   └─ Invokes function handler
   ↓
qe_evalFunction() in query_engine.cpp (lines 1676–2330)
   ├─ Dispatch on function name: "ST_Within"
   ├─ Call ST_Within(args)
   └─ Return AQLValue result
   ↓
FILTER evaluation: result.toBool() → true/false
```

### 3.2 Verified Working Examples

```sql
-- Point distance in LET
FOR doc IN collection 
  LET dist = ST_Distance(doc.location, [-73.97, 40.78])
  RETURN dist

-- Polygon containment in FILTER
FOR doc IN collection 
  FILTER ST_Within(doc.location, polygon)
  RETURN doc

-- Multiple predicates (AND)
FOR doc IN collection
  FILTER ST_Within(doc.location, city_polygon) && doc.age > 18
  RETURN doc

-- SORT by distance
FOR doc IN collection
  LET distance = ST_Distance(doc.coords, ref_point)
  SORT distance ASC
  RETURN doc

-- Nested ST_* functions
FOR doc IN collection
  FILTER ST_DWithin(doc.location, ref_point, 1000)  // within 1km
  RETURN {name: doc.name, distance: ST_Distance(doc.location, ref_point)}
```

---

## 4. Phase 2 Enhancements: Parser Integration Contract

Phase 2 adds **explicit optimization hints** without changing parser behavior.

### 4.1 Geospatial Function Recognition

```cpp
// include/query/geo_function_registry.h (NEW)

class GeoFunctionRegistry {
  public:
    /// Register that "ST_Within" is a spatial predicate
    static void registerSpatialFunction(
        const std::string& funcName,
        SpatialFunctionType type,  // DISTANCE, CONTAINMENT, BOUNDS, UTILITY
        const std::vector<std::string>& paramTypes);  // geometry, number, bool
    
    /// Check if function is spatial
    static bool isSpatialFunction(const std::string& funcName);
    
    /// Get suggested indexes for a spatial predicate
    static std::vector<IndexType> suggestIndexes(
        const std::string& funcName,
        const std::vector<AQLExpression*>& args);
};
```

Registry initialization:
```cpp
GeoFunctionRegistry::registerSpatialFunction("ST_Within", 
    SpatialFunctionType::CONTAINMENT, 
    {"geometry", "geometry"});
GeoFunctionRegistry::registerSpatialFunction("ST_Distance",
    SpatialFunctionType::DISTANCE,
    {"geometry", "geometry"});
// ... etc. for all 14 ST_* functions
```

### 4.2 Query Optimizer Recognition

Optimizer must recognize spatial predicates and suggest indexes:

```cpp
// include/query/geo_query_optimizer.h (NEW)

class GeoQueryOptimizer {
  public:
    /// Analyze query for spatial predicates
    /// @param node AQL query AST
    /// @return list of GeoOptimizationHint objects
    std::vector<GeoOptimizationHint> analyzeSpatialPredicates(
        const AQLNode* node);
    
    /// Suggest indexes for spatial query
    /// @param predicate FILTER node with ST_* function
    /// @return IndexType::GEO or nullptr
    IndexType* suggestSpatialIndex(const FunctionCallNode* predicate);
};

struct GeoOptimizationHint {
    std::string functionName;        // "ST_Within"
    std::string affectedField;       // "location" (the geometry field)
    bool hasIndexAvailable;          // true if GEO index on field
    double estimatedSelectivity;     // 0.1 = returns ~10% of docs
};
```

### 4.3 Index Selection Rules

```
If FILTER contains: ST_Within(doc.location, polygon)
   ├─ Check if GEO index exists on "location"
   ├─ If YES → use index (spatial scan instead of full table scan)
   └─ If NO → full table scan (still works, just slower)

If FILTER contains: ST_Distance(doc.loc, ref) <= 1000
   ├─ Rewrite to: ST_DWithin(doc.loc, ref, 1000)
   ├─ Check for GEO index
   └─ Use distance index scan if available
```

---

## 5. Integration Points

### 5.1 Parser Integration
- `AQLParser` — already handles ST_* via generic function call path
- `LetEvaluator` — evaluates ST_* in LET/FILTER/SORT contexts
- No changes to `aql_parser.cpp` required

### 5.2 Query Engine Integration
- `qe_evalFunction()` in `query_engine.cpp` (lines 1676–2330) — routes ST_* calls
- Function registry lookup: O(1) dispatch to ST_Within, ST_Distance, etc.
- No changes to `query_engine.cpp` required

### 5.3 Query Optimizer Integration
- `QueryOptimizer` must recognize spatial predicates
- New `GeoQueryOptimizer` class provides hints
- Optimizer uses hints to select spatial indexes

### 5.4 Index Selection Integration
- `GeospatialIndexSelector` (new) evaluates GEO index readiness
- Planner chooses spatial index scan vs. full table scan
- Cost model includes selectivity hints from GeoQueryOptimizer

---

## 6. Performance Targets (Phase 5 Gates)

Geospatial queries MUST meet these performance targets:

| Operation | Target | Basis |
|-----------|--------|-------|
| ST_Distance (2 points) | ≤ 0.1ms | haversine calculation, no I/O |
| ST_Within (1 point, polygon) | ≤ 0.5ms | point-in-polygon test |
| FILTER ST_Distance (batch 100 points) | ≤ 50ms | 0.5ms per point |
| FILTER ST_Distance (batch 100K points) | ≤ 100ms | >= 50x speedup vs. naive O(n²) |
| ST_Distance batch with GEO index | >= 50x speedup | vs. sequential scan baseline |
| Memory usage (1M points) | < 500MB | index + query state |

---

## 7. Error Handling

All ST_* functions must validate input and return null on error:

```cpp
/// Error Cases

ST_Distance(null, point)          → null (null propagation)
ST_Distance("invalid", point)     → error(ERR_INVALID_GEO_TYPE)
ST_Distance(point_outside_wgs84)  → error(ERR_INVALID_COORDINATES)

ST_Within(point, "not_polygon")   → error(ERR_INVALID_GEOMETRY)
ST_Within(point_nan, polygon)     → null (NaN detection)

// Malformed GeoJSON
ST_GeomFromGeoJSON("{invalid}")   → null (parse error)
ST_GeomFromGeoJSON("not_json")    → error(ERR_INVALID_JSON)
```

---

## 8. Testing Requirements (Phase 4)

Phase 2 implementation must include:

### Parser Acceptance Tests (27 existing + 0 new)
- Existing test file: `tests/aql/test_aql_st_predicates.cpp`
- Status: COMPLETE (2026-07-27)
- Coverage: FILTER/SORT/RETURN contexts, nested ST_* calls

### Geospatial Computation Tests (20+)
- ST_Distance accuracy vs. reference (PostGIS)
- ST_Within edge cases (antimeridian, poles, boundary points)
- ST_Intersects with complex polygons

### Optimizer Tests (15+)
- Spatial predicate recognition
- Index hint suggestions
- Cost model estimates

### Integration Tests (30+)
- Query with spatial index → returns correct results
- Query without spatial index → returns correct results (slower)
- Mixed spatial + non-spatial predicates
- Concurrent spatial queries

---

## 9. Test Coverage Matrix

```
                          Parser  Eval  Optimizer  Index  Integration
                          ------  ----  ---------  -----  -----------
ST_Distance               ✓       ✓     ✓          ✓      ✓
ST_DWithin               ✓       ✓     ✓          ✓      ✓
ST_Contains              ✓       ✓     ✓          ✓      ✓
ST_Within                ✓       ✓     ✓          ✓      ✓
ST_Intersects            ✓       ✓     ✓          ✓      ✓
ST_Area                  ✓       ✓     -          -      ✓
ST_Centroid              ✓       ✓     -          -      ✓
ST_Boundary              ✓       ✓     -          -      ✓
ST_Length                ✓       ✓     -          -      ✓
ST_Point                 ✓       ✓     -          -      ✓
ST_AsGeoJSON             ✓       ✓     -          -      ✓
ST_GeomFromGeoJSON       ✓       ✓     -          -      ✓
ST_IsValid               ✓       ✓     -          -      ✓
ST_GeometryType          ✓       ✓     -          -      ✓
```

---

## 10. Dependency Tree

```
Geospatial Phase 1-6
├── Phase 1: Design / API Contract (THIS DOCUMENT) ✓
├── Phase 2: Core Implementation (Weeks 3-11, parallel)
│   ├── Week 3-4: Parser integration (wire existing ST_* into FILTER/SORT/RETURN)
│   │   └── Output: 27 existing tests PASS
│   ├── Week 5-6: Query optimization (recognize spatial predicates)
│   │   └── Files: geo_query_optimizer.cpp, geo_function_registry.cpp
│   ├── Week 7-8: Performance hardening (optional CUDA, batch evaluation)
│   │   └── Files: geo_kernel.cu (optional), vectorized evaluation
│   └── Week 9-11: Testing + documentation
│       └── Benchmarks, user guide (docs/de/aql/aql_geospatial_guide.md)
├── Phase 3: Error Handling (Week 17-18)
├── Phase 4: Tests (1000+ total, incremental)
├── Phase 5: Performance Hardening (>= 50x speedup gates)
└── Phase 6: Documentation & Acceptance
```

**No blocking dependencies** — can run in parallel with Mutations and DDL.

---

## 11. Acceptance Criteria

Phase 2 implementation is COMPLETE when:

- [ ] All 14 ST_* functions work in FILTER/SORT/RETURN contexts
- [ ] Parser recognizes spatial predicates (GeoFunctionRegistry)
- [ ] Query optimizer generates GeoOptimizationHint for spatial queries
- [ ] Spatial index selection works (prefer GEO index when available)
- [ ] 27 geospatial parser tests PASS
- [ ] 15+ optimizer tests PASS
- [ ] 30+ integration tests PASS
- [ ] Performance: ST_Distance on 100K points in < 100ms (>= 50x speedup)
- [ ] Zero new CRITICAL scanner findings
- [ ] Doxygen 100% on geo_*.cpp/h files

---

## 12. Approval & Sign-Off

This contract MUST be approved by:

- [ ] **Query Engine Lead**: qe_evalFunction() integration verified
- [ ] **Parser Lead**: Confirms existing function call path is used
- [ ] **Query Optimizer Lead**: Spatial predicate recognition implementation
- [ ] **Index Manager Lead**: Spatial index selection integration
- [ ] **PM**: Geospatial track can proceed in parallel

**Approval Status**: PENDING SIGN-OFF

**Approved By**: _______________  
**Date**: _______________  
**Changes Since Frozen**: None (this is the frozen version)

---

## 13. Amendments Log

| Date | Amendment | Approved By |
|------|-----------|-------------|
| 2026-08-06 | Initial contract created | PENDING |

(Amendments require explicit written approval and update to this section)

